#include "DeviceCatalog.hpp"

#include <Devices.hpp>
#include <cassert>
#include <cstring>

namespace DeviceCatalog {

const std::vector<NamedDeviceDescription> &All() {
  static const std::vector<NamedDeviceDescription> devices = {
      {"bike", &Devices::bike},
      {"ben_s_bike", &Devices::ben_s_bike},
      {"will_bike", &Devices::will_bike},
      {"scarf", &Devices::scarf},
      {"lantern", &Devices::lantern},
      {"puck", &Devices::puck},
      {"two_side_puck", &Devices::two_side_puck},
      {"rainbow_cloak", &Devices::rainbow_cloak},
      {"backpack_tail", &Devices::backpack_tail},
      {"dan_jacket", &Devices::dan_jacket},
      {"will_jacket", &Devices::will_jacket},
      {"will_bike_front", &Devices::will_bike_front},
      {"will_top_hat", &Devices::will_top_hat},
      {"bike_front", &Devices::bike_front},
      {"hex_light", &Devices::hex_light},
      {"half_matrix_panel", &Devices::half_matrix_panel},
      {"backpack_rope", &Devices::backpack_rope},
      {"ufo", &Devices::ufo},
      {"brooke_bike", &Devices::brooke_bike},
      {"ross_backpack", &Devices::ross_backpack},
      {"whatever", &Devices::whatever},
      {"will_backpack", &Devices::will_backpack},
  };
#ifndef ARDUINO
  static const bool devices_are_valid = [] {
    for (size_t i = 0; i < devices.size(); ++i) {
      assert(devices[i].name != nullptr && devices[i].name[0] != '\0');
      assert(devices[i].description != nullptr);
      for (size_t j = i + 1; j < devices.size(); ++j) {
        assert(std::strcmp(devices[i].name, devices[j].name) != 0);
      }
    }
    return true;
  }();
  (void)devices_are_valid;
#endif
  return devices;
}

const NamedDeviceDescription *Find(const char *name) {
  if (name == nullptr) {
    return nullptr;
  }
  for (const NamedDeviceDescription &device : All()) {
    if (std::strcmp(device.name, name) == 0) {
      return &device;
    }
  }
  return nullptr;
}

}  // namespace DeviceCatalog
