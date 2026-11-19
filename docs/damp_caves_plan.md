# Damp Caves expansion — design and delivery roadmap

Created 2026-11-18. **DC01 and DC02 are Accepted and closed. DC03–DC07 are Planned.**

Extend the playable descent from three floors to six. Below the Underkeep, water
has broken through an old cistern into limestone galleries occupied by venomous
spiders and blind cave newts. The surveyors' abandoned route leads to a broodmother
in the lowest vault. The gameplay arc is to recognize a danger, choose an approach,
then decide which threat deserves the party's next action.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Starting point | Three persistent Underkeep floors. The third guardian ends the campaign. |
| Last closed slice | [DC02 — Limestone kit and survey trail](dc02_limestone_kit_design.md), accepted and closed by the player on 2026-11-19 after the pool atmosphere revision. |
| Current review | None. |
| Next eligible slice | DC03 — Venomous spider, two-turn poison and accessible cures. Planned. Start from the accepted Seep camp and teaching arch reservations. |

## Floor brief and progression targets

| Depth | What the player sees | Decision or payoff |
| --- | --- | --- |
| 4 · The Seep | A limestone dogleg below the cistern opens into a low, mossy cave. A dry survey camp holds antitoxin. Beyond it, one spider waits by a webbed arch. | Learn the bite in isolation. Recognize the same webbing at an optional shortcut or cache and decide whether to enter. |
| 5 · The Drowned Galleries | Higher chambers, shallow pools, broad stone ribs. The surveyors' rope marks a walkable way around the water. A lone blind newt introduces its heavy bite before the mixed encounter. | Pick a side of the pillar during approach. One route brings the spider within reach sooner, the other brings the newt into contact first. Both must work with actual sightlines, alert range and contact locking. |
| 6 · The Brood Vault | Web density rises toward the guardian. The rope disappears into one of two sealed egg clutches. Moving silk announces an impending hatch. | Spend an attack on a clutch or concentrate damage on the broodmother and deal with a finite number of young. |

Preserve the opening floors' gentle density, ordinary and guardian rosters, supplies
and XP. Floor 3 gains a way down after its guardian. The cave floors each have a
guardian objective, optional resources and a safe route back. The camp, rope and
clutches tell the survey story in the world, without a quest log. Dampness comes
from authored materials, pools, drip sounds and silhouettes. Traversal stays on
the grid. Ceiling stalactites dress overhead space. Broad floor stalagmites occupy
clearly legible blocking cells. Decorative webs are distinguishable from the Web spell.

Use the four-survivor party for the pacing calculation. The current Underkeep
budget gives each member **793.75 XP on a full clear** or **675 XP on the intended
ordinary route**. Level 3 requires 900. That leaves **425 or 900 party XP**,
respectively. Aim to cross 900 per member in the first two Seep encounters after
a full clear, and by the Seep guardian on the ordinary route without cave
optionals. Count XP actually available along each route. Keep the lone teaching
spider approachable. If ordinary enemy rewards make the early target implausible,
show the earliest defensible encounter in DC06 planning for review. Retain manual
advancement and the existing 2,700 XP level-four threshold. Report where a full
clear finishes relative to it. Earning level four is an open tuning outcome.
[Current encounter budgets](encounter_density_balance_design.md) · [Advancement](progression_design.md)

## Delivery slices

| Slice | Reviewable deliverable | Dependency | Status |
| --- | --- | --- | --- |
| DC01 | Deterministic cave layouts and three walkable blockouts | Existing floor generation and room review path | [Accepted and closed](dc01_cave_layout_design.md) |
| DC02 | Limestone kit, distributed ceiling groups, webs, shallow pools, shoreline mist and animated lookout lighting | DC01 placement and clearance contracts | [Accepted and closed](dc02_limestone_kit_design.md) |
| DC03 | Venomous spider, two-turn poison and accessible cures | DC02 cave palette and scale | Planned |
| DC04 | Blind cave newt and a mixed approach encounter | DC03 working spider | Planned |
| DC05 | Broodmother and two interruptible egg clutches | DC04 encounter handoff and DC02 vault | Planned |
| DC06 | Populated, playable standalone floors 4–6 with finite XP and rewards | DC01–DC05 content and mechanics | Planned |
| DC07 | Six-floor New Campaign, persistent travel, Continue and final victory | DC06 play-ready floors | Planned |

### DC01 — Cave layout and walkable blockouts

[Implemented design, exit checks and player recipe](dc01_cave_layout_design.md) · [Floor plans, walkthrough views and pool motion](screenshots/damp-caves-dc01/README.md).

Prepare one seeded cave generator with three depth profiles. Reuse the existing
floor grid, region membership, connection paths, protected landings, objective
approaches and tile reachability checks. Generate irregular connected chambers
and short passages. Reserve the camp-before-spider sequence, the divided pool
room and the two reachable clutch positions in their respective profiles.
Specify which cells block traversal and sight, including pool edges and pillars.
Define bounded attempts, scratch sizes and checked fallback geometry for each depth.

Start in an isolated floor review entry point. The production generator currently
selects Underkeep stone rooms, its four authored room purposes and Underkeep dressing
at multiple stages. Keep those obligations local to that biome. Use the same cave
geometry in preview and eventual campaign generation. Leave the ordinary campaign
terminal at floor 3 through DC06.

**Exit evidence.** A plan view for each depth and a walkable in-engine blockout.
Small owned fixtures cover determinism, blocked corners, door/landing clearance,
both pool routes, mandatory progression, bypassable optional branches and forced
fallbacks. Record sockets, occupied cells, ceiling clearance and prop anchors
for the artist. Review route choices before dressing them.
[Layout design](procedural_layout_design.md) · [Blueprint room precedent](blueprint_rooms_plan.md)

### DC02 — Limestone kit and survey trail

[Implemented design, exit checks and player recipe](dc02_limestone_kit_design.md) · [Native walkthrough views](screenshots/damp-caves-dc02/README.md).

Build and publish a reusable cave pack through the current prepared terrain,
scenery and audio paths. Use cave materials throughout the threshold below the cistern to establish
scale and lighting. Finish wet stone, mineral streaks, shallow water, ceiling
clusters, broad floor formations, webs, bedroll and rope. Use the existing
painted-prop workflow and retain editable sources and preparation inputs.
Compose all three DC01 spaces. DC02 owns the sealed clutch's scenery reference
and anchor. DC05 owns its final encounter art, state changes and targeting.

**Exit evidence.** Matched first-person views show where a party can walk, see
past a pillar, spot the web from either approach and follow the rope. Check
near-ceiling clipping, depth/occlusion, native lighting, map agreement and
prepared resource residency across a biome change. Record actual publication
and memory costs. Verify drip cue routing and note listening review separately.
[Painted prop guide](prop_modelling_guide.md) · [Environment rendering](renderer_environment_design.md) · [Audio](audio_design.md)

### DC03 — Venomous spider and the first poison encounter

Deliver one finished, targetable spider in an isolated version of the Seep arch.
Reuse the creature catalog, prepared directional poses, attack/impact schedule and
shared combat effect queries. On a landed venomous bite, a failed Constitution
save gives the victim disadvantage on attack rolls through their next two turns.
Repeated bites neither stack nor refresh the duration. Poison deals no periodic
damage. Save-based spells remain a useful response. Preserve Poison Spray's
existing damage behavior. Set bite damage, save DC, XP and the exact turn/phase
expiry rule in the detailed design, including the transition back to exploration.

Provide a finite, one-use antitoxin at the camp before contact, usable by any
party, and add poison to Lesser Restoration's target choices. Specify action
cost, living ally selection, item consumption, invalid requests and a full bag.
Show application, remaining duration and cure in the existing HUD/log and expose
the same result headlessly. Use current condition and item command paths. Record
any rules adaptations and the disposition of deferred class or ancestry traits.

**Exit evidence.** Play the camp and bite through the usual controls. Exercise
miss, successful save, failed save, repeat bite, advantage cancellation, two
victim turns, natural expiry, both cures and stale/double use. Demonstrate a
party without a cleansing spell can finish the fight. Cover pause, source death
and permitted save/restore boundaries. Review the spider's approach silhouettes,
bite cue and status readout at game scale.
[Bestiary delivery](bestiary_plan.md) · [Effects](effects_design.md) · [Inventory](inventory_design.md)

### DC04 — Blind cave newt and mixed approaches

Deliver the newt as a slow, close-range threat with a readable wind-up. In the
detailed design specify how a blind creature detects a nearby party using the
existing perception and sound facilities, when the bite commits, and which
defensive action a party can take after seeing its warning. The wind-up must
leave a real decision window in contact combat. Define how cover, loss of
contact and interruption resolve. Keep those rules local to the newt.

Stage a solo introduction and a spider/newt encounter in the DC01 divided room.
Use the production movement, alert and targeting rules. An all-melee party
should be able to choose its approach before contact. Give both creatures
distinct silhouettes over shallow water and sufficient space to engage.

**Exit evidence.** Walk both approaches with the same starting placement and
record which target can actually be attacked first. Exercise warning, response,
impact, interruption and bounded AI movement in a small fixture. Review normal
directional art and feedback with both species visible.
[Local alerts](goblin_ai_design.md) · [Combat positioning](combat_positioning.md)

### DC05 — Broodmother and interruptible clutches

Build the guardian fight in the isolated vault. Give the broodmother a distinct
silhouette and bite. Two visible clutches each produce at most one hatchling.
Publish the warning before a player action opportunity. The normal target picker,
attack validation and feedback must support breaking a clutch during that window,
including for a melee-only party. Set attack access, area damage, hatch timing
and the tie-break between a lethal hit and hatching in the slice design.

Use bounded clutch state and reserve legal hatchling placements during preparation.
Admit a hatching creature through the existing initiative/enrollment rules. Set
out objective membership, outstanding clutches and surviving young when the mother
dies so the final blow, rewards and eventual victory agree. Budget rewards for
both destroy and hatch outcomes and keep both choices viable. Disturbing the vault
must never create repeatable XP or drops.

**Exit evidence.** Play both a clutch-breaking route and a focus-the-mother route.
Check simultaneous resolution, interrupted attacks, occupied spawn cells, limits
on live actors, picking against the webbed backdrop, guardian death and exactly-once
completion. Record the state DC06 places and DC07 must reconstruct at legal
checkpoints. Provide a review view where both the warning and target are legible.
[Guardian objectives](procedural_guardians_design.md) · [Target selection](target_selection_design.md)

### DC06 — Population, rewards and floor pacing

Assemble all three standalone floors from the reviewed pieces using production
population, encounter planning, fixtures and reward paths. Give floors 4 and 5
distinct guardian formations using the reviewed creatures. Place optional
encounters and finite supplies upstream of each guardian. Keep the first arch a
solo lesson, the dry camp reachable first, and optional pockets avoidable even
under local alerts. Budget the depth-6 fight with both clutch outcomes.

Before implementing recipes, tabulate required/optional/guardian groups, actor
counts, XP, consumables, cumulative four-survivor XP and the first eligible
advancement on each route. Reconcile against current encounter/member/actor
capacities, including hatchlings. Use travel distance and actual sightlines to
place quiet stretches and threats. Set difficulty using placement and grouping
before increasing headcount. Record first-pass values and the specific player
feedback that would justify changing them.

**Exit evidence.** For each depth verify the ordinary route, optional detour,
guardian gate, rewards, safe backtrack and fallback with deterministic fixtures.
Report XP after the first spider, second encounter, Seep guardian and Brood Vault,
with both Underkeep starting totals. Offer a reproducible floor play recipe for
user pacing review. A route budget establishes XP availability. Win rates or
playtime require separate play evidence.
[Population baseline](encounter_density_balance_design.md) · [Campaign floor progression](procedural_guardians_design.md)

### DC07 — Campaign descent, persistence and victory

Wire the reviewed floors into normal New Campaign and Continue in the graphical
and headless paths. Change the Underkeep commander's completion to unlock the
descent. Floors 4 and 5 unlock their respective down stairs. Floor 6 owns the sole
campaign victory. Update names, destinations, objective text, maps, waypoint UI,
automation observations and remaining three-floor instructions.

Today location IDs 4–6 mean Inn, Smithy and Emporium, while multiple callers
index location arrays by dungeon depth. Establish one checked depth/location
mapping and audit generation, launch, snapshots, travel/recall, labels and
codec admission against it. Preserve town doorway identities. Scope content
and simulation identity changes in each affected slice. In DC07 planning, state
whether prior active campaigns can safely continue and how terminal runs are
recognized. If incompatible, follow the existing read-only rejection path and
surface the new-campaign requirement for review before implementation.

**Exit evidence.** A bounded deterministic integration route demonstrates guarded
3→4 descent, 4↔5↔6 travel, recall and return to a visited cave entrance, and
fresh-process Continue from a legal cave checkpoint. Check town interiors,
undiscovered waypoint rejection, returning to a looted camp and a cleared floor,
failed destination preparation leaving the live party intact, no duplicate
XP/supplies, and one final victory under the DC05 objective rule. Compare a
known Underkeep seed for geometry, encounter and reward regressions, allowing
the floor-3 exit change. Check six-floor preparation, save bounds and active
scene residency against existing budgets. Record any remaining user play review.
[Campaign travel](procedural_campaign_travel_design.md) · [Recall](procedural_campaign_recall_design.md) · [Save and Continue](procedural_campaign_save_design.md)

## Fresh-context execution and handoff

Use one slice ID per invocation. Read the checkpoint, dependency handoff and live
source before refining that slice. Mark it **In progress — planning** and link its
detailed design from this roadmap and the system overview. Settle material rules,
interfaces, lifetime/capacity limits and review fixtures there. Existing approval
in the selected invocation carries through to execution. For a planning-only
invocation, stop after the detailed plan.

**Planning prompt**

> Read AGENTS.md, docs/system_design.md and docs/damp_caves_plan.md. Plan `{SLICE}`
> only. Inspect its prerequisites, handoff and current code. Write and link the
> slice design, update the checkpoint and tracker, and give me the gameplay
> decisions and focused verification plan for review.

**Execution prompt**

> Implement the approved `{SLICE}` design from docs/damp_caves_plan.md. Preserve
> working-tree changes. Verify the slice through its normal entry points, update
> its design/handoff, tracker and system index, clean owned disposable output,
> and stop at Ready for review.

**Combined planning and execution prompt**

> Read AGENTS.md, docs/system_design.md and docs/damp_caves_plan.md. Plan and
> execute `{SLICE}` only. Start from its dependency handoff and current code.
> Write and index the detailed design before implementation. Carry the slice
> through its exit checks, record the player review recipe and limitations,
> update the checkpoint and tracker, clean owned output and stop at Ready for review.

Follow the configured build tools and the project's resource limits. Use small
owned fixtures for mechanics and application-owned captures for appearance.
Interactive desktop review belongs to the user. Balance experiments are opt-in.
Keep commands, outcomes, limitations, selected evidence, a short player review
recipe and the next slice's dependencies in the slice design. Record **Ready for
review** after implementation and **Accepted and closed** on user acceptance.
Keep review corrections under the same ID. [Verification workflow](verification_workflow.md)
