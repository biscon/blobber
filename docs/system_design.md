# System design

Renderer roadmap **Accepted and closed** (2026-10-31): R01–R06 and milestones
R-A/R-B/R-C are complete. The user accepted R06 with “slice accepted, close up the
new renderer plan”. [Sole renderer cutover and retirement](renderer_cutover_design.md)
documents the delivered ownership, interfaces and deletion inventory;
[verification](r06_verification.md) retains executed checks and limitations unchanged.
[Historical design and acceptance records](system_history.md) preserve prior evidence
without describing current defaults. Additional monsters require separate planning;
the campaign roadmap remains paused after P-B, with no automatic PC07 work.

## Boundaries and data flow

- **World:** compact immutable row-major solid-cell storage and immutable NPC
  loading/placement data; grid directions and coordinates are value types. Loading and connectivity validation happen before
  play. A map is limited to 1024×1024 cells and rejects malformed input explicitly.
- **Session/combat:** an immovable owner borrows map/catalog and owns actors,
  movement, AI, combat RNG and elapsed time. Standalone sessions own Party;
  campaign sessions borrow the single Party owned by CampaignRun. Actors and encounter membership
  are bounded at 256; initiative groups use catalog monster identities. Contact
  clears queued movement; commands validate turn tokens, life, equipment, range,
  formation and attacker availability before RNG. Acceptance returns synchronously;
  a fixed pending record retains the saved result until strike/arrival. Each update
  returns a borrowed timestamped batch (1,036 entries including effect notices),
  draining effect deadlines and existing attacks before movement/AI and then taking
  at most one new automatic step. Consume the batch before any subsequent session
  simulation operation, not only the next update.
  Acting turns, facing and results wait for pending impacts and presentation tails.
  Focus/map/details/history/menu pauses freeze session deadlines and gameplay time.
  Gameplay cooldowns retain the hybrid real-time/round-boundary policy.
- **Effects:** session-owned 64-instance fixed storage, up to three typed recipients
  per instance; world/serial IDs reject stale slot reuse. Party casters own at most
  one concentration each. Checked apply/remove operations and save/lifecycle events
  serve later spell commands; current review is headless. Full storage rejects
  before costs/RNG/replacement. Expiry precedes repeat saves and equal-time impacts;
  existing accepted attacks keep their saved rolls. Incapacitation masks suppress
  NPC facing as well as pathfinding; living held actors retain occupancy.
- **Party/rules:** immutable catalog values feed pure, allocation-free character
  derivation. A four-character party owns starting builds/gear, derived caches,
  resources, health/death-save state, and a formation permutation. Pure health
  transitions feed checked mutations; health/build snapshots remain const. Typed character/item indices never
  recycle; formation swaps preserve identities and resources. JSON/loading stays
  outside game code. Schema 2 adds party content; strict schema 1 remains readable
  as rules-only content, but startup requires a preset.
- **Actors:** a fixed-capacity runtime owner initializes monster HP and transforms
  from placements and the catalog. Checked movement, facing, attack cues, damage and removal
  atomically update records, occupancy and revisions. Typed IDs include a unique
  world identity; slots never recycle. Dead records remain readable, removed IDs
  fail lookup, and only living actors block movement.
- **AI/navigation:** fixed state for at most 256 actors; immutable placements supply
  guard/patrol/wander settings. Separate seeded randomness preserves combat rolls.
  One exploration tick per 100 ms uses a shared map-sized BFS workspace (six bytes
  per cell), at most 4096 search/cleanup visits and eight completed requests.
  Requests wait on budget exhaustion; checked ActorWorld steps revalidate occupancy.
  AI time freezes outside exploration; visual walk cues use paused session time.
- **Movement:** owns player pose, one pending command, transition state, and a
  revision counter. Exposes a const state snapshot; consumers cannot mutate its
  invariants. Key repetition is a separate graphics-independent input component.
- **Assets and preparation:** the offline world baker extracts registered source
  frames, prepares reusable material/sprite mip chains and CPU alpha masks, and
  publishes bounded versioned world metadata/textures. Runtime validates publication
  fingerprints and definitions before creating immutable shared CPU assets and GPU
  texture leases. Native portrait/creator atlases have independent retained bakers.
- **World rendering:** bounded static chunk/material meshes plus per-view CPU
  visibility and effects consume the same camera, actor animation, loot, doors and
  surface snapshots as GPU drawing. Picking has full per-pixel depth/IDs, with
  transparent holes, corpse occlusion and native status/health clipping. World
  effects never write picking IDs. A changed view invalidates rendering; idle views
  reuse their target without frame uploads. See the accepted R02–R05 designs.
- **Platform:** raylib window/input adaptation and GPU ownership. Typed texture
  and render-target handles refer to fixed-capacity, generation-checked slots.
  Borrowed raw raylib values stay inside short platform calls. RAII unloads GPU
  resources before the window closes, including exception paths.
- **Commands/UI:** a graphics-independent controller validates current body
  visibility and routes production requests through GameSession; it separately owns targeting, read-only hover, inspection, exclusive overlays, bounded enemy/ally
  targets, a 32×192-byte log, eight timed damage/MISS splashes and a fixed HUD snapshot. Confirmation must refer to
  the turn/outcome already presented. Focus/edge handling, Aid selection and
  aspect-fit hit testing are CPU-testable.
- **Presentation:** Presenter uses the shared application registry for its viewport
  two native 1920×1080
  targets: a point-filtered dungeon base and a transparent premultiplied UI layer
  fitted with bilinear filtering. The 500×282 GPU dungeon retains the accepted framing and sampling policy.
  Layout and input retain logical 640×360 coordinates; HudPainter converts geometry
  to native pixels, and fonts draw at their native atlas sizes. Artwork, geometry
  and text share one ordered UI pass; modal fills cover all underlying UI content.
  Six startup-loaded RAII font atlases provide Plex Sans regular 21/24 px, bold
  27 px, Inconsolata 24 px and DejaVu Serif Bold 27 px and Plex Sans Bold 48 px controls. Font ownership is separate
  from the generic texture registry and ends before window shutdown.
  Startup-loaded native art atlases retain prepared portrait/icon derivatives at
  display sizes with padded edges and bilinear sampling. Portraits center-crop and
  icons aspect-fit. Captures use one temporary diagnostic target. See [native HUD design](native_hud_design.md).
- **Application/tools:** normal launches enter menus, campaign or arena through one
  renderer. GameView borrows WorldRenderer by reference and leases active/staged
  views. Preparation succeeds before session travel commits. CLI/client/local MCP
  visual observations use the same CPU depth/ID pipeline without a graphics context.
  Replay capture owns a hidden context and exports application-owned targets.
  Standalone demos, preview/showcase scenarios and focused capture runners are gone.

Dependencies flow from application/presentation toward platform, CPU rendering,
movement, and world values. World/movement/input timing have no raylib dependency.
There is no ECS, generic event bus, virtual renderer hierarchy, or service locator.

## Ownership, storage, and limits

Maps own contiguous cell bytes. CampaignRun retains four stable locations and the
travelling Party; the application owns shared prepared rendering. One active
session borrows its location/Party, with value snapshots for inactive locations.
Travel stages replacement simulation/view owners at an explicit preparation
boundary and commits before the next simulation tick. Only active/staged owners
have navigation/frame scratch; there is no live streaming or inactive simulation.
Standalone authored/arena sessions retain their existing map/catalog lifetime
contract. See the PC04 design for snapshot validation and clock ownership.

Runtime actors retain up to 256 stable records and one byte of live occupancy
per map cell. The immutable placement mask is loading data only. The map and
catalog outlive the noncopyable/nonmovable actor owner. Runtime operations return
fixed result values without allocation. Lethal impact unblocks immediately; animation
plays a 1200 ms goblin death/corpse clip, overlaying white for its first 100 ms,
then hides body/shadow. Legacy missing death art retains the frozen 100 ms flash.

WorldRenderer retains immutable shared textures/alpha data and at most active and
staged views. Each view owns bounded static scene geometry, visibility scratch,
effects and GPU mesh/target leases. World asset budgets are 256 MiB mip textures
and 64 MiB alpha masks; scene limits are 64 MiB CPU/40 MiB GPU, view scratch 8 MiB,
and the aggregate world subsystem budget 352 MiB. Preparation rejects capacity or
allocation failures without replacing the playable view. These are preparation
boundaries, not frame-time growth policies.

Party stores four character identities/resources and owns one fixed 208-stack
inventory: four 48-cell bags plus four equipment positions per character. Startup
content retains up to 16 owned entries; live queries use inventory handles and
52-position sparse loadouts, never startup indices. Item handles carry party
inventory identity, pool index and generation. Moves preserve identity; complete
merges/consumption invalidate it, and exhausted generations retire slots.
Transactions stage fixed value storage and all four derived-stat records, committing
only after capacity, training, hand compatibility and weight validation. Weight is
integer hundredths of pounds with int64 totals. Catalog bounds remain 16 races,
16 classes and 64 items; all preparation bounds reject explicitly.

Party construction may copy strings; frame callers borrow const snapshots. The
catalog outlives sessions/parties. Campaign travel retains actual Party/inventory
identity and rebases portable deadlines without recreating starting builds. Arena
restart reconstructs borrowers while reusing prepared assets. These preparation
boundaries may allocate; ordinary engine-owned frames may not. Character IDs are
party-local; startup owned-item indices are character-local; catalog IDs remain
catalog-local. See [inventory design](inventory_design.md) for schema 5, live item
commands, safety, UI storage and tests.

R02 expands the application-owned GPU registry to 512 standalone textures, eight
render targets, eight static meshes and eight shaders. Presenter borrows the registry;
world scene/resource leases release resources at explicit preparation boundaries.
The new path additionally enforces texture/mesh/scene byte budgets in the
[R02 design](renderer_world_design.md). Normal gameplay always uses the world renderer. Supersampling defaults on;
`--supersampling off` selects native filtered quality. Graphical initialization
failure is an error, never a renderer fallback.
Handles include a typed slot index and generation. Invalid/stale lookup returns
no resource; checked platform operations reject it. Exhaustion is an explicit
startup error. Releasing a slot invalidates its generation; exhausted generation
counters retire that slot rather than allowing old handles to become valid again.
Registry identity prevents handles from accidentally crossing registries. Slots
contain resource values, not owning raw pointers. No extra handles are introduced
for ordinary grid coordinates or transient references.

The allocation guarantee covers engine-owned CPU gameplay processing. The test
intercepts ordinary and aligned C++ `new`/`new[]`; it does not intercept C
`malloc` calls. Engine-owned frame code uses no C allocation APIs. Loading,
cache preparation, exceptional error reporting, captures, and driver/raylib
internals are outside it. A targeted allocation guard is more useful here than
a Vulkan-style benchmark or stress-testing framework.

## Interfaces and preservation

GameView/WorldRenderer stage the destination geometry, visibility, effects and first
image before committing travel/Continue. The old view dies before its session/map;
failed preparation leaves the current session, image/IDs, Party and save intact.
Presenter accepts only world target/revision publications. Engine-owned frame work
uses prepared fixed storage; export and diagnostic readbacks are explicit boundaries.

Campaign schema/simulation remain 2/2; world publication format 1, appearance 4,
visibility world-visibility-v3 and terrain-rules-v1 remain. Strict trace replay
checks identities rather than translating retired renderer recordings. Protocol v1
remains; renderer/visibility selectors and renderer_fixture scenario are removed.

Prefer an installed compatible raylib 6.2 package, otherwise use the current
hash-verified FetchContent fallback. nlohmann/json 3.12.0 likewise prefers the
installed package with a hash-pinned download fallback; only asset I/O uses JSON.
Debug/Release presets stay CLion-compatible.

## Validation strategy

Follow [verification workflow and resource limits](verification_workflow.md).
Use generated, owned small fixtures and focused one-configuration checks during
iteration. CPU simulation, world geometry/visibility/effects, command/HUD, allocation,
transport/replay and save smoke checks run without a window. GPU integration covers
actual resource ownership, full-pixel CPU/GPU parity, hidden world/native UI drawing,
staged rollback and cleanup separately. No desktop input automation. Full persistence
and capacity/performance investigations are explicit; no repeated routine matrices.
See [R06 verification](r06_verification.md) for executed checks and limitations.

## Source map and design index

| Location | Responsibility and design |
| --- | --- |
| `src/world/`, `src/generation/` | Immutable quantized maps/surfaces, fixtures, bounded procedural generation; [environment](renderer_environment_design.md), [generation](procedural_generation_design.md) |
| `src/game/`, `src/rules/` | Session, Party, actors, combat/effects, movement, navigation; [actors](actors_design.md), [combat](melee_combat_design.md), [effects](effects_design.md), [inventory](inventory_design.md) |
| `src/render/world_*`, `src/assets/world_assets.*` | Geometry, camera, effects, depth/IDs and reusable publication; [world](renderer_world_design.md), [visibility](renderer_visibility_design.md), [integration](renderer_integration_design.md) |
| `src/platform/`, `src/ui/` | GPU resources, sole renderer, view leases, input and native presentation; [native HUD](native_hud_design.md), [cutover](renderer_cutover_design.md) |
| `src/app/` | Startup/menu/creator/arena/game orchestration and session transitions; [menu](menu_launch_design.md), [arena](arena_design.md) |
| `src/persistence/` | Checked content/character/scenario/campaign disk adapters; [campaign save](procedural_campaign_save_design.md) |
| `src/automation/`, `tools/` | Typed sessions, strict replay, CLI/client/local MCP, owned captures and opt-in experiments; [protocol](headless_protocol.md), [MCP](mcp_design.md) |
| `tests/` | Self-owned CPU/GPU regression fixtures and resource/allocation guards; no retired renderer harnesses |

The procedural campaign roadmap remains paused after accepted P-B. Additional
monsters/debug tools need separate planning; PC07 does not resume automatically.
