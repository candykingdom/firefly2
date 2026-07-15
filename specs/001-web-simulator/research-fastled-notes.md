# FakeFastLED porting notes (T002)

Ground truth for the JS port = **ademuri/FakeFastLED as fetched into `build/_deps/fake-fast-led-src/src/`** (this is what `vectorgen` and all host tests link). Read the C sources directly when porting; this file records the *decisions* and gotchas, not full listings.

## Which #if branches apply on host (macOS arm64/x86)

- Host is neither `__AVR__` nor `__arm__` (Apple Silicon defines `__aarch64__`, not `__arm__`) → the plain-C fallback branch of `lib8tion.h` applies: `QADD8_C 1`, `SCALE8_C 1`, `SCALE16_C 1`, `EASE8_C 1`, `BLEND8_C 1`, etc.
- `FASTLED_SCALE8_FIXED` is **never defined** in FakeFastLED → all `#if (FASTLED_SCALE8_FIXED == 1)` branches are FALSE. So:
  - `scale8(i, s)  = (i * s) >> 8`  (NOT the `(i * (1+s)) >> 8` fixed variant)
  - `scale16` etc. likewise use the non-fixed bodies — check each body in `lib8tion/scale8.h` and port the `FASTLED_SCALE8_FIXED == 0` path.
- `sin16 = sin16_C`, `sin8 = sin8_C` (the C table implementations, not AVR asm).

## Exact algorithms confirmed (port these verbatim, all math masked to uint8/uint16/int16)

- `random16()`: LCG `seed = (seed * 2053 + 13849) & 0xFFFF`, returns seed. `random8()` advances the same seed, returns `(low byte + high byte) & 0xFF`. `random16(lim)` = `(lim * random16()) >> 16`. `random8(lim)` = `(random8() * lim) >> 8`. Seed is the single global `rand16seed`, **initialized to `RAND16_SEED = 1337`** (`lib8tion.cpp:8`).
- `sin16_C(theta)`: base table `{0,6393,12539,18204,23170,27245,30273,32137}`, slope table `{49,48,44,38,31,23,14,4}`; `offset=(theta&0x3FFF)>>3`; mirror if `theta&0x4000`; `y = slope[offset/256] * ((offset&0xFF)/2) + base[offset/256]`; negate if `theta&0x8000`. Return int16.
- `sin8_C(theta)`: interleaved table `{0,49,49,41,90,27,117,10}`; see `lib8tion/trig8.h:215-243`. `cos16(t)=sin16(t+16384)`, `cos8(t)=sin8(t+64)`.
- `qadd8(i,j)`: `min(i+j, 255)`.
- `triwave8(in)`: `in&0x80 ? in=255-in : in; return (in<<1)&0xFF`. `cubicwave8(in)=ease8InOutCubic(triwave8(in))`; `quadwave8=ease8InOutQuad(triwave8(in))` — port `ease8InOutCubic`/`ease8InOutQuad` bodies from `lib8tion.h` (~lines 640-700, C branch).
- `ease8InOutApprox(i)`: `<64 → i/2`; `>191 → 255-((255-i)/2)`; else `i-64; i+=i/2; i+=32`.
- `lerp16by16(a,b,frac)`: `b>a ? a + scale16(b-a,frac) : a - scale16(a-b,frac)`.
- `hsv2rgb_rainbow`: port from `hsv2rgb.cpp` (~716 lines; the rainbow function is the big one). Gotchas: `K255/K171/K170/K85` constants, `scale8` usage inside, and the final `scale8_video`-style value scaling — port exactly, including sat/val special cases. This is THE fidelity-critical function since every CHSV-returning effect goes through `CHSV::operator CRGB()` → `hsv2rgb_rainbow` (`pixeltypes.h`).
- `CRGB operator/ (uint8_t)`: per-channel integer divide (`pixeltypes.h`) — used by central Dim.
- `blend(CHSV a, CHSV b, fract8 t, TGradientDirectionCode SHORTEST_HUES)`: in `colorutils.cpp` — used by PrideEffect. Port the CHSV blend path incl. hue direction handling.
- `Effect::GetThresholdSin(x, threshold)` (`lib/effect/Effect.cpp:5-13`): `v = sin16(x)/128` (**C integer division: truncates toward zero — do NOT use >>7**, x is int16 so v ∈ −255..255); `v < threshold ? 0 : (v-threshold)*256/(256-threshold)`. Note callers pass int16 x (wraps).
- Perlin (`lib/math/Perlin.hpp`): render-time reseeding! `perlinNoise` calls `random16_set_seed(tileHash(ix,iy))` then `random8()` per corner — deterministic (hash-seeded) but MUTATES the global `rand16seed`. Port with the same shared seed variable semantics. `tileHash` and interpolation (`ease8InOutApprox`-based) are in the same header. `Math.hpp` has `MirrorIndex` + `UNUSED`.

## Effect construction seeds (determinism, research R3)

- Fire/Rorschach: `random16_set_seed(analogRead…)` is `#ifdef ARDUINO` — **absent on host**. So on host, `offset = random16()` just advances the global LCG from wherever it is. With registration order (`LedManager.cpp:12-35`): Fire is the 3rd constructed effect but the **first** to call `random16()` at construction; Rorschach the second. From fresh seed 1337: Fire.offset = first `random16()` from 1337 → `(1337*2053+13849)&0xFFFF = 51869`... (compute in JS, don't trust hand math — replicate the LCG and construction order instead of hardcoding).
- Firefly: `offset_ = random(0, kBlinkPeriod/2)` where `random(min,max)` is the **repo's own host fake** `lib/types/Types.cpp:6`: `min + (rand() % max)` over **libc `rand()` unseeded** (i.e. `srand(1)` default). Platform-dependent! ⇒ `vectorgen` MUST NOT rely on replicating libc rand in JS. Instead `vectorgen` dumps the actual constructed offsets into `meta.effectSeeds` and the JS registry defaults are cross-checked/overridden from that file. To read the private offsets, VectorGen replays prediction: call `rand()%（kBlinkPeriod/2)` etc. — NO: simplest is `srand(1)` explicitly at start, then note that LedManager's constructor consumes libc rand in a known order; VectorGen can predict Firefly's offset by calling `srand(1); predicted = 0 + rand() % (kBlinkPeriod/2); srand(1);` **before** constructing the manager (Firefly is the only libc-rand consumer during construction — verify with grep before trusting). Same trick for the FastLED LCG: predict Fire/Rorschach offsets by replaying `random16()` from seed 1337 in construction order, then reset `random16_set_seed(1337)` before constructing.
- **JS side**: registry constructs effects in the exact `LedManager.cpp` order, replaying the FastLED LCG from 1337 for Fire/Rorschach; Firefly's offset comes from `meta.effectSeeds.Firefly` in the committed vector JSON (single source of truth), with `setEffectSeed()` override for exploration.
- `kBlinkPeriod` — read from `FireflyEffect.hpp` when porting.

## Porting conventions (all JS modules)

- ES modules, no deps. All byte math through helper masks: `& 0xFF`, `& 0xFFFF`, `| 0`; int16 via `(x << 16) >> 16` where signedness matters (sin16 return, GetThresholdSin input).
- CHSV/CRGB as plain `{h,s,v}` / `{r,g,b}` objects; module `fastled.js` exports `hsv2rgbRainbow({h,s,v}) → {r,g,b}`, plus the shared PRNG state (`randomSeed16` get/set) used by perlin + construction.
- Effects export a factory `makeXxxEffect(...) → {name, getRGB(ledIndex, timeMs, strip, show)}` where `show = {paletteIndex, controlRgb}`; `strip = {ledCount, flags:Set}`; return CRGB objects. Time arithmetic: uint32 wrap (`>>> 0`) — C `time_ms += offset` wraps uint32.
- `uint8_t led_count` etc: mind uint8 wrap in C when porting arithmetic like `led_count - led_index - 1` or `led_index * 255 / led_count` — replicate C's integer types exactly (C promotes uint8_t to int for arithmetic, so intermediate math is NOT truncated to 8 bits unless assigned back — replicate the assignments, e.g. `uint8_t x = a * b / c` truncates only at assignment).
- Primitive-level cross-check: `vectorgen` also emits `primitives` samples (sin16/sin8/hsv2rgb_rainbow at fixed inputs) so a math bug is distinguishable from an effect-port bug.
