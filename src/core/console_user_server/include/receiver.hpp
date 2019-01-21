#pragma once

#include "console_user_server_client.hpp"
#include "constants.hpp"
#include "input_source_manager.hpp"
#include "local_datagram/server_manager.hpp"
#include "shell_utility.hpp"
#include "types.hpp"
#include <nod/nod.hpp>
#include <pqrs/dispatcher.hpp>
#include <vector>

namespace krbn {
class receiver final : public pqrs::dispatcher::extra::dispatcher_client {
public:
  // Signals (invoked from the shared dispatcher thread)

  nod::signal<void(void)> bound;
  nod::signal<void(const boost::system::error_code&)> bind_failed;
  nod::signal<void(void)> closed;

  // Methods

  receiver(const receiver&) = delete;

  receiver(void) : dispatcher_client(),
                   last_select_input_source_time_stamp_(0) {
  }

  virtual ~receiver(void) {
    detach_from_dispatcher([this] {
      server_manager_ = nullptr;
    });

    logger::get_logger().info("receiver is terminated");
  }

  void async_start(void) {
    enqueue_to_dispatcher([this] {
      if (server_manager_) {
        return;
      }

      auto uid = getuid();
      auto socket_file_path = console_user_server_client::make_console_user_server_socket_file_path(uid);

      unlink(socket_file_path.c_str());

      size_t buffer_size = 32 * 1024;
      std::chrono::milliseconds server_check_interval(3000);
      std::chrono::milliseconds reconnect_interval(1000);

      server_manager_ = std::make_unique<local_datagram::server_manager>(socket_file_path,
                                                                         buffer_size,
                                                                         server_check_interval,
                                                                         reconnect_interval);

      server_manager_->bound.connect([this] {
        enqueue_to_dispatcher([this] {
          bound();
        });
      });

      server_manager_->bind_failed.connect([this](auto&& error_code) {
        enqueue_to_dispatcher([this, error_code] {
          bind_failed(error_code);
        });
      });

      server_manager_->closed.connect([this] {
        enqueue_to_dispatcher([this] {
          closed();
        });
      });

      server_manager_->received.connect([this](auto&& buffer) {
        if (auto type = types::find_operation_type(*buffer)) {
          switch (*type) {
            case operation_type::shell_command_execution:
              if (buffer->size() != sizeof(operation_type_shell_command_execution_struct)) {
                logger::get_logger().error("invalid size for operation_type::shell_command_execution");
              } else {
                auto p = reinterpret_cast<operation_type_shell_command_execution_struct*>(&((*buffer)[0]));

                // Ensure shell_command is null-terminated string even if corrupted data is sent.
                p->shell_command[sizeof(p->shell_command) - 1] = '\0';

                std::string background_shell_command = shell_utility::make_background_command(p->shell_command);
                system(background_shell_command.c_str());
              }
              break;

            case operation_type::select_input_source:
              if (buffer->size() != sizeof(operation_type_select_input_source_struct)) {
                logger::get_logger().error("invalid size for operation_type::select_input_source");
              } else {
                auto p = reinterpret_cast<operation_type_select_input_source_struct*>(&((*buffer)[0]));

                // Ensure input_source_selector's strings are null-terminated string even if corrupted data is sent.
                p->language[sizeof(p->language) - 1] = '\0';
                p->input_source_id[sizeof(p->input_source_id) - 1] = '\0';
                p->input_mode_id[sizeof(p->input_mode_id) - 1] = '\0';

                auto time_stamp = p->time_stamp;
                std::optional<std::string> language(std::string(p->language));
                std::optional<std::string> input_source_id(std::string(p->input_source_id));
                std::optional<std::string> input_mode_id(std::string(p->input_mode_id));
                if (language && language->empty()) {
                  language = std::nullopt;
                }
                if (input_source_id && input_source_id->empty()) {
                  input_source_id = std::nullopt;
                }
                if (input_mode_id && input_mode_id->empty()) {
                  input_mode_id = std::nullopt;
                }

                input_source_selector input_source_selector(language,
                                                            input_source_id,
                                                            input_mode_id);

                if (last_select_input_source_time_stamp_ == time_stamp) {
                  return;
                }
                if (input_source_manager_.select(input_source_selector)) {
                  last_select_input_source_time_stamp_ = time_stamp;
                }
              }
              break;

            default:
              break;
          }
        }
      });

      server_manager_->async_start();

      logger::get_logger().info("receiver is initialized");
    });
  }

private:
  std::unique_ptr<local_datagram::server_manager> server_manager_;
  input_source_manager input_source_manager_;
  absolute_time_point last_select_input_source_time_stamp_;
};
} // namespace krbn
