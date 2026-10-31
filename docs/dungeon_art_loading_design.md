# Dungeon art, stairwells, doors and responsive loading

Status: **Implemented; verified and ready for interactive review**, 2026-10-31.
The user approved the plan and requested ImageGen replacements for doors, hub decals
and outdoor sky. This is independent of the closed renderer/campaign roadmaps.

## Responsibilities and interfaces

Map WorldDefinition owns a stable asset-pack ID (default dungeon). Offline pack
manifests resolve wall/floor/ceiling/door/edge/stair/decal/sky artwork to bounded
publication indices. Cell material indices remain pack-local. Prepared immutable
CPU assets and GPU texture leases remain shared across active/staged views. Asset
purpose and sampler behavior are independent; skies are not terrain materials.
Source art, ImageGen prompts, manifests and publication fingerprints are retained.

Door frames use world-space planar UVs at wall density; leaf broad faces fit the
rectangular artwork, narrow edges use a wood region. Session fixture storage owns
current/remembered open bits separately from monotonic completion. Ordinary doors
toggle from either threshold side, keys remain acquired, guardian gates stay automatic.
Occupied thresholds (including live movement endpoints) reject closing. Accepted
changes invalidate geometry/navigation/discovery and animate for 300ms; logical
collision changes atomically and motion blocks commands. Saves retain settled state.

Stairs are eight-step procedural, cardinally rotated static meshes occupying one
cell with side walls and an opaque dark termination. Shafts extend vertically only
for presentation; there are no stacked simulation cells. The visible descending
flight falls 0.25 cells over its one-cell run so the fixed camera can see its treads
over the landing lip; a full-height 45-degree descent would be hidden by that lip.
The ascending flight rises one cell. Descending tread noses have a small bevel
that catches light; darkness then hides the continuation. Their floor/ceiling
footprint is replaced. All CPU/GPU depth and picking use identical triangles and
bounds. Stair cells block movement; the designated adjacent landing supplies F
interaction and safe room-facing arrival. Generation validates these new landings.

Application loading owns one CPU worker and bounded work/results. Disk/checksum,
parsing, campaign construction/restore and CPU scene preparation avoid the window
thread. Window/UI/GPU operations remain on the window thread, yielding after
individual operations with a 4ms soft work budget. Startup shows minimal loading
before full UI preparation; progress reports phases, never invented percentages.
One queued CPU task/result is consumed before submitting another. UI/font reads
reject beyond 64MiB before decode; prepared world textures retain their 128MiB read
ceiling and validated mip dimensions. UI/font bytes read on the worker, while one
raylib image/font at a time decodes on the window thread. No raylib APIs run on the
worker. RAII unwinds failures, joins tasks and destroys GPU leases before the window.
Driver operations remain indivisible and are not given hard timing guarantees.

Travel presentation fades all content out over 250ms, publishes a black frame,
prepares the destination while pumping events, commits while black, then fades in
over 250ms. Delayed loading status appears after 250ms. Simulation/Party accesses
freeze while workers borrow them; held input must release before gameplay resumes.
Failure fades back to the unchanged source. Recall charges only at successful commit.
Headless travel remains synchronous without presentation delays. Cancellation joins
workers before dependencies and GPU/window resources are destroyed.

## Ownership, bounds and compatibility

There is one loading worker, no unbounded task graph or inactive simulation. Existing
256MiB textures, 64MiB alpha, 64MiB CPU scene/40MiB GPU scene, 8MiB view scratch,
352MiB subsystem and 512MiB loading bounds continue to apply; failures reject staging.
Frame work allocates no engine-owned storage. Preparation/capture may allocate.

Campaign schema/simulation and generator/template identities advance for new door
state and landings. Older saves reject clearly without overwriting their files; no
migration. World publication/replay identities advance for changed interpretation.
Pack IDs and door state participate in persistence/fingerprints. Final identities:
campaign schema/simulation 3/3, generator/templates 4/4, world publication 2,
appearance 4, visibility world-visibility-v4, terrain-rules-v2 and
shared-door-motion-300ms-v2. Publication fingerprints hash canonical metadata,
including its checked payload checksum: changing role bindings or UV scale changes
identity even when texture payload bytes are identical.

## Verification strategy

Use small owned fixtures for UV density, door toggling/keys/occupancy/stale requests,
collision/discovery/navigation, stair orientations/landings/depth/picking, pack
validation/budgets and save roundtrips. Test transitions as a CPU state machine,
including black-before-preparation, rollback, paused clocks and exact-once cost.
Test steady-frame engine allocation guards. One build configuration, two build
workers maximum and one test process. Run focused checks while iterating, persistence
coverage once at integration, one hidden GPU integration suite and deterministic
application-owned captures. No OS input, focus or desktop automation. Report actual
checks and limitations in the final verification record.

## Implemented source boundaries

- `src/core/preparation.hpp`: one immovable worker, preparation scope/checkpoints,
  cooperative cancellation, exception transport and joined results. No frame tasks.
- `src/app/loading.hpp` and `travel_fade.hpp`: cached loading presentation and a
  graphics-independent blackout state machine; gameplay orchestrates transactional travel.
- `src/assets/preparation_file.hpp` / `prepared_image.hpp`: bounded worker reads,
  followed by main-thread raylib decoding. Full UI atlases still prepare once.
- WorldDefinition stores the pack ID; publication Pack records own bounded palette
  bindings. Visibility resolves the pack once during preparation, never per frame.
- `world_stairs.*` prepares fixed-capacity geometry. Scene building batches by
  material and fixture picking ID, includes extended vertical bounds, and retains
  the base topology independently of ordinary dynamic doors.
- Door leaves hinge through their thickness, leaving clearance at 90 degrees to
  avoid coplanar wall/leaf picking disagreements. Current/remembered open bits enter
  snapshots and digests; keys and guardian progress remain monotonic.

ImageGen source art and the complete final prompt set are retained under
`assets/packs/dungeon/source/` and `assets/packs/dungeon/prompts.json`. The default
manifest reuses the existing stone/floor/ceiling art and selects generated door,
locked variant, shield crest, vine and soft overcast panorama. The baker resizes
sources, preserves decal alpha and joins panorama edge texels for horizontal wrap.
Unused stair billboard textures are no longer published or required.

The CPU-only `blobber_world_assets` target is separate from scene geometry. The
offline baker links that asset target, so changing stair/scene meshes does not
rebuild NPC art or republish unchanged textures.

[Verification, captures and limitations](dungeon_art_loading_verification.md) record
the executed checks and retained artwork.
