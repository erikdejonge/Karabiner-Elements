#pragma once

#include <boost/signals2.hpp>

namespace krbn {
class krbn_notification_center final {
public:
  class core final {
  public:
    boost::signals2::signal<void(void)> input_event_arrived;
  };

  static core& get_instance(void) {
    static std::mutex mutex;
    std::lock_guard<std::mutex> guard(mutex);

    static std::unique_ptr<core> core_;
    if (!core_) {
      core_ = std::make_unique<core>();
    }

    return *core_;
  }
};
} // namespace krbn
