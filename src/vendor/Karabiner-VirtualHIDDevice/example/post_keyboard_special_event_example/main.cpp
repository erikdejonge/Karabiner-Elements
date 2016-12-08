#include "karabiner_virtual_hid_device_methods.hpp"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDDevice.h>
#include <IOKit/hid/IOHIDElement.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hid/IOHIDQueue.h>
#include <IOKit/hid/IOHIDValue.h>
#include <IOKit/hidsystem/IOHIDShared.h>
#include <IOKit/hidsystem/ev_keymap.h>
#include <cmath>
#include <iostream>
#include <thread>

int main(int argc, const char* argv[]) {
  if (getuid() != 0) {
    std::cerr << "post_keyboard_special_event_example requires root privilege." << std::endl;
  }

  kern_return_t kr;
  io_connect_t connect = IO_OBJECT_NULL;
  auto service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceNameMatching(pqrs::karabiner_virtual_hid_device::get_virtual_hid_root_name()));
  if (!service) {
    std::cerr << "IOServiceGetMatchingService error" << std::endl;
    goto finish;
  }

  kr = IOServiceOpen(service, mach_task_self(), kIOHIDServerConnectType, &connect);
  if (kr != KERN_SUCCESS) {
    std::cerr << "IOServiceOpen error" << std::endl;
    goto finish;
  }

  for (int i = 0; i < 2; ++i) {
    pqrs::karabiner_virtual_hid_device::keyboard_special_event keyboard_special_event;

    switch (i % 2) {
    case 0:
      keyboard_special_event.event_type = pqrs::karabiner_virtual_hid_device::event_type::key_down;
      keyboard_special_event.flavor = 6; // power key
      break;
    case 1:
      keyboard_special_event.event_type = pqrs::karabiner_virtual_hid_device::event_type::key_up;
      keyboard_special_event.flavor = 6; // power key
      break;
    }

    kr = pqrs::karabiner_virtual_hid_device_methods::post_keyboard_special_event(connect, keyboard_special_event);
    if (kr != KERN_SUCCESS) {
      std::cerr << "post_keyboard_special_event error" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

finish:
  if (connect) {
    IOServiceClose(connect);
  }
  if (service) {
    IOObjectRelease(service);
  }

  return 0;
}
