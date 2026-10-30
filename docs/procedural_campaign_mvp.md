# Procedural dungeon and campaign MVP

Date: 2026-10-30.

Status: **Proposed MVP specification for implementation planning. Not implemented.**

This document captures the intended first procedural dungeon/campaign milestone
following the static dungeon demo. It describes the player experience, scope,
constraints and completion criteria. It is **not an implementation plan**: it
does not assign slices, prescribe delivery order or authorize implementation.
The planning agent should inspect the current project before proposing a separate
multi-slice plan. Existing accepted gameplay contracts remain authoritative unless
a change is explicitly identified and approved.

The central proposal is a short, replayable descent: a four-character party leaves
a safe hub, explores three generated dungeon floors, finds each route downward,
overcomes floor guardians, and defeats a final guardian to win the run.

## 1. Intended experience

Preserve the existing first-person blobber format, discrete grid movement,
real-time exploration/approach and turn-based contact combat. The objective is a
fast dungeon expedition with meaningful exploration between fights, rather than
a succession of arenas or long tactical battles.

Players should regularly decide whether to explore an optional branch for supplies
or equipment, commit resources to the next fight, approach the floor guardian, or
return to safety. Clearing every enemy is not the objective. Finding and earning
access to the next floor is.

Procedural variation should change routes, room interiors, encounter combinations
and rewards while preserving readable navigation and sensible difficulty. A map
that is technically connected but tedious, repetitive or unfair is not successful.

## 2. Scope and initial tuning targets

The three-floor run, fixed hub, required guardians, persistent revisits and final
victory form the MVP scope. The numerical floor/content targets below are starting
points for testing, not claims of established balance or immutable engine limits.

| Area | Initial target |
| --- | --- |
| Party | Four characters using existing creation/library and formation systems |
| Campaign | One fixed hub and three generated floors |
| Theme | One masonry dungeon theme, reusing prepared scene assets |
| Floor dimensions | No more than approximately 40×40 cells initially |
| Rooms | Usually 8–12 meaningful rooms or spaces per floor |
| Encounters | Usually 3–5 ordinary groups plus one guardian encounter |
| Population | Initially aim to stay within 32 actors per floor |
| Exploration | At least one useful loop and 1–2 optional reward branches per floor |
| Keys | Floor 1 introduces the loop without a mandatory key; floors 2–3 each use one key/lock dependency |
| Progression | Existing normal XP and supported character levels 1–3 |
| Duration | Aim for roughly 10–15 minutes per floor, verified through playtesting |
| Replayability | New runs can use new seeds; a seed can be retained for reproduction |

The three floors must have a perceivable increase in challenge without requiring
three visual biomes. Depth selects a calibrated content band. The dungeon does
not secretly rescale itself to the party's current health, equipment or level.

## 3. Campaign, hub and run lifecycle

The hub is a small fixed safe location, not a town simulation. It provides access
to recovery, entry into the dungeon and the normal run/menu flow. Shops, NPC
dialogue, crafting and a hub economy are outside scope.

Floors connect through stairs. Floors 1 and 2 have a guarded descent; floor 3
culminates in the final guardian and a clear victory state. Players can ascend
through previously visited floors and return to the hub when travel is legal.

A generated floor is persistent for the duration of its run. Revisiting cannot
reroll enemies or loot, respawn defeated actors, close permanently opened doors,
reset keys, or repeat rewards and XP. The travelling party retains its actual
builds, inventory, equipment, formation, health, mana, XP and spent resources.
Entering a floor must not grant starting equipment or camp supplies again.

### Proposed recovery and defeat defaults

These are explicit proposal defaults from the brainstorming, not historical
accepted decisions. The implementation plan should call out any change to them.

- The hub offers existing sanctuary-style free recovery for living characters.
- Field long rests continue to consume camp kits under existing recovery rules.
- There is no free consumable refill, resurrection service or enemy respawn.
- Party wipe ends the run. Starting-character library entries survive; live run
  progress does not become permanent character upgrades.
- Save/resume supports interruption of an active run using the project's legal
  out-of-combat save boundary. A terminal defeated run is not resumable as active.
  No anti-copying or anti-save-scumming infrastructure is required.

Free hub recovery means attrition primarily creates pressure between hub visits,
not an irreversible resource countdown across the whole campaign. Accept and
measure this consequence. If optimal play becomes returning after every fight,
revisit recovery economics explicitly; do not hide the problem behind long walks
or unannounced respawns.

## 4. Floor structure and progression

Each floor includes a safe arrival area, connected exploration spaces, ordinary
encounters, optional rewards, a guardian approach and the floor objective.
Some rooms should be quiet or contain a key, supplies or a junction rather than
another mandatory battle. Long empty dead ends should not be the main source of
exploration time.

The ordinary route to descent passes through the guardian encounter. The guardian
is an explicitly designated group of actors, not simply whichever enemies happen
to be present when combat begins. Its defeat permanently unlocks descent and the
main guardian reward. Killing unrelated enemies, losing awareness, or leaving
combat must not satisfy the objective. A monster merely standing near stairs is
not a sufficient gate.

Guardian encounters may use a tougher supported variant and a small escort;
they do not require a unique monster species or boss subsystem on every floor.
The final guardian must be recognizably presented as the run's final challenge.
Exact creatures, mechanics and artwork belong to the later content/design plan.
Adapted variants must be identified as adaptations rather than unmodified SRD
stat blocks. A large new bestiary is not a prerequisite for generation.

Optional routes provide useful but nonessential rewards. The required route must
remain viable without rare loot or killing every optional enemy. Boss bypasses,
secret mandatory passages and multiple endings are deferred.

Illustrative topology, not a prescribed tile layout:

```text
                    key room ----- supply room
                   /                          \
entrance ----- junction ------------------- antechamber
                   |                          |
             optional treasure          locked approach
                                              |
                                         guardian arena
                                              |
                                    reward and descent
```

The key route loops back toward the locked destination. Optional connections
remain upstream of the guardian gate so they cannot accidentally bypass it.

## 5. Noncombat interactions

The initial noncombat objective is a simple key/lock relationship. One clearly
identified key permanently opens its designated door. Required keys are reachable
before their locks and are collected into party-wide quest state without inventory
weight or slot requirements. They cannot be dropped, sold or consumed by another
door. This is an explicit proposed adaptation to avoid softlocks.

Required progression must not depend on a particular class, random perception or
lockpicking checks, a scarce consumable, or a hidden interaction. Doors and keys
need recognizable associations and clear feedback. A key hunt supplies an
exploration dependency; it is not intended to be a deep puzzle by itself.

Pressure-plate puzzles are outside this first MVP. The party shares one world
position, so four separate characters cannot occupy four plates. A later extension
can introduce an authored plate room with visible, latched gate activation before
adding resettable switches, crates, weight or timed mechanisms. Do not build a
general puzzle language or puzzle solver for this MVP.

## 6. Generation approach and guarantees

Recommended foundation: a small room graph with deliberate loops, realized through
compatible authored room patterns and short cardinal corridors. Decide route and
objective relationships before filling spaces with enemies and loot.

A coarse 4×4 arrangement of possible room locations is a suitable initial spatial
constraint. Only some locations are occupied; neighboring locations connect through
designated ports. A few known-valid structural patterns can vary orientation,
branches, room interiors and encounters. This avoids general graph embedding and
unintended corridor crossings. The exact layout algorithm and representation are
design choices for the later plan, provided they satisfy this specification.

Templates should describe coherent spaces: entrances, walkable interiors, fixture
locations and compatible encounter positions/facings. Rotate related information
together. Suitable starting shapes include simple chambers, pillar rooms, bent
passages, supply alcoves and guardian arenas. Templates must produce readable
first-person spaces, not just attractive overhead maps.

Generation must provide:

- Deterministic output for the same seed, generator/content versions and inputs.
- Separate randomness for layout, encounter selection and loot, independent of
  live combat and AI randomness.
- Enclosed map bounds, valid placements and consistent corridor/room connections.
- Useful loops and optional routes without unintended objective bypasses.
- A structurally solvable route through every required key and guardian condition.
- Bounded generation work, explicit capacity handling and useful diagnostics.
- A known-valid fallback after bounded retries, with fallback use reported rather
  than concealing failure or silently removing required objectives.

Validate the finished tiles and fixtures, not only the abstract room graph.
Plain connectivity with doors open is insufficient. Reachability must account for
key possession and guardian completion: the key is accessible before its lock,
the objective becomes reachable after the required transitions, and the objective
cannot be reached by bypassing them. Winning a reachable required fight can be an
abstract transition for this check. Structural validation does not prove combat
balance or guarantee survival for an arbitrarily depleted party.

No new procedural-generation dependency is required by this specification. BSP
is a reasonable future alternative for spatial layout; cellular caves, general
graph grammars and Wave Function Collapse are not required for this milestone.

## 7. Combat pacing, difficulty and resources

Use curated encounter recipes rather than independently scattering random monsters
on tiles. Each recipe identifies its composition, suitable geometry, placement and
facing requirements, reward role and tested threat band. CR/XP information provides
source context; practical threat must be calibrated in this game's combat system.

SRD CR is a starting point, not an additive difficulty score or a mapping from floor
number to monster CR. The SRD assumption of a well-rested four-person party helps,
but formation protection, ranged approach, grouped initiative, mana and movement
restrictions materially change encounter outcomes. Ordinary encounters, guardians
and cumulative pressure before recovery need separate tuning.

Preserve normal XP advancement. Do not award one level automatically per floor.
Check how much XP the compulsory route actually provides before raising difficulty;
optional clearing should help without becoming a concealed requirement. Retain
some easier fights on deeper floors so advancement feels useful. Avoid untested
damage spikes and control combinations, especially for level-one characters.

The generator must respect actual perception, alert and reinforcement behavior.
Different room labels do not isolate fights. Use distance, corners, doors and
controlled patrols where appropriate, and verify likely approaches with the actual
AI. Unintentionally merging several encounters undermines the fast-combat goal.

Current contact combat locks party movement. Preserve that behavior for this MVP;
do not assume players can flee after engaging. Guardian approaches need readable
warning before commitment. Stair landings must remain valid and free of immediate
hostile contact or ranged fire, including when revisited. Geometry should create
different approaches and enemy arrangements rather than merely larger rooms.

Use existing recovery and loot systems. Ensure useful supplies and ordinary rewards
occur within controlled budgets; random drops must not be the only source of an
item required for progression. Exact starting supplies and reward quantities are
tuning decisions, not fixed by this document.

## 8. State, presentation and engine constraints

Keep the C++17/raylib engine, prepared-image rendering, fixed 1920×1080 presentation
and discrete movement. Generation does not require a renderer rewrite or runtime
generation of art. New door, stair and objective visuals must integrate with the
existing preparation and presentation approach.

Doors are gameplay geometry. Closed/open state must agree across player movement,
NPC navigation, visibility, ranged/projectile queries, scene rendering and picking.
Opening a door must update cached results consistently. A door must not be closed
visually while allowing shots or navigation through it.

Floor travel is a checked, stationary, safe transition with no unresolved movement
or attacks. Failure to prepare or restore the destination preserves the valid
current run. Cross-floor ownership, transient handles and UI references must remain
safe. There is one authoritative travelling party, not independent party copies
whose resources diverge between floors.

Save/resume must retain authoritative campaign state: party state, active floor
and position, generated floor identity/layout, enemies, remaining loot, doors,
keys, completed objectives and rewards already granted. Preserve exploration
discovery if present. A seed alone is not a run save. Starting-build library
entries remain independent of live campaign saves.

Proposed inactive-floor behavior is frozen simulation while the party is absent.
Party effects follow campaign time; floor-local effects remain with their floor.
The implementation plan must specify duration and identity restoration consistently
with existing rules, avoiding free recovery, duplicated effects or dangling
references during travel. Simulating every inactive floor continuously is not a
requirement.

Generation, loading and transition preparation may allocate outside normal frame
processing. Ordinary simulation, rendering and fixture interactions retain existing
engine-owned frame allocation guarantees. Use bounded, explicit capacities and
failures. Share prepared assets where appropriate rather than multiplying their
memory cost by the number of floors. Detailed ownership and data layout are left
to the implementation design.

## 9. Project context for the planning agent

This section is a dated orientation, not a replacement for inspecting current code.
On 2026-10-30, the overview records accepted progression through level 3, an arena,
headless sessions, automated players/balance experiments and a local MCP adapter.
Save/resume is still recorded as unstarted. Another agent may advance these systems
before this MVP is planned; do not duplicate work based on an outdated snapshot.

Relevant starting points:

- [System overview and current status](system_design.md).
- [Existing demo roadmap](dnd5e_demo_design.md).
- [Hybrid encounter behavior](hybrid_encounter_design.md).
- [Arena ownership and generated placements](arena_design.md).
- [Progression](progression_design.md).
- [Loot and recovery](loot_recovery_design.md).
- [Offline image preparation](image_preparation_design.md).
- [Headless sessions](headless_design.md) and
  [automated players](automated_players_design.md).
- [Local SRD 5.1 reference](references/README.md).

The current Map has immutable wall/floor cells; GameSession combines party and
floor-local systems while borrowing map/catalog. Mutable fixtures and persistent
multi-floor travel therefore need deliberate integration design. This specification
does not mandate a particular class split or refactor. Reuse the available arena,
headless and experiment tooling for relevant verification instead of inventing a
second testing platform. Expensive balance matrices remain opt-in, outside routine
correctness tests.

## 10. Using the existing agent automation

The existing headless and MCP systems are a strong foundation for this MVP's
playtesting. They drive the real simulation through checked actions and offer
recording, strict replay, deterministic policies, bounded experiments and readable
reports. Reuse them rather than implement a separate approximation of combat.
See [automated-player design](automated_players_design.md),
[headless protocol](headless_protocol.md), [MCP usage](mcp_usage.md) and
[experiment usage](../experiments/arena/README.md).

### Concrete uses

| Investigation | Useful evidence |
| --- | --- |
| Encounter calibration | Compare curated recipes against several sensible parties and policies; record casualties, turns, HP/mana and consumable expenditure |
| Approach and reinforcement | Run actual room approaches and examine whether neighboring groups join unexpectedly or stairs expose arriving parties |
| Expedition pacing | Track resources at each guardian, rests, hub returns, optional branches visited, required-path XP and time spent retracing cleared routes |
| Campaign integrity | Script key pickup, door opening, ascent/descent, revisits and save/load; verify no duplicated loot/XP or reset state |
| Difficult-seed investigation | Retain seed/version, party, policy, reaction timing and trace; replay the exact failure and inspect selected scene/HUD captures when supported |
| Change comparisons | Reuse declared seed sets and party/policy inputs to compare generator or content revisions; report distributions and uncertainty, not just an overall win rate |

These are intended uses, not claims that campaign automation already exists.
Current starts use level-one arena scenarios and fixed arena geometry. Current
policies know that open arena and do not navigate unknown dungeon walls. The
fixed-loadout experiment profile does not automate looting, resting, equipment
changes or advancement, although the protocol already exposes those gameplay
operations. Policy coverage also excludes some spells and higher-level decisions.
Doors, keys, campaign launch/travel and campaign observations will require
appropriate support alongside the new gameplay. Completing an arena roster must
not be mistaken for completing a campaign floor or winning a run.

The later plan should identify the smallest extensions needed to exercise this
MVP through normal engine validation. Preserve explicit coverage reporting:
unsupported decisions, policy failures, stalls, timeouts, budget exhaustion and
graphics/setup failures are not combat defeats. Existing run/trace budgets must
be assessed against longer expeditions; do not silently truncate a run or remove
bounds. Strict replay remains version/build-sensitive and is not a campaign save
format; compare changed builds through fresh experiments with retained inputs.

Use two clearly separated perspectives. Diagnostic agents may inspect full state
to find unreachable objectives or explain an unexpected encounter. Player-visible
agents must explore using legitimately discovered information, without reading
hidden keys, enemies or the full generated map from diagnostic output. Preserve
the existing distinction between these modes and the declared simulated reaction
cadence; agent execution speed must not grant instantaneous real-time actions.

Compare competent builds and sensible compositions, including relevant alternatives
with fewer magical resources. Do not require every possible party to win every
seed or flatten class differences to meet a universal success target. A losing
policy can indicate poor tactical coverage rather than overtuned content. Validate
policy competence and retain representative failures before drawing conclusions.

Balance matrices and full expedition playthroughs remain deliberate, opt-in
experiments, including small smoke matrices. They are not automatic build/CTest
requirements. Fast fixture-owned automation correctness checks remain separate.
Reports should help agents and the user identify specific problems; automatic
balance changes or an autonomous tuning optimizer are outside the MVP. Human
review still decides whether exploration is readable and enjoyable.

## 11. Completion criteria and verification expectations

The MVP is complete when a player can start a party in the hub, explore and descend
through three generated floors, revisit earlier locations without state resets,
use keys and recovery, earn normal progression, save/resume a valid active run,
and reach an unambiguous final victory or run-ending defeat.

Acceptance must establish:

- **Progression safety:** representative generated floors satisfy key/lock and
  guardian requirements on actual tiles; malformed and capacity-exhausted cases
  have defined outcomes. Required keys remain collectible with full inventory.
- **State integrity:** revisits/reloads preserve resources and floor changes;
  unrelated victories cannot open objectives; XP, loot and starting supplies
  cannot be duplicated through travel or repeated commands.
- **Interaction consistency:** door geometry agrees across all consumers; arrivals
  remain valid; stale interactions and failed travel/load operations are safe.
- **Replayability:** deterministic reproduction works and a broad seed sample shows
  meaningful variation, acceptable retry/fallback rates and bounded cost.
- **Pacing:** representative legal parties can progress without mandatory optional
  clearing; fights stay reasonably short; reinforcement chains and burst casualties
  are examined; cleared-corridor travel and hub visits do not dominate play.
- **Presentation and performance:** new fixtures remain readable in first person,
  preserve prepared rendering and meet frame-allocation/resource-lifecycle contracts.

Use fast, self-owned CPU fixtures for invariants and failure cases. Keep broad seed
sweeps and balance experiments separate; an initial 10,000-seed sweep is a useful
target, not a routine unit-test requirement. Automated players can reveal outliers
but do not prove fun or balance. Inspect representative deterministic captures.
Follow repository desktop restrictions: no OS input/focus automation; interactive
desktop review belongs to the user. Report unexecuted checks explicitly.

## 12. Explicitly outside scope

- Endless campaigns, large floor counts, branching campaigns or multiple endings.
- Multiple biomes and a broad monster/content expansion.
- General pressure-plate puzzles, crate pushing, timed gates or arbitrary puzzle synthesis.
- Secret guardian bypasses, required hidden doors, skill-gated critical progression.
- Shops, crafting, hunger, random item-affix systems or a full town economy.
- Permanent stat unlocks, resurrection services or enemy respawn/farming loops.
- New combat retreat mechanics, free tactical movement or a replacement combat system.
- Dynamic difficulty scaling, continuous inactive-floor simulation or a generic scripting framework.

## 13. Research basis

These references motivated the recommendation; the MVP is a project-specific
combination, not a reproduction of any one game's generator.

- [NetHack 3.6.7 generation source](https://github.com/NetHack/NetHack/blob/NetHack-3.6.7_Released/src/mklev.c):
  staged room/corridor generation, connectivity repair, extra connections and
  separate special-room placement.
- [Basic BSP dungeon generation](https://www.roguebasin.com/index.php?title=Basic_BSP_Dungeon_generation):
  non-overlapping spatial regions and connections; useful alternative to the
  initial coarse room arrangement.
- [Joris Dormans: Cyclic Dungeon Generation](https://www.youtube.com/watch?v=mA6PacEZX9M):
  deliberate loops and lock/key patterns as the foundation of exploration flow.
- [Brogue CE Architect.c](https://github.com/tmewett/BrogueCE/blob/master/src/brogue/Architect.c):
  explicit loop creation and blueprint-based placement of related features.
- [Wave Function Collapse reference implementation](https://github.com/mxgmn/WaveFunctionCollapse):
  local pattern constraints are useful but do not themselves establish this MVP's
  global progression or pacing requirements.

The requested next artifact is a separate multi-slice implementation plan based
on this specification and the current repository. That plan should surface any
material unresolved gameplay choices, define implementation designs and validation,
and preserve the existing project's acceptance process.
