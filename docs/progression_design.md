# Slice 13: levels 2 and 3

## Earned-level audio — 2026-11-09

Implemented and mechanically verified; listening review pending. GameSession compares a fixed four-bit
supported earned-level mask before and after each credited-death reward batch,
and before/after development XP grants. Any newly earned next-level threshold
emits one existing interface advancement cue for the whole batch, independent
of combat, safety or unconsciousness. The mask is transient: it follows current
level and XP and adds no save state. Additional XP while already eligible,
duplicate reward reconciliation, previews, rejected requests and advancement
completion are silent. Loading an already-eligible party does not replay a cue.
Dead characters remain excluded from rewards; capped/unsupported classes cannot
newly qualify. Reward distribution and actual advancement admission are unchanged.

Verify below/exact thresholds, simultaneous members/multiple deaths, duplicates,
unconscious/dead recipients, development no-ops, cap and silent completion. Keep
progression allocation checks and existing resource/choice regressions.

Debug progression (0.02 s), audible progression_allocations (0.03 s) and the two
audio suites (0.01 s each) pass in the single focused six-suite checkpoint.
Regression coverage includes exact natural reward thresholds, multiple deaths
and simultaneous party eligibility, duplicate reconciliation, silent completion,
restored eligible party/credited deaths, unsafe combat, unconscious/dead members,
unsupported classes, both supported development thresholds and level-cap no-ops.
The progression allocation workload enables its audio journal so the new emission
path is exercised under the guard. See [commands and limitations](inventory_design.md#bag-comparison-tooltips--2026-11-09).
No physical listening review was run; disposable output was cleaned.

## Earned-level reminders — 2026-11-09

Implemented and mechanically verified; interactive user review pending. HudCard adds levelEarned (supported progression
class, below level cap, sufficient XP), independent of life/action/safety gates.
Portrait reminders use this flag, so movement and combat cannot flicker it.
levelEligible continues to represent immediate session eligibility for the
Character-tab action; GameSession revalidates actual advancement unchanged.
No persistent fields or allocations are added. Regressions sample movement and
turn interpolation, unsafe combat, incapacity, advancement completion and the
level cap; existing progression allocation checks remain required.

Debug `progression` (0.01 s) and `progression_allocations` (0.02 s) pass. Hidden
captures show the reminder during an active turn while immediate eligibility is
false; all four label regions match their idle pixels exactly. Actual advancement
admission remains unchanged. See [commands and limitations](map_revision.md#playthrough-correction-verification--2026-11-09)
and [selected evidence](screenshots/playthrough-fixes/README.md). Interactive review
remains pending; disposable output is cleaned.

Status: implemented, **accepted and closed** (2026-10-30), including milestone C
and the review corrections below. Implementation was authorized by “Implement the
plan.” Acceptance: “I accept the slice, record it in the documentation.”
Slice 14 save/resume remains Planned and unstarted; see the
[accepted closeout](verification/archive/slice13_verification.md#accepted-closeout-and-next-agent-handoff).

## Ownership and progression

GameSession owns a 256-entry credited-death ledger, reconciled from permanent actor
death markers after simulation mutations, independently of presentation and loot.
Catalog schema 9 adds monster XP/humanoid and typed class progression metadata.
Older schemas retain zero XP/non-humanoid defaults. Current goblins award 50 XP.
Rewards divide among non-dead companions (including unconscious), retaining exact
twelfths and capping at 900 XP. Thresholds are 300/900. No objective/milestone XP.

Party owns live advancement, not the level-one library/scenario blueprints. Pure
preview/validation uses current live loadout and borrowed identity strings. Fixed
drafts carry world, character, source level, inventory/preparation/progression
revisions and only newly earned selections. Session rechecks conscious eligibility,
settled safe Exploration, no transition/impact/hazard and all revisions before an
atomic Party commit. Invalidate old advancement/rest/preparation requests. No
string copies, inventory reconstruction, starting gear or supply grants at commit.

One level per commit. Fixed-average HP gain: Fighter 6, Cleric/Rogue 5, Wizard 4,
plus CON (minimum one) and racial HP. Current HP/mana increase by the cap increase,
preserving deficits; mana caps 4/6/14. Add one available Hit Die. Preserve existing
spent charges; newly earned charges start available. No respec, resurrection or
full recovery. Existing names, portraits, abilities, equipment, formation and spell
sources remain. Restart/rematch reconstructs original level-one launch builds.

Wizard learns exactly two spells each level (6/8/10 total). At level 2 these are
the two remaining level-one spells; at level 3 choose two of five new spells.
Existing preparations are locked; add at most one per advancement. Long Rest
still owns replacements. Life grants Lesser Restoration/Spiritual Weapon at level
3 outside the ordinary preparation allowance. High Elf cantrips remain INT-based.

## Features and action sequencing

Fighter: level 2 Action Surge (one extra normal action, short/long-rest charge);
level 3 Champion weapon criticals on 19–20. Cleric: level 2 Preserve Life, action,
one short/long-rest charge, pool 5×level, recipients capped at half effective max HP.
The spellbook and character sheet state that at least one living party member must be
below that cap, rounded down. The disabled ability explains the same threshold.
The Debug `ctest -R '^level_spells$'` check passed for rejection at the cap with
the charge retained and one point of healing one HP below it.
Automatically distribute lowest HP percentage first, stable character-ID ties;
preview allocation and reject no benefit. Rogue: level 2 free existing Defend once
per own turn. After the last normal action resolves, automatically apply an unused
allowance and advance initiative, preserving terminal combat outcomes. Manual early
defence and explicit End Turn remain supported. Level 3 Thief Fast Hands may instead
spend that allowance on a potion before the last normal action; Sneak Attack 2d6.
See the [PC07.2 correction](pc072_variety_controls.md). Wizard: level 2 Evocation/Sculpt Spells, level 3 second-level
spells and existing four-mana Arcane Recovery scaling. Clearly disclose deferred
Turn Undead, skills/Expertise, locks/traps/climbing and spell-copying/Evocation Savant.

Except for the rogue completion rule above, actions and optional activations work
in either order. After impacts/tails settle, retain the actor while useful actions
remain; End Turn skips them. Reopened input
gets a fresh command token, not another turn/effect/allowance refresh. Sequential
actions reuse one owner attack slot. Real-time optional activations use separate
six-second deadlines; Action Surge grants one extra opportunity without resetting
other deadlines. Approach/contact/facing transitions preserve spent allowances.

## Frozen spells and adaptations

All cost three mana, unlock at character level 3, use full-action/six-second casts,
and do not upcast. Ranged selection uses existing six-cell visibility/LOS rules.

| Spell | Behavior |
| --- | --- |
| Aid (Cleric) | Up to three living allies, +5 effective max/current HP, eight hours, no concentration; no stacking or refresh healing. Expiry removes bonus/clamps HP; distinct from temporary HP. |
| Hold Person (both) | Visible humanoid, WIS save, concentration 60 s; existing paralysis/retry/clock behavior. |
| Lesser Restoration (Cleric/Life) | One selected supported blindness/paralysis on a living ally; no eligible condition rejects. No fake disease/poison/deafness support. |
| Spiritual Weapon (Cleric/Life) | One/caster, 60 s, no concentration. Cast places/attacks for 1d8+WIS force. Later free activation once/turn or six seconds; cast consumes its interval. Move up to two clear cells and optionally attack in destination cell; no actor occupancy/formation protection/wall attacks. May idle. |
| Scorching Ray (Wizard) | Allocate three independent 2d6 fire attack rays among one to three enemies, independent criticals, delayed impact. |
| Shatter (Wizard) | Wall-clipped aimed 3×3 area; 3d8 thunder, CON half. Includes caster/allies; Sculpt protects other three party members in preview/execution. Existing ally-safe spells unchanged; scenery destruction deferred. |
| Mirror Image (Wizard) | Self, three duplicates, 60 s, no concentration. SRD 6/8/11 redirection thresholds, AC 10+DEX, one duplicate destroyed by redirected hit. Blind attackers and areas/saves bypass. Reserve pending duplicate outcomes by effect identity; publish destruction at impact; cancellation releases reservations. |
| Web (Wizard) | Wall-clipped floor 2×2 forward/right from aimed cell, concentration one hour; difficult terrain, DEX restraint at creation/entry and six-second/full-round hazard checks. Escape action is STR check; restrained NPCs attempt it. Restrained can act but not move; attack disadvantage, attackers advantage, DEX-save disadvantage. An unrestrained ally carries party out, ending outside restraint. |

Fire impacts ignite web cells; at the next six-second/full-round burn boundary
deal 2d4 fire to occupants then remove cells. No vertical/obscurity system.
Overlapping same-kind hazards choose strongest DC/stable ID; difficult terrain
does not multiply. Spectral weapon/strikes and Web ground/status art are visible
prepared resources. Existing pulse/projectile/status presentation is reused.

## Interfaces, storage and presentation

Pure progression API, Party staged commit and GameSession preview/advance APIs
separate rules, ownership and safety. UI owns a bounded isolated draft and reuses
SpellSelection/FlowControls/review. Overview → Class features → applicable Spells
→ Review follows approved mockup 14 (Color Spray correction retained). Character
sheet shows immediate eligibility; portraits show an earned-level reminder independent
of temporary safety/action restrictions. Development-only `--development` exposes a pause
button topping every eligible member up to its next threshold without auto-level;
repeat before advancement is a no-op. No live file I/O in advancement.
The command-line parser explicitly enables this default-off flag; headless parser
and executable regression checks cover the documented launch option.

32 catalog spells/32-bit masks; grow class arrays from 16 to 32. Keep 38 picker
entries and 128 controls with checked insertion. 64 effects, 16 ground-cell records,
atomic full Web footprint reservation/concentration linkage and 260 restraint slots
(one per party member/NPC slot). Mirror Image and Spiritual Weapon each use at most
one shared effect slot per caster; their identity and reservation state live there. Recycled identities are
generation/world checked; capacity failure precedes costs/RNG/replacement. Existing
attack timeline has one slot per owner, reused after completion; optional actions
never add concurrent slots. The 8192-event buffer retains its static bound assertion.
Do not index new effect kinds into the existing four-condition array.

Creator atlas becomes 36 tiles (32 spells/four classes), retaining 16 MiB and eight
GPU texture slots. Preserve portrait IDs in small-icon mappings. Reuse Hold Person
master; generate missing artwork with original masters/provenance. Extend offline
ground/VFX preparation with bundle format 3 / renderer revision 3. Each strafe phase
has four Web floor frames in addition to existing background/Grease banks (24 full
frames total), plus 299,522 prepared spectral-weapon pixels for square sizes 4–96.
Runtime composition selects prepared pixels and applies bounded blending, lighting
and wall/NPC occlusion. Weapons do not occupy cells or add actor picking identities.
No runtime image resizing. Loading and baking account for the additional banks
before allocation, with existing resident/loading limits and explicit failure.

`rules/progression` owns pure previews and newly earned selection contexts;
`game/party_progression` owns live resources and revisions;
`game/session_progression` owns safety and death-credit reconciliation;
`game/advancement_commands` owns the fixed modal draft. `combat_progression`
separates normal actions from optional activations. `level_spells` validates and
executes concrete spell commands, while `level_effects` owns Aid, duplicate
reservations, spectral positions and Web lifecycle. These reuse the existing
command/effect controllers without a general spell scripting framework.

Ground identities are monotonic within a world, linked to the owning concentration
EffectId; expiry, dismissal, death/incapacitation or full burning clears footprint
and restraints. Mirror reservations carry EffectId plus a duplicate bit and accepted
hit result. Impact consumes the reserved duplicate; cancellation releases it. An
expired/replaced image cannot redirect the accepted outcome back to character HP.
Spiritual Weapon uses a separately checked six-second readiness deadline.

A Web footprint adds at most four records. All sixteen ground records can burn
at one boundary: four NPCs per cell plus the four-person party means at most 68
burn recipients, each with bounded damage, wake and concentration notifications.
Ground saves, restraint endings, effect removal and existing non-Web condition
notifications fit the existing 4096 condition/ground-event allowance; 64 shared
effects retain their separately bounded event allowance. Maximum-population testing
exercises 256 actors, sixteen Web cells and a full 64-effect table. Exhaustion rejects
before spending resources or replacing existing concentration. The stress result
is evidence for the selected workload, not a substitute for these storage bounds.

## Verification and delivery checklist

- [x] Schema/rules, exact XP and atomic live progression; all 16 race/class pairs.
- [x] Shared advancement UI and development grant; cancel/stale/double requests.
- [x] Class features, either-order actions, End Turn and clock/impact integration.
- [x] Eight spells, targeting, effects, costs, resources, art and presentation.
- [x] Identity/inventory/library/scenario preservation and rest/rematch integration.
- [x] Focused fixture-owned behavior tests, Debug/Release full suites, ASan/UBSan.
- [x] Allocation guard first/repeated operations; separate maximum-capacity stress.
- [x] Hidden GPU lifecycle and native/reduced/letterboxed deterministic captures.
- [x] Current design/ledger/roadmap, verification handoff and user review recipe.

Test reward duplication/fractions/life states; thresholds and cap; transaction
failure atomicity/revision exhaustion; deficits/charges/rest invalidation; both
action orders, no duplicate turn effects; every spell's failure/expiry/stacking/
concentration interactions, Aid with equipment/rest/advancement, overlapping Mirror
impacts, Web movement/fire/escape and sculpted friendly fire. Captures exercise
creation → launch → XP → advancement → combat/rest and arena rematch. No OS input,
focus or desktop automation. Interactive review belongs to the user; report skipped
checks explicitly. Verification results and review limitations are recorded in the [handoff](verification/archive/slice13_verification.md).
Slice 13 and milestone C are accepted and closed (2026-10-30).

## Slice 13 review: spell selection clarity (implemented, 2026-10-30)

The user requests compact descriptions, scrollbars instead of Up/Down controls,
blue whole-card selection outlines, aligned equal-height list/detail panels and removal of redundant locked cards during
advancement. Preparation remains optional, explicitly confirmed by the user. An
empty advancement preparation uses “Leave slot empty”; it does not silently look
like a completed selection. Explain the gained slot, existing preparation/domain
counts, available choices and level-three spell unlock separately from learning.
Existing preparations can only be replaced during Long Rest.

Keep the locked masks for validation but hide them in advancement lists. Other
shared-picker callers retain their domain grants with a plain text label. Clicking
an editable card selects/deselects and inspects it. Multi-pick over-budget clicks
still inspect without changing choices; the single-slot correction below allows
direct replacement. Selected cards have a blue outline, no checkboxes or L
controls. Shared fixed scrollbar routing handles wheel, track clicks and captured
thumb drags in creation, rest and advancement. Description line metrics are a
presentation-only cache populated by measured wrapping; input consumes the last
presented range, without introducing font/GPU dependencies into core rules. All
storage remains bounded. Verify routed input, optional progression and immutable
baseline, then inspect hidden native/reduced/letterboxed captures of both empty
and selected Cleric/Wizard states and shared creation/rest screens.

Review verification: focused Debug/Release checks, sanitizer checks and refreshed
hidden captures are recorded in the [handoff](verification/archive/slice13_verification.md). These
corrections were accepted with slice 13 on 2026-10-30.

Selection-indicator review correction (2026-10-30): remove inspected/focused card
outlines. The blue outline represents selected spells only; recommendations have
no separate marker. A picker with one available slot replaces its current pick
when another eligible card is clicked; clicking the selected card clears it.
Recommended picks applies its choices and previews a selected spell immediately.
Multi-pick budgets and immutable grants remain unchanged. Verify replacement,
clear/reselect, recommendation idempotence and the rendered single-border state.


## Level four and ability improvements — approved implementation specification

Status: **implemented, mechanically verified, accepted and closed** (2026-11-16). The user authorized
implementation after approving complete level-four gains in campaign and arena,
and explicitly selected strict campaign save versioning. This section does not
change acceptance of the previous levels-two/three slice.

The user explicitly accepted the completed level-four work on 2026-11-16:
“looks good, approved.” This closes the level-four advancement and ASI scope;
skills/tools, feats and new classes remain deferred.

### Rules and ownership

Extend the four current classes through level four. XP thresholds become
300/900/2700, with exact fractional awards. The original 2700 storage cap was lifted for [DC06 cave pacing](dc06_population_rewards_design.md). Keep one level
per atomic commit, earned reminders/audio, safety and stale-request validation.
Level-four proficiency remains +2. No level five, skills/tools, feats, new classes,
encounter/reward rebalancing or respec is included.

Creation scores remain immutable. CharacterIdentity and its borrowed view gain a
typed fixed-size level-four ASI record containing six byte-sized increases. Below
four every increase must be zero; at four exactly two points must be distributed,
with each entry 0..2 and final scores (including race) at most 20. This supports
+2 to one ability or +1 to two. Do not introduce unused skill/feat infrastructure.
A shared checked ability calculation must feed derivation, preparation and UI.
AdvancementChoices gains the allocation and a new-cantrip mask; AdvancementPreview
returns the resulting ASI record and grants. Partial UI drafts are permissible;
authoritative preview/commit requires the complete legal allocation.

Recompute attacks/damage, armor limits, saves, initiative, carrying capacity,
spell attacks/DCs and preparation from effective scores. Constitution applies
retroactively to all four levels under existing fixed-average/minimum HP rules.
Current HP/mana increase by maximum differences, preserving deficits and Aid/item
bonuses. Gain one available Hit Die; spent charges stay spent. Identity, inventory,
formation and original library blueprints remain unchanged.

Cleric and Wizard gain one class cantrip (four total), keeping racial cantrips
separate and rejecting duplicates. Wizard learns exactly two additional spells
(twelve total). Maximum spell level remains two and base mana becomes 17. Use
existing content and artwork. Arcane Recovery stays four mana, Sneak Attack 2d6;
Second Wind scales to level four and Preserve Life to 20. No cantrip damage scaling.
Preserve preparations/domain grants, allowing optional new preparations equal to
the increase in preparation limit, including the ASI casting-modifier increase.
Bound choices by remaining legal spells. Existing level-two/three and Long Rest
replacement policies remain unchanged.

### UI and arena integration

Shared flow: Overview → Class features → Ability scores → Cantrips → Learned
spells → Preparation → Review, skipping inapplicable steps. Level-two/three flows
remain unchanged. Six minus/plus score rows show original/proposed scores and
modifiers, with two initially unassigned points. Both points are mandatory.
Abilities precede spells. Reducing preparation capacity by revising abilities
clears new preparation selections and requires reviewing that step again; baseline
preparations remain. Empty preparation remains an explicit optional choice.
Review displays selected improvements, effective scores/modifiers, shared-rule HP
and mana previews, and cantrip/spell choices. Remove independent UI HP arithmetic.
The character sheet shows a compact level-four improvement summary.

Retain fixed-capacity drafts/controls and allocation-free ordinary processing.
Confirmation validates all existing revisions/safety, then atomically commits
level, ASI, spells, stats and resources. Cancel/stale/invalid requests mutate none.
Arena supports three transitions and four level buttons, mandatory martial ASIs,
shared preview/replay, and rematches from stored preparation.

### Persistence

Bump campaign schema/simulation 22/24 to 23/25. No migration; reject incompatible
saves without overwriting them. Serialize and validate the ASI before deriving
restored stats. Level-one library format stays unchanged. Arena writers emit
scenario version three with ASI/cantrip choices; retain reading versions one/two
under their original level-one-to-three constraints, with zero new fields. Update
arena experiment validators without extending combat policies. Audit level bounds,
checkpoint validation, errors, XP displays and development XP grants consistently.

### Required verification and handoff

Focused fixtures must cover all race/class combinations; +2/split allocations;
score-20 boundaries; incomplete/forged/premature improvements; unchanged creation
scores; level cap; derived modifier thresholds, armor caps, racial magic and
retroactive Constitution with damaged/Aid/equipped characters. Verify fourth
cantrips, twelve learned spells, racial duplicates, immutable baselines, optional
preparation gains of one/two and choices invalidated by revised abilities.
Exercise 2699/2700 thresholds, capped rewards, audio/reminders, stale/double/cancel
requests, safety, deficits/charges, development no-ops, save round trips/malformed
ASI/old-version rejection, travel/rest, arena rematches and legacy scenarios.
Extend first/repeated progression allocation checks through previews and commits.

Run affected rules/progression/creation/inventory/spell/arena/persistence/allocation
checks using configured CMake/Ninja, two build workers maximum and one test process.
Inspect hidden application-owned native/reduced/letterboxed captures of allocation,
two-slot preparation, review and character sheet. No desktop input automation;
interactive acceptance belongs to the user. Own disposable output through the
agent-artifact workflow; keep at most six selected review images and clean the run.
Record actual commands/results and limitations here. Verification commands and final outcomes are recorded below.


### Implemented interfaces and verification — 2026-11-16

`AbilityScoreImprovement` contains six byte-sized increases in STR/DEX/CON/INT/WIS/CHA
order. `CharacterIdentity`/`CharacterIdentityView::levelFourAsi` retain the earned
record; `effectiveAbilities` provides checked effective scores and an explicitly
partial mode for UI drafts. Authoritative derivation requires the complete record.
`AdvancementChoices` carries `abilityImprovement` and new `cantrips` alongside the
existing learned/prepared masks. The new spell-choice overload of
`advancementChoices` retains the old bool overload for existing callers.
`AdvancementPreview` returns the resulting record and derived stats. Stats may be
available while required spell selections are incomplete; only `valid` authorizes
a commit. Party adds active Aid to the preview and commits the complete result.

`AdvancementHud` shares candidate identity, ability controls and spell-selection
storage between campaign and arena. Arena retains three four-member transitions.
Scenario version 3 stores `ability_improvement` as six integer increases and
`cantrips` as stable spell IDs; versions 1/2 retain their original bounds. Campaign
schema/simulation are 23/25; previous 22/24 saves reject without migration or writes.
The level-one library format is unchanged.

The headless `advancement` action accepts optional `ability_improvement` and
`cantrips` fields with the same shapes as arena choices. Existing level-two/three
requests remain legal without them; level four requires complete choices through
the same authoritative validation. Descriptors expose ability order/cap/points and
new cantrip options, and explain that preparation depends on the complete choices.
Observations expose `level_four_asi` separately from `base_abilities` and effective
`abilities`. Wire regressions cover malformed arrays, omitted mandatory ASIs,
non-mutating preview, commit, observation and replay rejection.

All selected Debug suites pass across focused runs: **rules, party, creator,
inventory, spells, level_spells, progression, progression_allocations, arena,
arena_experiment_tools, campaign_save_smoke, campaign_save_cli, automation**
(13 distinct suites). Progression/arena allocation guards report zero engine-owned
C++ allocations, including the full level-four UI route. The dedicated campaign
progression run also passes level-four serialized round trips, missing-ASI and
previous-version rejection, floor travel, recall and revisit. Final application,
headless and affected test targets build with the configured CLion CMake/Ninja;
the final initializer and wire builds emit no warnings. `git diff --check` passes.

The CLI save/replay fixture now follows the observed dungeon entrance instead of
the obsolete small-hub route. The older earned-XP automation fixture uses one
nearby target and one distant survivor, explicitly continuing after local victory;
it preserves the live-arena advancement invariant without depending on an
unrelated pursuing pack's search duration. The production AI was not changed.

Reproduction, from the repository root (use a fresh owned agent-run directory for
capture output):

```sh
# Build targets: blobber, blobber_headless, blobber_automation_tests, and each
# blobber_<suite>_tests target named above (the Python tooling suite has no target).
# Use the configured tool, at low priority, with at most two workers:
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-debug --target blobber blobber_headless blobber_progression_tests blobber_progression_allocations_tests blobber_arena_tests blobber_campaign_save_tests blobber_automation_tests blobber_world_integration_gpu_tests -j 2

# Selected CTest suites (one test process):
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest --test-dir cmake-build-debug --output-on-failure -j 1 -R '^(rules|party|creator|inventory|spells|level_spells|progression|progression_allocations|arena|arena_experiment_tools|campaign_save_smoke|campaign_save_cli|automation)$'

# Narrow campaign persistence/travel integration, avoiding the full slow suite:
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 cmake-build-debug/blobber_campaign_save_tests --progression

# Hidden production UI export; replace RUN_DIRECTORY with the owned run path:
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2 nice -n 15 ionice -c 3 cmake-build-debug/blobber_world_integration_gpu_tests assets RUN_DIRECTORY/visual-review --level-four-review cmake-build-debug/assets/prepared
```

The hidden OpenGL capture passes and its six outputs were visually inspected:
allocation at 1920×1080, 1280×720 and letterboxed 1024×768, plus two-slot preparation,
final review and the committed character sheet. The wizard shows INT 16→18,
17 mana, four class cantrips and twelve learned spells. See the
[selected evidence](screenshots/level-four/README.md). No OS input was injected and
no desktop windows were controlled. The user accepted the completed work on
2026-11-16 (“looks good, approved.”). Recorded verification limitations remain.

Limitations: Release/sanitizer configurations, benchmarks and the full slow save
suite were not run; verification selected affected Debug behavior instead. The
sandbox could not open the display, so the authorized hidden capture ran with
normal display access. The staged Debug asset tree could not initialize UI artwork
because it lacks `ui/items/padded.png`; successful capture used source artwork and
separately supplied prepared atlases. This is not a staged-package verification.
No unresolved level-four mechanical failures remain. The owned disposable run
was previewed and cleaned (38 files, 3,962,884 bytes removed). Only the six selected
review images remain; build caches and unrelated runs were preserved.
