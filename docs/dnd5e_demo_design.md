# SRD 5.1 goblin demo: design and delivery roadmap

Status: gameplay roadmap; slice 01 is Accepted; slice 02 is eligible for planning.
Created: 2026-10-25. Execution records below distinguish implementation from
user acceptance.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Latest accepted slice | 01 — Rules foundation and content (2026-10-25) |
| Active slice | None |
| Awaiting user review | None |
| Blockers | None identified for planning slice 02 |
| Next eligible slice | 02 — Characters and preset party |
| Next action | In a clean context, read slice 01's handoff, inspect relevant code, and propose slice 02's implementation plan |

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
- Immutable `NpcWorld` placement/occupancy data, currently bounded to 32 types
  and 256 instances. There is no live movement, health, AI, or removal.
- Idle, walk-in-place, and attack visual loops. `NpcAnimator` uses an absolute
  clock; the frame pipeline observes movement and animation revisions.
- Existing CPU correctness/allocation tests and separate desktop GPU checks.

Relevant entry points are `src/world/npc.*`, `src/game/movement.*`,
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

Mana, cell-based areas, shared maneuvers, formation protection, and the other
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
- At each round's beginning, choose one shared party step, turn, or hold before
  individual turns. A surprised opening suppresses the opening maneuver and
  affected characters' first turns. Surprise requires unnoticed attackers;
  side/rear position alone does not cause it.
- The front row protects the back row from ordinary frontal melee until both
  front characters are unconscious/dead. Rear attacks expose the back row.
  Back-row step-forward melee uses the character's action without permanently
  changing formation. The encounter plan must resolve side attacks and threats
  from multiple directions consistently with these rules.
- Include melee, ranged attack, spell, item, defend, and back-row melee commands.
  Omit bonus actions, reactions, readying, opportunity attacks, and tabletop
  movement distances. Explicitly adapt signature features instead of silently
  deleting class identity; Second Wind using an action and a formation-aware
  Sneak Attack trigger are design starting points for the class-feature plan.
- Include unconsciousness, death saves, stabilization, healing, and death. Party
  defeat offers restart, and loading once persistence exists. Rest does not
  resurrect dead characters.

Pause/menu transitions, cooldowns, combat entry/exit, retreat, and reinforcement
entry must share coherent timing rules. A phase change cannot reset costs or
grant duplicate attacks. The slice 08 plan must define encounter membership,
disengagement, maneuver collision, and timing before implementation.

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
| 02 | [Characters and preset party](#slice-02-characters-and-preset-party) | 01 | Planned | A |
| 03 | [Mutable monster world](#slice-03-mutable-monster-world) | 02 | Planned | A |
| 04 | [Party HUD and commands](#slice-04-party-hud-and-commands) | 03 | Planned | A |
| 05 | [First melee combat](#slice-05-first-melee-combat) | 04 | Planned | A |
| 06 | [Goblin hurt/death animations](#slice-06-goblin-hurt-and-death-animations) | 05 | Planned | B |
| 07 | [Moving goblin AI](#slice-07-moving-goblin-ai) | 06 | Planned | B |
| 08 | [Hybrid encounter loop](#slice-08-hybrid-encounter-loop) | 07 | Planned | B |
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
| A — First fight | A preset party can fight stationary goblins, win, or lose through actual combat rules. | Planned |
| B — Dungeon skirmishes | Goblins approach, move, attack, react to hits, and die through the hybrid encounter loop. | Planned |
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

**Execution record:** Not started; no verification or acceptance recorded.

### Slice 03: Mutable monster world

**Deliver:** Runtime actors initialized from placements, with health, position,
facing, lifecycle, and atomic occupancy updates. Define identity/lifetime and
connect actor revisions to movement collision and frame invalidation.

**Boundary:** No autonomous AI yet. Preserve existing asset formats where possible
and document any compatibility change. Prepared appearances remain immutable.

**Acceptance:** Controlled actor moves/removals update stationary views and collision;
blocked moves, occupied slots, stale references, and resource bounds are handled.
Existing transitions and no-allocation checks still work.

**Execution record:** Not started; no verification or acceptance recorded.

### Slice 04: Party HUD and commands

**Deliver:** Four portrait panels, HP/mana/status display, active character,
action selection, targeting, and bounded combat log. Use the command/snapshot
boundary rather than allowing UI widgets to mutate rules state directly.

**Boundary:** Functional placeholder art is acceptable until slice 15. Exercise
command flows with deterministic development scenarios; initiative arrives next.

**Acceptance:** Legal selections are clear; invalid/unavailable commands explain
why and spend no resources. Check focus loss, held input, target changes, and
aspect-fit input coordinates. Inspect deterministic UI captures.

**Execution record:** Not started; no verification or acceptance recorded.

### Slice 05: First melee combat

**Deliver:** Individual initiative, enemy-group turns, melee, defend, formation
protection, back-row melee, health/death saves/stabilization, victory, and defeat.
Use stationary encounters and simple feedback to reach a playable checkpoint.

**Boundary:** Shared maneuvers, ranged approach, and full spells come later.
Record these temporary milestone limits rather than implying the full loop exists.

**Acceptance:** A preset party can win or lose a complete goblin fight. Cover
invalid/dead targets, missed attacks, front-row collapse, healing from zero HP,
death-save outcomes, and exactly-once turn/death resolution. User reviews milestone A.

**Execution record:** Not started; no verification or acceptance recorded.

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

**Execution record:** Not started; no verification or acceptance recorded.

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

**Execution record:** Not started; no verification or acceptance recorded.

### Slice 08: Hybrid encounter loop

**Deliver:** Real-time ranged weapon approach/cooldowns, adjacency combat entry,
shared round maneuvers, awareness-based ambushes, rear exposure, reinforcements,
retreat, and disengagement. Define the clock/phase policy later spells will share.

**Boundary:** Plan encounter membership, perception, line of sight, range, misses,
turn ordering, and multi-direction threats before implementation. Spell casting
plugs into these rules in slice 10. Pause must not recharge combat opportunistically.

**Acceptance:** Exercise approach-to-contact, blocked maneuvers, side/rear contact,
new arrivals, retreat/re-entry, focus/menu pauses, and mode transitions with no
duplicate actions or reset costs. User reviews milestone B and overall combat feel.

**Execution record:** Not started; no verification or acceptance recorded.

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
