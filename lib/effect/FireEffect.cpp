#include "FireEffect.hpp"

#include <Math.hpp>
#include <Perlin.hpp>

FireEffect::FireEffect() : Effect() {
#ifdef ARDUINO
  random16_set_seed((analogRead(A0) << 10) | analogRead(A0));
#endif
  offset = random16();
}

CRGB FireEffect::GetRGB(uint8_t led_index, uint32_t time_ms,
                        const StripDescription *strip,
                        RadioPacket *setEffectPacket) {
  UNUSED(setEffectPacket);
  uint32_t side_differentiator = 0;
  uint8_t led_count = strip->led_count;
  if (strip->FlagEnabled(Mirrored)) {
    if (led_index > led_count / 2) {
      side_differentiator = 6789;
    }
    MirrorIndex(&led_index, &led_count);
  }

  time_ms += offset;
  uint8_t noise;
  if (strip->FlagEnabled(Circular)) {
    uint8_t angle;
    uint8_t radius;
    GetPosOnCircle(led_count, led_index, &angle, &radius);

    // Create 2 octave Perlin noise by averaging multiple samples.
    noise = (perlinNoisePolar(time_ms / 8, side_differentiator, angle, radius) /
             4 * 3) +
            (perlinNoisePolar(time_ms / 2 + 1234567, 0, angle, radius) / 4 * 1);
  } else {
    // Create 2 octave Perlin noise by averaging multiple samples.
    noise = (perlinNoise(led_index * 20, time_ms / 8) / 4 * 3) +
            (perlinNoise(led_index * 10 + 1234, time_ms / 2 + 1234567) / 4 * 1);
  }

  return palette.GetGradient(noise << 8, false);
}
