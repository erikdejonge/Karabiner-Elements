#pragma once

#include "boost_defs.hpp"

#include "Karabiner-VirtualHIDDevice/dist/include/karabiner_virtual_hid_device_methods.hpp"
#include "apple_hid_usage_tables.hpp"
#include "constants.hpp"
#include "input_source_utility.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include "stream_utility.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDUsageTables.h>
#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <boost/functional/hash.hpp>
#include <boost/optional.hpp>
#include <cstring>
#include <iostream>
#include <json/json.hpp>
#include <regex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace krbn {
class device_detail;

enum class operation_type : uint8_t {
  none,
  // console_user_server -> grabber
  connect,
  system_preferences_updated,
  frontmost_application_changed,
  input_source_changed,
  // grabber -> console_user_server
  shell_command_execution,
  select_input_source,
};

enum class device_id : uint32_t {
  zero = 0,
};

enum class hid_usage_page : uint32_t {
  zero = 0,
  generic_desktop = kHIDPage_GenericDesktop,
  keyboard_or_keypad = kHIDPage_KeyboardOrKeypad,
  leds = kHIDPage_LEDs,
  button = kHIDPage_Button,
  consumer = kHIDPage_Consumer,
  apple_vendor_keyboard = kHIDPage_AppleVendorKeyboard,
  apple_vendor_top_case = kHIDPage_AppleVendorTopCase,
};

enum class hid_usage : uint32_t {
  zero = 0,

  gd_pointer = kHIDUsage_GD_Pointer,
  gd_mouse = kHIDUsage_GD_Mouse,
  gd_keyboard = kHIDUsage_GD_Keyboard,
  gd_x = kHIDUsage_GD_X,
  gd_y = kHIDUsage_GD_Y,
  gd_z = kHIDUsage_GD_Z,
  gd_wheel = kHIDUsage_GD_Wheel,

  led_caps_lock = kHIDUsage_LED_CapsLock,

  csmr_consumercontrol = kHIDUsage_Csmr_ConsumerControl,
  csmr_power = kHIDUsage_Csmr_Power,
  csmr_display_brightness_increment = kHIDUsage_Csmr_DisplayBrightnessIncrement,
  csmr_display_brightness_decrement = kHIDUsage_Csmr_DisplayBrightnessDecrement,
  csmr_fastforward = kHIDUsage_Csmr_FastForward,
  csmr_rewind = kHIDUsage_Csmr_Rewind,
  csmr_scan_next_track = kHIDUsage_Csmr_ScanNextTrack,
  csmr_scan_previous_track = kHIDUsage_Csmr_ScanPreviousTrack,
  csmr_eject = kHIDUsage_Csmr_Eject,
  csmr_play_or_pause = kHIDUsage_Csmr_PlayOrPause,
  csmr_mute = kHIDUsage_Csmr_Mute,
  csmr_volume_increment = kHIDUsage_Csmr_VolumeIncrement,
  csmr_volume_decrement = kHIDUsage_Csmr_VolumeDecrement,
  csmr_acpan = kHIDUsage_Csmr_ACPan,

  apple_vendor_keyboard_spotlight = kHIDUsage_AppleVendorKeyboard_Spotlight,
  apple_vendor_keyboard_dashboard = kHIDUsage_AppleVendorKeyboard_Dashboard,
  apple_vendor_keyboard_function = kHIDUsage_AppleVendorKeyboard_Function,
  apple_vendor_keyboard_launchpad = kHIDUsage_AppleVendorKeyboard_Launchpad,
  apple_vendor_keyboard_expose_all = kHIDUsage_AppleVendorKeyboard_Expose_All,
  apple_vendor_keyboard_expose_desktop = kHIDUsage_AppleVendorKeyboard_Expose_Desktop,
  apple_vendor_keyboard_brightness_up = kHIDUsage_AppleVendorKeyboard_Brightness_Up,
  apple_vendor_keyboard_brightness_down = kHIDUsage_AppleVendorKeyboard_Brightness_Down,

  av_top_case_keyboard_fn = kHIDUsage_AV_TopCase_KeyboardFn,
  av_top_case_brightness_up = kHIDUsage_AV_TopCase_BrightnessUp,
  av_top_case_brightness_down = kHIDUsage_AV_TopCase_BrightnessDown,
  av_top_case_video_mirror = kHIDUsage_AV_TopCase_VideoMirror,
  av_top_case_illumination_toggle = kHIDUsage_AV_TopCase_IlluminationToggle,
  av_top_case_illumination_up = kHIDUsage_AV_TopCase_IlluminationUp,
  av_top_case_illumination_down = kHIDUsage_AV_TopCase_IlluminationDown,
};

enum class event_type : uint32_t {
  key_down,
  key_up,
  single,
};

enum class key_code : uint32_t {
  // 0x00 - 0xff are keys in keyboard_or_keypad usage page.

  a = kHIDUsage_KeyboardA,
  b = kHIDUsage_KeyboardB,
  c = kHIDUsage_KeyboardC,
  d = kHIDUsage_KeyboardD,
  e = kHIDUsage_KeyboardE,
  f = kHIDUsage_KeyboardF,
  g = kHIDUsage_KeyboardG,
  h = kHIDUsage_KeyboardH,
  i = kHIDUsage_KeyboardI,
  j = kHIDUsage_KeyboardJ,
  k = kHIDUsage_KeyboardK,
  l = kHIDUsage_KeyboardL,
  m = kHIDUsage_KeyboardM,
  n = kHIDUsage_KeyboardN,
  o = kHIDUsage_KeyboardO,
  p = kHIDUsage_KeyboardP,
  q = kHIDUsage_KeyboardQ,
  r = kHIDUsage_KeyboardR,
  s = kHIDUsage_KeyboardS,
  t = kHIDUsage_KeyboardT,
  u = kHIDUsage_KeyboardU,
  v = kHIDUsage_KeyboardV,
  w = kHIDUsage_KeyboardW,
  x = kHIDUsage_KeyboardX,
  y = kHIDUsage_KeyboardY,
  z = kHIDUsage_KeyboardZ,

  return_or_enter = kHIDUsage_KeyboardReturnOrEnter,
  escape = kHIDUsage_KeyboardEscape,
  delete_or_backspace = kHIDUsage_KeyboardDeleteOrBackspace,
  tab = kHIDUsage_KeyboardTab,
  spacebar = kHIDUsage_KeyboardSpacebar,

  caps_lock = kHIDUsage_KeyboardCapsLock,

  f1 = kHIDUsage_KeyboardF1,
  f2 = kHIDUsage_KeyboardF2,
  f3 = kHIDUsage_KeyboardF3,
  f4 = kHIDUsage_KeyboardF4,
  f5 = kHIDUsage_KeyboardF5,
  f6 = kHIDUsage_KeyboardF6,
  f7 = kHIDUsage_KeyboardF7,
  f8 = kHIDUsage_KeyboardF8,
  f9 = kHIDUsage_KeyboardF9,
  f10 = kHIDUsage_KeyboardF10,
  f11 = kHIDUsage_KeyboardF11,
  f12 = kHIDUsage_KeyboardF12,
  f13 = kHIDUsage_KeyboardF13,
  f14 = kHIDUsage_KeyboardF14,
  f15 = kHIDUsage_KeyboardF15,
  f16 = kHIDUsage_KeyboardF16,
  f17 = kHIDUsage_KeyboardF17,
  f18 = kHIDUsage_KeyboardF18,
  f19 = kHIDUsage_KeyboardF19,
  f20 = kHIDUsage_KeyboardF20,
  f21 = kHIDUsage_KeyboardF21,
  f22 = kHIDUsage_KeyboardF22,
  f23 = kHIDUsage_KeyboardF23,
  f24 = kHIDUsage_KeyboardF24,

  right_arrow = kHIDUsage_KeyboardRightArrow,
  left_arrow = kHIDUsage_KeyboardLeftArrow,
  down_arrow = kHIDUsage_KeyboardDownArrow,
  up_arrow = kHIDUsage_KeyboardUpArrow,

  keypad_slash = kHIDUsage_KeypadSlash,
  keypad_asterisk = kHIDUsage_KeypadAsterisk,
  keypad_hyphen = kHIDUsage_KeypadHyphen,
  keypad_plus = kHIDUsage_KeypadPlus,
  keypad_enter = kHIDUsage_KeypadEnter,
  keypad_1 = kHIDUsage_Keypad1,
  keypad_2 = kHIDUsage_Keypad2,
  keypad_3 = kHIDUsage_Keypad3,
  keypad_4 = kHIDUsage_Keypad4,
  keypad_5 = kHIDUsage_Keypad5,
  keypad_6 = kHIDUsage_Keypad6,
  keypad_7 = kHIDUsage_Keypad7,
  keypad_8 = kHIDUsage_Keypad8,
  keypad_9 = kHIDUsage_Keypad9,
  keypad_0 = kHIDUsage_Keypad0,
  keypad_period = kHIDUsage_KeypadPeriod,
  keypad_equal_sign = kHIDUsage_KeypadEqualSign,
  keypad_comma = kHIDUsage_KeypadComma,

  home = kHIDUsage_KeyboardHome,
  page_up = kHIDUsage_KeyboardPageUp,
  delete_forward = kHIDUsage_KeyboardDeleteForward,
  end = kHIDUsage_KeyboardEnd,
  page_down = kHIDUsage_KeyboardPageDown,

  mute = kHIDUsage_KeyboardMute,
  volume_decrement = kHIDUsage_KeyboardVolumeDown,
  volume_increment = kHIDUsage_KeyboardVolumeUp,

  left_control = kHIDUsage_KeyboardLeftControl,
  left_shift = kHIDUsage_KeyboardLeftShift,
  left_option = kHIDUsage_KeyboardLeftAlt,
  left_command = kHIDUsage_KeyboardLeftGUI,
  right_control = kHIDUsage_KeyboardRightControl,
  right_shift = kHIDUsage_KeyboardRightShift,
  right_option = kHIDUsage_KeyboardRightAlt,
  right_command = kHIDUsage_KeyboardRightGUI,

  // usage in keyboard_or_keypad usage page is reserved until 0xffff.

  // 0x10000 - are karabiner own virtual key codes or keys not in keyboard_or_keypad usage page.
  extra_ = 0x10000,
  // A pseudo key that does not send any event.
  vk_none,

  // Keys that are not in generic keyboard_or_keypad usage_page.
  fn,
  display_brightness_decrement,
  display_brightness_increment,
  dashboard,
  launchpad,
  mission_control,
  illumination_decrement,
  illumination_increment,
  rewind,
  play_or_pause,
  fastforward,
  eject,
  apple_display_brightness_decrement,
  apple_display_brightness_increment,
  apple_top_case_display_brightness_decrement,
  apple_top_case_display_brightness_increment,
};

enum class consumer_key_code : uint32_t {
  power = kHIDUsage_Csmr_Power,
  display_brightness_increment = kHIDUsage_Csmr_DisplayBrightnessIncrement,
  display_brightness_decrement = kHIDUsage_Csmr_DisplayBrightnessDecrement,
  fastforward = kHIDUsage_Csmr_FastForward,
  rewind = kHIDUsage_Csmr_Rewind,
  scan_next_track = kHIDUsage_Csmr_ScanNextTrack,
  scan_previous_track = kHIDUsage_Csmr_ScanPreviousTrack,
  eject = kHIDUsage_Csmr_Eject,
  play_or_pause = kHIDUsage_Csmr_PlayOrPause,
  mute = kHIDUsage_Csmr_Mute,
  volume_increment = kHIDUsage_Csmr_VolumeIncrement,
  volume_decrement = kHIDUsage_Csmr_VolumeDecrement,
};

enum class pointing_button : uint32_t {
  zero,

  button1,
  button2,
  button3,
  button4,
  button5,
  button6,
  button7,
  button8,

  button9,
  button10,
  button11,
  button12,
  button13,
  button14,
  button15,
  button16,

  button17,
  button18,
  button19,
  button20,
  button21,
  button22,
  button23,
  button24,

  button25,
  button26,
  button27,
  button28,
  button29,
  button30,
  button31,
  button32,

  end_,
};

enum class pointing_event : uint32_t {
  button,
  x,
  y,
  vertical_wheel,
  horizontal_wheel,
};

enum class modifier_flag : uint32_t {
  zero,
  caps_lock,
  left_control,
  left_shift,
  left_option,
  left_command,
  right_control,
  right_shift,
  right_option,
  right_command,
  fn,
  end_,
};

enum class led_state : uint32_t {
  on,
  off,
};

enum class registry_entry_id : uint64_t {
  zero = 0,
};

enum class vendor_id : uint32_t {
  zero = 0,
};

enum class product_id : uint32_t {
  zero = 0,
};

enum class location_id : uint32_t {
};

class hid_value final {
public:
  hid_value(uint64_t time_stamp,
            CFIndex integer_value,
            boost::optional<hid_usage_page> hid_usage_page,
            boost::optional<hid_usage> hid_usage) : time_stamp_(time_stamp),
                                                    integer_value_(integer_value),
                                                    hid_usage_page_(hid_usage_page),
                                                    hid_usage_(hid_usage) {
  }

  hid_value(IOHIDValueRef value) {
    time_stamp_ = IOHIDValueGetTimeStamp(value);
    integer_value_ = IOHIDValueGetIntegerValue(value);
    if (auto element = IOHIDValueGetElement(value)) {
      hid_usage_page_ = hid_usage_page(IOHIDElementGetUsagePage(element));
      hid_usage_ = hid_usage(IOHIDElementGetUsage(element));
    }
  }

  nlohmann::json to_json(void) const {
    nlohmann::json j;
    j["time_stamp"] = time_stamp_;
    j["integer_value"] = integer_value_;
    if (hid_usage_page_) {
      j["hid_usage_page"] = *hid_usage_page_;
    }
    if (hid_usage_) {
      j["hid_usage"] = *hid_usage_;
    }
    return j;
  }

  uint64_t get_time_stamp(void) const {
    return time_stamp_;
  }

  CFIndex get_integer_value(void) const {
    return integer_value_;
  }

  boost::optional<hid_usage_page> get_hid_usage_page(void) const {
    return hid_usage_page_;
  }

  boost::optional<hid_usage> get_hid_usage(void) const {
    return hid_usage_;
  }

  bool conforms_to(hid_usage_page hid_usage_page,
                   hid_usage hid_usage) const {
    return hid_usage_page_ == hid_usage_page &&
           hid_usage_ == hid_usage;
  }

  bool operator==(const hid_value& other) const {
    return time_stamp_ == other.time_stamp_ &&
           integer_value_ == other.integer_value_ &&
           hid_usage_page_ == other.hid_usage_page_ &&
           hid_usage_ == other.hid_usage_;
  }

private:
  uint64_t time_stamp_;
  CFIndex integer_value_;
  boost::optional<hid_usage_page> hid_usage_page_;
  boost::optional<hid_usage> hid_usage_;
};

class pointing_motion final {
public:
  pointing_motion(void) : x_(0),
                          y_(0),
                          vertical_wheel_(0),
                          horizontal_wheel_(0) {
  }

  pointing_motion(int x,
                  int y,
                  int vertical_wheel,
                  int horizontal_wheel) : x_(x),
                                          y_(y),
                                          vertical_wheel_(vertical_wheel),
                                          horizontal_wheel_(horizontal_wheel) {
  }

  pointing_motion(const nlohmann::json& json) : pointing_motion() {
    if (auto v = json_utility::find_optional<int>(json, "x")) {
      x_ = *v;
    }
    if (auto v = json_utility::find_optional<int>(json, "y")) {
      y_ = *v;
    }
    if (auto v = json_utility::find_optional<int>(json, "vertical_wheel")) {
      vertical_wheel_ = *v;
    }
    if (auto v = json_utility::find_optional<int>(json, "horizontal_wheel")) {
      horizontal_wheel_ = *v;
    }
  }

  nlohmann::json to_json(void) const {
    nlohmann::json j;
    j["x"] = x_;
    j["y"] = y_;
    j["vertical_wheel"] = vertical_wheel_;
    j["horizontal_wheel"] = horizontal_wheel_;
    return j;
  }

  int get_x(void) const {
    return x_;
  }

  void set_x(int value) {
    x_ = value;
  }

  int get_y(void) const {
    return y_;
  }

  void set_y(int value) {
    y_ = value;
  }

  int get_vertical_wheel(void) const {
    return vertical_wheel_;
  }

  void set_vertical_wheel(int value) {
    vertical_wheel_ = value;
  }

  int get_horizontal_wheel(void) const {
    return horizontal_wheel_;
  }

  void set_horizontal_wheel(int value) {
    horizontal_wheel_ = value;
  }

  bool is_zero(void) const {
    return x_ == 0 &&
           y_ == 0 &&
           vertical_wheel_ == 0 &&
           horizontal_wheel_ == 0;
  }

  void clear(void) {
    x_ = 0;
    y_ = 0;
    vertical_wheel_ = 0;
    horizontal_wheel_ = 0;
  }

  bool operator==(const pointing_motion& other) const {
    return x_ == other.x_ &&
           y_ == other.y_ &&
           vertical_wheel_ == other.vertical_wheel_ &&
           horizontal_wheel_ == other.horizontal_wheel_;
  }

  friend size_t hash_value(const pointing_motion& value) {
    size_t h = 0;
    boost::hash_combine(h, value.x_);
    boost::hash_combine(h, value.y_);
    boost::hash_combine(h, value.vertical_wheel_);
    boost::hash_combine(h, value.horizontal_wheel_);
    return h;
  }

private:
  int x_;
  int y_;
  int vertical_wheel_;
  int horizontal_wheel_;
};

class device_identifiers final {
public:
  device_identifiers(void) : vendor_id_(vendor_id::zero),
                             product_id_(product_id::zero),
                             is_keyboard_(false),
                             is_pointing_device_(false) {
  }

  device_identifiers(vendor_id vendor_id,
                     product_id product_id,
                     bool is_keyboard,
                     bool is_pointing_device) : vendor_id_(vendor_id),
                                                product_id_(product_id),
                                                is_keyboard_(is_keyboard),
                                                is_pointing_device_(is_pointing_device) {
  }

  device_identifiers(const nlohmann::json& json) : device_identifiers() {
    json_ = json;

    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "vendor_id") {
          if (value.is_number()) {
            vendor_id_ = vendor_id(static_cast<uint32_t>(value));
          } else {
            logger::get_logger().error("Invalid form of {0}: {1}", key, value.dump());
          }
        }
        if (key == "product_id") {
          if (value.is_number()) {
            product_id_ = product_id(static_cast<uint32_t>(value));
          } else {
            logger::get_logger().error("Invalid form of {0}: {1}", key, value.dump());
          }
        }
        if (key == "is_keyboard") {
          if (value.is_boolean()) {
            is_keyboard_ = value;
          } else {
            logger::get_logger().error("Invalid form of {0}: {1}", key, value.dump());
          }
        }
        if (key == "is_pointing_device") {
          if (value.is_boolean()) {
            is_pointing_device_ = value;
          } else {
            logger::get_logger().error("Invalid form of {0}: {1}", key, value.dump());
          }
        }
      }

    } else {
      logger::get_logger().error("Invalid form of device_identifiers: {0}", json.dump());
    }
  }

  nlohmann::json to_json(void) const {
    auto j = json_;
    j["vendor_id"] = static_cast<uint32_t>(vendor_id_);
    j["product_id"] = static_cast<uint32_t>(product_id_);
    j["is_keyboard"] = is_keyboard_;
    j["is_pointing_device"] = is_pointing_device_;
    return j;
  }

  vendor_id get_vendor_id(void) const {
    return vendor_id_;
  }

  product_id get_product_id(void) const {
    return product_id_;
  }

  bool get_is_keyboard(void) const {
    return is_keyboard_;
  }

  bool get_is_pointing_device(void) const {
    return is_pointing_device_;
  }

  bool is_apple(void) const {
    return vendor_id_ == vendor_id(0x05ac) ||
           vendor_id_ == vendor_id(0x004c);
  }

  bool operator==(const device_identifiers& other) const {
    return vendor_id_ == other.vendor_id_ &&
           product_id_ == other.product_id_ &&
           is_keyboard_ == other.is_keyboard_ &&
           is_pointing_device_ == other.is_pointing_device_;
  }

private:
  nlohmann::json json_;
  vendor_id vendor_id_;
  product_id product_id_;
  bool is_keyboard_;
  bool is_pointing_device_;
};

class input_source_identifiers final {
public:
  input_source_identifiers(void) {
  }

  input_source_identifiers(TISInputSourceRef p) : language_(input_source_utility::get_language(p)),
                                                  input_source_id_(input_source_utility::get_input_source_id(p)),
                                                  input_mode_id_(input_source_utility::get_input_mode_id(p)) {
  }

  input_source_identifiers(const boost::optional<std::string>& language,
                           const boost::optional<std::string>& input_source_id,
                           const boost::optional<std::string>& input_mode_id) : language_(language),
                                                                                input_source_id_(input_source_id),
                                                                                input_mode_id_(input_mode_id) {
  }

  input_source_identifiers(const nlohmann::json& json) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "language") {
          if (value.is_string()) {
            language_ = value.get<std::string>();
          } else {
            logger::get_logger().error("language should be string: {0}", json.dump());
          }
        } else if (key == "input_source_id") {
          if (value.is_string()) {
            input_source_id_ = value.get<std::string>();
          } else {
            logger::get_logger().error("input_source_id should be string: {0}", json.dump());
          }
        } else if (key == "input_mode_id") {
          if (value.is_string()) {
            input_mode_id_ = value.get<std::string>();
          } else {
            logger::get_logger().error("input_mode_id should be string: {0}", json.dump());
          }
        } else {
          logger::get_logger().error("json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    } else {
      logger::get_logger().error("input_source_identifiers should be object: {0}", json.dump());
    }
  }

  nlohmann::json to_json(void) const {
    auto json = nlohmann::json::object();

    if (language_) {
      json["language"] = *language_;
    }

    if (input_source_id_) {
      json["input_source_id"] = *input_source_id_;
    }

    if (input_mode_id_) {
      json["input_mode_id"] = *input_mode_id_;
    }

    return json;
  }

  const boost::optional<std::string>& get_language(void) const {
    return language_;
  }

  const boost::optional<std::string>& get_input_source_id(void) const {
    return input_source_id_;
  }

  const boost::optional<std::string>& get_input_mode_id(void) const {
    return input_mode_id_;
  }

  bool operator==(const input_source_identifiers& other) const {
    return language_ == other.language_ &&
           input_source_id_ == other.input_source_id_ &&
           input_mode_id_ == other.input_mode_id_;
  }

  friend size_t hash_value(const input_source_identifiers& value) {
    size_t h = 0;
    if (value.language_) {
      boost::hash_combine(h, *(value.language_));
    }
    if (value.input_source_id_) {
      boost::hash_combine(h, *(value.input_source_id_));
    }
    if (value.input_mode_id_) {
      boost::hash_combine(h, *(value.input_mode_id_));
    }
    return h;
  }

private:
  boost::optional<std::string> language_;
  boost::optional<std::string> input_source_id_;
  boost::optional<std::string> input_mode_id_;
};

class input_source_selector final {
public:
  input_source_selector(const boost::optional<std::string>& language_string,
                        const boost::optional<std::string>& input_source_id_string,
                        const boost::optional<std::string>& input_mode_id_string) : language_string_(language_string),
                                                                                    input_source_id_string_(input_source_id_string),
                                                                                    input_mode_id_string_(input_mode_id_string) {
    update_regexs();
  }

  input_source_selector(const nlohmann::json& json) {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "language") {
          if (value.is_string()) {
            language_string_ = value.get<std::string>();
          } else {
            logger::get_logger().error("complex_modifications json error: input_source_selector.language should be string: {0}", json.dump());
          }
        } else if (key == "input_source_id") {
          if (value.is_string()) {
            input_source_id_string_ = value.get<std::string>();
          } else {
            logger::get_logger().error("complex_modifications json error: input_source_selector.input_source_id should be string: {0}", json.dump());
          }
        } else if (key == "input_mode_id") {
          if (value.is_string()) {
            input_mode_id_string_ = value.get<std::string>();
          } else {
            logger::get_logger().error("complex_modifications json error: input_source_selector.input_mode_id should be string: {0}", json.dump());
          }
        } else {
          logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    } else {
      logger::get_logger().error("complex_modifications json error: input_source_selector should be array of object: {0}", json.dump());
    }

    update_regexs();
  }

  nlohmann::json to_json(void) const {
    auto json = nlohmann::json::object();

    if (language_string_) {
      json["language"] = *language_string_;
    }

    if (input_source_id_string_) {
      json["input_source_id"] = *input_source_id_string_;
    }

    if (input_mode_id_string_) {
      json["input_mode_id"] = *input_mode_id_string_;
    }

    return json;
  }

  const boost::optional<std::string>& get_language_string(void) const {
    return language_string_;
  }

  const boost::optional<std::string>& get_input_source_id_string(void) const {
    return input_source_id_string_;
  }

  const boost::optional<std::string>& get_input_mode_id_string(void) const {
    return input_mode_id_string_;
  }

  bool test(const input_source_identifiers& input_source_identifiers) const {
    if (language_regex_) {
      if (auto& v = input_source_identifiers.get_language()) {
        if (!regex_search(std::begin(*v),
                          std::end(*v),
                          *language_regex_)) {
          return false;
        }
      } else {
        return false;
      }
    }

    if (input_source_id_regex_) {
      if (auto& v = input_source_identifiers.get_input_source_id()) {
        if (!regex_search(std::begin(*v),
                          std::end(*v),
                          *input_source_id_regex_)) {
          return false;
        }
      } else {
        return false;
      }
    }

    if (input_mode_id_regex_) {
      if (auto& v = input_source_identifiers.get_input_mode_id()) {
        if (!regex_search(std::begin(*v),
                          std::end(*v),
                          *input_mode_id_regex_)) {
          return false;
        }
      } else {
        return false;
      }
    }

    return true;
  }

  bool operator==(const input_source_selector& other) const {
    return language_string_ == other.language_string_ &&
           input_source_id_string_ == other.input_source_id_string_ &&
           input_mode_id_string_ == other.input_mode_id_string_;
  }

  friend size_t hash_value(const input_source_selector& value) {
    size_t h = 0;
    if (value.language_string_) {
      boost::hash_combine(h, *(value.language_string_));
    }
    if (value.input_source_id_string_) {
      boost::hash_combine(h, *(value.input_source_id_string_));
    }
    if (value.input_mode_id_string_) {
      boost::hash_combine(h, *(value.input_mode_id_string_));
    }

    // We can skip *_regex_ since *_regex_ is synchronized with *_string_.

    return h;
  }

private:
  void update_regexs(void) {
    std::string s;

    try {
      if (language_string_) {
        s = *language_string_;
        language_regex_ = std::regex(s);
      }

      if (input_source_id_string_) {
        s = *input_source_id_string_;
        input_source_id_regex_ = std::regex(s);
      }

      if (input_mode_id_string_) {
        s = *input_mode_id_string_;
        input_mode_id_regex_ = std::regex(s);
      }
    } catch (std::exception& e) {
      logger::get_logger().error("complex_modifications json error: Regex error: \"{0}\" {1}", s, e.what());
    }
  }

  boost::optional<std::string> language_string_;
  boost::optional<std::string> input_source_id_string_;
  boost::optional<std::string> input_mode_id_string_;

  boost::optional<std::regex> language_regex_;
  boost::optional<std::regex> input_source_id_regex_;
  boost::optional<std::regex> input_mode_id_regex_;
};

class mouse_key final {
public:
  mouse_key(void) : x_(0),
                    y_(0),
                    vertical_wheel_(0),
                    horizontal_wheel_(0),
                    speed_multiplier_(1.0) {
  }

  mouse_key(int x,
            int y,
            int vertical_wheel,
            int horizontal_wheel,
            double speed_multiplier) : x_(x),
                                       y_(y),
                                       vertical_wheel_(vertical_wheel),
                                       horizontal_wheel_(horizontal_wheel),
                                       speed_multiplier_(speed_multiplier) {
  }

  mouse_key(const nlohmann::json& json) : mouse_key() {
    if (json.is_object()) {
      for (auto it = std::begin(json); it != std::end(json); std::advance(it, 1)) {
        // it.key() is always std::string.
        const auto& key = it.key();
        const auto& value = it.value();

        if (key == "x") {
          if (value.is_number()) {
            x_ = value.get<int>();
          } else {
            logger::get_logger().error("complex_modifications json error: mouse_key.x should be number: {0}", json.dump());
          }
        } else if (key == "y") {
          if (value.is_number()) {
            y_ = value.get<int>();
          } else {
            logger::get_logger().error("complex_modifications json error: mouse_key.y should be number: {0}", json.dump());
          }
        } else if (key == "vertical_wheel") {
          if (value.is_number()) {
            vertical_wheel_ = value.get<int>();
          } else {
            logger::get_logger().error("complex_modifications json error: mouse_key.vertical_wheel should be number: {0}", json.dump());
          }
        } else if (key == "horizontal_wheel") {
          if (value.is_number()) {
            horizontal_wheel_ = value.get<int>();
          } else {
            logger::get_logger().error("complex_modifications json error: mouse_key.horizontal_wheel should be number: {0}", json.dump());
          }
        } else if (key == "speed_multiplier") {
          if (value.is_number()) {
            speed_multiplier_ = value.get<double>();
          } else {
            logger::get_logger().error("complex_modifications json error: mouse_key.speed_multiplier should be number: {0}", json.dump());
          }
        } else {
          logger::get_logger().error("complex_modifications json error: Unknown key: {0} in {1}", key, json.dump());
        }
      }
    }
  }

  nlohmann::json to_json(void) const {
    nlohmann::json j;
    j["x"] = x_;
    j["y"] = y_;
    j["vertical_wheel"] = vertical_wheel_;
    j["horizontal_wheel"] = horizontal_wheel_;
    j["speed_multiplier"] = speed_multiplier_;
    return j;
  }

  int get_x(void) const {
    return x_;
  }

  int get_y(void) const {
    return y_;
  }

  int get_vertical_wheel(void) const {
    return vertical_wheel_;
  }

  int get_horizontal_wheel(void) const {
    return horizontal_wheel_;
  }

  double get_speed_multiplier(void) const {
    return speed_multiplier_;
  }

  bool is_zero(void) const {
    // Do not check speed_multiplier_ here.

    return x_ == 0 &&
           y_ == 0 &&
           vertical_wheel_ == 0 &&
           horizontal_wheel_ == 0;
  }

  void invert_wheel(void) {
    vertical_wheel_ = -vertical_wheel_;
    horizontal_wheel_ = -horizontal_wheel_;
  }

  mouse_key& operator+=(const mouse_key& other) {
    x_ += other.x_;
    y_ += other.y_;
    vertical_wheel_ += other.vertical_wheel_;
    horizontal_wheel_ += other.horizontal_wheel_;

    // multiply speed_multiplier_.
    speed_multiplier_ *= other.speed_multiplier_;

    return *this;
  }

  const mouse_key operator+(const mouse_key& other) const {
    mouse_key result = *this;
    result += other;
    return result;
  }

  bool operator==(const mouse_key& other) const {
    return x_ == other.x_ &&
           y_ == other.y_ &&
           vertical_wheel_ == other.vertical_wheel_ &&
           horizontal_wheel_ == other.horizontal_wheel_ &&
           speed_multiplier_ == other.speed_multiplier_;
  }

  friend size_t hash_value(const mouse_key& value) {
    size_t h = 0;
    boost::hash_combine(h, value.x_);
    boost::hash_combine(h, value.y_);
    boost::hash_combine(h, value.vertical_wheel_);
    boost::hash_combine(h, value.horizontal_wheel_);
    boost::hash_combine(h, value.speed_multiplier_);
    return h;
  }

private:
  int x_;
  int y_;
  int vertical_wheel_;
  int horizontal_wheel_;
  double speed_multiplier_;
};

class system_preferences final {
public:
  system_preferences(void) : keyboard_fn_state_(false),
                             swipe_scroll_direction_(true),
                             keyboard_type_(40) {
  }

  bool get_keyboard_fn_state(void) const {
    return keyboard_fn_state_;
  }

  void set_keyboard_fn_state(bool value) {
    keyboard_fn_state_ = value;
  }

  bool get_swipe_scroll_direction(void) const {
    return swipe_scroll_direction_;
  }

  void set_swipe_scroll_direction(bool value) {
    swipe_scroll_direction_ = value;
  }

  uint8_t get_keyboard_type(void) const {
    return keyboard_type_;
  }

  void set_keyboard_type(uint8_t value) {
    keyboard_type_ = value;
  }

  bool operator==(const system_preferences& other) const {
    return keyboard_fn_state_ == other.keyboard_fn_state_ &&
           swipe_scroll_direction_ == other.swipe_scroll_direction_ &&
           keyboard_type_ == other.keyboard_type_;
  }

  bool operator!=(const system_preferences& other) const { return !(*this == other); }

private:
  bool keyboard_fn_state_;
  bool swipe_scroll_direction_;
  uint8_t keyboard_type_;
};

class types final {
public:
  static device_id make_new_device_id(const std::shared_ptr<device_detail>& device_detail) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static auto id = device_id::zero;

    id = device_id(static_cast<uint32_t>(id) + 1);

    auto& map = get_device_id_map();
    map[id] = device_detail;

    return id;
  }

  static void detach_device_id(device_id device_id) {
    auto& map = get_device_id_map();
    map.erase(device_id);
  }

  static const std::shared_ptr<device_detail> find_device_detail(device_id device_id) {
    auto& map = get_device_id_map();
    auto it = map.find(device_id);
    if (it == std::end(map)) {
      return nullptr;
    }
    return it->second;
  }

  static boost::optional<modifier_flag> make_modifier_flag(key_code key_code) {
    // make_modifier_flag(key_code::caps_lock) == boost::none

    switch (key_code) {
      case key_code::left_control:
        return modifier_flag::left_control;
      case key_code::left_shift:
        return modifier_flag::left_shift;
      case key_code::left_option:
        return modifier_flag::left_option;
      case key_code::left_command:
        return modifier_flag::left_command;
      case key_code::right_control:
        return modifier_flag::right_control;
      case key_code::right_shift:
        return modifier_flag::right_shift;
      case key_code::right_option:
        return modifier_flag::right_option;
      case key_code::right_command:
        return modifier_flag::right_command;
      case key_code::fn:
        return modifier_flag::fn;
      default:
        return boost::none;
    }
  }

  static boost::optional<modifier_flag> make_modifier_flag(hid_usage_page usage_page, hid_usage usage) {
    if (auto key_code = make_key_code(usage_page, usage)) {
      return make_modifier_flag(*key_code);
    }
    return boost::none;
  }

  static boost::optional<modifier_flag> make_modifier_flag(const hid_value& hid_value) {
    if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
      if (auto hid_usage = hid_value.get_hid_usage()) {
        return make_modifier_flag(*hid_usage_page,
                                  *hid_usage);
      }
    }
    return boost::none;
  }

  static boost::optional<pqrs::karabiner_virtual_hid_device::hid_report::modifier> make_hid_report_modifier(modifier_flag modifier_flag) {
    switch (modifier_flag) {
      case modifier_flag::left_control:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_control;
      case modifier_flag::left_shift:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_shift;
      case modifier_flag::left_option:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_option;
      case modifier_flag::left_command:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::left_command;
      case modifier_flag::right_control:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_control;
      case modifier_flag::right_shift:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_shift;
      case modifier_flag::right_option:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_option;
      case modifier_flag::right_command:
        return pqrs::karabiner_virtual_hid_device::hid_report::modifier::right_command;
      default:
        return boost::none;
    }
  }

  static boost::optional<key_code> make_key_code(modifier_flag modifier_flag) {
    switch (modifier_flag) {
      case modifier_flag::zero:
        return boost::none;

      case modifier_flag::caps_lock:
        return key_code::caps_lock;

      case modifier_flag::left_control:
        return key_code::left_control;

      case modifier_flag::left_shift:
        return key_code::left_shift;

      case modifier_flag::left_option:
        return key_code::left_option;

      case modifier_flag::left_command:
        return key_code::left_command;

      case modifier_flag::right_control:
        return key_code::right_control;

      case modifier_flag::right_shift:
        return key_code::right_shift;

      case modifier_flag::right_option:
        return key_code::right_option;

      case modifier_flag::right_command:
        return key_code::right_command;

      case modifier_flag::fn:
        return key_code::fn;

      case modifier_flag::end_:
        return boost::none;
    }
  }

  // string -> hid usage map
  static const std::vector<std::pair<std::string, key_code>>& get_key_code_name_value_pairs(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::vector<std::pair<std::string, key_code>> pairs({
        // From IOHIDUsageTables.h
        {"a", key_code(kHIDUsage_KeyboardA)},
        {"b", key_code(kHIDUsage_KeyboardB)},
        {"c", key_code(kHIDUsage_KeyboardC)},
        {"d", key_code(kHIDUsage_KeyboardD)},
        {"e", key_code(kHIDUsage_KeyboardE)},
        {"f", key_code(kHIDUsage_KeyboardF)},
        {"g", key_code(kHIDUsage_KeyboardG)},
        {"h", key_code(kHIDUsage_KeyboardH)},
        {"i", key_code(kHIDUsage_KeyboardI)},
        {"j", key_code(kHIDUsage_KeyboardJ)},
        {"k", key_code(kHIDUsage_KeyboardK)},
        {"l", key_code(kHIDUsage_KeyboardL)},
        {"m", key_code(kHIDUsage_KeyboardM)},
        {"n", key_code(kHIDUsage_KeyboardN)},
        {"o", key_code(kHIDUsage_KeyboardO)},
        {"p", key_code(kHIDUsage_KeyboardP)},
        {"q", key_code(kHIDUsage_KeyboardQ)},
        {"r", key_code(kHIDUsage_KeyboardR)},
        {"s", key_code(kHIDUsage_KeyboardS)},
        {"t", key_code(kHIDUsage_KeyboardT)},
        {"u", key_code(kHIDUsage_KeyboardU)},
        {"v", key_code(kHIDUsage_KeyboardV)},
        {"w", key_code(kHIDUsage_KeyboardW)},
        {"x", key_code(kHIDUsage_KeyboardX)},
        {"y", key_code(kHIDUsage_KeyboardY)},
        {"z", key_code(kHIDUsage_KeyboardZ)},
        {"1", key_code(kHIDUsage_Keyboard1)},
        {"2", key_code(kHIDUsage_Keyboard2)},
        {"3", key_code(kHIDUsage_Keyboard3)},
        {"4", key_code(kHIDUsage_Keyboard4)},
        {"5", key_code(kHIDUsage_Keyboard5)},
        {"6", key_code(kHIDUsage_Keyboard6)},
        {"7", key_code(kHIDUsage_Keyboard7)},
        {"8", key_code(kHIDUsage_Keyboard8)},
        {"9", key_code(kHIDUsage_Keyboard9)},
        {"0", key_code(kHIDUsage_Keyboard0)},
        {"return_or_enter", key_code(kHIDUsage_KeyboardReturnOrEnter)},
        {"escape", key_code(kHIDUsage_KeyboardEscape)},
        {"delete_or_backspace", key_code(kHIDUsage_KeyboardDeleteOrBackspace)},
        {"tab", key_code(kHIDUsage_KeyboardTab)},
        {"spacebar", key_code(kHIDUsage_KeyboardSpacebar)},
        {"hyphen", key_code(kHIDUsage_KeyboardHyphen)},
        {"equal_sign", key_code(kHIDUsage_KeyboardEqualSign)},
        {"open_bracket", key_code(kHIDUsage_KeyboardOpenBracket)},
        {"close_bracket", key_code(kHIDUsage_KeyboardCloseBracket)},
        {"backslash", key_code(kHIDUsage_KeyboardBackslash)},
        {"non_us_pound", key_code(kHIDUsage_KeyboardNonUSPound)},
        {"semicolon", key_code(kHIDUsage_KeyboardSemicolon)},
        {"quote", key_code(kHIDUsage_KeyboardQuote)},
        {"grave_accent_and_tilde", key_code(kHIDUsage_KeyboardGraveAccentAndTilde)},
        {"comma", key_code(kHIDUsage_KeyboardComma)},
        {"period", key_code(kHIDUsage_KeyboardPeriod)},
        {"slash", key_code(kHIDUsage_KeyboardSlash)},
        {"caps_lock", key_code(kHIDUsage_KeyboardCapsLock)},
        {"f1", key_code(kHIDUsage_KeyboardF1)},
        {"f2", key_code(kHIDUsage_KeyboardF2)},
        {"f3", key_code(kHIDUsage_KeyboardF3)},
        {"f4", key_code(kHIDUsage_KeyboardF4)},
        {"f5", key_code(kHIDUsage_KeyboardF5)},
        {"f6", key_code(kHIDUsage_KeyboardF6)},
        {"f7", key_code(kHIDUsage_KeyboardF7)},
        {"f8", key_code(kHIDUsage_KeyboardF8)},
        {"f9", key_code(kHIDUsage_KeyboardF9)},
        {"f10", key_code(kHIDUsage_KeyboardF10)},
        {"f11", key_code(kHIDUsage_KeyboardF11)},
        {"f12", key_code(kHIDUsage_KeyboardF12)},
        {"print_screen", key_code(kHIDUsage_KeyboardPrintScreen)},
        {"scroll_lock", key_code(kHIDUsage_KeyboardScrollLock)},
        {"pause", key_code(kHIDUsage_KeyboardPause)},
        {"insert", key_code(kHIDUsage_KeyboardInsert)},
        {"home", key_code(kHIDUsage_KeyboardHome)},
        {"page_up", key_code(kHIDUsage_KeyboardPageUp)},
        {"delete_forward", key_code(kHIDUsage_KeyboardDeleteForward)},
        {"end", key_code(kHIDUsage_KeyboardEnd)},
        {"page_down", key_code(kHIDUsage_KeyboardPageDown)},
        {"right_arrow", key_code(kHIDUsage_KeyboardRightArrow)},
        {"left_arrow", key_code(kHIDUsage_KeyboardLeftArrow)},
        {"down_arrow", key_code(kHIDUsage_KeyboardDownArrow)},
        {"up_arrow", key_code(kHIDUsage_KeyboardUpArrow)},
        {"keypad_num_lock", key_code(kHIDUsage_KeypadNumLock)},
        {"keypad_slash", key_code(kHIDUsage_KeypadSlash)},
        {"keypad_asterisk", key_code(kHIDUsage_KeypadAsterisk)},
        {"keypad_hyphen", key_code(kHIDUsage_KeypadHyphen)},
        {"keypad_plus", key_code(kHIDUsage_KeypadPlus)},
        {"keypad_enter", key_code(kHIDUsage_KeypadEnter)},
        {"keypad_1", key_code(kHIDUsage_Keypad1)},
        {"keypad_2", key_code(kHIDUsage_Keypad2)},
        {"keypad_3", key_code(kHIDUsage_Keypad3)},
        {"keypad_4", key_code(kHIDUsage_Keypad4)},
        {"keypad_5", key_code(kHIDUsage_Keypad5)},
        {"keypad_6", key_code(kHIDUsage_Keypad6)},
        {"keypad_7", key_code(kHIDUsage_Keypad7)},
        {"keypad_8", key_code(kHIDUsage_Keypad8)},
        {"keypad_9", key_code(kHIDUsage_Keypad9)},
        {"keypad_0", key_code(kHIDUsage_Keypad0)},
        {"keypad_period", key_code(kHIDUsage_KeypadPeriod)},
        {"non_us_backslash", key_code(kHIDUsage_KeyboardNonUSBackslash)},
        {"application", key_code(kHIDUsage_KeyboardApplication)},
        {"power", key_code(kHIDUsage_KeyboardPower)},
        {"keypad_equal_sign", key_code(kHIDUsage_KeypadEqualSign)},
        {"f13", key_code(kHIDUsage_KeyboardF13)},
        {"f14", key_code(kHIDUsage_KeyboardF14)},
        {"f15", key_code(kHIDUsage_KeyboardF15)},
        {"f16", key_code(kHIDUsage_KeyboardF16)},
        {"f17", key_code(kHIDUsage_KeyboardF17)},
        {"f18", key_code(kHIDUsage_KeyboardF18)},
        {"f19", key_code(kHIDUsage_KeyboardF19)},
        {"f20", key_code(kHIDUsage_KeyboardF20)},
        {"f21", key_code(kHIDUsage_KeyboardF21)},
        {"f22", key_code(kHIDUsage_KeyboardF22)},
        {"f23", key_code(kHIDUsage_KeyboardF23)},
        {"f24", key_code(kHIDUsage_KeyboardF24)},
        {"execute", key_code(kHIDUsage_KeyboardExecute)},
        {"help", key_code(kHIDUsage_KeyboardHelp)},
        {"menu", key_code(kHIDUsage_KeyboardMenu)},
        {"select", key_code(kHIDUsage_KeyboardSelect)},
        {"stop", key_code(kHIDUsage_KeyboardStop)},
        {"again", key_code(kHIDUsage_KeyboardAgain)},
        {"undo", key_code(kHIDUsage_KeyboardUndo)},
        {"cut", key_code(kHIDUsage_KeyboardCut)},
        {"copy", key_code(kHIDUsage_KeyboardCopy)},
        {"paste", key_code(kHIDUsage_KeyboardPaste)},
        {"find", key_code(kHIDUsage_KeyboardFind)},
        {"mute", key_code(kHIDUsage_KeyboardMute)},
        {"volume_decrement", key_code(kHIDUsage_KeyboardVolumeDown)},
        {"volume_increment", key_code(kHIDUsage_KeyboardVolumeUp)},
        {"locking_caps_lock", key_code(kHIDUsage_KeyboardLockingCapsLock)},
        {"locking_num_lock", key_code(kHIDUsage_KeyboardLockingNumLock)},
        {"locking_scroll_lock", key_code(kHIDUsage_KeyboardLockingScrollLock)},
        {"keypad_comma", key_code(kHIDUsage_KeypadComma)},
        {"keypad_equal_sign_as400", key_code(kHIDUsage_KeypadEqualSignAS400)},
        {"international1", key_code(kHIDUsage_KeyboardInternational1)},
        {"international2", key_code(kHIDUsage_KeyboardInternational2)},
        {"international3", key_code(kHIDUsage_KeyboardInternational3)},
        {"international4", key_code(kHIDUsage_KeyboardInternational4)},
        {"international5", key_code(kHIDUsage_KeyboardInternational5)},
        {"international6", key_code(kHIDUsage_KeyboardInternational6)},
        {"international7", key_code(kHIDUsage_KeyboardInternational7)},
        {"international8", key_code(kHIDUsage_KeyboardInternational8)},
        {"international9", key_code(kHIDUsage_KeyboardInternational9)},
        {"lang1", key_code(kHIDUsage_KeyboardLANG1)},
        {"lang2", key_code(kHIDUsage_KeyboardLANG2)},
        {"lang3", key_code(kHIDUsage_KeyboardLANG3)},
        {"lang4", key_code(kHIDUsage_KeyboardLANG4)},
        {"lang5", key_code(kHIDUsage_KeyboardLANG5)},
        {"lang6", key_code(kHIDUsage_KeyboardLANG6)},
        {"lang7", key_code(kHIDUsage_KeyboardLANG7)},
        {"lang8", key_code(kHIDUsage_KeyboardLANG8)},
        {"lang9", key_code(kHIDUsage_KeyboardLANG9)},
        {"alternate_erase", key_code(kHIDUsage_KeyboardAlternateErase)},
        {"sys_req_or_attention", key_code(kHIDUsage_KeyboardSysReqOrAttention)},
        {"cancel", key_code(kHIDUsage_KeyboardCancel)},
        {"clear", key_code(kHIDUsage_KeyboardClear)},
        {"prior", key_code(kHIDUsage_KeyboardPrior)},
        {"return", key_code(kHIDUsage_KeyboardReturn)},
        {"separator", key_code(kHIDUsage_KeyboardSeparator)},
        {"out", key_code(kHIDUsage_KeyboardOut)},
        {"oper", key_code(kHIDUsage_KeyboardOper)},
        {"clear_or_again", key_code(kHIDUsage_KeyboardClearOrAgain)},
        {"cr_sel_or_props", key_code(kHIDUsage_KeyboardCrSelOrProps)},
        {"ex_sel", key_code(kHIDUsage_KeyboardExSel)},
        {"left_control", key_code(kHIDUsage_KeyboardLeftControl)},
        {"left_shift", key_code(kHIDUsage_KeyboardLeftShift)},
        {"left_alt", key_code(kHIDUsage_KeyboardLeftAlt)},
        {"left_gui", key_code(kHIDUsage_KeyboardLeftGUI)},
        {"right_control", key_code(kHIDUsage_KeyboardRightControl)},
        {"right_shift", key_code(kHIDUsage_KeyboardRightShift)},
        {"right_alt", key_code(kHIDUsage_KeyboardRightAlt)},
        {"right_gui", key_code(kHIDUsage_KeyboardRightGUI)},

        // Extra
        {"vk_none", key_code::vk_none},

        {"fn", key_code::fn},
        {"display_brightness_decrement", key_code::display_brightness_decrement},
        {"display_brightness_increment", key_code::display_brightness_increment},
        {"mission_control", key_code::mission_control},
        {"launchpad", key_code::launchpad},
        {"dashboard", key_code::dashboard},
        {"illumination_decrement", key_code::illumination_decrement},
        {"illumination_increment", key_code::illumination_increment},
        {"rewind", key_code::rewind},
        {"play_or_pause", key_code::play_or_pause},
        {"fastforward", key_code::fastforward},
        {"eject", key_code::eject},
        {"apple_display_brightness_decrement", key_code::apple_display_brightness_decrement},
        {"apple_display_brightness_increment", key_code::apple_display_brightness_increment},
        {"apple_top_case_display_brightness_decrement", key_code::apple_top_case_display_brightness_decrement},
        {"apple_top_case_display_brightness_increment", key_code::apple_top_case_display_brightness_increment},

        // Aliases
        {"left_option", key_code(kHIDUsage_KeyboardLeftAlt)},
        {"left_command", key_code(kHIDUsage_KeyboardLeftGUI)},
        {"right_option", key_code(kHIDUsage_KeyboardRightAlt)},
        {"right_command", key_code(kHIDUsage_KeyboardRightGUI)},
        {"japanese_eisuu", key_code(kHIDUsage_KeyboardLANG2)},
        {"japanese_kana", key_code(kHIDUsage_KeyboardLANG1)},
        {"japanese_pc_nfer", key_code(kHIDUsage_KeyboardInternational5)},
        {"japanese_pc_xfer", key_code(kHIDUsage_KeyboardInternational4)},
        {"japanese_pc_katakana", key_code(kHIDUsage_KeyboardInternational2)},
        {"vk_consumer_brightness_down", key_code::display_brightness_decrement},
        {"vk_consumer_brightness_up", key_code::display_brightness_increment},
        {"vk_mission_control", key_code::mission_control},
        {"vk_launchpad", key_code::launchpad},
        {"vk_dashboard", key_code::dashboard},
        {"vk_consumer_illumination_down", key_code::illumination_decrement},
        {"vk_consumer_illumination_up", key_code::illumination_increment},
        {"vk_consumer_previous", key_code::rewind},
        {"vk_consumer_play", key_code::play_or_pause},
        {"vk_consumer_next", key_code::fastforward},
        {"volume_down", key_code(kHIDUsage_KeyboardVolumeDown)},
        {"volume_up", key_code(kHIDUsage_KeyboardVolumeUp)},
    });

    return pairs;
  }

  static const std::unordered_map<std::string, key_code>& get_key_code_name_value_map(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::unordered_map<std::string, key_code> map;

    if (map.empty()) {
      for (const auto& pair : get_key_code_name_value_pairs()) {
        auto it = map.find(pair.first);
        if (it != std::end(map)) {
          logger::get_logger().error("duplicate entry in get_key_code_name_value_pairs: {0}", pair.first);
        } else {
          map.emplace(pair.first, pair.second);
        }
      }
    }

    return map;
  }

  static boost::optional<key_code> make_key_code(const std::string& name) {
    auto& map = get_key_code_name_value_map();
    auto it = map.find(name);
    if (it == map.end()) {
      logger::get_logger().error("unknown key_code: \"{0}\"", name);
      return boost::none;
    }
    return it->second;
  }

  static boost::optional<std::string> make_key_code_name(key_code key_code) {
    for (const auto& pair : get_key_code_name_value_pairs()) {
      if (pair.second == key_code) {
        return pair.first;
      }
    }
    return boost::none;
  }

  static boost::optional<key_code> make_key_code(hid_usage_page usage_page, hid_usage usage) {
    auto u = static_cast<uint32_t>(usage);

    switch (usage_page) {
      case hid_usage_page::keyboard_or_keypad:
        if (kHIDUsage_KeyboardErrorUndefined < u && u < kHIDUsage_Keyboard_Reserved) {
          return key_code(u);
        }
        break;

      case hid_usage_page::apple_vendor_top_case:
        switch (usage) {
          case hid_usage::av_top_case_keyboard_fn:
            return key_code::fn;
          case hid_usage::av_top_case_brightness_up:
            return key_code::apple_top_case_display_brightness_increment;
          case hid_usage::av_top_case_brightness_down:
            return key_code::apple_top_case_display_brightness_decrement;
          case hid_usage::av_top_case_illumination_up:
            return key_code::illumination_increment;
          case hid_usage::av_top_case_illumination_down:
            return key_code::illumination_decrement;
          default:
            break;
        }
        break;

      case hid_usage_page::apple_vendor_keyboard:
        switch (usage) {
          case hid_usage::apple_vendor_keyboard_dashboard:
            return key_code::dashboard;
          case hid_usage::apple_vendor_keyboard_function:
            return key_code::fn;
          case hid_usage::apple_vendor_keyboard_launchpad:
            return key_code::launchpad;
          case hid_usage::apple_vendor_keyboard_expose_all:
            return key_code::mission_control;
          case hid_usage::apple_vendor_keyboard_brightness_up:
            return key_code::apple_display_brightness_increment;
          case hid_usage::apple_vendor_keyboard_brightness_down:
            return key_code::apple_display_brightness_decrement;
          default:
            break;
        }
        break;

      default:
        break;
    }

    return boost::none;
  }

  static boost::optional<key_code> make_key_code(const hid_value& hid_value) {
    if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
      if (auto hid_usage = hid_value.get_hid_usage()) {
        return make_key_code(*hid_usage_page,
                             *hid_usage);
      }
    }
    return boost::none;
  }

  static boost::optional<hid_usage_page> make_hid_usage_page(key_code key_code) {
    switch (key_code) {
      case key_code::fn:
      case key_code::illumination_decrement:
      case key_code::illumination_increment:
      case key_code::apple_top_case_display_brightness_decrement:
      case key_code::apple_top_case_display_brightness_increment:
        return hid_usage_page::apple_vendor_top_case;

      case key_code::dashboard:
      case key_code::launchpad:
      case key_code::mission_control:
      case key_code::apple_display_brightness_decrement:
      case key_code::apple_display_brightness_increment:
        return hid_usage_page::apple_vendor_keyboard;

      case key_code::mute:
      case key_code::volume_decrement:
      case key_code::volume_increment:
      case key_code::display_brightness_decrement:
      case key_code::display_brightness_increment:
      case key_code::rewind:
      case key_code::play_or_pause:
      case key_code::fastforward:
      case key_code::eject:
        return hid_usage_page::consumer;

      case key_code::vk_none:
        return boost::none;

      default:
        return hid_usage_page::keyboard_or_keypad;
    }
  }

  static boost::optional<hid_usage> make_hid_usage(key_code key_code) {
    switch (key_code) {
      case key_code::fn:
        return hid_usage::av_top_case_keyboard_fn;

      case key_code::illumination_decrement:
        return hid_usage::av_top_case_illumination_down;

      case key_code::illumination_increment:
        return hid_usage::av_top_case_illumination_up;

      case key_code::apple_top_case_display_brightness_decrement:
        return hid_usage::av_top_case_brightness_down;

      case key_code::apple_top_case_display_brightness_increment:
        return hid_usage::av_top_case_brightness_up;

      case key_code::dashboard:
        return hid_usage::apple_vendor_keyboard_dashboard;

      case key_code::launchpad:
        return hid_usage::apple_vendor_keyboard_launchpad;

      case key_code::mission_control:
        return hid_usage::apple_vendor_keyboard_expose_all;

      case key_code::apple_display_brightness_decrement:
        return hid_usage::apple_vendor_keyboard_brightness_down;

      case key_code::apple_display_brightness_increment:
        return hid_usage::apple_vendor_keyboard_brightness_up;

      case key_code::mute:
        return hid_usage::csmr_mute;

      case key_code::volume_decrement:
        return hid_usage::csmr_volume_decrement;

      case key_code::volume_increment:
        return hid_usage::csmr_volume_increment;

      case key_code::display_brightness_decrement:
        return hid_usage::csmr_display_brightness_decrement;

      case key_code::display_brightness_increment:
        return hid_usage::csmr_display_brightness_increment;

      case key_code::rewind:
        return hid_usage::csmr_rewind;

      case key_code::play_or_pause:
        return hid_usage::csmr_play_or_pause;

      case key_code::fastforward:
        return hid_usage::csmr_fastforward;

      case key_code::eject:
        return hid_usage::csmr_eject;

      case key_code::vk_none:
        return boost::none;

      default:
        return hid_usage(key_code);
    }
  }

  static const std::vector<std::pair<std::string, consumer_key_code>>& get_consumer_key_code_name_value_pairs(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::vector<std::pair<std::string, consumer_key_code>> pairs({
        {"power", consumer_key_code::power},
        {"display_brightness_increment", consumer_key_code::display_brightness_increment},
        {"display_brightness_decrement", consumer_key_code::display_brightness_decrement},
        {"fastforward", consumer_key_code::fastforward},
        {"rewind", consumer_key_code::rewind},
        {"scan_next_track", consumer_key_code::scan_next_track},
        {"scan_previous_track", consumer_key_code::scan_previous_track},
        {"eject", consumer_key_code::eject},
        {"play_or_pause", consumer_key_code::play_or_pause},
        {"mute", consumer_key_code::mute},
        {"volume_increment", consumer_key_code::volume_increment},
        {"volume_decrement", consumer_key_code::volume_decrement},
    });

    return pairs;
  }

  static const std::unordered_map<std::string, consumer_key_code>& get_consumer_key_code_name_value_map(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::unordered_map<std::string, consumer_key_code> map;

    if (map.empty()) {
      for (const auto& pair : get_consumer_key_code_name_value_pairs()) {
        auto it = map.find(pair.first);
        if (it != std::end(map)) {
          logger::get_logger().error("duplicate entry in get_consumer_key_code_name_value_pairs: {0}", pair.first);
        } else {
          map.emplace(pair.first, pair.second);
        }
      }
    }

    return map;
  }

  static boost::optional<consumer_key_code> make_consumer_key_code(const std::string& name) {
    auto& map = get_consumer_key_code_name_value_map();
    auto it = map.find(name);
    if (it == map.end()) {
      logger::get_logger().error("unknown consumer_key_code: \"{0}\"", name);
      return boost::none;
    }
    return it->second;
  }

  static boost::optional<std::string> make_consumer_key_code_name(consumer_key_code consumer_key_code) {
    for (const auto& pair : get_consumer_key_code_name_value_pairs()) {
      if (pair.second == consumer_key_code) {
        return pair.first;
      }
    }
    return boost::none;
  }

  static boost::optional<consumer_key_code> make_consumer_key_code(hid_usage_page usage_page, hid_usage usage) {
    auto u = static_cast<uint32_t>(usage);

    switch (usage_page) {
      case hid_usage_page::consumer:
        switch (consumer_key_code(u)) {
          case consumer_key_code::power:
          case consumer_key_code::display_brightness_increment:
          case consumer_key_code::display_brightness_decrement:
          case consumer_key_code::fastforward:
          case consumer_key_code::rewind:
          case consumer_key_code::scan_next_track:
          case consumer_key_code::scan_previous_track:
          case consumer_key_code::eject:
          case consumer_key_code::play_or_pause:
          case consumer_key_code::mute:
          case consumer_key_code::volume_increment:
          case consumer_key_code::volume_decrement:
            return consumer_key_code(u);
        }

      default:
        break;
    }

    return boost::none;
  }

  static boost::optional<consumer_key_code> make_consumer_key_code(const hid_value& hid_value) {
    if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
      if (auto hid_usage = hid_value.get_hid_usage()) {
        return make_consumer_key_code(*hid_usage_page,
                                      *hid_usage);
      }
    }
    return boost::none;
  }

  static boost::optional<hid_usage_page> make_hid_usage_page(consumer_key_code consumer_key_code) {
    return hid_usage_page::consumer;
  }

  static boost::optional<hid_usage> make_hid_usage(consumer_key_code consumer_key_code) {
    return hid_usage(static_cast<uint32_t>(consumer_key_code));
  }

  static const std::vector<std::pair<std::string, pointing_button>>& get_pointing_button_name_value_pairs(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::vector<std::pair<std::string, pointing_button>> pairs({
        // From IOHIDUsageTables.h

        {"button1", pointing_button::button1},
        {"button2", pointing_button::button2},
        {"button3", pointing_button::button3},
        {"button4", pointing_button::button4},
        {"button5", pointing_button::button5},
        {"button6", pointing_button::button6},
        {"button7", pointing_button::button7},
        {"button8", pointing_button::button8},

        {"button9", pointing_button::button9},
        {"button10", pointing_button::button10},
        {"button11", pointing_button::button11},
        {"button12", pointing_button::button12},
        {"button13", pointing_button::button13},
        {"button14", pointing_button::button14},
        {"button15", pointing_button::button15},
        {"button16", pointing_button::button16},

        {"button17", pointing_button::button17},
        {"button18", pointing_button::button18},
        {"button19", pointing_button::button19},
        {"button20", pointing_button::button20},
        {"button21", pointing_button::button21},
        {"button22", pointing_button::button22},
        {"button23", pointing_button::button23},
        {"button24", pointing_button::button24},

        {"button25", pointing_button::button25},
        {"button26", pointing_button::button26},
        {"button27", pointing_button::button27},
        {"button28", pointing_button::button28},
        {"button29", pointing_button::button29},
        {"button30", pointing_button::button30},
        {"button31", pointing_button::button31},
        {"button32", pointing_button::button32},
    });

    return pairs;
  }

  static const std::unordered_map<std::string, pointing_button>& get_pointing_button_name_value_map(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::unordered_map<std::string, pointing_button> map;

    if (map.empty()) {
      for (const auto& pair : get_pointing_button_name_value_pairs()) {
        auto it = map.find(pair.first);
        if (it != std::end(map)) {
          logger::get_logger().error("duplicate entry in get_pointing_button_name_value_pairs: {0}", pair.first);
        } else {
          map.emplace(pair.first, pair.second);
        }
      }
    }

    return map;
  }

  static boost::optional<pointing_button> make_pointing_button(const std::string& name) {
    auto& map = get_pointing_button_name_value_map();
    auto it = map.find(name);
    if (it == map.end()) {
      logger::get_logger().error("unknown pointing_button: \"{0}\"", name);
      return boost::none;
    }
    return it->second;
  }

  static boost::optional<std::string> make_pointing_button_name(pointing_button pointing_button) {
    for (const auto& pair : get_pointing_button_name_value_pairs()) {
      if (pair.second == pointing_button) {
        return pair.first;
      }
    }
    return boost::none;
  }

  static boost::optional<pointing_button> make_pointing_button(hid_usage_page usage_page, hid_usage usage) {
    if (usage_page == hid_usage_page::button) {
      return pointing_button(usage);
    }
    return boost::none;
  }

  static boost::optional<pointing_button> make_pointing_button(const hid_value& hid_value) {
    if (auto hid_usage_page = hid_value.get_hid_usage_page()) {
      if (auto hid_usage = hid_value.get_hid_usage()) {
        return make_pointing_button(*hid_usage_page,
                                    *hid_usage);
      }
    }
    return boost::none;
  }

private:
  static std::unordered_map<device_id, std::shared_ptr<device_detail>>& get_device_id_map(void) {
    static std::unordered_map<device_id, std::shared_ptr<device_detail>> map;
    return map;
  }
};

struct operation_type_connect_struct {
  operation_type_connect_struct(void) : operation_type(operation_type::connect) {
    strlcpy(user_core_configuration_file_path,
            constants::get_user_core_configuration_file_path().c_str(),
            sizeof(user_core_configuration_file_path));
  }

  const operation_type operation_type;
  pid_t pid;
  char user_core_configuration_file_path[_POSIX_PATH_MAX];
};

struct operation_type_system_preferences_updated_struct {
  operation_type_system_preferences_updated_struct(void) : operation_type(operation_type::system_preferences_updated) {}

  const operation_type operation_type;
  system_preferences system_preferences;
};

struct operation_type_frontmost_application_changed_struct {
  operation_type_frontmost_application_changed_struct(void) : operation_type(operation_type::frontmost_application_changed) {
    bundle_identifier[0] = '\0';
    file_path[0] = '\0';
  }

  const operation_type operation_type;
  char bundle_identifier[256];
  char file_path[_POSIX_PATH_MAX];
};

struct operation_type_input_source_changed_struct {
  operation_type_input_source_changed_struct(void) : operation_type(operation_type::input_source_changed) {
    language[0] = '\0';
    input_source_id[0] = '\0';
    input_mode_id[0] = '\0';
  }

  const operation_type operation_type;
  char language[256];
  char input_source_id[256];
  char input_mode_id[256];
};

struct operation_type_shell_command_execution_struct {
  operation_type_shell_command_execution_struct(void) : operation_type(operation_type::shell_command_execution) {
    shell_command[0] = '\0';
  }

  const operation_type operation_type;
  char shell_command[256];
};

struct operation_type_select_input_source_struct {
  operation_type_select_input_source_struct(void) : operation_type(operation_type::select_input_source) {
    language[0] = '\0';
    input_source_id[0] = '\0';
    input_mode_id[0] = '\0';
  }

  const operation_type operation_type;
  uint64_t time_stamp;
  char language[256];
  char input_source_id[256];
  char input_mode_id[256];
};

// stream output

#define KRBN_TYPES_STREAM_OUTPUT(TYPE)                                                                                                               \
  inline std::ostream& operator<<(std::ostream& stream, const TYPE& value) {                                                                         \
    return stream_utility::output_enum(stream, value);                                                                                               \
  }                                                                                                                                                  \
                                                                                                                                                     \
  template <template <class T, class A> class container>                                                                                             \
  inline std::ostream& operator<<(std::ostream& stream, const container<TYPE, std::allocator<TYPE>>& values) {                                       \
    return stream_utility::output_enums(stream, values);                                                                                             \
  }                                                                                                                                                  \
                                                                                                                                                     \
  template <template <class T, class H, class K, class A> class container>                                                                           \
  inline std::ostream& operator<<(std::ostream& stream, const container<TYPE, std::hash<TYPE>, std::equal_to<TYPE>, std::allocator<TYPE>>& values) { \
    return stream_utility::output_enums(stream, values);                                                                                             \
  }

KRBN_TYPES_STREAM_OUTPUT(device_id);
KRBN_TYPES_STREAM_OUTPUT(hid_usage_page);
KRBN_TYPES_STREAM_OUTPUT(hid_usage);
KRBN_TYPES_STREAM_OUTPUT(key_code);
KRBN_TYPES_STREAM_OUTPUT(consumer_key_code);
KRBN_TYPES_STREAM_OUTPUT(pointing_button);
KRBN_TYPES_STREAM_OUTPUT(vendor_id);
KRBN_TYPES_STREAM_OUTPUT(product_id);
KRBN_TYPES_STREAM_OUTPUT(location_id);

#undef KRBN_TYPES_STREAM_OUTPUT

inline std::ostream& operator<<(std::ostream& stream, const event_type& value) {
  switch (value) {
    case event_type::key_down:
      stream << "key_down";
      break;
    case event_type::key_up:
      stream << "key_up";
      break;
    case event_type::single:
      stream << "single";
      break;
  }

  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const modifier_flag& value) {
#define KRBN_MODIFIER_FLAG_STREAM_OUTPUT(NAME) \
  case modifier_flag::NAME:                    \
    stream << #NAME;                           \
    break;

  switch (value) {
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(zero);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(caps_lock);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(left_control);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(left_shift);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(left_option);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(left_command);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(right_control);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(right_shift);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(right_option);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(right_command);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(fn);
    KRBN_MODIFIER_FLAG_STREAM_OUTPUT(end_);
  }

#undef KRBN_MODIFIER_FLAG_STREAM_OUTPUT

  return stream;
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<modifier_flag, std::allocator<modifier_flag>>& values) {
  return stream_utility::output_enums(stream, values);
}

template <template <class T, class H, class K, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<modifier_flag, std::hash<modifier_flag>, std::equal_to<modifier_flag>, std::allocator<modifier_flag>>& values) {
  return stream_utility::output_enums(stream, values);
}

inline std::ostream& operator<<(std::ostream& stream, const pointing_motion& value) {
  stream << "pointing_motion:" << value.to_json();
  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const input_source_identifiers& value) {
  stream << "language:";

  if (auto& v = value.get_language()) {
    stream << *v;
  } else {
    stream << "---";
  }

  stream << ",input_source_id:";

  if (auto& v = value.get_input_source_id()) {
    stream << *v;
  } else {
    stream << "---";
  }

  stream << ",input_mode_id:";

  if (auto& v = value.get_input_mode_id()) {
    stream << *v;
  } else {
    stream << "---";
  }

  return stream;
}

inline std::ostream& operator<<(std::ostream& stream, const input_source_selector& value) {
  stream << "language:";

  if (auto& v = value.get_language_string()) {
    stream << *v;
  } else {
    stream << "---";
  }

  stream << ",input_source_id:";

  if (auto& v = value.get_input_source_id_string()) {
    stream << *v;
  } else {
    stream << "---";
  }

  stream << ",input_mode_id:";

  if (auto& v = value.get_input_mode_id_string()) {
    stream << *v;
  } else {
    stream << "---";
  }

  return stream;
}

template <template <class T, class A> class container>
inline std::ostream& operator<<(std::ostream& stream, const container<input_source_selector, std::allocator<input_source_selector>>& values) {
  bool first = true;
  stream << "[";
  for (const auto& v : values) {
    if (first) {
      first = false;
    } else {
      stream << ",";
    }
    stream << v;
  }
  stream << "]";
  return stream;
}

inline void to_json(nlohmann::json& json, const event_type& value) {
  switch (value) {
    case event_type::key_down:
      json = "key_down";
      break;
    case event_type::key_up:
      json = "key_up";
      break;
    case event_type::single:
      json = "single";
      break;
  }
}

inline void from_json(const nlohmann::json& json, event_type& value) {
  auto s = json.get<std::string>();

  if (s == "key_up") {
    value = event_type::key_up;
  } else if (s == "single") {
    value = event_type::single;
  } else {
    value = event_type::key_down;
  }
}

inline void to_json(nlohmann::json& json, const pointing_motion& pointing_motion) {
  json = pointing_motion.to_json();
}

inline void to_json(nlohmann::json& json, const device_identifiers& identifiers) {
  json = identifiers.to_json();
}

inline void to_json(nlohmann::json& json, const input_source_selector& input_source_selector) {
  json = input_source_selector.to_json();
}
} // namespace krbn

namespace std {
template <>
struct hash<krbn::pointing_motion> final {
  std::size_t operator()(const krbn::pointing_motion& v) const {
    return hash_value(v);
  }
};

template <>
struct hash<krbn::input_source_identifiers> final {
  std::size_t operator()(const krbn::input_source_identifiers& v) const {
    return hash_value(v);
  }
};

template <>
struct hash<krbn::input_source_selector> final {
  std::size_t operator()(const krbn::input_source_selector& v) const {
    return hash_value(v);
  }
};

template <>
struct hash<krbn::mouse_key> final {
  std::size_t operator()(const krbn::mouse_key& v) const {
    return hash_value(v);
  }
};
} // namespace std
