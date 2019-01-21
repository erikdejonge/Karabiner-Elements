#pragma once

#include "async_file_writer.hpp"
#include "logger.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <pqrs/filesystem.hpp>
#include <unistd.h>

namespace krbn {
class json_utility final {
public:
  template <typename T>
  static std::optional<T> find_optional(const nlohmann::json& json,
                                        const std::string& key) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      try {
        return it->get<T>();
      } catch (std::exception&) {
      }
    }

    return std::nullopt;
  }

  static const nlohmann::json* find_array(const nlohmann::json& json,
                                          const std::string& key) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      if (it->is_array()) {
        return &(*it);
      }
    }
    return nullptr;
  }

  static const nlohmann::json* find_object(const nlohmann::json& json,
                                           const std::string& key) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      if (it->is_object()) {
        return &(*it);
      }
    }
    return nullptr;
  }

  static const nlohmann::json* find_json(const nlohmann::json& json,
                                         const std::string& key) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      return &(*it);
    }
    return nullptr;
  }

  static nlohmann::json find_copy(const nlohmann::json& json,
                                  const std::string& key,
                                  const nlohmann::json& fallback_value) {
    auto it = json.find(key);
    if (it != std::end(json)) {
      return *it;
    }
    return fallback_value;
  }

  static void async_save_to_file(const nlohmann::json& json,
                                 const std::string& file_path,
                                 mode_t parent_directory_mode,
                                 mode_t file_mode) {
    async_file_writer::enqueue(file_path,
                               json.dump(4),
                               parent_directory_mode,
                               file_mode);
  }

  static void sync_save_to_file(const nlohmann::json& json,
                                const std::string& file_path,
                                mode_t parent_directory_mode,
                                mode_t file_mode) {
    async_save_to_file(json,
                       file_path,
                       parent_directory_mode,
                       file_mode);

    async_file_writer::wait();
  }
};
} // namespace krbn
