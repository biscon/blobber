# Procedural campaign MVP: design and delivery roadmap

Created / last updated: 2026-10-30.
Status: **Roadmap accepted; PC01–PC06 Accepted and closed; milestones P-A/P-B Accepted (2026-10-30).**
PC07–PC09 plans remain unapproved; do not advance automatically.

**Paused after P-B (2026-10-30), at the user's request.** Work now turns to
rendering and presentation, followed by additional monsters, before returning
to the procedural campaign. The user has accepted the low-resolution 3D renderer
direction with stacked cells deferred; its [six-slice roadmap](renderer_plan.md)
is proposed for review, with no renderer slice implemented. This pause supersedes
earlier next-action instructions to plan PC07.

This is the successor delivery roadmap to the [closed static-demo roadmap](dnd5e_demo_design.md).
The [MVP specification](procedural_campaign_mvp.md) supplies the experience and
requirements; this document adds implementation order, proposed system boundaries,
review checkpoints and handoffs based on the current code. It does not authorize
implementation of every slice. Existing accepted gameplay remains authoritative
unless an explicit change below is approved.

Long-term direction: a procedural first-person crawler beneath an authored town,
with shops and periodic returns, culminating in a bottom-floor boss. The MVP is
one small fixed hub, three persistent generated floors and a final guardian.
Shops and a wider economy are deliberately later work, as clarified by the user.
The demo's accepted slices stay closed. Its save/resume requirement moves here,
after playable generation and persistent travel; its polish and static-level
integration slices are retired without claiming they were implemented as slices.

### Replacement of the static New Game flow

The user explicitly authorizes the procedural campaign to **replace** the existing
static-map New Game mode. There is no requirement to keep that mode playable,
retain a legacy menu option, or build a compatibility layer for its launch path.
Remove obsolete static-demo launch code, exclusive content/configuration and
mode-specific tests as the replacement lands; retain shared gameplay systems,
useful self-owned regression fixtures and the separate arena/investigation tools.
Preserving accepted history does not require preserving an obsolete product mode.

PC01 may replace the static launch with its generated-floor prototype; temporary
coexistence is allowed only if useful during delivery, not an acceptance requirement.
By PC04, normal New Game launches the campaign in the authored hub, and obsolete
static-mode wiring is removed. PC09 verifies the shipped flow has no leftover
legacy option or dependency. Authored map support still serves the fixed hub,
arena and test fixtures; it does not imply retaining the old game mode.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Latest accepted foundation | Demo slices through 13.3 and milestones A/B/C; campaign PC01–PC06 and milestones P-A/P-B accepted (2026-10-30) |
| Active slice | None; PC06 Accepted and closed |
| Awaiting user review | None |
| Blockers | None; later slices require their own plans and approvals |
| Next eligible slice | PC07 — Depth bands and finite recovery encounters; detailed planning only |
| Next action | Campaign paused after P-B: review the renderer roadmap and plan R01 when requested, then additional monsters after rendering; resume PC07 planning only when the user returns to the campaign |
| Accepted direction | Replace the static New Game mode and remaining static-demo work with this MVP; paid recall, floor-entry return, optional walking fallback, no respawns, finite optional encounters for recovery/XP |
| Accepted roadmap | Scope, sequence and defaults accepted; PC01–PC06 Accepted and closed; milestones P-A/P-B Accepted; PC07–PC09 plans unapproved |

Update this checkpoint, tracker and affected execution record together. Roadmap
acceptance is not slice implementation or evidence of delivered gameplay acceptance.

## How agents must use this document

1. Read [AGENTS.md](../AGENTS.md), [system design](system_design.md), the MVP spec,
   this checkpoint, and the preceding slice's handoff. Inspect actual code before
   choosing a design. Preserve existing working-tree changes.
2. Work on one slice at a time. The tracker dependency is the immediate prerequisite;
   earlier unfinished prerequisites still matter. Do not automatically advance.
3. Mark the selected slice **In progress — planning** and write a practical detailed
   implementation plan, with material gameplay choices called out. Link a dedicated
   design under `docs/` from the system overview before changing architecture.
4. Obtain review of that plan before implementation. Existing explicit approval in
   the conversation suffices; do not ask again for work already authorized.
5. Implement the approved scope. Extend commands, observations and replay with the
   feature where relevant. Avoid postponing automation integrity until PC08.
6. Run appropriate focused checks; record exact commands, outcomes and limitations.
   Update system design, tracker, checkpoint and a durable verification/handoff.
7. Mark **Ready for review** and stop. Only the user can mark **Accepted**. Review
   corrections stay in that slice. After acceptance, plan the next in a clean context.

Use suffixes such as PC04.1 if a slice needs a separately approved split; preserve
IDs and history. Do not silently defer failed acceptance criteria. No commits,
installed agent configuration changes or extra infrastructure are implied.

| Status | Meaning |
| --- | --- |
| Planned | No slice implementation; detailed planning has not started |
| In progress | Planning, implementation or review corrections underway; name the phase |
| Blocked | A named prerequisite or decision prevents progress; record how to unblock |
| Ready for review | Implementation and available required checks complete; user acceptance pending |
| Accepted | User explicitly accepted the result and recorded limitations |
| Superseded / Retired | Deliberate scope change, never shorthand for successful implementation |

Unavailable graphics checks are unexecuted, not passes. Never control OS input,
window focus or desktops. Use headless checks and, where supported, hidden
application-owned captures with in-process scripts. Interactive review is the user's.
All balance experiments, including smoke matrices and full expeditions, are opt-in,
outside default builds/tests/CI and automatic unrelated-change checks. Tooling uses
remain open-ended wherever relevant to project work; examples are not restrictions.

### Reusable prompts

> Read AGENTS.md, docs/system_design.md, docs/procedural_campaign_mvp.md and
> docs/procedural_campaign_plan.md. Inspect the current checkpoint and previous
> handoff, then the relevant code. Plan the next eligible slice only for review.
> Preserve all working-tree changes. Do not implement or run balance experiments.

> Implement the approved campaign slice. Follow tracking and desktop restrictions,
> preserve existing changes, update designs and handoff, and run appropriate checks.
> Balance experiments require explicit opt-in. Stop at Ready for review.

### Execution-record template

```markdown
**Stage / last updated:** Planning, implementing, reviewing or accepted; YYYY-MM-DD.
**Approved plan:** Link or durable summary, including user approval and decisions.
**Delivered:** Observable behavior and relevant code/design entry points.
**Verification:** Exact commands/results; failed, skipped and unavailable checks.
**Review recipe:** Setup, actions and expected results, including captures if available.
**Deviations / limitations:** Scope changes, unresolved questions and accepted limits.
**Next-agent handoff:** Ownership, schema/command changes, pitfalls and next action.
**Acceptance:** User statement/date, or explicitly pending.
```

## Scope and gameplay decisions

### Agreed direction and retained contracts

- Four-character party, discrete movement, real-time exploration/approach and
  movement-locked contact combat. Preserve normal XP and supported levels 1–3.
- One fixed safe hub; three generated masonry floors; each has a designated guardian.
  Floors 1–2 unlock descent, floor 3's guardian wins the run. Ordinary encounter
  victory and clearing all actors are neither floor completion nor campaign victory.
- Persistent floors for the run. **No respawns**, rerolled loot, repeated XP or
  farming resets. A new run is a fresh population; library builds remain independent.
- A finite reserve of optional, approachable encounters offers XP, supplies and gear
  when a party struggles at the guardian. The user explicitly wants this fallback
  for the variety of viable D&D party compositions. Optional does not mean every
  fight is harder than the main route. Do not gate that reserve behind the guardian.
- Paid recall to town and return to an unlocked floor entrance, never a bookmark at
  the recall cell. Ordinary ascent remains free if supplies run out; the user accepts
  walking time as the cost of this fallback. Do not pad corridors to impose it.
- No shops in this MVP. No new combat escape, boss-bypass, resurrection, permanent
  stat unlocks, continuous inactive simulation or general puzzle framework.

### Concrete defaults to carry into detailed plans

The user accepted this roadmap, including these defaults as the planning baseline.
Detailed command semantics and implementation choices still receive their own slice
review; material changes must be called out rather than silently changing the baseline.

| Topic | Proposed MVP rule / review consequence |
| --- | --- |
| Recall cost | One existing camp kit per party recall, consumed only when travel commits. Camp kits remain finite and usable for field long rests. No separate currency or scroll system. |
| Recall eligibility | Explicit confirmed action from settled safe exploration, with a conscious able member; no moving, combat, active approach, unresolved impacts, hostile area hazard or terminal run. Use the same checked safety authority as rest/travel. |
| Return waypoints | A floor's entry unlocks only on first legal arrival via stairs. From the hub, choose any already unlocked entry for no additional kit; no warp to a guardian, unexplored depth or exact departure position. No dungeon-to-dungeon waypoint jumps. |
| Recall position | Never retained as a return portal. Do not relocate or reset enemies, refill resources or clear effects as part of recall. Preparation failure leaves party, kit and current floor intact. |
| Walking fallback | Adjacent stairs remain usable whenever safe, irrespective of kit count or waypoint use. Zero kits cannot trap a living party solely through a travel-resource requirement. |
| Recovery | Existing sanctuary long-rest behavior for living characters in the hub; existing short rests/hit dice and camp-funded field long rests. Recovery advances effect clocks; it is not a free refill merely on arrival. No resurrection or consumable refill. |
| Defeat | Party wipe ends the run. Save/resume supports interruption of a live run, not an in-game restart from an earlier save after recorded defeat. No anti-copying infrastructure. |
| Inactive floors | Freeze AI, actor/floor effect clocks and local RNG while absent. Party effects follow campaign gameplay time, including rest; menus/loading do not advance it. |
| Progression keys | Party-wide unweighted quest flags; a key permanently opens its one designated door. Cannot sell, drop or spend it elsewhere. Floor 1 has no mandatory key; floors 2–3 have one each. |
| Field knowledge | Small explored-tile record and readable map of discovered space; do not show the whole generated map or hidden objectives to player-visible agents. Existing full-map debug view remains explicitly diagnostic. |

Recall cannot guarantee camp supplies are always preferable to town recovery:
a player near the entrance will reasonably choose town. The intended tradeoff is
one kit for recovery **in place**, versus one kit for recall plus lost position
and retraversal from an entry. Free walking recovery remains possible by design.
Measure decisions and retracing time; do not promise a mathematically exploit-proof
economy or covertly add costs/respawns when tuning exposes a problem.

No respawns also means finite total XP. With level 3 as the cap, an extra level is
not always available: optional gear/supplies and easier fights provide additional
margin. Explicitly audit remaining XP to the next threshold and mandatory-route
XP before increasing deeper-floor threat. The direct route should work for
competent representative parties; it need not work for every build, depleted
party or tactical choice. Avoid universal win-rate targets that erase class differences.

## Current engine findings and proposed boundaries

Inspected on 2026-10-30; these findings are constraints, not a future API promise.

| Existing system | Actual integration consequence |
| --- | --- |
| [Map](../src/world/map.hpp) | Immutable row-major solid cells, no fixtures; many systems call `solid`. Door semantics must agree across every consumer. |
| [GameSession](../src/game/session.hpp) / [construction](../src/game/session.cpp) | Noncopyable owner combines Party, actors, loot, movement, AI, RNG, effects and combat while borrowing map/catalog. Constructing another session from a preset resets live party state. |
| [Navigation](../src/game/navigation.hpp) / [perception](../src/game/perception.cpp) | Incremental bounded BFS and sight/melee queries read the map. Pending searches need invalidation when geometry changes; room labels do not stop alerts. |
| [Frame pipeline](../src/render/frame_pipeline.cpp) / [renderer](../src/render/scene_renderer.hpp) | Prepared scene data is reusable across maps; cached frames/picking lack a door revision. A floor change needs fresh borrowers and frame state, not a copy of each prepared asset bank. |
| [Gameplay view](../src/app/gameplay.cpp) | Bundles session, animator, frames and command controller; restart reconstructs them. Floor switching must deliberately rebuild these borrowers. Keep orchestration out of subsystem implementation. |
| [Recovery](../src/game/session_recovery.cpp) / [XP](../src/game/session_progression.cpp) / [loot](../src/game/loot.hpp) | Rest advances gameplay effects without NPC walking; XP and loot use once-only death bookkeeping. Preserve those ledgers, not just alive/dead flags. |
| [Effects](../src/game/effects.hpp) | World-scoped actor/effect/area IDs, party casters and local deadlines cross ownership boundaries. Raw snapshots/runtime IDs are not portable saves. |
| [Automation session](../src/automation/session.hpp) | Arena-owned map/session, arena completion semantics, 10 ms ticks and a 360,000-tick ceiling (one simulated hour). Campaign launch/results and safe replacement need explicit support. |
| [Experiment config](../tools/arena_experiments/config.py) / [policies](../tools/arena_experiments/policies.py) | Level-one arena scenarios and limited fixed-loadout tactical coverage. Current policies do not explore dungeon topology or manage complete expeditions. |

### Generation pipeline

Use a small staged C++ subsystem with plain bounded records, not a generator
framework. Proposed flow:

1. Derive stable floor/phase seeds from run seed, depth and declared versions.
   Define integer PRNG and bounded sampling behavior; do not rely on implementation-
   specific `std::hash` or unordered iteration for output identity.
2. Choose a connected graph on neighboring sites of a coarse 4×4 lattice. Use a
   small catalog of valid route/loop patterns with rotations and meaningful branch
   variants. No arbitrary graph embedding, general corridor crossing repair or WFC.
3. Assign entrance, junctions, optional XP/supply branches, key route, guarded
   approach and objective. Ensure a useful loop shortens a return route; a tiny
   redundant cycle does not satisfy the experience requirement.
4. Stamp compatible authored room templates with cardinal ports and short
   corridors into an enclosed tile grid. Rotate footprints, ports, fixtures,
   facings and encounter anchors together. Check actual clearances for movement,
   front-three melee and existing visibility, not overhead appearance alone.
5. Select curated encounter/reward recipes from depth bands. Layout, encounters,
   loot and runtime combat/AI have independent random streams. Stable room/recipe
   identifiers prevent unrelated iteration changes from perturbing every reward.
6. Validate final cells, fixtures and placements, then publish an immutable floor
   definition with mutable state initialized separately. Retain diagnostics and
   a deterministic fingerprint. Never publish a partially valid floor.

Proposed initial bounds: 40×40 cells, 16 coarse sites, usually 8–12 occupied rooms,
32 actors, 32 fixtures, one mandatory key per keyed floor and one guardian group.
Keep existing engine ceilings (256 actors, 64 effects, current loot bounds) intact;
the generator's lower caps are content admission limits, not an engine-wide rewrite.
Use at most 16 complete candidate attempts, then one authored fallback per depth.
All fallback floors preserve that depth's keys, guardian, loop and rewards and pass
the same validators. Report attempt count, rejection causes and fallback identity.
If fallback/content validation fails, reject generation and preserve the active run.
Bounds and expected memory must be measured/reviewed in PC01, not silently increased.

Validation searches `(cell, acquired-key bits, guardian-complete bit)` on the actual
floor using the same passability rules as play. Check the initial key is reachable,
arrival and optional reserve are reachable before required combat, and descent is
unreachable without guardian completion (and the designated key on keyed floors).
Then prove the objective and return route reachable after legal transitions.
Guardian defeat is an abstract transition at a reachable, valid encounter approach;
this proves structural solvability, not victory for a real party. Detect bypasses,
unmatched ports, blocked fixtures, duplicate IDs and overlaps explicitly.

### Geometry, fixtures and discovery

Prefer adding a bounded floor-fixture layer and an effective collision/revision
view to the current map query path; support authored hub/arena/test maps and
empty-fixture behavior without preserving the obsolete static New Game mode. Do not maintain separate combat, AI and renderer copies of door truth.
A single-cell closed door blocks occupancy and sight; an opened door stays open
for the run. Opening is an explicit nearby faced interaction through a checked
command, not automatic wall removal or a debug mutation.

PC02 must choose the smallest concrete Map/geometry interface after auditing
movement, actor placement, incremental navigation, perception, melee, projectiles,
spell areas, scene composition and picking. Geometry revision invalidates stale
navigation and stationary cached frames. Preallocate all interaction state.
Prepare door/stair/key/waypoint imagery offline in the existing bank pipeline;
share depth/occlusion and picking rules. Review a small visual sample before a
full art set. Do not require new raster art just to establish PC01 topology.

Store discovered cells and known fixture states per floor. Player observations
and map UI use that record and current legitimate visibility; diagnostic queries
may inspect all state. Do not leak hidden rooms through action lists, errors,
entity counts or policy planning. Discovery is a bounded bitset plus fixture
knowledge, not a generic perception/memory framework.

### Campaign ownership, time and safe travel

Proposed owner: one `CampaignRun` holds a single live Party, run identity/status,
seed/version identities, campaign gameplay clock, keys, unlocked entries and four
location records (hub + three floors). One active floor session borrows that Party;
inactive floors retain owned value snapshots. Standalone arena construction keeps
an owned-party convenience path. Never create independent live party copies per
floor or derive travelling gear from the starting build.

Define floor export/import for PC04 independently of disk serialization: tile and
fixture definition, actor identity/HP/life/transforms/AI intent, loot and death/XP
ledgers, objective/reward bits, discovery, local RNG state and local effects/clocks.
Keep values contiguous where useful. Only the active floor has navigation/render
scratch. Retain all three generated definitions; eager generation at new-run
preparation is a reasonable bounded default and lets a bad floor fail before play.
No streaming service, database, background worker or generic entity serializer.

Travel is an explicit staged transaction at a settled safe boundary, not a flag
that makes the next frame reconstruct owners. Validate destination and landing,
prepare replacement simulation/view borrowers off the live path, then commit party
binding, floor state and any kit cost together. Exceptions, invalid snapshots,
capacity failures or asset failures leave the old run playable and uncharged.
Reset targeting, pending input, hover and overlays; rebuild animator/picking/frame
owners. Runtime world/session identities change so stale requests fail after travel,
load or replacement even if local entity indices coincide.

Landings need a real invariant: reserve entrance cells and their protected arrival
space from spawns, wandering, pursuit destinations and loot drops that could block
use; use geometry to prevent ranged sight into arrivals. Validate both first entry
and revisits with actual AI. Do not teleport blockers away or heal on arrival.
A failed landing check rejects travel without spending anything. The detailed plan
must show how safety is maintained, rather than treating a spawn-only check as proof.

Maintain separate campaign party-effect time and floor-local frozen time. Serialize
remaining durations and logical references, then rebase deadlines without extra
expiry, saves or RNG draws. Existing short/long rest skips must still apply once.
**Accepted roadmap boundary to specify in the PC04 detailed plan:** portable party-only effects and
concentration travel with the party; concentration attached to local enemies,
areas or a spectral weapon must be explicitly dismissed before travel. Explain
this in eligibility feedback; never silently discard it. Other local effects
freeze with their floor. Enumerate mixed-target spells and every supported clock
in the implementation design; avoid a blanket "copy effects" promise.

### Persistence and automation

PC06 serializes the already exercised PC04/PC05 state model. Store actual layouts
and mutations, not seed alone or a trace. Use content IDs and versioned schemas;
reject unsupported versions/content explicitly without a migration framework.
Rebuild derived stats, occupancy, IDs, navigation, UI and graphics after validation.
No raw pointers or live handles on disk; custom identities/portraits and spell/build
choices are independent of deleted library entries. RNG state belongs in the save.

Use one explicit out-of-combat save slot with temporary write and atomic replacement,
bounded decode and transactional load. Propose a terminal marker when a saved run
wins/is defeated so normal Continue cannot resurrect it; it is not a general autosave
framework. Define failure/retry behavior if that marker cannot be written, including
an honest warning and in-process Continue rejection. Manual copying is out of scope.
Do not silently delete the only good file on failed load/write.

Add campaign mode beside arena mode to existing headless commands, Python client,
replay and MCP adapter. Commands for fixtures/travel/recall/save go through game
validation, with preview/stale/error semantics. Campaign result is explicit.
Include generation identities, floor transitions, resources and objective events
in traces; strict replay remains separate from save/load. Retain arena compatibility.

PC08 adds the smallest useful exploration/expedition policy and reporting support
on the existing bounded serial runner. Do not promise current arena policies already
solve campaigns. Test fog-respecting exploration, legal actions, reaction cadence,
rest/loot/equipment/advancement coverage and travel choices. Reassess the one-hour
simulated cap, wall deadlines, trace bytes and cancellation latency with measured
runs; keep explicit finite budgets and distinct failure outcomes. No hosted MCP,
parallel worker platform, unattended balancing optimizer or new experiment service.

## Slice tracker and milestones

Execute sequentially unless the user approves a dependency change. Review the
individual detailed plan before implementation; a roadmap approval is not that plan.

| ID | Slice | Prerequisite | Status | Milestone |
| --- | --- | --- | --- | --- |
| PC01 | [Generated-floor walking prototype](#pc01-generated-floor-walking-prototype) | Roadmap accepted (2026-10-30) | Accepted (2026-10-30) | P-A |
| PC02 | [Doors, keys and discovered navigation](#pc02-doors-keys-and-discovered-navigation) | PC01 | Accepted | P-A |
| PC03 | [Guardians and structurally safe progression](#pc03-guardians-and-structurally-safe-progression) | PC02 | Accepted (2026-10-30) | P-A |
| PC04 | [Persistent descent and fixed hub](#pc04-persistent-descent-and-fixed-hub) | PC03 | Accepted (2026-10-30) | P-B |
| PC05 | [Paid recall and entry waypoints](#pc05-paid-recall-and-entry-waypoints) | PC04 | Accepted (2026-10-30) | P-B |
| PC06 | [Campaign save and resume](#pc06-campaign-save-and-resume) | PC05 | Accepted (2026-10-30) | P-B |
| PC07 | [Depth bands and finite recovery encounters](#pc07-depth-bands-and-finite-recovery-encounters) | PC06 | Planned | P-C |
| PC08 | [Campaign agents and expedition reports](#pc08-campaign-agents-and-expedition-reports) | PC07 | Planned | P-C |
| PC09 | [Integrated expedition and MVP acceptance](#pc09-integrated-expedition-and-mvp-acceptance) | PC08 | Planned | P-C |

| Milestone | Acceptance experience | Status |
| --- | --- | --- |
| P-A — Generated floor | Explore a seeded readable floor, obtain a key where required, defeat its guardian and unlock its objective without clearing every enemy. | Accepted (2026-10-30) |
| P-B — Persistent expedition | Travel among the fixed hub and three persistent floors, recall at a cost, revisit entry waypoints and resume an interrupted run. | Accepted (2026-10-30); PC04–PC06 accepted |
| P-C — Campaign MVP | A calibrated, reproducible three-floor descent with optional recovery encounters, normal XP, readable final guardian, victory/defeat and useful investigation evidence. | Planned |

### PC01: Generated-floor walking prototype

**Deliver:** Bounded graph/template generator, stable seeds/versions, tile/port
validation, retry/fallback diagnostics and a generated single-floor launch using
existing party and scene assets. Include entrance/objective/branch metadata and
small debug output showing seed, topology and retry causes. The player can walk
several generated layouts; this is the first generation checkpoint before saving.

**Boundary:** No campaign travel, disk saves, gameplay keys/doors or tuned population.
Objective metadata is provisional until PC02/PC03 validate actual gates. Existing
arena tooling remains supported. The static New Game launch may be replaced now;
it is not a compatibility requirement. Avoid an unrelated main-menu redesign.

**Acceptance / validation:** Fixed self-owned fixtures cover determinism, seed
separation, all allowed rotations, enclosed bounds, capacity failures, useful loops,
port connectivity and fallback failure. Expose a caller-invoked seed sweep/report
command, not a default 10,000-seed test. Walk representative first-person rooms;
inspect prepared rendering/strafe/occlusion and allocation checks. Record graphics
unavailability if applicable. No claim of combat balance or full progression yet.

**Review recipe:** Launch two retained seeds and replay one; compare fingerprint
and readable routes, inspect a forced fallback and a rejected invalid template.
Verify the ordinary arena still launches through existing tooling.

**Execution record:** Accepted and closed, 2026-10-30.
[Approved and implemented design](procedural_generation_design.md); user authorized
implementation with “Implement the plan.” New Game now launches empty seeded floors;
bounded generation/validation/fallback, diagnostic maps and strict replay are delivered.
[Verification and handoff](pc01_verification.md): headless Debug/Release 35/35,
graphics Debug/Release 53/53, focused sanitizers and zero tracked frame allocations,
hidden GPU/menu/arena checks, and thirteen retained review PNGs. No balance experiments
or broad seed sweep. User accepted: “slice accepted, record it in the doc” (2026-10-30).
Verification limitations remain unchanged; acceptance does not establish interactive
desktop execution. At this closeout PC02 remained Planned; the current PC02 record below supersedes that next action.

### PC02: Doors, keys and discovered navigation

**Deliver:** Shared effective geometry and revision contract, permanent opening,
checked adjacent fixture interaction, party quest-key state, one matching lock
on keyed floors, prepared door/key/stair markers and basic discovered-map navigation.
Add normal commands/observations/replay cases and player/diagnostic separation.

**Boundary:** No puzzle language, arbitrary switches or door closing. No mandatory
random skill check, inventory capacity test or rare consumable for progression.
Fixture visuals fit the prepared-image path, without rendering 3D geometry at runtime.

**Acceptance / validation:** Same closed/open outcome for movement, actors, BFS,
LOS/melee/projectiles/areas, rendering and picking, including opening while stationary.
Reject stale/remote requests without mutation; full inventory can still acquire key.
Tile-state reachability rejects key-behind-lock, bypass corridors and rotated defects.
Test hidden-map/fixture leaks and frame allocation. Inspect a small visual sample
before completing the fixture set; no OS input automation.

**Review recipe:** Find the key via a loop, inspect the locked approach, open it once,
repeat the request, and compare AI path/sight plus the first-person door image.
Show the discovered map and a separately labelled diagnostic map.

**Execution record:** Accepted and closed, 2026-10-30. User approved the
[PC02 detailed design](procedural_fixtures_design.md): “Implement the plan.”
Facing discovery, safe settled interaction and isolated full-map debug reveal confirmed.
Shared geometry, permanent doors, party keys, prepared fixture art, discovery/maps,
checked F/click/headless/MCP interaction and strict replay are implemented.
[Verification/handoff](pc02_verification.md) records full headless Debug/Release
37/37 and graphics Debug/Release 56/56, final focused 3/3 in each preset, focused
ASan/UBSan 4/4, final threshold-prompt checks (headless 4/4, graphics 5/5 each;
sanitizer 2/2), zero tracked runtime C++ allocations, hidden GPU/captures and
[capture gallery](screenshots/pc02/index.html). One test-owned temporary lifetime
bug was corrected following sanitizer detection. No balance experiments, broad
seed sweep or OS input automation. The user subsequently tested the available
gameplay and reported it working; this does not claim exhaustive interactive coverage.
Generator/templates 2/2; prepared format/renderer 4/4; scenario/protocol remain 1.
**Acceptance:** User: “okay in that case I accept the slice, ive tested as far as I could
and it is working, update the doc” (2026-10-30). The user understands that the gallery
shows separate test floors and traversal is not implemented until PC04. Basic door
art is accepted for this functional slice, not as final visual quality; no renderer
redesign is authorized by this closeout. PC03 is now eligible for its own reviewed plan.

### PC03: Guardians and structurally safe progression

**Deliver:** Curated initial encounters, stable designated guardian membership,
explicit descent gate, once-only guardian reward and floor objective status.
Floor 1 is unkeyed; depths 2–3 use their key dependencies. Validate final tiles,
actor placements, loops/optional branches and key/guardian state transitions.
Implement protected landing rules and pre-contact guardian warning. Extend replay
and commands; reuse existing monster definitions for initial mechanics.

**Boundary:** No broad bestiary or final difficulty claims. Removing an actor,
unrelated battle victory, lost awareness or leaving combat cannot count as guardian
kills. Gate checks must not depend only on which actors joined the current fight.

**Acceptance / validation:** Partial/multiple guardian deaths, reinforcement joins,
stale handles, reward capacity and repeat commands cannot open gates or duplicate
XP/loot incorrectly. Test approaches with actual perception/AI, ranged threat,
turn-based movement locks and protected landings. Fallback must meet the same
objective guarantees. Malformed seeds fail usefully, never quietly remove a lock.

**Review recipe:** Visit an optional route, attempt early descent, defeat an unrelated
group, then all designated guardians and unlock descent. Retain strict replay and
representative pre/post-gate captures. Obtain P-A acceptance separately from balance.

**Execution record:** Accepted and closed, 2026-10-30. User approved
[the detailed design](procedural_guardians_design.md) with “Implement the plan.”
Easy placeholder encounters and permanent-open stairs doors with protected alcoves
were confirmed during planning and implemented. All depths use two ordinary goblin
encounters plus a 14-HP melee guardian and an archer guardian. Only their permanent
deaths open the separate gate and publish reserved supplies once. Normal doors,
key dependencies, optional routes and protected arrival rooms are validated together.

[Verification/handoff](pc03_verification.md): headless Debug/Release 38/38 each,
graphics Debug/Release 57/57 each, five targeted ASan/UBSan suites, zero tracked
runtime C++ allocations, hidden GPU lifecycle, strict checked-combat replay and
[21 retained captures](screenshots/pc03/index.html). Generator/templates 3/3;
prepared format/renderer 4/5; protocol/scenario 1. Caption-only follow-up checks and
limits are recorded in the handoff. No balance experiments, broad seed sweep,
OS input automation or later-slice work. Stairs travel is still unavailable.
**Acceptance:** PC03 and milestone P-A accepted on 2026-10-30. The user played
through the accessible floor, defeated the guardian and collected the loot:
“I played through the floor that I can access and defeated the guardian, got the loot yay :) slice accepted, record it in the doc”. This accepts the functional slice; difficulty tuning remains deferred.
PC04 is next eligible for its own detailed plan, not implementation.

### PC04: Persistent descent and fixed hub

**Deliver:** One campaign Party, active-floor session binding and inactive floor
records; fixed authored safe hub and three generated floors connected by checked
stairs. New-run party creation uses existing library/formation flow exactly once.
Revisits retain mutations and resources; floor 3's designated guardian ends the run
in victory, a party wipe in defeat. Normal New Game now enters the authored hub;
remove obsolete static-mode launch wiring and exclusive dependencies. Add campaign
launch/status/travel to headless,
client/replay/MCP via existing adapters. Document clocks/effect portability first.

**Boundary:** In-memory persistence only; no save file, shops, recall yet, respawns
or simulation of inactive floors. Initial content proves the loop; PC07 tunes depth.
Do not leave a divergent legacy campaign simulation in the application entry point.

**Acceptance / validation:** Revisit after deaths, loot/drop, keys, open doors,
XP/advancement and rests; no starting gear or reward duplication. Include poisoned
stale command/target IDs, active effects/concentration, AI/RNG continuity, expired
party buffs and frozen local durations. Destination preparation failure rolls back
without party loss. Validate arrivals after enemies have moved, not just spawn.
Check borrower destruction/rebinding with sanitizers and repeated travel; verify
prepared assets are shared and no steady-frame allocation regression.

**Review recipe:** Use normal New Game and confirm it starts in town rather than
the old static dungeon. Descend to two floors, change HP/gear/formation,
leave loot, ascend and re-enter. Compare canonical state except documented clocks/
new runtime IDs. Walk all the way back with zero kits. Exercise final victory and
wipe with small owned fixtures; this is a lifecycle check, not a balance run.

**Execution record:** **Accepted and closed**, 2026-10-30. User approved:
“Implement the plan.” [Implemented design](procedural_campaign_travel_design.md),
[verification/handoff](pc04_verification.md), [gallery](screenshots/pc04/index.html).
New Game starts in the authored hub; shared campaign ownership, three persistent
floors, checked free stairs, portable effects/clocks and terminal Main Menu flow
are delivered through graphical/headless/client/replay/MCP adapters. Final full
headless Debug/Release 39/39 each, graphics 58/58 each, ASan/UBSan 3/3, allocation
checks and 9/14 preparation failure points, strict replay and hidden GPU/captures
pass. Interactive play coverage/live-host MCP remain unverified. No balance
experiments or later-slice work. **Acceptance:** User: “slice accepted, record it in the plan doc”
(2026-10-30). No additional interactive play-coverage details were supplied; recorded
verification results and limitations remain unchanged. PC05 is eligible for its own
reviewed detailed plan only; milestone P-B remains Planned.

### PC05: Paid recall and entry waypoints

**Deliver:** Confirmed camp-kit recall, persistent entry unlocks, fixed hub return
selector and clear cost/destination feedback. Reuse the PC04 travel transaction;
commit resource consumption and travel together. Field rest remains available and
useful because it preserves position. Extend observations/events/replay/MCP normally.

**Boundary:** No instant return to departure cell, portal object, deeper-floor
unlock by menu selection, shop/refill, combat escape or recall-specific currency.
Walking stays free; town-to-entry travel has no second kit fee under the proposal.

**Acceptance / validation:** Zero kits, insufficient quantity, stale inventory,
repeat/preview request, unsafe phase, moving, hazard, concentration restriction,
unvisited/unsafe landing and destination failure all leave resources/run intact.
One successful recall spends exactly one kit; reload/revisit cannot refund it.
Test free-walk and warp routes for hidden refills, XP duplication and progression
bypasses. Include the same input held across transition; no accidental second action.

**Review recipe:** Camp once in place, recall once from deeper in a floor, rest in
town and return to its entry. Confirm the old recall location is not a waypoint.
Compare kit counts, effect time and return distance. Demonstrate the zero-kit stairs.

**Execution record:** **Accepted and closed**, 2026-10-30.
User approved: “Implement the plan.” [Implemented design](procedural_campaign_recall_design.md),
[verification/handoff](pc05_verification.md), [14-image gallery](screenshots/pc05/index.html).
Confirmed one-kit recall and free unlocked-entry return share atomic staged travel;
first stairs arrivals alone unlock entries. Dedicated Travel panel preserves
Inventory T; field rest/explicit sanctuary recovery and free walking remain intact.
Commands/observations/events, CLI/client/local MCP and strict replay are extended.
Full headless Debug/Release 39/39 each, graphics Debug/Release 58/58 each, focused
ASan/UBSan 3/3 plus final campaign reruns, zero tracked allocations and 9/14
preparation failures per travel kind pass. Hidden GPU and deterministic captures
pass. Human interactive/live-host MCP coverage remains unverified. No balance
experiments, disk saves or PC06 work. **Acceptance:** User: “slice accepted, record it in the plan doc”
(2026-10-30). Verification results and limitations are unchanged; no additional
interactive play-coverage details were supplied. PC06 is next for reviewed detailed
planning only; P-B remains Planned.

### PC06: Campaign save and resume

**Deliver:** Versioned single-slot save at the legal stationary settled boundary;
load/Continue with atomic replacement and staged validation. Persist all campaign
and visited/unvisited generated definitions, party/build/inventory resources,
fixtures/keys, objectives/reward ledgers, discovery, waypoint unlocks, RNG and clocks.
Retain immutable generation provenance alongside actual layouts. Define incompatible
content/version behavior and terminal save handling. Integrate menu and automation.

**Boundary:** Supersedes demo slice 14; no mid-fight/active-approach saves, library
aliasing, generic migration infrastructure, save-scumming prevention or trace-as-save.
Must restore independently of deleted starting library entries.

**Acceptance / validation:** Round-trip hub and each depth before/after key, guardian,
recall, loot, advancement and time-based effects. Compare gameplay continuation and
RNG draws, not raw runtime IDs. Reject corrupt/truncated/oversized data, duplicate
ownership, unknown content, invalid references and unsupported schemas without
partial load. Inject write/rename/load/terminal-marker failures; retain valid prior
save and current run. Test terminal Continue rejection and resource lifecycle.

**Review recipe:** Save after visiting multiple floors and recalling, restart the
process, resume, revisit mutations and verify exact resources/unlocks. Try a bad
file and a removed library entry. Verify a defeated run is not offered as active.
Obtain P-B acceptance with actual persistence and documented failure behavior.

**Execution record:** **Accepted and closed**, 2026-10-30. User approved the
[detailed design](procedural_campaign_save_design.md) with “Implement the plan.”
Save + Save and Main Menu selected; a new run replaces the slot only on its first
confirmed successful save. Versioned checked snapshots, independent Party restore,
atomic single-slot publication, staged Continue, bounded terminal denial and
headless/client/local MCP/strict replay are implemented.
[Verification and handoff](pc06_verification.md), [14-image gallery](screenshots/pc06/index.html):
full headless Debug/Release 41/41 each, graphics 60/60 each; focused ASan/UBSan 3/3
plus final 1/1; final protocol follow-up 3/3 in all four presets, zero tracked frame
C++ allocations and 55/60 restore/view allocation failure points pass. Cross-build
and fresh-process saves, corrupted UTF-8 rejection, retained replay/corruption,
local SDK stdio, hidden GPU and all 19 repeat-identical captures pass. Human
interactive/native-close event and live-host MCP remain unverified. No balance
experiments, broad sweep, expedition experiment, commits or later-slice work.
**Acceptance:** User accepted the save/load slice on 2026-10-30: “I accept the save and love slice”. PC06 is closed, completing accepted PC04–PC06 for milestone P-B. The review corrections are included; verification results and limitations remain unchanged. No additional interactive play-coverage details were supplied. PC07 is next for detailed planning only, when requested.

### PC07: Depth bands and finite recovery encounters

**Deliver:** Three curated depth bands, 3–5 ordinary groups (including optional groups) plus a guardian as an
initial target, meaningful quiet/key/supply rooms and 1–2 optional reward branches.
Use existing supported monsters or explicitly labelled adaptations with prepared
art. Give the final guardian clear presentation; a unique species/boss framework
is not required. Allocate enough approachable optional XP/rewards to offer a
fallback when a party reaches the guardian underpowered.

**Boundary:** No respawns, dynamic party scaling, automatic level-per-floor,
mandatory full clear or universal party victory promise. No hidden food/repair tax
or shop economy. Target 10–15 minutes per floor is a hypothesis, not a test assertion.

**Acceptance / validation:** Record mandatory/optional XP by depth and attainable
level thresholds; show where extra encounters can help and where level 3 caps it.
Keep optional fallback accessible before the gate and avoid merging it into the
mandatory fight via alerts. Budget kits between field rest and recall; walking
remains possible if depleted. Validate encounter geometry and placements with fast
fixtures. With explicit opt-in, use existing arena calibration plus scripted real
floor approaches to inspect damage spikes, consumables and required-route viability.
Do not pass this slice based solely on CR sums or arena win rate.

**Review recipe:** Present the recipe/XP/supply table and sample generated floors.
Show a direct route and a struggling-party optional detour, including fewer-magic
compositions. Retain representative failures. If playtesting was not authorized,
report it as pending evidence rather than inventing balance confidence.

**Execution record:** Not started; PC06 is accepted. Campaign paused after P-B on 2026-10-30 for rendering/presentation and then additional monsters. PC07 remains next eligible when the user resumes campaign work.

### PC08: Campaign agents and expedition reports

**Deliver:** Bounded graph-aware exploration policy using legitimate discovered
information, legal fixture/travel decisions and declared tactical/resource/advancement
coverage. Reuse existing client, serial jobs, reports, replay/captures and MCP job
lifecycle. Extend scenario/provenance schemas for campaign seeds and depth recipes.
Add resources at each guardian, optional XP taken/remaining, hub/recall/rest counts,
retracing time, encounter merges and generation retry/fallback diagnostics to reports.

**Boundary:** No duplicated game rules, omniscient player agent, autonomous tuning
optimizer, new test service or exhaustive spell/build coverage promise. Diagnostic
agents may inspect full state, but cannot supply hidden knowledge to a player run.
Retain arena modes, output retention rules and bounded status/cancellation behavior.

**Acceptance / validation:** Fixture-owned short scripts test policy loops, wall/door
navigation, stalls, legal action cadence, missing capabilities and budget exhaustion.
Verify cancellation/reaping and partial reports mid-travel/save/replay. Measure
expedition-length budget requirements before raising any cap; expose explicit limits.
Keep policy error, stall, timeout, capacity/trace exhaustion, generation failure and
graphics/setup failure distinct from party defeat. Compare direct and adapter state.
No autonomous balance playthrough becomes a default CTest/build/CI check.

**Review recipe:** Inspect a small explicit campaign experiment configuration, start
only with opt-in, inspect status/results, cancel a job and reproduce one trial by
seed/versions/trace. Compare diagnostic and player-visible observations, and inspect
application-owned captures only where supported. Report untested decisions honestly.

**Execution record:** Not started; depends on PC07 acceptance.

### PC09: Integrated expedition and MVP acceptance

**Deliver:** Cohesive new-run/hub/descent/recall/save/result flow, player-facing
instructions, retained reproduction cases and measured MVP scope. Fix defects in
this campaign experience; this does not resurrect the retired general polish or
static-dungeon integration slice. Confirm no obsolete static-mode menu option,
launch dependency or compatibility-only code remains. Keep the hub modest and authored.

**Acceptance / validation:** Relevant Debug/Release headless and graphics suites,
allocation checks, focused lifetime sanitizers and available hidden GPU/capture
checks. Separately opt into a broad seed sweep (initial target 10,000), report
diversity, retries/fallbacks and bounded generation cost. Separately opt into
representative complete expeditions and required-route/optional-detour comparisons.
Measure compulsory XP, burst deaths, encounter merges, kit use, recall versus camp,
walking/retracing and guardian pacing; inspect outliers rather than a single mean.
Human review decides readability and fun. No universal win-rate or arbitrary
party viability requirement; no unexecuted check labelled passing.

**Review recipe:** Start in the fixed hub with a normal four-person party; explore,
find required keys, descend after guardians, use an optional XP/supply detour,
recall/return, save/restart/resume, and finish the final guardian. Verify persistent
revisits and separately show defeat/new run. Review seed/report/capture evidence
and explicit limitations. Stop for user acceptance of P-C and the MVP.

**Execution record:** Not started; depends on PC08 acceptance.

## Validation, risks and deferred work

Routine CPU tests use dedicated generated catalogs/maps/materials, fixed seeds and
isolated temporary files. Small structural seed sets test invariants; broad sweeps
measure coverage/variation separately. Avoid depending on shipped mutable content
for correctness fixtures. Allocation guards begin after preparation and cover new
ordinary fixture, navigation, simulation and presentation paths. Generation,
transaction preparation, file I/O and captures are explicit allocation phases.

Existing build/test presets include `headless-debug`, `headless-release`, `debug`
and `release`. Per-slice handoffs must name actual tests and record exact commands
once those targets exist. Do not invent executable/tool flags in a plan and claim
them as completed checks. Graphics-disabled builds must remain usable.

| Risk / pushback | Response and decision checkpoint |
| --- | --- |
| Fixed lattice feels repetitive | Start small enough to debug, vary meaningful loop/branch/interior/encounter choices; inspect route diversity and first-person readability before adding a more complex generator. PC01/PC09. |
| Door feature expands into renderer rewrite | One-cell permanent-open fixtures through shared geometry and prepared variants; sample-review occlusion/picking early. PC02. |
| Arbitrary safe arrivals are not guaranteed by a spawn check | Protected geometry/AI landing invariant, revisit tests and transactional rejection. PC03/PC04. |
| Party/effect transfer silently resets or duplicates state | Single Party owner, value snapshots, explicit clock/identity map and failure injection before disk saves. PC04. |
| Recall makes field rests pointless | Shared finite kit cost and loss of position; measure behavior, accept free walking fallback, adjust explicit economics only after review. PC05/PC07/PC09. |
| Guardian difficulty traps some reasonable parties | Accessible finite easier encounters/rewards provide margin; audit XP thresholds and supplies. Still fix overtuned required fights; no respawns or guaranteed grind level beyond cap. PC07/PC09. |
| Bot losses mistaken for encounter difficulty | Validate navigation/tactical coverage and retain trace; classify unsupported behavior separately. PC08. |
| Save design is premature or endlessly deferred | Establish generated floors and in-memory revisits first; implement PC06 before final content and long playtesting. |

Deferred: authored town shops/economy and dialogue, more floors/biomes, broader
bestiary and boss mechanics, levels beyond 3, branching campaigns, authored puzzles,
respawn/grinding systems, hunger, random affixes, permanent progression, combat
retreat, mid-fight saving and full inactive simulation. These are possible future
work, not this roadmap's implicit next slice. Town/floor identity and content
boundaries should accommodate them without building their systems now.

## Decision log

| Date | Decision | Basis |
| --- | --- | --- |
| 2026-10-30 | Renderer direction accepted, stacked cells deferred; separate six-slice renderer roadmap proposed. Campaign pause remains in force. | User agreed to the renderer proposal and requested a slice plan like this roadmap. No automatic implementation or PC07 work authorized. |
| 2026-10-30 | Pause campaign work after accepted P-B; prioritize rendering/presentation, then additional monsters before resuming. | User explicitly requests the pause and discussion of a low-resolution 3D renderer. Renderer implementation is not yet approved. |
| 2026-10-30 | PC06 Accepted and closed; milestone P-B complete through accepted PC04–PC06. PC07 next for detailed planning only. | User: “I accept the save and love slice, record it in the plan doc”. Review corrections included; verification and limitations unchanged. No new play-coverage details or balance authorization. |
| 2026-10-30 | PC06 implemented and Ready for review; PC06/P-B acceptance pending. | Approved plan: “Implement the plan.” Exact saved campaign ownership, staged Continue, single-slot atomic writes/terminal handling and matching automation; verification/handoff recorded. No balance or PC07 work. |
| 2026-10-30 | PC05 Accepted and closed; PC06 next for detailed planning only. | User: “slice accepted, record it in the plan doc”. Verification results and limitations unchanged; milestone P-B remains Planned. |
| 2026-10-30 | PC05 detailed plan approved with “Implement the plan”; implementation Ready for review. | One-kit recall, free unlocked-entry returns and dedicated Travel panel; verification/handoff recorded. Acceptance pending; do not advance to PC06. |
| 2026-10-30 | PC04 Accepted and closed; PC05 next for detailed planning only. | User: “slice accepted, record it in the plan doc”. Verification results and limitations unchanged; milestone P-B remains Planned. |
| 2026-10-30 | PC04 plan approved with “Implement the plan”; implementation Ready for review. | One Party, authored hub, persistent checked descent and final lifecycle; full checks and handoff recorded. User acceptance pending; do not advance. |
| 2026-10-30 | PC03 Accepted and closed; milestone P-A Accepted. PC04 is next for detailed planning only. | User played through the accessible floor, defeated the guardian, collected the loot and explicitly accepted the slice. |
| 2026-10-30 | Roadmap accepted as the scope, sequence and design/default baseline. PC01–PC09 remain Planned; next step is detailed PC01 planning in a fresh context. | User: “Ive read through and accept the plan/roadmap”; asks to record acceptance before a fresh context plans the first slice. |
| 2026-10-30 | Campaign replaces the static New Game mode; remove obsolete mode-specific code/content/tests during delivery. Arena tooling and shared engine behavior remain useful. | User explicitly says the existing static-map game mode need not be kept and this system will replace it. |
| 2026-10-30 | Close the static-demo delivery roadmap. Retire demo 15/16; transfer save/resume from 14 to PC06 after playable generation/travel. Preserve all prior acceptance history. | User says remaining static work is obsolete and requests a new multi-slice MVP plan. |
| 2026-10-30 | Long-term town has shops; first MVP follows the brainstorm and excludes them. | User clarifies expanded description is the eventual destination, not expanded MVP scope. |
| 2026-10-30 | Recall consumes a camp kit; return at an unlocked floor entry rather than departure position. Free walking ascent remains fallback. | User endorses the proposal and accepts walking time as the fallback cost. Exact refinements remain reviewable. |
| 2026-10-30 | No respawns in the MVP. Finite approachable optional encounters supply extra XP/resources for struggling parties; no need to clear all foes. | User agrees to start without respawns and proposes backtracking to skipped mobs as a fallback for varied parties. |
| 2026-10-30 | Nine proposed slices with acceptance gates; save after generation/travel, tooling extended with gameplay and expedition policies later. | Code inspection and proposed roadmap; awaiting review, not implementation approval. |

## Roadmap planning record and next-agent handoff

2026-10-30: Planning artifact only. Read the supplied MVP specification, accepted
system overview and demo tracking workflow; inspected Map, GameSession, navigation/
perception, effects, recovery/XP/loot, gameplay ownership, renderer/frame caching,
automation session and experiment config/policies. No gameplay code changed.
No builds, tests, balance experiments or graphics checks were run for this planning
task; graphics behavior is unverified here. Documentation validation passed:
`git diff --check` and a local checker of 355 relative links and applicable Markdown
heading anchors across the seven changed/new documents. Tracker dependencies and
supersession dispositions were also reviewed. The user-approved scope amendments are recorded above and in the spec.

### Roadmap acceptance and fresh-context handoff

2026-10-30: The user explicitly accepted: “Ive read through and accept the
plan/roadmap”, and requested recording it before a fresh context plans the first
slice. Roadmap planning is complete and accepted. All nine slices and all three
campaign milestones remain **Planned**; no slice implementation or delivery
acceptance is implied.

Next action: in the fresh context requested by the user, read AGENTS.md, the system
overview, the MVP specification and this roadmap; inspect relevant current code,
then produce the detailed **PC01-only** implementation plan for review. Preserve
working-tree changes. Do not repeat roadmap approval or implement PC01 before its
detailed plan is reviewed. The reusable planning prompt above is ready to use.

The old demo roadmap stays closed. Static New Game replacement is authorized;
there is no legacy-mode preservation requirement. Save/resume remains PC06 after
generation/travel. Balance experiments remain opt-in, and desktop restrictions
remain in force. Prior verification evidence and limitations are unchanged.
This acceptance update changes documentation only; no builds, gameplay tests,
experiments or graphics checks were run, and no commit was requested or created.


### PC01 accepted closeout and next-agent handoff (2026-10-30, historical)

The user explicitly accepted: “slice accepted, record it in the doc”. PC01 is
**Accepted and closed**. See [verification/handoff](pc01_verification.md) and
[capture gallery](screenshots/pc01/index.html). Earlier roadmap-planning directions
above are historical. Existing verification evidence and limitations are unchanged.

PC02 — Doors, keys and discovered navigation — is next eligible for detailed planning
in a fresh context when requested. Read AGENTS.md, the system overview, MVP spec,
this checkpoint and the PC01 handoff; inspect current code, then plan PC02 only for
review. Do not implement or advance automatically. PC02–PC09 and milestones P-A/P-B/P-C
remain Planned.

This acceptance update changes documentation only. All existing working-tree changes
are preserved; no builds, gameplay tests, captures, experiments or commits were made.

### PC02 accepted closeout and next-agent handoff (2026-10-30, historical)

The user explicitly accepted: “okay in that case I accept the slice, ive tested as
far as I could and it is working, update the doc”. PC02 is **Accepted and closed**.
The user tested the available gameplay and confirmed it works. Gallery floors are
separate scenarios; no traversal is expected before PC04. The door visuals remain
basic and are not treated as finished art. A renderer redesign needs its own scope.

The checkpoint and PC02 execution record above supersede historical PC01 next-agent
instructions. See the [handoff](pc02_verification.md), [design](procedural_fixtures_design.md)
and [gallery](screenshots/pc02/index.html). PC01 remains Accepted and closed.
PC03–PC09 and milestones P-A/P-B/P-C remain Planned. PC03 is next eligible for detailed
planning in a fresh context when requested: read AGENTS.md, the system overview,
MVP spec, this checkpoint and the PC02 handoff, then inspect code and plan PC03 only
for review. Do not implement or advance automatically.

This acceptance update changes documentation only. Existing verification results
remain unchanged, supplemented by the user's limited interactive testing. Live
Codex-host MCP interaction remains unverified. All working-tree changes are preserved;
no builds, gameplay tests, captures, balance experiments or commits were made.


### PC03 accepted closeout and next-agent handoff (2026-10-30, historical)

PC03 is **Accepted and closed**; milestone **P-A is Accepted**. The user played
through the accessible floor, defeated the guardian and collected the loot, then
explicitly accepted the slice. See [verification and reproduction](pc03_verification.md),
[implemented design](procedural_guardians_design.md) and [gallery](screenshots/pc03/index.html).
This checkpoint supersedes the historical PC02 handoff's instruction to plan PC03.

PC04 — Persistent descent and fixed hub — is next eligible for detailed planning
in a fresh context when requested. Read AGENTS.md, the system overview, MVP spec,
this checkpoint and the PC03 handoff; inspect current code and plan PC04 only for
review. Do not implement or advance automatically. PC04–PC09 and milestones P-B/P-C
remain Planned. Travel, hub, persistence and campaign victory remain unimplemented.
Intentionally easy placeholder content is accepted for functional progression;
more monsters and difficulty tuning remain later work, with balance experiments opt-in.

This acceptance update changes documentation only. Existing verification results
are supplemented by the user's accessible-floor playthrough; other depths remain
covered by separate automated scenarios. Live Codex-host MCP remains unverified.
All working-tree changes are preserved; no builds, gameplay tests, captures,
balance experiments or commits were made during this closeout.


### PC04 accepted closeout and next-agent handoff (2026-10-30, historical)

PC04 is **Accepted and closed**. The user accepted on 2026-10-30:
“slice accepted, record it in the plan doc”. Implementation was approved earlier
with “Implement the plan.” The [implemented design](procedural_campaign_travel_design.md),
[verification/review recipe](pc04_verification.md) and
[12-image gallery](screenshots/pc04/index.html) describe delivery and limitations.
This checkpoint supersedes the historical PC03 instruction to plan PC04.

Normal New Game starts in the fixed hub; one Party travels through three persistent
floors using free checked stairs. Effects and clocks have an explicit portable/local
boundary; final guardian completion closes new actions while accepted consequences
settle. Run victory/defeat returns to Main Menu. Existing arena/investigation tools
remain supported. Starting supplies, loot/XP, doors, keys, resources and discovery
persist without respawns or refills. No recall, waypoint selector, disk save or tuning.

Final headless Debug/Release 39/39 each and graphics 58/58 each pass; focused
ASan/UBSan 3/3, zero tracked runtime C++ allocations, 9/14 preparation allocation
failure points, strict replay/corruption rejection, direct/client/local MCP adapter
checks, hidden GPU and application-owned captures pass. Interactive play coverage
and live Codex-host MCP remain unverified. The victory image is explicitly a HUD
reconstruction, not a fought expedition. No balance experiment or broad sweep ran.

Next action: in a fresh context when requested, read AGENTS.md, the system overview,
MVP specification, current checkpoint and PC04 handoff; inspect current code and
plan **PC05 — Paid recall and entry waypoints — only for review**. Do not implement
or advance automatically. PC05–PC09 and milestones P-B/P-C remain Planned.

This acceptance closeout changes documentation only. Verification results and
recorded limitations are unchanged; acceptance supplies no additional interactive
play-coverage details. All working-tree changes are preserved. No builds, gameplay
tests, captures, balance experiments, commits, resets or stashes were performed
during this closeout.


### PC05 accepted closeout and next-agent handoff (2026-10-30, historical)

PC05 is **Accepted and closed**. The user accepted on 2026-10-30:
“slice accepted, record it in the plan doc”. Implementation was approved earlier with
“Implement the plan” after selecting the dedicated Travel panel. See the
[implemented design](procedural_campaign_recall_design.md),
[verification and review recipe](pc05_verification.md) and
[gallery](screenshots/pc05/index.html). This supersedes PC04's next-planning instruction.

T/header Travel explicitly confirms one-kit recall; the hub entrance selects free
unlocked floor entries. Inventory T retains transfer. Safe ordinary stairs stay
free with zero kits. One campaign transaction stages the kit and supplier stats,
floor/effect snapshots and presentation before commit. No automatic recovery,
refill, new unlock by selection, departure portal, disk save or content tuning.

All four full suites, focused sanitizers, allocation/fault checks, strict replay,
CLI/client/local MCP, hidden GPU and repeat-identical retained captures pass as
recorded in the handoff. Human interactive/live Codex-host MCP remain unverified;
no balance experiment, broad sweep or complete expedition experiment was run.

Next action: in a fresh context when requested, read AGENTS.md, the system
overview, MVP specification, current checkpoint and PC05 handoff; inspect current
code and plan **PC06 — Campaign save and resume — only for review**. Do not implement
or advance automatically. PC06–PC09 and milestones P-B/P-C remain Planned; earlier
accepted slices remain closed.

This acceptance closeout changes documentation only. Verification results and
limitations are unchanged; acceptance supplies no additional interactive play-coverage
details. All existing working-tree changes are preserved. No builds, gameplay tests,
captures, balance experiments, commits, resets or stashes were performed during
this closeout.


### PC06 implementation and next-agent handoff (2026-10-30, historical)

PC06 is **Ready for review**, not Accepted. The user approved its reviewed plan
with “Implement the plan.” The [implemented design](procedural_campaign_save_design.md),
[verification/review recipe](pc06_verification.md) and [gallery](screenshots/pc06/index.html)
record delivery. The current checkpoint supersedes PC05's historical next-planning
instruction. PC05/P-A stay accepted; PC07–PC09 and P-B remain Planned.

The single manual slot retains all actual campaign/party/location state and legal
save clocks; Continue does not recreate starting builds or rerun generation.
Checked validation and staged presentation preserve valid owners on load failure.
Publication/uncertain durability and terminal-marker failure/retry are explicit.
Keep the travelling Party, exact navigation/RNG continuation, fresh runtime epochs,
content names and player/diagnostic boundary. Strict replay records external I/O
and uses a memory backend; it remains separate from save compatibility.

All four full suites, final focused protocol/sanitizer checks, tracked frame
allocation guards, preparation fault injection, fresh-process/cross-build saves,
CLI/client/local SDK MCP, retained replay/corruption and hidden GPU/captures pass.
No human interactive/native-close event or live-host review is claimed. No balance
experiment, broad sweep or full expedition experiment ran. All existing changes
were preserved; no commits, resets, stashes or installed configuration changes.

Next action: user review of PC06/P-B. Do not self-accept or start PC07. Only after
acceptance, in a fresh context when requested, inspect current code and plan PC07
for its own review; balance experiments still require explicit opt-in.

PC06 presentation review correction: unavailable Continue is hidden, confirmation
panels are compact, and failed writes use plain language with Retry. Four focused
Release checks pass; all 19 refreshed captures match a repeat, with 14 retained.
See the [review correction verification](pc06_verification.md#pc06-presentation-review-correction-2026-10-30).
Acceptance remains pending.


### PC06 accepted closeout and next-agent handoff (2026-10-30, current)

PC06 — Campaign save and resume — is **Accepted and closed**. The user accepted
the save/load slice on 2026-10-30: “I accept the save and love slice, record it in the
plan doc”. This includes the reviewed UI corrections: hidden unavailable Continue,
compact confirmations and plain save-write failure language. Accepted PC04–PC06
complete milestone **P-B — Persistent expedition**. PC01–PC05/P-A stay closed.
See the [implemented design](procedural_campaign_save_design.md),
[verification/handoff](pc06_verification.md) and [gallery](screenshots/pc06/index.html).
This record supersedes historical PC06 review-pending and next-planning notes.

Campaign work is **paused after P-B** at the user's request: the
[renderer roadmap](renderer_plan.md) comes next, followed by additional monsters,
before returning here. Renderer direction is accepted; its slices remain unimplemented.
PC07 — Depth bands and finite recovery encounters — remains next eligible for its own
reviewed detailed plan when the user resumes campaign work. Read AGENTS.md, the
system overview, MVP specification, current checkpoint and PC06 handoff; inspect
current code before planning. PC07–PC09 and milestone P-C remain Planned. Do not
implement or advance automatically; balance experiments still require explicit opt-in.

This closeout changes documentation only. Existing verification and limitations
remain unchanged; no additional interactive play-coverage details were supplied.
All working-tree changes were preserved. No builds, gameplay tests, captures,
balance experiments, commits, resets or stashes were performed during this closeout.
