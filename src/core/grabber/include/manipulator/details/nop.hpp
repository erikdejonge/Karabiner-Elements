#pragma once

#include "manipulator/details/base.hpp"
#include "manipulator/details/types.hpp"

namespace krbn {
namespace manipulator {
namespace details {
class nop final : public base {
public:
  nop(void) : base() {
  }

  virtual ~nop(void) {
  }

  virtual bool already_manipulated(const event_queue::queued_event& front_input_event) {
    return false;
  }

  virtual manipulate_result manipulate(event_queue::queued_event& front_input_event,
                                       const event_queue& input_event_queue,
                                       const std::shared_ptr<event_queue>& output_event_queue,
                                       uint64_t now) {
    return manipulate_result::passed;
  }

  virtual bool active(void) const {
    return false;
  }

  virtual bool needs_virtual_hid_pointing(void) const {
    return false;
  }

  virtual void handle_device_keys_and_pointing_buttons_are_released_event(const event_queue::queued_event& front_input_event,
                                                                          event_queue& output_event_queue) {
  }

  virtual void handle_device_ungrabbed_event(device_id device_id,
                                             const event_queue& output_event_queue,
                                             uint64_t time_stamp) {
  }

  virtual void handle_event_from_ignored_device(const event_queue::queued_event& front_input_event,
                                                event_queue& output_event_queue) {
  }

  virtual void handle_pointing_device_event_from_event_tap(const event_queue::queued_event& front_input_event,
                                                           event_queue& output_event_queue) {
  }

  virtual void manipulator_timer_invoked(manipulator_timer::timer_id timer_id, uint64_t now) {
  }
};
} // namespace details
} // namespace manipulator
} // namespace krbn
