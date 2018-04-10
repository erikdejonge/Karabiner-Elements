#pragma once

#include "cf_utility.hpp"
#include "input_source_utility.hpp"
#include "logger.hpp"
#include "types.hpp"
#include <Carbon/Carbon.h>

namespace krbn {
class input_source_manager final {
public:
  input_source_manager(void) {
    logger::get_logger().info("input_source_manager initialize");

    CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                    this,
                                    static_enabled_input_sources_changed_callback,
                                    kTISNotifyEnabledKeyboardInputSourcesChanged,
                                    nullptr,
                                    CFNotificationSuspensionBehaviorDeliverImmediately);

    enabled_input_sources_changed_callback();
  }

  ~input_source_manager(void) {
    logger::get_logger().info("input_source_manager terminate");

    CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(),
                                       this,
                                       kTISNotifyEnabledKeyboardInputSourcesChanged,
                                       nullptr);
  }

  bool select(const input_source_selector& input_source_selector) {
    for (const auto& e : entries_) {
      if (input_source_selector.test(e->get_input_source_identifiers())) {
        TISSelectInputSource(e->get_tis_input_source_ref());
        return true;
      }
    }

    return false;
  }

private:
  class entry final {
  public:
    entry(const entry&) = delete;

    entry(TISInputSourceRef tis_input_source_ref) : input_source_identifiers_(tis_input_source_ref),
                                                    tis_input_source_ref_(tis_input_source_ref) {
      if (tis_input_source_ref_) {
        CFRetain(tis_input_source_ref_);
      }
    }

    ~entry(void) {
      if (tis_input_source_ref_) {
        CFRelease(tis_input_source_ref_);
      }
    }

    const input_source_identifiers& get_input_source_identifiers(void) const {
      return input_source_identifiers_;
    }

    TISInputSourceRef get_tis_input_source_ref(void) const {
      return tis_input_source_ref_;
    }

  private:
    input_source_identifiers input_source_identifiers_;
    TISInputSourceRef tis_input_source_ref_;
  };

  static void static_enabled_input_sources_changed_callback(CFNotificationCenterRef center,
                                                            void* observer,
                                                            CFStringRef notification_name,
                                                            const void* observed_object,
                                                            CFDictionaryRef user_info) {
    auto self = static_cast<input_source_manager*>(observer);
    if (self) {
      self->enabled_input_sources_changed_callback();
    }
  }

  void enabled_input_sources_changed_callback(void) {
    entries_.clear();

    if (auto properties = cf_utility::create_cfmutabledictionary()) {
      CFDictionarySetValue(properties, kTISPropertyInputSourceIsSelectCapable, kCFBooleanTrue);
      CFDictionarySetValue(properties, kTISPropertyInputSourceCategory, kTISCategoryKeyboardInputSource);

      if (auto input_sources = TISCreateInputSourceList(properties, false)) {
        for (CFIndex i = 0;; ++i) {
          auto s = cf_utility::get_value<TISInputSourceRef>(input_sources, i);
          if (!s) {
            break;
          }

          entries_.push_back(std::make_unique<entry>(s));
        }

        CFRelease(input_sources);
      }

      CFRelease(properties);
    }
  }

  std::vector<std::unique_ptr<entry>> entries_;
};
} // namespace krbn
