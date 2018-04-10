#pragma once

#include "cf_utility.hpp"
#include "input_source_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <Carbon/Carbon.h>

namespace krbn {
class input_source_observer final {
public:
  typedef std::function<void(const input_source_identifiers& input_source_identifiers)> callback;

  input_source_observer(const callback& callback) : callback_(callback) {
    logger::get_logger().info("input_source_observer initialize");

    CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                    this,
                                    static_input_source_changed_callback,
                                    kTISNotifySelectedKeyboardInputSourceChanged,
                                    nullptr,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);

    input_source_changed_callback();
  }

  ~input_source_observer(void) {
    logger::get_logger().info("input_source_observer terminate");

    CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(),
                                       this,
                                       kTISNotifySelectedKeyboardInputSourceChanged,
                                       nullptr);
  }

private:
  static void static_input_source_changed_callback(CFNotificationCenterRef center,
                                                   void* observer,
                                                   CFStringRef notification_name,
                                                   const void* observed_object,
                                                   CFDictionaryRef user_info) {
    auto self = static_cast<input_source_observer*>(observer);
    if (self) {
      self->input_source_changed_callback();
    }
  }

  void input_source_changed_callback(void) {
    if (auto p = TISCopyCurrentKeyboardInputSource()) {
      if (callback_) {
        callback_(input_source_identifiers(p));
      }

      CFRelease(p);
    }
  }

  callback callback_;
};
} // namespace krbn
