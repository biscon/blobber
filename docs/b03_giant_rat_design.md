# B03 — Giant rat and Pack Tactics

2026-10-31. **Accepted and closed (2026-10-31).** The user approved the detailed
plan with “Implement the plan.” Quick bite selected during planning. Reference
v3 approved by the user: “approved, thats a mean and nasty rat :)”. The user accepted the completed slice with “slice accepted, record it in the plan doc”.

## Delivery, responsibilities and ownership

Deliver standard `giant.rat`, its complete production art, native initiative
portrait and Pack Tactics in normal Arena Setup. Catalog owns immutable species
and attack data; ActorWorld owns checked identity/HP/occupancy; effects owns
conditions; combat owns support evaluation, saved rolls and delayed impacts.
Existing prepared assets/atlas own art. No new owner, cache, faction framework,
footprint, movement rule or campaign population change. Campaign/PC07 stay paused.

SRD 5.1 p. 378: abilities 7/15/11/2/10/4, AC 12, fixed HP 7, 25 XP,
`giant.rat.bite` +4, 1d4+2 piercing (critical 2d4+2). Small beast, nonhumanoid,
sleep-susceptible, unarmored, normal damage profile. No loot, gold or potion drops.
Omit Keen Smell, tabletop senses/speed and the diseased variant. No poison,
disease or new recovery requirement. Existing approach, formation protection,
one-subcell occupancy and six-second action economy remain.

## Trait and attack interfaces

Add PackTactics to the byte-sized rules::Trait mask. Catalog schema 11 recognizes
`pack_tactics` and `nimble_escape`, at most two distinct traits; schemas 1–10
retain their previous vocabulary/limits. Reject unknown/duplicate schema-11
traits and unsupported programmatic bits. Old catalog schemas remain readable.

At an accepted NPC attack, after target/ward selection and before the d20, scan
at most 256 actors, stopping at the first supporter. Require a different valid,
living, conscious NPC whose cell satisfies combatAdjacent against the shared
party cell. Same-cell allies and protected rear subcells count; species, facing,
awareness, encounter membership and individual party-character adjacency do not.
All current NPCs share the hostile side. Existing reach, conservative diagonal
LOS, elevations and door geometry apply. Use EffectController::conscious, not
canAct or the navigation incapacitation mask: Sleep/Hold Person prevent support;
prone, blind, slowed or restrained supporters still qualify while conscious.

Feed opening || supported into the existing attackMode resolver alongside all
existing disadvantages. Multiple advantages never stack; advantage/disadvantage
cancel. Support adds no RNG and is not queried per frame. Accepted attacks retain
their result if a supporter dies/is controlled during windup; subsequent attacks
re-evaluate. Existing attacker cancellation and hurt delay remain authoritative.

CombatOutcome adds compact NPC-weapon metadata: packTactics and attackRollMode.
Retain it in existing pending results. Publish/log it on resolved NPC weapon
attacks only, keeping acceptance/cancellation rolls private. Existing fixed log
reports “Pack Tactics: advantage” or “Pack Tactics: canceled by disadvantage”.
Automation adds pack_tactics and attack_roll_mode to resolved NPC weapon events;
protocol remains v1. No capacities grow; scans and gameplay feedback allocate no
engine-owned frame memory.

Campaign simulation identity becomes 5 (was 4); save schema stays 3, arena format
1. No saved structure changes. Reject old simulation/content without rewriting
files or migration. Fingerprints and strict replay retain their existing roles.

## Art and preparation

Use imagegen skill/built-in tool with actual shipped melee/archer references.
Review a four-direction reference before full sheets. User requested a more vicious, less cute rat: lean wiry body, angular muzzle, small narrowed eyes, exposed incisors and bristling crouch. Natural brown-gray fur,
rounded ears, incisors, whiskers and bare tail; shaded textured retro dungeon
style, no diseased features. Target .30-cell standing height and .40 body width;
anatomy-appropriate canvases fit existing <=1-cell limits. Body and picking mask
are not enlarged for readability. Keep registered feet/ground stable and fallen
poses low. Clip no tail, paws or whiskers.

48 poses / 21 sheets: four-direction idle 600/600 ms, walk 300/300, hurt 150/150,
death 200/200/200/600, static prone; front bite 300/150/150/300, impact 450 ms.
No unused ranged clip. Transparent masters, exact prompts/references/corrections
and measured extraction metadata live in assets/npcs/giant-rat; derivatives stay
in the build tree. Reuse full-resolution NPC cropping, mip/alpha, shadows and
picking. Add a generated native initiative portrait with an art ID outside the
reserved party-portrait range starting at 64. No renderer/atlas architecture or
memory/count budget changes. Report production publication totals.

Arena reuses existing selection/quantities/save/edit/seed/rematch. Review ordinary
level-one parties: one rat, three rats, two rats plus one melee goblin, then
thinning/control. These are bounded review recipes, not campaign balance claims.

## Verification and handoff

Small owned fixtures cover solo/same-cell/neighbor/mixed/protected/multiple support,
self/dead/removed/foreign IDs, distance/corner/door/elevation boundaries, Sleep,
legal humanoid Hold Person, conscious prone/blind/slow/restrained support, rat
spell restrictions, advantage cancellation/nonstacking, deterministic RNG,
miss/critical damage, 450 ms impact, interruption/cancellation, saved windups,
exactly-once XP and no loot. Guard support/acceptance/impact/feedback allocations.
Check loader versions, automation fields, portrait selection, arena persistence/
rematch, campaign continuation and previous-simulation rejection without rewrite.

Use one Release configuration, focused affected CPU tests, one production bake
after registration settles, one affected hidden GPU integration check and strict
production traces/selected application-owned captures. Inspect all directions,
diagonal near/far/overlap, alpha holes, selection among feet/shadows/corpses,
portrait, bite/hurt/prone/death. No OS input/focus/desktop automation. Human fun/
interactive review and unavailable checks remain explicitly unverified.

One intensive job, <=2 build workers, one test process, low CPU/I/O priority;
check RAM/scratch before costly work, bound internal threads. No routine benchmark,
stress, sanitizer or configuration matrix. Record executed evidence/limitations
in b03_verification.md, keep designs/checkpoint aligned, finish Ready for review.
Only the user closes the slice; do not start B04 or campaign integration.


## Delivery checkpoint

The approved design is implemented, including full art, native portrait, normal
arena selection, schema/trait/combat/feedback changes and simulation identity 5.
The rat reference and completed slice are accepted. Exact
checks, byte totals, five strict review traces, captures and limitations are in
[b03_verification.md](verification/archive/b03_verification.md). B04 is next for separate detailed planning; no B04 implementation or campaign work is started.
