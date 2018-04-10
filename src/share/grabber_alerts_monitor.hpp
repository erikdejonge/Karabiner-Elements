#pragma once

#include "constants.hpp"
#include "file_monitor.hpp"
#include "filesystem.hpp"
#include "gcd_utility.hpp"
#include "json_utility.hpp"
#include "logger.hpp"
#include <fstream>

namespace krbn {
class grabber_alerts_monitor final {
public:
  typedef std::function<void(void)> callback;

  grabber_alerts_monitor(const grabber_alerts_monitor&) = delete;

  grabber_alerts_monitor(const callback& callback) : callback_(callback) {
    auto file_path = constants::get_grabber_alerts_json_file_path();
    auto directory = filesystem::dirname(file_path);

    std::vector<std::pair<std::string, std::vector<std::string>>> targets = {
        {directory, {file_path}},
    };
    file_monitor_ = std::make_unique<file_monitor>(targets,
                                                   [this](const std::string&) {
                                                     auto file_path = constants::get_grabber_alerts_json_file_path();
                                                     std::ifstream stream(file_path);
                                                     if (stream) {
                                                       try {
                                                         auto json = nlohmann::json::parse(stream);

                                                         // json example
                                                         //
                                                         // {
                                                         //     "alerts": [
                                                         //         "system_policy_prevents_loading_kext"
                                                         //     ]
                                                         // }

                                                         if (auto v = json_utility::find_array(json, "alerts")) {
                                                           if (!v->empty()) {
                                                             auto s = json.dump();
                                                             if (json_string_ != s) {
                                                               json_string_ = s;
                                                               if (callback_) {
                                                                 callback_();
                                                               }
                                                             }
                                                           }
                                                         }
                                                       } catch (std::exception& e) {
                                                         logger::get_logger().error("parse error in {0}: {1}", file_path, e.what());
                                                       }
                                                     }
                                                   });
  }

  ~grabber_alerts_monitor(void) {
    gcd_utility::dispatch_sync_in_main_queue(^{
      file_monitor_ = nullptr;
    });
  }

private:
  callback callback_;

  std::string json_string_;
  std::unique_ptr<file_monitor> file_monitor_;
};
} // namespace krbn
