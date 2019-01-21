#include "constants.hpp"
#include "dispatcher_utility.hpp"
#include "logger.hpp"
#include "monitor/configuration_monitor.hpp"
#include "monitor/system_preferences_monitor.hpp"
#include <iostream>

int main(int argc, const char* argv[]) {
  krbn::dispatcher_utility::initialize_dispatchers();

  signal(SIGINT, [](int) {
    CFRunLoopStop(CFRunLoopGetMain());
  });

  auto configuration_monitor = std::make_shared<krbn::configuration_monitor>(krbn::constants::get_user_core_configuration_file_path());

  auto monitor = std::make_unique<krbn::system_preferences_monitor>(configuration_monitor);

  monitor->system_preferences_changed.connect([](auto&& system_preferences) {
    std::cout << "system_preferences_updated_callback:" << std::endl;
    std::cout << "  com.apple.keyboard.fnState: " << system_preferences.get_keyboard_fn_state() << std::endl;
    std::cout << "  com.apple.swipescrolldirection: " << system_preferences.get_swipe_scroll_direction() << std::endl;
    std::cout << "  keyboard_type: " << static_cast<int>(system_preferences.get_keyboard_type()) << std::endl;
  });

  monitor->async_start();

  configuration_monitor->async_start();

  CFRunLoopRun();

  configuration_monitor = nullptr;

  monitor = nullptr;

  krbn::dispatcher_utility::terminate_dispatchers();

  return 0;
}
