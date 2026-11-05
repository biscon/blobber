# Early bestiary: design and delivery roadmap

Created: 2026-10-31. Last updated: 2026-11-01.
Status: **Closed for this expansion (2026-11-01). B01–B05 Accepted and closed; B06 Superseded without implementation by campaign PC07 planning.**

The user requested a modest slice plan for new monsters, implemented and reviewed
in the arena **before campaign integration**. The roster is skeleton, bugbear,
standard giant rat, zombie and goblin shaman, alongside the existing melee goblin
and goblin archer. Giant rat replaces the proposed centipede; poison recovery is
outside this expansion. Every new monster includes production imagegen artwork
and the goblins' applicable animation set, in a coherent visual style.

The first three campaign floors are a polished vertical slice for testing fun,
not the complete game. The long-term direction is roughly 20+ floors. This roster
serves the opening character levels, within the currently supported **levels 1–3**;
whether the eventual three-floor route ends around level two or three remains a
campaign pacing decision. Floor number does not imply character level or CR.

The five monster slices are accepted. The user chose to close this roadmap and
transfer B06's useful preparation/calibration scope to
[campaign PC07](procedural_campaign_plan.md#pc07-depth-bands-and-finite-recovery-encounters).
Population, guardians, XP and recovery pacing now belong to that campaign plan.
PC07 is next for detailed planning; no campaign implementation or balance
experiment has started as part of this closeout.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Accepted foundation | Existing goblins, arena, character progression through level 3; renderer R01–R06 and bestiary B01–B05 closed |
| Active slice | None; this expansion is closed with B01–B05 accepted |
| Awaiting review | None; B05 reference v1 and completed slice accepted |
| Next eligible slice | Campaign PC07; B06 superseded without implementation |
| Next action | Plan PC07's population, progression and targeted calibration under the campaign roadmap |
| Campaign status | Planning resumes after accepted P-B; implementation and experiments unstarted |
| Deferred size work | Two-/four-subcell actors; all new monsters use one subcell |

Update checkpoint, tracker and affected execution record together. Roadmap
approval, detailed-plan approval, implementation and user acceptance are distinct.
Only the user accepts a delivered slice.

## Delivery workflow

1. Read [AGENTS.md](../AGENTS.md), [system design](system_design.md), this checkpoint
   and the preceding slice's handoff; inspect current code and preserve local work.
2. Work on one slice at a time. Write its practical design under `docs/`, link it
   from the system overview, and obtain plan review before implementation. Existing
   explicit approval suffices; do not re-request settled decisions.
3. Deliver mechanics, generated art, arena availability, feedback and affected
   automation/persistence together. Internal fixtures may use generated simple art;
   a finished monster may not ship with a placeholder or missing normal poses.
4. Record exact checks, limitations, retained captures and next-agent instructions
   in `docs/bNN_verification.md`; keep designs aligned with the result.
5. Mark **Ready for review**. After user acceptance mark **Accepted and closed**;
   identify the next planning step without automatically implementing it.

Use the existing statuses: Planned, In progress — planning/implementation, Ready
for review, Accepted, Blocked, Superseded/Retired. B01–B05 are delivered and
accepted; B06 is superseded by campaign planning. Retain the historical slice IDs
and evidence rather than treating transferred work as implemented.

## Roster and experience

| Addition | Arena role | Player decision | Footprint |
| --- | --- | --- | --- |
| Skeleton | Readable undead melee opponent | Bring blunt damage; choose effective spells | One subcell, Medium |
| Bugbear | Heavy front-line threat | Control or focus it before its next blow | One subcell, Medium |
| Giant rat | Weak individual, dangerous group | Thin the pack to remove its advantage | One subcell, Small |
| Zombie | Durable, low-AC opponent | Choose a reliable finishing attack | One subcell, Medium |
| Goblin shaman | Protected support and save-based damage | Reach the caster or break concentration | One subcell, Small goblin |

SRD source: [official 5.1 CC edition](https://media.wizards.com/2023/downloads/dnd/SRD_CC_v5.1.pdf),
printed pp. 266, 346, 356–357, 378 and 395. These are source baselines, not claims
that every tabletop ability is implemented. Record adopted values, omitted traits,
XP/loot choices and adaptations in the [source ledger](srd_content_ledger.md) as
each monster lands. Fixed HP and existing grid/formation/action rules remain the
starting point. CR supplies context; it is not a tested arena difficulty label.

Avoid mandatory cleansing, equipment damage, random loss of party control and
unavoidable rear-character focus in this expansion. The roster should reward
different attacks, spells and priorities while allowing ordinary parties to play.

## Shared implementation boundaries

### Combat, ownership and bounded work

Keep immutable species/attack data in `rules::Catalog`, actor identity/HP/occupancy
in `ActorWorld`, and action decisions, costs and impact scheduling in combat.
Use small typed fields/functions for the concrete traits below; no generic ability
scripting language, behavior tree or entity framework. Split cohesive combat helpers
when needed instead of growing application entry points.

Carry damage type and critical/source information through the authoritative damage
path, including delayed and multi-hit spells. Resolve mitigation consistently for
all sources before lethal consequences. Preserve accepted attack rolls and stable
impact ordering; document any new RNG at impact explicitly. A surviving zombie
must never briefly publish death, free occupancy or trigger rewards.

B05 adds a tagged effect source identifying a party character or world-checked
ActorId, reusing existing identity types. Do not use NPC catalog identity as caster
identity or borrow long-lived actor pointers. Support NPC Bless ownership and its
damage/incapacitation/removal cleanup while retaining party-only restrictions for
unrelated spells. Keep NPC resource/action state in bounded session-owned storage.

Retain the current 32 monster definitions, 32 appearances, 256 actors, 64 active
effect instances and the current prepared-world byte budgets. B02 explicitly
raises mip/GPU ceilings to 512/608 MiB and adds full-resolution NPC cropping;
alpha/CPU and count limits remain fixed. Derive event/timeline bounds
when adding NPC casts; reject capacity exhaustion before costs/RNG, never drop
consequences. Preparation can allocate; engine-owned gameplay frames cannot.
Pack checks scan bounded actors only when an attack is accepted, not per pixel or
per frame. Do not increase budgets or capacities speculatively for 20+ floors.

### Arena first; existing campaign safety

Each monster becomes selectable in normal Arena Setup when its appearance and
rules are available. Reuse existing scenario IDs, quantities, seed, rematch and
editing. Manual review currently uses level-one arena parties; focused level-two/
three checks may construct parties through the existing advancement API. The
proposed reusable level preparation now belongs to PC07. Do not describe those
checks as a delivered arena UI.

Shared combat changes must preserve current campaign save/travel behavior. If
saved structures or simulation meaning change, update validation/version identity
and perform the affected persistence checks in that slice. Existing policy is
explicit incompatible-save rejection, without migration or rewriting rejected files.
This compatibility work is required even though campaign population is deferred.
NPC-owned effects/resources stay with their location and must not accidentally
travel as party-owned portable effects.

### Art and animation contract — applies to B01–B05

Use the **imagegen skill and built-in image generation tool** for the final raster
art, following the established goblin/archer workflow. Inspect actual shipped
goblin masters/poses and use them as image references, not just a textual style
description. Generate a four-direction reference and review it beside both goblins
before producing that monster's full set. Reference approval and completed-slice
acceptance are separate; future unseen art is not approved by this roadmap.

Match the detailed shaded, textured retro dungeon look, muted material colors,
lighting, edge detail and apparent world scale. Give each creature its own anatomy
and silhouette. A rat stays rat-sized and a bugbear looks bulkier than a goblin;
neither gets a different occupancy footprint. Keep identities/equipment consistent
across directions, avoid mirrored handedness, and verify the result inside the
actual low-resolution dungeon beside existing goblins.

| Clip | Goblin-equivalent delivery baseline |
| --- | --- |
| Idle | Front/back/left/right; two frames each, 600/600 ms |
| Walk | Four directions; two frames each, 300/300 ms |
| Melee attack | Front-facing four-frame action, matching the existing melee contract |
| Hurt | Four directions; two frames each, 150/150 ms |
| Death/corpse | Four directions; four frames each, 200/200/200/600 ms |
| Prone | Four directions; one registered pose each, shared by relevant conditions |
| Ranged/cast, when used | Four directions; explicit release/impact registration, following the archer's two-frame action contract |

Every monster receives the full applicable set, including prone for undead even
when sleep-immune. Timing changes justified by anatomy must be specified and
reviewed in that slice; do not deliver a stiff idle-only substitute. Melee-only
creatures need no unused bow animation. The shaman needs both its melee fallback
and a readable casting action, with a distinct presentation from an arrow shot.

Preserve transparent masters, exact prompts/references/corrections, measured
registration and metadata under `assets/npcs/<monster>/`. Use the existing offline
extraction, mip/alpha preparation, animation selection, shadow and picking pipeline.
Keep ground/pivots stable; never normalize collapsing frames to standing height.
Use anatomy-appropriate canvases with unclipped limbs/tails/weapons. Validate all
directions, diagonal front cells, near/far sizes, overlapping ranks, alpha holes,
hurt interruptions, death and readable selection. No renderer replacement or
per-frame image generation/decoding is part of this work.

## Slice tracker

| ID | Slice | Dependency | Status |
| --- | --- | --- | --- |
| B01 | [Skeleton and typed damage](b01_skeleton_design.md) | Roadmap accepted | Accepted and closed (2026-10-31) |
| B02 | [Bugbear and sprite memory preparation](b02_bugbear_design.md) | B01 accepted | Accepted and closed (2026-10-31) |
| B03 | [Giant rat and Pack Tactics](b03_giant_rat_design.md) | B02 accepted | Accepted and closed (2026-10-31) |
| B04 | [Zombie and Undead Fortitude](b04_zombie_design.md) | B03 accepted | Accepted and closed (2026-11-01) |
| B05 | [Goblin shaman and enemy spellcasting](b05_goblin_shaman_design.md) | B04 accepted | Accepted and closed (2026-11-01) |
| B06 | Arena preparation and roster polish | B05 accepted | Superseded (2026-11-01); useful scope transferred to PC07, unimplemented |

### B01 — Skeleton and typed damage

Deliver one melee skeleton, its full applicable art set and normal arena selection.
Add a compact monster damage profile and shared damage handling for bludgeoning
vulnerability and poison immunity. Use existing sleep immunity/non-humanoid checks
for the relevant spell interactions; do not introduce every absent condition.
The initial melee-only loadout is an explicit adaptation, not a hidden bow ability.

Verify equivalent behavior across weapon hits, immediate/delayed spells, areas and
individual multi-hit impacts. Immune damage must not produce damage-driven effects
or consume damage-save RNG; criticals, zero damage, HP, logs and kill credit must
remain consistent. Arena review contrasts blunt and ordinary weapon attacks and
an ineffective poison spell, with concise feedback explaining the difference.

**Exit:** a finished skeleton is playable and visibly coherent in the arena;
typed damage and undead spell restrictions have meaningful regression coverage.

### B02 — Bugbear

Deliver a bulky but one-subcell melee bugbear with the full art set. Its source
weapon damage already includes Brute; do not apply the extra damage twice. Retain
ordinary front protection and range. Initial scope omits the separate Surprise
Attack feature and thrown weapon; record these adaptations, with no reach system.

Review a lone bugbear and a small goblin-supported encounter. Check normal melee
approach, front blocking, control, hurt/death timing and damage spikes. Quantities
are arena recipes, not promises that this is the first campaign guardian. Level-one
review must clearly identify a dangerous encounter rather than silently weakening
the monster or granting hidden party protection.

**Exit:** the heavy role reads through silhouette and attack presentation; combat
and arena rematch work without new footprint or reach mechanics.

### B03 — Giant rat and Pack Tactics

Deliver the standard giant rat, with no diseased variant or poison, and its full
applicable art set. Pack Tactics grants attack advantage when another conscious
ally is in melee contact with the shared party cell. Use the existing conservative
contact/LOS rules; check same-cell allies, exclude the attacker, and handle removed,
dead and incapacitated supporters. The detailed plan must distinguish incapacity
from merely being prone. No individual ally-to-party adjacency simulation.

Verify ordinary advantage/disadvantage cancellation and support removal through
killing or control. Compare a lone rat with a small pack, using bounded encounters
rather than a swarm/capacity stress case. Check that its low silhouette is easy to
notice and select among feet, shadows and corpses without artificially enlarging it.

**Exit:** thinning or disabling the pack changes observable attacks; the rat is
visually readable and has no lingering ailment/recovery requirement.

### B04 — Zombie and Undead Fortitude

**Accepted and closed (2026-11-01).** [Design](b04_zombie_design.md) ·
[verification/gallery](verification/archive/b04_verification.md). Reference v1 and completed slice accepted.

Deliver the zombie and its full applicable art set, reusing B01's damage profile
for poison immunity and the existing undead spell restrictions. Undead Fortitude
uses a Constitution save against lethal damage; radiant damage and critical hits
bypass it. On success retain one HP, occupancy and actor identity. Resolve this
before death markers, animation, loot, XP, arena-clear or guardian notifications.

Check save success/failure, bypasses, zero/nonlethal damage, repeated impacts and
exactly-once rewards. Provide clear survival feedback; the creature must not appear
to die and resurrect. Arena review compares ordinary finishing attacks with radiant
finishers. Do not invent a radiant vulnerability or universally double damage.
Review pacing: a few durable targets should be interesting, not an exhausting wall.

**Exit:** death prevention is atomic and understandable, with dependable finishers
and no incorrect death/reward or save/restore transitions.

### B05 — Goblin shaman and enemy spellcasting

**Accepted and closed (2026-11-01).** [Design](b05_goblin_shaman_design.md) ·
[verification/gallery](verification/archive/b05_verification.md). Reference v1 and the completed slice
accepted by the user; verification evidence and limitations remain recorded.

Deliver a visibly distinct goblin caster, full art set and readable casting action.
Use the Acolyte as a baseline with **Bless, Sacred Flame and a weak melee fallback**.
Delivered resource default: six NPC mana, two per Bless, free cantrip; no combat
regeneration or silent refill at an encounter boundary. This is a project adaptation.

Use a small deterministic policy: cast Bless when useful on up to three eligible
allies and not already concentrating; otherwise use a legal Sacred Flame target,
reposition or fall back to melee. Preserve one-action movement/casting decisions,
LOS, facing, range, incapacitation and cooldown rules. Select legal party targets
without automatic weakest-character execution. Do not import healing, Sanctuary,
or a generic caster policy framework in this slice.

Implement the tagged NPC effect source, concentration damage saves/cleanup,
resources and feedback together. Blessing and damaging casts must produce normal
events/observations with clear caster identity. Audit bounded event storage and
replay identity; no special permissive automation path. A disabled/dead source
must not leave a permanent buff or make a second cast while resolving the first.

Review the shaman alone and behind goblins, including attacking its concentration
and reaching it with ranged attacks. The existing arena's ordered roster can form
front/rear arrangements; verify actual approach behavior rather than trusting the
starting rows. Avoid a general placement editor.

**Exit:** enemy support/cantrip behavior is legible and counterable; ownership,
capacity rejection, resources, pause, interruption and persistence checks pass.

### B06 — Arena preparation and roster polish

**Superseded without implementation (2026-11-01).** The user chose to close the
bestiary expansion after accepted B01–B05. Reproducible level preparation and
targeted encounter measurements now belong to
[PC07](procedural_campaign_plan.md#pc07-depth-bands-and-finite-recovery-encounters),
where they serve population, XP and recovery decisions. General arena/roster
polish is deferred unless campaign review exposes a concrete need. B06 is not an
acceptance prerequisite for campaign work, and no B06 results are claimed.

The original proposal below is retained as history, not an active delivery gate.

Add bounded arena preparation for party levels 1–3 using existing advancement
rules and actual spell choices. Preserve level-one character-library snapshots.
Store the arena target level and required advancement choices as scenario-owned
values; prepare a fresh party outside simulation and retain an immutable launch
specification for rematch. Reuse advancement selection/validation, not a second
character progression implementation. Start with full ordinary resources for that
level, the same starting equipment, and no hidden consumable grants.

Saved arena scenarios and automation must reproduce those choices. Define a
versioned format, preserve supported old level-one scenarios with explicit defaults,
reject invalid choices and leave the current setup intact on failure. No general
party editor, campaign import, equipment shop or formation editor rewrite.

Retain a small review set: blunt-vs-skeleton, lone-vs-pack rats, zombie finisher,
lone bugbear, goblin front with archer/shaman support, and an undead group. Reuse
normal scenario loading; do not restore retired standalone demo/capture runners.
Record roster order, quantity, seed, party choices, intended level and actual
limitations. If launch order fails to demonstrate support ranks, correct the
small arena placement policy and version it explicitly; do not add a map editor.

Review readability, target selection, reaction/attack timing, explanations, battle
length and whether choices feel rewarding. Preserve rematch/edit/reset ownership
and review all seven appearances together inside the resource budgets. User arena
feedback drives focused corrections. Automated correctness is not evidence of fun.

**Exit:** all five additions are accepted and ordinary arena review works across
the intended low-level range. Record an explicit full-roster user review and a
campaign handoff, with no claim of campaign balance or automatic PC07 resumption.

## Verification and review cost

Follow [verification policy](verification_workflow.md): focused changed-code tests
in one configuration, one intensive job at a time, at most two build workers and
one test process; low CPU/I/O priority for lengthy work. No routine benchmark,
capacity sweep, repeated full build matrix or unattended balance expedition.

Use small self-owned generated fixtures for CPU rules, trait interactions, resources,
event order, failure paths and allocation guards. Shared state changes get targeted
save smoke/roundtrip and strict replay/observation checks; broad persistence runs
belong at the relevant integration checkpoint. Production art gets one affected
bake and selected hidden application-owned GPU captures, with sprite/ID parity and
resource-lifetime checks where affected. Never control OS input, focus or desktops.
Human interactive arena review belongs to the user; report unavailable/unexecuted
checks honestly. Arena testing requested here does not authorize broad automated
balance sweeps; propose a concrete bounded experiment separately if needed.

## Deferred work and campaign handoff

Two-/four-subcell creatures require a later coordinated footprint design covering
placement, occupancy masks, movement/pathfinding, facing/rotation, formation
protection, targeting, rendering anchors and persistence. Image width is not a
collision footprint. All selected creatures fit the existing one-subcell model;
no placeholder footprint subsystem is required now.

The user has directed the return to campaign planning. PC07 owns species/recipe
selection, depth bands, guardian compositions, rewards, required-path XP and
recovery pacing, plus the useful preparation/calibration scope transferred from
B06. Start with campaign targets, measure candidate encounters, then check
accumulated difficulty on generated routes. Existing arena evidence establishes
monster behavior, not dungeon balance. A reviewed detailed PC07 plan and explicit
bounded experiment opt-in still precede implementation and balance runs.
Long-term 20+ floor count, higher-level progression, later monster tiers,
poison/cleansing, corrosion and the mouther remain separate work.

## Decision and execution record

- **2026-10-31:** User requested a restrained slice roadmap matching existing plans;
  arena implementation/review must precede campaign integration. Three floors are
  the opening vertical slice of an eventual roughly 20+ floor game. Poison enemy
  replacement proposed: standard giant rat. No poison/disease recovery in scope.
- **2026-10-31:** User requested the existing goblin imagegen/animation workflow and
  coherent art style. All five monsters include complete applicable poses. Larger
  two-/four-subcell occupancy is deferred because this roster does not require it.
- **Planning delivery:** Repository/SRD review and documentation checks only. No
  gameplay implementation, image generation, builds, captures, balance runs or
  commits. Next work is roadmap review, then B01 detailed planning when requested.

- **2026-10-31 B01 start:** User accepted the roadmap and approved the detailed B01 plan. Implementation is active; art reference review precedes full sheets.

- **2026-10-31 B01 delivery:** Skeleton reference approved; complete 48-pose production
  art, native portrait and typed damage delivered in the normal arena. Focused
  Release CPU checks, hidden GPU integration and three strict production-asset
  arena replays pass. [Handoff and evidence](verification/archive/b01_verification.md) retain exact
  results and limitations. **Ready for review**, awaiting user acceptance;
  B02 remains Planned and campaign work remains paused.

- **2026-10-31 B01 acceptance:** User: “cool slice accepted, record it in the plan
  doc, so we can get started on the next mob”. **B01 Accepted and closed**, including
  its full skeleton art/mechanics, 12.5% appearance reduction, right-aligned
  initiative and head-centered health bars. Existing verification evidence and
  limitations remain recorded in the [handoff](verification/archive/b01_verification.md). **B02 — Bugbear
  is next eligible for detailed planning**, beginning with the approximately
  4.8 MiB remaining mip budget. B02 implementation is not started; B03–B06 remain
  Planned and campaign integration/PC07 stay paused.

- **2026-10-31 B02 approval and delivery:** User approved the detailed plan with
  “Implement the plan” and accepted the turnaround with “reference accepted,
  looks great”. Full bugbear mechanics, 48-pose art, initiative portrait and normal
  arena binding are delivered. User explicitly authorized a 512 MiB texture
  budget while preserving prepared detail; NPC crop offsets/shared CPU/GPU
  geometry reduce the four-appearance publication to 173.65 MiB mip textures /
  30.22 MiB alpha. No atlas or streaming system was added. Focused Release CPU,
  publication/build, hidden GPU and six strict production replay checks pass.
  [Verification/gallery](verification/archive/b02_verification.md) records evidence and limitations.
  **B02 Ready for review**, awaiting completed-slice user acceptance. B03 remains
  Planned; campaign integration and PC07 stay paused.

- **2026-10-31 B02 acceptance:** User: “slice accepted, record it in the plan doc”.
  **B02 Accepted and closed**, including bugbear mechanics, full 48-pose art,
  initiative portrait, arena integration, transparent-margin cropping and revised
  texture/GPU budgets. Existing [verification evidence and limitations](verification/archive/b02_verification.md)
  remain recorded. **B03 — Giant rat and Pack Tactics** is next for detailed
  planning; implementation is unstarted. Campaign integration and PC07 stay paused.

- **2026-10-31 B03 approval:** User approved the detailed plan with “Implement the plan.” Quick bite uses 300/150/150/300 ms, impact 450 ms. B03 implementation is active under its [design](b03_giant_rat_design.md); art reference and completed-slice acceptance remain pending. Campaign/PC07 stay paused.

- **2026-10-31 B03 reference acceptance:** User requested a less cute, more vicious rat and approved turnaround-v3 with “approved, thats a mean and nasty rat :)”. Full production animation is now authorized; completed-slice acceptance remains pending.

- **2026-10-31 B03 delivery:** Standard giant rat, bounded acceptance-time Pack
  Tactics, quick bite, full 48-pose production art, native portrait and normal arena
  integration are delivered. Schema 11 / campaign simulation 5 preserve explicit
  incompatible-save rejection. Publication is 232.81 MiB mip / 41.32 MiB alpha,
  within unchanged budgets. Focused CPU, hidden GPU integration and five strict
  production arena replays/captures pass. [Handoff/gallery](verification/archive/b03_verification.md)
  records exact checks and limitations. **B03 Ready for review**; reference v3 is
  accepted, completed-slice acceptance pending. B04/campaign/PC07 remain unstarted.

- **2026-10-31 B03 acceptance:** User: “slice accepted, record it in the plan doc”.
  **B03 Accepted and closed**, including standard giant rat mechanics, Pack Tactics,
  quick bite, full 48-pose art, native portrait, arena integration and affected
  automation/save identities. Existing [verification evidence and limitations](verification/archive/b03_verification.md)
  remain recorded. **B04 — Zombie and Undead Fortitude** is next for separate
  detailed planning; implementation is unstarted. Campaign integration/PC07 stay paused.

- **2026-10-31 B04 approval:** User approved the detailed plan with “Implement the plan.”
  Decayed villager direction selected; [B04 design](b04_zombie_design.md) records
  atomic Fortitude, complete art, arena and compatibility work. Implementation is
  active; reference acceptance and completed-slice acceptance remain pending.
  Campaign integration and PC07 stay paused.

- **2026-10-31 B04 reference acceptance:** User: “reference accepted”.
  Turnaround v1 is accepted; full production animation is authorized. Completed
  B04 acceptance remains pending; campaign/PC07 stay paused.

- **2026-11-01 B04 delivery:** Zombie, atomic Undead Fortitude, full 48-pose
  production art, native portrait and normal arena integration are delivered.
  Schema 12 / campaign simulation 6 retain explicit incompatible-save rejection.
  Six-appearance publication is 260.95 MiB mip / 46.60 MiB alpha within unchanged
  budgets. Seventeen focused CPU checks, hidden GPU integration and four strict
  production arena traces/captures pass; later hurt/corpse captures also match.
  [Verification/gallery](verification/archive/b04_verification.md) records exact checks and limitations.
  **B04 Ready for review**; reference v1 accepted, completed-slice acceptance pending.
  B05–B06 remain Planned; campaign integration/PC07 stay paused.

- **2026-11-01 B04 acceptance:** User: “slice accepted, record it in the plan doc”.
  **B04 Accepted and closed**, including zombie mechanics, atomic Undead Fortitude,
  full 48-pose production art, native portrait, normal arena integration and
  affected automation/save identities. Existing [verification evidence and
  limitations](verification/archive/b04_verification.md) remain recorded. **B05 — Goblin shaman and
  enemy spellcasting** is next for separate detailed planning; implementation is
  unstarted. B06 remains Planned; campaign integration and PC07 stay paused.

- **2026-11-01 B05 approval:** User approved the detailed plan with “Implement the plan.” Ally-first Bless, rotating conscious party targets and tribal staff caster art selected. Implementation active under the [design](b05_goblin_shaman_design.md); reference and completed-slice acceptance pending. Campaign/PC07 and B06 remain unstarted.

- **2026-11-01 B05 reference acceptance:** User: “reference approved”. Turnaround v1 accepted; full production animation is authorized. Completed-slice acceptance remains pending.

- **2026-11-01 B05 delivery:** Goblin shaman, tagged NPC Bless/concentration, six-mana
  resources, rotating Sacred Flame targets, support positioning, full 56-pose art,
  native portrait and normal arena binding delivered. Schema 13 /campaign 4/7
  retain explicit incompatible-save rejection. Publication is 314.56 MiB mip /
  56.67 MiB alpha within unchanged limits. Focused CPU/allocation/persistence,
  hidden GPU and four strict arena traces/captures pass;
  [verification/gallery](verification/archive/b05_verification.md) records exact checks and limitations.
  **B05 Ready for review**; completed-slice acceptance pending. B06 and campaign/PC07
  remain unstarted.

- **2026-11-01 B05 acceptance:** User: “slice accepted, record it in the plan doc”.
  **B05 Accepted and closed**, including goblin shaman mechanics, tagged NPC
  Bless/concentration, mana persistence, casting timelines, full 56-pose production
  art, native portrait, normal arena integration and affected automation/save
  identities. Existing [verification evidence and limitations](verification/archive/b05_verification.md)
  remain recorded. **B06 — Arena preparation and roster polish** is next for
  separate detailed planning; implementation is unstarted. Campaign integration
  and PC07 stay paused.

- **2026-11-01 roadmap closeout and campaign handoff:** User proposed closing the
  bestiary plan and moving B06 into the procedural campaign plan. This expansion
  is **closed with B01–B05 Accepted and closed**. **B06 is Superseded without
  implementation**; its reproducible level preparation and useful encounter
  measurements transfer to PC07 to inform population, XP and recovery pacing.
  General arena polish is deferred. Campaign planning resumes with PC07; its
  detailed implementation plan remains unapproved. No new gameplay results,
  balance experiments or campaign implementation are claimed by this closeout.
