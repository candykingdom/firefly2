#ifndef __DEVICES_HPP__
#define __DEVICES_HPP__

#include <DeviceDescription.hpp>
#include <Types.hpp>

namespace Devices {

const uint32_t RF_BOARD_MA_SUPPORTED = 2400 - 50;

const DeviceDescription SimpleRfBoardDescription(uint8_t led_count,
                                                 std::vector<StripFlag> flags) {
  return DeviceDescription(RF_BOARD_MA_SUPPORTED,
                           {
                               StripDescription(led_count, flags),
                           });
}

const DeviceDescription bike = SimpleRfBoardDescription(30, {Bright});
const DeviceDescription ben_s_bike = SimpleRfBoardDescription(28, {Bright});
const DeviceDescription will_bike = SimpleRfBoardDescription(63, {Bright});
const DeviceDescription scarf = SimpleRfBoardDescription(46, {});
const DeviceDescription lantern = SimpleRfBoardDescription(5, {Tiny});
const DeviceDescription puck = SimpleRfBoardDescription(12, {Tiny, Circular});
const DeviceDescription two_side_puck =
    SimpleRfBoardDescription(24, {Tiny, Circular, Mirrored});
const DeviceDescription rainbow_cloak = DeviceDescription(
    RF_BOARD_MA_SUPPORTED, {
                               StripDescription(11, {Tiny, Circular}),
                               StripDescription(94, {}),
                               StripDescription(11, {Tiny, Circular, Reversed}),
                           });
const DeviceDescription backpack_tail = SimpleRfBoardDescription(11, {});
const DeviceDescription dan_jacket = SimpleRfBoardDescription(60, {});
const DeviceDescription will_jacket = SimpleRfBoardDescription(56, {});
const DeviceDescription will_bike_front =
    SimpleRfBoardDescription(27, {Circular});
const DeviceDescription will_top_hat = SimpleRfBoardDescription(50, {Circular});
const DeviceDescription bike_front = SimpleRfBoardDescription(18, {Circular});
const DeviceDescription hex_light =
    SimpleRfBoardDescription(12, {Circular, Tiny});
const DeviceDescription half_matrix_panel = DeviceDescription(
    RF_BOARD_MA_SUPPORTED, {
                               StripDescription(16, {}),
                               StripDescription(16, {Reversed}),
                               StripDescription(16, {}),
                               StripDescription(16, {Reversed}),
                               StripDescription(16, {}),
                               StripDescription(16, {Reversed}),
                               StripDescription(16, {}),
                               StripDescription(16, {Reversed}),
                           });
const DeviceDescription backpack_rope = DeviceDescription(
    RF_BOARD_MA_SUPPORTED, {
                               StripDescription(96, {Dim}),
                               StripDescription(96, {Dim, Reversed}),
                           });
const DeviceDescription ufo = DeviceDescription(
    RF_BOARD_MA_SUPPORTED,
    {
        StripDescription(12, {Circular}),          // Top Circle
        StripDescription(16, {Circular}),          // Bottom Circle
        StripDescription(12, {Bright, Circular}),  // Bottom Lights
        StripDescription(52, {Dim, Circular}),     // Rim
    });
const DeviceDescription brooke_bike = DeviceDescription(
    RF_BOARD_MA_SUPPORTED, {
                               StripDescription(15, {Circular}),
                               StripDescription(19, {}),
                               StripDescription(16, {Circular}),
                               StripDescription(33, {Bright}),
                           });
const DeviceDescription ross_backpack = DeviceDescription(
    RF_BOARD_MA_SUPPORTED, {
                               StripDescription(13, {Reversed}),
                               StripDescription(12, {}),
                               StripDescription(13, {Reversed}),
                               StripDescription(12, {}),
                           });

const DeviceDescription whatever = SimpleRfBoardDescription(18, {Circular});

// Modify this variable to easily switch between devices.
const DeviceDescription &current = scarf;

static_assert(sizeof(current) <= DeviceDescription::kMaxSize,
              "Current device too large");

}  // namespace Devices

#endif  // __DEVICES_HPP__
