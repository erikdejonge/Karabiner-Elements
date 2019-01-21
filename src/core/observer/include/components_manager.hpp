#pragma once

// `krbn::components_manager` can be used safely in a multi-threaded environment.

#include "constants.hpp"
#include "device_observer.hpp"
#include "grabber_client.hpp"
#include "monitor/version_monitor_utility.hpp"
#include <pqrs/dispatcher.hpp>

namespace krbn {
class components_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  components_manager(const components_manager&) = delete;

  components_manager(void) : dispatcher_client() {
    version_monitor_ = version_monitor_utility::make_version_monitor_stops_main_run_loop_when_version_changed();

    async_start_grabber_client();
  }

  virtual ~components_manager(void) {
    detach_from_dispatcher([this] {
      stop_grabber_client();
      stop_device_observer();

      version_monitor_ = nullptr;
    });
  }

private:
  void async_start_grabber_client(void) {
    enqueue_to_dispatcher([this] {
      if (grabber_client_) {
        return;
      }

      grabber_client_ = std::make_shared<grabber_client>();

      grabber_client_->connected.connect([this] {
        if (version_monitor_) {
          version_monitor_->async_manual_check();
        }

        start_device_observer();
      });

      grabber_client_->connect_failed.connect([this](auto&& error_code) {
        if (version_monitor_) {
          version_monitor_->async_manual_check();
        }

        stop_device_observer();
      });

      grabber_client_->closed.connect([this] {
        if (version_monitor_) {
          version_monitor_->async_manual_check();
        }

        stop_device_observer();
      });

      grabber_client_->async_start();
    });
  }

  void async_stop_grabber_client(void) {
    enqueue_to_dispatcher([this] {
      stop_grabber_client();
    });
  }

  void stop_grabber_client(void) {
    if (!grabber_client_) {
      return;
    }

    grabber_client_ = nullptr;
  }

  void start_device_observer(void) {
    if (device_observer_) {
      return;
    }

    device_observer_ = std::make_shared<device_observer>(grabber_client_);
  }

  void stop_device_observer(void) {
    if (!device_observer_) {
      return;
    }

    device_observer_ = nullptr;
  }

  std::shared_ptr<version_monitor> version_monitor_;
  std::shared_ptr<grabber_client> grabber_client_;
  std::shared_ptr<device_observer> device_observer_;
};
} // namespace krbn
