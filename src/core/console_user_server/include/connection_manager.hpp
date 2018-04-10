#pragma once

#include "apple_notification_center.hpp"
#include "configuration_manager.hpp"
#include "constants.hpp"
#include "frontmost_application_observer.hpp"
#include "gcd_utility.hpp"
#include "grabber_client.hpp"
#include "input_source_observer.hpp"
#include "logger.hpp"
#include "receiver.hpp"
#include "session.hpp"
#include "system_preferences_monitor.hpp"
#include "version_monitor.hpp"
#include <thread>

namespace krbn {
class connection_manager final {
public:
  connection_manager(const connection_manager&) = delete;

  connection_manager(version_monitor& version_monitor) : version_monitor_(version_monitor) {
    apple_notification_center::observe_distributed_notification(this,
                                                                static_grabber_is_launched_callback,
                                                                constants::get_distributed_notification_grabber_is_launched());

    auto current_uid = getuid();

    timer_ = std::make_unique<gcd_utility::main_queue_timer>(
        dispatch_time(DISPATCH_TIME_NOW, 1 * NSEC_PER_SEC),
        1 * NSEC_PER_SEC,
        0,
        ^{
          if (auto uid = session::get_current_console_user_id()) {
            if (current_uid == *uid) {
              try {
                if (!receiver_) {
                  receiver_ = std::make_unique<receiver>();
                }

                if (!grabber_client_) {
                  version_monitor_.manual_check();

                  grabber_client_ = std::make_unique<grabber_client>();
                  grabber_client_->connect();
                  logger::get_logger().info("grabber_client_ is connected");
                }

                if (!configuration_manager_) {
                  configuration_manager_ = std::make_unique<configuration_manager>();
                }

                if (!system_preferences_monitor_) {
                  system_preferences_monitor_ = std::make_unique<system_preferences_monitor>(
                      std::bind(&connection_manager::system_preferences_updated_callback, this, std::placeholders::_1),
                      configuration_manager_->get_configuration_monitor());
                }

                if (!frontmost_application_observer_) {
                  frontmost_application_observer_ = std::make_unique<frontmost_application_observer>(
                      std::bind(&connection_manager::frontmost_application_changed_callback, this, std::placeholders::_1, std::placeholders::_2));
                }

                if (!input_source_observer_) {
                  input_source_observer_ = std::make_unique<input_source_observer>(
                      std::bind(&connection_manager::input_source_changed_callback, this, std::placeholders::_1));
                }

                return;

              } catch (std::exception& ex) {
                logger::get_logger().warn(ex.what());
              }
            }
          }

          release();
        });
  }

  ~connection_manager(void) {
    timer_ = nullptr;

    gcd_utility::dispatch_sync_in_main_queue(^{
      release();
    });

    apple_notification_center::unobserve_distributed_notification(this,
                                                                  constants::get_distributed_notification_grabber_is_launched());
  }

private:
  void release(void) {
    input_source_observer_ = nullptr;
    frontmost_application_observer_ = nullptr;
    system_preferences_monitor_ = nullptr;
    configuration_manager_ = nullptr;
    grabber_client_ = nullptr;
    receiver_ = nullptr;
  }

  static void static_grabber_is_launched_callback(CFNotificationCenterRef center,
                                                  void* observer,
                                                  CFStringRef notification_name,
                                                  const void* observed_object,
                                                  CFDictionaryRef user_info) {
    auto self = static_cast<connection_manager*>(observer);
    self->grabber_is_launched_callback();
  }

  void grabber_is_launched_callback(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      logger::get_logger().info("connection_manager::grabber_is_launched_callback");
      release();
    });
  }

  void system_preferences_updated_callback(const system_preferences& system_preferences) {
    if (grabber_client_) {
      grabber_client_->system_preferences_updated(system_preferences);
    }
  }

  void frontmost_application_changed_callback(const std::string& bundle_identifier,
                                              const std::string& file_path) {
    if (bundle_identifier == "org.pqrs.Karabiner.EventViewer" ||
        bundle_identifier == "org.pqrs.Karabiner-EventViewer") {
      return;
    }

    if (grabber_client_) {
      grabber_client_->frontmost_application_changed(bundle_identifier, file_path);
    }
  }

  void input_source_changed_callback(const input_source_identifiers& input_source_identifiers) {
    if (grabber_client_) {
      grabber_client_->input_source_changed(input_source_identifiers);
    }
  }

  version_monitor& version_monitor_;

  std::unique_ptr<gcd_utility::main_queue_timer> timer_;

  std::unique_ptr<receiver> receiver_;

  std::unique_ptr<configuration_manager> configuration_manager_;
  std::unique_ptr<grabber_client> grabber_client_;
  std::unique_ptr<system_preferences_monitor> system_preferences_monitor_;

  // `frontmost_application_observer` does not work properly in karabiner_grabber after fast user switching.
  // Therefore, we have to use `frontmost_application_observer` in `console_user_server`.
  std::unique_ptr<frontmost_application_observer> frontmost_application_observer_;

  std::unique_ptr<input_source_observer> input_source_observer_;
};
} // namespace krbn
