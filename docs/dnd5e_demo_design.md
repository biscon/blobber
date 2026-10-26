# SRD 5.1 goblin demo: design and delivery roadmap

Status: gameplay roadmap; slices 01–08 and milestones A/B are Accepted. Slice 08 was accepted on 2026-10-26, including the three-cell melee front and edge-visibility correction. Slice 09 remains Planned; the user wants to discuss graphics before starting it.
Created: 2026-10-25. Execution records below distinguish implementation from
user acceptance.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Latest accepted slice | 08 — Hybrid encounter loop and review corrections (2026-10-26) |
| Active slice | None |
| Awaiting user review | None |
| Blockers | None |
| Next eligible slice | 09 — Effects and concentration (Planned; not started) |
| Next action | Discuss the user’s graphical changes before planning or starting slice 09 |

Update this checkpoint, the tracker, and the relevant execution record together.
Do not infer completion or user acceptance from code being present.

## How agents must use this document

This is the authoritative delivery roadmap and cross-context handoff for the
demo. [System design](system_design.md) describes implemented architecture;
this document distinguishes intended behavior from delivered behavior. Read
[AGENTS.md](../AGENTS.md) first. Later user instructions take precedence over
this document; record changes so the next agent inherits them.

### One slice, then human review

1. Read this document, the system design, relevant subsystem designs, and the
   preceding slice's execution record. Inspect the actual implementation before
   planning. Never assume an old handoff is more accurate than current code.
2. Check the current checkpoint and tracker. Work on one slice only. A previous
   slice awaiting review, an unfinished active slice, or an unaccepted prerequisite
   prevents automatically advancing to another slice.
3. Mark the selected slice **In progress**, recording whether it is in planning
   or implementation. Make its detailed implementation plan and ask the user
   about material ambiguities. The roadmap is not blanket approval of every
   future implementation choice.
4. Let the user review that plan before implementation. Existing approval in the
   conversation is sufficient; do not ask again for already authorized work.
   Once approved, preserve the plan or a concise durable summary in the execution
   record, linking a dedicated design file if one is warranted.
5. Implement the authorized slice, documenting changed designs before code changes
   and matching the documentation to the final result. Make routine technical
   decisions autonomously. Discuss changes to gameplay, visual direction, scope,
   or milestones with the user rather than silently changing the target.
6. Run the relevant checks. Update the execution record, tracker, checkpoint,
   and implemented system documentation. Provide a concrete user review recipe.
7. Mark **Ready for review** and stop. Do not begin the next slice automatically.
   Only the user can accept the result. Record conversational acceptance when it
   arrives, including any explicitly accepted limitations.

Keep review corrections in the same agent context when practical. After user
acceptance, start the next slice in a clean context. A replacement agent must
also be able to resume an interrupted slice from its execution record.

In Plan mode or another read-only session, report proposed tracking updates in
the plan and persist them when writing is allowed. These instructions do not
override the session's permission or mode constraints.

If a slice proves too large, propose a split with a useful review checkpoint.
Preserve existing slice IDs; use suffixes such as 10a/10b rather than silently
renumbering completed history. Record approved dependency and milestone changes.
Do not quietly push unfinished requirements into another slice.

### Status meanings

| Status | Meaning |
| --- | --- |
| Planned | Work has not started. |
| In progress | Planning, implementation, or review corrections are underway. |
| Blocked | A named decision or prerequisite prevents further progress; record what can unblock it. |
| Ready for review | Implementation and required available checks are complete; user acceptance is pending. |
| Accepted | The user accepted the result and any recorded limitations. |

Failed required checks are unfinished work, not a passing checkpoint. Unavailable
desktop checks must be identified as unexecuted. They may be presented for review
with that limitation, but cannot be reported as passing. Agents cannot mark their
own work Accepted. If review requests changes, return the slice to In progress.

### Reusable prompts

For a clean-context planning session:

> Read AGENTS.md, docs/system_design.md, and docs/dnd5e_demo_design.md. Identify
> the next eligible slice and read the previous handoff notes. Inspect the
> relevant code, then make an implementation plan for that slice only. Ask about
> material ambiguities. If a previous slice is awaiting review or prerequisites
> are incomplete, report that instead of advancing.

After reviewing its plan:

> Implement the approved slice. Follow the roadmap's tracking instructions,
> update the design and handoff records, and run the relevant checks. Stop at
> Ready for review; do not start another slice.

### Execution-record template

Replace each slice's initial empty record with this structure as work proceeds.
Use repository-relative links and exact verification commands. Do not rely on
chat history, temporary files, or a commit ID as the only explanation of the work.

```markdown
**Stage / last updated:** Planning, implementing, or reviewing; YYYY-MM-DD.
**Approved plan:** Durable summary or link; record the user's approval.
**Delivered:** Observable behavior and relevant code/design entry points.
**Verification:** Commands and results; failed, skipped, and unavailable checks.
**Review recipe:** Setup, controls/actions, and expected observable results.
**Deviations / limitations:** Explicit changes, remaining work, and user decisions.
**Next-agent handoff:** Integration contracts, pitfalls, and unresolved questions.
**Acceptance:** Pending, or date and a faithful summary of the user's acceptance.
```

Preserve material findings and acceptance history when updating a record. Link
long verification reports instead of filling the roadmap with raw command output.

## Target and existing foundation

The completed demo supports exploring the existing test level, fighting moving
goblins, managing a four-character party, progressing through level 3, and saving
and resuming outside combat. It uses a streamlined SRD 5.1 ruleset with a phased
turn-based encounter loop, not continuous real-time melee combat.

Current implemented foundation, inspected on 2026-10-25:

- C++17/raylib, discrete movement, prepared wall and NPC images, and deterministic
  capture support. The 640×360 art composition is presented at fixed 1920×1080.
- A 25×25 test map and four goblin placements sharing one cell's 2×2 subslots.
- Immutable `NpcWorld` loading data initializes an `ActorWorld`, bounded to 32
  appearance types and 256 stable actor slots. Checked movement/facing/HP/removal
  updates live occupancy. No AI or combat resolution exists yet.
- Idle, walk-in-place, and attack visual loops. `NpcAnimator` uses an absolute
  clock, including 100 ms white damage/death feedback; the frame pipeline observes
  movement, actor and animation revisions.
- Existing CPU correctness/allocation tests and separate desktop GPU checks.

Relevant entry points are `src/world/npc.*`, `src/game/actors.*`, `src/game/movement.*`,
`src/game/npc_animation.*`, `src/render/frame_pipeline.*`, `src/ui/hud.*`, and
`src/app/application.cpp`. Recheck these when planning; they will evolve.

### Sources and adaptation policy

The original user brief was `/home/bison/Downloads/5e_blobber_system_spec.md`.
Its agreed requirements and subsequent clarifications are captured here; a new
agent does not need access to that external file. This document supersedes its
ambiguous choices for this demo.

Use the [official SRD 5.1 Creative Commons edition](https://media.dndbeyond.com/compendium-images/srd/5.1/SRD_CC_v5.1.pdf),
not SRD 5.2 or an unrestricted Monster Manual content collection. Slice 01 must
establish a source/adaptation ledger, preserve required attribution when adopting
content, and cite the source of implemented rules and definitions.

The SRD 5.1 goblin is present on printed page 315: CR 1/4, AC 15, 7 HP, and +4
weapon attacks. Use that baseline and tune encounter size; no CR 1 substitute
is needed. Existing dagger artwork may remain, but any mechanical equipment or
feature changes must be recorded as adaptations rather than claimed as verbatim
SRD behavior. Nimble Escape depends on omitted bonus-action mechanics and needs
an explicit adaptation/defer decision in the AI plan.

Mana, cell-based areas, automatic facing, formation protection, and the other
departures below are project adaptations. Do not imply that changing the action
economy preserves tabletop encounter balance automatically. Validate pacing in
play. Do not import a non-SRD rulebook wholesale because the original brief
called its mana model the official spell-points framework.

## Agreed gameplay and presentation

### Party and progression

- Exactly four active characters occupy one world cell in a 2×2 formation.
- Automatically create a balanced preset: Human Fighter, Dwarf Cleric, Halfling
  Rogue, and Elf Wizard. Use fixed names, builds, and starting equipment chosen
  in slice 02. Race and class are independent definitions; preset pairings are
  not race/class restrictions.
- Support levels 1–3 with regular XP thresholds and monster awards. Add a clearly
  development-only button granting each character enough XP for their next level;
  it must use normal advancement rather than directly overwriting derived stats.
- Use preset advancement choices initially. The level-3 plan selects the supported
  SRD subclasses and records adapted/deferred features. Character creation and
  a general build editor are future work.

### Encounter rhythm and formation

- Exploration and ranged approach run in real time. Characters can use ranged
  weapons or spells against legal visible targets, with per-character cooldowns.
  Unaware targets can grant the brief's opening-shot advantage. The encounter
  plan must define awareness, detection, and provocation, including misses.
- Adjacent hostile contact starts individual turn-based combat. Characters and
  enemy groups roll initiative; actions resolve in that order. Preserve hidden
  initiative rolls while making the active actor and phase legible.
- Enter initiative directly; no shared maneuver prompt or party surprise penalty.
  Keep facing while any enemy remains in the three cells immediately ahead. After it clears and presentation
  settles, automatically face the next adjacent group by shortest rotation,
  clockwise on ties. Preserve the active action and clock. No player movement,
  manual rotation or retreat command during combat.
- Offscreen enemies cannot attack. Adjacent side/rear enemies wait until faced;
  distant offscreen enemies can approach once at the round boundary. New arrivals
  can be attacked immediately once faced but join initiative next round.
- Each front slot always protects its paired rear slot from ordinary melee until
  that front character is unconscious/dead. Enemy slots also use paired protection.
  The Rogue back-row exception stays in slice 10. NPC awareness, surprise and
  unnoticed opening-shot advantage remain; party surprise is removed by review.
- Include melee, ranged attack, spell, item, defend, and back-row melee commands.
  Omit bonus actions, reactions, readying, opportunity attacks, and tabletop
  movement distances. Explicitly adapt signature features instead of silently
  deleting class identity; Second Wind using an action and a formation-aware
  Sneak Attack trigger are design starting points for the class-feature plan.
- Include unconsciousness, death saves, stabilization, healing, and death. Party
  defeat offers restart, and loading once persistence exists. Rest does not
  resurrect dead characters.

Pause/menu transitions, cooldowns, combat entry/exit and reinforcement entry share
coherent timing rules. A phase change cannot reset costs or grant duplicate attacks.
No adjacent threat means Approach after holds; incomplete rounds grant no six-second
advance. Re-contact retains initiative and action deadlines. See the slice 08 design.

### Spells and effects

Use mana with the brief's costs: spell levels 1/2/3/4/5 cost 2/3/5/6/7 respectively.
Only cantrips and curated first- and second-level spells are required for this
demo. Define the level-1–3 mana progression during the spell slice. Higher-level
examples such as Fireball are future content, not demo requirements.

Retain attack rolls, saving throws, concentration, and relevant durations for
supported spells. Map spatial targeting into explicit cell patterns. The brief's
primary-tile "maximum damage" means full rolled damage before relevant saves or
reductions, not maximum dice results. Do not convert every touch spell into an
area spell: each supported spell needs an explicit adapted targeting rule.
The spell plan must specify visibility, range, friendly-fire, and splash behavior
for the actual selected spells rather than building a speculative spell engine.

### Inventory, equipment, and screens

- Each character has a fairly large equal-slot inventory grid; every icon occupies
  one slot, with no inventory packing puzzle. Exact grid dimensions belong to
  the inventory screen plan. Large screens may cover the dungeon viewport.
- Include quantities, stack merging/splitting, transfers, descriptions, ammunition,
  consumables, equipment slots, and a paper-doll silhouette. Equipped items change
  relevant statistics and remain part of carried weight.
- Use Strength × 15 lb carrying capacity (SRD 5.1, printed pp. 79–80), without the
  optional graded encumbrance variant. Validate capacity and ownership on item
  transactions, including transfers and load restoration.
- Browsing inventory, equipment, and spellbooks pauses simulation, including the
  real-time approach phase. Casting/consuming still pays the appropriate action
  or cooldown cost. Equipment changes and inventory transfers occur outside combat.
- Keep the current fantasy aesthetic. Deliver four portraits, item/spell icons,
  a paper-doll silhouette with slot icons, readable status/action displays, and
  targeting/damage feedback. Layered illustrated clothing is not required.

### Recovery, loot, and saving

Short rests spend Hit Dice and refresh supported features outside danger. Long
rests require either a designated safe location or camp supplies and cannot occur
during an active threat. Define resource amounts and safety checks in slice 12.
Loot must have explicit ownership independent of a monster's visual corpse.

Provide one versioned, validated out-of-combat save covering party state,
inventory, progression, party position, and persistent level changes, including
defeated enemies and collected loot. Saving during hostile approach is not a
way to bypass the out-of-combat restriction. Mid-fight restoration is deferred.

## Proposed system responsibilities

| Subsystem | Responsibility and ownership |
| --- | --- |
| Rules and content | Immutable race, class, monster, item, spell, and progression definitions; stable content identifiers; source/adaptation records. |
| Game session | Owns party state, mutable actors, encounter phase, simulation clock, RNG, and command processing. Application code orchestrates its lifecycle. |
| World simulation | Live occupancy, actor movement/facing, awareness, and AI; placements initialize runtime actors rather than remaining the live state. |
| Combat and effects | Validates actions and resolves rolls, resource costs, damage, death, and timed effects independently of presentation. |
| Inventory and progression | Owns item transactions, equipment-derived statistics, XP, and advancement invariants. Avoid duplicate authoritative state. |
| Presentation | Reads snapshots and bounded visual events; owns HUD/screens, targeting feedback, and animation playback. |
| Persistence | Serializes gameplay state using stable content IDs and restores it through validation; excludes GPU/prepared-image resources. |

Keep interfaces small: commands enter simulation; read-only snapshots and bounded
events leave it. A general ECS, event bus, scripting language, or inheritance
framework is not required. Rules and simulation must remain testable without a
window. Asset loading and JSON parsing stay outside simulation hot paths.

Mutable actor revisions must invalidate views alongside movement and animation
revisions. Reuse immutable prepared sprites as actors move; do not rebake assets
per move. Damage and death resolve once in simulation, independent of visual
playback, hidden actors, interrupted animations, or skipped frames.

Use contiguous bounded runtime storage, preallocated pathfinding/targeting/effect
scratch, and bounded logs/events. Declare capacities, lifetimes, and exhaustion
behavior in each slice plan. Preserve existing limits unless a measured need
justifies changing them. Loading and explicit save/export phases may allocate;
normal engine-owned frame processing must retain the allocation regression checks.
Use typed actor/item references; generation-check recycled slots, or document
stable non-recycled lifetimes. Do not persist raw pointers or transient handles.

Preserve the prepared-image renderer, existing projection, discrete movement,
and fixed 1920×1080 presentation. Initially retain the 640×360 art composition;
layout and overlay changes do not require a new world renderer. Test input
mapping through the existing aspect-fit presentation.

## Slice tracker and milestones

Execute sequentially. Each slice requires all preceding slices to be Accepted,
unless the user explicitly approves a dependency change. The prerequisite column
names the immediate checkpoint, not permission to ignore earlier incomplete work.

| ID | Slice | Prerequisite | Status | Milestone |
| --- | --- | --- | --- | --- |
| 01 | [Rules foundation and content](#slice-01-rules-foundation-and-content) | None | Accepted | A |
| 02 | [Characters and preset party](#slice-02-characters-and-preset-party) | 01 | Accepted | A |
| 03 | [Mutable monster world](#slice-03-mutable-monster-world) | 02 | Accepted | A |
| 04 | [Party HUD and commands](#slice-04-party-hud-and-commands) | 03 | Accepted | A |
| 05 | [First melee combat](#slice-05-first-melee-combat) | 04 | Accepted | A |
| 06 | [Goblin hurt/death animations](#slice-06-goblin-hurt-and-death-animations) | 05 | Accepted | B |
| 07 | [Moving goblin AI](#slice-07-moving-goblin-ai) | 06 | Accepted | B |
| 08 | [Hybrid encounter loop](#slice-08-hybrid-encounter-loop) | 07 | Accepted | B |
| 09 | [Effects and concentration](#slice-09-effects-and-concentration) | 08 | Planned | C |
| 10 | [Spells and class identity](#slice-10-spells-and-class-identity) | 09 | Planned | C |
| 11 | [Inventory and equipment](#slice-11-inventory-and-equipment) | 10 | Planned | C |
| 12 | [Loot and recovery](#slice-12-loot-and-recovery) | 11 | Planned | C |
| 13 | [Levels 2–3](#slice-13-levels-2-and-3) | 12 | Planned | C |
| 14 | [Save and resume](#slice-14-save-and-resume) | 13 | Planned | D |
| 15 | [Cohesive presentation](#slice-15-cohesive-presentation) | 14 | Planned | D |
| 16 | [Demo integration](#slice-16-demo-integration) | 15 | Planned | D |

| Milestone | Acceptance experience | Status |
| --- | --- | --- |
| A — First fight | A preset party can fight stationary goblins, win, or lose through actual combat rules. | Accepted |
| B — Dungeon skirmishes | Goblins approach, move, attack, react to hits, and die through the hybrid encounter loop. | Accepted |
| C — RPG loop | Class abilities, spells, inventory, equipment, loot, recovery, and levels 1–3 work together. | Planned |
| D — Finished demo | The test level is playable, resumable, visually coherent, documented, and verified. | Planned |

Every slice ends with user review. At milestone boundaries, also review the
combined experience; passing isolated tests does not establish the intended feel.
Record milestone acceptance in its closing slice and update the milestone table.

### Slice 01: Rules foundation and content

**Deliver:** Deterministic dice resolution, abilities/modifiers, proficiency,
attack/save resolution, damage primitives, and validated content identifiers.
Establish the source/adaptation ledger and attribution for adopted SRD content.

**Boundary:** Build the minimum foundation needed by the preset and goblin.
Do not import the entire SRD or create a generic rules scripting system.

**Acceptance:** Headless tests exercise ordinary rolls, advantage/disadvantage,
critical attack results, failure paths, and invalid content. Reproducible RNG
supports later encounter tests without depending on rendering or wall-clock time.

**Stage / last updated:** Review complete (Accepted); 2026-10-25.
**Approved plan:** User approved the implementation plan with “Implement the plan.”
Editable JSON was explicitly selected. See [rules design](rules_design.md):
deterministic CPU rules, bounded validated catalog, goblin baseline, attribution,
startup loading, headless correctness/allocation checks, and Debug/Release suites.
**Delivered:** Graphics-independent `blobber_rules` and `blobber_rules_io` targets;
explicitly seeded fixed-storage rolls, pure attack/save resolution, physical
damage primitives, and immutable typed catalog definitions. Editable version-1
JSON contains the goblin baseline and two attack profiles; startup validates it
before map/image preparation. Source/adaptation ledger and copied attribution
are included. Entry points: `src/rules/resolve.hpp`, `src/rules/content.hpp`,
`src/assets/rules_loader.hpp`, and `assets/rules/catalog.json`.
**Verification:** Debug and Release configure/build succeeded; all 8 CTest suites
passed in each (including zero-allocation checks). Shipped/copied catalogs and
startup failure ordering passed. X11/OpenGL startup from `/tmp`, 99 captures,
and clean GPU/window shutdown passed; corridor and goblin captures inspected.
See [exact commands, results, and limitations](slice01_verification.md).
**Review recipe:** Run the focused headless tests and `--validate` diagnostic,
inspect/edit a temporary catalog, and optionally launch the existing dungeon as
described in the [review instructions](slice01_verification.md#user-review).
**Deviations / limitations:** No scope deviation. No gameplay integration beyond
startup validation; character definitions and equipment remain slice 02.
No Windows/macOS execution, interactive gameplay recheck, or GPU pixel-baseline
comparison was performed. Existing rendering/movement code is unchanged.
**Next-agent handoff:** Slice 01 was accepted on 2026-10-25. Slice 02 is now
eligible for planning in a clean context; inspect the current code and propose
its implementation plan for user review before implementing it.
Extend the existing versioned catalog deliberately for independent race/class
and equipment definitions. Keep JSON outside simulation. Catalog IDs never
recycle and are valid only for their owning catalog lifetime; they are not NPC
type IDs. Retain stable strings for future persistence. Proficiency currently
rejects levels outside 1–3; damage types are physical only. The application owns
definitions but creates no gameplay session or production RNG yet. Dagger art
uses the scimitar mechanical baseline; Nimble Escape remains deferred to slice 07.
**Acceptance:** 2026-10-25 — User explicitly accepted slice 01: “slice accepted,
update the plan so its ready for the next slice”. No additional conditions or
scope changes were requested. Verification limits above remain recorded;
milestone A is not yet complete.

### Slice 02: Characters and preset party

**Deliver:** Independent race/class definitions, fixed four-character builds,
formation, resources, derived statistics, and minimal starting item/equipment
definitions. Record which level-1 features are active and which arrive in slice 10.

**Boundary:** No character-creation UI or general inventory screen. Starting gear
must use real definitions so later inventory work does not replace hardcoded stats.

**Acceptance:** All four characters load validly at level 1; race/class pairing
is not structurally restricted, gear affects statistics, and formation/resources
have deterministic headless checks.

**Stage / last updated:** Review complete (Accepted); 2026-10-25.
**Walking review correction:** User requested continuous movement using the existing
walk art. Implemented 600 ms visual travel between committed cells, continuous
projection/shadows/picking, pause/death handling and arrival-gated combat. Both
19-suite configurations pass; final captures and the separate 256-actor CPU
composition measurement are recorded in the verification report. Logical collision
remains discrete; no slice 08 work was introduced.
**Approved plan:** User said “Implement the plan.” See [character design](characters_design.md).
User selected SRD variants (Hill Dwarf, Lightfoot Halfling, High Elf) and active
stat passives, including Dwarven Toughness and Fighter Defense. Implement immutable
content, validated four-character builds/gear, derived statistics, fixed party
resources/formation, startup validation, headless diagnostics/tests, and handoff.
No UI, combat, mana progression, inventory transactions, or later-slice work.
**Delivered:** Version-2 catalog with independent race/class/item definitions and
fixed four-character preset; legacy version 1 remains readable as rules-only
content. Pure character derivation and bounded party state provide initialized
HP/Hit Dice/XP, stable identity, checked formation swaps, and read-only snapshots.
The application constructs the party before map/image preparation. The existing
content diagnostic prints formation, resources, equipment, and derived statistics.
Entry points: `src/rules/character.hpp`, `src/game/party.hpp`, and
`src/assets/rules_character_loader.cpp`; shipped data remains in the rules catalog.
**Verification:** Debug and Release configure/build succeeded without new warnings;
all 10 CTest suites passed in both, including the extended zero-allocation guard.
Shipped/copied catalogs and attribution checks passed. Six startup failure-order
checks passed. Desktop startup from `/tmp` generated 99 captures and shut down
cleanly; corridor and goblin captures were visually inspected. See
[exact commands, results, and limitations](slice02_verification.md).
**Review recipe:** Run focused tests and the `--validate` diagnostic; inspect the
four builds and try temporary equipment edits described in the
[review instructions](slice02_verification.md#user-review).
**Deviations / limitations:** No scope deviation. Party state is initialized but
has no UI or combat/resource-spending commands. Mana and conditional/signature
features remain deferred as documented. No interactive gameplay recheck, historical
pixel-baseline comparison, standalone GPU smoke suite, Windows/macOS execution,
or scale benchmark was performed.
**Next-agent handoff:** Slice 02 was accepted on 2026-10-25. Slice 03 is now
eligible for planning in a clean context; inspect current world/placement,
movement, animation, and frame-pipeline code and propose its implementation plan
for user review before implementing it. The immutable catalog must outlive and remain unmoved for
the party; take snapshots by const reference to avoid copying startup-owned strings.
Character IDs are party-local, owned-item indices character-local, and definition
IDs catalog-local; none recycle. Four fixed records and a formation permutation
preserve identity/resources on swaps. Equipped entries borrow the same owned gear
counted once toward weight. Derived caches are read-only; later resource/equipment
changes need authoritative validated operations. The movement controller still owns
the shared world pose. No mutable NPC world, session clock/RNG, mana, or combat was
introduced. Schema 2 requires a valid preset at application startup; schema 1 remains
supported by the rules loader/diagnostic. The 16-entry starting-gear bound does not
choose slice 11's inventory size. Milestone A is not yet complete.
**Acceptance:** 2026-10-25 — User explicitly accepted slice 02: “slice accepted,
close it so we're ready for the next”. No additional conditions or scope changes
were requested. Verification limits above remain recorded; milestone A is not yet
complete. At that acceptance checkpoint, slice 03 remained Planned.

### Slice 03: Mutable monster world

**Deliver:** Runtime actors initialized from placements, with health, position,
facing, lifecycle, and atomic occupancy updates. Define identity/lifetime and
connect actor revisions to movement collision and frame invalidation.

**Boundary:** No autonomous AI yet. Preserve existing asset formats where possible
and document any compatibility change. Prepared appearances remain immutable.

**Acceptance:** Controlled actor moves/removals update stationary views and collision;
blocked moves, occupied slots, stale references, and resource bounds are handled.
Existing transitions and no-allocation checks still work.

**Stage / last updated:** Review complete (Accepted); 2026-10-25.
**Walking review correction:** User requested continuous movement using the existing
walk art. Implemented 600 ms visual travel between committed cells, continuous
projection/shadows/picking, pause/death handling and arrival-gated combat. Both
19-suite configurations pass; final captures and the separate 256-actor CPU
composition measurement are recorded in the verification report. Logical collision
remains discrete; no slice 08 work was introduced.
**Approved plan:** User said “Implement the plan.” See [actor design](actors_design.md).
Placement-only stable runtime actors, checked movement/facing/damage/removal, live
occupancy and rendering revisions, and scripted checks/captures. User selected
a brief white flash including lethal hits, then hiding; dead actors immediately
stop blocking. Preserve prepared rendering and asset formats; no AI or combat UI.
**Delivered:** `ActorWorld` owns 256 fixed records and live occupancy, initialized
from placements and catalog monster HP. World-scoped IDs reject foreign/removed
references; slots never recycle. Checked mutations are atomic and allocation-free.
Movement queries live occupancy; actor revisions refresh stationary views and
both transition endpoints. Animation supplies a 100 ms white flash, frozen lethal
pose, then hiding; death releases occupancy immediately. Original asset formats
and appearance preparation remain unchanged. Sixteen scripted captures exercise
mutation without adding gameplay controls.
**Verification:** Debug/Release builds succeeded without warnings; all 12 CTest
suites passed in both, including expanded zero-allocation checks. Unknown monster
binding failed before window initialization. Desktop capture from `/tmp` exited
cleanly with 115 PNGs; all 99 original captures were pixel-identical. New actor
captures were visually inspected. GPU lifecycle checks passed. See
[commands, results and limitations](slice03_verification.md).
**Review recipe:** Run focused tests and generate the scripted captures described
in the [user review](slice03_verification.md#user-review); compare move/turn,
flash/expiry, removal and transition images.
**Deviations / limitations:** No scope deviation. Normal gameplay initializes actors
but exposes no mutation controls; scenarios exercise them explicitly. No spawning,
AI, attack resolution, loot/rewards, hurt/death clips, or general session clock.
No manual interactive movement recheck, Windows/macOS execution, scale benchmark,
or combat playtest was performed.
**Next-agent handoff:** Slice 03 was accepted on 2026-10-25. Slice 04 is now
eligible for planning in a clean context; inspect party/actor snapshots, existing
HUD/presenter, input/aspect-fit mapping, and application orchestration, then
propose its implementation plan for user review before implementing it. The map
and catalog must outlive the noncopyable/nonmovable actor owner. Actor IDs contain a
unique world identity and stable slot; dead records are readable, removed ones
fail lookup, and neither can act. Spatial subslots may be reused by other living
actors; removing an old dead record must not clear its replacement's occupancy.
Pass the controller's committed logical cell to actor movement. Apply mutations
before sampling `NpcAnimator` on the same elapsed-time axis as damage cues, then
compose; stale/foreign animation snapshots are rejected. Rendering uses stable
slot indices, including terminal records. Death is simulation-owned and unblocks
immediately, independently of the flash. Existing static/animated asset JSON is
unchanged; each appearance type string must resolve to a catalog monster ID.
The optional no-animation render path hides dead actors immediately. Slice 04
remains Planned and eligible for planning; milestone A is incomplete.
**Acceptance:** 2026-10-25 — User explicitly accepted slice 03: “I accept this
slice, close it up so were ready for the next”. No additional conditions or scope
changes were requested. Verification limits above remain recorded. Milestone A
is not yet complete; slice 04 has not started.

### Slice 04: Party HUD and commands

**Deliver:** Four portrait panels, HP/mana/status display, active character,
action selection, targeting, and bounded combat log. Use the command/snapshot
boundary rather than allowing UI widgets to mutate rules state directly.

**Boundary:** Functional placeholder art is acceptable until slice 15. Exercise
command flows with deterministic development scenarios; initiative arrives next.

**Acceptance:** Legal selections are clear; invalid/unavailable commands explain
why and spend no resources. Check focus loss, held input, target changes, and
aspect-fit input coordinates. Inspect deterministic UI captures.

**Stage / last updated:** Accepted; 2026-10-25.
**Review request:** Replace default typography with supplied Inconsolata for logs
and IBM Plex Sans for controls/labels, decrease size, use separate atlases per
face/size and rounded text draw positions. Delivered: four distinct native-size
atlases, smaller Plex controls/labels and Inconsolata log text, native 1080p text
composition, explicit font-load errors, and matched-font measurement/truncation.
Follow-up: button labels center by actual glyph bounds rather than legacy top
offsets; 15 captured labels have top/bottom ink margins equal within one pixel.
Reverified 14/14 tests in Debug and Release, desktop captures and font cleanup;
114 existing dungeon viewport crops remain unchanged. See [typography design](party_hud_design.md#typography-review-correction-2026-10-25).
Animation follow-up: the interactive demo accidentally held its capture clock at
700 ms. It now advances from that pose; only captures retain fixed time. Default
idle phases are staggered by stable actor slot, with explicit authored offsets
preserved. The UI demo and its captures now select Idle for every actor from an
isolated placement copy. Authored walk/attack showcase loops remain outside the
UI demo; future gameplay must drive them from actual movement/combat.
All 14 Debug/Release suites pass, including phase/lifecycle regressions and the
allocation guard. Live desktop animation and 123 fixed-time captures were checked.
The earlier 114-viewport identity result applies before this correction; side/back
NPC captures now intentionally show staggered idle fallback poses. See
[animation verification](slice04_verification.md#live-demo-animation-review-correction).
Log follow-up: expanded entries now flow according to measured wrapped height
with a small gap, replacing five oversized fixed slots. The panel fits 16 short
entries; long targeted commands wrap cleanly. Debug/Release builds passed;
Release command/allocation checks and desktop wrapping/scroll checks passed.
Only the expanded-log image changed among 122 logical captures.
**Walking review correction:** User requested continuous movement using the existing
walk art. Implemented 600 ms visual travel between committed cells, continuous
projection/shadows/picking, pause/death handling and arrival-gated combat. Both
19-suite configurations pass; final captures and the separate 256-actor CPU
composition measurement are recorded in the verification report. Logical collision
remains discrete; no slice 08 work was introduced.
**Approved plan:** User said “Implement the plan.” See [HUD design](party_hud_design.md).
User chose bottom portraits/sidebar controls, viewport plus list targeting, and
preview commands plus explicit deterministic development scenarios.
**Delivered:** Four formation cards, HP/mana availability/status presentation,
selected character, six action choices, checked command previews, numbered target
list and visible-silhouette picking, 32-entry bounded log with expanded reading,
shared aspect-fit hit testing and focus/edge input gates. Optional renderer picking
reuses prepared alpha/depth composition; no production resources are changed.
--ui-demo supplies eight isolated scenarios; --capture adds eight ui-* images.
**Verification:** Debug/Release builds succeeded without compiler warnings; all
14 CTest suites passed in both, including expanded zero-allocation checks.
Desktop GPU lifecycle passed; final capture run exported 123 PNGs. All 114 existing
logical dungeon viewport crops were pixel-identical to the baseline. UI captures
and desktop mouse/keyboard, held confirm, focus return, map, scenario controls,
letterbox/pillarbox selections were inspected. See [commands/results/limits](slice04_verification.md).
**Review recipe:** Follow [slice 04 user review](slice04_verification.md#user-review),
run --ui-demo, and inspect the new ui-* captures. Confirm previews never spend
resources and held/focus-return input does not duplicate requests.
**Deviations / limitations:** No scope deviation. Expanded log reading is a
readability detail within the bounded log. Portraits are drawn placeholders;
normal mana is unavailable. Sample resources/statuses are development-only. No
combat, initiative, AI, inventory, range/LOS rules, or resource-spending behavior.
Physical high-DPI desktop, Windows/macOS, scale benchmarks, combat playtest and
manual dungeon traversal were not performed. A held control must receive a
focused release before rearming, including when physically released while blurred.
**Next-agent handoff:** Slice 04 is accepted, including typography, alignment,
idle animation, and compact wrapped-log review corrections. Slice 05 is now
eligible for planning in a clean context; inspect the relevant code and present
its own implementation plan before implementation. CommandController borrows
immovable session owners; character IDs remain party-local and actor IDs
world-scoped. Refresh from current
picking/interaction flags before input; call movementStarted for instant and
animated pose changes. PreviewAccepted is presentation validation only, never
combat legality or resolution. Later combat must authoritatively validate range,
LOS, turns and costs. Party/resource snapshots remain const; demo overrides must
never become authoritative data. Picking is null during transitions, stores slot
indices plus world/revision, and excludes dead flash pixels from selection while
letting them occlude living bodies. Log storage is bounded and names are borrowed.
Retain idle-only UI scenario preparation and the synthetic focus-reset regression
when changing input. No general session clock or production RNG was introduced.
Slice 05 remains Planned and unstarted; milestone A remains incomplete until its
combat checkpoint is accepted.
**Acceptance:** User explicitly accepted slice 04 on 2026-10-25: “slice accepted,
close it in the plan document so we can move on to the next slice”. All review
corrections above are included. Recorded scope and verification limitations remain
unchanged; no additional scope changes were requested.

### Slice 05: First melee combat

**Deliver:** Individual initiative, enemy-group turns, melee, defend, formation
protection, rear-row exposure, health/death saves/stabilization, victory, and defeat.
Use stationary encounters and simple feedback to reach a playable checkpoint.

**Boundary:** Shared maneuvers, ranged approach, and full spells come later.
Record these temporary milestone limits rather than implying the full loop exists.

**Acceptance:** A preset party can win or lose a complete goblin fight. Cover
invalid/dead targets, missed attacks, front-row collapse, healing from zero HP,
death-save outcomes, and exactly-once turn/death resolution. User reviews milestone A.

**Stage / last updated:** Accepted and closed; 2026-10-25.
**Walking review correction:** User requested continuous movement using the existing
walk art. Implemented 600 ms visual travel between committed cells, continuous
projection/shadows/picking, pause/death handling and arrival-gated combat. Both
19-suite configurations pass; final captures and the separate 256-actor CPU
composition measurement are recorded in the verification report. Logical collision
remains discrete; no slice 08 work was introduced.
**Approved plan:** User said “Implement the plan.” See [melee combat design](melee_combat_design.md).
Frontal single-cell fights; individual/group initiative; row protection on both
sides; visible-body targeting; Dodge-style Defend and Wisdom Aid; authoritative
health/death saves; immediate defeat; victory stabilization without healing;
restart with fresh owners and seeded reproduction. General back-row step-forward
melee was superseded by the user's clarification; Rogue exceptions stay in slice 10.
**Delivered:** Graphics-independent session/combat ownership and pure health rules;
checked party mutations; turn-token validation and bounded automatic advancement;
production HUD/action routing with ally targeting, status/counters and outcomes.
Review corrections add per-slot exposure, same-cell melee enemy advancement on its
next turn, one-shot attacks on hits/misses, and one-second damage/MISS splashes.
--combat-demo starts beside the goblins; --seed reproduces play; --ui-demo remains
isolated preview-only. Combat captures include real-API recovery fixtures. Existing
prepared-image rendering, fixed presentation, fonts and actor flash behavior remain.
**UI revision delivered:** Dungeon-first layout, right portrait rail, shallow action/log
strip, generated portraits/icons, direct basic attacks and Defend, portrait Aid/details,
hover reach/cost/damage previews, active-first initiative, readable scrolling history,
exclusive paused overlays, and new feedback anchors. Mechanics were accepted by the
user; the user subsequently accepted the revised UI, closing slice 05 and milestone A.
**Verification:** Debug/Release builds and all 17 headless CTest suites passed,
including expanded zero-allocation HUD/gameplay checks. Authorized hidden-window
captures and the GPU lifecycle check passed on Linux/X11/OpenGL. Native, reduced
and letterboxed images were inspected. See [current UI verification](dungeon_ui_verification.md).
Earlier combat/sanitizer/catalog evidence and the invalid desktop automation
attempt remain documented in [historical verification](slice05_verification.md).
No OS input or desktop/window control was used for the UI revision.
**Review recipe:** Run --combat-demo --seed 5 and follow the
[slice 05 user review](slice05_verification.md#user-review): front/rear restrictions,
Defend/Aid, death saves, a complete win/loss and restart. User performs desktop
review; agents must not control mouse/keyboard, windows or desktops.
**Deviations / limitations:** Approved temporary adaptations: frontal contact only,
both sides' paired-slot protection, untrained Wisdom Aid, immediate defeat when no
one is conscious, automatic post-victory stabilization without HP recovery. Healing
has real APIs/development fixtures but no normal action yet. No retreat, moving AI,
shared maneuvers, reach, Rogue exception, ranged/spell attacks, loot/XP, rests or saves.
Physical desktop input, Windows/macOS, physical high-DPI and scale
benchmarks remain unverified; deterministic GPU visual QA passed. The user explicitly prohibited desktop automation.
**Next-agent handoff:** Slice 05 and milestone A are accepted and closed. Slice 06
is now eligible for planning in a clean context; inspect relevant code and present
its implementation plan for user review before implementation. No slice 06 work
was started during this closeout.
Read the [UI design](dungeon_ui_design.md) and [UI handoff](dungeon_ui_verification.md)
before changing presentation; old slice 04 controls/layout are historical.
GameSession is immovable; map/catalog outlive it. Rebuild commands, animator and
frame pipeline after restart. Party snapshots stay const; use checked health APIs.
Combat validates rank/range/turn/life/equipment independently of UI body visibility.
Requests carry actor-world identity and turn serial; invalid requests spend no RNG.
Refresh picking before input and again after mutation; do not confirm a new turn,
newly regained action or result before it has been presented. Damage and animation
share session time; focus/map/log pause it, and automatic steps never catch up in a
burst. Simulation owns outcomes; animation must never resolve damage/death. Slice 06
must preserve immediate dead occupancy release and replace presentation separately.
**Acceptance:** 2026-10-25 — User explicitly accepted the completed slice, including
its combat corrections and dungeon-first UI: “much better, this slice is accepted,
close it in the document”. This closes slice 05 and milestone A — First fight.
Recorded scope and verification limitations remain unchanged; no additional
conditions were requested. Slice 06 remains Planned and has not started.

### Slice 06: Goblin hurt and death animations

**Deliver:** Generate four-direction hurt/death sprite sheets using the existing
reference-based image-generation workflow. Preserve prompts, selected sheets,
registration metadata, and prepared runtime frames. Extend clip definitions and
runtime playback for triggered one-shot attacks, hurt, and death.

**Boundary:** Preserve identity, outfit, dagger handedness, transparent background,
consistent scale, and ground registration. Inspect existing
`assets/npcs/goblin/animation/` and `tools/prepare_npc_animation.cpp` before adapting
them. Standing-sprite grounding, canvas width, and idle fallback assumptions may
not fit a collapsing body; address this deliberately, without per-frame size drift.

**Acceptance:** Hurt returns to idle; death interrupts living behavior, plays once,
holds a brief final corpse pose, then hides. Dead actors immediately stop blocking;
loot lifetime is independent. No direction falls back to a living idle during death.
Check all directions, hidden actors, frame skips, interruptions, corpse grounding,
occlusion, transition views, and prepared memory limits. Visually inspect source
sheets and deterministic in-engine captures; compilation alone is insufficient.

**Stage / last updated:** Accepted and closed; 2026-10-26.
**Walking review correction:** User requested continuous movement using the existing
walk art. Implemented 600 ms visual travel between committed cells, continuous
projection/shadows/picking, pause/death handling and arrival-gated combat. Both
19-suite configurations pass; final captures and the separate 256-actor CPU
composition measurement are recorded in the verification report. Logical collision
remains discrete; no slice 08 work was introduced.
**Approved plan:** User said “Implement the plan.” See [hurt/death design](goblin_reactions_design.md). User selected the existing 100 ms flash overlay, a brief collapse/corpse hold, and preservation of front-only attack art. Four-direction hurt/death presentation preserves authoritative damage, occupancy and session time.
**Delivered:** Eight imagegen sheets and 24 runtime frames with preserved prompts,
references and registration. Hurt plays 150/150 ms; death plays 200/200/200/600 ms,
then hides. Latest living cue wins (damage on ties), death interrupts all behavior,
and canceled attacks never resume. Optional legacy clips, wider death canvases,
explicit source grounding/unequal row splits, dead picking exclusion and living
replacement ordering are implemented. Added `--capture-reactions` diagnostics.
**Review correction:** User reported enemy damage/MISS appearing before the attack.
Markers and portrait hit highlights now begin at the middle attack frame (950 ms
for goblin), with a full one-second marker lifetime after impact. Bounded pending
feedback preserves multiple attacks on one recipient, pauses and cancellation.
Player attacks remain immediate with hurt/flash; HP, logs and turn mechanics are
unchanged. Both 18-suite builds and allocation checks pass; 45 hidden HUD captures
include real hit/MISS guard, wind-up and strike comparisons.
**Verification:** Debug/Release builds and all 18 headless suites passed, including
zero engine-owned C++ frame allocations. Strengthened reaction/asset checks passed
again in both configurations. Hidden GPU lifecycle and 97 focused/252 full captures
passed on Linux/X11/OpenGL; selected native/reduced images were inspected. Combined
NPC pixel storage is 112.98 MiB within the unchanged 128 MiB limit. All 44 runtime
frames and the manifest reproduce byte-for-byte; original 20 PNGs remain unchanged.
Local documentation links and `git diff --check` passed. See [verification](slice06_verification.md).
**Front-slot correction (2026-10-26):** A protected rear enemy may advance into
the opposite vacant front slot and attack on its turn, preferring straight ahead
when available. Current target protection stays paired. Turn preview predicts
vacancy claims without moving actors, and execution rechecks occupancy. All 18
suites pass in Debug/Release, including allocation coverage of diagonal advances;
51 hidden HUD captures include inspected before/after examples. No slice 07 work.
**Review recipe:** Run `--combat-demo --seed 5` and follow the
[slice 06 review](slice06_verification.md#user-review). Use `--capture-reactions DIRECTORY`
for all four directions, timings, transitions, replacement overlap and outcomes.
**Deviations / limitations:** No gameplay/scope deviation. Generated sheets needed
explicit non-midpoint row splits; the left hurt dagger hand was corrected through
imagegen. Physical interactive review, Windows/macOS, high-DPI hardware and scale
benchmarks remain unverified. The existing victory overlay can obscure part of the
last corpse but does not freeze its clock. No OS input/window/desktop automation.
No moving AI, new attack directions, loot or slice 07 work.
**Next-agent handoff:** Slice 06 is accepted and closed, including attack-impact
timing and diagonal front-slot advancement. Slice 07 is next eligible but remains
Planned; inspect relevant code and prepare its plan in a new planning task before
implementation. Enemy turn eligibility must use reachable front
slots, separately from current target exposure. Preview reserves predicted slots
in a local mask; only execution may perform the checked ActorWorld move.
Production HUD controllers must receive prepared NPC timings; pending impact
feedback must remain on paused session time, separate from authoritative outcomes.
Retain ActorWorld ownership and immediate
occupancy release; animation never resolves death, removes records or owns loot.
Sample after mutations on paused session time. Death/hurt clips require all four
views and cannot be placement loops or use idle direction fallback. Old assets
retain flash-only compatibility. Rebuild session borrowers on restart. Keep the
128 MiB budget and 1..4-frame limit; do not resize individual generated poses.
See the detailed design/verification for reproduction and capture commands.
**Acceptance:** Accepted on 2026-10-26. The user said: “I accept the slice, finish it in the document”.
Acceptance covers the delivered hurt/death animations and both review corrections
(attack-impact feedback and diagonal front-slot advancement). No additional
conditions or new work were requested. Recorded verification limitations remain
as documented; acceptance does not turn unexecuted platform checks into passes.
Slice 07 remains Planned and has not started.

### Slice 07: Moving goblin AI

**Deliver:** Awareness state, idle/patrol behavior, pursuit, facing, bounded
pathfinding, legal subslot occupancy, and basic combat decisions. Drive walking
and attack presentation from actual behavior.

**Boundary:** Use discrete world movement and practical goblin behavior, not a
general behavior-tree framework. Record the Nimble Escape adaptation/defer choice.

**Acceptance:** Representative maps exercise corridors, turns, blocked routes,
multiple goblins, contention, and unreachable targets. Actors cannot cross walls,
enter the party cell, or overlap occupied subslots. Pathfinding work/storage is
bounded and does not allocate in normal simulation updates.

**Stage / last updated:** Accepted and closed; 2026-10-26.
**Review correction:** User reported being able to attack while an enemy's attack
was still playing. Implemented a session-time presentation hold for enemy/player
attacks: no next turn or result overlay before animation/reaction and damage/MISS
expiry. Goblin hold is 1950 ms. HP/death still resolve once immediately. Debug and
Release all 19 suites, allocation checks and 59 HUD captures passed; inspected
strike, feedback-tail and handoff images. See the AI design and verification.
**Walking review correction:** User requested continuous movement using the existing
walk art. Implemented 600 ms visual travel between committed cells, continuous
projection/shadows/picking, pause/death handling and arrival-gated combat. Both
19-suite configurations pass; final captures and the separate 256-actor CPU
composition measurement are recorded in the verification report. Logical collision
remains discrete; no slice 08 work was introduced.
**Approved plan:** User said “Implement the plan.” See [AI design](goblin_ai_design.md).
Guards, waypoint patrols and wandering; directional sight/memory; deliberate
600/1200 ms movement; side/rear waiting; frozen roaming during combat/results;
dedicated AI demo; Nimble Escape deferred to slice 08.
**Delivered:** Session-owned bounded AI/navigation, validated optional placement
settings, checked movement/facing/walk cues, independent RNG, --ai-demo and
--capture-ai. Existing stationary review modes, combat outcomes, impact timing,
front-slot correction and prepared rendering remain intact.
**Verification:** Debug/Release builds and all 19 headless suites passed, including
extended zero-allocation checks. Focused ASan/UBSan passed with leak detection
disabled due to the environment's ptrace limitation. Hidden GPU lifecycle and
40 AI captures passed; selected native images inspected. Separate 256-request
stress runs respected work/storage limits. See [commands, measurements and review](slice07_verification.md).
**Review recipe:** Run --ai-demo --seed 5; observe routines, provoke directional
pursuit, break sight around the wall, test pauses and face adjacent pursuers to
enter combat. The verification report gives coordinates and capture alternatives.
**Deviations / limitations:** No scope change. Bounded shared BFS can produce long
queues at extreme map/actor scale; maximum-map stress is capacity evidence, not
responsive gameplay evidence. Physical interactive review, Windows/macOS, physical
high-DPI and LeakSanitizer remain unverified. No OS input/focus automation.
**Next-agent handoff:** Slice 07 is accepted and closed, including both review
corrections. Slice 08 is next eligible but remains Planned; inspect the relevant
code and prepare its plan in a new planning task before implementation.
Production GameSession and HUD
must receive matching prepared timings. CombatSnapshot::resolvingAction retains
the old participant/serial with awaitingAction=false; update releases it at the
deadline and never executes a second action in that same call. Preserve presented-
turn confirmation gating and delayed terminal overlays. Preserve the explicit exploration
AI clock and separate combat RNG; navigation results are advisory, ActorWorld
revalidates each move. Keep travel offsets in animation invalidation, and keep
contact awaiting all living members' arrival before initiative. Last-seen cells still occupied by the party use adjacent
approach and facing, preventing self-turn sight loss from stalling pursuit.
Runtime AI copies optional placement routines; diagnostic animation loops remain
separate. Restart rebuilds every borrower. Keep immediate death occupancy release,
reaction priority, impact timing and front-slot advancement. Slice 08 must decide
multidirectional membership, reinforcements, phase clocks and Nimble Escape.
**Acceptance:** Accepted on 2026-10-26. The user said: “slice accepted, update the document”.
Acceptance covers moving goblin AI and both review corrections: attack-turn
sequencing and interpolated cross-cell walking with arrival-gated combat. No
additional conditions or new work were requested. Recorded verification and
scale limitations remain documented; acceptance does not turn unexecuted checks
into passes. Milestone B remains open; slice 08 remains Planned and has not started.

### Slice 08: Hybrid encounter loop

**Slice 07 handoff:** Nimble Escape is explicitly inactive in slice 07; decide its adaptation here alongside disengagement and awareness-based encounters.

**Deliver:** Real-time ranged weapon approach/cooldowns, adjacency combat entry,
automatic facing between front groups, NPC awareness/surprise, reinforcements,
and disengagement when adjacency ends. Define the clock/phase policy later spells
will share. The approved review correction removes maneuvers, retreat and party surprise.

**Boundary:** Plan encounter membership, perception, line of sight, range, misses,
turn ordering, and multi-direction threats before implementation. Spell casting
plugs into these rules in slice 10. Pause must not recharge combat opportunistically.

**Acceptance:** Exercise approach-to-contact, front persistence, side/rear automatic
facing, pending arrivals, partial-round re-entry, focus/menu pauses and mode transitions with no
duplicate actions or reset costs. User reviews milestone B and overall combat feel.

**Stage / last updated:** Accepted; 2026-10-26.
**Approved plan:** User said “Implement the plan.” See [hybrid encounter design](hybrid_encounter_design.md).
**Delivered:** Shared NPC AI/typed trait, ranged approach and bow clips, front-focused contact/automatic facing, fixed protection, NPC surprise, reinforcements, persistent clocks/costs and direct action controls. Single-weapon AI defaults and two bow-only demo goblins share the existing art; mixed-weapon behavior remains unchanged. The three-cell melee front includes diagonal contact, front-band routing and inward edge presentation.
**Verification:** Debug and Release 20/20; focused ASan/UBSan, zero-allocation frame guard, separate scale checks, hidden GPU smoke and hybrid/HUD captures. See [verification and handoff](slice08_verification.md).
**Review recipe:** Run `cmake-build-debug/blobber --hybrid-demo --seed 5`; select Merrin, press 3 and click a goblin. Review direct attacks and automatic side/rear facing with preserved actions. Full recipe and capture locations are in the handoff.
**Deviations / limitations:** Approved project adaptations are recorded in the design and SRD ledger. The user accepted the delivered combat feel and presentation; further graphical work is a separate discussion. Generated-fixture stress does not establish shipped-art performance at maximum population. LeakSanitizer was not run.
**Next-agent handoff:** Slice 08 and milestone B are closed. Preserve travel/impact/turn holds and the 160 MiB cap. Discuss the user’s graphical changes before planning or starting slice 09.
**Acceptance:** Accepted on 2026-10-26: “yes I accept this slice so close it”. This closes milestone B, including all slice 08 review corrections. The user requested a graphics discussion before the next slice.

### Slice 09: Effects and concentration

**Deliver:** Bounded effect lifecycle for the planned curated spells/features:
durations, expiry, saves, concentration, stacking rules where needed, and cleanup.

**Boundary:** First identify concrete consumers in slices 10 and 13. Avoid a
universal effect language or implementation of unused conditions.

**Acceptance:** Effects behave across real-time/turn phases, pauses, unconsciousness,
death, and encounter completion. Test interruption, replacement, expiry boundaries,
invalid targets, capacity exhaustion, and allocation behavior.

**Execution record:** Not started; no verification or acceptance recorded.

### Slice 10: Spells and class identity

**Deliver:** Mana, curated cantrips/first-level spells, spellbook and target previews,
and explicit level-1 signature-feature adaptations for all four classes. Define
resource and targeting rules in the content ledger, including incompatible omissions.

**Boundary:** Use the effect primitives and phase clocks already implemented.
Second-level spells and higher class features arrive with progression. Spell
selection must demonstrate distinct class roles, not just differently named damage.

**Acceptance:** All four classes have useful distinct actions. Cover attacks/saves,
mana and usage limits, legal/illegal targeting, concentration, durations, healing,
tile areas, pause behavior, and spell use in both approach and turn-based phases.

**Execution record:** Not started; no verification or acceptance recorded.

### Slice 11: Inventory and equipment

**Deliver:** Large per-character equal-slot grids, equipment-slot paper dolls,
descriptions, weight, stack merge/split, transfers, ammunition, and consumables.
Route all operations through authoritative inventory/equipment transactions.

**Boundary:** No item-size packing puzzle or layered clothing art. Define exact
capacities, stack eligibility/limits, equipment compatibility, and full-bag behavior
in this slice's plan. Full-screen browsing pauses; equipment/transfers are out of combat.

**Acceptance:** Operations conserve items and quantities, reject over-capacity or
invalid changes without partial mutation, and recalculate derived stats correctly.
Test full bags, weight limits, equipped-item ownership, split/merge boundaries,
consumption/ammunition costs, and combat restrictions. Inspect the large-screen UI.

**Execution record:** Not started; no verification or acceptance recorded.

### Slice 12: Loot and recovery

**Deliver:** Bounded loot ownership and collection, supplies/consumable acquisition,
Hit Dice short rests, feature recovery, safe-location long rests, and camp supplies.

**Boundary:** No shops or quest system. Define safety, resource quantities, corpse
independence, rest eligibility, and full-inventory handling explicitly.

**Acceptance:** Several encounters can be played with meaningful recovery decisions.
Test duplicate collection, full inventories, resource depletion, repeated rest,
threat restrictions, dead characters, and exactly-once resource consumption.

**Execution record:** Not started; no verification or acceptance recorded.

### Slice 13: Levels 2 and 3

**Deliver:** XP awards, normal advancement, preset class/subclass choices, resources,
supported higher features, and curated second-level spells. Add development-only
next-level XP grants for every party member, capped to supported progression.

**Boundary:** Use normal XP mechanics, not custom objective scripts or milestone
leveling. Freeze the supported feature/spell list in the plan and ledger; do not
pretend unsupported features are functioning. Character creation remains deferred.

**Acceptance:** Each preset reaches level 3 through the normal advancement flow.
Test thresholds, repeat requests, exactly-once XP rewards and features, derived
statistics/resources, and new spell behavior. User reviews the complete milestone C loop.

**Execution record:** Not started; no verification or acceptance recorded.

### Slice 14: Save and resume

**Deliver:** One versioned out-of-combat save, validated restoration, safe file
replacement, and useful load/save failure reporting. Save authoritative gameplay
state and reconstruct derived data and presentation resources.

**Boundary:** No mid-fight or active-approach saves, autosave framework, or historical
version migration promise. Define unsupported-version behavior in the save plan.

**Acceptance:** Round-trip party position, builds, equipment, quantities, resources,
progression, and persistent monster/loot state. Test corrupt/truncated files,
unknown content, unsupported versions, failed writes, and invalid ownership.
Failures must not partially replace the running session or destroy a valid save.

**Execution record:** Not started; no verification or acceptance recorded.

### Slice 15: Cohesive presentation

**Deliver:** Final four portraits, curated item/spell icons, paper-doll silhouette,
readable management screens, targeting, damage/status feedback, and coherent art.
Review a concrete layout/art sample with the user before producing the whole set.

**Boundary:** Preserve the existing fantasy aesthetic and fixed presentation path.
No dressed-body equipment layers or renderer rewrite. Maintain UI/gameplay separation.

**Acceptance:** Inspect exploration, combat, inventory, equipment, spellbook,
rest, advancement, defeat, and save/load flows. Check scaling, input coordinates,
overlays, legibility, focus, no-clipping, and generated-art consistency through
deterministic captures and desktop interaction where available.

**Execution record:** Not started; no verification or acceptance recorded.

### Slice 16: Demo integration

**Deliver:** Configure encounters and recovery resources in the existing test
level, tune pacing, finish player-facing documentation, and verify the integrated demo.

**Boundary:** Do not add campaign infrastructure or broaden the agreed content
scope to fix pacing. Use the development XP control to inspect later levels.

**Acceptance:** A fresh session supports exploration, ranged approach, moving
goblin combat, looting, equipment, casting, recovery, progression, saving, and
resuming. Exercise defeat and restart/load. Run relevant Debug/Release tests,
allocation checks, and available desktop/capture checks. Record limits and obtain
user acceptance of milestone D before declaring the demo complete.

**Execution record:** Not started; no verification or acceptance recorded.

## Validation and deferred work

Tests must own deterministic generated or small dedicated fixtures, not depend on
mutable game artwork/content. Use controlled RNG and simulation time. Prefer
observable behavior and invariants over assertions repeating implementation details.

Cover interactions as they appear: stale targets, multiple deaths, occupancy and
turn queues, effect expiry, full inventories, rest/reward duplication, save failure,
and real-time/turn-based boundaries. Extend allocation guards to new engine-owned
simulation and presentation paths after their storage is prepared. Larger stress
checks belong outside routine correctness tests.

Run focused checks per slice and Debug/Release suites as required by the project.
Keep CPU checks headless. Run GPU lifecycle, input/presentation, animation, and
deterministic visual checks separately when a desktop is available. Report what
was actually executed; absence of hardware is not a test pass.

Deferred beyond this roadmap: character creation, additional race/class content,
levels beyond 3, multiclassing, a general build editor, layered clothing artwork,
shops, campaigns/quests, procedural levels, multiplayer, and mid-combat saves.
Support future expansion through concrete data boundaries, not by implementing
these systems speculatively now.

## Decision log

Add dated entries when the user changes direction. Preserve superseded decisions
with a replacement reference instead of deleting the history. Technical slice
choices belong in their execution/design records unless they change this roadmap.

| Date | Decision | Basis |
| --- | --- | --- |
| 2026-10-25 | Slice 05 uses frontal single-cell contact, Dodge-style Defend, Wisdom Aid, conscious-target priority, immediate defeat and post-victory stabilization without HP recovery. | User's slice 05 planning selections. |
| 2026-10-25 | Original planning decision (superseded by the slice 05 review correction below): whole-row protection applies to attackers and targets on both sides; body visibility is also required. Ordinary step-forward melee is superseded; Rogue exception remains slice 10. | User rejected melee over front-row heads and clarified both formations. |
| 2026-10-25 | Agents must not control mouse/keyboard, switch windows/desktops or automate desktop input. | Explicit user instruction following the invalid desktop test; recorded in AGENTS.md. |
| 2026-10-25 | Deliver a goblin demo with four classes/races and levels 1–3; generate a balanced fixed party. | User planning answers; proposed preset pairings retained in approved roadmap. |
| 2026-10-25 | Individual initiative turns, real-time ranged approach with cooldowns, and one shared maneuver each round. | User planning answers. |
| 2026-10-25 | Keep death saves; use awareness-based surprise and rear-row exposure. | User planning answers. |
| 2026-10-25 | Adapt signature features while omitting bonus actions/reactions and other listed tabletop mechanics. | User planning answer and source brief. |
| 2026-10-25 | Large per-character equal-slot grids, no inventory Tetris, Strength-based weight, and usable stack merging/splitting in the demo. | User clarification and planning answer. |
| 2026-10-25 | Pause while browsing; long rests at safe locations or with supplies; one out-of-combat save. | User planning answers. |
| 2026-10-25 | Use ordinary XP plus a development next-level XP button; no custom objective system required. | User refinement superseding the earlier objective-award suggestion. |
| 2026-10-25 | Cohesive portraits/icons and simple paper-doll artwork; add goblin hurt/death animations through the existing generation workflow. | User planning answer and explicit animation request. |
| 2026-10-25 | Sixteen slices, four milestones, durable handoffs, and user acceptance between slices; no full-auto execution. | User requested self-tracking workflow and approved its documentation implementation. |

Initial documentation verification (2026-10-25): a Python standard-library check
passed for local Markdown links/heading anchors in this file and the system
overview, all sixteen sequential tracker IDs and dependencies, milestone groups,
Planned statuses, empty execution records, and trailing whitespace. No build,
gameplay, or desktop tests were run for this documentation-only change. No gameplay
slice completion is implied.


Superseded slice 05 planning assumption: the earlier roadmap allowed any back-row
character to step forward for a melee action without changing formation. The
2026-10-25 row-protection clarification above replaces it; no such ordinary action
is implemented. The dungeon-first dock omits the unsupported Back-row melee control.

### Slice 05 review correction decision — 2026-10-25

User requests attacks visible on misses, paired-slot protection, melee enemies
advancing into vacant front slots, and temporary red damage/white MISS splashes
on monsters and party portraits. These supersede whole-row protection and the
attack-playback deferral above. Implemented and accepted within slice 05
on 2026-10-25. Same-cell advancement occurs on the enemy's next attack turn; general
moving AI and new hurt/death assets remain future work. Details and validation
strategy: [melee combat review corrections](melee_combat_design.md#review-corrections--design-2026-10-25).

## Approved dungeon-first UI revision

User accepted mechanics/functionality and approved [this UI plan](dungeon_ui_design.md).
Implementation accepted and closed with slice 05 and milestone A on 2026-10-25;
no slice 06 work. The user said: “much better, this slice is accepted, close it in the document”.
Seventeen Debug/Release suites, extended allocation guard, hidden GPU lifecycle
check and final native/reduced captures passed. See [verification and review](dungeon_ui_verification.md). Hidden application-owned
captures are authorized; OS mouse/keyboard/focus automation remains prohibited.

Review decision (2026-10-26): user approved front-focused combat, automatic facing,
normal party initiative and no retreat. This supersedes the 2026-10-25 shared
maneuver and rear-exposure decisions. Implemented within slice 08; acceptance pending.

Review decision (2026-10-26): user authorized general melee-only/ranged-only AI
rules, preserving the existing mixed-weapon goblin policy, and requested two
bow-only goblins in the hybrid demo. Implemented and verified within slice 08.
Random preferences, richer mixed-weapon roles and caster decisions remain deferred.

Review correction (2026-10-26): reproduced the healthy goblin's route onto the
right flank in the hybrid demo. Front-side pursuit now prefers the front tile,
with bounded fallback for unavailable routes. Distant living melee targets report
out of range. User explicitly requested leaving the initiative strip unchanged;
its remaining-turn preview is preserved. Verified within slice 08; acceptance pending.

Review decision (2026-10-26): user approved the three-cell melee front, diagonal
contact and close-range archer behavior, and AI preference for diagonal front cells
when the center is full. After shipped-art checks found two hidden edge slots,
user approved drawing the front row inward without changing camera/FOV or grid
positions. Implemented within slice 08; human review remains pending.

Acceptance (2026-10-26): user explicitly accepted and closed slice 08, including
its review corrections, completing milestone B. This supersedes the pending-review
notes above. Slice 09 is eligible but remains Planned and unstarted; discuss the
user’s graphical work before advancing.
