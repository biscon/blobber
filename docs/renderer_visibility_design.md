# R03 — Billboards, picking and visual automation

Status: **Accepted and closed** (2026-10-31).
Approved by “Implement the plan.” Parent: [renderer roadmap](renderer_plan.md).
The user accepted R03: “slice accepted, record it in the plan doc”.
R02 is Accepted. R04 is next eligible for separate planning; it, campaign balancing,
and new content remain unstarted.

## Ownership and interfaces

A graphics-independent immutable CPU asset owner retains the world publication and
base alpha masks. GPU textures borrow this owner; active and staged views share it.
A CPU presentation pipeline consumes one movement/actor/animation/loot/fixture
snapshot, produces bounded camera-facing billboards and a logical depth/ID buffer,
and publishes a revision key. GPU color and live HUD use that same prepared value;
windowless visual automation needs neither color images nor a context. Geometry
and snapshot borrowers must outlive the pipeline. Replacement prepares before
publication; stale owner/revision checks apply even on cache hits.

Keep the existing ActorId/LootId/FixtureId, TargetSnapshot and command validation.
Extend PickingBuffer with preparation-allocated pixel depth; legacy column-depth
behavior stays available. Full depth distinguishes opaque geometry from bodies,
allowing owner-excluding badge clipping. Images and picking share a snapshot key.
Quality invalidates color, never interaction. Player transitions expose no picking.

## Transforms, coverage and drawing

Use NpcAnimator clip/frame/offset/flash/expiry selections and the registered pivots,
width overrides and directions. Generalize close-range formation fitting to the
continuous camera; bodies, shadow centers, attachments and canonical idle/prone
badge anchors share the fit. Direction selection uses player-cell centers and
actor travel, excluding camera setback and lateral fitting. Loot retains one bag
per cell, stable first-stack selection, existing 4–64 pixel sizing and at-feet
access. Fixture discovery/reachability stays authoritative.

Rasterize the retained triangles and billboard quads at 500×282 pixel centers,
with bounded frustum clipping, consistent edge coverage, perspective-correct UVs
and linear camera depth. Reuse chunk culling and triangle-bounds scratch. Actor
coverage requires nonzero prepared base alpha; loot/markers require alpha >=16.
World blockers win depth ties; live bodies win corpse ties and stable slot order
resolves remaining ties. Bodies win equal-depth loot/marker overlaps. Corpses
occlude without becoming targets; shadows never own IDs. Ground projection uses
the same camera and existing spellCell validation. Supersampling resolves color
only; IDs always use the canonical logical-center sample.

GPU drawing uses opaque geometry/proxies, floor shadows, and ordered premultiplied
billboards. Separate base-mask depth-writing coverage from filtered color-only
fringes preserves antialiased art. Retain attenuation/fog, hit flash and shadow
appearance. Restore GL/rlgl state before native UI. Presenter receives matching
picking data. Closed fixtures remain instantaneous solid-cell proxies over an
already-prepared base map; R04 owns slabs, hinges and animation barriers.

## Storage, lifetimes and failures

Reserve 256 bodies, 2368 loot candidates, 32 fixtures and 256 shadows. Never
silently omit authoritative targets. Fixed grouping/sort/clipping scratch and
reused buffers avoid frame allocations or texture/mesh resource uploads. Each
view has an additional 8 MiB CPU ceiling, including double-buffered visibility;
R02's 352 MiB aggregate CPU/GPU ceilings and registry limits remain. Count capacity,
not just size, and shared masks only once. Preparation errors retain the old
image, picking and owners. Old views are destroyed before borrowed sessions.

## Rollout and replay

Normal gameplay and existing visual mode remain legacy by default. Extend the
development world preview with seeded isolated GameSession/HUD interaction cases.
Add --mode visual --visibility legacy|world to the headless runner and Python
client. The world backend builds without graphics, uses only metadata/masks and
has no CPU color renderer. Advertise backend availability separately from capture.
Keep protocol 1, recording backend, world-visibility-v1, publication fingerprint,
format and appearance identity. Reject incompatible strict replays. Replay tooling
forwards the recorded backend; world replay captures explicitly report unsupported
until R04. Hidden preview scripts provide R03 GPU evidence.

## Validation and delivery

First establish CPU coverage parity and benchmark feasibility before wider wiring.
Use self-owned generated maps/art for alpha holes/edges, walls, living/dead overlap,
all registered clips/directions, travel/near clipping/fitting, attachments, ground,
badges, loot generation, fixture completion, cache invalidation and stale owners.
Compare live/windowless targeting and complete command eligibility. GPU diagnostic
ID coverage uses the same base-mask convention; require exact interiors and target
membership, with documented geometric edge differences bounded to one logical pixel.
Inspect actual filtered native/supersampled motion separately.

Run Debug/Release graphics/headless suites, focused sanitizers, protocol/replay/client
checks, CPU allocation guards, hidden GPU lifecycle/state restoration and separate
capacity stress. Benchmark 120 warmup/600 samples/three trials, separating visibility,
CPU submission, synchronized completion, transfers/cache and active/staged memory.
Investigate CPU visibility p95 above 2 ms on the R02 reference machine; no implicit
GPU readback or legacy-color fallback. Record actual evidence and limitations in
r03_verification.md, update this design to match implementation, then mark Ready
for review. User acceptance alone closes the slice. No OS input automation.

## Implemented interfaces and evidence

`world_assets::CpuAssets` owns the validated publication and retained base masks;
live/staged GPU views share a const owner, and the windowless adapter borrows one.
`world_render::VisibilityPipeline` borrows immutable scene/assets, binds one map,
validates snapshots before cache reuse, and produces `WorldFrame` billboards,
proxy cells, picking and loot membership. Body packets include direction-selected
release/torso attachments; their fitted models yield world attachment positions.
Three prepared depth/ID buffers cover front/back publication and opaque caching,
totaling 4,827,808 bytes with packet/sort storage, below 8 MiB per view. Triangle
clipping uses fixed 16-vertex scratch, half-open pixel-center edges and a 1e-5
camera-depth tie tolerance. No color frame or graphics dependency enters this code. Missing future loot/fixture
sprites and required clip directions reject preparation, before any normal frame
can need them.

`WorldRenderer::stage(FrameInput)` prepares base-map geometry plus instantaneous
fixture proxies and completes both CPU/GPU outputs before commit. Lazy shared
billboard resources roll back with failed first staging. `render(FrameInput)`
returns the matching image and exposes picking only while settled; geometry-only
R02 entry points remain available for controlled comparison. GPU billboards reuse
quad/proxy meshes and cached uniform locations; quality changes reuse canonical
visibility. Optional frame timing is diagnostic-only. Resources/borrowers are
released in lifetime order, including capture exceptions and scene replacement.

`PickingBuffer(true)` carries full opaque/combined pixel depth. The bounded UI
clipping helper handles 12×12 status art and 46×6 health bars, retaining legacy
health behavior for column-depth buffers. `automation::makeWorldVisual` uses the
same CPU pipeline and NpcAnimator without raylib. CLI/client/replay selection and
version identities follow the rollout above. No publication/save format changed.

[Verification and handoff](r03_verification.md) records actual suites, sanitizers,
GPU parity/failure checks, capacity/allocation evidence, 403 identical Debug/Release
captures, timing/memory reports and verification limitations unchanged. R03 is
Accepted; R04 and normal-renderer cutover remain unstarted.
