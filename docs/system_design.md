# System design

Implemented, **accepted and closed** (2026-10-29): [slice 12.4 creator and reusable selection](character_creator_design.md).
Fixed drafts and shared choice panels integrate creation, checked library saving
and atomic Long Rest preparation. A bounded 16 MiB detail-art atlas reuses existing
masters. [Verification and handoff](slice124_verification.md): Debug/Release 37/37,
focused ASan/UBSan 4/4 with leak detection, zero tracked frame allocations, hidden
GPU checks and 187 captures. The user accepted slice 12.4 on 2026-10-29 and retained
the current 27-point buy, 8–15 base scores and fixed racial bonuses. Slice 13 is
next eligible for separate planning and remains unstarted.

Implemented, **accepted and closed** (2026-10-29): [slice 12.3 menu and launch](menu_launch_design.md).
Normal startup loads catalog/library/UI only. Bounded library browsing, read-only
review, confirmed deletion and a four-character picker precede transactional dungeon
preparation. Runs own independent starting builds; defeat replays those builds and
seed. Confirmed Main Menu teardown returns to the menu. [Verification/handoff](slice123_verification.md)
records Debug/Release 35/35, focused sanitizers, allocation/GPU checks and hidden
captures. Review correction adds direct card-to-slot dragging, swaps, drop feedback and
focused search carets; focused Debug/Release, sanitizer, allocation and hidden
capture checks pass. The user accepted slice 12.3, including both review corrections, on 2026-10-29.
Slice 12.4 is accepted and closed above.

Implemented, **accepted and closed** (2026-10-29): [slice 12.2 character builds, library
and portraits](character_builds_design.md). Fixed creation drafts/previews and curated
gear reuse derivation; schema 8 authors explicit per-character class/domain/racial
spell grants. A versioned 128-character starting-build library uses checked atomic
POSIX transactions. Independent party/session construction adds three launch camp
kits. Twenty stable portraits use offline padded QOI atlases (40 MiB), with no
frame decode/resize. [Verification and handoff](slice122_verification.md): Debug and
Release 34/34, focused sanitizers 5/5, allocation/GPU checks, 225 portrait/build
captures and 482 spell captures. The user accepted slice 12.2 on 2026-10-29; 12.3 is accepted and closed above.

Implemented, **accepted and closed** (2026-10-29): portrait follow-up to the
[12.1 visual review](spell_visual_review_design.md#portrait-follow-up). Timed status
and concentration tooltips show turns in combat and seconds/minutes in real time;
a framed gold concentration emblem replaces the blue portrait block. Focused
Debug/Release 5/5, allocation and GPU checks pass; 482 captures include the follow-up.
The full-suite/sanitizer results below record the preceding visual correction baseline.

Implemented, **accepted and closed** (2026-10-29): [slice 12.1 expanded spells](expanded_spells_design.md)
and its [visual review corrections](spell_visual_review_design.md). Spell-level
and Abilities tabs preserve selection per character/tab. Bounded status summaries
supply portrait icons and duration tooltips; enemy badges reuse spell art. Prepared
Grease floor banks and required four-direction static prone clips preserve offline
image preparation and frame allocation guarantees. Binary bundles now use format 2.
The ten spell additions retain checked casts, bounded conditions/ground areas,
temporary HP and turn-relative short effects. [Local SRD](references/README.md)
is the source for future planning. [Verification and handoff](slice121_verification.md):
Debug/Release 31/31, focused sanitizers 6/6, zero tracked frame allocations,
capacity baking, hidden GPU checks and 470 captures pass. User acceptance on 2026-10-29 includes all visual and portrait
follow-ups. Slice 12.2 is now accepted and closed, as recorded above.

Design accepted (2026-10-28), **implemented across slices 12.1–12.4**:
[main menu and character-creation flow](character_creation_flow_proposal.md), with
[sixteen approved imagegen mockups](mockups/character-creator-v1/index.html).
The [12.1–12.4 roadmap](dnd5e_demo_design.md#character-creation-interlude-accepted-flow-and-shared-contracts)
adds expanded level-one spells; pure build rules, per-character spell grants,
versioned starting-build storage and twenty portraits; menu/library/party launch;
and the creator plus shared preparation/advancement presentation. The user approved
expanded spell selection and Color Spray in place of illustrative Ray of Sickness.
Library blueprints and live party state have independent ownership; menu/session
lifecycle and bounded draft/UI storage preserve existing rendering/allocation
contracts. Detailed subsystem designs are written in each later planning pass.
The data foundation is implemented in 12.2; menu/library/party launch is implemented
in accepted slice 12.3. The creator is implemented and accepted in slice 12.4.
Slices 12.1 and 12.2 are accepted and closed.
The 12.4 prerequisite for slice 13 is satisfied; slice 13 remains unstarted. Earlier checkpoint history remains below.

Implemented, **accepted and closed** (2026-10-27): slice 12 follow-up adds Inventory
Drop into the current cell, automatic Take All closure, and a small generated Rest
icon beside Settings while restoring full log widths. Session validates floor
capacity before staged Party removal; 256 generation-checked reusable player-drop
slots complement the unchanged 2,112 death/authored reservations. Equipment-derived
stats/effects reconcile after a drop. Fixed storage, per-pixel picking identities
and modal input rules remain intact. See [design](loot_recovery_design.md#inventory-drop-and-top-bar-rest-follow-up)
and [current verification](slice12_verification.md): Debug/Release 29/29, focused
sanitizers 3/3, allocation/stress/hidden GPU checks and 108 hidden captures pass.
The user accepted slice 12, including all review corrections, on 2026-10-27.
This was the slice-12 checkpoint; the accepted 12.x interlude above now precedes slice 13.

Implemented, **accepted and closed** (2026-10-27): slice 12
[loot and recovery](loot_recovery_design.md), including the approved loot-bag review
correction; [verification and handoff](slice12_verification.md). GameSession owns
bounded corpse-independent loot and checked rest transactions; Party stages whole-
stack acquisition and supply consumption. One prepared bag per nonempty cell opens
a paused grid with contained/aligned details, shared Inventory slot painting, live
portrait recipient selection, per-stack pickup and Take All. Leftovers retain the
bag. One projection fixes approach scaling. The Rest icon in the top bar
is dim/inert when neither rest has a legal benefit.
Schema 6 adds authored drops, weighted currency and camp kits. Floor imagery is
baked offline and shares depth-aware picking/transitions; normal frames allocate no
engine-owned C++ storage. Camp commits recovery once and plays a cosmetic bonfire.
Debug/Release 29/29, focused sanitizers, capacity stress, allocation guard, hidden
GPU checks and 93 native/reduced/letterboxed captures passed at that checkpoint.
The user accepted the completed slice and its follow-up corrections on 2026-10-27.
Automated verification limits remain recorded in the handoff.

Implemented, **accepted and closed**: approved [tabbed inventory and character sheet](character_sheet_design.md),
replacing the portrait inspector while preserving the current inventory layout.
The shared paused overlay uses fixed snapshots of live stats, equipment, resources,
features and effects; XP remains read-only. Both entry routes and tab directions
preserve character identity. [Verification and captures](character_sheet_verification.md)
record focused Debug/Release, allocation and hidden GPU checks. The user accepted slice 11 and this extension on 2026-10-27.

Implemented, **accepted and closed**: [slice 11 review correction](inventory_design.md#review-correction-simpler-inventory-controls-2026-10-27): contextual actions, drag-based organization, equipment placement, centered controls, right-aligned quantities, M/I shortcuts and a backpack dock icon. Availability uses bounded authoritative previews shared by buttons and shortcuts; no new gameplay rules. See the [updated handoff](slice11_verification.md#inventory-review-correction-2026-10-27).

Implemented, **accepted and closed** (2026-10-27): slice 11 [inventory and equipment](inventory_design.md), with [verification and handoff](slice11_verification.md).
Party owns bounded live stacks and separate gear, with atomic staged transactions,
weight/stat/effect reconciliation and strict schema-5 content. Full-screen paused
browsing supports drag, split, transfer and equipment controls. Potions and finite
party arrows share authoritative combat costs. Native item art preserves prepared
dungeon rendering. Debug/Release suites, focused sanitizers, allocation guards and
hidden GPU/captures pass. The user accepted slice 11 on 2026-10-27. Slice 12's implemented result is tracked above.

Implemented, **accepted and closed** (2026-10-27): [Class Features picker correction](spellcasting_ui_design.md#class-features-presentation).
The shared picker distinguishes spells, activated features and read-only automatic
features. Class casting capability controls headings/resource visibility and slot-4
art; gameplay commands, costs and schemas remain unchanged. UI storage stays bounded,
with native-resolution artwork and captured fighter/rogue verification. The full
Debug suite passes (26/26), with focused follow-up tests and hidden GPU checks.

Implemented post-acceptance [diagonal-contact animation fix](hybrid_encounter_design.md#diagonal-contact-animation-correction-2026-10-27):
Combat already faces NPCs toward the party at initiative
entry and attack acceptance. Sprite direction now uses that same cell-center geometry,
including interpolated travel/strafe offsets, rather than the subcell placement and
camera setback used to project the body. Bodies and projectile attachments share
the direction query; projection, occupancy, clocks and prepared banks stay unchanged.
This is constant work per NPC without new storage or allocations. All 26 Debug suites
pass, including 48 approach/render combinations, side/back fallback, travel and
allocation checks. Hidden GPU checks and shipped-art captures also pass. Slice 10's
acceptance remains recorded below; this correction does not start slice 11.

Implemented correction, accepted by the user on 2026-10-27: [native-resolution HUD and impact alerts](native_hud_design.md), with [verification](native_hud_verification.md).

Implemented, **accepted and closed** (2026-10-27): slice 10
[spells and class identity](spells_design.md), with [verification and handoff](slice10_verification.md).
Fourteen curated spells use checked commands, fixed party payloads and the existing
impact/effect clocks. Party owns mana/charges; Combat owns readiness and Sneak Attack;
Effects owns concentration and Guiding deadlines. The paused spellbook resumes into
live targeting, with multi-target/area confirmation and bounded cosmetic feedback.
Schema 4 retains versions 1–3 compatibility. The user accepted slice 10, including
all UI and impact-alert corrections. Slice 11 is tracked separately at the top of this overview. The approved [casting UI revision](spellcasting_ui_design.md)
is implemented without changing spell mechanics: independent pending casts, compact picker,
casting dock, direct recipient feedback and a shared authoritative area query. See
[UI verification and captures](slice10_ui_verification.md). The completed review correction
stabilizes badges with projection-based anchors and adjusts HUD spacing; see the
[review polish design](spellcasting_ui_design.md#review-polish-badge-stability-and-spacing-2026-10-27).

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
- **Presentation:** Presenter owns the viewport texture and two native 1920×1080
  targets: a point-filtered dungeon base and a transparent premultiplied UI layer
  fitted with bilinear filtering. The 500×282 prepared dungeon remains unchanged.
  Layout and input retain logical 640×360 coordinates; HudPainter converts geometry
  to native pixels, and fonts draw at their native atlas sizes. Artwork, geometry
  and text share one ordered UI pass; modal fills cover all underlying UI content.
  Five startup-loaded RAII font atlases provide Plex Sans regular 21/24 px, bold
  27 px, Inconsolata 24 px and DejaVu Serif Bold 27 px. Font ownership is separate
  from the generic texture registry and ends before window shutdown.
  A 2048×1024 startup atlas holds 28 original-art derivatives at native display
  sizes, with padded edges and bilinear sampling. Portraits center-crop and icons
  aspect-fit. No low-resolution UI image is enlarged threefold. Captures use one
  temporary diagnostic target. See [native HUD design](native_hud_design.md).
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
catalog outlives sessions/parties. Restart reconstructs session borrowers while
reusing prepared assets; that preparation boundary may allocate. Character IDs are
party-local; startup owned-item indices are character-local; catalog IDs remain
catalog-local. See [inventory design](inventory_design.md) for schema 5, live item
commands, safety, UI storage and tests.

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
| `src/rules/` | Deterministic rules, character derivation, bounded creation drafts/previews, per-grant spell queries and immutable typed catalog |
| `src/persistence/` | Versioned starting-build codec, checked UUID/revision library transactions; independent of future run saves |
| `src/game/` | Session/encounter ownership, runtime actors/occupancy, movement, animation state, bounded party/health/formation |
| `src/input/` | Platform-independent key repeat timing |
| `src/assets/` | CPU image ownership, materials, NPC JSON/images, and separate rules JSON loading |
| `src/render/` | Projection, prepared scene data, composition, transitions, frame reuse |
| `src/core/` | Fixed-capacity typed resource slots |
| `src/platform/` | Window lifetime, keyboard adapter, GPU registry and presentation |
| `src/ui/` | Portrait/action HUD, shared layout/input routing, targeting and bounded-log presentation |
| `src/app/` | Startup/options, frame scheduling, diagnostic capture cases |
| `tools/` | Offline asset/portrait preparation and headless build/library diagnostic |
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
