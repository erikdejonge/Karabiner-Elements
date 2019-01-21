#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include "pressed_keys_manager.hpp"

TEST_CASE("pressed_keys_manager") {
  krbn::pressed_keys_manager manager;

  REQUIRE(manager.empty());

  manager.insert(krbn::key_code::a);
  REQUIRE(!manager.empty());

  manager.insert(krbn::key_code::a);
  REQUIRE(!manager.empty());

  manager.insert(krbn::consumer_key_code::mute);
  REQUIRE(!manager.empty());

  manager.insert(krbn::pointing_button::button1);
  REQUIRE(!manager.empty());

  manager.erase(krbn::key_code::a);
  REQUIRE(!manager.empty());

  manager.erase(krbn::consumer_key_code::mute);
  REQUIRE(!manager.empty());

  manager.erase(krbn::pointing_button::button10);
  REQUIRE(!manager.empty());

  manager.erase(krbn::pointing_button::button1);
  REQUIRE(manager.empty());
}
