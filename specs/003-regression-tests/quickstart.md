# Quickstart: Validating the Regression-Test Feature

**Plan**: [plan.md](plan.md) | **Suite contracts**: [contracts/regression-suites.md](contracts/regression-suites.md)

## Prerequisites

- The feature worktree (`.claude/worktrees/002-fix-audit-findings`), current with master
- Host toolchain (cmake + GCC/Clang); PlatformIO only for commit 1's lib-touching gate
- Network access at CMake configure time (FetchContent: googletest, FakeFastLED, + new nlohmann/json)

## Baseline (before any change)

```bash
cd build && cmake .. -DBUILD_SIMULATOR=false && make -j8
time make test          # record wall time — SC-005 baseline
```

## Per-commit gate

```bash
./lint.sh check
cd build && cmake .. -DBUILD_SIMULATOR=false && make -j8 && make test
pio run -e node-arm64 -e fancy-node    # commit 1 only (touches lib/)
```

## Break-demonstrations (required, then reverted — record in commit messages)

| SC | How | Expected |
|----|-----|----------|
| SC-001 | Perturb one constant in any covered effect's math (e.g. RainbowEffect hue step) | `ReferenceVectorTest` fails naming case id / effect / device / LED |
| SC-002 | In `RadioPacket::Deserialize`, comment out the `dataLength` assignment | ≥ 5 failures across RadioStateMachine/NetworkManager/RadioStateIntegration suites — not just RadioPacketTest |
| SC-003 | Separately break Off (skip black), Dim (skip ÷8), Reversed (skip flip) in `RunEffect` | One named LedManagerTest failure per flag |

Each demonstration: apply → `make smalltests && ./smalltests` → observe → revert → green.

## Suite-specific runs

```bash
./build/smalltests --gtest_filter='ReferenceVector*'     # G1 corpus comparison
./build/smalltests --gtest_filter='RadioState*:NetworkManager*'  # G2 wire-path regression surface
./build/smalltests --gtest_filter='LedManager*'          # G3 flag semantics
./build/smalltests --gtest_filter='DeviceDescription*:Battery*:ColorPalette*'  # G4
./build/largetests                                       # fuzz unweakened (FR-005)
node --test "sim/test/cases/*.test.mjs"                  # sim suite still green
```

## Corpus invariants (commit 2)

```bash
make vectorgen && ./vectorgen > /tmp/ref-after.json
diff /tmp/ref-after.json ../sim/test/vectors/reference.json   # only firmwareGitDescribe line may differ
```

Regeneration remains: `./vectorgen > ../sim/test/vectors/reference.json` (FR-003, documented in docs/build-and-test.md).

## Done when

- All commits landed with gates green; SC-001/002/003 demonstrations recorded
- `./ci.sh` green at tip; suite wall time within SC-005's +25% budget vs baseline
- CI logs of the landing push show the new suites executing (FR-009 / SC-004)
