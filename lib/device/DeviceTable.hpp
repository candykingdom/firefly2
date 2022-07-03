#ifndef __DEVICE_TABLE_H__
#define __DEVICE_TABLE_H__

#include <vector>

#include "DeviceDescription.hpp"
#include "StripDescription.hpp"

const uint32_t RF_BOARD_MA_SUPPORTED = 2400 - 50;

static const DeviceDescription *SimpleRfBoardDescription(
    char* name, uint8_t led_count, std::vector<StripFlag> flags) {
  return new DeviceDescription(name, RF_BOARD_MA_SUPPORTED,
                               {
                                   new StripDescription(led_count, flags),
                               });
}

/**
 * @brief A table of all the different known device descriptions.
 *
 * WARNING: The order of these must not be modified! The indicies are stored on
 * devices and must remain consistent.
 */
std::vector<const DeviceDescription *> device_table{
    SimpleRfBoardDescription("Fallback", 10, {}),
    SimpleRfBoardDescription("Scarf", 46, {}),
    SimpleRfBoardDescription("Bike", 30, {Bright}),
    SimpleRfBoardDescription("Lantern", 5, {Tiny}),
    SimpleRfBoardDescription("Puck", 12, {Tiny, Circular}),
    SimpleRfBoardDescription("Two Sided Puck", 2, {Tiny, Circular, Mirrored}),
    new DeviceDescription(
        "Rainbow Cloak", RF_BOARD_MA_SUPPORTED,
        {
            new StripDescription(11, {Tiny, Circular}),
            new StripDescription(94, {}),
            new StripDescription(11, {Tiny, Circular, Reversed}),
        }),
    SimpleRfBoardDescription("Backpack Tail", 11, {}),
};

#endif  // __DEVICE_TABLE_H__