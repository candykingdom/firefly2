# Quickstart: Reproduce the Bug and Validate the Fix

## Prerequisites

```bash
mkdir -p build && cd build && cmake .. -DBUILD_SIMULATOR=false && make -j8
```

Node ≥ 20 for the sim suite (`node --test` with a quoted glob — the bare-directory form errors on this repo's Node).

## 1. See the bug (pre-fix)

Render one frame of `RainbowEffect`, rainbow palette (index 8), 100-LED non-Tiny strip, v=128, and print per-LED drive. Fastest existing harness: a ~30-line host program linking `libgeneric.a` + FakeFastLED (pattern: construct `RadioPacket` with `writeSetEffect(0, 0, 8)`, `StripDescription strip(100, {})`, call `effect.GetRGB(i, 0, strip, &packet)`; compile with `-Ilib/{effect,color,radio,types,device,debug} -Ibuild/_deps/fake-fast-led-src/src build/libgeneric.a build/_deps/fake-fast-led-build/libfake-fast-led.a`). Alternatively, once the regression test exists, just run it — it fails pre-fix.

Expected pre-fix output (one 32-LED cycle, t=0):

```text
LED  0: (65, 0, 0)  drive 65   ← red
LED  4: (44,25, 0)  drive 69
LED  7: (44,43, 0)  drive 87   ← THE BUG: lone yellow LED, +34% over baseline
LED 11: ( 0,64, 2)  drive 66   ← green
LED 21: ( 0, 4,62)  drive 66   ← blue
(all LEDs other than 4-10 hold drive 64-66)
```

Browser view: `python3 -m http.server 8642 -d sim`, open `http://localhost:8642/`, drive via `window.sim` to the Rainbow effect with the Rainbow palette on a ≥60-LED device — the bright yellow pixel is visible at each red→green crossover, repeating every 32 LEDs.

## 2. Prove the failing test (red)

Add the regression tests first ([contracts/flattened-gradient.md](contracts/flattened-gradient.md) §property) and run them against unmodified rendering code:

```bash
cd build && make smalltests && ./smalltests --gtest_filter=GradientPowerTest*   # MUST FAIL
node --test "sim/test/cases/gradientPower.test.mjs"                            # MUST FAIL
```

Both must fail identifying a LED whose drive exceeds the endpoint baseline (87 vs ~68 allowed).

## 3. Apply the fix

Implement the contract (helper + 4 C++ call sites + JS mirror + 4 JS call sites), then regenerate the corpus **in the same change**:

```bash
cd build && make vectorgen && ./vectorgen > ../sim/test/vectors/reference.json
```

## 4. Validate (green)

```bash
./ci.sh                                          # cmake + smalltests + largetests
node --test "sim/test/cases/*.test.mjs"          # full sim suite incl. vectors + new test
./lint.sh check                                  # clang-format (Google style)
npm run lint                                     # ESLint over sim/
```

Expected: everything passes. The new tests now assert max drive ≤ endpoint×1.05 (post-fix max is 66 vs 65 baseline).

Spot-check the fixed frame (same harness as step 1):

```text
LED  7: (32,32, 0)  drive 64   ← flattened; hue still yellow, no pop
(every LED in the cycle now holds drive 63-66)
```

## 5. Review the corpus diff

```bash
git diff --stat sim/test/vectors/reference.json
python3 - <<'EOF'
# confirm only the four effects' cases changed (SC-005)
import json, subprocess
old = json.loads(subprocess.run(['git','show','HEAD:sim/test/vectors/reference.json'],
                                capture_output=True, text=True).stdout)
new = json.load(open('sim/test/vectors/reference.json'))
changed = {c['effect'] for o, c in zip(old['cases'], new['cases']) if o != c}
print(sorted(changed))
EOF
```

Expected: a subset of `{Rainbow, Color Cycle, Rainbow Bumps, Display Color Palette}` (exact key names as in the corpus), nothing else. (Adjust field access to the corpus's actual case schema — see `test/VectorGenCommon.hpp`.)

## 6. Watchdog sanity (hardware budget)

No measurement needed beyond code review: the helper adds one `hsv2rgb_rainbow` (branchy 8-bit math, no loops) and three 32-bit divides per LED, only in the four effects. CI's PlatformIO builds (`node`, `fancy-node`, `controller`) confirm it compiles for all targets:

```bash
pio run -e node && pio run -e fancy-node && pio run -e controller
```
