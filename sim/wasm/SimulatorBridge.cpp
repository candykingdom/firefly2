#include "SimulatorBridge.hpp"

#include <DeviceCatalog.hpp>
#include <Effect.hpp>
#include <EffectRegistry.hpp>
#include <cstdint>
#include <map>
#include <memory>

#include "SimulatorRenderer.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#define FF_SIM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define FF_SIM_EXPORT
#endif

namespace {

constexpr uint32_t kAbiVersion = 1;
constexpr int32_t kSuccess = 0;
constexpr int32_t kInvalidHandle = -1;

std::map<uint32_t, std::unique_ptr<SimulatorRenderer>> &Renderers() {
  static std::map<uint32_t, std::unique_ptr<SimulatorRenderer>> renderers;
  return renderers;
}

SimulatorRenderer *FindRenderer(uint32_t handle) {
  const auto found = Renderers().find(handle);
  return found == Renderers().end() ? nullptr : found->second.get();
}

const NamedDeviceDescription *DeviceAt(uint32_t device_index) {
  const auto &devices = DeviceCatalog::All();
  return device_index < devices.size() ? &devices[device_index] : nullptr;
}

const StripDescription *StripAt(uint32_t device_index,
                                uint32_t strip_index) {
  const NamedDeviceDescription *device = DeviceAt(device_index);
  if (device == nullptr ||
      strip_index >= device->description->strips.size()) {
    return nullptr;
  }
  return &device->description->strips[strip_index];
}

}  // namespace

extern "C" {

FF_SIM_EXPORT uint32_t ff_sim_abi_version() { return kAbiVersion; }

FF_SIM_EXPORT uint32_t ff_sim_effect_count() {
  return static_cast<uint32_t>(EffectRegistry::WireTable().size());
}

FF_SIM_EXPORT const char *ff_sim_effect_name(uint32_t wire_index) {
  const auto &effects = EffectRegistry::WireTable();
  return wire_index < effects.size() ? effects[wire_index]->name : nullptr;
}

FF_SIM_EXPORT uint32_t ff_sim_effect_weight(uint32_t wire_index) {
  const auto &effects = EffectRegistry::WireTable();
  return wire_index < effects.size() ? effects[wire_index]->weight : 0;
}

FF_SIM_EXPORT uint32_t ff_sim_random_effect_count() {
  return EffectRegistry::RandomEffectCount();
}

FF_SIM_EXPORT uint32_t ff_sim_palette_count() {
  return static_cast<uint32_t>(Effect::palettes().size());
}

FF_SIM_EXPORT const char *ff_sim_palette_name(uint32_t palette_index) {
  const auto &names = Effect::palette_names();
  return palette_index < names.size() ? names[palette_index] : nullptr;
}

FF_SIM_EXPORT uint32_t ff_sim_palette_color_count(uint32_t palette_index) {
  const auto &palettes = Effect::palettes();
  return palette_index < palettes.size() ? palettes[palette_index].Size() : 0;
}

FF_SIM_EXPORT uint32_t ff_sim_palette_color_hsv(uint32_t palette_index,
                                                uint32_t color_index) {
  const auto &palettes = Effect::palettes();
  if (palette_index >= palettes.size() ||
      color_index >= palettes[palette_index].Size()) {
    return 0;
  }
  const CHSV color = palettes[palette_index].GetColor(color_index);
  return static_cast<uint32_t>(color.h) |
         (static_cast<uint32_t>(color.s) << 8) |
         (static_cast<uint32_t>(color.v) << 16);
}

FF_SIM_EXPORT uint32_t ff_sim_device_count() {
  return static_cast<uint32_t>(DeviceCatalog::All().size());
}

FF_SIM_EXPORT const char *ff_sim_device_name(uint32_t device_index) {
  const NamedDeviceDescription *device = DeviceAt(device_index);
  return device == nullptr ? nullptr : device->name;
}

FF_SIM_EXPORT uint32_t ff_sim_device_milliamps(uint32_t device_index) {
  const NamedDeviceDescription *device = DeviceAt(device_index);
  return device == nullptr ? 0 : device->description->milliamps_supported;
}

FF_SIM_EXPORT uint32_t ff_sim_device_strip_count(uint32_t device_index) {
  const NamedDeviceDescription *device = DeviceAt(device_index);
  return device == nullptr
             ? 0
             : static_cast<uint32_t>(device->description->strips.size());
}

FF_SIM_EXPORT uint32_t ff_sim_strip_led_count(uint32_t device_index,
                                              uint32_t strip_index) {
  const StripDescription *strip = StripAt(device_index, strip_index);
  return strip == nullptr ? 0 : strip->led_count;
}

FF_SIM_EXPORT uint32_t ff_sim_strip_flags(uint32_t device_index,
                                          uint32_t strip_index) {
  const StripDescription *strip = StripAt(device_index, strip_index);
  return strip == nullptr ? 0 : strip->GetFlags();
}

FF_SIM_EXPORT uint32_t ff_sim_renderer_create(uint32_t fire_offset,
                                              uint32_t firefly_offset,
                                              uint32_t rorschach_offset) {
  EffectSeedOverrides seeds;
  seeds.fire_offset = static_cast<uint16_t>(fire_offset);
  seeds.firefly_offset = firefly_offset;
  seeds.rorschach_offset = static_cast<uint16_t>(rorschach_offset);

  static uint32_t next_handle = 1;
  uint32_t handle;
  do {
    handle = next_handle++;
  } while (handle == 0 || Renderers().find(handle) != Renderers().end());
  Renderers()[handle].reset(new SimulatorRenderer(seeds));
  return handle;
}

FF_SIM_EXPORT int32_t ff_sim_renderer_destroy(uint32_t handle) {
  const auto found = Renderers().find(handle);
  if (found == Renderers().end()) {
    return kInvalidHandle;
  }
  Renderers().erase(found);
  return kSuccess;
}

FF_SIM_EXPORT int32_t ff_sim_render_device(
    uint32_t handle, uint32_t device_index, uint32_t effect_byte,
    uint32_t palette_byte, uint32_t time_ms, uint32_t control_active,
    uint32_t control_rgb) {
  SimulatorRenderer *renderer = FindRenderer(handle);
  return renderer == nullptr
             ? kInvalidHandle
             : renderer->RenderDevice(device_index, effect_byte, palette_byte,
                                      time_ms, control_active, control_rgb);
}

FF_SIM_EXPORT int32_t ff_sim_render_strip(
    uint32_t handle, uint32_t led_count, uint32_t strip_flags,
    uint32_t effect_byte, uint32_t palette_byte, uint32_t time_ms,
    uint32_t control_active, uint32_t control_rgb) {
  SimulatorRenderer *renderer = FindRenderer(handle);
  return renderer == nullptr
             ? kInvalidHandle
             : renderer->RenderStrip(led_count, strip_flags, effect_byte,
                                     palette_byte, time_ms, control_active,
                                     control_rgb);
}

FF_SIM_EXPORT uint32_t ff_sim_render_size(uint32_t handle) {
  SimulatorRenderer *renderer = FindRenderer(handle);
  return renderer == nullptr ? 0 : renderer->OutputSize();
}

FF_SIM_EXPORT uint8_t *ff_sim_render_data(uint32_t handle) {
  SimulatorRenderer *renderer = FindRenderer(handle);
  return renderer == nullptr ? nullptr : renderer->OutputData();
}

}  // extern "C"
