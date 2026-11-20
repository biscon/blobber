# Damp Caves expansion — design and delivery roadmap

Created 2026-11-18. **DC01–DC06 are Accepted and closed. DC07 is Ready for review.**

Extend the playable descent from three floors to six. Below the Underkeep, water
has broken through an old cistern into limestone galleries occupied by venomous
spiders and blind cave newts. The surveyors' abandoned route leads to a broodmother
in the lowest vault. The gameplay arc is to recognize a danger, choose an approach,
then decide which threat deserves the party's next action.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Playable checkpoint | Six persistent dungeon floors. The Underkeep commander opens the descent to The Seep. Victory requires the broodmother and her surviving young. |
| Last closed slice | [DC06 — Population, rewards and floor pacing](dc06_population_rewards_design.md#player-acceptance), accepted and closed by the player on 2026-11-19. |
| Current review | [DC07 — Ready for review](dc07_campaign_descent_design.md). Start with a chosen party on floor four and carry it through the Vault. The [Galleries nest balance trial](dc06_population_rewards_design.md#galleries-balance-trial) is ready for player review. [Player walkthrough](dc07_campaign_descent_design.md#player-review) · [Native views](screenshots/damp-caves-dc07/README.md) |
| Next eligible slice | None. The roadmap awaits the player’s DC07 review. |

## Floor brief and progression targets

| Depth | What the player sees | Decision or payoff |
| --- | --- | --- |
| 4 · The Seep | A limestone dogleg below the cistern opens into a low, mossy cave. A dry survey camp holds antitoxin. Beyond it, one spider waits by a webbed arch. | Learn the bite in isolation. Recognize the same webbing at an optional shortcut or cache and decide whether to enter. |
| 5 · The Drowned Galleries | Higher chambers, shallow pools, broad stone ribs. Survey rope lies along the north bank. The pillars and water divide the two approaches. A lone blind newt introduces its heavy bite before the mixed encounter. | Pick a side of the pillar during approach. One route brings the spider within reach sooner, the other brings the newt into contact first. Both must work with actual sightlines, alert range and contact locking. |
| 6 · The Brood Vault | Web density rises toward the guardian. Old survey rope lies beneath the nests. Two ceiling sacs pulse before their young hatch. | Spend an attack on a clutch or concentrate damage on the broodmother and deal with recurring young from the surviving clutches. |

Preserve the opening floors' gentle density, ordinary and guardian rosters, supplies
and XP. Floor 3 gains a way down after its guardian. The cave floors each have a
guardian objective, optional resources and a safe route back. The camp, rope and
clutches tell the survey story in the world, without a quest log. Dampness comes
from authored materials, pools, drip sounds and silhouettes. Traversal stays on
the grid. Ceiling stalactites dress overhead space. Broad floor stalagmites occupy
clearly legible blocking cells. Decorative webs are distinguishable from the Web spell.

The 2026-11-20 balance trial uses kill XP across all three floors and the one-time
200 party XP for defeating the brood. The old DC06 milestone target is retired.
With four survivors on the ordinary route, each enters the Seep at 675 XP, the
Galleries at 850, and the Vault at 1,150. [Current trial and player recipe](dc06_population_rewards_design.md#galleries-balance-trial).
[Current encounter budgets](encounter_density_balance_design.md) · [Advancement](progression_design.md)

## Delivery slices

| Slice | Reviewable deliverable | Dependency | Status |
| --- | --- | --- | --- |
| DC01 | Deterministic cave layouts and three walkable blockouts | Existing floor generation and room review path | [Accepted and closed](dc01_cave_layout_design.md) |
| DC02 | Limestone kit, distributed ceiling groups, webs, shallow pools, shoreline mist and animated lookout lighting | DC01 placement and clearance contracts | [Accepted and closed](dc02_limestone_kit_design.md) |
| DC03 | Venomous spider, two-turn poison and accessible cures | DC02 cave palette and scale | [Accepted and closed](dc03_venomous_spider_design.md) |
| DC04 | Blind cave newt and a mixed approach encounter | DC03 working spider | [Accepted and closed](dc04_blind_cave_newt_design.md) |
| DC05 | Broodmother and two interruptible egg clutches | DC04 encounter handoff and DC02 vault | [Accepted and closed](dc05_broodmother_design.md#player-acceptance) |
| DC06 | Populated, playable standalone floors 4–6 with finite XP and rewards | DC01–DC05 content and mechanics | [Accepted and closed](dc06_population_rewards_design.md#player-acceptance) |
| DC07 | Six-floor campaign with travel, Continue, final victory and an in-game choice of party and starting floor for debug runs | DC06 play-ready floors | [Ready for review](dc07_campaign_descent_design.md) |

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

[Implemented design, exit checks and player recipe](dc03_venomous_spider_design.md) · [Native encounter views](screenshots/damp-caves-dc03/README.md).

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

[Implemented design, exit checks and player recipe](dc04_blind_cave_newt_design.md) · [Six native encounter views](screenshots/damp-caves-dc04/README.md).

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
silhouette and bite. Each visible clutch can replace its slain young while the mother lives, with one living child per clutch.
Publish the warning before a player action opportunity. The normal target picker,
attack validation and feedback must support breaking a clutch during that window,
using ranged weapons or Fire Bolt. A melee-only party faces recurring young. Set attack access, area damage, hatch timing
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

**Accepted and closed by the player on 2026-11-19.** [Encounter budgets, player recipe, measured results and DC07 handoff](dc06_population_rewards_design.md).

Assemble all three standalone floors from the reviewed pieces using production
population, encounter planning, fixtures and reward paths. Give floors 4 and 5
distinct guardian formations using the reviewed creatures. For floor 5, evaluate the approved newt bite against the party level and equipment expected after the Underkeep and Seep. The DC04 standalone review used a level-one party. Place optional
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

**Ready for review, 2026-11-19.** [Detailed design and exit checks](dc07_campaign_descent_design.md#verification) · [Player walkthrough](dc07_campaign_descent_design.md#player-review) · [Review gallery](screenshots/damp-caves-dc07/README.md).

Wire the reviewed floors into normal New Campaign and Continue in the graphical
and headless paths. Change the Underkeep commander's completion to unlock the
descent. Floors 4 and 5 unlock their respective down stairs. Floor 6 owns the sole
campaign victory. Update names, destinations, objective text, maps, waypoint UI,
automation observations and remaining three-floor instructions.

Player review takes place in the finished campaign through the game menus.
In development mode, reuse the New Campaign party picker so the player can
choose preset or saved characters, then select a starting dungeon floor (1–6).
Show the starting level and XP before launch. Use the ordinary-route entry
budgets. Start at level 2 with 675 XP per member on floor 4, level 2 with
850 on floor 5, and level 3 with 1,150 on floor 6. Prepare the chosen party
through its class progression and place it at the selected entrance, with the
earlier stairs open for backtracking and the current floor's rewards available.
From there the party uses normal campaign travel, inventory, XP, save, Continue
and victory. Apply the usual new-run save replacement rules.

At the DC06 handoff, location IDs 4–6 meant Inn, Smithy and Emporium, while multiple callers indexed location arrays by dungeon depth. Establish one checked depth/location
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
scene residency against existing budgets. Verify that a debug start on floor 4
uses the selected roster, survives save and Continue, and carries earned XP
and remaining supplies into floor 5. Check a direct floor-6 start through the
ordinary victory condition. The player recipe should begin at the in-game party
and floor selectors. Starting in the Seep lets the player judge when level
three becomes available and how much kill XP the same party earns per
survivor in the Vault. [DC06 player feedback](dc06_population_rewards_design.md#player-acceptance).
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
