#include "libkrbn.h"
#include "constants.hpp"
#include "libkrbn.hpp"
#include "thread_utility.hpp"
#include "types.hpp"
#include <iostream>
#include <json/json.hpp>

void libkrbn_initialize(void) {
  thread_utility::register_main_thread();
}

const char* libkrbn_get_distributed_notification_observed_object(void) {
  static std::mutex mutex;
  static bool once = false;
  static std::vector<char> result;

  std::lock_guard<std::mutex> guard(mutex);

  if (!once) {
    once = true;
    libkrbn::cfstring_to_cstring(result, constants::get_distributed_notification_observed_object());
  }

  return &(result[0]);
}

const char* libkrbn_get_distributed_notification_grabber_is_launched(void) {
  static std::mutex mutex;
  static bool once = false;
  static std::vector<char> result;

  std::lock_guard<std::mutex> guard(mutex);

  if (!once) {
    once = true;
    libkrbn::cfstring_to_cstring(result, constants::get_distributed_notification_grabber_is_launched());
  }

  return &(result[0]);
}

const char* libkrbn_get_core_configuration_file_path(void) {
  return constants::get_core_configuration_file_path();
}

const char* libkrbn_get_devices_json_file_path(void) {
  return constants::get_devices_json_file_path();
}

bool libkrbn_get_hid_system_key(uint8_t* _Nonnull key, const char* key_name) {
  if (auto key_code = krbn::types::get_key_code(key_name)) {
    if (auto value = krbn::types::get_hid_system_key(*key_code)) {
      *key = *value;
      return true;
    }
  }
  return false;
}

bool libkrbn_get_hid_system_aux_control_button(uint8_t* _Nonnull button, const char* key_name) {
  if (auto key_code = krbn::types::get_key_code(key_name)) {
    if (auto value = krbn::types::get_hid_system_aux_control_button(*key_code)) {
      *button = *value;
      return true;
    }
  }
  return false;
}

char* _Nonnull libkrbn_allocate_beautified_json_string(const char* _Nonnull json_string) {
  // nlohmann::json sorts dictionary keys.
  std::stringstream stream;

  auto json = nlohmann::json::parse(json_string);
  stream << std::setw(4) << json << std::endl;
  auto string = stream.str();

  auto buffer_length = string.size() + 1;
  auto buffer = static_cast<char*>(malloc(buffer_length));
  strlcpy(buffer, string.c_str(), buffer_length);
  return buffer;
}
