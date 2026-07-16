#ifndef SIM_WASM_SIMULATOR_BRIDGE_HPP_
#define SIM_WASM_SIMULATOR_BRIDGE_HPP_

#include <cstdint>

extern "C" {

uint32_t ff_sim_abi_version();

uint32_t ff_sim_effect_count();
const char *ff_sim_effect_name(uint32_t wire_index);
uint32_t ff_sim_effect_weight(uint32_t wire_index);
uint32_t ff_sim_random_effect_count();

uint32_t ff_sim_palette_count();
const char *ff_sim_palette_name(uint32_t palette_index);
uint32_t ff_sim_palette_color_count(uint32_t palette_index);
uint32_t ff_sim_palette_color_hsv(uint32_t palette_index,
                                  uint32_t color_index);

uint32_t ff_sim_device_count();
const char *ff_sim_device_name(uint32_t device_index);
uint32_t ff_sim_device_milliamps(uint32_t device_index);
uint32_t ff_sim_device_strip_count(uint32_t device_index);
uint32_t ff_sim_strip_led_count(uint32_t device_index,
                                uint32_t strip_index);
uint32_t ff_sim_strip_flags(uint32_t device_index, uint32_t strip_index);

uint32_t ff_sim_renderer_create(uint32_t fire_offset,
                                uint32_t firefly_offset,
                                uint32_t rorschach_offset);
int32_t ff_sim_renderer_destroy(uint32_t handle);

int32_t ff_sim_render_device(uint32_t handle, uint32_t device_index,
                             uint32_t effect_byte, uint32_t palette_byte,
                             uint32_t time_ms, uint32_t control_active,
                             uint32_t control_rgb);
int32_t ff_sim_render_strip(uint32_t handle, uint32_t led_count,
                            uint32_t strip_flags, uint32_t effect_byte,
                            uint32_t palette_byte, uint32_t time_ms,
                            uint32_t control_active, uint32_t control_rgb);

uint32_t ff_sim_render_size(uint32_t handle);
uint8_t *ff_sim_render_data(uint32_t handle);

}  // extern "C"

#endif  // SIM_WASM_SIMULATOR_BRIDGE_HPP_
