# Low-resolution 3D renderer: design and delivery roadmap

Created: 2026-10-30. Last updated: 2026-10-31.
Status: **Roadmap accepted; R01–R04 and milestones R-A/R-B Accepted.**

The user agreed to replace the prepared perspective-image world renderer with a
low-resolution GPU 3D world, retaining billboard monster/item artwork and discrete
movement. Stacked cells are explicitly deferred. The user requested a slice plan
like the campaign and older demo plans; this document supplies that plan, not
authorization to implement all slices automatically.

The [campaign roadmap](procedural_campaign_plan.md) remains paused after accepted
P-B (PC01–PC06). Renderer/presentation work comes first, additional monsters next,
then a return to campaign planning. New monsters, campaign balancing and PC07 are
not renderer deliverables. Prior accepted gameplay remains authoritative except
for the explicit, bounded elevation extensions described below.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Latest accepted foundation | Campaign P-A/P-B and prior demo/automation slices; current renderer remains production |
| Accepted direction | Low-resolution 3D world, existing billboard artwork, interpolated camera, static prepared meshes, outdoor/height/decoration support; no stacked cells |
| Active slice | None; R04 accepted and closed |
| Awaiting review | None |
| Next eligible slice | R05 — Heights, outdoor hub and decoration; eligible for separate planning, not started |
| Next action | Plan R05 when requested; obtain its detailed-plan review before implementation |
| Blockers | None known; filtering, picking parity and performance require evidence from the slices |
| Campaign status | Paused after P-B; do not start PC07 |

Update this checkpoint, tracker, decision log and affected execution record together.
Direction approval, detailed-plan approval, implementation and user acceptance are
different states. Only the user can accept a delivered slice.

## Delivery workflow

1. Read [AGENTS.md](../AGENTS.md), [system design](system_design.md), this checkpoint
   and the preceding slice's verification/handoff. Inspect current code and preserve
   existing working-tree changes.
2. Work on one slice at a time. Mark it **In progress — planning**, write its
   practical detailed design under `docs/`, and link it from the system overview.
   Specify ownership, interfaces, capacities, failure behavior and validation.
3. Obtain review of that slice's plan before implementation. Existing explicit
   approval in the conversation suffices. Do not re-request settled direction
   decisions or turn routine reversible implementation choices into approval gates.
4. Implement the slice and its affected tests, automation and resource lifecycle
   together. Keep normal play working while the replacement is incomplete.
5. Record exact checks, outcomes, visual evidence, limitations and next-agent
   instructions in `docs/rNN_verification.md`. Update designs to match the result.
6. Mark **Ready for review**. After explicit user acceptance, close the slice and
   identify the next eligible slice for planning; do not automatically implement it.

Status vocabulary: **Planned**, **In progress — planning/implementation**,
**Ready for review**, **Accepted**, **Blocked**, **Superseded/Retired**. A blocked
record must identify the external prerequisite or decision, not merely unfinished
work. If a slice proves too large, propose a suffixed split (for example R05.1)
with revised dependencies; never declare acceptance by dropping failed criteria.
No commits, toolchain changes, balance experiments or installed-agent changes
are implied by this roadmap.

## Scope and appearance contract

- Keep the fixed 1920×1080 presentation, existing native-resolution HUD and logical
  640×360 input/layout coordinates. Start with the existing **500×282 dungeon
  viewport** at its current location. A 640×360 world target or larger viewport
  would be a separate layout choice, not an incidental renderer migration.
- Preserve 84° horizontal FOV, horizon at 34% of viewport height, 0.5-unit eye
  height and 0.4-cell camera setback as the matching baseline. Use an off-centre
  projection rather than camera pitch to reproduce the horizon. R01 must inspect
  the setback during turns and near walls: reject camera paths that clip through
  corners, and document any necessary presentation-only correction for review.
- Keep grid coordinates, four headings, movement queue/collision behavior,
  simulation clocks and accepted transition durations. Interpolate camera position
  and yaw from authoritative movement snapshots; never drive gameplay from camera
  coordinates. Include strafe, backward motion, animation-off and paused movement.
- Port the existing `shadeScene` attenuation, face factors and dark blue-grey fog
  first, using linear camera-space depth. Outdoor ambient/fog/sky settings are a
  later environment profile. No PBR, dynamic shadow system or lighting engine is
  needed to establish the look.
- Keep monster animation clips, directions, pivots, attachments and item artwork.
  Their texture packaging changes; content replacement is not part of this work.
  Existing close-range lateral fitting is a deliberate visual behavior: compare
  it with world-space billboards before deciding whether to preserve or revise it.
- Finish with nearest-neighbour enlargement of the logical world image. Compare
  native resolution with mipmapped materials against 2× per-axis supersampling
  resolved back to the same logical image. Use moving sequences, not just stills.
  World-texture minification and final image enlargement are separate decisions.
  MSAA may be an additional measured candidate, not a substitute for texture
  filtering. No temporal jitter/TAA or camera bob in the initial baseline.
- **User-selected quality policy (2026-10-31):** supersampling is the default.
  Retain a user-facing Supersampling on/off option: on renders at 1000×564 and
  resolves to 500×282; off uses native filtered 500×282 rendering (probe button 2).
  Both retain mip filtering and nearest final enlargement. Raw nearest (button 1)
  remains a diagnostic control, not the production “off” setting. Quality changes
  must preserve camera pose, logical viewport, UI coordinates and gameplay picking.
- Support single-layer floor/ceiling variation, outdoor sky, wall decorations,
  simple static mesh props and procedural slab doors. No stacked traversable cells,
  bridges over passages, jumping, flying, falling, climbing or free-look controls.

## Existing implementation and migration boundaries

The current `SceneRenderer` composites prepared pixels on the CPU. Its one-depth-
per-column wall coverage assumes equal-height walls; it cannot simply be extended
to handle arbitrary vertical occlusion. `FramePipeline` caches source/destination
images and supplies image transitions, except for prepared strafe phases.

`PickingBuffer` is also gameplay infrastructure: body/loot/fixture IDs, visible
target membership, anchors and status-icon clipping feed commands and UI. The
windowless visual automation backend uses the same CPU renderer to obtain those
results. Removing it without replacing this contract would break automation or
change which commands the player can issue.

`Presenter` currently uploads a CPU world image and composes separate native game
and UI targets. R02 now adds a typed GPU-world entry and a shared
registry with 512 texture/eight target/eight mesh/eight shader slots, with separate
world byte budgets. Normal gameplay still uses the CPU entry; raw GPU ownership is
not exposed to gameplay. Campaign travel stages a replacement view alongside
simulation state before committing. Preserve that transaction and failure behavior.

Useful source entry points:

| Responsibility | Current source |
| --- | --- |
| Projection, fog and image composition | `src/render/projection.hpp`, `lighting.hpp`, `scene_renderer.cpp`, `frame_pipeline.cpp` |
| Sprite placement and animation | `src/render/npc_projection.hpp`, `npc_sprites.*`, `src/game/npc_animation.*` |
| Picking and UI occlusion | `src/render/picking.hpp`, `src/ui/status_occlusion.hpp`, `src/game/target_visibility.hpp` |
| World geometry and navigation | `src/world/map.*`, `fixtures.*`, `src/game/movement.cpp`, `navigation.cpp`, `perception.cpp` |
| Presentation/resources | `src/platform/presenter.*`, `gpu_resources.*` |
| Assets and build | `src/assets/prepared_*`, `npc_metadata.*`, `npc_derivatives.*`, `CMakeLists.txt` |
| Launch, travel and save | `src/app/session_launch.cpp`, `gameplay.cpp`, `src/game/campaign*`, `src/persistence/campaign_*` |
| Windowless visual observations | `src/automation/visual.*`, `session.*`; [headless design](headless_design.md) |

## Proposed architecture

### World data, geometry and camera

Keep simulation data graphics-independent. Static mesh generation consumes a
read-only geometry/environment description and produces contiguous vertex/index
arrays, material ranges and chunk bounds. Emit exposed faces, not six faces for
every solid cell. Build at map preparation, upload once, and reuse until the
geometry description changes. Doors remain separate dynamic instances.

Start with small fixed-size chunks and material batches; choose actual dimensions
and limits in R02 using R01 measurements. Frustum/distance culling is sufficient
initially. Submit a conservative region throughout camera transitions, including
newly revealed corners and turning views. Exact visible-cell rebuilding, portal
graphs, GPU occlusion queries and streaming are unnecessary at current floor scale.
Visibility is not discovery: rendering must not reveal diagnostic map knowledge.

A CPU camera value contains view/projection parameters and settled/interpolated
pose. Mesh rendering, billboards, ground projection, picking and capture scripts
consume that same value. Camera interpolation is presentation-only; accepted
commands, pending impacts and simulation timestamps keep their existing ownership.

### Assets, GPU ownership and frame work

Retain offline preparation where useful: pack source frames, normalize pivots,
prepare alpha/mip data, validate references and publish a versioned manifest.
Replace depth/strafe-specific perspective raster banks with reusable material and
sprite textures. Decode/allocate/upload at explicit preparation boundaries.
Preserve reproducible incremental builds and clear errors for missing, corrupt,
incompatible or over-budget publications. Source art is not a routine frame input.

Application-owned shared assets outlive active/staged scene views. Each scene owns
its static mesh resources, bounded dynamic-instance scratch and cached visibility
data. Platform RAII owners release meshes, shaders, textures and render targets
before context destruction. Use typed generation-checked handles where resources
are recyclable; borrowed raw GL/raylib values stay within platform operations.

Render opaque world/doors, alpha-tested billboard bodies and cutout decorations
with depth writes, followed by the explicitly ordered translucent effects/shadows.
The chosen alpha-edge treatment must preserve existing artwork; hard discard is
not permission to remove antialiased edges without review. Resolve supersampling
if enabled, enlarge the logical world image and draw native UI. Reuse cached world
output when camera, geometry, actors and effects are unchanged.

Normal engine-owned frames allocate nothing: reserve draw lists, visibility/depth
buffers and upload scratch at preparation. Define numerical limits and overflow
behavior in R02/R03 before coding. Scene/resource exhaustion fails preparation
without replacing the old playable scene. Never silently omit collision geometry
or targets when a buffer fills. Dynamic cosmetic overflow uses an explicit bounded
policy, preserving the current separation from authoritative combat outcomes.
Include the peak memory of both old and staged scenes in budgets.

### Picking, visibility and automation

Proposed default: a reusable **CPU depth/ID visibility pass**, without CPU color
rendering, consumes the same geometry, camera, billboard transforms and prepared
alpha masks. It supplies exact logical-pixel picking, target membership, loot and
fixture handles, anchors and status occlusion to both live UI and windowless visual
automation. This avoids making gameplay decisions dependent on driver readback or
requiring a GPU context for ordinary visual observations.

This is real migration work, not a promise that the old column buffer can survive.
R03 must establish rasterization/coverage conventions and compare the result with
GPU captures, especially sprite holes, wall edges, overlapping actors, dead bodies
and near clipping. Publish IDs and pixels from the same snapshot/revision. Stale
world/generation/revision checks and disabled interaction during transitions remain.
Do not average IDs when resolving supersampling: define a deterministic coverage
selection rule and a consistent alpha threshold for interaction. Cosmetic filtering
must not change authoritative visibility between quality settings.

Benchmark this pass before committing to its production implementation. If it is
too expensive or cannot satisfy parity, R03's detailed plan must revise the approach
explicitly, including windowless behavior and synchronization cost. A full GPU ID
readback every frame is not an unexamined fallback. No separate permissive target
list for automation, and no retaining the entire obsolete color renderer solely
to keep visual mode functioning.

CPU diagnostic/headless builds must remain windowless and graphics-independent.
Visual observations may load prepared masks/metadata without a context; actual GPU
images remain a separate hidden-context capture capability. Replay identity must
record changes to visibility semantics/assets where relevant, and reject incompatible
strict replays clearly rather than claiming unchanged pixel-level results.

### Heights, environments and persistence

R05 introduces a compact row-major extension with one floor elevation and optional
ceiling per cell, material/environment references and bounded decoration records.
Keep cell identity two-dimensional. Flat existing maps receive explicit defaults.
Quantized elevations and a maximum legal step are preferable to unconstrained
per-cell floating-point values; exact units/ranges belong in the R05 design.

Different floor elevations are not just mesh offsets. Shared edge traversal must
check step/clearance limits for player movement, actors, pathfinding and generation
validation. Preserve existing movement costs and ranges unless separately approved.
Ground loot, shadows, spell areas, eye heights and attachment positions follow the
surface. LOS, discovery and targeting must agree with height-dependent blockers;
do not create invisible barriers or visible shoot-through walls. Define/test the
supported height geometry rather than implicitly adding a general 3D tactics system.
No new cover bonuses, elevation damage bonuses or vertical area spell rules.

Sky profiles, ceiling omission and outdoor fog are environment data, not a separate
game mode. Wall-attached decorations identify their cell/face, offset, size and
material; transparent planes need depth-bias and sorting rules. Static decorative
models initially have no independent collision: blocking props must correspond to
authoritative blocked cells, and nonblocking placements must not obscure passage
semantics. No general model editor, skeletal pipeline or physics subsystem.

Campaign snapshots/save content identities must retain or deterministically recover
environment/height/decoration data, including inactive locations. Never regenerate
random decorations differently after travel or Continue. Choose an explicit schema
migration or clean incompatible-version rejection in R05; no silent reinterpretation
of old saves. GPU resources, camera interpolation and transient door animations are
not serialized. Doors restore directly to the authoritative open/closed state.

## Slice tracker and milestones

Six slices separate the visual decision from infrastructure, interaction and full
gameplay integration. Four would bury picking and automation in an oversized
renderer rewrite; more than six is not currently justified. R03 and R05 carry the
largest uncertainty and may warrant a reviewed split if their prototypes expose it.

| ID | Slice | Dependency | Status | Milestone |
| --- | --- | --- | --- | --- |
| R01 | [Visual and motion proof](#r01-visual-and-motion-proof) | Accepted direction | Accepted (2026-10-31) | R-A |
| R02 | [Production world rendering and assets](#r02-production-world-rendering-and-assets) | R01 accepted | Accepted (2026-10-31) | R-B |
| R03 | [Billboards, picking and visual automation](#r03-billboards-picking-and-visual-automation) | R02 accepted | Accepted (2026-10-31) | R-B |
| R04 | [Doors, effects and campaign integration](#r04-doors-effects-and-campaign-integration) | R03 accepted | Accepted (2026-10-31) | R-B |
| R05 | [Heights, outdoor hub and decoration](#r05-heights-outdoor-hub-and-decoration) | R04 accepted | Planned | R-C |
| R06 | [Production cutover and retirement](#r06-production-cutover-and-retirement) | R05 accepted | Planned | R-C |

| Milestone | User-visible outcome | Status |
| --- | --- | --- |
| R-A — Look established | Representative room and moving comparisons establish camera, filtering and art compatibility | Accepted (2026-10-31) |
| R-B — Playable replacement | Current campaign/arena work through the GPU renderer, including combat, interactions, travel and automation | Accepted (2026-10-31) |
| R-C — Renderer delivered | Outdoor hub, single-layer height variation and decoration work; production uses the replacement and obsolete renderer dependencies are retired | Planned |

### R01: Visual and motion proof

**Deliver:** A small isolated renderer probe using existing raylib/OpenGL integration:
generated representative room/corridor geometry, a monster billboard, one procedural
door, a wall decoration and an opening onto a sky. Match current framing and fog.
Script forward/back/strafe/turn sequences and compare native logical rendering with
mipmaps against 2× per-axis supersampling. Include oblique floors and close actors.
Use representative existing art for review, with generated self-owned test assets
for correctness. Record baseline startup, resident memory and frame timing.

**Boundary:** Diagnostic/prototype entry point only; no production gameplay switch,
new world schema, new monster, generalized asset framework or claim of full parity.
Prototype doors/sky are visual specimens, not completed production systems.

**Acceptance / validation:** Inspect settled views and motion/contact sheets at
logical and native sizes; check pixel scale, flicker, sprite edges, perspective,
camera corner clipping and door depth. CPU-check projection/transition endpoints;
exercise hidden GPU resource creation/cleanup when available. Record filtering
choice, quality/performance tradeoff and unresolved appearance differences. Measure
moving and stationary work separately; no unmeasured efficiency claim.

**Review recipe:** Show the same camera sequence for the existing renderer and each
candidate, with a short recommendation. User accepts the look and selected default
before the production architecture is expanded around it.

**Execution record:** **Accepted**, 2026-10-31; milestone R-A Accepted. User approved the
[R01 detailed design](renderer_probe_design.md) with “Implement the plan.” Standalone
manual/hidden probe, three sampling profiles, unchanged legacy comparison, source-art
billboard, slab door/sky/decoration specimens and deterministic motion gallery are
implemented. [Verification/handoff](r01_verification.md) records focused graphics
6/6 each in Debug/Release, headless 3/3 each, final CPU/GPU checks, CPU ASan/UBSan,
zero tracked warmed C++ allocations, shipped/generated captures and fresh-process
benchmarks. [Review gallery](screenshots/r01/index.html). Detailed manual coverage
and other GPUs remain unverified. No production renderer replacement or later-slice
work. The user prefers supersampling and requests retaining native filtered as an off
option; this supersedes the provisional native-filtered recommendation. The user
reports both modes look good without flickering. The user explicitly accepted R01 and R-A on 2026-10-31.

### R02: Production world rendering and assets

**Deliver:** Production mesh builder/chunk batches, exposed-face extraction,
off-centre camera, depth/fog material shader, reusable render targets and bounded
GPU ownership. Versioned material/sprite texture preparation replaces perspective
baking for the new path. Wire a development-selectable world view into Presenter
with unchanged UI, coordinate conversion and captures; legacy play remains default.
Define and test the active/staged scene lifecycle and resource budgets now.
Retain both approved quality paths with supersampling on by default and an off
option selecting native filtered; switching must not change simulation or picking.

**Boundary:** Flat production maps first. Prototype actors may be shown for
diagnostics, but full interaction is R03 and gameplay-complete integration is R04.
Do not present an incomplete new path as normal playable campaign support.

**Acceptance / validation:** Generated fixtures cover shared-face elimination,
material boundaries, enclosed/outside cells, seams, all headings, near/far clipping,
turn/strafe culling and capacity rejection. Check stationary invalidation and zero
engine-owned allocations during motion. Validate corrupt/missing asset errors,
incremental/no-op builds, resource cleanup, failed staged uploads and peak memory.
GPU integration checks must include state restoration before the native UI pass.

**Review recipe:** Walk scripted flat rooms/corridors, compare framing/materials
against R01, demonstrate no corner popping, and show a failed preparation leaving
the prior scene usable. Report draw calls/upload volume and frame timings.

**Execution record:** **Accepted and closed** (2026-10-31).
[Approved design](renderer_world_design.md), implemented after “Implement the plan.”
[Verification/handoff](r02_verification.md) and [selected evidence](screenshots/r02/README.md)
record graphics 62/62 and headless 43/43 in Debug/Release, final focused checks,
sanitizers, hidden GPU rollback/lifecycle and allocation/upload guards, capacity
stress, incremental multi-config fixtures and 81 repeat-identical captures. Normal
legacy gameplay remains default. R03/R04 work is not implemented. The user explicitly
accepted R02: “slice accepted, record it in the plan doc”. R03 is eligible for
planning but has not started; milestone R-B remains Planned.

### R03: Billboards, picking and visual automation

**Deliver:** All current actor and item clips/directions, continuous travel transforms,
death/prone/reaction states, shadows, attachments and existing close-range formation
framing. Implement the shared CPU visibility/depth/ID contract, ground projection,
actor/loot/fixture interaction, HUD anchors and status occlusion. Replace the visual
automation dependency on CPU color frames for this path. Preserve command validation
and stale-safe IDs; exclude interaction during movement as currently required.

**Boundary:** No new monsters or animation content. Full spell/projectile migration
and animated production doors follow in R04; tests isolate this slice's interactions.

**Acceptance / validation:** Self-owned sprite fixtures cover transparent holes,
partial wall/body occlusion, overlapping/dead actors, directional frame changes,
subpixel travel, near clipping, loot generations, ground picking, badges, stale
snapshots and restart/travel identity replacement. Compare CPU interaction coverage
with GPU evidence, including the selected filtering mode and alternative quality
settings. Live and windowless visual observation paths must return matching targets
for identical snapshots. Measure visibility cost and verify allocation invariants.
Run strict replay/protocol checks affected by visibility; record version changes.

**Review recipe:** Select a partially hidden actor, click through a transparent hole,
inspect corpse occlusion, pick up/drop an item and replay the same action sequence
through windowless visual mode. Show correctly clipped health/status indicators.

**Execution record:** **Accepted and closed** (2026-10-31). User approved implementation with
“Implement the plan.” [Design](renderer_visibility_design.md),
[verification/handoff](r03_verification.md) and [review gallery](screenshots/r03/index.html)
record shared CPU/GPU billboards, full-pixel visibility, HUD clipping, explicit
windowless backend and replay identities. Graphics 64/64 and headless 45/45 in
Debug/Release, final affected checks/sanitizers/hidden GPU lifecycle and parity,
zero tracked frame C++ allocations/uploads, 2,656-candidate stress and 403 identical
Debug/Release captures pass. Legacy defaults remain; full integration stays R04.
The user explicitly accepted R03: “slice accepted, record it in the plan doc”.
R04 is eligible for separate planning, not started; milestone R-B remains Planned.

### R04: Doors, effects and campaign integration

**Deliver:** Production procedural textured slab doors/gates, frames and hinge motion;
all existing attack/projectile/spell visuals, spectral weapons, ground effects and
target previews using world depth. Wire the complete alternative renderer through
New Game, arena, travel/recall, save/Continue and capture/replay tooling. Keep the
old path temporarily available for controlled comparison until R06.

**Door contract:** Fixture completion remains authoritative and permanent under the
current rules. Cosmetic motion must not permit selecting/attacking through a visibly
closed slab or move the camera through it. Define a presentation barrier consistent
with existing command/timing semantics in the detailed design, including queued
movement, AI and attacks. Restores show the settled door; no new closing mechanic.
Test and document any necessary command scheduling change instead of hiding it in
the draw code.

**Acceptance / validation:** Retain impact/death timing, pause behavior, transparent
effects ordering, wall/body occlusion, no effect-owned picking IDs, ground preview
eligibility and visual-overflow policy. Exercise locks, keys, guardian gates,
stationary door opening and movement immediately afterward. Stage new GPU scenes
transactionally on travel/Continue; inject preparation failure and preserve the
current session/save. Check representative accepted combat/spell/campaign traces,
CLI/client/local MCP integration and hidden captures using the replacement.

**Review recipe:** Play/replay a compact expedition segment: unlock a door, fight
with melee/ranged/ground/spectral effects, collect loot, descend, recall, save and
Continue. Compare meaningful gameplay results and visual timing with the baseline.
This milestone establishes playable parity, not campaign balance.

**Execution record:** **Accepted and closed** (2026-10-31); milestone **R-B Accepted**.
User approved the reviewed plan with “Implement the plan.” [Design](renderer_integration_design.md),
[verification/handoff](r04_verification.md) and [gallery](screenshots/r04/index.html).
Shared door scheduling/geometry, complete current effects, explicit world gameplay,
staged campaign/save/Continue and CLI/client/local MCP/replay captures are delivered.
Fresh graphics 65/65 and headless 46/46 Debug/Release suites, final affected/protocol
checks, sanitizers, hidden GPU parity/lifecycle/failure/allocation guards and asset
fixtures pass. The same 1,070 expedition requests match legacy admission and inspected
gameplay; 612 initial Debug/Release door/spell images match exactly. The baseline
spell review corrections restore weapon clearance, spell tint/fade and impact
visibility, with focused CPU/GPU/allocation/sanitizer checks and correctly timed
captures. Verification records actual reruns and limits. The user explicitly accepted
the delivered baseline: “I accept the slice, record it in the plan doc”. Richer spell
graphics remain deferred to later passes. Legacy stays default until R06. R05 is
eligible for separate planning, not started; no new monsters, balancing or PC07 work.

### R05: Heights, outdoor hub and decoration

**Deliver:** Single-surface elevation/ceiling/environment data and validated geometry,
bounded legal steps with shared movement/navigation/LOS integration, sky and outdoor
lighting profiles, wall-attached decals and simple static mesh props. Demonstrate
different ceiling heights, a raised traversable area and indoor/outdoor adjacency.
Apply the outdoor profile and a small decoration set to the existing authored hub
without adding shops or changing its role. Keep generated campaign floors flat by
default; richer procedural layout/content selection remains later campaign work.

**Boundary:** No stacked cells, ramps requiring continuous movement, free vertical
motion, vertical combat bonuses, general model import/editor project, weather or
day/night system. Reuse or procedurally generate a small prop set; broad art production
and additional monsters are outside this slice.

**Acceptance / validation:** CPU fixtures cover unequal neighboring floors/ceilings,
missing ceilings, exposed risers, legal/rejected steps, clearance, boundaries and
height-sensitive sight. Player and NPC navigation must agree; validate loot/areas/
shadows/door pivots at raised surfaces. Test decal bias/overlap, prop occlusion,
sky rotation/seams, indoor/outdoor transitions and fog termination without visible
world holes. Save/travel/Continue preserve environment data and invalid input fails
without state loss. Re-run affected headless observations, replay and allocations.

**Review recipe:** Walk from a covered passage into the outdoor hub, turn under the
sky, inspect wall art/props and climb a legal step. Show a rejected high ledge and
a height-sensitive occlusion case. Travel away and reload to show identical state.

**Execution record:** Not started; R04 acceptance satisfies its prerequisite. Its detailed design
must settle height quantization, traversal/LOS semantics and save compatibility
before implementation; split the slice if those cannot stay bounded.

### R06: Production cutover and retirement

**Deliver:** Make the new renderer the normal path; retire obsolete CPU color
composition, image transitions, perspective sprite/scene banks and exclusive build
dependencies/options after verifying their consumers have migrated. Retain shared
art preparation, CPU visibility, meaningful regression fixtures, reference evidence
and current automation. Update build/runtime documentation and system design to
describe the implemented architecture, not two permanent interchangeable renderers.

**Boundary:** No late appearance redesign or new rendering features. Necessary
fixes remain in scope; adding feature work requires an explicit follow-up slice.

**Acceptance / validation:** Clean graphics and graphics-disabled builds, relevant
Debug/Release correctness suites, focused sanitizers, CPU allocation guards, strict
replay, save/Continue, campaign travel, arena/menu/UI and GPU resource lifecycle.
Test startup/shutdown/restart and failure cleanup; missing GPU support is a clear
error for graphics, not a silent legacy fallback. Verify routine windowless visual
observations still work and image capture explicitly owns its hidden context.

Run separate representative and stress performance checks: startup/bake time,
CPU/GPU resident and peak staged memory, draw calls/uploads, stationary work,
moving/turning/combat p50/p95/p99 frame times and visibility cost. Record hardware,
build, resolution, warmup and workload; compare to R01 baseline and budgets agreed
in R02. A 60 Hz presentation target implies a 16.67 ms total frame budget on the
named reference machine, not a claim about every device. Do not report CPU timings
as GPU timings or gate routine correctness on noisy benchmark thresholds.

**Review recipe:** A retained end-to-end gallery and motion sequence spanning
indoor/outdoor scenes, doors, heights, creatures, effects and UI, plus the performance
comparison and known limitations. User accepts the complete renderer milestone.
The next work is planning additional monsters, not automatically resuming PC07.

**Execution record:** Not started; depends on R05 acceptance.

## Evidence and test policy

Keep CPU geometry/camera/visibility/command tests windowless with deterministic,
self-owned small fixtures. Production artwork may appear in review captures but
must not be a dependency of routine correctness tests. Keep large maps, full-capacity
stress and performance investigations opt-in and separate from the fast suite.

Hidden application-owned captures and in-process scripted input are the verification
route, subject to the project's desktop restriction. Never inject OS input, focus
another window or control the user's desktop. If GPU/display facilities are absent,
report those checks as unexecuted and leave interactive review to the user.

Define tolerances per image/coverage test; do not require identical floating-point
GPU output across vendors. Same-environment deterministic sequences are useful but
are not a cross-device guarantee. Inspect motion for shimmer/popping in addition to
endpoint images. Preserve diagnostic details outside player-facing UI.

Each handoff records implementation/design paths, asset/schema/visibility versions,
executed commands, results, captures, hardware limitations, material risks and the
exact next action. Do not carry forward historical passing counts as new evidence.

## Risks and decisions still requiring slice evidence

| Risk | Mitigation / owner |
| --- | --- |
| Crisp stills shimmer in motion | R01 moving comparisons; mip/alpha preparation and supersampling decision |
| Raised horizon/setback or near actors look different | R01 framing/corner review; R03 formation-fitting comparison |
| CPU visibility disagrees with GPU depth/alpha | R03 shared geometry/snapshots, coverage contract and measured parity gate |
| Picking reintroduces CPU cost or GPU stalls | R03 benchmark the ID-only pass; no unexamined per-frame readback |
| Doors look closed after logical opening | R04 explicit transition/command contract, tested with AI and immediate actions |
| Height fields quietly change gameplay | R05 shared traversal/LOS validation, bounded semantics and replay/save coverage |
| New assets or staged views exceed memory | R02 numerical budgets; peak two-scene accounting and preparation rejection |
| Renderer replacement becomes permanent dual maintenance | R04 parity gate, R06 mandatory consumer migration and old-path removal |
| A small visual demo hides missing effects/automation | Separate R03/R04 acceptance and end-to-end R06 review |

## Decision log

| Date | Decision | Basis |
| --- | --- | --- |
| 2026-10-31 | R04 and R-B Accepted and closed | Explicit user acceptance: “I accept the slice, record it in the plan doc”; baseline spell corrections included, richer effects deferred; R05 eligible for separate planning, not started |
| 2026-10-31 | R04 implemented; Ready for review | User approved the reviewed plan; complete alternative gameplay/tooling and evidence delivered; legacy default retained, acceptance pending |
| 2026-10-31 | R03 Accepted and closed | Explicit user acceptance: “slice accepted, record it in the plan doc”; R04 eligible for separate planning, not started; R-B remains Planned |
| 2026-10-31 | R03 implemented; Ready for review | Shared billboards/CPU visibility and explicit windowless backend; evidence retained; normal defaults unchanged and acceptance pending |
| 2026-10-31 | R03 detailed plan approved; implementation in progress | Explicit world visibility selector preserves legacy defaults; shared CPU/GPU transforms and mask-based visibility, no R04 work |
| 2026-10-31 | R02 Accepted and closed | Explicit user acceptance: “slice accepted, record it in the plan doc”; R03 eligible for planning, not started; R-B remains Planned |
| 2026-10-31 | R02 implemented and Ready for review | Approved design implemented; verification, production-asset captures and measurements retained; no gameplay cutover |
| 2026-10-31 | R01 and R-A Accepted; R02 detailed plan approved for implementation | Explicit user acceptance followed by “Implement the plan”; [design](renderer_world_design.md) |
| 2026-10-31 | Supersampling default; retain native filtered as the user-facing off option. Raw nearest remains diagnostic. | User prefers button 3 for distant detail and reports buttons 2 and 3 look good without flickering; requests keeping both. This is quality-policy feedback, not recorded whole-slice acceptance. |
| 2026-10-31 | R01 implemented and Ready for review; retain native filtered as provisional default pending appearance acceptance | Approved detailed plan: “Implement the plan.” Verification and review gallery recorded; no production cutover or R02 work |
| 2026-10-30 | Campaign paused after accepted P-B; rendering/presentation, then more monsters, precede campaign resumption | Explicit user direction |
| 2026-10-30 | Low-resolution GPU world with retained billboard artwork; stacked cells deferred | User: “okay I agree with your proposal and yes lets put stacked cells off for now” |
| 2026-10-30 | Propose six delivery slices and three milestones, with detailed design and acceptance per slice | User requested a roadmap analogous to the campaign/demo plans and invited judgment on count |

## Current next-agent handoff

R01/R-A and R02 are Accepted. R02's [design](renderer_world_design.md),
[verification](r02_verification.md) and [selected evidence](screenshots/r02/README.md)
record the delivered slice. User acceptance closes R02 on 2026-10-31 and supersedes
the pending-review status in its implementation handoff documents.
R03 is **Accepted and closed** (2026-10-31) under its
[approved design](renderer_visibility_design.md). The user explicitly accepted it:
“slice accepted, record it in the plan doc”. [Verification/handoff](r03_verification.md)
and [gallery](screenshots/r03/index.html) retain the implementation, evidence and
limitations unchanged; no additional interactive-review coverage is inferred.
R04 is **Accepted and closed** (2026-10-31); milestone **R-B Accepted** under its
[approved integration design](renderer_integration_design.md). The user explicitly
accepted the delivered baseline: “I accept the slice, record it in the plan doc”.
[Verification](r04_verification.md) and [gallery](screenshots/r04/index.html) retain
actual evidence and limitations, including the baseline spell corrections. Richer
spell graphics remain deferred. Acceptance adds no unperformed verification claims.
R05 is eligible for separate planning, remains unstarted and requires its own
reviewed plan. Do not add monsters or resume PC07.
This checkpoint supersedes earlier R04-unstarted and pending-review records,
including the status recorded in its implementation handoff documents.
Keep legacy gameplay working and the supersampling/native-filtered quality policy.
No commits or desktop input automation.
