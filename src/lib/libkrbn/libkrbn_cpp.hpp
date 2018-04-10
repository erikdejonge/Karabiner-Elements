#pragma once

#include "configuration_monitor.hpp"
#include "libkrbn.h"

class libkrbn_cpp final {
public:
  static krbn::device_identifiers make_device_identifiers(const libkrbn_device_identifiers& device_identifiers) {
    krbn::device_identifiers identifiers(krbn::vendor_id(device_identifiers.vendor_id),
                                         krbn::product_id(device_identifiers.product_id),
                                         device_identifiers.is_keyboard,
                                         device_identifiers.is_pointing_device);
    return identifiers;
  }

  class libkrbn_core_configuration_class final {
  public:
    libkrbn_core_configuration_class(std::shared_ptr<krbn::core_configuration> core_configuration) : core_configuration_(core_configuration) {
    }

    krbn::core_configuration& get_core_configuration(void) {
      return *core_configuration_;
    }

  private:
    std::shared_ptr<krbn::core_configuration> core_configuration_;
  };

  class libkrbn_configuration_monitor_class final {
  public:
    libkrbn_configuration_monitor_class(const libkrbn_configuration_monitor_class&) = delete;

    libkrbn_configuration_monitor_class(libkrbn_configuration_monitor_callback callback, void* refcon) : callback_(callback), refcon_(refcon) {
      configuration_monitor_ = std::make_shared<krbn::configuration_monitor>(
          krbn::constants::get_user_core_configuration_file_path(),
          [this](const std::shared_ptr<krbn::core_configuration> core_configuration) {
            if (callback_) {
              auto* p = new libkrbn_core_configuration_class(core_configuration);
              callback_(p, refcon_);
            }
          });
    }

    std::weak_ptr<krbn::configuration_monitor> get_configuration_monitor(void) const {
      return configuration_monitor_;
    }

  private:
    libkrbn_configuration_monitor_callback callback_;
    void* refcon_;

    std::shared_ptr<krbn::configuration_monitor> configuration_monitor_;
  };
};
