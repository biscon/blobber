# PC07.1 — Arena preparation and encounter calibration

Status: **Accepted and closed** (2026-11-01). The user approved the
reviewed plan with “Implement the plan”, including the bounded study, and accepted
the completed slice with “slice accepted, record it in the plan doc”. Recorded
verification evidence and limitations are unchanged by acceptance.
PC07 is split at the evidence boundary: PC07.1 delivers preparation and measured
encounter recommendations; PC07.2 selects campaign population and validates real
routes after PC07.1 acceptance. PC08 remains after completed PC07. No campaign
population, save format, generator, monster statistics or rendering changes here.

## Ownership and interfaces

ArenaScenario retains four independent level-one StartingSnapshots, a uniform
level 1–3 target and a fixed two-by-four array of incremental AdvancementChoices.
Only transitions below the target may contain choices. Pure validation walks the
same rules::previewAdvancement used by live progression. A preparation factory
constructs one ordinary Party, grants each required XP threshold and calls
Party::advance; fresh HP/mana/charges naturally remain full. It grants starting
gear and three kits once, without resetting resources or reconstructing inventory
at level-up. Prepared Party ownership transfers to GameSession through its existing
unique_ptr constructor. Map/catalog outlive it; UI/animation borrowers die first.
All allocation is confined to preparation, including rejected candidates.

Arena launch, headless start and graphical rematch use that same factory. The
immutable scenario, not the last fight's Party, is the rematch source. Party
library snapshots and campaign launch remain level one.

Arena schema 2 / layout 1 adds required preparation `{target_level, advancements}`;
advancements contains target_level-1 arrays of four `{learned, prepared}` spell-ID
lists, in party slot order. Schema 1 defaults to level one and no choices. Unknown
fields, invalid counts/levels/IDs, duplicate choices and illegal transitions reject
before active replacement. Encoding always emits schema 2; canonical scenario
identity/replay includes preparation. Campaign save/generator identities are unchanged.

Arena Setup edits an isolated candidate. Level selection starts a shared
SpellSelection/FlowControls wizard over character transitions, with required
learning, optional preparation, recommendations and final review. Cancel discards
all edits. Party identity/formation changes reset preparation to level one with
feedback; seed/name/roster edits preserve it. Loading validates before replacing.
The live UI uses fixed records and borrowed catalog data; no new per-frame owner.

## Calibration protocol

Six candidate recipes (ordered front to rear): goblin_pair (2 goblins, L1,100 XP),
rat_pack (3 rats,L1,75), undead_pair (skeleton/zombie,L1,100), bugbear_solo
(bugbear,L1,200), goblin_support (2 goblins/archer/shaman,L1,200), bugbear_support
(bugbear/goblin/shaman,L2,300). These are hypotheses, not depth assignments.
Use balanced and no_wizard standalone packages with seeds 5/8/12: at most 36
trials. Each recipe is one six-trial serial batch, inspected before the next.
Target campaign progression is L2 before the final guardian: 1,200 total monster
XP for four living members; no milestone XP or level-3 target.

Aggressive policy v2 retains v1 behavior and adds checked Action Surge, Preserve
Life and Cunning Action priorities. Rescue first; Surge for a useful extra attack,
Preserve Life for useful rescue healing, Cunning Action before ending a turn.
Calibration keeps starting preparations, explicitly leaves new slots empty, and
L2 Wizard learns false_life/color_spray without preparing them. Unsupported active
abilities fail preflight. L3 is manual/correctness support only. Diagnostic
information and the ordinary 250ms real-time cadence are disclosed.

Each trial: 60 wall seconds and existing bounded ticks/requests/decisions/traces.
Each recipe: 450 seconds and 512 MiB; study: 45 minutes and 3 GiB. Check memory/disk
before running, low CPU/I/O priority, one runner, disk-backed new output paths.
Stop on tooling/coverage/artifact failure, cancellation or budget exhaustion;
retain partial evidence. Do not expand/repeat the sample automatically.

Reports retain resolved incoming damage per member, largest hit, six-second and
round bursts, healing, sampled HP/downings (explicit lower bounds), final deaths,
mana/consumables, rounds and separate approach/total simulation duration. Burst
window storage is bounded; overflow is explicit incomplete metric evidence, never
silent truncation. Keep choices, roster order, seeds, versions and traces. Existing
retention preserves defeats/failures/outliers. Replay one retained case per recipe.
No fresh-party win is evidence that a campaign sequence is affordable.

## Verification and handoff

Owned small CPU fixtures cover level 1–3 transitions/full resources, invalid
choices, schema1 compatibility/schema2 rejection, immutable inputs, staged cancel,
manual/headless launch and rematch equivalence. Focused Release arena/progression/
menu/automation/allocation/Python tooling checks; build graphical application and
inspect permitted hidden application captures. No routine test starts the study.
No broad matrix, production bake, benchmarks or OS input automation.

Deliver exact verification/experiment commands and outcomes plus candidate costs,
XP, suitability, placement risks and reproduction evidence. Retain insufficient
or partial evidence honestly. Stop Ready for review; PC07.2 planning follows user
acceptance and chooses population from these measurements.

## Delivered result

[Verification/handoff](pc071_verification.md), [candidate cost table](pc071_calibration/README.md)
and [gallery](screenshots/pc071/index.html) record the final result. All 36 authorized
trials completed: 36 victories, three observed downings, no permanent deaths or
runner/policy/budget failures. Six retained traces strictly matched; 26 selected
traces are retained compressed. No additional trials or population integration.

The final UI skips martial transitions with no choices, uses shared advancement
selection/review for casters, and finishes with a whole-party resource review.
Identity/build/formation comparison preserves preparation when the party is unchanged.
The 4,096-record-per-member burst queue reports incomplete metrics on overflow;
none occurred. Round bursts reset on started/round events. Class features are used
in contact turns only. Existing selected-success retention did not keep one of
the three downing traces; its decisions/metrics remain and this limitation is explicit.
