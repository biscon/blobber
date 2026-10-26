# Slice 8.1 verification and handoff

Status: **Ready for review**, 2026-10-26. Implementation was authorized by
“Implement the plan.” The user selected **NPC profile C** (alpha-aware area
filtering and half-size source derivatives) for **all current and future NPCs**,
and **surface option A** (existing point sampling). Profile selection was not
acceptance of the whole slice. Slice 8.2 remains Planned.

## Delivered

The shared `assets/preparation.json` applies C to every appearance in the offline
bake. Originals, registered sheets and high-resolution provenance remain intact;
the goblin definition only gains a `source_sheets` authoring reference. The build
produces separate derivatives and versioned scene/NPC bundles, with reproducible
fingerprints and a publication manifest written last. Missing, corrupt, stale or
mixed-generation bundles fail with rebuild guidance. Runtime loads JSON metadata
and streamed prepared arrays; it does not decode NPC/material PNGs or resize them.

Compact NPC storage retains only nontransparent rectangles while preserving
logical canvases, ground anchors, all 26 depth slots and independent shadows.
Dungeon raster sampling remains unchanged. The frame path remains allocation-free
for engine-owned processing. See the [implemented design](image_preparation_design.md)
for responsibilities, bounds, formats and the new-appearance contract.

## Visual review

The original comparison uses A = nearest/full, B = area/full, C = area/half.
Both B and C filter sheet extraction and stationary/travel preparation.

- [Near comparison](screenshots/slice81/comparison-near.png)
- [Directions](screenshots/slice81/comparison-directions.png)
- [Hurt/death](screenshots/slice81/comparison-reactions.png)
- [Far/travel](screenshots/slice81/comparison-distance-travel.png)
- [Approved C native screenshot](screenshots/slice81/C-area-half-full.png)
- [Surface comparison](screenshots/slice81/surface-comparison.png): historical
  rejected experiment; the user retained A. The experimental surface filter and
  its CLI switch were removed.

The three NPC comparison runs each produced **396 PNGs**. Final source-free runtime
captures also produced **396 PNGs**, all **pixel-identical** to the approved C
reference captured before compact storage and bundle loading. This checks both
storage changes and retained surface pixels. Coverage includes near/far/diagonal
views, strafe, facing, reaction/corpse frames, hybrid encounters and NPC travel.
Native reference/final front images were visually inspected. Captures use hidden
application-owned windows and in-process state; no OS input/focus automation.
Interactive desktop review is left to the user.

Full temporary capture sets: `/tmp/blobber-81-area-half-captures` (reference),
`/tmp/blobber-81-runtime-only-captures` (final). Durable comparison images are linked
above; temporary files are not required for builds or tests.

## Memory and timing

All byte figures below are measured with the shipped complete 52-frame appearance.
The earlier baseline used nearest/full images and retained dense prepared bodies.

| Storage | Before | Final |
| --- | ---: | ---: |
| NPC source pixels during preparation | 58,179,584 B | 14,544,896 B |
| NPC source pixels loaded by game | 58,179,584 B | 0 B |
| NPC stationary pixels | 92,144,944 B | 34,758,592 B |
| NPC travel pixels | 10,186,752 B | 5,215,276 B |
| NPC shadow pixels | 14,845 B | 14,845 B |
| Total NPC prepared pixels | 102,346,541 B | 39,988,713 B |
| Final NPC metadata/capacity overhead | — | 60,784 B |
| Final NPC resident storage | — | 40,049,497 B (38.19 MiB) |
| Final loaded scene storage | — | 114,664,792 B (109.35 MiB) |
| Total loaded prepared storage | — | 154,714,289 B (147.55 MiB) |

NPC prepared pixels fell **60.9%**. The scene and NPC files occupy 114,663,460 and
40,049,121 bytes. Loading uses exact final arrays without a whole-file copy.
The scene baker's transient/growth capacity differs from loaded resident capacity.

Tracked offline high water: derivatives **40,179,744 B**, NPC source decode copies
**14,956,544 B**, NPC raster preparation **57,911,449 B**, scene preparation
**126,080,672 B**. Stages release raster ownership between scene and NPC work.
Runtime prepared-array loading peaks at its **154,714,289 B** resident total.
Separate resident and tracked loading limits are each **536,870,912 B (512 MiB)**;
exhaustion is explicit, with no LRU or runtime fallback.

The separate capacity run duplicates the entire complete appearance (104 frames,
without assuming an archer frame count). Conservative scene-baker capacity plus
two NPC banks totals **202,701,882 B (193.31 MiB)**; NPC raster loading high water is
**112,504,538 B (107.29 MiB)**. This establishes useful headroom, not an unlimited
promise for arbitrary future content.

One Release startup probe previously measured NPC decode 165 ms + NPC bake 144 ms
+ scene bake 362 ms (~671 ms), with process peak RSS 291,824 KiB. The final headless
application startup with source art absent measured **165 ms** for the prepared
asset-loading phase, **173 ms** total process wall time and **157,708 KiB** peak RSS.
A normal offline Release bake measured about **3.0 s**; a separate full bake plus
the two-appearance capacity check measured **5.55 s**, peak RSS **129,992 KiB**.
These are individual runs with filesystem caching, not latency guarantees. The
old probe did not include offline sheet extraction, so its work is not equivalent
to a complete bake. Startup no longer repeats downsampling.

Tracked byte limits cover owned raster arrays, their metadata/capacity and explicit
source/scratch buffers. JSON has separate bounded file sizes. Decoder/encoder
internals, allocator overhead, file buffers, GPU/driver allocations and other game
state are not included. Process RSS is measured separately using Python's child
process `resource.getrusage`, not asserted equal to the tracked byte count.

## Checks executed

- **Debug and Release: 22/22 passed.** Added bundle/source-profile tests cover
  deterministic serialization, metadata without PNGs, future static NPC derivatives,
  alpha-aware downsampling, relocation identity, malformed anchors, empty crops,
  budget preflight, removed sheet registrations with stale generated files, missing
  publication, mixed generations, corruption with valid
  checksums, invalid counts/indices/NaNs/signatures, truncation and trailing bytes.
  Round trips check stationary/strafe/reaction/travel pixels and picking.
- Existing generated-fixture tests retain independent area-integral, wall slab,
  floor-shadow, projection, lifetime, animation and movement checks. The allocation
  guard now exercises **loaded bundles**, including first and subsequent frames.
- Focused **ASan/UBSan: 3/3 passed** (`prepared_assets`, `image_preparation`,
  `npc_animation`, `ASAN_OPTIONS=detect_leaks=0`). Final empty-crop/preflight/stale-sheet additions
  were also rerun in Debug, Release and ASan/UBSan. LeakSanitizer was attempted but
  fails because this execution environment uses tracing/ptrace; leak detection is
  therefore unverified. Third-party raylib itself was not sanitizer-instrumented.
- Hidden GPU lifecycle check passed, including resource replacement, stale handles,
  capacity exhaustion, render/readback and cleanup.
- Clean Ninja build in `/tmp/blobber-81-clean` baked once. Rebuilding the unchanged
  bake target did no work and preserved output mtimes. Touching the profile input
  rebaked all outputs; its original mtime was restored afterward. Clean, incremental
  rebake and normal Release banks **and manifest are byte-identical** (SHA-256).
  Bake-code changes also triggered rebuilding the tool and generated outputs.
- A runtime-only tree containing metadata, prepared banks, rules and UI/fonts had
  **zero NPC PNGs and no dungeon texture directory**. `--check-prepared` and all
  396 hidden captures succeeded. Runtime no longer requires those source images.
- Legacy nearest extraction reproduced all 52 original PNG frames and `frames.json`
  byte-for-byte. Before adopting C, 183 common baseline captures matched unchanged.
  All master PNGs remain unchanged in the worktree.
- Compiler logs were checked for warnings; `git diff --check` and document links
  were checked. Routine tests own generated fixtures and isolated temporary output.

Known pre-existing limitation: broad `--capture` fails with “Deterministic combat
capture did not reach victory,” reproduced before this slice. It is not counted
as passing or weakened. `--capture-preparation` explicitly runs the scene, reaction,
hybrid and AI/travel suites and completes. No interactive desktop review was run.

## Rebuild and review

```sh
cmake --preset release
cmake --build --preset release -j4
ctest --preset release
cmake-build-release/blobber --check-prepared
cmake-build-release/blobber --hybrid-demo --seed 5
```

The game build automatically requires `blobber_bake_runtime_assets`. An unchanged
build does not rebake. For a separate asset tree or a capacity measurement:

```sh
cmake-build-release/blobber_bake_assets assets /tmp/blobber-prepared --capacity-check
```

For deliberate regeneration, remove only the generated files
`cmake-build-release/assets/prepared/{scene.bin,npcs.bin,metadata.json}`, then run
`cmake --build --preset release --target blobber_bake_runtime_assets`. Custom asset
trees passed to `--assets` require their matching `preparation.json`, NPC definition
JSON and `prepared` directory. Source PNGs are needed only by the bake tool.

Preserved-master extraction remains available separately:

```sh
cmake --build --preset release --target blobber_prepare_npc_animation
cmake-build-release/blobber_prepare_npc_animation assets/npcs/goblin/animation/sheets.json \
  --filter area --resolution 0.5 --output /tmp/blobber-npc-derivatives
```

## Next-agent handoff

Await user review of 8.1. Do not mark Accepted or start 8.2 without explicit
acceptance. Keep profile C global and surfaces point sampled. Future NPC authors
follow the [appearance integration contract](image_preparation_design.md#implemented-interfaces-and-appearance-integration-contract):
register masters with `source_sheets` or provide even-sized frame masters, preserve
anchors and clip timings, and rebuild. Do not add per-NPC filtering exceptions.
No archer art, projectile work, gameplay changes or viewport change belongs here.
