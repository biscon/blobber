# SRD 5.1 goblin demo: design and delivery roadmap

**Closed / superseded delivery roadmap — 2026-10-30.** The user ended the
static-demo workstream in favor of a procedural crawler. Accepted slices through
13.3 and milestones A/B/C remain accepted. This does **not** claim completion of
unimplemented slices 14–16 or milestone D. See the [closeout](#roadmap-closeout-2026-10-30)
and the successor [procedural campaign roadmap](procedural_campaign_plan.md).
The detailed scope and execution history below are retained as historical records.

Historical delivery status: slices 01–08 and milestones A/B are Accepted. Slice 08 was accepted on 2026-10-26, including the three-cell melee front and edge-visibility correction. Graphical slice 8.1 was accepted and closed on 2026-10-26; 8.2 was accepted and closed on 2026-10-26; 8.3 was accepted and closed on 2026-10-27, including the front-cell spacing correction. Slice 09 was accepted and closed on 2026-10-27. Slice 10 was accepted and closed on 2026-10-27, including the spellcasting UI, native-resolution HUD and impact-triggered alerts. Slice 11 was accepted and closed on 2026-10-27, including inventory UI corrections and the tabbed Character sheet. Slice 12 was accepted and closed on 2026-10-27, including loot bags, Inventory Drop, Take All auto-close and the top-bar Rest icon. The character-creation visual proposal and expanded spell catalog were accepted on 2026-10-28. Slices 12.1–12.4 are inserted before slice 13; slice 12.1 was accepted and closed on 2026-10-29, including all visual and portrait follow-ups. Slice 12.2 was accepted and closed on 2026-10-29. Slice 12.3 was accepted and closed on 2026-10-29, including the formation dragging and search-caret corrections. Slice 12.4 was accepted and closed on 2026-10-29. The user approved arena slice 12.5 before slice 13; 12.5 was accepted and closed on 2026-10-29. Slice 13 and milestone C were accepted and closed on 2026-10-30, including the development-flag and spell-picker review corrections.
Slices 13.1, 13.2 and 13.3 were accepted and closed on 2026-10-30.
Created: 2026-10-25. Execution records below distinguish implementation from
user acceptance.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Latest accepted slice | 13.3 — MCP play and investigation (2026-10-30); slice 13 and milestone C remain Accepted |
| Active slice | None |
| Awaiting user review | None |
| Blockers | None |
| Next eligible slice | None in this closed roadmap; successor PC01 is eligible for detailed planning under the [accepted campaign roadmap](procedural_campaign_plan.md) |
| Next action | Fresh context plans campaign PC01 only for review; do not start demo slice 14, 15 or 16 |
| Accepted design | Slices 13.1–13.3 implemented and accepted, 2026-10-30; [13.3 design](mcp_design.md) and [closeout/handoff](slice133_verification.md#accepted-closeout-and-next-agent-handoff) |

Update this checkpoint, the tracker, and the relevant execution record together.
Do not infer completion or user acceptance from code being present.

## Roadmap closeout (2026-10-30)

The user considers the presentation polish sufficient and no longer wants a separate
slice 15. Improving/tuning the static dungeon in slice 16 would be discarded by the
planned procedural crawler. Close those tasks as retired, not implemented/accepted.
Keep the save/resume requirement, but implement it only after generated floors and
persistent travel establish the actual campaign state.

The user also explicitly authorizes replacing the existing static-map New Game
mode. Future campaign slices may remove its obsolete launch path and exclusive
content/code/tests; no legacy mode or compatibility option is required. Accepted
history remains recorded, while shared engine systems and arena tooling continue
into the new campaign. This closeout does not itself remove gameplay code.

| Old work | Disposition | Successor |
| --- | --- | --- |
| 14 — Save/resume | Superseded; never implemented under this slice | [PC06 — Campaign save/resume](procedural_campaign_plan.md#pc06-campaign-save-and-resume), after PC01–PC05 |
| 15 — Cohesive presentation | Retired by user; existing polish is sufficient for moving on | Only feature-specific presentation in campaign slices; no replacement polish project |
| 16 — Demo integration | Retired; static-level tuning no longer desired | [PC09](procedural_campaign_plan.md#pc09-integrated-expedition-and-mvp-acceptance) verifies the new procedural campaign, not the old static demo |
| D — Finished demo | Retired as a delivery milestone; not accepted/completed | New campaign milestones P-A/P-B/P-C |

The long-term direction is an authored town with shops over a procedural dungeon
and a lowest-floor boss. The [MVP spec](procedural_campaign_mvp.md) deliberately
limits the first milestone; shops remain later work. The new roadmap also records
agreed paid recall/entry return, optional walking ascent, no respawns and finite
optional encounters as an XP/resource fallback. All accepted gameplay/tooling
contracts and verification limits remain intact. No gameplay implementation or
new verification is implied by this documentation closeout.

Current authority for future delivery is the [campaign roadmap](procedural_campaign_plan.md),
accepted by the user on 2026-10-30. Its next step is detailed PC01 planning in a fresh
context; no campaign implementation has started.
Older future-tense references to slice 14 below or in dated handoffs describe the
previous sequence and are superseded by this closeout. Do not reopen accepted
slices or follow an old next-agent prompt to start 14 automatically.

## How agents must use this document

The workflow below is retained as the historical delivery process for the demo;
use the successor campaign roadmap's checkpoint and workflow for new work. This
was the authoritative delivery roadmap and cross-context handoff for the demo. [System design](system_design.md) describes implemented architecture;
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
renumbering completed history. The user explicitly chose decimal IDs 8.1–8.3
for the graphical interlude after slice 08 and 12.1–12.4 for character creation and expanded spells before slice 13. Record approved dependency and milestone changes.
Do not quietly push unfinished requirements into another slice.

### Status meanings

| Status | Meaning |
| --- | --- |
| Planned | Work has not started. |
| In progress | Planning, implementation, or review corrections are underway. |
| Blocked | A named decision or prerequisite prevents further progress; record what can unblock it. |
| Ready for review | Implementation and required available checks are complete; user acceptance is pending. |
| Accepted | The user accepted the result and any recorded limitations. |
| Superseded | Unimplemented work transferred to the successor roadmap; not accepted as delivered. |
| Retired | User removed remaining work or a milestone from scope; not a claim of implementation. |

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

The completed demo supports creating and storing characters, choosing a four-person
party from the main menu, exploring the existing test level, fighting moving
goblins, progressing through level 3, and saving
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
- Offer the balanced preset from slice 02 as the first-run library and explicit
  default-party shortcut: Human Fighter, Dwarf Cleric, Halfling Rogue, Elf Wizard.
  Slices 12.1–12.4 add persisted custom starting builds and party selection at
  Start new game. Race and class remain independent; preset pairings are not
  restrictions. The active party still contains exactly four characters.
- Support levels 1–3 with regular XP thresholds and monster awards. Add a clearly
  development-only button granting each character enough XP for their next level;
  it must use normal advancement rather than directly overwriting derived stats.
- Slice 13 advances both presets and custom characters through the shared 12.4
  selection/review flow. Its plan selects supported SRD subclasses and records
  adapted/deferred features. General respec/build editing remains future work;
  character creation is now in scope through the accepted 12.x interlude.

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
- Keep the current fantasy aesthetic. Deliver twenty portraits (five per supported
  race, retaining the existing four), item/spell icons,
  a paper-doll silhouette with slot icons, readable status/action displays, and
  targeting/damage feedback. Layered illustrated clothing is not required.

### Recovery, loot, and saving

Short rests spend Hit Dice and refresh supported features outside danger. Long
rests require either a designated safe location or camp supplies and cannot occur
during an active threat. Define resource amounts and safety checks in slice 12.
Loot must have explicit ownership independent of a monster's visual corpse.
Monsters drop visible items onto the floor when they die; clicking a floor item
collects it into the active character's inventory. The slice 12 record defines
this approved interaction and the remaining pickup design decisions.

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
| Library and creation | Owns persistent level-one starting builds and isolated drafts; pure validation supplies previews; library instances never own live party state. |
| Persistence | Library storage arrives in 12.2; slice 14 serializes independent gameplay state using stable content IDs and validates restoration. Neither stores GPU/prepared-image resources. |

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

**Historical sequence, now closed:** do not execute the remaining retired/superseded rows.
During delivery, each slice required all preceding slices to be Accepted,
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
| 8.1 | [Image preparation and memory](#slice-81-image-preparation-and-memory) | 08 | Accepted | B follow-up |
| 8.2 | [Distinct goblin archer artwork and loadouts](#slice-82-distinct-goblin-archer-artwork-and-loadouts) | 8.1 | Accepted | B follow-up |
| 8.3 | [Projectiles and impact-time consequences](#slice-83-projectiles-and-impact-time-consequences) | 8.2 | Accepted | B follow-up |
| 09 | [Effects and concentration](#slice-09-effects-and-concentration) | 8.3 | Accepted | C |
| 10 | [Spells and class identity](#slice-10-spells-and-class-identity) | 09 | Accepted | C |
| 11 | [Inventory and equipment](#slice-11-inventory-and-equipment) | 10 | Accepted | C |
| 12 | [Loot and recovery](#slice-12-loot-and-recovery) | 11 | Accepted | C |
| 12.1 | [Expanded level-one spell catalog](#slice-121-expanded-level-one-spell-catalog) | 12 | Accepted | C interlude |
| 12.2 | [Character builds, library storage and portraits](#slice-122-character-builds-library-storage-and-portraits) | 12.1 | Accepted | C interlude |
| 12.3 | [Main menu, character library and party launch](#slice-123-main-menu-character-library-and-party-launch) | 12.2 | Accepted | C interlude |
| 12.4 | [Complete creator and reusable selection flow](#slice-124-complete-creator-and-reusable-selection-flow) | 12.3 | Accepted | C interlude |
| 12.5 | [Arena](arena_design.md) | 12.4 | Accepted | C interlude |
| 13 | [Levels 2–3](#slice-13-levels-2-and-3) | 12.5 | Accepted | C |
| 13.1 | [Headless arena sessions and reproducible play](automation_plan.md#slice-131-headless-arena-sessions-and-reproducible-play) | 13 | Accepted | Tooling interlude |
| 13.2 | [Automated players and balance experiments](automation_plan.md#slice-132-automated-players-and-balance-experiments) | 13.1 | Accepted | Tooling interlude |
| 13.3 | [MCP play and investigation](automation_plan.md#slice-133-mcp-play-and-investigation) | 13.2 | Accepted | Tooling interlude |
| 14 | [Save and resume](#slice-14-save-and-resume) | 13.3 | Superseded → PC06 | D (retired) |
| 15 | [Cohesive presentation](#slice-15-cohesive-presentation) | 14 | Retired | D (retired) |
| 16 | [Demo integration](#slice-16-demo-integration) | 15 | Retired | D (retired) |

| Milestone | Acceptance experience | Status |
| --- | --- | --- |
| A — First fight | A preset party can fight stationary goblins, win, or lose through actual combat rules. | Accepted |
| B — Dungeon skirmishes | Goblins approach, move, attack, react to hits, and die through the hybrid encounter loop. | Accepted |
| C — RPG loop | Menu, custom party creation, class abilities, expanded spells, inventory, equipment, loot, recovery and shared-flow advancement through levels 1–3 work together. | Accepted |
| D — Finished demo | Historical static-demo target; replaced by procedural campaign milestones. | Retired |

The B follow-up subslices do not reopen accepted slice 08 or milestone B. Each
is a separate planning, implementation and user-acceptance checkpoint. They must
all be Accepted before slice 09 starts; IDs 09–16 retain their existing scope.

The accepted 12.x interlude does not reopen slice 12. All four new slices require
individual acceptance before slice 13 starts. Full session persistence remains
slice 14; the new character library has separate ownership and storage.

The [arena automation interlude](automation_plan.md) inserts 13.1–13.3 before
slice 14. Each requires separate planning, implementation, verification, and user
acceptance. Milestone C remains Accepted; slices 14–16 retain their scope. Replay
from an initial scenario is separate from player-facing run persistence.

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
**Next-agent handoff:** Slice 08 and milestone B are closed. Plan 8.1 next, then 8.2 and 8.3, with separate user acceptance. Preserve accepted combat geometry and presentation except where these subslices explicitly change it. The old 160 MiB cap is an implementation guardrail, not a user requirement; 8.1 may replace it with measured, documented budgets. Slice 8.3 intentionally supersedes immediate damage and occupancy release at attack calculation.
**Acceptance:** Accepted on 2026-10-26: “yes I accept this slice so close it”. This closes milestone B, including all slice 08 review corrections. The user requested a graphics discussion before the next slice.

### Graphical interlude: shared decisions and planning context

Approved scope (2026-10-26): the user requested three subslices, numbered 8.1,
8.2 and 8.3, so each can be planned and implemented in a clean context. This is
roadmap authorization, not approval of their detailed implementation plans. Use
the normal reusable prompts above; do not plan or implement all three together.
Read the preceding accepted slice's handoff and inspect current code before each
plan. Record a durable design and verification/handoff for each as it progresses.

User decisions to preserve:

- Preserve high-resolution originals and generated sheets. Runtime derivatives
  must be reproducible and replaceable; do not destructively resize masters.
- No texture-atlas work or NPC weapon paper dolls. Separate weapon-specific NPC
  appearances are a content convention; the engine still supports mixed equipment.
- Regular demo goblins become melee-only; archers are bow-only with their own art.
- Show outgoing and incoming projectiles. Apply attack consequences at impact,
  including health, life state, reactions, damage/MISS feedback and result logs.
  Calculating rolls in advance is allowed; showing HP loss before impact is not.
- Keep the prepared-image renderer, discrete grid, current camera/FOV and fixed
  presentation. A future higher-resolution dungeon viewport is motivation for
  keeping masters, not a feature to implement in this interlude.
- Keep bounded storage and allocation-free engine frame work. The historical
  160 MiB NPC pixel cap is not a user constraint and may be revised with evidence.
- Hidden application-owned captures are allowed; never automate OS input/focus.

Inspection baseline for the 8.1 planner (verify against current code): the animated
manifest uses 52 extracted frames at 384×536, 512×536 or 768×536, rather than the
large static goblin back.png. Offline extraction currently uses nearest-neighbor
sampling. Startup builds CPU distance banks and a movement raster per pose, then
releases loaded source pixels. NPCs are CPU-composited into the 500×282 dungeon
image, uploaded as one viewport texture; there are no individual NPC GPU draws.
Source pixels measured 58,179,584 bytes; prepared pixels including shadows measured
102,346,541 bytes. Their 160,526,125-byte sum is a startup accounting figure, not
resident GPU usage or a complete peak-memory measurement. A static calculation
estimated 87.88 MiB of stationary banks, 75.65 MiB in the two closest depth slots
in the party's grid row. Audit those slots, including strafe/travel visibility,
before deciding which can be omitted or represented more compactly.

### Slice 8.1: Image preparation and memory

**Deliver:** Reproducible runtime image preparation from preserved masters, better
control of downsampling quality, and materially leaner prepared NPC storage.
Separate source/loading costs from resident prepared pixels in diagnostics and
budgets. Document regeneration commands and how a future viewport-resolution
change would rebuild derivatives. Establish enough measured capacity for the
second complete NPC appearance in 8.2 without inventing an exact asset count.

**Planning focus:** Inspect the existing sheet extraction, NPC loading,
PreparedNpcs distance/travel banks, source lifetime and application upload path.
Audit oversized close-depth canvases and transparent padding first: shrinking
inputs alone does not shrink projection-sized outputs. Compare nearest-neighbor
with alpha-aware area and/or higher-quality filtering on representative actual
poses; choose the default through visual review. Plan safe edge/anchor handling,
reproducible derived-output metadata, capacity/exhaustion behavior and loading
lifetime. Use an offline derivative workflow; retain startup preparation where
appropriate. Do not introduce atlas packing or replace the rendering architecture.

**Boundary:** No new goblin art/loadouts, projectile system, combat-timing changes,
or increased viewport resolution. Preserve ground registration, shadows, picking,
three-cell front visibility and interpolated travel. Revised memory limits must
be measured and documented, not an unexplained increase to bypass inefficiency.

**Acceptance:** Compare deterministic before/after captures at near/far distances,
all headings, diagonal edge slots, strafe and NPC travel. Verify transparent edges,
anchors, wall occlusion and all existing animation clips. Use generated fixtures
for CPU correctness, derivative reproducibility and allocation checks; inspect
shipped art separately. Report before/after source, resident and loading-memory
accounting and preparation cost, stating measurement limits. Check hidden GPU
integration. Originals remain intact and derivatives can be regenerated.

**Stage / last updated:** Accepted and closed; 2026-10-26.
**Approved plan:** User said “Implement the plan.” Both NPC and dungeon raster
preparation move offline; preserve masters. NPC profile C applies to all current
and future NPCs. Surface option A retains point sampling. See
[implemented design](image_preparation_design.md).
**Delivered:** Shared profile, reproducible derivatives, cropped NPC banks,
validated versioned scene/NPC bundles, metadata-only startup, incremental CMake
bake target, separate bounded raster budgets and source-free startup diagnostics.
**Verification:** Debug/Release 22/22, focused ASan/UBSan with leak detection
unavailable under tracing, loaded-bank allocation guard, hidden GPU lifecycle,
396 final captures identical to the approved pre-compaction C reference, clean /
incremental / input-triggered builds and source-free startup. Resident prepared
storage is 147.55 MiB; NPC pixels fell about 61%. Two complete appearance sets fit.
See [verification and handoff](slice81_verification.md) for measurements and limits.
**Review recipe:** Build and run `cmake-build-release/blobber --hybrid-demo --seed 5`.
Review startup, NPC edges/animation/travel and the retained point-sampled dungeon.
**Deviations / limitations:** Both caches bake offline, superseding the original
allowance for startup preparation. The pre-existing broad combat capture fixture
still fails its victory assertion; the focused preparation capture completes.
Interactive desktop review remains the user's task. No 8.2 work has begun.
**Next-agent handoff:** Slice 8.1 is accepted and closed. Preserve global NPC C /
surface A and the appearance integration contract. Plan 8.2 only in a clean context;
its implementation still requires review of its own plan.
Predecessor: [slice 08 handoff](slice08_verification.md).
**Acceptance:** 2026-10-26 — the user said “slice accepted, close it so we can start
8.2”. This explicitly accepts the completed slice; the verification limitations
above remain recorded.

### Slice 8.2: Distinct goblin archer artwork and loadouts

**Deliver:** A separate goblin archer appearance bound to the existing bow-only
rules variant. Its idle, walk, bow attack, hurt and death poses consistently show
appropriate equipment, with the directional coverage required by the renderer.
Use a recognizable bow/quiver silhouette and a cohesive match to existing art.
Bind the two hybrid-demo archers to that appearance. Make regular demo goblins
melee-only and remove their need for bow art in the production appearance.

**Planning focus:** Read 8.1's accepted regeneration/storage contract and the
existing NPC appearance/rules binding and animation authoring workflow. Use the
image-generation skill for new artwork; preserve master sheets and provenance.
Review an archer reference before generating the full set. Reuse suitable existing
bow poses only when visually consistent; registration, equipment and identity
must remain coherent across clips. Plan content/fixture updates for changing the
regular goblin loadout without weakening mixed-weapon engine regression coverage.

**Boundary:** No paper dolls, engine restriction to one weapon, new AI preferences,
or projectile/timing implementation. Mixed-weapon behavior remains supported
through dedicated fixtures. Do not implement spells, inventories or ammunition.

**Acceptance:** Verify appearance/rules bindings and melee-only versus ranged-only
behavior. Inspect all required directional clips, ground/scale continuity, edge
visibility, hit picking and shadows using deterministic captures. Exercise both
variants together in the hybrid demo. Validate content, memory accounting and
frame allocation behavior using 8.1's pipeline. User reviews archer consistency
and enemy-role readability before this slice is Accepted.

**Stage / last updated:** Accepted and closed; 2026-10-26.
**Approved plan:** User said “Implement the plan.” See [archer design](goblin_archer_design.md).
User selected the muted green hood/shoulder covering, exposed ears, brown leather,
bow and quiver. The four-view reference was accepted before animation production
on 2026-10-26 (“yes it looks great, approved :)”). This was reference approval,
not acceptance of the completed slice.
**Delivered:** Separate `goblin.archer` appearance with 20 sheets / 48 poses across
five four-direction clips, name-bound to the existing bow-only monster. Both hybrid
archers use it; regular goblins are melee-only. Original melee art and historical
bow sources are preserved. Shared profile C, existing AI, timings and mixed-weapon
support remain. Source prompts, corrections and registration are tracked.
**Verification:** Debug and Release 22/22, focused ASan/UBSan 3/3, zero-allocation
loaded two-role frame processing, hidden GPU lifecycle and shipped metadata/catalog
loading passed. 498 preparation captures include 102 archer images; 256 source-free
runtime captures match byte-for-byte. Clean/repeated banks reproduce exactly.
Combined runtime bank storage is 181,300,462 bytes, below the 512 MiB limit.
See [complete verification](slice82_verification.md) for commands, accounting,
selected native/reduced screenshots, visual findings and limitations.
**Review recipe:** Open `docs/screenshots/slice82/`, then run
`cmake-build-release/blobber --hybrid-demo --seed 5`. Check equipment continuity,
role readability, ground/scale consistency, bow release, travel and reactions.
**Deviations / limitations:** No 8.3 work. Interactive desktop review remains the
user's task. Leak detection is unverified; the pre-existing broad-capture victory
failure remains documented, while focused preparation/hybrid suites passed.
**Next-agent handoff:** Slice 8.2 is accepted and closed. Plan 8.3 in a clean context using
[slice82_verification.md](slice82_verification.md#limitations-and-next-agent-contract).
Preserve both appearance IDs, global profile C, front-band presentation and the
600 ms bow-release marker. Damage and occupancy release remain immediate until 8.3.
**Acceptance:** Accepted on 2026-10-26. User said “slice accepted, close it up”.
This closes the completed slice; the verification limitations above remain recorded.

### Slice 8.3: Projectiles and impact-time consequences

**Deliver:** Visible arrows traveling from the party into the dungeon and from
enemy archers toward the viewer, with perspective size changes and distinct
hit/miss endings. Launch enemy arrows at their authored bow-release marker;
place source/target anchors consistently with displayed NPC positions, including
the inward front-row adjustment. Synchronize arrival with all attack consequences.

Calculate hit/damage once in advance if useful, spend the action/cooldown at attack
start, and apply damage at impact. HP, unconsciousness/death, occupancy release,
hurt/death animation, portrait state, splashes and result logs must agree at that
instant. Apply the same consequence-timing rule to existing melee strikes. Preserve
turn presentation holds and freeze pending impacts/projectiles during pauses.
This supersedes slice 08's immediate damage/life changes, not its accepted combat
geometry, initiative rules or AI policy.

**Planning focus:** Inspect CombatController attack/hold/update paths, ActorWorld
and PartyState damage, NpcAnimator cues, CommandController feedback/log scheduling,
and session phase clocks. Design a bounded authoritative pending-impact mechanism
separate from cosmetic projectiles; gameplay consequences must not be dropped when
visual capacity is exhausted. Resolve launch interruption, overlapping real-time
shots, target death/removal before arrival, party/NPC movement or turning during
flight, phase transitions, large time steps, restart and victory/defeat ordering
in the detailed plan. Define flight duration/trajectory and anchoring with visual
review. Preserve deterministic RNG and exactly-once damage; visuals never reroll
or determine a hit by collision. Simple authored arrow art need not use imagegen.

**Boundary:** Provide a small reusable presentation path for later spell projectiles,
without implementing spells, concentration, beams, area effects, physics simulation
or a speculative general VFX framework. Do not defer health display correction
while only adding moving arrows. Integrate with 8.1's memory/preparation approach.

**Acceptance:** Tests check immediately before/at/after release and impact for
party/enemy melee and ranged hits, misses and lethal attacks. Cover pause/resume,
overlapping approach attacks, invalidated targets, movement, phase changes,
restart, bounded capacity and no duplicate damage/RNG consumption. Verify no
premature HP/life/log/reaction changes and no early turn or victory handoff. Run
allocation checks and hidden captures at launch, mid-flight and impact, near/far
and diagonal targets. User reviews shot direction, target attribution and pacing.

**Stage / last updated:** Accepted and closed; 2026-10-27 (including front-cell spacing correction).
**Approved plan:** User said “Implement the plan.” See
[projectile/impact design](projectile_impact_design.md). Movement preserves rolled
outcomes; incapacitation cancels unreleased attacks; nonlethal hurt suspends and
resumes windup; readable 180–450 ms arrow flight; party melee strikes at 200 ms.
**Delivered:** Bounded authoritative impact timeline and timestamped event batches;
prepared outgoing/incoming arrows with perspective, arc and distinct endings;
200 ms party melee slash; authored enemy release/strike; hurt suspension/resumption,
cancellation, released-shot survival and impact-synchronous HP/life/occupancy/UI.
**Verification:** Debug and Release 24/24 pass; affected final rechecks pass;
ASan/UBSan 4/4 pass; zero tracked frame allocations. Full 260-attack stress delivers
520 events without loss/replay. Hidden GPU check passes; broad capture completes
292 images (inherited victory failure did not recur). New suite exports 380 images,
all byte-identical with source-free assets. 183 shared 8.2 images are unchanged.
See [verification, selected captures and handoff](slice83_verification.md).
**Review correction (2026-10-27):** User reported center-cell monster overlap after
the diagonal edge fit. The fit now preserves the original center-slot spacing and
compresses only outside those pivots. A regression first failed on the old mapping,
then passed in both rows/all headings. Release 24/24, focused Debug 5/5 (including
zero frame allocations), and 320 hidden hybrid/appearance captures pass. Both
center pairs and all six front-band targets remain pickable. See the
[corrected captures](slice83_verification.md#review-correction-center-cell-pair-spacing).
**Review recipe:** Run `cmake-build-release/blobber --hybrid-demo --seed 5`.
Review party/enemy arrows, recipient attribution, melee windup, impact feedback,
pauses and hurt before release; deterministic capture instructions are in the report.
**Deviations / limitations:** No scope deviation. Interactive pacing/desktop review
remains the user's task. Leak detection remains unavailable under the tracing
restriction; ASan/UBSan ran with it disabled. Third-party raylib is not instrumented.
**Next-agent handoff:** Slice 8.3 is accepted and closed. Slice 09 is next eligible
for planning in a clean context; it has not started. Consume whole borrowed
`CombatEvents` batches: `AttackStarted` is acceptance, `Attack` is committed impact,
`occurredMs` uses session time, and `step` omits drained impacts. Keep hybrid gameplay
cooldowns/round effects separate from presentation deadlines. Cosmetic exhaustion
never cancels gameplay. Read the design and verification before any follow-up.
**Acceptance:** 2026-10-27. The user said “slice accepted, update doc”, accepting
the completed slice including the front-cell spacing correction. Recorded
verification limitations remain; acceptance does not turn unexecuted checks into passes.

### Slice 09: Effects and concentration

**Deliver:** Bounded effect lifecycle for the planned curated spells/features:
durations, expiry, saves, concentration, stacking rules where needed, and cleanup.

**Boundary:** Requires accepted 8.1–8.3. Read 8.3’s impact/phase-clock handoff so
effect application and concentration consequences agree with attack impact. First
identify concrete consumers in slices 10 and 13. Avoid a universal effect language
or implementation of unused conditions.

**Acceptance:** Effects behave across real-time/turn phases, pauses, unconsciousness,
death, and encounter completion. Test interruption, replacement, expiry boundaries,
invalid targets, capacity exhaustion, and allocation behavior.

**Stage / last updated:** Accepted and closed; 2026-10-27.
**Approved plan:** User said “okay implement the plan”. See [effects design](effects_design.md).
Selected Bless, Bane, Shield of Faith, Mage Armor and Hold Person; headless review.
Hold retries every six gameplay seconds outside turns and at full-round boundaries
during turns. Mobile allies carry paralyzed members; paralysis removes paired protection.
**Delivered:** Session-owned bounded effects, stacking and armor/roll modifiers,
initial/repeat saves, impact-time concentration, chronological expiry/impact ordering,
paralysis action/facing/formation integration, and deterministic headless review.
Spell casting, mana and slice 10 content remain unavailable.
**Verification:** Final Debug 25/25, Release 25/25, focused ASan/UBSan 5/5;
zero tracked C++ frame allocations. Separate effect-capacity and 260-attack stress
pass. Debug/Release traces are byte-identical. See [verification and handoff](slice09_verification.md).
**Review recipe:** `cmake-build-release/blobber_effects_tests --trace`; compare
real-time retries, no retry while waiting on a player decision, one retry at the
full-round boundary, and concentration/HP changing at impact. See [retained trace](slice09_trace.txt).
**Deviations / limitations:** No scope expansion. Only party casters exist, so an
all-paralyzed concentrating party cannot persist: the last caster's incapacitation
ends Hold. Tests cover self-incapacitation and skipped-turn behavior without adding
NPC casters. Leak detection is disabled/unverified; no GPU or interactive checks
were run for this headless slice.
**Next-agent handoff:** Slice 09 is accepted and closed. Slice 10 is next eligible
for planning in a clean context and remains unstarted. Consume the whole event batch before any subsequent session
simulation operation, including synchronous apply/remove/submit/move. Effect APIs
are low-level consequences, not player casts: future spell commands own resource,
class, range/humanoid/visibility validation and provocation. Effects use gameplay
time; attack impact and event presentation use session time. Accepted attacks retain
their rolls; damage checks concentration at impact. Read the design and verification.
**Acceptance:** 2026-10-27. The user said “I accept the slice, mark it as such in the plan doc”.
Recorded verification limitations remain; acceptance does not turn unexecuted checks
into passes. Slice 10 remains Planned and unstarted.

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

**Stage / last updated:** Accepted and closed; 2026-10-27.
**Approved plan:** User said “Implement the plan”. See [spells design](spells_design.md).
Expanded roster with Inflict Wounds replacing Healing Word, 4/6/14 mana, Rogue
back-row reach, safe Arcane Recovery, three-cell areas, live target selection,
and class features only (existing racial passives retained).
**Delivered:** All fourteen spells, mana, class features, live multi-target/area
selection, prepared spell glints and complete event-batch consumption. No slice 11
inventory/equipment changes, progression, resting or additional racial features.
**Verification:** Debug/Release 26/26; focused ASan/UBSan 6/6; zero-allocation guard;
full-population spell/weapon and effect-capacity stress; hidden GPU lifecycle,
228 spell/feature captures (39 reduced), source-free startup and unchanged seeded trace.
The approved [UI correction](spellcasting_ui_design.md) adds the compact picker,
60%-width casting dock, direct portrait/dart/area feedback, focus preservation,
individual invalid-target pruning and seventeen painted icons. See the latest
[UI verification and handoff](slice10_ui_verification.md), which supersedes the
original [verification](slice10_verification.md) for casting interactions. Subsequent
badge/spacing polish passed focused Debug 6/6 and Release 8/8, with 231 spell
and 73 HUD captures; see the same handoff for the exact checks. The latest
[native HUD and impact-alert correction](native_hud_verification.md) passed full
Debug/Release 26/26, focused sanitizers 5/5, generated-art GPU checks, allocation
guards and capacity stress; native/reduced captures are retained there.
**Acceptance:** Explicitly accepted by the user on 2026-10-27: “I accept the slice,
close it in the doc”. Acceptance includes all spellcasting UI corrections, stable
badges, native-resolution portraits/icons and HUD, and impact-triggered alerts.
The user reported that the result looks good and works well. Remaining technical
verification limitations in the handoffs are retained; acceptance does not imply
that previously skipped hardware/leak checks were run.
The subsequent [Class Features correction](spellcasting_ui_design.md#class-features-verification)
was also accepted on 2026-10-27: the user described it as “very nice” and requested
that acceptance be recorded. Its non-caster labels/icon, passive browsing, resource
clarity and layout fix are accepted and closed; its verification is linked above.
**Next-agent handoff:** Slice 10 is accepted and closed. Slice 11 is next eligible
for planning and remains Planned/unstarted. Preserve the documented combat,
resource, rendering and allocation contracts; do not implement slice 11 as part
of this closure.

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

**Stage / last updated:** Accepted and closed; 2026-10-27.
**Approved plan:** User said “Implement the plan.” See [inventory design](inventory_design.md).
**Implemented:** Four 48-cell bags and separate equipment, atomic weight/stack/equipment transactions, safe-only gear/transfers, paused bag organization, drag/button/keyboard controls, Shift-drag quantity default one, fifteen painted item icons and silhouette. Healing/mana potions and finite party arrows use existing action/readiness clocks. Schema 5 retains 1–4 compatibility; gear changes update stats and end Mage Armor when required.
**Verification:** Debug/Release 27/27; affected suites rerun after refinements, ASan/UBSan, zero-allocation guard, 100,000 split/merge cycles, hidden GPU and 41 native/reduced/letterboxed captures. See [durable verification and handoff](slice11_verification.md), including test limitations and retained review images.
**Review correction (2026-10-27):** Implemented the user's screenshot feedback: contextual actions with unavailable states, drag-only move/merge/split initiation, torso/ranged slot placement, centered captions and large split controls, right-aligned counts, I inventory/M map, and a painted backpack dock icon with an updated tooltip. No later-slice rules were added. Follow-up checks and replacement review images are recorded in the [correction handoff](slice11_verification.md#inventory-review-correction-2026-10-27).
**Approved Character tab extension (2026-10-27):** User accepted the image-generated mockup and authorized implementation. The fullscreen panel now shares Inventory / Character tabs and the character strip; portrait info opens Character, inventory controls open Inventory. Live read-only stats/resources/equipment replace the small inspector. XP displays progress but awards and advancement remain deferred. Focused Debug/Release 6/6, allocation and hidden GPU checks pass; 77 captures generated. See [design](character_sheet_design.md) and [verification](character_sheet_verification.md). The implementation was accepted with slice 11 on 2026-10-27.
**Acceptance:** Explicitly accepted by the user on 2026-10-27 after the inventory corrections and Character sheet implementation. The user requested documentation closure and a fresh context for slice 12. Interactive desktop review was not automated; recorded verification limits remain unchanged.
**Historical handoff at slice 11 closeout:** Slice 12 was next eligible, Planned and unstarted at that checkpoint. It has since been accepted and closed; see the current checkpoint and slice 12 record below. Read [slice 11 handoff](slice11_verification.md#accepted-closeout-and-slice-12-handoff) and [Character sheet handoff](character_sheet_verification.md) before planning it. Preserve the accepted UI, authoritative inventory transactions and bounded allocation contracts. Define loot/rest ownership, safety, capacities and recovery rules in the slice 12 plan; ask about material ambiguities and obtain plan approval before coding. XP awards/level-up remain slice 13; save/resume remains slice 14.

### Slice 12: Loot and recovery

**Current review correction:** Implemented and verified: one-bag loot dialog with contained/aligned details, shared Inventory slots and portrait recipient selection; Take All now closes automatically; Inventory Drop places selected stacks in the current cell; a small generated top-bar Rest icon replaces the large dock and log widths are restored. [Current handoff](slice12_verification.md) and [design](loot_recovery_design.md#inventory-drop-and-top-bar-rest-follow-up). Accepted by the user on 2026-10-27.

**Deliver:** Bounded loot ownership and collection, supplies/consumable acquisition,
Hit Dice short rests, feature recovery, safe-location long rests, and camp supplies.

**Approved loot interaction (corrected 2026-10-27):** Death publishes drops once
at the monster's death cell, independently of corpse animation/removal. One bag
represents all remaining contents. Clicking opens a paused grid with hover stats,
click-to-take, Take All and Close/Escape. Paging never requires pickup. Portrait
clicks switch the selected recipient while the dialog remains open; collection
never silently redirects items to another party member. Whole-stack transactions
leave failed contents on the floor with capacity/weight/eligibility feedback.

**Boundary:** No shops or quest system. Define safety, resource quantities, corpse
independence, rest eligibility, and full-inventory handling explicitly.
The slice 12 plan must also specify pickup reach/visibility, combat eligibility and
any action cost, drop contents/quantities, and presentation/picking of overlapping
floor bags. These decisions are implemented in the linked design.

**Acceptance:** Several encounters can be played with meaningful recovery decisions.
Test duplicate collection, full inventories, resource depletion, repeated rest,
threat restrictions, dead characters, and exactly-once resource consumption.
Verify that death creates one set of floor drops, corpse removal preserves them,
clicking collects into the active character's bag, changing the selected character
changes the recipient, failed collection preserves the drop, and repeated clicks
cannot duplicate items. Verify visible floor placement and reliable item picking.

**Stage / last updated:** Accepted and closed; 2026-10-27.
**Approved plan:** User said “Implement the plan.” See the [loot/recovery design](loot_recovery_design.md) for behavior, ownership, capacities and verification.
**Decisions:** Safe-only whole-stack pickup into the selected character, current/three-front-cell reach, one bag per cell and ten-slot dialog pages; usable gear, weighted gold and occasional potions; three fixed camp kits and no sanctuary in normal dungeon play. One/eight-hour short/long rest skips advance effects/cooldowns without moving NPCs. Stable allies can recover; dead allies cannot. Future hub/shops remain out of scope.
**Visual checkpoint:** The user accepted [camp mockup v1](mockups/slice12/camp-screen-v1.png) on 2026-10-27 (“yes that looks very nice, continue”), before code and production animation work.
**Delivered:** Session-owned floor drops, staged pickup, schema-6 content and level metadata; safe Short/Long Rest transactions; native exclusive camp UI and four-frame bonfire playback; prepared bags with occlusion, paused grid collection and visible Rest button; recovery demo and hidden capture mode. See [verification and selected captures](slice12_verification.md).
**Verification:** Corrected implementation: Debug 29/29 and Release 29/29; focused recovery/inventory/render ASan+UBSan 3/3; zero tracked frame C++ allocations; separate full-capacity loot publication/render stress, hidden GPU lifecycle, and 108 native/reduced/letterboxed captures pass. The user accepted the result; automated verification limits remain unchanged. Earlier baseline checks remain recorded separately in the handoff.
**Next-agent handoff (updated 2026-10-29):** Slice 12 is accepted and closed. Slice 12.1 has since been [accepted and closed](slice121_verification.md#accepted-closeout-and-next-agent-handoff); 12.2 and 12.3 are now accepted and closed; 12.4 is now accepted and closed; slice 13 is next eligible for planning. The historical closeout predates this insertion. Read the [accepted closeout](slice12_verification.md#accepted-closeout-and-next-agent-handoff) before planning. Preserve accepted UI, checked loot/inventory transactions, generation-safe floor handles and allocation guarantees. This documentation closeout does not start slice 13.
**Acceptance:** Explicitly accepted on 2026-10-27: “thats better, record the slice as accepted in the doc”. Acceptance covers the completed slice and all loot-bag, Drop, Take All and Rest-icon corrections.

### Character creation interlude: accepted flow and shared contracts

The user accepted the [visual flow](character_creation_flow_proposal.md) and
[sixteen-screen HTML gallery](mockups/character-creator-v1/index.html) on
2026-10-28, explicitly including an expanded spell catalog and a dedicated slice
for its missing spells before slice 13. This interlude comprises **three creation
slices plus one spell-content slice**, numbered 12.1–12.4. Slice 12.1 is accepted and
closed; 12.2 is also accepted and closed, while 12.3 is accepted and closed and 12.4 is accepted and closed;
visual/roadmap approval is not implementation acceptance. Each still receives its
own planning pass, implementation authorization, verification and user review.

**Mandatory visual references:** Open the gallery and the PNGs linked by the
selected slice before planning UI or generating assets. Keep the approved dark
green panels, restrained gold headings/borders, teal selections, painted portraits,
readable native text, fixed navigation and scrollable content. Use shared layouts
and existing fonts rather than reproducing imagegen's incidental typography or
alignment differences. Preserve the fixed 1920×1080 presentation and prepared-image
renderer. Mockups are concept images, not runtime textures. Preserve their originals
and [prompt provenance](mockups/character-creator-v1/prompts.json).

**Approved content correction:** The user chose **Color Spray instead of Ray of
Sickness** after the latter was found absent from SRD 5.1. Screens 10 and 14 retain
their historical illustrative label; their captions and the plan override it.
Do not implement Ray of Sickness or silently change the project's source policy.

**Shared completion target:** Launch into Main menu; Start new game opens a
four-character party/formation picker and then the current test level. Character
Creator opens the persistent library. Creation is Identity → Class → Abilities →
Skills placeholder → conditional Spells → Equipment → Review. Save returns to the
caller. Library deletion and discarding an unsaved draft require confirmation.
Quit closes cleanly. No new campaign, skill system, respec or race/class roster.

**Rules/data contracts to preserve across slices:**

- Point buy: 27 points; base 8–15; costs 0/1/2/3/4/5/7/9; existing racial bonuses
  applied separately. Recommended allocation spends all points. Completion requires
  the full budget; no rerolls or point carryover. Names retain the existing 64-byte
  printable-text validation; show the actual limit rather than the mockup's 32.
- Four existing races and classes remain freely combinable. Fighter Defense and
  Cleric Life Domain are the supported defaults. Five portraits per race, twenty
  total; keep the current four and generate sixteen additions. Portraits are
  cosmetic, independent of class and formation; include varied adult faces and
  fighter/mage/cleric/rogue/general-adventurer clothing.
- Wizard: three class cantrips, six learned level-one spells, and preparation up to
  max(1, level + INT modifier). Cleric: three class cantrips, class-list preparation
  up to max(1, level + WIS modifier), plus always-prepared Life Domain Bless/Cure
  Wounds outside that allowance. Require at least one ordinary prepared spell;
  choosing fewer than the maximum is legal. Defaults fill the allowance.
- Activate High Elf's one additional wizard cantrip through a separate racial
  grant using Intelligence, including on Fighters/Rogues/Clerics. No duplicated
  cantrip choice across grants. This concrete grant is the only new racial
  spell trait in scope; other inactive racial traits remain disclosed as inactive.
  A racial cantrip costs no mana and does not grant a non-caster a mana pool.
- Spell eligibility is per character and per grant, not the entire class catalog.
  The same authoritative query must drive the picker, preview and cast validation.
  Keep class mana caps 4/6/14 and cantrip/first-level costs 0/2. Level-two spell
  costs and actual progression remain slice 13. Skills remain a non-blocking
  reserved step; no backgrounds, skill proficiencies or skill selection mechanics.
- Saved library entries are reusable level-one starting builds. Each dungeon run
  owns a separate party copy. XP, spent supplies, injuries and level-ups never
  overwrite the library. Persist content IDs and a separate saved-character ID,
  not catalog indices, formation indices or runtime handles. Repeated display
  names are legal. Deleting a blueprint does not delete an existing session/save.
- Preserve ordinary class starting supplies and provide class-appropriate gear
  choices. Assign the existing three party camp kits once at launch, irrespective
  of class composition; include their weight/capacity in the launch transaction.
  Each fresh run receives that initial grant exactly once; launch retry, resume or
  level-up must not append another grant to an existing party.

The 12.x plans must turn these contracts into small cohesive systems: pure build
rules; versioned library I/O; an isolated creation draft; immutable portrait
metadata/prepared assets; application menu/session orchestration; and existing
Party/Combat ownership. No generic wizard framework, ECS or file I/O in rendering.
Specify limits and exhaustion behavior before code. Menu input, searches, scrolls,
draft edits and normal gameplay must use preallocated/reused storage; explicit
load/save/content preparation phases may allocate. Maintain failure atomicity.

### Slice 12.1: Expanded level-one spell catalog

**Prerequisite:** Slice 12 accepted. **Deliver:** Implement the missing spells and
their real effects, targeting, art and feedback before creation selects them.
This is the dedicated spell-content slice requested on 2026-10-28; no second-level
spells or advancement. Use the official SRD 5.1 source and record project-specific
adaptations in the [ledger](srd_content_ledger.md) before implementing them.

**Visual references:** [Cantrips 09](mockups/character-creator-v1/09-cantrips-v1.png),
[spellbook 10](mockups/character-creator-v1/10-spellbook-v1.png),
[preparation 11](mockups/character-creator-v1/11-prepared-spells-v1.png),
[cleric variant 15](mockups/character-creator-v1/15-cleric-preparation-v1.png).
These establish icon/detail/count presentation. Runtime casting must retain the
accepted [spell UI](spellcasting_ui_design.md), not replace it with creator screens.

**Content target:** Retain all fourteen existing spells and add these ten:

| Class / tier | Additions | Concrete system needs for the detailed plan |
| --- | --- | --- |
| Wizard cantrips | Ray of Frost; Shocking Grasp; Poison Spray | Cold damage with a bounded slow; melee spell attack with metal-armor advantage; short-range Constitution save poison damage. Freeze supported targeting and timing adaptations. Reactions remain omitted, so disclose Shocking Grasp's unused reaction rider. |
| Wizard level one | Sleep; False Life; Grease; Color Spray | HP-budget sleep, temporary HP, a persistent ground area with movement/prone consequences, and HP-budget blindness. These must have working gameplay consequences and visible state, not just spell entries/icons. |
| Cleric cantrips | Guidance; Resistance | Concentration buffs consumed by one eligible ability check or save. Guidance must have an actual current consumer: initiative is an ability check. No skill system is required. |
| Cleric level one | Sanctuary | A protective ward that changes hostile single-target attack eligibility, ends on the appropriate offensive action, and interacts correctly with AI and area effects. Adapt its bonus-action casting to the existing action/cooldown economy. |

This yields 24 catalog spells: Wizard five cantrips/eight level-one spells, Cleric
four cantrips/seven level-one spells. The extra Poison Spray keeps High Elf wizard
choices meaningful after its additional racial cantrip; Sanctuary gives the
Cleric a preparation alternative beyond the four non-domain spells shown in 15.
Keep Healing Word's earlier deliberate exclusion. No cosmetic-only Light/Guidance
entries pretending to work, no higher-tier imports, no automatic import of all SRD.

**Planning responsibilities:** Inspect `rules/spells.*`, `rules/health.*`,
`game/combat_spells.cpp`, `game/effects.*`, `game/npc_ai.*`, `game/movement.*`,
`game/recovery.*`, `game/spell_commands.cpp`, catalog loading and prepared effect
assets. Write a focused expanded-spells design, linked from system design. For
each spell freeze range, recipients, friendly fire, LOS, pattern, save/attack,
mana/action costs, concentration, duration, release/impact timing and expiry.
Do not assume tabletop movement distances or turn boundaries map automatically.

The detailed plan must explicitly resolve these interactions before implementation:

- Sleep/Color Spray use current-HP budgets, deterministic HP ordering/ties and
  complete area enumeration. Distinguish sleep from dying/dead; define waking by
  damage and an action, attack advantages/criticals, paired protection and AI
  occupancy. Blindness must affect attacks and sight-dependent targeting/AI.
- Temporary HP are separate from actual/max HP, do not heal or revive, and do
  not stack. Define replacement choice, expiry, rest and concentration damage
  checks when the temporary buffer absorbs the entire hit.
- Grease is a bounded persistent world area, not a three-recipient buff. Define
  entry/creation/turn checks, difficult terrain, prone recovery and legal step
  scheduling in both clocks. Ray of Frost needs a corresponding discrete slow
  rule. Do not change camera movement, input or introduce freeform movement.
- Guidance/Resistance need an explicit one-shot consumption policy (recommended
  automatic next eligible roll, visibly disclosed), combination with Bless/Bane,
  natural death-save results, concentration and expiry. Guidance cannot modify
  attacks or saves; add no player-facing skill selection just to support it.
- Sanctuary must not let AI repeatedly reroll a ward save until it succeeds,
  double-spend an action, gain free retargets, or evade formation constraints.
  Specify behavior when every legal target is warded and at delayed-impact ties.

Recalculate effect/event/target capacities for the new conditions, areas and
recipients; existing 64 effect slots, three-recipient effects and event-bound
proofs are not automatically adequate. Prefer dedicated bounded data where a
ground area differs from an ordinary buff. Preserve accepted roll-at-acceptance,
impact-time damage, pause, concentration and alert contracts. The catalog limit
32 and class-list limit 16 suffice for this target; audit UI icon/list limits too.

**Boundary:** Keep existing class-wide spell access temporarily so every new spell
is testable through the current casting UI. Slice 12.2 replaces that access with
validated per-character choices; do not smuggle in creator or level-up UI here.
Approval of the visual proposal does not approve an unstated movement/condition
adaptation: present concrete adaptations in this slice's normal planning review.

**Acceptance:** All ten spells can be cast in representative deterministic fixtures
with correct costs and observable consequences. Cover save/miss/immune/no-target
and exhaustion paths, low/high HP budgets, tied recipients, simultaneous impacts,
stack/refresh/expiry, waking, temp-HP absorption, blind/prone/slow AI, ward failure,
concentration loss, pause/phase/rest transitions and stale identities. Rejected
commands leave resources/RNG unchanged. Test-owned CPU fixtures and allocation
checks stay small; maximum area/effect/event loads are a separate stress check.
Run Debug/Release, focused sanitizers and hidden GPU/presentation captures of all
new spells/statuses with prepared art at native/reduced sizes. Report skipped
hardware checks. Preserve the existing fourteen-spell behavior.

**Handoff:** Publish spell IDs, class membership, effect/casting interfaces, source
adaptations, capacities and reusable selection metadata for 12.2. Stop for review.
**Execution record:** Accepted and closed, 2026-10-29. Implemented the approved [design](expanded_spells_design.md): ten spells, ground targeting, conditions, temporary HP, one-shot buffs, Sanctuary, schema 7 and prepared art. Completed the [visual review corrections](spell_visual_review_design.md): level/Abilities tabs, status icons and duration tooltips, generated Grease pool, and required four-direction prone poses. [Verification/handoff](slice121_verification.md): Debug/Release 31/31, focused sanitizers 6/6, zero tracked frame allocations, capacity baking, hidden GPU checks and 470 captures. Portrait follow-up adds turn-based duration labels and a gold concentration emblem with spell/duration hover: focused Debug/Release 5/5, allocation/GPU checks and 482 captures pass. User acceptance is recorded below; 12.2 has since been accepted and closed.

**Acceptance:** 2026-10-29 — the user said “nice work, record the slice as accepted”. This accepts the completed spell catalog and all visual/portrait corrections, including spell-level tabs, Grease artwork, static prone poses, status icons, turn-duration tooltips and the concentration emblem. Recorded verification limitations remain unchanged.

**Historical next-agent handoff at 12.1 closeout:** Slice 12.1 is accepted and closed. Read its [accepted closeout](slice121_verification.md#accepted-closeout-and-next-agent-handoff) and the approved character-creation references, then inspect code and plan 12.2 in a fresh context. Implementation requires approval of that slice's plan. This closeout does not start 12.2.

### Slice 12.2: Character builds, library storage and portraits

**Prerequisite:** 12.1 accepted. **Deliver:** Headless-testable character creation
rules, persistent starting-build library, per-character spell access, twenty
portraits and party construction from validated chosen builds. This is the data
and asset foundation; normal startup still follows its existing route until 12.3.

**Visual references:** [Library 02](mockups/character-creator-v1/02-character-library-v1.png),
[identity/portraits 05](mockups/character-creator-v1/05-identity-and-portrait-v1.png),
[class 06](mockups/character-creator-v1/06-class-v1.png),
[point buy 07](mockups/character-creator-v1/07-point-buy-v1.png),
[equipment 12](mockups/character-creator-v1/12-starting-equipment-v1.png),
[review 13](mockups/character-creator-v1/13-review-and-save-v1.png).
Generate production portrait assets through imagegen with preserved masters and
prompts; never crop the mockup into runtime UI or replace the current four portraits.

**Planning responsibilities:** Inspect `rules/character.*`, catalog/schema loading,
`game/party.*`, inventory initialization/validation, spell command validation,
HUD portrait lookup, `platform/presenter.*` and prepared asset ownership. Document the
build/draft/library design before code; keep JSON/filesystem outside rules/core.
Define a bounded draft containing identity, portrait ID, base abilities, supported
feature selections, class/racial cantrips, learned/prepared/granted spell IDs, and
starting item/equipment selections. Pure validation and preview return errors and
derived values without constructing a live session or copying strings each frame.

**Required behavior/contracts:**

- Implement the shared point-buy, selection counts, class features and High Elf
  grant. Casting ability belongs to the spell grant: an Elf Cleric's wizard cantrip
  uses INT while cleric spells use WIS. Fighter/Rogue racial cantrips must appear
  alongside features and cast legally without mana. Query-based validation must
  also reject a forged direct command for an unlearned/unprepared spell.
- Make supported gear choices data-driven and valid for all sixteen race/class
  pairs. Freeze exact loadout options in the slice plan using implemented weapons,
  armor and ammunition; add quarterstaff content/art if the catalog lacks the
  mockup's choice. Include supplies, equipment slots, weights and launch camp kits
  exactly once. Reuse inventory/stat validation, not a divergent preview calculator.
- Introduce a stable portrait identity in builds and snapshots. Resolve it across
  HUD, inventory, character sheet, spell targets and loot recipients; never select
  portrait from party slot or class. Define missing/unknown portrait handling.
- Separate library save format from rules catalog and future session saves. Use
  versioned bounded records, stable content references, atomic replacement and
  explicit read/write failure reporting. Freeze file path, byte/record limits,
  ID-generation strategy and overflow policy in the detailed plan. A practical
  initial target is 128 saved characters; expose capacity exhaustion clearly.
- Seed four presets only when no library has ever been initialized. Preserve
  intentional empty libraries and do not overwrite corrupt/unsupported files
  with defaults. Decide recovery/backup behavior explicitly. Keep names, stats,
  gear and existing portraits of presets, but author legal new spell selections
  for them; document the deliberate transition from all-class-spell access.
- Build an independent Party from exactly four validated starting builds plus
  formation. No borrowing mutable library records into a session. Keep the
  catalog alive for Party, and preserve generation-safe inventory identities.
  Loading, deleting or editing a library must not invalidate an existing run.

**Boundary:** No full session save/resume, campaign state, player skill mechanics,
general build editor/respec, XP or actual leveling. Do not use legacy schema
compatibility as a permanent loophole granting every class spell to new characters.
Define migration/defaulting for existing authored presets and test fixtures;
update captures that intentionally relied on unrestricted test spell access.

**Acceptance:** Exhaust point-cost transitions and boundary budgets; test all
race/class pairs and supported loadouts, separate racial casting stats, duplicate
spells, invalid references/portraits, count/subset/domain rules, overweight or
illegal gear, and independent session copies. Library tests own temporary files:
first run vs empty, round-trip, repeated names, corrupt/truncated/unknown-version
and oversized input, duplicate IDs, unavailable content, failed save/delete,
interrupted replacement and capacity exhaustion. Errors preserve prior durable
state. Check all twenty portrait masters/prepared outputs, crop readability and
GPU lifetime; native/reduced HUD and sheet captures prove portrait identity after
formation changes. Run focused/full CPU suites, allocation checks and relevant
sanitizers; distinguish disk-operation allocations from steady-state frame work.

**Handoff:** Record the schema and compatibility policy, IDs/capacities, pure draft
API, library API, portrait manifest, explicit-party constructor, preset selections
and verified spell-access migration. Supply a deterministic headless diagnostic
or test scenario for inspection before the GUI lands. Stop for review.
**Execution record:** **Accepted and closed**, 2026-10-29. [Implemented design](character_builds_design.md) and [verification/handoff](slice122_verification.md). Delivered strict builds, schema-8 grants/gear/portraits, version-1 atomic blueprint library, independent party/session constructors, three launch camp kits, twenty portrait masters and bounded prepared atlases. Full Debug/Release 34/34, focused sanitizers 5/5, zero tracked frame allocations, hidden GPU checks, 225 build/portrait captures and 482 spell captures pass. The user selected three kits, curated gear and a user-data library location.
**Acceptance:** The user explicitly accepted slice 12.2 on 2026-10-29: “slice accepted, record it in the plan doc, so we can get started on the next slice”. Verification limitations remain as recorded.
**Historical next-agent handoff at 12.2 closeout:** Slice 12.2 is accepted and closed. Slice 12.3 was next eligible for planning; it is now accepted and closed. Read the [durable API and ownership handoff](slice122_verification.md#next-agent-handoff), approved menu/library references and relevant code before proposing its plan. Obtain approval of that plan before implementation. Library CRUD and strict creation validation are already implemented; normal startup remains the dungeon route. This closeout does not start 12.3.

### Slice 12.3: Main menu, character library and party launch

**Prerequisite:** 12.2 accepted. **Deliver:** Startup menu, persistent scrollable
library with read-only detail and confirmed deletion, party selection/formation,
and reliable launch of the current test level with the chosen builds.

**Visual references:** [Main menu 01](mockups/character-creator-v1/01-main-menu-v1.png),
[library 02](mockups/character-creator-v1/02-character-library-v1.png),
[party selection 03](mockups/character-creator-v1/03-party-selection-v1.png),
[delete confirmation 04](mockups/character-creator-v1/04-delete-confirmation-v1.png),
[review layout 13](mockups/character-creator-v1/13-review-and-save-v1.png).
Read the [gallery captions](mockups/character-creator-v1/index.html) for routing.

**Planning responsibilities:** Refactor `app/application.cpp` into small menu,
session preparation/lifetime and orchestration responsibilities; preserve `main`
as startup/shutdown/error handling. Inspect window/presenter destruction order,
input routing, native aspect-fit mapping, diagnostics and existing capture/demo
entry points. Menu needs catalog/library/UI assets, not map/NPC/dungeon banks.
Do not merely hide an already loaded or running level behind a menu.

**Required behavior/contracts:**

- Exactly the three requested main entries: Start new game, Character Creator,
  Quit. Start opens the party picker. Creator opens the library. Quit and OS close
  release resources in valid context order. Read-only View uses the review layout
  with no misleading Save/Change controls.
- Grid scrolling, search and filters preserve stable record selection; empty
  library/no results and full capacity have clear messages. Delete identifies the
  target by ID and name/portrait, defaults to Keep character, commits once and
  preserves the card on disk failure. Filtering/reordering cannot change the target
  of an open confirmation. Cancel/Escape does not activate underlying controls.
- Choose four distinct valid records in a 2×2 formation, remove/replace/swap slots,
  and show readiness. Two characters with identical names remain distinguishable
  by ID/portrait. Never require one of each class. Default party is an explicit
  shortcut and must work after user records are deleted, without silently
  reseeding or mutating the library; stage the built-in four for that launch.
- Enter dungeon validates everything again, stages party plus one launch supply
  grant, prepares the existing map/scene, then publishes a live session. Show a
  loading transition; failures return to the picker with selections and a useful
  error. Double activation/retry cannot create two sessions or duplicate supplies.
- Preserve debug/demo/capture command-line modes with explicit deterministic menu
  bypass. Record their default-party/fixture behavior, seeds and asset lifetimes.
  A menu/capture failure must not accidentally load the user's character library.

**Boundary:** Creating new records is 12.4; its New character entry points arrive
with the working editor rather than inert placeholders. This temporary staged
omission must be recorded for review and does not count as final creator delivery.
Library CRUD must already be complete/testable via 12.2's API. No save/resume or
Continue menu entry. In-game exit/restart routes must have explicit semantics and
retain the accepted combat/reset behavior; do not silently erase a live session.

**Acceptance:** Verify default startup constructs no dungeon/session and shows
the approved menu; seeded and empty library, many-row scrolling, search/filter,
keyboard focus, View and delete confirm/cancel/failure; four-character selection,
all-one-class parties, formation, duplicates/stale records and invalid launch.
Verify correct live names/portraits/stats/equipment/spell access after launch,
exactly three camp kits, fresh-run resources and independent library data.
Exercise launch failure/retry, clean Quit, repeated menu/session lifecycle and
legacy diagnostic/capture bypass without OS input automation. Use synthetic input
tests, engine allocation guards and hidden captures at 1920×1080, 1280×720 and a
letterboxed aspect. CPU checks remain headless; separately verify GPU cleanup.

**Handoff:** Record application states and input ownership, selected-record and
formation representation, launch transaction, error UI, default-party shortcut,
menu capture fixtures and the return-to-caller hook needed by 12.4. Stop for review.
**Execution record:** Accepted and closed, 2026-10-29. Implemented the user-approved
[menu/launch design](menu_launch_design.md). [Verification and handoff](slice123_verification.md):
Debug/Release 35/35, focused sanitizers 3/3, allocation guard, hidden GPU lifecycle
and 37 captures. Startup no longer loads a dungeon before party launch. Confirmed
Main Menu and original-build/seed defeat restart follow the user's planning decisions.
Review correction implemented: drag available/assigned cards onto formation slots;
focused search fields blink a caret. Focused Debug/Release 2/2, sanitizer 1/1,
allocation guard and 49 hidden captures pass; see the same handoff.
**Acceptance:** 2026-10-29 — the user said “slice accepted, record it in the doc”. Acceptance includes the completed menu/library/party launch and all drag-and-drop/search-caret review corrections. Recorded verification limitations remain unchanged.

**Historical next-agent handoff at 12.3 closeout:** Slice 12.3 is accepted and closed. Slice 12.4 was next eligible for separate planning; its plan was subsequently approved and its implementation is now accepted and closed below. The [accepted closeout](slice123_verification.md#accepted-closeout) remains the predecessor contract.

### Slice 12.4: Complete creator and reusable selection flow

**Prerequisite:** 12.3 accepted. **Deliver:** Complete the approved creation flow
and its library/party entry points, integrate spell preparation with long rest,
and leave a tested reuse boundary for slice-13 advancement.

**Visual references:** [Identity 05](mockups/character-creator-v1/05-identity-and-portrait-v1.png),
[class 06](mockups/character-creator-v1/06-class-v1.png),
[abilities 07](mockups/character-creator-v1/07-point-buy-v1.png),
[skills 08](mockups/character-creator-v1/08-skills-reserved-v1.png),
[cantrips 09](mockups/character-creator-v1/09-cantrips-v1.png),
[spellbook 10](mockups/character-creator-v1/10-spellbook-v1.png),
[preparation 11](mockups/character-creator-v1/11-prepared-spells-v1.png),
[equipment 12](mockups/character-creator-v1/12-starting-equipment-v1.png),
[review 13](mockups/character-creator-v1/13-review-and-save-v1.png),
[cleric preparation 15](mockups/character-creator-v1/15-cleric-preparation-v1.png),
[discard confirmation 16](mockups/character-creator-v1/16-discard-draft-v1.png).
[Level-up example 14](mockups/character-creator-v1/14-level-up-reuse-v1.png) defines
the reuse target; it does not authorize implementing progression in this slice.

**Planning responsibilities:** Build a cohesive draft controller and reusable
panels using the 12.2 pure validation/query API. Define a short explicit step list
for creation and context-specific spell preparation rather than a generic workflow
engine. Inspect the accepted native overlays, input, recovery, party and spellbook
interfaces. Document layout, ownership, capacities, validation and transactional
commit before code. Actual widgets/text must remain crisp at native resolution.

**Required behavior/contracts:**

- New character from library returns there with the saved card visible; New
  character from the party picker preserves already selected companions and
  returns with the new character available for the intended empty slot. Do not
  auto-launch the dungeon or auto-replace an occupied slot on Save.
- Identity supports text editing/validation, race and five matching portraits.
  Class presents real supported features. Point buy has recommendation, reset,
  +/- costs, budget, racial bonuses and live final stats. Show validation reasons
  adjacent to fields and retain a usable path back from incomplete steps.
- Reserve Skills clearly and non-blockingly. Skip irrelevant spell stages:
  ordinary Fighters/Rogues have none; High Elf martial characters choose the one
  racial wizard cantrip; Clerics have cantrips/preparation; Wizards additionally
  choose a spellbook. Separate the racial budget and INT source where applicable.
- Spell cards show selection state/count, details and recommended picks; allow
  list scrolling without moving navigation. Distinguish inspected from selected,
  learned from prepared and domain/racial grants from user picks. A full budget
  disables new picks without preventing deselection. Known/granted choices are
  visibly locked in contexts where they cannot change.
- Equipment options and included supplies feed the authoritative preview; show
  equipped vs carried, weights, slots, AC and attacks. Use real existing stack
  behavior; the mockup's separate potion pictures do not change stack rules.
- Back/completed-step navigation preserves input. On race/class changes retain
  valid choices, clear invalid dependent choices and mark what needs attention;
  never silently save a stale portrait, old class spell or invalid loadout. Keep
  the name. Explain material resets and make recommended choices explicit.
- Review offers Change links and a final validation summary. Save is an atomic
  one-time library commit. Disk failure keeps the entire draft editable; repeated
  activation cannot create duplicates. Dirty Cancel/Escape uses screen 16; clean
  cancellation simply returns. Confirmations capture input exclusively.
- Add optional re-preparation to the existing safe Long Rest flow using the same
  preparation panel. Stage party choices without changing learned spells/cantrips;
  recheck safety, revision and camp supply before a single rest+preparation commit.
  Cancellation preserves the previous prepared lists and does not spend a kit.
  Keeping current preparation must allow the ordinary Long Rest path unchanged.
  Short Rest does not grant re-preparation. Define the exact commit point relative
  to the existing cosmetic camp playback and prevent duplicate recovery.

**Reuse contract:** The selection panel accepts character identity, an immutable
baseline, available options, already-known/locked grants, a choice allowance and
a draft result. Summary can show before/after values. Commit belongs to its caller:
library creation, safe rest, or later advancement. Write focused tests exercising
an advancement-shaped selection context with locked existing spells and newly
earned choices; do not enable XP controls, derived level-two stats or real level-up
buttons until slice 13. No new point-buy budget, portrait reset, supplies or copied
starting items on advancement. Preserve live session identity throughout.

**Acceptance:** Create and launch all sixteen race/class combinations with selected
portraits, legal point buy, spell counts/grants and equipment; inspect actual cast
availability and spell ability source. Test backtracking/invalidation, selection
limits, empty/filter results, minimum preparation, recommendation validity, dirty
cancel, duplicate Save, library-full and failed writes. Restart the app and verify
durable builds, names, portraits and independent live state. Exercise Long Rest
keep/change/cancel/failure/stale-state paths with exactly-once supply/recovery.
Capture every applicable approved screen, a non-caster, racial-only caster and
Cleric, errors/disabled states and a many-row library at native/reduced/letterboxed
sizes. Inspect the full menu → create → select four → dungeon loop. Run relevant
CPU/sanitizer/allocation/GPU checks; no desktop input automation. User reviews the
complete interlude before any slice-13 planning starts.

**Handoff:** Record reusable flow interfaces, final supported spell/grant selections,
library schema/portrait IDs, rest transaction, capture recipes, limitations and
accepted UI corrections. Update the source ledger/system design with implemented
status only after work lands. Slice 13 must use this flow and preserve custom
builds; slice 14 must save actual run state, not just refer back to blueprints.
**Execution record:** **Accepted and closed** (2026-10-29). Implemented the user-approved
[detailed design](character_creator_design.md): complete creator, shared selection/review,
checked library return, and staged Long Rest preparation. User selected preparation-only
Long Rest at normal cost and highlight-only return after Save from the picker.
[Verification and handoff](slice124_verification.md): Debug/Release 37/37, ASan/UBSan
4/4 with leak detection, zero tracked frame allocations, hidden GPU checks and
187 application-owned captures. The capture script creates/saves/reloads all sixteen
combinations and launches a created party. Interactive desktop use is unverified.
**Acceptance:** 2026-10-29 — the user said “Slice accepted, record it in the doc”.
Acceptance covers the delivered creator, checked library saving, shared selection/review
and Long Rest preparation. After discussing BG3, the user chose to keep the current
27-point buy, base-score range 8–15 and fixed racial ability bonuses for now; no
assignable-bonus or higher starting-score house rule is authorized. Recorded automated
verification and limitations remain as documented.

**Historical handoff at 12.4 closeout:** Slice 12.4 is accepted and closed. Slice 13
was next eligible; arena slice 12.5 has since been inserted, implemented and accepted.
Progression's later implementation is recorded below. At that handoff, the instruction was to read the
[accepted closeout](slice124_verification.md#accepted-closeout-and-next-agent-handoff)
and relevant code before proposing its plan. This documentation closeout does not
start slice 13.


### Slice 13: Levels 2 and 3

**Prerequisite:** Slice 12.5 arena accepted, following accepted 12.4; all 12.x
spells, custom builds and creator contracts are implemented and accepted.

**Deliver:** XP awards, normal advancement for presets and custom characters,
supported class/subclass choices, resources, higher features and curated
second-level spells. Reuse 12.4’s draft/choice/review flow; see the approved
[level-up mockup](mockups/character-creator-v1/14-level-up-reuse-v1.png), with Color
Spray replacing Ray of Sickness. Add development-only next-level XP grants for
every party member, capped to supported progression.

**Boundary:** Use normal XP mechanics, not custom objective scripts or milestone
leveling. Freeze the supported feature/spell list in the plan and ledger; do not
pretend unsupported features are functioning. Preserve custom names, portraits,
abilities, equipment, learned/prepared spells, domain/racial grants and casting
ability sources. Level-up commits to the live session, never its library blueprint.
Grant only newly earned choices (including two new wizard spellbook spells per
level); do not repeat point buy, creation or starting supplies. Review class-list,
spell/effect/icon/event capacities before adding second-level content; the 12.1
catalog already contains 24 spells. First-level expansion belongs to 12.1.

**Acceptance:** Each preset and representative custom race/class parties reach
level 3 through the shared advancement flow without losing their chosen identity.
Test thresholds, repeat requests, exactly-once XP rewards and features, derived
statistics/resources, and new spell behavior. User reviews the complete milestone C loop.

**Execution record:** **Accepted and closed**, 2026-10-30. User approved the plan with
“Implement the plan.” Delivered XP/atomic live advancement, shared new-choice/review
flow, development grants, fixed subclass features, optional-action sequencing and
eight second-level spells with prepared art. [Design](progression_design.md) and
[verification/handoff](slice13_verification.md): Debug/Release 41/41, ASan/UBSan 8/8
with leak detection, zero tracked allocations, maximum-capacity stress, hidden GPU
checks and 69 initial captures. Subsequent development-flag and shared-picker review
corrections, focused checks and refreshed captures are recorded in the handoff.
The user accepted the implementation, including Action Surge through the existing
class-feature bar: “I accept the slice, record it in the documentation.” This closes
slice 13 and milestone C. No additional Action Surge change was requested.
The subsequent enemy status-icon occlusion correction was also accepted and closed
on 2026-10-30: “yes that works, if this needs to be recorded as accepted do so”.
[Accepted closeout and next-agent handoff](slice13_verification.md#accepted-closeout-and-next-agent-handoff).
At closeout, slice 14 became next eligible. The subsequent
[automation insertion](automation_plan.md) places slices 13.1–13.3 first;
13.1 and 13.2 are now Accepted as recorded below.
13.3 and slice 14 remain Planned and unstarted.

### Arena automation interlude: slices 13.1–13.3

Slice 13.1 is implemented and **Accepted and closed** (2026-10-30), following the user's
“Implement the plan” instruction and subsequent “slice accepted, update the docs.” [Design](headless_design.md),
[protocol/client guide](headless_protocol.md), [verification/handoff](slice131_verification.md).
Diagnostic and visual headless sessions, checked action coverage, bounded stepping,
record/replay and graphics-free builds are delivered. Verification includes normal
Debug/Release 48/48 and graphics-disabled Debug/Release 30/30. The recorded verification
limitations remain unchanged. 13.2 is now implemented and Accepted; its
[design](automated_players_design.md), [usage](../experiments/arena/README.md) and
[verification](slice132_verification.md) record the delivered tools, explicit-only
playtesting workflow and limits. The user accepted and closed 13.2 on 2026-10-30:
“close up the slice”. 13.3 is now implemented, Accepted and closed; see its
[design](mcp_design.md), [usage](mcp_usage.md) and [verification/handoff](slice133_verification.md).

On 2026-10-30 the user requested recording the agreed three-slice direction before
the next gameplay slice. The dedicated [automation plan](automation_plan.md)
contains proposed shared contracts, scope, acceptance criteria, deferred work,
and per-slice execution records. Sequence: headless/reproducible arena play →
automated players and balance experiments → MCP play and investigation → slice 14.
At insertion all three were Planned and unstarted; it did not approve their detailed
implementation plans. The subsequent 13.1 delivery and acceptance are recorded above.

### Slice 14: Save and resume

**Disposition (2026-10-30): Superseded, unimplemented.** Requirements transfer to
[PC06](procedural_campaign_plan.md#pc06-campaign-save-and-resume) after generation
and persistent travel. The original scope below is retained for reference.

**Prerequisite:** Slice 13.3 accepted, following 13.1 and 13.2.

**Deliver:** One versioned out-of-combat save, validated restoration, safe file
replacement, and useful load/save failure reporting. Save authoritative gameplay
state and reconstruct derived data and presentation resources. Include custom
identities/portraits and actual learned/prepared/domain/racial spells, not a pointer
to a library entry. Restoration must work after its starting blueprint is deleted.
Keep the 12.2 library format and run-save format separate; reconstruct transient
effect/area identities rather than persisting runtime handles.

**Boundary:** No mid-fight or active-approach saves, autosave framework, or historical
version migration promise. Define unsupported-version behavior in the save plan.

**Acceptance:** Round-trip party position, builds, equipment, quantities, resources,
progression, and persistent monster/loot state. Test corrupt/truncated files,
unknown content, unsupported versions, failed writes, and invalid ownership.
Failures must not partially replace the running session or destroy a valid save.

**Execution record:** Superseded on 2026-10-30; unimplemented. Save/resume transfers to PC06; no verification or acceptance claimed.

### Slice 15: Cohesive presentation

**Disposition (2026-10-30): Retired by user.** Existing polish is sufficient; no
separate implementation or acceptance of this slice is claimed. Historical scope:

**Deliver:** Polish the twenty-portrait library from 12.2, curated item/spell icons, paper-doll silhouette,
readable management screens, targeting, damage/status feedback, and coherent art.
Review a concrete layout/art sample with the user before producing the whole set.

**Boundary:** Preserve the existing fantasy aesthetic and fixed presentation path.
No dressed-body equipment layers or renderer rewrite. Maintain UI/gameplay separation.

**Acceptance:** Inspect exploration, combat, inventory, equipment, spellbook,
rest, advancement, defeat, and save/load flows. Check scaling, input coordinates,
overlays, legibility, focus, no-clipping, and generated-art consistency through
deterministic captures and desktop interaction where available.

**Execution record:** Retired on 2026-10-30 by user direction; no separate implementation, verification or acceptance claimed.

### Slice 16: Demo integration

**Disposition (2026-10-30): Retired by user.** Static-dungeon tuning is obsolete
for the new direction. New campaign integration belongs to PC09. Historical scope:

**Deliver:** Configure encounters and recovery resources in the existing test
level, tune pacing, finish player-facing documentation, and verify the integrated demo.

**Boundary:** Do not add campaign infrastructure or broaden the agreed content
scope to fix pacing. Use the development XP control to inspect later levels.

**Acceptance:** A fresh session supports exploration, ranged approach, moving
goblin combat, looting, equipment, casting, recovery, progression, saving, and
resuming. Exercise defeat and restart/load. Run relevant Debug/Release tests,
allocation checks, and available desktop/capture checks. Record limits and obtain
user acceptance of milestone D before declaring the demo complete.

**Execution record:** Retired on 2026-10-30 by user direction; static-demo integration not executed or accepted.

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

Deferred beyond this roadmap: additional race/class content,
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
| 2026-10-30 | Close this roadmap; retire 15/16 and milestone D, transfer unimplemented save/resume 14 to campaign PC06 after generation/travel. Preserve all accepted history. [Successor roadmap](procedural_campaign_plan.md). | User says existing polish is sufficient, static-dungeon improvements are obsolete, and requests a multi-slice procedural MVP plan before saving. |
| 2026-10-30 | Accepted and closed 13.3 after capture-gallery review. [Closeout/handoff](slice133_verification.md#accepted-closeout-and-next-agent-handoff). Verification scope and limitations unchanged; tooling use stays open-ended and all balance experiments remain opt-in. Slice 14 is next eligible for separate planning and remains unstarted. | User: “I accept the slice, record it in the doc”. |
| 2026-10-30 | Accepted and closed 13.2. [Closeout/handoff](slice132_verification.md#accepted-closeout-and-next-agent-handoff). Verification scope and limitations unchanged. Use remains open-ended wherever relevant to project work; examples neither restrict its purpose nor add requirements. 13.3 is next eligible for separate planning; 14 stays after 13.3. | User accepted the executed checks as sufficient and instructed “close up the slice”; clarified examples must not restrict the tooling’s uses. |
| 2026-10-30 | Implemented 13.2 and marked Ready for review. [Verification](slice132_verification.md). Three policies/party packages, bounded serial experiments, honest reports and reproducible traces. All balance playthroughs remain opt-in, outside routine tests; no gameplay tuning. 13.3/14 remain unstarted. | User approved the detailed plan with “implement the plan”. Implementation acceptance remains pending. |
| 2026-10-30 | Start 13.2 planning only. [Proposed design](automated_players_design.md). Balance experiments, even small matrices, are explicit playtesting tools outside default CTest/build/CI and unrelated-change verification. Short fixture-owned tooling checks remain separate. Compare a few sensible, competently built parties; no universal viability/win-rate target or weak-build sweep. 13.3 and 14 stay unstarted. | User requested a detailed 13.2 plan for review, rejected running balance playthroughs on every change, and clarified that deliberately weak/novelty parties need not win. |
| 2026-10-30 | Accepted and closed slice 13.1. Verification scope/limitations unchanged. 13.2 becomes next eligible for separate planning; 13.2/13.3 remain Planned and unstarted. [Closeout/handoff](slice131_verification.md#accepted-closeout-and-next-agent-handoff). | User: “slice accepted, update the docs.” Requested a next-slice prompt for a fresh context. |
| 2026-10-30 | Insert tooling slices 13.1–13.3 before save/resume: headless reproducible arena play, automated players/balance experiments, then MCP play/investigation. Separate [plan](automation_plan.md); all Planned, detailed implementation plans pending. Milestone C stays Accepted. | User requested recording the three-slice direction in the demo plan before the next slice or in a dedicated plan. |
| 2026-10-30 | Slice 13.1 implemented and Ready for review: headless arena protocol, shared metadata/visibility, bounded stepping, strict replay and scripted client. [Verification](slice131_verification.md). 13.2/13.3 remain Planned; no acceptance claimed. | User: “Implement the plan.” |
| 2026-10-30 | Accepted and closed the enemy status-icon occlusion follow-up. No review remains pending; slice 14 stays unstarted. | User: “yes that works, if this needs to be recorded as accepted do so”. |
| 2026-10-30 | Accepted and closed slice 13 and milestone C, including the development flag and spell-picker corrections. Existing Action Surge behavior accepted. Slice 14 becomes next eligible for separate planning and remains unstarted. | User: “I accept the slice, record it in the documentation.” |
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
| 2026-10-25 | Original sixteen slices, four milestones, durable handoffs, and user acceptance between slices; no full-auto execution. Later interludes retain this review workflow. | User requested self-tracking workflow and approved its documentation implementation. |
| 2026-10-28 | Accepted the sixteen-screen character-creator proposal, main menu, Start-new-game party selection, persistent library, point buy, five portraits per race, skills placeholder, starting equipment and reusable spell-selection flow. Insert 12.1–12.4 before 13. | User: “Proposal accepted, go make the plan”; implementation still receives separate per-slice planning/review. |
| 2026-10-29 | Accepted and closed slice 12.1, including all spell, visual and portrait follow-ups. Slice 12.2 becomes next eligible for planning; it remains unstarted. | User: “nice work, record the slice as accepted”. |
| 2026-10-29 | Accepted and closed slice 12.2, including character builds, library storage, spell access and portraits. Slice 12.3 becomes next eligible for planning; it remains unstarted. | User: “slice accepted, record it in the plan doc, so we can get started on the next slice”. |
| 2026-10-29 | Accepted and closed slice 12.3, including main menu, library, party launch, formation dragging and search caret. Slice 12.4 becomes next eligible for planning and remains unstarted. | User: “slice accepted, record it in the doc”. |
| 2026-10-29 | Accepted and closed slice 12.4. Retain 27-point buy, base scores 8–15 and fixed racial bonuses for now. Slice 13 becomes next eligible for separate planning and remains unstarted. | User: “okay ill roll with the standard rules for now, try it out :). Slice accepted, record it in the doc”. |
| 2026-10-28 | Expand the level-one/cantrip catalog in a dedicated 12.x slice before second-level spells in slice 13; normal creation selection counts replace unrestricted class-wide availability. | Explicit user approval of the expanded catalog assumption. |
| 2026-10-28 | Color Spray replaces illustrative Ray of Sickness in screens 10/14 and the implementation target; retain SRD 5.1 scope. | User: “use color spray for now”. |

Character-creation roadmap verification (2026-10-28): checked 212 local Markdown
links/anchors across the roadmap, system overview, flow and source ledger; HTML
gallery links; sixteen image/provenance records; 23 tracker entries; ordered
12.1–12.4 dependencies and Planned records; slice 13's new prerequisite; and
`git diff --check`. The visual acceptance and Color Spray correction are recorded
in the flow, gallery captions, provenance and decision log. This was documentation
work only: no gameplay builds/tests or desktop automation ran, and no new slice
implementation or acceptance was claimed at that checkpoint. The current 12.1
implementation/review state is recorded above.

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

Roadmap amendment (2026-10-26): user requested graphical subslices 8.1–8.3
between accepted 08 and Planned 09, with clean-context planning and implementation
for each. Their scope and shared decisions are recorded above. Dependency chain
is 08 → 8.1 → 8.2 → 8.3 → 09; milestone B stays Accepted. The graphical
interlude has not started, and no detailed implementation plan is yet approved.

## Arena insertion decision (2026-10-29)

The user chose arena before progression, level one initially, defaults plus saved
characters, portable scenario files, and Rematch/Edit Setup. Creature-size footprints
are explicitly deferred. This approved insertion changes slice 13’s prerequisite to
12.5 without changing the content of the level 2–3 slice.

## Slice 12.5 execution record

Approved for implementation: arena before levels 2–3, default/saved level-one party,
portable scenario files, generated room, Rematch/Edit Setup. Design and validation
strategy: [arena](arena_design.md). Creature-size footprints explicitly deferred.
User: “Implement the plan.” Status: **Accepted and closed**, 2026-10-29.

Delivered the approved [design](arena_design.md): main-menu setup, default/saved
party snapshots, monster quantities, seed, versioned portable scenarios, generated
room, normal gameplay and Rematch/Edit Setup. [Verification/handoff](slice125_verification.md):
Debug/Release 38/38, ASan/UBSan 5/5 with leak detection, zero tracked frame allocations,
256-actor stress, hidden GPU checks and 54 captures. The user accepted the slice on
2026-10-29: “slice accepted, record it in the doc”. Verification results and their
limitations remain unchanged. At that closeout, slice 13 became next eligible for
separate planning; its subsequent implementation is recorded in the current
checkpoint and slice 13 execution record.

## Slice 13.3 execution record

2026-10-30: Implemented, **Accepted and closed**. User authorized with “Implement the plan.”
[Design](mcp_design.md), [usage/configuration](mcp_usage.md), [verification/handoff](slice133_verification.md).
Session play, bounded explicit jobs, honest results, strict reproduction/investigation
and application-owned replay captures reuse the accepted systems. 19 fast contracts;
focused headless Debug/Release 6/6 and graphics 7/7; sanitizer, SDK lifecycle/cancellation,
opt-in smoke/reproduction, hidden GPU checks and seven captures. Live Codex-host and
interactive desktop review remain unverified. No OS input/focus control; no automatic
balance checks or gameplay changes. 13.1/13.2 remain closed; 14 remains unstarted.
The user reviewed the capture gallery and accepted: “I accept the slice, record it in the doc”.
[Closeout/handoff](slice133_verification.md#accepted-closeout-and-next-agent-handoff).
Slice 14 is next eligible for separate planning; do not start it automatically. No commit requested.
