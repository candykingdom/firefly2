#include "FireEffect.hpp"

// #include "Math.hpp"
#include "Perlin.hpp"

FireEffect::FireEffect(const DeviceDescription *device) : Effect(device) {
#ifdef ARDUINO
  random16_set_seed((analogRead(A0) << 10) | analogRead(A0));
#endif
  offset = random16();
}

CRGB FireEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                        RadioPacket *setEffectPacket) {
  timeMs += offset;
  uint8_t noise;
  if (device->FlagEnabled(Circular)) {
    uint8_t angle;
    uint8_t radius;
    // GetPosOnCircle(device->led_count, ledIndex, &angle, &radius);

    // Create 2 octave Perlin noise by averaging multiple samples.
    noise = (perlinNoisePolar(timeMs / 8, 0, angle, radius) / 4 * 3) +
            (perlinNoisePolar(timeMs / 2 + 1234567, 0, angle, radius) / 4 * 1);
  } else {
    // Create 2 octave Perlin noise by averaging multiple samples.
    noise = (perlinNoise(ledIndex * 20, timeMs / 8) / 4 * 3) +
            (perlinNoise(ledIndex * 10 + 1234, timeMs / 2 + 1234567) / 4 * 1);
  }

  return palette.GetGradient(noise << 8, false);
}
