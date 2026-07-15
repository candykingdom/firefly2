#include "VectorGenCommon.hpp"

#include <Devices.hpp>
#include <Radio.hpp>
#include <cstdlib>

namespace vectorgen {

namespace {

// FireflyEffect::kBlinkPeriod / 2 (private, so duplicated here). See
// lib/effect/FireflyEffect.hpp: kPeriodMs=20000, kSinMultiplier=64,
// kBlinkPeriod=(1<<16)/kSinMultiplier=1024.
constexpr uint16_t kFireflyRandMax = 512;

const DeviceDescription &ResetPrngsBeforeLedManager(
    const DeviceDescription &device) {
  PredictEffectSeedsAndResetPrngs();
  return device;
}

}  // namespace

EffectSeeds PredictEffectSeedsAndResetPrngs() {
  EffectSeeds seeds;

  srand(1);
  random16_set_seed(kFastLedSeed);
  // Construction order (LedManager.cpp): ColorCycle, ContrastBumps, Fire
  // (1st random16 consumer), Firefly (1st libc rand consumer), Lightning,
  // Pride, RainbowBumps, Rainbow, Rorschach (2nd random16 consumer), ...
  seeds.fire = random16();
  seeds.rorschach = random16();

  srand(1);
  seeds.firefly = 0 + (rand() % kFireflyRandMax);

  // Reset again so the real construction below consumes the exact predicted
  // sequence.
  srand(1);
  random16_set_seed(kFastLedSeed);
  return seeds;
}

const std::vector<EffectInfo> &Effects() {
  static const std::vector<EffectInfo> effects = {
      {0, "Color Cycle"},
      {1, "Color Cycle"},
      {2, "Contrast Bumps"},
      {3, "Contrast Bumps"},
      {4, "Fire"},
      {5, "Firefly"},
      {6, "Firefly"},
      {7, "Lightning"},
      {8, "Pride"},
      {9, "Rainbow Bumps"},
      {10, "Rainbow Bumps"},
      {11, "Rainbow Bumps"},
      {12, "Rainbow Bumps"},
      {13, "Rainbow"},
      {14, "Rainbow"},
      {15, "Rainbow"},
      {16, "Rainbow"},
      {17, "Rorschach"},
      {18, "Rorschach"},
      {19, "Spark"},
      {20, "Spark"},
      {21, "Spark"},
      {22, "Spark"},
      {23, "Swinging Lights"},
      {24, "Swinging Lights"},
      {25, "Swinging Lights"},
      {26, "Swinging Lights"},
      {27, "Swinging Lights (Police)"},
      {28, "Stop Light"},
      {29, "Simple Blink 60ms"},
      {30, "Simple Blink 30ms"},
      {31, "Simple Blink 12ms"},
      {32, "Simple Blink 300ms"},
      {33, "Display Color Palette"},
      {34, "Dark"},
  };
  return effects;
}

const std::vector<uint8_t> &RepresentativeEffectIndices() {
  static const std::vector<uint8_t> indices = {
      0, 2, 4, 5, 7, 8, 9, 13, 17, 19, 23, 27, 28, 29, 30, 31, 32, 33, 34};
  return indices;
}

const std::vector<PaletteInfo> &Palettes() {
  static const std::vector<PaletteInfo> palettes = {
      {0, "Red"},
      {1, "Orange"},
      {2, "Yellow"},
      {3, "Green"},
      {4, "Aqua"},
      {5, "Blue"},
      {6, "Purple"},
      {7, "Pink"},
      {8, "Rainbow"},
      {9, "Warm"},
      {10, "Cool"},
      {11, "Yellow-Green"},
      {12, "80s Miami"},
      {13, "Vaporwave"},
      {14, "Cool Popo"},
      {15, "Candy Cane"},
      {16, "Winter Mint"},
      {17, "Fire"},
      {18, "Pastel Rainbow"},
      {19, "Jazz Cup"},
      {20, "Yellow & Double Purp"},
      {21, "Double Rainbow"},
  };
  return palettes;
}

const std::vector<uint8_t> &PaletteGrid() {
  static const std::vector<uint8_t> palettes = {0, 9, 8, 21};
  return palettes;
}

const std::vector<uint32_t> &TimeGrid() {
  static const std::vector<uint32_t> times = {0,     1,           1000,
                                              60000, 2147483648u, 4294967295u};
  return times;
}

const std::vector<DeviceInfo> &DeviceGrid() {
  static const std::vector<DeviceInfo> devices = {
      {"scarf", Devices::scarf},
      {"puck", Devices::puck},
      {"ufo", Devices::ufo},
  };
  return devices;
}

const std::vector<CRGB> &ControlRgbGrid() {
  static const std::vector<CRGB> rgbs = {CRGB(255, 0, 0), CRGB(12, 34, 56)};
  return rgbs;
}

const std::vector<uint32_t> &ControlTimeGrid() {
  static const std::vector<uint32_t> times = {0, 1000};
  return times;
}

DeviceRig::DeviceRig(const DeviceDescription &device)
    : radio(),
      network_manager(&radio),
      state_machine(&network_manager),
      manager(ResetPrngsBeforeLedManager(device), &state_machine) {}

namespace {

std::vector<CRGB> ReadLeds(DeviceRig *rig, uint8_t led_count) {
  std::vector<CRGB> leds;
  leds.reserve(led_count);
  for (uint8_t i = 0; i < led_count; i++) {
    leds.push_back(rig->manager.GetLed(i));
  }
  return leds;
}

RenderedCase RenderEffectCase(DeviceRig *rig, const char *device_name,
                              uint8_t effect_index, uint8_t palette_index,
                              uint32_t time_ms, uint8_t led_count) {
  RadioPacket packet;
  packet.writeSetEffect(effect_index, 0, palette_index);
  rig->state_machine.SetEffect(&packet);
  setMillis(time_ms);
  rig->manager.RunEffect();

  RenderedCase rendered;
  rendered.is_control = false;
  rendered.device = device_name;
  rendered.effect_index = effect_index;
  rendered.palette_index = palette_index;
  rendered.control_rgb = CRGB(0, 0, 0);
  rendered.time_ms = time_ms;
  rendered.leds = ReadLeds(rig, led_count);
  return rendered;
}

RenderedCase RenderControlCase(DeviceRig *rig, const char *device_name,
                               const CRGB &rgb, uint32_t time_ms,
                               uint8_t led_count) {
  RadioPacket packet;
  packet.writeControl(0, rgb);
  // RadioStateMachine::SetEffect() unconditionally parses the packet as a
  // SET_EFFECT packet (it's meant for effect changes only), so inject the
  // SET_CONTROL packet directly - this mirrors what handleSlaveEvent/
  // handleMasterEvent do for a received SET_CONTROL packet.
  *rig->state_machine.GetSetEffect() = packet;
  setMillis(time_ms);
  rig->manager.RunEffect();

  RenderedCase rendered;
  rendered.is_control = true;
  rendered.device = device_name;
  rendered.effect_index = 0;
  rendered.palette_index = 0;
  rendered.control_rgb = rgb;
  rendered.time_ms = time_ms;
  rendered.leds = ReadLeds(rig, led_count);
  return rendered;
}

}  // namespace

void ForEachCase(const std::function<void(const RenderedCase &)> &visit) {
  for (const DeviceInfo &device_info : DeviceGrid()) {
    std::unique_ptr<DeviceRig> rig(new DeviceRig(device_info.description));
    uint8_t led_count = device_info.description.GetLedCount();

    for (uint8_t effect_index : RepresentativeEffectIndices()) {
      for (uint8_t palette_index : PaletteGrid()) {
        for (uint32_t time_ms : TimeGrid()) {
          visit(RenderEffectCase(rig.get(), device_info.name, effect_index,
                                 palette_index, time_ms, led_count));
        }
      }
    }
  }

  for (const DeviceInfo &device_info : DeviceGrid()) {
    std::unique_ptr<DeviceRig> rig(new DeviceRig(device_info.description));
    uint8_t led_count = device_info.description.GetLedCount();

    for (const CRGB &rgb : ControlRgbGrid()) {
      for (uint32_t time_ms : ControlTimeGrid()) {
        visit(RenderControlCase(rig.get(), device_info.name, rgb, time_ms,
                                led_count));
      }
    }
  }
}

}  // namespace vectorgen
