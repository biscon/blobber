# B01 — Skeleton and typed damage

Status: **Ready for review**, 2026-10-31. The user accepted the
bestiary roadmap and approved this detailed plan with “Implement the plan.”
Campaign integration stays paused; B02 is unstarted.

## Delivery and art review

Deliver one melee-only skeleton in the normal arena. AC 13, HP 13, abilities
10/14/15/6/8/5, shortsword +4 for 1d6+2 piercing, 50 XP, one shortsword loot.
Bludgeoning vulnerability, poison immunity, sleepImmune=true, humanoid=false,
metalArmor=true. No shortbow, shield, new footprint, poison condition or Turn Undead.

The selected imagegen direction is a weathered dungeon guard: yellowed bones,
sparse rusted armor, tattered brown cloth, right-hand shortsword, no shield.
Use both existing goblins as actual visual references. Present a four-view reference
for user approval before generating full animation sheets, as required by the
accepted roadmap. Mechanics/owned fixture work may continue during reference review.
Reference accepted on 2026-10-31: “yes concept art approved”.
Full production art and normal arena availability are delivered. See
[verification and review](b01_verification.md); user acceptance is pending.

Use the built-in imagegen skill, retain transparent masters, exact prompts and
measured registration in assets/npcs/skeleton. Upright world height 0.85 cells;
widths follow canvas proportions and existing ground anchors. Match the goblins’
shaded textured retro style and muted materials. Death collapses into bones;
prone remains a fallen intact skeleton. Full applicable set: four-direction idle
(2 frames, 600/600 ms), walk (2, 300/300), hurt (2, 150/150), death/corpse
(4, 200/200/200/600), prone (1); front melee (4, 700/250/250/350, impact 950).
That is 48 poses in 21 master sheets. Use current preparation, alpha/mip, shadows,
animation and picking. No unused ranged clip or renderer replacement.

## Rules, ownership and data

Add two uint16 masks to immutable MonsterDefinition: damage immunities and
vulnerabilities. Default profiles are normal. Rules catalog schema 10 requires
both damage_immunities and damage_vulnerabilities arrays of supported type names;
legacy catalogs default to normal. Reject duplicates, unknown bits/types and overlap.
No speculative resistance system. Existing capacities and budgets stay fixed.

One pure allocation-free resolver accepts rolled damage, damage type and profile;
it returns raw/resolved damage and Normal/Immune/Vulnerable adjustment. Clamp
negative rolled totals to zero; saturate doubling at INT64_MAX. Low-level actor
HP mutation remains HP/occupancy-only. Apply mitigation exactly once before it.

CombatOutcome retains resolved damage (before HP clamping) and adds rawDamage,
damageType and adjustment. Pending payloads retain saved raw rolls and type;
resolve at impact with no rerolls. Preserve critical and Sneak Attack aggregation,
Magic Missile grouping and separate Scorching Ray impacts. Acceptance/release
publications must not reveal saved rolls.

Weapon, immediate/delayed spell, area, Spiritual Weapon, Shatter and burning Web
paths use the same resolver. Burning Web currently mutates health separately;
keep its scheduling but share mitigation. Party profiles remain normal.
Pure damage saves skip immune targets, retaining ordinary costs and the shared
cast damage roll. Thunderwave still saves for pushing. Zero resolved damage causes
no hurt, interruption, awakening, concentration check, death or reward. Independent
hit riders and environmental ignition keep their existing semantics.

## Arena, feedback and compatibility

Append appearance skeleton without reordering existing definitions or adding
campaign placements. Existing arena catalog/appearance binding exposes it when
production art is complete. Replace hardcoded Goblin log labels with bounded
actual identities. Show neutral IMMUNE feedback, explain vulnerability in logs,
and apply profiles to damage previews. Poison targeting warns but remains legal.

Resolved attack observations add damage_type, raw_damage and damage_adjustment;
protocol v1 stays. Existing binary/catalog/appearance hashes reject old strict
replays. Campaign schema stays 3; simulation becomes 4. Reject old saves without
rewriting them. No arena format change, new runtime owner or frame allocation.

## Verification and completion

Use owned small fixtures/fixed seeds for resolver boundaries, schema validation,
legacy defaults, weapon/critical/zero damage, immediate/delayed/area/multi-hit/Web
paths, cost/RNG and independent rider behavior. Cover Sleep, Hold Person, metal
armor, blindness and knockdown. Check death/loot/XP once, arena save/rematch,
observations/replay, campaign save smoke and engine-owned allocation guards.

Use one Release configuration with targeted CPU tests; one resource-intensive job
at a time, at most two build workers and one test process, low CPU/I/O priority.
Production art gets an affected bake and selected hidden GPU captures covering
all clips/directions, gaps, overlapping ranks, diagonals, near/far, interruptions,
prone/death and mixed goblins. No OS input/focus automation or balance sweeps.

Retain arena scenarios (seed 5): one skeleton; two skeletons; two skeletons and one
goblin archer. Default party demonstrates mace vulnerability; a legal wizard
snapshot selecting Poison Spray demonstrates immunity. Arena level preparation
is B06, not this slice. Human interactive review belongs to the user.

Update the source ledger and system overview; record executed checks, limitations,
visual evidence and continuation in b01_verification.md. Finish Ready for review
only after complete art and behavior. User acceptance closes B01.

## Art registration integration

Reference approved; 21 original sheets and eight corrective imagegen edits retain
48 poses. Selected canvas height is 480 (rather than the goblins’ 536), with uniform
per-sheet scales and shared widths within clips, to keep the combined publication
within its existing 256 MiB mip budget. Source masters retain full resolution.
Body width remains .45 cells. Clip canvases use the existing world format’s <=1-cell
cap, including the idle/weapon padding of a taller skeleton; metadata now admits
that same existing bound for every clip. This changes no actor occupancy or world
format. Wide fallen canvases are capped at 560 pixels through uniform sheet scaling.

In-game capture found the existing initiative strip always used goblin art. B01
adds a generated skeleton portrait to the same startup UI atlas and a fixed HUD
portrait discriminator initialized from immutable monster identity. There is no
per-frame allocation, new GPU texture slot or save field.
