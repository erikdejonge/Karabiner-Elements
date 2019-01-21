#pragma once

// `krbn::console_user_id_monitor` can be used safely in a multi-threaded environment.

#include "logger.hpp"
#include "session.hpp"
#include <memory>
#include <nod/nod.hpp>
#include <optional>
#include <pqrs/dispatcher.hpp>

namespace krbn {
class console_user_id_monitor final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(std::optional<uid_t>)> console_user_id_changed;

  // Methods

  console_user_id_monitor(const console_user_id_monitor&) = delete;

  console_user_id_monitor(void) : dispatcher_client(),
                                  timer_(*this) {
  }

  virtual ~console_user_id_monitor(void) {
    detach_from_dispatcher([] {
    });
  }

  void async_start(void) {
    timer_.start(
        [this] {
          check();
        },
        std::chrono::milliseconds(1000));

    logger::get_logger().info("console_user_id_monitor is started.");
  }

  void async_stop(void) {
    timer_.stop();

    logger::get_logger().info("console_user_id_monitor is stopped.");
  }

private:
  // This method is executed in the dispatcher thread.
  void check(void) {
    auto u = session::get_current_console_user_id();
    if (!uid_ || *uid_ != u) {
      uid_ = std::make_unique<std::optional<uid_t>>(u);
      enqueue_to_dispatcher([this, u] {
        console_user_id_changed(u);
      });
    }
  }

  pqrs::dispatcher::extra::timer timer_;
  std::unique_ptr<std::optional<uid_t>> uid_;
};
} // namespace krbn
