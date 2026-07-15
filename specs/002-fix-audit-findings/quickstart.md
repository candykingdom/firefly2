# Quickstart: Validating the Audit-Finding Fixes

**Plan**: [plan.md](plan.md) | **Contract tests**: [contracts/wire-format.md](contracts/wire-format.md)

## Prerequisites

- Working directory: the feature worktree (`.claude/worktrees/002-fix-audit-findings`), branch `worktree-002-fix-audit-findings`
- Host toolchain: cmake ≥ 3.11, a C++14 compiler (ASan/UBSan are always on in the host build)
- PlatformIO CLI for firmware builds (`pio`)
- clang-format (Google style) via `./lint.sh`

## One-time setup

```bash
mkdir -p build && cd build
cmake .. -DBUILD_SIMULATOR=false
```

## Per-commit gate (run before EVERY commit — PR-004)

```bash
./lint.sh check                          # 1. formatting (CI-enforced)
cd build && make && make test && cd ..   # 2. full host suite: smalltests + largetests, ASan+UBSan
pio run -e node -e fancy-node            # 3. firmware builds for shared-code commits (D1–D6, D8)
pio run -e controller                    # 3b. additionally for the controller commit (D7)
```

Then, for commits 1–7: run the **verifier** review on the staged diff (defect ID + spec/plan references + CLAUDE.md invariants; address findings first). Commit 8 (D8) skips review. One defect per commit; commit message references the defect ID and spec (e.g., `Fix D1: set dataLength on radio receive so rebroadcasts carry payload (specs/002-fix-audit-findings)`).

## Targeted test runs while implementing

```bash
./build/smalltests --gtest_filter='RadioPacketTest*'      # D1 codec + D2 heartbeat boundaries
./build/smalltests --gtest_filter='FireflyEffectTest*'    # D3 shift-count coverage
./build/smalltests --gtest_filter='EffectsTest*'          # D4 fuzz + golden spot-check
./build/smalltests --gtest_filter='LedManagerTest*'       # D5 regression
./build/largetests                                        # invalid-packet fuzz (slow)
```

New `test/*Test.cpp` files are auto-globbed into `smalltests` — no CMake edits needed; just re-run `cmake ..` in `build/` after adding a file.

## Expected outcomes per fix

| Fix | Proof it works |
|-----|----------------|
| D1 | New `RadioPacketTest` round-trip cases pass (decode→re-encode is byte-identical, lengths 0/1/4/58); relay scenario decodes the same heartbeat time after one rebroadcast hop; `InvalidPacketTest` still green |
| D2 | Heartbeat round-trip at 0x80000000 / 0xFFFFFFFF passes with no UBSan report (pre-fix, these inputs trip UBSan's shift check) |
| D3 | Firefly over all indices 0–255 on a Controller strip: UBSan-clean; indices 0–31 outputs match pre-fix golden values |
| D4 | `EffectsTest` full fuzz unchanged-green; golden spot-check values identical to pre-fix captures |
| D5 | Full host suite green; review confirms single effect/packet/timestamp resolution per frame |
| D6 | `pio run -e node -e fancy-node` build; review confirms count fixed at construction |
| D7 | `pio run -e controller` builds; manual test below |
| D8 | `pio run -e node` builds; `grep -n print_alive_at src/devices/node/node.cpp` → no matches |

## End-to-end / hardware validation (final, after all commits)

1. `./ci.sh` — the exact CI pipeline (cmake without simulator + smalltests + largetests) must be green.
2. **Manual controller test (D7)** — flash `controller` env; in Effect mode press right button 2: *right* button 2's LED shows pressed brightness (pre-fix: left button 2's LED lit instead). Repeat for right buttons in DirectColor mode.
3. **Mesh relay smoke test (D1, if ≥3 nodes available)** — master + relay + far node placed so the far node only hears the relay: far node must adopt the master's effect/palette and animate in sync (pre-fix: far node never syncs, promotes itself to master).

## Done when

All 8 commits on the branch, each passing the per-commit gate at its commit; `./ci.sh` green at the tip; reviews recorded for commits 1–7; manual D7 test documented in that commit's message.
