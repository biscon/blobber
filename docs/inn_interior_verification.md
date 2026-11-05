# Sanctuary inn — verification and handoff

2026-11-05. Implemented and mechanically verified; ready for user visual review.

## Delivered

Normal campaigns contain the city, three dungeon floors and an authored inn. F at
(16,5), facing east, enters; the interior arrival is (5,9), facing north. Facing
south there offers F to leave, returning to (16,5), west. Entry/exit are free and
use the existing staged fade/load/commit lifecycle. An immutable optional doorway
link per authored location uses world-scoped slot zero; slots never recycle.
Both approaches are protected from NPC placement. Doorway arrival validation reuses
the landing-safety checks at an explicit cell, retaining the two existing dungeon
landing slots and unchanged generator fingerprints.

The 12×12 map contains an 8×8 common-room floor, enclosing modeled walls and outer
solid padding. Its 32 wall placements and ten scenery placements remain below
existing capacities. Scenery bases preserve connected walking routes, including
the staff aisle. Shelves, table/chair groups, bar, hearth, stairs, pottery, ale
barrels and plants use nine named models and the existing prepared mesh pipeline.
Blender source, GLB, metric atlas exports and imagegen masters/prompts are retained
in [the inn pack](../assets/packs/sanctuary-inn/README.md).

Free long rest is a location-wide recovery flag, enabled only for the canonical
inn in campaigns. Outdoor sanctuary recovery is removed; its anchor now describes
recall arrival. Recall still costs one kit and does not heal. Ordinary short rests
and paid field rests retain their rules. HUD, rest panel, map, save listing and
protocol distinguish the inn from dungeon depth. The upper staircase is scenery;
NPCs, upstairs rooms, storage and other shop interiors remain deferred.

Schema/simulation are 10/14. Previous 9/13 saves reject without rewriting their
bytes. Generator/templates stay 9/7. Optional tagged hearth fingerprint data leaves
absent-hearth dungeon fingerprints unchanged; existing binary/content replay checks
remain. View-owned effect storage adds 24 packets for one immutable hearth anchor.
Deterministic 20 Hz flames/embers use no simulation RNG, picking IDs or dynamic
lighting. Pausing retains the frame; an offscreen hearth does not invalidate it.

## Executed checks

All work used one Debug configuration, at most two build workers, one test process
and sequential low-priority intensive jobs. No benchmarks or stress matrix ran.

- Built `blobber`, `blobber_headless`, affected tests and `blobber_review_inn`.
- `inn`: passed, including wrong-facing/remote entry, world-scoped requests,
  allocation-failure/canceled preparation, ignored unrelated depth fields, F command
  dispatch, all walkable free-rest cells, zero kits, outdoor rest rejection, active
  save/Continue, canonical validation, discovery and dropped-item persistence,
  inactive snapshots, return pose and allocation-free doorway queries.
- Added wire-path coverage after the initial test pass; the focused `inn` executable
  passes again. Actual protocol movement reaches the door, entry/exit emit exactly
  two free doorway transitions, five locations are reported, and no dungeon entry
  is unlocked by visiting the inn.
- `campaign`, `campaign_save_cli`, `map`, `recovery`, `commands`, `world_cpu`,
  `world_residency`, `world_integration`: passed at final integration (nine selected
  suites including `inn`, 17.02 seconds total).
- `world_integration` includes owned small hearth artwork: deterministic animation,
  pause/cache behavior, offscreen culling and zero guarded C++ allocations over
  warmed updates. Existing combat-effect coverage also passes.
- `sanctuary_city` and `campaign_save_smoke`: passed during development. Extended
  `campaign_save` ran once at integration and passed in 93.66 seconds, including
  malformed saves, progression, inactive effects, navigation and I/O failures.
- `git diff --check`: clean. The bounded Blender exporter completed with model/UV
  packing and geometry-bound checks. Blender's sandbox-denied thumbnail/cache writes
  did not affect successful packed source, GLB or runtime exports.

## Hidden OpenGL checks and captures

The application-owned runner used NVIDIA GeForce RTX 3060 / OpenGL 3.3, hidden
1920×1080 windows, supersampling and environment AO. No OS input, window selection
or focus automation was used. The first sandboxed display attempt was unavailable;
the authorized hidden OpenGL run succeeded. See the [capture gallery](screenshots/inn/README.md).

Reviewed the common room, bar, seating, fireplace, staircase, plants, ale barrels,
exit, rest panel and map. Close-up review exposed inward side faces on rounded props;
the exporter now orients and asserts outward side normals before publication. The final
asset rebuild and complete hidden capture pass succeeded after that correction. The runner also checks staged cancellation and commit, paused fire/image cache,
unchanged picking revision while flames animate, disk Continue and source asset
release. All pass. These are application/driver integration checks, not a claim
that driver internals never allocate. Interactive gameplay and final visual acceptance
remain user-owned.

| Measurement | Observed | Existing ceiling |
| --- | ---: | ---: |
| City-only mip textures | 261.29 MiB | 512 MiB resident |
| Inn-only mip textures | 133.09 MiB | 512 MiB resident |
| City + staged inn mip textures | 348.09 MiB | 512 MiB resident |
| Overlap alpha masks | 7.83 MiB | 64 MiB |
| Overlap tracked world CPU | 28.68 MiB | 352 MiB |
| Overlap tracked world GPU | 377.10 MiB | 608 MiB |
| Peak application GPU, including UI/scratch | 492.70 MiB | Advisory reference, not a hard cap |
| Active-inn serialized campaign | 2.05 MiB | 16 MiB |

Cancel and return restore city-only residency. Releasing the final view leaves zero
location textures and alpha; renderer/UI scratch follows its existing lifetime.
No resource ceiling or texture-density reduction was needed.
