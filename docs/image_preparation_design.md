# Slice 8.1: offline image preparation and memory

Status: implemented, **Ready for review** (2026-10-26); not yet accepted.
Slice 08 is accepted; 8.2 remains Planned. The user selected **NPC profile C**
(area filtering with half-size source derivatives) for **all current and future
NPC appearances**, and **surface option A** (existing point sampling).
[Verification and handoff](slice81_verification.md) records the measurements and
screenshots. Both NPC and dungeon preparation now happen offline.

## Approved outcome and review checkpoint

Move NPC and dungeon wall/floor/ceiling raster preparation from game startup to
an incremental offline build. Keep the prepared-image renderer, projection,
1920×1080 presentation, grid movement, gameplay and all high-resolution masters.
Preserve source sheets, registrations and provenance. No atlas, paper dolls,
projectiles, archer content, LRU cache or on-demand image loading.

The completed selection checkpoint produced matching screenshots of current nearest sampling, alpha-aware area
sampling at current derivative dimensions, and area sampling at half dimensions.
It covered faces, weapons, transparent edges, reactions, near/far/diagonal views and
travel. The user explicitly requested to pick from screenshots. Production profile C was adopted only after that selection. Memory is not
scarce: prioritize detail; 512 MiB is acceptable headroom. The profile selection was not acceptance of the completed slice.

## Responsibilities, ownership and data

Offline asset I/O reads registered sheets and produces reproducible runtime
derivatives into a separate output root. Sampling profiles explicitly select
nearest or area and a common resolution multiplier. Area filtering integrates
premultiplied color and alpha over source footprints, with transparent samples
outside each registered cell; normalization returns straight-alpha pixels.
Ground planes and horizontal pivots are preserved; visible clipping is rejected.
The existing nearest profile remains reproducible for legacy registrations.

### Surface-filtering review (2026-10-26)

The user inspected current point sampling and 128/256-pixel area-filtered surface
variants, then explicitly selected **A: retain existing point sampling**. They
preferred its appearance at wall/floor boundaries. The experimental surface
filter, CLI switch and material-resizing changes were removed. Retain the
[comparison screenshots](screenshots/slice81/surface-comparison.png) as a historical
review record, not the production appearance. This does not change NPC profile C.
Both surface and NPC banks still move to offline baking; surface pixels must
remain identical to their accepted point-sampled baseline.

Offline preparation builds immutable NPC distance/travel rasters and shared
shadows, and the existing eight-phase dungeon surface cache. All 26 NPC depth
slots remain: same-row sprites are visible during strafes. Each NPC raster stores
only its nontransparent bounding rectangle plus the original canvas dimensions,
pivot and crop offset. Cropping changes storage, never projection or sampling.
Empty bodies keep valid records and independent shadows. Pixels remain contiguous
and frame rendering allocates nothing. With profile C, stationary pixels occupy 33.15 MiB and travel pixels 4.97 MiB.
All 396 captures match the pre-compaction profile-C reference pixel-for-pixel.

Split definition/timing/placement loading from source PNG decoding. Production
loads matching metadata and prepared bundles, without loading NPC source PNGs or
dungeon material images. The renderer takes ownership of prepared scene/NPC data;
preparation constructors remain for the bake tool and generated-fixture tests.
Application orchestration remains small; asset I/O owns file formats and validation.

Use separate versioned NPC and surface bundles with explicit fixed-width,
little-endian fields, checksums, counts and rendering-configuration signatures;
never dump C++ object layout. Validate all section ranges, counts, arithmetic,
indices, dimensions, finite values, appearance order and clip timings. Stream
into final owning arrays without retaining a whole-file duplicate. Missing,
corrupt or incompatible bundles fail with a rebuild command, never a silent
runtime bake. Publish each bank using temporary-file replacement. Delete the publication
manifest before baking and publish it last. Runtime validates its format/profile,
definition identity, file sizes and footer hashes before loading banks; each
reader then verifies the full payload against that footer. Missing publication
or mixed bank generations fail closed. Checksums are FNV-1a change/corruption
detection, not cryptographic authenticity guarantees.

## Build, regeneration and bounds

An incremental CMake bake target is required by the game target. Clean builds
prepare once; input, profile and bake-code changes invalidate output. Bundles
live in the build asset directory, not version control. Explicit tool output
roots support custom asset trees and comparison captures. Deterministic metadata
records the profile, registration, source/output fingerprints, dimensions and
format/tool versions. Hash source art offline, not on game launch. A future
viewport change invalidates the bundles and rebuilds derivatives from masters.

Separate resident prepared and tracked loading-working-set limits are each
512 MiB across scene/NPC storage. Keep existing structural bounds and reject
exhaustion. Report source pixels, stationary/travel/shadow pixels, surface pixels,
metadata capacity, bundle sizes and tracked loading high water independently.
Report process peak RSS separately; neither managed-byte accounting nor C++
allocation interception measures all decoder/driver allocations. Preallocate
final arrays and bounded raster scratch; no runtime decompression or file access
in the frame path. Verify capacity with two complete appearances without fixing
the future archer frame count.

## Validation and handoff

Generated fixtures cover sampling, transparent edges, anchors, malformed inputs,
reproducibility, bundle round trips, corruption/truncation, incompatible versions
and signatures, and budget failures using small injected limits. Compare compact rendering with the pre-compaction untrimmed capture reference;
retain generated-fixture projection, independent slab-occlusion and floor-shadow
checks. Round-trip fixtures cover asymmetric padding/pivots, empty bodies,
all available reaction frames, stationary/strafe/travel pixels and picking. Storage-only changes and dungeon surfaces must remain pixel-identical.

Check clean/incremental/rebake builds and startup without source art. Exercise
loaded bundles in allocation tests; production startup must never call raster
preparation. Run Debug/Release suites, focused sanitizers, hidden GPU lifecycle
and application-owned captures. Never use OS input or focus automation. Report
unavailable checks. Use shipped artwork separately from routine test fixtures.

Measure bake/load time, resident bytes and peak RSS before/after. The planning
probe (one Release run, not a guarantee) measured NPC decode 165 ms, NPC bake
144 ms, surface bake 362 ms and process peak RSS 291,824 KiB. Its NPC prepared
pixel count was 102,346,541 bytes; source pixels were 58,179,584 bytes.

Record exact regeneration commands, chosen sampling profile, measured budgets,
verification and the appearance integration contract for 8.2 in a durable
verification/handoff. This slice is Ready for review. Only the user can accept 8.1; 8.2 remains Planned.

## Implemented interfaces and appearance integration contract

- `assets/preparation.json` is the shared production policy: version 1,
  `npc_filter: area`, `npc_resolution: 0.5`, `surface_filter: nearest`.
  `loadPreparationProfile` validates the exact supported policy and hashes its
  canonical JSON. Every appearance receives it in the bake tool. Legacy optional
  per-definition `sampling` still supports standalone source tests/tools; it does
  not override the shared production profile.
- `prepareNpcDerivatives(level, output)` writes a separate generated tree. Each
  definition may name `source_sheets`, relative to its directory, for example
  `animation/sheets.json`. This invokes the existing registered sheet extractor
  directly from masters with profile C. Registration supplies canvases/pivots and
  ground positions; normalized anchors in the definition remain unchanged.
  Sheet extraction retains its authored alpha cutoff of 15. Paths must stay
  inside the definition directory. All referenced frames must be generated.
- A definition without `source_sheets` uses its source frame PNGs as masters.
  Their dimensions must be even, positive and at most 2048; area filtering halves
  them with the definition's cutoff and normalized ground anchor. A shared image
  path must have a consistent ground anchor. Both routes record deterministic
  source/output fingerprints, dimensions, registration/profile provenance. No
  source PNG or original sheet is overwritten.
- `loadNpcMetadata` reads definitions, timings and placements without decoding
  images. Its fingerprint covers canonical ordered definitions (including image
  paths/anchors), not placement positions or relocation of definition filenames.
  Runtime definition edits invalidate bundles even if clip durations stay equal.
- `PreparedNpcs` owns flat cropped pixel arenas and logical canvas records.
  Preparation counts crop bounds before allocating the final arena, then fills
  it in a second pass. This doubles offline sampling work while avoiding a dense
  temporary bank and allocator growth. Empty bodies retain shadow records.
  `npc_preparation.cpp` owns baking; composition files only read the immutable bank.
- `prepareScene` retains the exact point-sampled image algorithm. Bounded geometric
  reserves account for transient old/new vector storage. `ScenePreparationStats`
  reports material storage and preparation high water. Loading allocates exact
  serialized counts, so its resident capacity is lower than the baker's capacity.
- `save/loadPreparedScene`, `save/loadPreparedNpcs` use separate version-1 binary
  formats (`BLBSCN01`/`BLBNPC01`). Fixed-width little-endian scalar fields precede
  flat RGBA8/alpha arrays; offsets are explicit, never serialized C++ layouts.
  Shared headers carry renderer revision, viewport/projection signature and
  profile/definition fingerprints. Bump `prepared_io::rendererRevision` when
  changing lighting or raster semantics; a projection/viewport change also fails
  the stored signature. Rebuild from masters afterward.
- `SceneRenderer(PreparedScene&&, PreparedNpcs&&)` takes ownership. Production
  startup uses this constructor exclusively. `--check-prepared` performs the same
  CPU asset-loading path and exits before opening a window. A source constructor
  remains for generated-fixture tests and offline tools.
- `blobber_bake_assets SOURCE_ASSETS OUTPUT_PREPARED [--capacity-check]` prepares
  scene and NPC banks sequentially and publishes `metadata.json` last. The CMake
  `blobber_bake_runtime_assets` target depends on the tool, all NPC PNG/JSON inputs,
  surface PNGs, shared profile and maze. The game target requires it. Generated
  bundles/derivatives stay under the build tree, and an unchanged build skips it.

The 512 MiB resident and loading limits cover prepared containers and tracked
raster/source/scratch storage, not total process memory. JSON has separate bounded
file sizes. Decoder/encoder internals, allocator bookkeeping, filesystem buffers,
GPU/driver storage and unrelated game/UI state are outside that accounting.
Process peak RSS is reported separately. Sheet scratch is capped at 256 MiB;
derivative, scene and NPC bake stages do not retain each other's raster buffers.
Runtime streams each uncompressed bank directly into final arrays, validates before
use, then renders without file I/O, resampling or engine-owned frame allocations.
