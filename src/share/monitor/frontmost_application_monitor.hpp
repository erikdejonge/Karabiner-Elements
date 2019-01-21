#pragma once

// `krbn::frontmost_application_monitor` can be used safely in a multi-threaded environment.

#include "frontmost_application_monitor_objc.h"
#include "logger.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace krbn {
class frontmost_application_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(const std::string& bundle_identifier, const std::string& file_path)> frontmost_application_changed;

  // Methods

  frontmost_application_monitor(void) : dispatcher_client(),
                                        monitor_(nullptr) {
  }

  virtual ~frontmost_application_monitor(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (monitor_) {
        logger::get_logger().warn("frontmost_application_monitor is already started.");
        return;
      }

      krbn_frontmost_application_monitor_initialize(&monitor_,
                                                    static_cpp_callback,
                                                    this);

      logger::get_logger().info("frontmost_application_monitor is started.");
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  void stop(void) {
    if (!monitor_) {
      return;
    }

    krbn_frontmost_application_monitor_terminate(&monitor_);
    monitor_ = nullptr;

    logger::get_logger().info("frontmost_application_monitor is stopped.");
  }

  static void static_cpp_callback(const char* bundle_identifier,
                                  const char* file_path,
                                  void* context) {
    auto monitor = reinterpret_cast<frontmost_application_monitor*>(context);
    if (monitor) {
      std::string bundle_identifier_string;
      if (bundle_identifier) {
        bundle_identifier_string = bundle_identifier;
      }

      std::string file_path_string;
      if (file_path) {
        file_path_string = file_path;
      }

      monitor->cpp_callback(bundle_identifier_string, file_path_string);
    }
  }

  void cpp_callback(const std::string& bundle_identifier,
                    const std::string& file_path) {
    enqueue_to_dispatcher([this, bundle_identifier, file_path] {
      frontmost_application_changed(bundle_identifier, file_path);
    });
  }

  krbn_frontmost_application_monitor_objc* monitor_;
};
} // namespace krbn
