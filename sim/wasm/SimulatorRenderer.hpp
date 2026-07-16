#ifndef SIM_WASM_SIMULATOR_RENDERER_HPP_
#define SIM_WASM_SIMULATOR_RENDERER_HPP_

#include <EffectRegistry.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class SimulatorRenderer {
 public:
  explicit SimulatorRenderer(const EffectSeedOverrides &seeds);
  ~SimulatorRenderer();

  SimulatorRenderer(const SimulatorRenderer &) = delete;
  SimulatorRenderer &operator=(const SimulatorRenderer &) = delete;

  int32_t RenderDevice(uint32_t device_index, uint32_t effect_byte,
                       uint32_t palette_byte, uint32_t time_ms,
                       uint32_t control_active, uint32_t control_rgb);

  int32_t RenderStrip(uint32_t led_count, uint32_t strip_flags,
                      uint32_t effect_byte, uint32_t palette_byte,
                      uint32_t time_ms, uint32_t control_active,
                      uint32_t control_rgb);

  uint32_t OutputSize() const;
  uint8_t *OutputData();

 private:
  struct DeviceRig;

  int32_t Render(DeviceRig *rig, uint32_t effect_byte,
                 uint32_t palette_byte, uint32_t time_ms,
                 uint32_t control_active, uint32_t control_rgb);

  EffectSeedOverrides seeds_;
  std::vector<std::unique_ptr<DeviceRig>> device_rigs_;
  std::vector<uint8_t> output_;
};

#endif  // SIM_WASM_SIMULATOR_RENDERER_HPP_
