#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "dispatcher_utility.hpp"
#include "monitor/grabber_alerts_monitor.hpp"

TEST_CASE("initialize") {
  krbn::dispatcher_utility::initialize_dispatchers();
}

TEST_CASE("grabber_alerts_monitor") {
  {
    system("rm -rf target");
    system("mkdir -p target");

    std::string file_path = "target/karabiner_grabber_alerts.json";
    krbn::grabber_alerts_monitor grabber_alerts_monitor(file_path);

    std::optional<std::string> last_alerts;

    grabber_alerts_monitor.alerts_changed.connect([&](auto&& alerts) {
      last_alerts = alerts->dump();
    });

    grabber_alerts_monitor.async_start();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    REQUIRE(!last_alerts);

    // ========================================
    // Empty alerts
    // ========================================

    {
      last_alerts = std::nullopt;

      auto json = nlohmann::json::object({{"alerts", nlohmann::json::array()}});

      system(fmt::format("echo '{0}' > {1}", json.dump(), file_path).c_str());

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      REQUIRE(last_alerts);
      REQUIRE(*last_alerts == json["alerts"].dump());
    }

    // ========================================
    // Alerts
    // ========================================

    {
      last_alerts = std::nullopt;

      auto json = nlohmann::json::object({{"alerts", nlohmann::json::array({"example"})}});

      system(fmt::format("echo '{0}' > {1}", json.dump(), file_path).c_str());

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      REQUIRE(last_alerts);
      REQUIRE(*last_alerts == json["alerts"].dump());
    }

    // ========================================
    // Broken json
    // ========================================

    {
      last_alerts = std::nullopt;

      system(fmt::format("echo '[' > {0}", file_path).c_str());

      std::this_thread::sleep_for(std::chrono::milliseconds(500));

      REQUIRE(!last_alerts);
    }
  }
}

TEST_CASE("terminate") {
  krbn::dispatcher_utility::terminate_dispatchers();
}
