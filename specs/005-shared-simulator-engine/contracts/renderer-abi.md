# Renderer ABI Contract

## Version and errors

- ABI version: `1`.
- Every exported function uses fixed-width integer and pointer values safe across the C boundary.
- Index metadata accessors return neutral values for invalid indices; render operations return a negative status.
- The JavaScript adapter checks the ABI before reading metadata or creating engines.

Status values:

| Value | Meaning |
|-------|---------|
| `0` | Success |
| `-1` | Invalid renderer handle |
| `-2` | Invalid device index |
| `-3` | Output unavailable/internal validation failure |

## Exported metadata operations

```text
uint32 ff_sim_abi_version()

uint32 ff_sim_effect_count()
char*  ff_sim_effect_name(uint32 wire_index)
uint32 ff_sim_effect_weight(uint32 wire_index)
uint32 ff_sim_random_effect_count()

uint32 ff_sim_palette_count()
char*  ff_sim_palette_name(uint32 palette_index)
uint32 ff_sim_palette_color_count(uint32 palette_index)
uint32 ff_sim_palette_color_hsv(uint32 palette_index, uint32 color_index)

uint32 ff_sim_device_count()
char*  ff_sim_device_name(uint32 device_index)
uint32 ff_sim_device_milliamps(uint32 device_index)
uint32 ff_sim_device_strip_count(uint32 device_index)
uint32 ff_sim_strip_led_count(uint32 device_index, uint32 strip_index)
uint32 ff_sim_strip_flags(uint32 device_index, uint32 strip_index)
```

`ff_sim_palette_color_hsv` packs `h | (s << 8) | (v << 16)`. Strings are immutable UTF-8 owned for the module lifetime. Effect metadata is the expanded 35-entry wire table; each duplicate reports its declaration's full weight, matching the current public API.

## Renderer lifecycle

```text
uint32 ff_sim_renderer_create(uint32 fire_offset,
                              uint32 firefly_offset,
                              uint32 rorschach_offset)
int32  ff_sim_renderer_destroy(uint32 handle)
```

Offsets are narrowed to their production field widths. Handle `0` is always invalid. Destroy is idempotent only for a currently live handle; invalid handles return `-1`.

## Rendering

```text
int32 ff_sim_render_device(uint32 handle,
                           uint32 device_index,
                           uint32 effect_byte,
                           uint32 palette_byte,
                           uint32 time_ms,
                           uint32 control_active,
                           uint32 control_rgb)

int32 ff_sim_render_strip(uint32 handle,
                          uint32 led_count,
                          uint32 strip_flags,
                          uint32 effect_byte,
                          uint32 palette_byte,
                          uint32 time_ms,
                          uint32 control_active,
                          uint32 control_rgb)

uint32 ff_sim_render_size(uint32 handle)
uint8* ff_sim_render_data(uint32 handle)
```

`control_rgb` packs `r | (g << 8) | (b << 16)`. Successful rendering replaces the handle's output with packed `r,g,b` bytes in global device order. JavaScript must copy the bytes before the next render on the same handle. `render_strip` exists for fuzz/boundary coverage and uses the exact production rendering loop; it is not part of `window.sim`.

Effect and palette arguments use their low byte. Production registry/palette wrapping occurs inside the renderer, including invalid byte values 0-255.

## JavaScript adapter contract

`sim/js/renderer.js` exports one initialized adapter object with:

```text
metadata.effects
metadata.palettes
metadata.devices
metadata.randomEffectCount
getRenderer(seedTuple)
renderDevice(seedTuple, deviceName, show, timeMs) -> Uint8Array copy
renderStrip(seedTuple, strip, show, timeMs) -> Uint8Array copy
```

The adapter caches one native handle per distinct seed tuple and exposes existing metadata shapes. It never implements a pixel algorithm or fallback renderer.
