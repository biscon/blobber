# Sanctuary city: design and delivery roadmap

Created: 2026-11-04. Last updated: 2026-11-05.
Status: **SC01.1 accepted. SC01.2 detailed planning is next.**

Replace the small Sanctuary courtyard with an authored, walkable town: older and
slightly overgrown but still cared for, with a fountain square, market scenery,
connected streets, sheltered greenery, and occasional two-storey houses.
The user requested the same staged planning, implementation and acceptance workflow
as the [renderer roadmap](renderer_plan.md) and [campaign roadmap](procedural_campaign_plan.md).
This roadmap does not authorize automatic implementation of every slice.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Agreed direction | Four slices, SC01–SC04; original town architecture inspired by the supplied references |
| Visual direction | [Fountain square](mockups/sanctuary-city-v1/01-fountain-square.png) and [residential lane](mockups/sanctuary-city-v1/02-residential-lane.png); user endorsed these as useful references, not a promise of pixel-identical rendering |
| Active slice | None; SC01.1 — Location-owned assets accepted 2026-11-05 |
| Awaiting review | None; SC01.1 [verification/handoff](sc011_verification.md) accepted |
| Next eligible slice | SC01.2 — Architecture foundation and finished street corner |
| Next action | Detailed planning for SC01.2 when requested |
| Save policy | Existing development campaign saves need no migration or compatibility layer |
| Rendering policy | Prepared low-poly geometry, material batching and basic frustum culling; no speculative advanced visibility system |
| Blockers | None |

Update this checkpoint, tracker, decision log and relevant handoff together.
Direction agreement, detailed-plan approval, implementation and delivered-slice
acceptance are different states. Only the user accepts a delivered slice.
This work does not advance PC08 or reopen accepted renderer/demo slices.

## Delivery workflow

1. Read [AGENTS.md](../AGENTS.md), [system design](system_design.md), this checkpoint,
   the prior slice's handoff and relevant code. Preserve existing working-tree changes.
2. Work on one slice at a time. Mark it **In progress — planning** and write its
   practical design under `docs/`, linked from the system overview before implementation.
   Specify responsibilities, interfaces, ownership, storage bounds, failure behavior,
   asset delivery and focused verification in proportion to the work.
3. Obtain review of the detailed plan before implementation. Existing explicit
   approval in the conversation suffices; do not re-request settled decisions or
   approval for routine reversible implementation choices.
4. Implement the approved slice, including affected persistence/publication fields,
   automation contracts, resource lifetimes and tests. Do not accumulate correctness
   work until SC04. Keep normal campaign play working while the city is developed.
5. Update design notes to match the result. Record checks, visual evidence, known
   limits and next-agent instructions in a durable verification/handoff document.
6. Mark **Ready for review** and stop. Review corrections belong to the same slice.
   After the user accepts the result, record acceptance and identify the next slice
   for detailed planning; do not automatically implement it.

Use **Planned**, **In progress — planning/implementation**, **Ready for review**,
**Accepted**, **Blocked**, and **Superseded/Retired** consistently. Blocked means a
named prerequisite, not merely unfinished work. If necessary, propose a suffixed
split such as SC01.1 with explicit scope and dependencies. Do not drop failed
acceptance criteria to claim completion. No commits or toolchain changes are implied.

Performance corrections may be handled as focused interludes at a slice handoff
before proceeding. Record the concrete problem, changed scope, checks and outcome;
preserve the roadmap checkpoint. There is no speculative optimization slice, nor
an obligation to benchmark every delivery. A material regression must be disclosed,
not silently deferred. Subsequent user instructions determine interlude priorities.

## Scope and appearance contract

- An authored safe town, initially around 24×24 cells with roughly 8–12 buildings.
  These are layout starting points, not fixed capacity requirements. Refine them
  from actual camera views and walking distances during SC01/SC02.
- Central fountain square, approximately two connected street loops, a narrower
  alley, planted courtyard, market edge, sanctuary/rest landmark and dungeon entrance.
  Main streets generally two or three cells wide, occasional one-cell alleys.
  Preserve convenient access to rest and dungeon travel; do not pad walking time.
- Mostly low houses with occasional two-storey exterior shells. Recognizable
  future inn, smithy and provisioner facades may reserve identities without
  implementing services. Interior routes and upstairs traversal are absent.
- Warm limestone and worn pale plaster, dark weathered roofs, thick rounded stone
  arches with real recess depth, timber doors with dull iron fittings, small inset
  windows and restrained vegetation. Age should read as wear in a lived-in refuge.
- Build simple modular Blender geometry and original ImageGen material/foliage art.
  Follow the [painted prop guide](prop_modelling_guide.md), including coherent
  texel density, baked painted recess/underside shading, padded atlases and reproducible
  source/export files. Use the actual renderer to judge art; Blender lighting can
  conceal weaknesses in painted materials.
- Preserve the current fixed 1920×1080 presentation, viewport/camera contract,
  discrete movement and interpolation, prepared asset pipeline, and native HUD.
  Do not restore the retired perspective-image renderer or introduce another renderer.
- House doors initially have static visual variants. Existing functioning dungeon
  doors retain their behavior. Do not expand swinging-door geometry merely to show
  a decorative arched house entrance.
- Market stalls are scenery. House interiors/submaps, shop transactions, economy,
  townspeople, combat/balance changes, stacked walkable floors, free movement,
  day/night cycles and a dynamic shadow system are outside this roadmap.
- Fountain includes a modest looping water effect in SC03; no fluid simulation,
  reflections or elaborate transparency pipeline. Water movement is presentation
  state and should respect the established pause/time policy.

Mockups and exact prompts are retained in [the reference folder](mockups/sanctuary-city-v1/README.md).
They guide materials and relationships, not literal topology, camera calibration or
implementation guarantees. The accepted SC01 street corner becomes the practical
in-engine quality reference for the rest of the town.

## Existing foundation and proposed system boundaries

These are roadmap-level design directions, not implemented interfaces. Each slice
must inspect current code before selecting its concrete records and capacities.

### Authored world and scenery

`src/world/environment_maps.cpp` currently builds a 9×9 courtyard. `Map` owns
immutable row-major terrain, surfaces and decorations; movement and sight derive
from authoritative world data. Retain this separation from visual meshes.

Introduce only the model catalog and placed-scenery data needed by the town:
stable asset references resolved at preparation, transforms, explicit physical
footprints and an explicit relationship to terrain presentation. Building shells
replace their underlying generated wall faces without removing collision or sight
blocking. Recessed doors must not expose duplicate backing walls or coplanar surfaces.
Select the exact face-replacement representation in SC01; avoid a generic entity framework.

Buildings remain opaque gameplay obstacles. Trees, stalls, bushes and fountain
pieces have intentional movement reservations rather than collision inferred from
every rendered triangle. Distinguish walkable overhangs/canopies from physical bases.
Define sight behavior for props explicitly. Validate connected walkable space,
nonoverlapping reservations and protected arrival/travel positions at preparation.

Town authoring belongs in cohesive world/content code, not campaign orchestration
or `main`. Centralize named entry, rest and dungeon anchors rather than scattering
coordinates among startup, fixtures and restore validation. Reserve useful doorway
identity/placement information for later work without implementing a dormant portal system.

### Prepared assets, geometry and visibility

Current `world_assets::Pack` has four furniture model slots; mesh validation limits
extent and count for the existing props. `DecorationKind` and scene preparation
also contain prop-specific mappings. SC01 extends these deliberately for buildings
without weakening validation or imposing city dependencies on the dungeon pack.
Keep an explicit city asset pack and reproducible Blender source, export script,
runtime mesh data, texture masters and prepared publication.

Assets are prepared before play. Map/scene owners borrow immutable shared assets;
view-owned contiguous geometry and fixed scratch are reused during frames. Retain
typed resource handles, RAII and staged preparation rollback. Define mesh, instance,
material, texture, draw-list and aggregate active-plus-staged budgets before coding.
Exhaustion rejects preparation with a clear error; never silently omit geometry.
Avoid frame-time container growth, decoding or resource creation.

Use the simple rendering baseline agreed with the user: submit prepared low-poly
geometry in material batches and perform basic frustum/distance culling. The engine
already uses 8×8 chunks with geometry-derived bounds and shares scene culling between
GPU drawing and CPU visibility work. Reuse this where adequate. A flat list of
building/object bounds is an option if it improves the actual placement design;
per-building culling is not mandatory if chunk batching is sufficient.
Bounds must include roofs, overhangs and foliage beyond anchor cells, throughout turns.

Do not add occlusion queries, octrees, portal traversal, streaming, automatic LOD
or a bespoke instancing system without a demonstrated need and a revised scoped
decision. Rendering some hidden opaque geometry is acceptable at this town scale.
Low polygon count alone does not establish laptop performance: draw calls, alpha
overdraw, texture memory, supersampling and CPU picking/visibility also matter.

Current far distance is 16 cells. SC02 should evaluate long streets and roof
silhouettes against current fog/clipping; compose within it where practical.
Any adjustment must account for CPU/GPU visibility and bounded draw storage together,
not merely increase a GPU camera constant. Preserve settled picking and movement-frame
work avoidance; verify the recent [movement work](movement_frame_work.md) is not undone.

### Trees and foliage

The agreed starting point is modeled low-poly trunks/branches with fixed crossed
or clustered alpha-cutout cards, each depicting a leafy clump or branch. Trees
should hold up when walked around. This is distinct from rotating an entire tree
billboard toward the camera. Keep card counts and overlap modest, prepare alpha/mips,
and inspect edges, depth, minification and turning behavior in the actual viewport.

SC03 compares one representative tree before producing the set. Opaque painted
canopy masses with cards at their edges are a fallback if denser foliage is needed
or card overlap is costly. Individual-leaf instancing is not the default: it reduces
submission overhead but retains vertex/pixel work and risks tiny-leaf shimmer.
Do not build both approaches into a general foliage framework without a concrete need.

### Campaign and persistence

`src/game/campaign.cpp` currently defines hub entry, sanctuary and stairs directly;
`src/game/campaign_checkpoint.cpp` validates exact courtyard dimensions/coordinates.
SC04 replaces those assumptions with the new authored town definition and anchors.
Keep travel, rest and resource rules unchanged, including separate rest after recall.
Preserve prepare-before-commit behavior and the current session/Party on failure.

The user explicitly waived compatibility with existing development campaign saves.
Version incompatible schemas/identities when required; reject unsupported saves
clearly without modifying their bytes. No migration, old-hub compatibility mode,
automatic deletion or unrelated character-library changes. New city saves must
round-trip correctly. If earlier slices change serialized world records/publication
formats, update their codecs, identities and owned tests in that slice; SC04 is the
campaign cutover, not permission to leave intermediate formats inconsistent.

## Slice tracker and milestones

| Slice | Deliverable | Dependency | Status |
| --- | --- | --- | --- |
| SC01 | Asset residency foundation and finished street corner | Agreed roadmap direction | In progress — implementation |
| SC01.1 | Location-owned assets and memory accounting | Approved detailed plan | Accepted 2026-11-05 |
| SC01.2 | Architecture foundation and finished street corner | SC01.1 Accepted | Planned |
| SC02 | Complete town architecture and walkable layout | SC01 Accepted | Planned |
| SC03 | Vegetation, market and fountain | SC02 Accepted | Planned |
| SC04 | Campaign hub replacement and delivery | SC03 Accepted | Planned |

| Milestone | Meaning | Status |
| --- | --- | --- |
| S-A | Architectural scale and art quality proven in-engine through SC01 | Planned |
| S-B | Complete dressed city accepted through SC03 | Planned |
| S-C | City is the working campaign hub through SC04 | Planned |

### SC01 — Asset residency and architecture foundation

SC01.1 implements the approved [location-owned asset design](sc011_design.md)
and was accepted by the user on 2026-11-05. SC01.2 retains the architectural
deliverable below and is now eligible for detailed planning. Supersampling remains on by default; 512 MiB–1 GiB is
a memory target, not a quality-reduction mandate. No automatic continuation.

### SC01.2 — Architecture foundation and street corner

**Purpose:** prove the actual appearance and model/world integration before mass art production.

Deliver a bounded development review scene with one low house, one two-storey
house, visible corners/rooflines, deeply recessed arched entrances and paving.
Use representative finished textures, not only greyboxes. Establish the small
model/placement extension, footprints, generated-face replacement, correct bounds,
prepared export pipeline and a reproducible review launch/capture path. Keep the
normal hub available until SC04; the review scene is development tooling.

Acceptance:
- Walk, strafe, back up and turn around the corner using the actual camera.
  House scale, door height, recess depth and visible upper storey are convincing.
- No duplicate terrain faces, visible mesh intersections, corner leaks or roof
  popping at the chosen review positions; gameplay cannot walk through buildings.
- Materials have coherent physical detail and read in the engine's painted mode.
  The user accepts this as the quality reference before expansion.
- Focused owned CPU/publication checks cover placement, reservations, invalid
  references/bounds and capacity failure. Available hidden GPU checks/captures
  cover geometry/depth, visibility agreement and resource lifetimes. Check affected
  frame allocation invariants and existing dungeon callers.

Expected records: `docs/sc01_design.md` and `docs/sc01_verification.md`, created
when that slice starts. A substantial foundation problem may warrant an explicit
split; do not pass off a crude proof as the agreed finished visual checkpoint.

### SC02 — Complete town architecture and layout

**Purpose:** turn the accepted corner into the entire navigable town.

Expand the architectural kit into the proposed building count and varied facades,
windows, roof silhouettes and static door styles. Author square, loops, alley,
courtyard and town boundary; place identifiable rest/dungeon landmarks and future
service buildings. Keep the fountain/market/planting areas reserved for SC03.
Use a readable top-down layout alongside representative first-person views.

Acceptance:
- All intended streets and destinations connect; footprints and narrow passages
  behave correctly. The layout has distinct landmarks and avoids excessive repetition.
- The full town can be explored in the development review scenario without
  campaign cutover. Useful entrances face accessible standing cells.
- Rooflines, distant streets and tall buildings behave under current camera,
  clipping and culling; any necessary visibility change is verified with its callers.
- Focused connectivity/placement tests and selected movement captures cover the
  full layout. Record actual asset/scene totals against declared budgets.

Expected records: `docs/sc02_design.md` and `docs/sc02_verification.md`.

### SC03 — Vegetation, market and fountain

**Purpose:** deliver the complete city art and its lived-in character.

Build and review one representative tree with the agreed foliage approach before
expanding the set. Add bushes, selective ivy, planting beds, timber market stalls
with muted cloth canopies, and a modest stone fountain with looping water.
Integrate these into the town rather than merely delivering isolated models.

Acceptance:
- The fountain square and residential street evoke the retained mockups while
  staying readable at the actual viewport resolution and during movement.
- Tree silhouettes work from multiple sides; foliage has appropriate depth and
  clean enough edges without objectionable shimmer or conspicuous card patterns.
- Physical bases block movement where intended; canopies and decoration do not
  obstruct planned walking routes or reveal unwanted wall intersections.
- Water animation respects pause/time behavior and reuses bounded resources.
- Focused foliage/depth/resource checks use owned fixtures; production art review
  covers overlapping foliage, close door views, stalls, fountain and turning.
  Record draw/asset totals and any observed performance issue for a scoped interlude.

Expected records: `docs/sc03_design.md` and `docs/sc03_verification.md`.

### SC04 — Campaign hub replacement and delivery

**Purpose:** ship the accepted town as Sanctuary with complete campaign behavior.

Switch normal campaign startup to the city. Centralize authoritative hub anchors;
integrate new-game arrival, recall, return from the dungeon, sanctuary rest, floor
map discovery, save/load and validation. Retire obsolete courtyard-specific
production assumptions while retaining useful self-owned regression fixtures.
Ensure prepared assets are available in the configuration the user will run.

Acceptance:
- New Game arrives at the intended town position/facing. The player can explore,
  enter the dungeon, return by the existing travel routes and rest at the sanctuary.
- Recall costs, unlocked-floor return rules, Party identity/resources and inactive
  dungeon state remain correct. Arrival does not implicitly perform a rest.
- New saves preserve town pose, exploration and the ongoing campaign through
  Continue; unsupported older identities are rejected clearly and left untouched.
- Failed scene preparation retains the current playable session, Party and view.
- Focused campaign travel/recovery/map/persistence checks and one meaningful final
  integration review pass. Use extended persistence coverage only where justified
  by actual changed behavior, not an automatic full configuration matrix.
- No ordinary game flow depends on development review switches; document the
  final user review recipe and remaining limitations before requesting acceptance.

Expected records: `docs/sc04_design.md` and `docs/sc04_verification.md`.

## Verification, resource limits and handoffs

Follow [the verification workflow](verification_workflow.md). Tests own small
deterministic fixtures; routine correctness does not depend on game artwork.
Keep CPU correctness/allocation checks windowless and actual GPU integration
separate. Inspect representative production stills and movement captures where
available. Passing geometry checks alone does not establish visual acceptance.

Never control desktop input, focus or other windows. Use command-line/headless
checks and permitted hidden application-owned captures with in-process input.
Interactive review remains user-owned. Report unavailable graphics checks as
unexecuted, not passing.

Run only one intensive job at a time, including Blender, baking, builds and tests.
Use low CPU/I/O priority, at most two build workers and one test process, bounded
internal threads, and check memory/scratch space before expensive work. No routine
benchmarks, capacity stress or repeated Debug/Release/GPU matrices. For a reported
performance issue, state a concrete question and bounded representative workload
before profiling; record hardware/quality and avoid claiming untested laptop results.

Each handoff records:
- Approved scope, implementation entry points, asset regeneration and review steps.
- Exact checks/outcomes, skipped checks, visual evidence and actual resource totals.
- Schema/publication changes, failure behavior, deviations and remaining limitations.
- User acceptance or explicitly pending review, and the next eligible action.

### Reusable prompts

> Read AGENTS.md, docs/system_design.md and docs/sanctuary_city_plan.md. Inspect
> the current checkpoint, previous slice handoff and relevant code. Plan the next
> eligible slice only for review; preserve working-tree changes. Do not implement.

> Implement the approved Sanctuary city slice. Follow its detailed plan, resource
> limits and desktop restrictions. Update designs, roadmap checkpoint and durable
> verification/handoff. Run focused checks and stop at Ready for review.

## Decision log

2026-11-05: Approved SC01.1 residency prerequisite / SC01.2 architecture split
and implementation. Dedicated city review flag selected for SC01.2. Supersampling
stays on; prioritize fidelity over strict adherence to the memory target.

| Date | Decision |
| --- | --- |
| 2026-11-05 | User accepted SC01.1 location-owned assets and memory accounting. SC01.2 is eligible for detailed planning; no further implementation authorized by this acceptance. |
| 2026-11-04 | User requested a walkable city inspired by supplied Wizardry 7 architecture, with original art and house interiors deferred. |
| 2026-11-04 | User selected older/slightly overgrown character and occasional two-storey houses; two ImageGen mockups were retained under docs/mockups/sanctuary-city-v1 and endorsed as useful visual references. |
| 2026-11-04 | User agreed to four slices and proposed scope boundaries, and requested this roadmap before slice planning/implementation. |
| 2026-11-04 | User explicitly waived preservation/compatibility of existing development saves; no migration work is required. |
| 2026-11-04 | User favored simple low-poly rendering with basic frustum culling, and iterative performance corrections at slice handoffs rather than speculative advanced rendering infrastructure. |
| 2026-11-04 | User agreed to modeled trunks with clustered foliage cards as the starting tree approach, with a possible opaque-canopy/card hybrid; no individual-leaf instancing by default. |

## Current next-agent handoff

SC01.1 was accepted by the user on 2026-11-05. Read the [design](sc011_design.md) and
[verification/handoff](sc011_verification.md). Supersampling and texture fidelity
are unchanged. Current supersampled production evidence is about 178 MiB in the
hub, 371 MiB on floor one and 373 MiB during their transition, including UI/fonts.
Unused biome/creature payloads stay on disk; source-only assets release on travel.
The 512 MiB–1 GiB target and 384/448 MiB reference lines are advisory. Hardware
with that memory capacity was not available for verification.

No city architecture or new hub has been implemented. The next eligible action
is SC01.2 detailed planning when requested. Preserve the dedicated launch-flag decision and
retained ImageGen references. Do not advance PC08, SC02 or campaign cutover.
