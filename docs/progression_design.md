# Slice 13: levels 2 and 3

Status: implemented, **accepted and closed** (2026-10-30), including milestone C
and the review corrections below. Implementation was authorized by “Implement the
plan.” Acceptance: “I accept the slice, record it in the documentation.”
Slice 14 save/resume remains Planned and unstarted; see the
[accepted closeout](slice13_verification.md#accepted-closeout-and-next-agent-handoff).

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
Automatically distribute lowest HP percentage first, stable character-ID ties;
preview allocation and reject no benefit. Rogue: level 2 free existing Defend once
per own turn; level 3 Thief Fast Hands may instead spend that same allowance on a
potion; Sneak Attack 2d6. Wizard: level 2 Evocation/Sculpt Spells, level 3 second-level
spells and existing four-mana Arcane Recovery scaling. Clearly disclose deferred
Turn Undead, skills/Expertise, locks/traps/climbing and spell-copying/Evocation Savant.

Actions and optional activations work in either order. After impacts/tails settle,
retain the actor while useful actions remain; End Turn skips them. Reopened input
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
sheet/portraits show eligibility. Development-only `--development` exposes a pause
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
checks explicitly. Verification results and review limitations are recorded in the [handoff](slice13_verification.md).
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
hidden captures are recorded in the [handoff](slice13_verification.md). These
corrections were accepted with slice 13 on 2026-10-30.

Selection-indicator review correction (2026-10-30): remove inspected/focused card
outlines. The blue outline represents selected spells only; recommendations have
no separate marker. A picker with one available slot replaces its current pick
when another eligible card is clicked; clicking the selected card clears it.
Recommended picks applies its choices and previews a selected spell immediately.
Multi-pick budgets and immutable grants remain unchanged. Verify replacement,
clear/reselect, recommendation idempotence and the rendered single-border state.
