# Data Model: Shared Simulator Rendering Engine

## Effect registration

| Field | Type | Rules |
|-------|------|-------|
| name | string | Human-readable, non-empty |
| kind | enum | Selects one production C++ effect factory |
| weight | byte | `>0` expands into the random wire table; `0` appends once to the manual tail |
| parameter | integer | Optional constructor value, used by blink variants |

The ordered definition table expands to the wire table. Total expanded entries must be less than 256. Display Color Palette and Dark remain the final two entries.

## Effect seed set

| Field | Type | Default simulator value | Production behavior |
|-------|------|-------------------------|---------------------|
| Fire offset | uint16 | 6198 | Random at construction |
| Firefly offset | uint32 | 423 | Random at construction |
| Rorschach offset | uint16 | 24359 | Random at construction |

A seed tuple identifies a reusable renderer handle. Default engines share the default tuple's handle; `setEffectSeed` selects another cached tuple.

## Palette metadata

| Field | Type | Rules |
|-------|------|-------|
| index | byte | Position in the authoritative production palette vector |
| name | string | Human-readable, non-empty, unique |
| colors | ordered HSV list | Exact production bytes; at least one color |

Palette byte inputs wrap by the production palette count.

## Device metadata

| Field | Type | Rules |
|-------|------|-------|
| index | integer | Stable within one artifact; not a wire protocol field |
| name | string | Existing public simulator identifier, unique |
| milliamps | uint32 | Production device budget |
| strips | ordered strip list | References exact production strip definitions |

## Strip metadata

| Field | Type | Rules |
|-------|------|-------|
| ledCount | byte | 0-255 |
| flags | byte bitset | Uses production `StripFlag` values |

The adapter expands flag bits into the existing public string names. C++ remains responsible for `Reversed`, `Dim`, and `Off` during rendering.

## Renderer handle

| Field | Type | Rules |
|-------|------|-------|
| id | positive integer | Opaque outside the ABI |
| seeds | Effect seed set | Immutable for the handle lifetime |
| device rigs | lazy map | At most one production `LedManager` rig per requested catalog device |
| output buffer | byte vector | Packed RGB bytes for the most recent render |

Invalid or destroyed handles return an error and never expose stale output.

## Render request

| Field | Type | Rules |
|-------|------|-------|
| renderer handle | integer | Must reference a live seed tuple |
| device index or custom strip | integer/strip | Catalog index must exist; custom strip is test-only adapter surface |
| effect | byte | Production wrap semantics apply |
| palette | byte | Production wrap semantics apply |
| network time | uint32 | Wraps naturally |
| control active | boolean | When true, control RGB overrides effect/palette |
| control RGB | three bytes | Ignored when control is inactive |

## Render result

Packed RGB bytes in device-global strip order, plus a status code. The JavaScript adapter partitions the bytes using authoritative strip metadata and produces the existing `{name, strips:[{flags, leds}]}` snapshot shape.

## Artifact manifest

| Field | Type | Rules |
|-------|------|-------|
| schemaVersion | integer | Manifest contract version |
| rendererAbi | integer | Must equal adapter expectation |
| emscriptenVersion | string | Exact pinned version |
| fakeFastLedCommit | hex string | Exact pinned commit |
| sourceFingerprint | hex string | SHA-256 over normalized ordered source inputs and build flags |
| files | map | Generated filename to SHA-256 and byte size |

The manifest contains no timestamps or absolute paths so two clean builds are byte-identical.
