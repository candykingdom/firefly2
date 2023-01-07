#ifndef __DEVICES_HPP__
#define __DEVICES_HPP__

#include <DeviceDescription.hpp>
#include <Types.hpp>

namespace Devices {

const uint32_t RF_BOARD_MA_SUPPORTED = 2400 - 50;

const DeviceDescription *SimpleRfBoardDescription(
    uint8_t led_count, std::vector<StripFlag> flags) {
  return new DeviceDescription(RF_BOARD_MA_SUPPORTED,
                               {
                                   new StripDescription(led_count, flags),
                               });
}

const DeviceDescription *const bike = SimpleRfBoardDescription(30, {Bright});
const DeviceDescription *const will_bike =
    SimpleRfBoardDescription(63, {Bright});
const DeviceDescription *const scarf = SimpleRfBoardDescription(46, {});
const DeviceDescription *const lantern = SimpleRfBoardDescription(5, {Tiny});
const DeviceDescription *const puck =
    SimpleRfBoardDescription(12, {Tiny, Circular});
const DeviceDescription *const two_side_puck =
    SimpleRfBoardDescription(24, {Tiny, Circular, Mirrored});
const DeviceDescription *const rainbow_cloak = new DeviceDescription(
    RF_BOARD_MA_SUPPORTED,
    {
        new StripDescription(11, {Tiny, Circular}),
        new StripDescription(94, {}),
        new StripDescription(11, {Tiny, Circular, Reversed}),
    });
const DeviceDescription *const backpack_tail = SimpleRfBoardDescription(11, {});
const DeviceDescription *const dan_jacket = SimpleRfBoardDescription(60, {});
const DeviceDescription *const will_jacket = SimpleRfBoardDescription(56, {});
const DeviceDescription *const will_bike_front =
    SimpleRfBoardDescription(27, {Circular});
const DeviceDescription *const will_top_hat =
    SimpleRfBoardDescription(50, {Circular});
const DeviceDescription *const bike_front =
    SimpleRfBoardDescription(18, {Circular});
const DeviceDescription *const hex_light =
    SimpleRfBoardDescription(12, {Circular, Tiny});

// Modify this variable to easily switch between devices.
const DeviceDescription *const current = scarf;

}  // namespace Devices

#endif  // __DEVICES_HPP__
