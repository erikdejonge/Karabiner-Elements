#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "../share/json_helper.hpp"
#include "dispatcher_utility.hpp"
#include "grabber_alerts_manager.hpp"
#include <unistd.h>

TEST_CASE("initialize") {
  krbn::dispatcher_utility::initialize_dispatchers();
}

TEST_CASE("set_alert") {
  using namespace std::string_literals;

  std::vector<std::string> file_names{
      "grabber_alerts_manager0.json"s,
      "grabber_alerts_manager1.json"s,
      "grabber_alerts_manager2.json"s,
  };

  for (const auto& file_name : file_names) {
    auto tmp_file_path = "tmp/"s + file_name;
    unlink(tmp_file_path.c_str());
  }

  {
    auto grabber_alerts_manager = std::make_unique<krbn::grabber_alerts_manager>(
        "tmp/grabber_alerts_manager0.json");
    grabber_alerts_manager->async_save_to_file();
  }

  {
    auto grabber_alerts_manager = std::make_unique<krbn::grabber_alerts_manager>(
        "tmp/grabber_alerts_manager1.json");
    grabber_alerts_manager->set_alert(krbn::grabber_alerts_manager::alert::system_policy_prevents_loading_kext, true);
  }

  {
    auto grabber_alerts_manager = std::make_unique<krbn::grabber_alerts_manager>(
        "tmp/grabber_alerts_manager2.json");
    grabber_alerts_manager->set_alert(krbn::grabber_alerts_manager::alert::system_policy_prevents_loading_kext, false);
  }

  krbn::async_file_writer::wait();

  for (const auto& file_name : file_names) {
    REQUIRE(krbn::unit_testing::json_helper::compare_files("tmp/"s + file_name,
                                                           "expected/"s + file_name));
  }
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
