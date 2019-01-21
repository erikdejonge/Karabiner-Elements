#pragma once

// `krbn::local_datagram::client_manager` can be used safely in a multi-threaded environment.

#include "local_datagram/client.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>

namespace krbn {
namespace local_datagram {
class client_manager final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> connected;
  nod::signal<void(const boost::system::error_code&)> connect_failed;
  nod::signal<void(void)> closed;

  // Methods

  client_manager(const std::string& path,
                 std::optional<std::chrono::milliseconds> server_check_interval,
                 std::chrono::milliseconds reconnect_interval) : dispatcher_client(),
                                                                 path_(path),
                                                                 server_check_interval_(server_check_interval),
                                                                 reconnect_interval_(reconnect_interval),
                                                                 reconnect_enabled_(false) {
  }

  virtual ~client_manager(void) {
    detach_from_dispatcher([this] {
      stop();
    });
  }

  std::shared_ptr<client> get_client(void) {
    std::lock_guard<std::mutex> lock(client_mutex_);

    return client_;
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      reconnect_enabled_ = true;

      connect();
    });
  }

  void async_stop(void) {
    enqueue_to_dispatcher([this] {
      stop();
    });
  }

private:
  void stop(void) {
    // We have to unset reconnect_enabled_ before `close` to prevent `enqueue_reconnect` by `closed` signal.
    reconnect_enabled_ = false;

    close();
  }

  void connect(void) {
    if (client_) {
      return;
    }

    // Guard client_ for `get_client`.

    {
      std::lock_guard<std::mutex> lock(client_mutex_);
      client_ = std::make_shared<client>();

      client_->connected.connect([this] {
        enqueue_to_dispatcher([this] {
          connected();
        });
      });

      client_->connect_failed.connect([this](auto&& error_code) {
        enqueue_to_dispatcher([this, error_code] {
          connect_failed(error_code);
        });

        close();
        enqueue_reconnect();
      });

      client_->closed.connect([this] {
        enqueue_to_dispatcher([this] {
          closed();
        });

        close();
        enqueue_reconnect();
      });

      client_->async_connect(path_,
                             server_check_interval_);
    }
  }

  void close(void) {
    if (!client_) {
      return;
    }

    client_ = nullptr;
  }

  void enqueue_reconnect(void) {
    enqueue_to_dispatcher(
        [this] {
          if (!reconnect_enabled_) {
            return;
          }

          connect();
        },
        when_now() + reconnect_interval_);
  }

  std::string path_;
  std::optional<std::chrono::milliseconds> server_check_interval_;
  std::chrono::milliseconds reconnect_interval_;

  std::shared_ptr<client> client_;
  std::mutex client_mutex_;

  bool reconnect_enabled_;
};
} // namespace local_datagram
} // namespace krbn
