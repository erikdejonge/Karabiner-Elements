#pragma once

#include "boost_defs.hpp"

#include "apple_hid_usage_tables.hpp"
#include "gcd_utility.hpp"
#include "iokit_utility.hpp"
#include "spdlog_utility.hpp"
#include "types.hpp"
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hid/IOHIDValue.h>
#include <boost/optional.hpp>
#include <cstdint>
#include <functional>
#include <list>
#include <mach/mach_time.h>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <vector>

class human_interface_device final {
public:
  typedef std::function<void(human_interface_device& device,
                             IOHIDValueRef _Nonnull value,
                             IOHIDElementRef _Nonnull element,
                             uint32_t usage_page,
                             uint32_t usage,
                             CFIndex integer_value)>
      value_callback;

  typedef std::function<void(human_interface_device& device,
                             IOHIDReportType type,
                             uint32_t report_id,
                             uint8_t* _Nonnull report,
                             CFIndex report_length)>
      report_callback;

  typedef std::function<bool(human_interface_device& device)> is_grabbable_callback;

  human_interface_device(const human_interface_device&) = delete;

  human_interface_device(spdlog::logger& logger,
                         IOHIDDeviceRef _Nonnull device) : logger_(logger),
                                                           device_(device),
                                                           registry_entry_id_(0),
                                                           queue_(nullptr),
                                                           is_grabbable_log_reducer_(logger) {
    // ----------------------------------------
    // retain device_

    CFRetain(device_);

    if (auto registry_entry_id = iokit_utility::get_registry_entry_id(device_)) {
      registry_entry_id_ = *registry_entry_id;
    } else {
      logger_.error("iokit_utility::get_registry_entry_id error @ {0}", __PRETTY_FUNCTION__);
    }

    // ----------------------------------------
    // setup elements_

    if (auto elements = IOHIDDeviceCopyMatchingElements(device_, nullptr, kIOHIDOptionsTypeNone)) {
      for (CFIndex i = 0; i < CFArrayGetCount(elements); ++i) {
        // Add to elements_.
        auto element = static_cast<IOHIDElementRef>(const_cast<void*>(CFArrayGetValueAtIndex(elements, i)));
        auto usage_page = IOHIDElementGetUsagePage(element);
        auto usage = IOHIDElementGetUsage(element);

        auto key = elements_key(usage_page, usage);
        if (elements_.find(key) == elements_.end()) {
          CFRetain(element);
          elements_[key] = element;
        }
      }
      CFRelease(elements);
    }

    // ----------------------------------------
    // setup queue_

    const CFIndex depth = 1024;
    queue_ = IOHIDQueueCreate(kCFAllocatorDefault, device_, depth, kIOHIDOptionsTypeNone);
    if (!queue_) {
      logger_.error("IOHIDQueueCreate error @ {0}", __PRETTY_FUNCTION__);
    } else {
      // Add elements into queue_.
      for (const auto& it : elements_) {
        IOHIDQueueAddElement(queue_, it.second);
      }
      IOHIDQueueRegisterValueAvailableCallback(queue_, static_queue_value_available_callback, this);
    }
  }

  ~human_interface_device(void) {
    // Release device_ and queue_ in main thread to avoid callback invocations after object has been destroyed.
    gcd_utility::dispatch_sync_in_main_queue(^{
      // Unregister all callbacks.
      unschedule();
      unregister_report_callback();
      unregister_value_callback();
      close();

      // ----------------------------------------
      // release queue_

      if (queue_) {
        CFRelease(queue_);
        queue_ = nullptr;
      }

      // ----------------------------------------
      // release elements_

      for (const auto& it : elements_) {
        CFRelease(it.second);
      }
      elements_.clear();

      // ----------------------------------------
      // release device_

      CFRelease(device_);
    });
  }

  uint64_t get_registry_entry_id(void) const { return registry_entry_id_; }

  IOReturn open(IOOptionBits options = kIOHIDOptionsTypeNone) {
    return IOHIDDeviceOpen(device_, options);
  }

  IOReturn close(void) {
    return IOHIDDeviceClose(device_, kIOHIDOptionsTypeNone);
  }

  void schedule(void) {
    IOHIDDeviceScheduleWithRunLoop(device_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    if (queue_) {
      IOHIDQueueScheduleWithRunLoop(queue_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    }
  }

  void unschedule(void) {
    if (queue_) {
      IOHIDQueueUnscheduleFromRunLoop(queue_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
    }
    IOHIDDeviceUnscheduleFromRunLoop(device_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
  }

  void register_report_callback(const report_callback& callback) {
    report_callback_ = callback;

    resize_report_buffer();
    IOHIDDeviceRegisterInputReportCallback(device_, &(report_buffer_[0]), report_buffer_.size(), static_input_report_callback, this);
  }

  void unregister_report_callback(void) {
    resize_report_buffer();
    IOHIDDeviceRegisterInputReportCallback(device_, &(report_buffer_[0]), report_buffer_.size(), nullptr, nullptr);

    report_callback_ = nullptr;
  }

  void register_value_callback(const value_callback& callback) {
    value_callback_ = callback;
    if (queue_) {
      IOHIDQueueStart(queue_);
    }
  }

  void unregister_value_callback(void) {
    if (queue_) {
      IOHIDQueueStop(queue_);
    }
    value_callback_ = nullptr;
  }

  // High-level utility method.
  void observe(const value_callback& callback) {
    auto r = open();
    if (r != kIOReturnSuccess) {
      logger_.error("IOHIDDeviceOpen error: {1} @ {0}", __PRETTY_FUNCTION__, r);
      return;
    }

    register_value_callback(callback);
    schedule();
  }

  // High-level utility method.
  void unobserve(void) {
    unschedule();
    unregister_value_callback();
    close();
  }

  // High-level utility method.
  void grab(const value_callback& callback) {
    auto r = open(kIOHIDOptionsTypeSeizeDevice);
    if (r != kIOReturnSuccess) {
      logger_.error("IOHIDDeviceOpen error: {1} @ {0}", __PRETTY_FUNCTION__, r);
      return;
    }

    register_value_callback(callback);
    schedule();
  }

  // High-level utility method.
  void ungrab(void) {
    unschedule();
    unregister_value_callback();
    close();
  }

  boost::optional<long> get_max_input_report_size(void) const {
    return iokit_utility::get_max_input_report_size(device_);
  }

  boost::optional<long> get_vendor_id(void) const {
    return iokit_utility::get_vendor_id(device_);
  }

  boost::optional<long> get_product_id(void) const {
    return iokit_utility::get_product_id(device_);
  }

  boost::optional<long> get_location_id(void) const {
    return iokit_utility::get_location_id(device_);
  }

  boost::optional<std::string> get_manufacturer(void) const {
    return iokit_utility::get_manufacturer(device_);
  }

  boost::optional<std::string> get_product(void) const {
    return iokit_utility::get_product(device_);
  }

  boost::optional<std::string> get_serial_number(void) const {
    return iokit_utility::get_serial_number(device_);
  }

  boost::optional<std::string> get_transport(void) const {
    return iokit_utility::get_transport(device_);
  }

  std::string get_name_for_log(void) const {
    if (auto product_name = get_product()) {
      return *product_name;
    }
    if (auto vendor_id = get_vendor_id()) {
      if (auto product_id = get_product_id()) {
        std::stringstream stream;
        stream << std::hex
               << "(vendor_id:0x" << *vendor_id
               << ", product_id:0x" << *product_id
               << ")"
               << std::dec;
        return stream.str();
      }
    }

    std::stringstream stream;
    stream << "(registry_entry_id:" << registry_entry_id_ << ")";
    return stream.str();
  }

  IOHIDElementRef _Nullable get_element(uint32_t usage_page, uint32_t usage) const {
    auto key = elements_key(usage_page, usage);
    auto it = elements_.find(key);
    if (it == elements_.end()) {
      return nullptr;
    } else {
      return it->second;
    }
  }

  size_t get_pressed_keys_count(void) const {
    return pressed_key_usages_.size();
  }

  void clear_pressed_keys(void) {
    pressed_key_usages_.clear();
  }

  void set_is_grabbable_callback(const is_grabbable_callback& callback) {
    is_grabbable_callback_ = callback;
  }

  bool is_grabbable(void) {
    if (repeating_key_) {
      // We should not grab the device while a key is repeating since we cannot stop the key repeating.
      // (To stop the key repeating, we have to send a hid report to the device. But we cannot do it.)

      is_grabbable_log_reducer_.warn(std::string("We cannot grab ") + get_name_for_log() + " while a key is repeating.");
      return false;
    }

    if (!pressed_buttons_.empty()) {
      // We should not grab the device while a button is pressed since we cannot release the button.
      // (To release the button, we have to send a hid report to the device. But we cannot do it.)

      is_grabbable_log_reducer_.warn(std::string("We cannot grab ") + get_name_for_log() + " while mouse buttons are pressed.");
      return false;
    }

    if (is_grabbable_callback_) {
      if (!is_grabbable_callback_(*this)) {
        return false;
      }
    }

    return true;
  }

#pragma mark - usage specific utilities

  // This method requires root privilege to use IOHIDDeviceGetValue for kHIDPage_LEDs usage.
  boost::optional<krbn::led_state> get_caps_lock_led_state(void) const {
    if (auto element = get_element(kHIDPage_LEDs, kHIDUsage_LED_CapsLock)) {
      auto max = IOHIDElementGetLogicalMax(element);

      IOHIDValueRef value;
      auto r = IOHIDDeviceGetValue(device_, element, &value);
      if (r != kIOReturnSuccess) {
        logger_.error("IOHIDDeviceGetValue error: {1} @ {0}", __PRETTY_FUNCTION__, r);
        return boost::none;
      }

      auto integer_value = IOHIDValueGetIntegerValue(value);
      if (integer_value == max) {
        return krbn::led_state::on;
      } else {
        return krbn::led_state::off;
      }
    }

    return boost::none;
  }

  // This method requires root privilege to use IOHIDDeviceSetValue for kHIDPage_LEDs usage.
  IOReturn set_caps_lock_led_state(krbn::led_state state) {
    if (auto element = get_element(kHIDPage_LEDs, kHIDUsage_LED_CapsLock)) {
      CFIndex integer_value = 0;
      if (state == krbn::led_state::on) {
        integer_value = IOHIDElementGetLogicalMax(element);
      } else {
        integer_value = IOHIDElementGetLogicalMin(element);
      }

      if (auto value = IOHIDValueCreateWithIntegerValue(kCFAllocatorDefault, element, mach_absolute_time(), integer_value)) {
        auto r = IOHIDDeviceSetValue(device_, element, value);
        CFRelease(value);
        return r;
      } else {
        logger_.error("IOHIDValueCreateWithIntegerValue error @ {0}", __PRETTY_FUNCTION__);
      }
    }

    return kIOReturnError;
  }

  bool is_keyboard(void) {
    return IOHIDDeviceConformsTo(device_, kHIDPage_GenericDesktop, kHIDUsage_GD_Keyboard);
  }

  bool is_pointing_device(void) {
    return IOHIDDeviceConformsTo(device_, kHIDPage_GenericDesktop, kHIDUsage_GD_Pointer) ||
           IOHIDDeviceConformsTo(device_, kHIDPage_GenericDesktop, kHIDUsage_GD_Mouse);
  }

private:
  static void static_queue_value_available_callback(void* _Nullable context, IOReturn result, void* _Nullable sender) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<human_interface_device*>(context);
    if (!self) {
      return;
    }

    auto queue = static_cast<IOHIDQueueRef>(sender);
    if (!queue) {
      return;
    }

    self->queue_value_available_callback(queue);
  }

  void queue_value_available_callback(IOHIDQueueRef _Nonnull queue) {
    while (true) {
      auto value = IOHIDQueueCopyNextValueWithTimeout(queue, 0.);
      if (!value) {
        break;
      }

      auto element = IOHIDValueGetElement(value);
      if (element) {
        auto usage_page = IOHIDElementGetUsagePage(element);
        auto usage = IOHIDElementGetUsage(element);
        auto integer_value = IOHIDValueGetIntegerValue(value);

        // Update repeating_key_
        if (auto key_code = krbn::types::get_key_code(usage_page, usage)) {
          bool pressed = integer_value;
          if (pressed) {
            if (krbn::types::get_modifier_flag(*key_code) != krbn::modifier_flag::zero) {
              // The pressed key is a modifier key.
              repeating_key_ = boost::none;
            } else {
              repeating_key_ = *key_code;
            }
          } else {
            if (repeating_key_ && *repeating_key_ == *key_code) {
              repeating_key_ = boost::none;
            }
          }
        }

        // Update pressed_buttons_
        if (auto pointing_button = krbn::types::get_pointing_button(usage_page, usage)) {
          bool pressed = integer_value;
          if (pressed) {
            pressed_buttons_.push_back(*pointing_button);
          } else {
            pressed_buttons_.remove(*pointing_button);
          }
        }

        // Update pressed_key_usages_.
        if ((usage_page == kHIDPage_KeyboardOrKeypad) ||
            (usage_page == kHIDPage_AppleVendorTopCase && usage == kHIDUsage_AV_TopCase_KeyboardFn) ||
            (usage_page == kHIDPage_Button)) {
          bool pressed = integer_value;
          uint64_t u = (static_cast<uint64_t>(usage_page) << 32) | usage;
          if (pressed) {
            pressed_key_usages_.push_back(u);
          } else {
            pressed_key_usages_.remove(u);
          }
        }

        // Call value_callback_.
        if (value_callback_) {
          value_callback_(*this, value, element, usage_page, usage, integer_value);
        }
      }

      CFRelease(value);
    }
  }

  static void static_input_report_callback(void* _Nullable context,
                                           IOReturn result,
                                           void* _Nullable sender,
                                           IOHIDReportType type,
                                           uint32_t report_id,
                                           uint8_t* _Nullable report,
                                           CFIndex report_length) {
    if (result != kIOReturnSuccess) {
      return;
    }

    auto self = static_cast<human_interface_device*>(context);
    if (!self) {
      return;
    }

    self->input_report_callback(type, report_id, report, report_length);
  }

  void input_report_callback(IOHIDReportType type,
                             uint32_t report_id,
                             uint8_t* _Nullable report,
                             CFIndex report_length) {
    if (report_callback_) {
      report_callback_(*this, type, report_id, report, report_length);
    }
  }

  uint64_t elements_key(uint32_t usage_page, uint32_t usage) const {
    return ((static_cast<uint64_t>(usage_page) << 32) | usage);
  }

  void resize_report_buffer(void) {
    size_t buffer_size = 32; // use this provisional value if we cannot get max input report size from device.
    if (auto size = get_max_input_report_size()) {
      buffer_size = *size;
    }

    report_buffer_.resize(buffer_size);
  }

  spdlog::logger& logger_;

  IOHIDDeviceRef _Nonnull device_;
  uint64_t registry_entry_id_;
  IOHIDQueueRef _Nullable queue_;
  std::unordered_map<uint64_t, IOHIDElementRef> elements_;

  boost::optional<krbn::key_code> repeating_key_;
  std::list<krbn::pointing_button> pressed_buttons_;
  std::list<uint64_t> pressed_key_usages_;
  value_callback value_callback_;
  report_callback report_callback_;
  std::vector<uint8_t> report_buffer_;

  is_grabbable_callback is_grabbable_callback_;
  spdlog_utility::log_reducer is_grabbable_log_reducer_;
};
