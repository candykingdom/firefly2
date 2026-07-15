# Firefly2 Documentation

Code research notes for the Firefly2 LED control system. These are maintained alongside the code — if you change behavior documented here, update the doc.

## Contents

| Document | Covers |
|---|---|
| [architecture.md](architecture.md) | One-page mental model: how devices sync, layering, key design decisions, directory map. **Start here.** |
| [radio-network.md](radio-network.md) | RadioPacket wire format, flood mesh + dedup, master election, network time sync, effect scheduling timers, RFM69 config, protocol invariants |
| [led-effects.md](led-effects.md) | LedManager registry and weighted random selection, the `GetRGB` effect contract, full effect catalog, palettes, Perlin/math utilities, FastLED power limiting, the SDL simulator |
| [devices.md](devices.md) | StripDescription flags, DeviceDescription + flash storage (DeviceMode), the Devices.hpp catalog, per-target firmware walkthroughs (node, fancy-node, controller, dmx, disabled targets), battery thresholds, debug facility |
| [build-and-test.md](build-and-test.md) | Every PlatformIO environment, host CMake/GoogleTest setup, smalltests vs largetests, test doubles, CI workflows, lint, flashing and bootloader procedures |
| [hardware.md](hardware.md) | KiCad project catalog, board↔firmware mapping, bootloader install, common electronics |
| [simulator.md](simulator.md) | Browser simulator (`sim/`): hardware-free show testing, `window.sim` API, JS test suite, firmware reference vectors and regeneration |

## Related files elsewhere

- [`../CLAUDE.md`](../CLAUDE.md) — working-agreement summary for AI-assisted development (points back here)
- [`../GETTING_STARTED.md`](../GETTING_STARTED.md) — onboarding narrative for new engineers
- [`../README.md`](../README.md) — programming quick notes
- [`../bootloader/README.md`](../bootloader/README.md) — first-time bootloader flashing
