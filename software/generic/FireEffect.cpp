#include "FireEffect.hpp"

#include "Perlin.hpp"

FireEffect::FireEffect(DeviceDescription *const device) : Effect(device) {
#ifdef ARDUINO
  random16_set_seed((analogRead(A0) << 10) | analogRead(A0));
#endif
  offset = random16();
}

CRGB FireEffect::GetRGB(uint8_t ledIndex, uint32_t timeMs,
                        RadioPacket *setEffectPacket) {
  timeMs += offset;
  // Create 2 octave Perlin noise by averaging multiple samples.
  uint8_t noise =
      (perlinNoise(ledIndex * 20, timeMs / 8) / 4 * 3) +
      (perlinNoise(ledIndex * 10 + 1234, timeMs / 2 + 1234567) / 4 * 1);

  return palette.GetGradient(noise << 8, false);
}
