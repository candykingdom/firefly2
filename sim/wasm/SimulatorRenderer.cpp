#include "SimulatorRenderer.hpp"

#include <DeviceCatalog.hpp>
#include <DeviceDescription.hpp>
#include <Effect.hpp>
#include <FakeRadio.hpp>
#include <LedManager.hpp>
#include <FireflyNetworkManager.hpp>
#include <Radio.hpp>
#include <RadioStateMachine.hpp>
#include <StripDescription.hpp>
#include <Types.hpp>
#include <utility>

namespace {

constexpr int32_t kSuccess = 0;
constexpr int32_t kInvalidDevice = -2;
constexpr int32_t kOutputUnavailable = -3;

class BufferLedManager : public LedManager {
 public:
  BufferLedManager(const DeviceDescription &device,
                   RadioStateMachine *state_machine,
                   const EffectSeedOverrides &seeds)
      : LedManager(device, state_machine, seeds), leds_(device.GetLedCount()) {}

  void SetGlobalColor(const CRGB &rgb) override {
    for (CRGB &led : leds_) {
      led = rgb;
    }
  }

  const std::vector<CRGB> &Leds() const { return leds_; }

 protected:
  void SetLed(uint8_t led_index, const CRGB &rgb) override {
    if (led_index < leds_.size()) {
      leds_[led_index] = rgb;
    }
  }

  void WriteOutLeds() override {}

 private:
  std::vector<CRGB> leds_;
};

std::vector<StripFlag> DecodeFlags(uint8_t flags) {
  std::vector<StripFlag> decoded;
  for (StripFlag flag : {Tiny, Bright, Circular, Mirrored, Reversed,
                         Controller, Dim, Off}) {
    if ((flags & static_cast<uint8_t>(flag)) != 0) {
      decoded.push_back(flag);
    }
  }
  return decoded;
}

}  // namespace

struct SimulatorRenderer::DeviceRig {
  DeviceRig(const DeviceDescription &device,
            const EffectSeedOverrides &seed_overrides)
      : radio(),
        network_manager(&radio),
        state_machine(&network_manager),
        manager(device, &state_machine, seed_overrides) {}

  FakeRadio radio;
  FireflyNetworkManager network_manager;
  RadioStateMachine state_machine;
  BufferLedManager manager;
};

SimulatorRenderer::SimulatorRenderer(const EffectSeedOverrides &seeds)
    : seeds_(seeds), device_rigs_(DeviceCatalog::All().size()) {}

SimulatorRenderer::~SimulatorRenderer() = default;

int32_t SimulatorRenderer::RenderDevice(uint32_t device_index,
                                        uint32_t effect_byte,
                                        uint32_t palette_byte,
                                        uint32_t time_ms,
                                        uint32_t control_active,
                                        uint32_t control_rgb) {
  const auto &devices = DeviceCatalog::All();
  if (device_index >= devices.size()) {
    output_.clear();
    return kInvalidDevice;
  }
  if (!device_rigs_[device_index]) {
    device_rigs_[device_index].reset(
        new DeviceRig(*devices[device_index].description, seeds_));
  }
  return Render(device_rigs_[device_index].get(), effect_byte, palette_byte,
                time_ms, control_active, control_rgb);
}

int32_t SimulatorRenderer::RenderStrip(uint32_t led_count,
                                       uint32_t strip_flags,
                                       uint32_t effect_byte,
                                       uint32_t palette_byte,
                                       uint32_t time_ms,
                                       uint32_t control_active,
                                       uint32_t control_rgb) {
  const uint8_t count = static_cast<uint8_t>(led_count);
  const uint8_t flags = static_cast<uint8_t>(strip_flags);
  const DeviceDescription device(
      0, {StripDescription(count, DecodeFlags(flags))});
  DeviceRig rig(device, seeds_);
  return Render(&rig, effect_byte, palette_byte, time_ms, control_active,
                control_rgb);
}

int32_t SimulatorRenderer::Render(DeviceRig *rig, uint32_t effect_byte,
                                  uint32_t palette_byte, uint32_t time_ms,
                                  uint32_t control_active,
                                  uint32_t control_rgb) {
  if (rig == nullptr || EffectRegistry::WireTable().empty() ||
      Effect::palettes().empty()) {
    output_.clear();
    return kOutputUnavailable;
  }

  RadioPacket packet;
  if (control_active != 0) {
    const CRGB rgb(static_cast<uint8_t>(control_rgb),
                   static_cast<uint8_t>(control_rgb >> 8),
                   static_cast<uint8_t>(control_rgb >> 16));
    packet.writeControl(0, rgb);
    *rig->state_machine.GetSetEffect() = packet;
  } else {
    const uint8_t effect = static_cast<uint8_t>(effect_byte) %
                           EffectRegistry::WireTable().size();
    const uint8_t palette =
        static_cast<uint8_t>(palette_byte) % Effect::palettes().size();
    packet.writeSetEffect(effect, 0, palette);
    rig->state_machine.SetEffect(&packet);
  }

  setMillis(time_ms);
  rig->manager.RunEffect();

  const std::vector<CRGB> &leds = rig->manager.Leds();
  output_.clear();
  output_.reserve(leds.size() * 3);
  for (const CRGB &led : leds) {
    output_.push_back(led.r);
    output_.push_back(led.g);
    output_.push_back(led.b);
  }
  if (output_.size() != leds.size() * 3) {
    output_.clear();
    return kOutputUnavailable;
  }
  return kSuccess;
}

uint32_t SimulatorRenderer::OutputSize() const {
  return static_cast<uint32_t>(output_.size());
}

uint8_t *SimulatorRenderer::OutputData() {
  return output_.empty() ? nullptr : output_.data();
}
