# System design

Implemented, **Ready for review** (2026-10-27): slice 10
[spells and class identity](spells_design.md), with [verification and handoff](slice10_verification.md).
Fourteen curated spells use checked commands, fixed party payloads and the existing
impact/effect clocks. Party owns mana/charges; Combat owns readiness and Sneak Attack;
Effects owns concentration and Guiding deadlines. The paused spellbook resumes into
live targeting, with multi-target/area confirmation and bounded cosmetic feedback.
Schema 4 retains versions 1–3 compatibility. User acceptance is pending; slice 11
remains Planned. The approved [casting UI revision](spellcasting_ui_design.md)
is implemented without changing spell mechanics: independent pending casts, compact picker,
casting dock, direct recipient feedback and a shared authoritative area query. See
[UI verification and captures](slice10_ui_verification.md).

Implemented, **accepted and closed** (2026-10-27): slice 09
[effects and concentration](effects_design.md), with
[verification and handoff](slice09_verification.md). GameSession owns 64 fixed
instances for Bless, Bane, Shield of Faith, Mage Armor and Hold Person. Effects
share the hybrid gameplay clock, with real-time deadlines interleaved with attack
impacts and full-round retries during turns. Concentration changes at damage impact;
paralysis blocks actions/facing and paired protection while retaining occupancy.
Read-only effective stats leave base builds unchanged. Slice 09 supplied consequences;
player casting and mana arrive in slice 10 above. The user accepted slice 09 on 2026-10-27.

Implemented, **accepted and closed** (2026-10-26): slice 8.1
[offline image preparation and memory](image_preparation_design.md), with
[verification and handoff](slice81_verification.md). NPC and dungeon surface banks
are baked by an incremental build target and loaded without source image decoding
or resizing at startup. Shared NPC profile C (alpha-aware area/half-size derivatives)
applies to every current and future appearance. Walls/floors/ceilings retain point
sampling. Masters remain intact. Compact NPC banks and separate 512 MiB resident /
tracked-loading limits supersede the historical 160 MiB startup pixel cap below.

Implemented, **accepted and closed** (2026-10-26): slice 8.2
[distinct goblin archer artwork and loadouts](goblin_archer_design.md), with
[verification and handoff](slice82_verification.md). Appearance `goblin.archer`
provides 48 hooded bow/quiver poses across five four-direction clips and binds by
name to the existing bow-only rules variant. The two hybrid-demo archers use it;
regular `goblin` is melee-only. Both immutable prepared appearances share profile C,
existing actor/animator interfaces and zero-allocation frame processing. Actual
combined runtime banks occupy 181,300,462 bytes. Original melee artwork and historical
bow masters remain intact. The user accepted the completed slice on 2026-10-26.

Implemented, **accepted and closed** (2026-10-27): slice 8.3
[projectiles and impact-time consequences](projectile_impact_design.md), with
[verification and handoff](slice83_verification.md). Combat owns 260 pending
attacks; validation, costs and rolls occur at acceptance, while HP/life/occupancy,
reactions and result feedback change at impact. Timestamped batches drain existing
deadlines before movement/AI and phase handoff. Separate bounded cosmetics render
prepared arrows, a melee slash and impact glints. Hurt suspends unreleased NPC
windups; released arrows survive shooter death. Current prepared resident accounting
is 182,314,670 bytes. This supersedes the earlier immediate-damage and UI-delayed-
feedback descriptions below. The user accepted 8.3, including the front-cell
spacing correction. Slice 09 is now accepted and closed (see above).

Implemented review correction (2026-10-27): the front-band edge fit previously compressed
center-cell pairs, causing visible overlap. The corrected mapping preserves ordinary projection between
the two center-slot pivots (lateral ±0.25 cells), tapering compression only outside
that interval toward the existing diagonal edge limit. It retains the continuous depth/
strafe blend and shared body/shadow/picking/projectile projection. Center pair
separation in both rows/all headings, diagonal visibility, travel and allocations
pass regression checks; hidden captures confirm the shipped-art result. See the
[correction verification](slice83_verification.md#review-correction-center-cell-pair-spacing).

Implemented and accepted (2026-10-26): [three-cell melee front](hybrid_encounter_design.md#three-cell-melee-front), shared diagonal contact/archer spacing, and front-band AI routing. Approved edge presentation shifts clipped diagonal NPCs inward with matching shadows/picking; camera/FOV and world occupancy remain unchanged.

Implemented and accepted (2026-10-26): [front-approach routing correction](hybrid_encounter_design.md#front-approach-routing-correction). Pursuit from ahead prefers the center and diagonal front cells, with bounded navigation fallback when that band is full or unreachable. Living melee targets beyond reach report out of range. Initiative strip unchanged.

Implemented and accepted (2026-10-26): [single-weapon AI correction](hybrid_encounter_design.md#single-weapon-review-correction). Melee-only NPCs close and attack; ranged-only NPCs hold shooting positions and create distance at contact. Mixed-weapon behavior is preserved. Slice 8.2 replaces the two hybrid archers' historical placement MonsterId overrides with a distinct appearance bound by name; generic overrides remain supported.

Implemented and accepted (2026-10-26): slice 08
[hybrid encounters](hybrid_encounter_design.md). GameSession owns separate
presentation and gameplay clocks; shared NPC AI/perception supports six-cell bows,
local alerts, NPC surprise and boundary-enrolled reinforcements. Combat enters
initiative directly, retains facing while the three-cell front group lives, and automatically
turns to the next adjacent group without spending an action. Offscreen enemies
cannot attack; fixed front-row protection replaces directional exposure. No
maneuver/retreat command or party surprise penalty remains. Catalog schema 3 adds typed traits and
a dedicated ranged slot while retaining schemas 1/2. Prepared bow clips keep the
fixed 1920×1080 presentation and 160 MiB pixel cap. See
[verification and handoff](slice08_verification.md). Slice 09 is now accepted and closed;
slice 08 and milestone B are closed. Graphical subslices 8.1–8.3 are also
accepted and closed; see the slice 09 checkpoint above.

Slice 07 [moving goblin AI](goblin_ai_design.md) remains accepted, including
600 ms [visual travel](goblin_ai_design.md#review-correction-interpolated-walking)
and attack-presentation holds. Slice 08 renames the shared controller to NpcAiController
and supersedes the former frontal-only contact and frozen-roaming phase policy.
[Slice 07 verification](slice07_verification.md) is its historical acceptance record.
The earlier slice descriptions below record their accepted scope; hybrid encounter
rules above supersede their stationary-combat restrictions.

Implemented and accepted by the user (2026-10-26): slice 06 [goblin hurt/death reactions](goblin_reactions_design.md).
Four-direction prepared clips provide 300 ms hurt and 1200 ms death/corpse playback,
with a 100 ms white overlay. See [verification and review](slice06_verification.md).
Simulation releases occupancy when lethal damage is committed, now at impact;
art never resolves damage or death. The historical UI-only delay in the
[impact timing design](goblin_reactions_design.md#attack-impact-feedback) is
superseded by slice 8.3's authoritative timeline and direct impact feedback.
Implemented review correction (2026-10-26): same-cell melee advancement may use
either vacant front slot, preferring straight ahead. Turn eligibility is separate
from current target exposure; see [melee design](melee_combat_design.md#front-slot-review-correction-2026-10-26).

Implemented and accepted by the user (2026-10-25):
[dungeon-first UI revision](dungeon_ui_design.md) for slice 05.
Combat and presentation are accepted; slice 05 and milestone A are closed. See
[UI verification and captures](dungeon_ui_verification.md).

Implemented and accepted by the user (2026-10-25), slice 05:
[first melee combat](melee_combat_design.md).
An immovable gameplay session owns party health, actors, movement, seeded RNG and
stationary frontal encounters. Initiative, row protection on both sides, melee,
Defend, Aid, death saves, victory and defeat now execute authoritative commands.
See [slice 05 verification and review](slice05_verification.md). Interactive desktop
review remains unverified; do not automate the user's mouse, keyboard or desktop.

Implemented and accepted by the user (2026-10-25): slice 04
[party HUD and command design](party_hud_design.md). Four portraits, command
previews, visible-body targeting and a bounded log read the existing party/actor
state. Slice 05 adds combat execution; the isolated demo retains preview-only semantics with the revised UI.
Explicit UI demo
scenarios keep sample mana/status values separate from authoritative party state.
See [slice 04 verification](slice04_verification.md). The typography correction uses
size-specific font atlases and native 1920×1080 text over the enlarged art,
with rounded text positions. The HUD design records the live-animation correction:
interactive demo clocks advance; capture clocks remain explicit, and automatic
idle phase offsets prevent default-placed actors from moving in lockstep. The UI
demo prepares its own idle-only placement copy, shared by interactive and
capture scenarios, so stationary actors do not inherit showcase walk/attack loops.
The dock keeps four wrapped log lines readable; expanded history pauses play.
Both use bounded word wrapping and scrolling that preserves the reading position
when new messages arrive.

Implemented and accepted by the user (2026-10-25):
slice 03, [mutable monster world](actors_design.md).
Runtime actors now supply live collision/rendering state, health and lifecycle.
Prepared appearances and asset formats remain unchanged; scripted diagnostics
exercise mutation and the 100 ms white damage/death flash; slice 06 adds reaction clips.

Gameplay extension: [SRD 5.1 demo design and self-tracking roadmap](dnd5e_demo_design.md).
It defines sixteen individually reviewed slices toward a four-character goblin
combat demo. Slice 01's [rules foundation](rules_design.md) is implemented and
accepted by the user (2026-10-25): explicitly seeded dice, pure attack/save/damage rules,
and an immutable catalog loaded from bounded JSON before image preparation.
The application owns the catalog; GameSession owns the live party and actors.
Rules and health transitions are graphics-independent, JSON stays in a separate I/O library,
and typed catalog indices are stable for one non-recycled catalog lifetime.
The [source ledger](srd_content_ledger.md) records adopted SRD data and adaptations.
The roadmap owns delivery
status, user decisions, verification records, and handoff notes; this overview
continues to describe implemented systems and must be updated as slices land.

Implemented and accepted by the user (2026-10-25):
[characters and preset party](characters_design.md) is slice 02. Independent race/class/item definitions and an editable preset feed
pure stat derivation and a bounded four-character owner. It initializes HP, Hit
Dice, XP, and relative formation before world/image preparation. The party exposes
const state, checked health operations and formation swaps locked during combat.
Inventory transactions and mana remain deferred. See [slice 02 verification](slice02_verification.md).

NPC extension: [directional sprites and slow animation](npc_design.md) is implemented.
It adds immutable level NPC placement data, JSON assets, and prepared alpha animation
frames. Runtime actors initialize from those placements; a separate animation
player selects idle, walk-in-place, or frontal attack frames and transient hit feedback.
Stationary combat is integrated. Review corrections add paired-slot protection,
same-cell enemy advancement, one-shot existing attack clips and bounded damage/MISS
feedback; see the melee design. Slice 07 adds autonomous cross-cell movement; slice 06 supplies four-direction
hurt/death clips.
The [NPC design](npc_design.md#rendering) also covers prepared, floor-projected
contact shadows, blended beneath bodies and clipped against wall depth.

Status: implemented. The foundation refactor preserves the established renderer,
projection, movement, and art. See [verification](verification.md) for executed
checks and limits.

## Boundaries and data flow

- **World:** compact immutable row-major solid-cell storage and immutable NPC
  loading/placement data; grid directions and coordinates are value types. Loading and connectivity validation happen before
  play. A map is limited to 1024×1024 cells and rejects malformed input explicitly.
- **Session/combat:** an immovable owner borrows the map/catalog and owns party,
  actors, movement, AI, combat RNG and elapsed time. Actors and encounter membership
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
- **Assets and preparation:** the offline bake tool reads masters/materials and
  produces versioned scene/NPC banks. Scene banks retain eight lateral phases and
  sixteen floor/ceiling variants per phase. NPC banks use cropped contiguous
  clip/direction/frame/depth and travel rasters with unchanged logical pivots,
  plus independent shadows. Runtime reads bounded JSON metadata and validates a
  publication manifest and both banks, streaming into their final owners. It
  neither decodes NPC/material sources nor prepares rasters. Standalone preparation
  remains available to generated-fixture tests. See the image preparation design
  for format, invalidation, budget and appearance-integration contracts.
- **CPU rendering:** immutable prepared data plus caller-owned fixed-size frames.
  Scene composition and image transitions write into supplied buffers. A frame
  pipeline owns source/destination/display buffers and only updates changed views.
  Its independent 260-slot projectile owner copies visual snapshots; cosmetic
  exhaustion replaces the oldest visual without affecting pending damage. Compile-
  time arrow/slash/glint rasters total 1,012,160 bytes. Shared NPC projection and
  optional directional attachments keep bodies and arrows aligned. Wall/body depth
  clips effects, which never write picking IDs. Extra picking scratch is reused
  for effects during transitions.
- **Platform:** raylib window/input adaptation and GPU ownership. Typed texture
  and render-target handles refer to fixed-capacity, generation-checked slots.
  Borrowed raw raylib values stay inside short platform calls. RAII unloads GPU
  resources before the window closes, including exception paths.
- **Commands/UI:** a graphics-independent controller validates current body
  visibility and routes production requests through GameSession; a separate const-owner
  mode retains preview-only diagnostics. It separately owns targeting, read-only hover, inspection, exclusive overlays, bounded enemy/ally
  targets, a 32×192-byte log, eight timed damage/MISS splashes and a fixed HUD snapshot. Confirmation must refer to
  the turn/outcome already presented. Focus/edge handling, Aid selection and
  aspect-fit hit testing are CPU-testable.
- **Presentation:** the HUD draws from movement, map and command snapshots; the presenter
  owns handles for the viewport texture and three targets. It retains the 640×360
  art grid → 1920×1080 texture → aspect-fit framebuffer path. HUD geometry stays
  on the art grid; text is drawn at native resolution onto a separate transparent
  1920×1080 target. Final aspect-fit sampling uses point filtering for art and
  bilinear filtering only for premultiplied text, preserving strokes when reduced.
  Captures temporarily composite both layers into an owned diagnostic target.
  Five startup-loaded RAII font atlases provide Plex Sans regular 21/24 px, bold
  27 px, Inconsolata 24 px for logs, and DejaVu Serif Bold 27 px for headings; every draw/measurement uses its atlas
  base size. Font ownership is separate from the generic texture registry and
  ends before window shutdown, including partial loading failures. A startup-loaded
  256×192 UI atlas holds four portraits, a goblin thumbnail and six action icons.
  The 500×282 dungeon sits left of the portrait rail, above a 15%-height dock.
  Overlay text clears only its own native-target rectangle to prevent underlying
  glyphs showing through. Tooltips use matching atlas measurements in both layers.
- **Application/tools:** option parsing, loading, scheduling, and lifecycle only.
  Captures use a hidden GLFW window, application-owned targets and in-process UiInput scripts, with no OS input injection or foreign-window selection. They are an explicit diagnostic mode with startup-prepared cases; output paths, filesystem work and image export are not normal gameplay frame work.

Dependencies flow from application/presentation toward platform, CPU rendering,
movement, and world values. World/movement/input timing have no raylib dependency.
There is no ECS, generic event bus, virtual renderer hierarchy, or service locator.

## Ownership, storage, and limits

Maps own contiguous cell bytes. The application owns the map and renderer for
the session. Movement and UI borrow them only during calls. A scene change would
prepare a replacement outside the frame loop; live streaming is not implemented.

Prepared sprites refer to ranges in contiguous column and color arrays, replacing
per-column pixel vectors. These are immutable offsets with cache-local lifetime,
not recyclable object IDs. Preparation may grow containers; rendering cannot.
The projection and eight phases bound the cache. Each phase permits at most 1024
sprites, 512,000 columns, and 16 million colors; exceeding a limit fails loading.
No eviction or hidden runtime growth occurs.

Runtime actors retain up to 256 stable records and one byte of live occupancy
per map cell. The immutable placement mask is loading data only. The map and
catalog outlive the noncopyable/nonmovable actor owner. Runtime operations return
fixed result values without allocation. Lethal impact unblocks immediately; animation
plays a 1200 ms goblin death/corpse clip, overlaying white for its first 100 ms,
then hides body/shadow. Legacy missing death art retains the frozen 100 ms flash.

Each 500×282 RGBA frame allocates once, is move-only, and cannot resize. The live
pipeline owns three frames (about 1.61 MiB), reusing them across idle, movement,
collision, NPC alpha composition, and transitions. Composition copies pixels but
never allocates output storage. Input buffers at most one fresh movement command. UI confirmation is edge-triggered
and never queued. UI formatting uses stack buffers. Each frame pipeline also owns
two fixed 500×282 uint16 picking buffers (~551 KiB combined; one is transition effect scratch), written alongside prepared body
pixels. Living target membership, visible-body centers and top edges are scanned after composition; transitions expose
no picking. Transparent holes/walls/shadows do not select, and dead bodies occlude
without becoming targets. Revisions invalidate picking with the image.

Party storage is four records with up to 16 owned starting item entries each,
not the future inventory grid. Definitions allow 16 races, 16 classes, and 64
items; exceeding a bound rejects preparation. Equipment borrows owned entries,
weight uses integer hundredths of pounds and int64 totals, and derived stats use
fixed arrays. Party construction may copy strings; steady-state callers borrow
its const snapshot. The application keeps the catalog alive and unmoved until
after session/party destruction. Restart reconstructs all session borrowers while
reusing prepared assets; this explicit preparation boundary may allocate. Character IDs are party-local, owned-item IDs are
character-local, and catalog IDs remain catalog-local.

The GPU registry holds up to eight standalone textures and four render targets.
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

The composition API writes to `Frame&`; callers must supply distinct buffers for
image transitions. The frame pipeline consumes a read-only movement snapshot,
tracks movement, actor-world identity/revision and NPC animation revisions, and publishes a const display frame plus a changed flag so idle
frames do not trigger texture uploads. Prepared cache configuration and content
remain immutable after construction. A pipeline belongs to one map, scene cache,
actor world, animation player, and controller lifetime: construct a new pipeline when replacing those systems,
because revisions are local to that controller. Moved-from frames may only be
destroyed or assigned another frame before use.

Keep controls, collision, queued-command behavior, the 0.4-tile viewpoint setback,
strafe parallax, and existing image transition effects. Capture filenames remain
stable for before/after comparisons.

Implemented and accepted by the user (2026-10-26): narrower dungeon framing.
horizontal FOV reduced from 90° to 84° and horizon raised from 36% to 34% of
viewport height. `ViewProjection` remains the shared compile-time configuration
for prepared walls/floors, NPCs, shadows and picking; no new API, runtime setting,
storage ownership or frame-time work is introduced. The larger projected NPC
images exceed the previous 128 MiB combined source/prepared pixel cap with shipped
art; that revision raised the startup budget to 160 MiB. Slice 8.1 supersedes
that historical budget with separate 512 MiB limits and offline source loading. Retain the 0.4-tile setback
to expose current-cell floor for future dropped items. At 500×282 this leaves
32 central floor rows (96 presentation pixels) facing an adjacent wall. Rebuild
prepared images offline (the original startup path was superseded by slice 8.1). Validate all four adjacent-wall headings
with generated materials, preserve side-opening/occlusion/strafe tests, run
Debug/Release correctness and allocation suites, and compare deterministic hidden
captures of walls, passages, rooms and goblins. Item interaction remains deferred.
Both 19-suite builds, allocation checks and the hidden GPU lifecycle check pass;
278 captures per projection completed. See the [framing verification and retained
comparisons](verification.md#84-framing-experiment-2026-10-26).

Prefer an installed compatible raylib 6.2 package, otherwise use the current
hash-verified FetchContent fallback. nlohmann/json 3.12.0 likewise prefers the
installed package with a hash-pinned download fallback; only asset I/O uses JSON.
Debug/Release presets stay CLion-compatible.

## Validation strategy

- Independent core tests: movement, malformed/bounded maps, input repeat, focus
  reset, state transitions, and aspect-fit behavior.
- Renderer tests: generated CPU materials and a dedicated test-owned regression
  map; preserve the independent geometric oracle, side-passage/floor framing,
  strafe reversal, and transition endpoint checks.
- Resource tests: typed handles, stale IDs, slot reuse, capacity, registry lifetime,
  and CPU material loading failures using isolated test-owned files.
- NPC tests: bounded JSON loading, generated sprites, 2x2 occupancy and collision,
  four view sectors, scale/grounding, transparent overlap, viewport clipping,
  and wall occlusion against independent ray/block intersections.
- Party/rules tests: deterministic dice, strict versioned content, independent
  race/class builds, armor/weapon/weight calculations, initialized resources,
  formation identity, and rejected invalid input.
- Actor tests: generated catalogs/maps, capacity and binding failures, stale/foreign
  handles, transactional occupancy, queued movement, exactly-once death/removal,
  stationary view refresh, hit-flash/reaction boundaries and all movement transitions.
  Reaction tests cover interruptions, corpse holds, hidden expiry, wider-canvas scale,
  reused-subslot ordering, pauses/Continue/restart, and budget exhaustion. Routine fixtures own their data;
  shipped content and startup validation are checked separately.
- Command/UI tests: preview validation without costs, live formation, foreign and
  stale identities, 256-target paging, bounded logs, development-only statuses,
  focus/held input, letterbox/pillarbox/high-DPI inverse mapping, alpha/depth-aware
  picking, transition suspension, and stationary invalidation. Direct actions, non-mutating hover, portrait Aid/details,
  exclusive modal input, keyboard feedback, initiative preview versus actual execution,
  and wrapped-history read-position preservation have dedicated HUD tests.
- Combat/health tests: pure health boundaries, death saves and stabilization;
  initiative/group ordering, both row collapses in four directions, authoritative
  requests, current visibility, Aid/Defend, pauses, queued contact, complete wins/losses,
  restart identity and input on newly displayed turns/results. Fixtures own their data.
- Effect tests: generated fixtures for the five supported consumers; atomic
  replacement, stacking, invalid/stale references, full capacity, expiry/retry
  boundaries, concentration at impact, natural death saves, paralysis and formation,
  frozen flight outcomes, pause/round/victory transitions and deterministic traces.
- Allocation regression: enable an allocation counter after preparation, exercise
  first and subsequent moves, all transitions, blocked/queued input, and held/advancing NPC
  frames, actor mutations/flash expiry/removal, rules/party queries and formation swaps, HUD refresh/commands/log rollover/input/picking, first combat entry and subsequent
  turns/Aid/healing/outcomes, and require zero C++ allocations in the normal CPU frame path.
- Desktop integration: check actual GPU handle creation/release and presentation,
  produce the same deterministic captures, and compare them with the baseline.
  Capture/export allocations are intentionally excluded from the frame guarantee.
- Build and run CTest in Debug and Release. Record executed checks and limitations
  in [verification notes](verification.md).

Historical techniques and visual choices are documented in [research](research.md).

## Source map and extension points

| Location | Responsibility |
| --- | --- |
| `src/world/` | Grid values, immutable map and NPC placements/occupancy |
| `src/rules/` | Deterministic dice, pure attack/save/damage and character derivation, immutable typed content catalog |
| `src/game/` | Session/encounter ownership, runtime actors/occupancy, movement, animation state, bounded party/health/formation |
| `src/input/` | Platform-independent key repeat timing |
| `src/assets/` | CPU image ownership, materials, NPC JSON/images, and separate rules JSON loading |
| `src/render/` | Projection, prepared scene data, composition, transitions, frame reuse |
| `src/core/` | Fixed-capacity typed resource slots |
| `src/platform/` | Window lifetime, keyboard adapter, GPU registry and presentation |
| `src/ui/` | Portrait/action HUD, shared layout/input routing, targeting and bounded-log presentation |
| `src/app/` | Startup/options, frame scheduling, diagnostic capture cases |
| `tools/` | Offline texture normalization |
| `tests/` | Core, renderer, resource, NPC, allocation, and explicit desktop checks |

NPC appearance indices remain stable for a prepared level lifetime. Runtime actor
slots never recycle; removal leaves a terminal record. Runtime spawning is deferred. Other future interactive objects should have their own
owner and lifetime model before introducing handles for them. They should not be
added to the GPU resource pool.
Future overlays belong in the UI; new movement rules belong in the controller or
world queries; material loading remains outside composition. Preparing a new
level or material set is an explicit loading operation. These boundaries do not
promise live streaming, asynchronous loading, or arbitrary wall heights.

Implemented extension: [slow NPC sprite animation](npc_design.md#sprite-animation)
adds bounded clip timing and prepared animation frames, preserving immutable
placements and the current image-composition/presentation pipeline. The application
owns a graphics-independent animation player with fixed-capacity snapshots. Source
images are released after preparation; timing metadata survives independently.
Authored visual loops remain isolated diagnostics; production gameplay
uses idle placements plus timestamped walk, attack and damage cues while AI and
combat independently resolve movement and turns. Controlled actor movement and
health/lifecycle mutation are described in the actor design; prepared caches stay
immutable and are reused at each runtime position.
