#ifndef __MATH_HPP__
#define __MATH_HPP__

#include <Types.hpp>

/**
 * @brief Gets the polar vector for evenly spaced points on a circle.
 *
 * @param led_count How many total points are on the circle.
 * @param led_index Which point we are referring to.
 * @param angle Returns the angle of the point on the circle from [0, 255].
 * @param radius Returns the magnitude of the vector, AKA the radius of the
 * circle. This is in an arbitrary unit that is designed to look good with the
 * Perlin noise functoins.
 */
void GetPosOnCircle(uint8_t led_count, uint8_t led_index, uint8_t *angle,
                    uint8_t *radius);

/**
 * @brief Mirrors the second half of an LED range.
 *
 * Takes two pointers to non-const uint8_t values and modifies them such that
 * the second half of the LED range is mirrored about the middle LED.
 *
 * @param led_index A pointer to the unmodified index of the LED.
 * @param led_count A pointer to the unmodified count of all the LEDs.
 */
void MirrorIndex(uint8_t *led_index, uint8_t *led_count);

#endif  // __MATH_HPP__
