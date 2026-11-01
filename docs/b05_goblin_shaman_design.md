# B05 — Goblin shaman and enemy spellcasting

2026-11-01. **Ready for review.** The user approved the detailed
plan with “Implement the plan.” Reference v1 accepted with “reference approved”; completed-slice acceptance
remains pending. Campaign population, guardians, PC07 and B06 remain
unstarted.

## Content and responsibility

Deliver `goblin_shaman`, a Small one-subcell humanoid, with abilities
10/10/10/10/14/11, AC 10, fixed HP 9, DC 12, 50 XP and a staff used as a club:
+2, 1d4 bludgeoning (critical 2d4). No loot, damage modifiers, metal armor,
sleep immunity or Nimble Escape. SRD 5.1 Acolyte, printed p.395, is the baseline;
goblin identity, six mana, existing six-cell spell range and omitted spells,
skills/senses/languages are explicit project adaptations. No healing or Sanctuary
casting is added.

Catalog schema 13 adds typed `spellcasting` profiles `none` / `acolyte_support`;
older schemas default to none. Immutable profile enables only Bless/Sacred Flame.
ActorWorld retains identity/HP/occupancy; combat owns bounded per-actor mana,
target cursors, decisions and pending actions; effects owns Bless and concentration.
Prepared assets and the existing UI atlas own art. No generic caster framework.

## Policy and action economy

Six mana initially; Bless costs two, Sacred Flame is free. No regeneration/refill
on disengagement, victory or a new encounter. Fresh arena rematches reset resources.
Preserve standing, incapacity, surprise, restraint escape and six-second cooldowns.
Use one shared checked casting path for real-time approach and initiative turns.

Prefer Bless when not concentrating and another conscious unblessed encounter
ally is eligible. Choose up to three other allies in stable actor-slot order,
then self if space remains. Otherwise rotate Sacred Flame through conscious party
members in current formation order using a saved per-actor cursor; never inspect
HP. Sanctuary tries legal candidates in that order; all wards blocking spends the
normal action/cooldown. Melee/approach is the fallback, with movement consuming the
action. Never move and cast together. Preserve frontal encounter admission, LOS,
six-cell range and facing; Bless also permits co-located allies.

Casting capability participates in initiative selection/preview. An aware caster
with legal ranged geometry holds its position, including cooldown, in exploration
movement locks and round-end AI. No retreat/kiting behavior or arena placement
editor. Ordinary ranged attacks can reach it behind allies.

## Effect sources, reservations and damage

Use a tagged EffectSource containing either party CharacterId or world-checked
ActorId in effect requests, instances, lifecycle events and concentration queries.
No catalog identity as caster identity or long-lived actor pointers. NPC sources
are accepted only for Bless on NPC allies; unrelated spells/conditions/areas remain
party-only. One concentration per source; ordinary Bless nonstacking remains.
Death, removal and incapacity end concentration; prone alone does not.

Generalize damage notifications to both source kinds. Authoritative combat impacts
and burning Web notify exactly once after mitigation/HP commit. Zero/immune damage
consumes no concentration RNG. Surviving conscious sources save Constitution at
max(10, full adjusted damage/2), with existing effect modifiers; lethal/incapacitating
hits end concentration without a save. Preserve existing multi-hit granularity.

Reuse each actor's pending-action slot/attack serial with a small weapon/Bless/
Sacred Flame tag. Casting uses the existing ranged-action animation slot,
600 ms windup and 400 ms recovery. Resolve at release with no arrow flight.
Preserve pause, presentation holds, hurt suspension and pre-release cancellation.
Validate identities, legality, resources, serial/deadlines and capacity before
costs/RNG. Acceptance spends once; cancellation does not refund.

Reserve Bless capacity in the existing 64 effect slots before accepting; pending
Bless state is separate from Mirror Image reservation bits. Commit at release,
starting the 60-second duration then. Remove dead/removed recipients, add no new
ones; invalid source or zero remaining recipients cancels/releases capacity.
Sacred Flame privately retains its accepted 1d8 radiant roll and Dexterity save;
resolve save/damage events at release only. Downed/dead target cancels without
retargeting. No attack/save/damage disclosure in acceptance/cancellation events. Timings come
from the shared animation metadata; missing headless-only clips default to 600/400.

Retain 32 monsters/appearances, 256 actors, 64 effects, timeline capacities and
8192 events. Reservations consume effect capacity. Re-derive named event allowances
for NPC cast/save/damage plus concentration cleanup with compile-time proof; never
drop consequences. All storage is session-owned and allocated/prepared before
normal gameplay; bounded scans and fixed value results only in frames.

## Persistence, automation and feedback

Campaign schema/simulation become 4/7, rejecting old saves without rewriting or
migration. Save mana/cursors, tagged sources and canonical digest fields. Validate
source tag/slot/profile, resource bounds and reservation-free settled snapshots.
Canonicalize/rebind NPC source epochs with target epochs on floor export/import.
NPC resources/effects stay on their location and freeze while inactive. Portable
effects require party source; NPC Bless must not travel or require party dismissal.

Protocol v1 adds tagged caster source; existing party caster fields stay numeric
for party and null for NPC. Add visible NPC concentration, diagnostic mana/cursors,
resolved events with actor identity, and strict replay identity updates. Existing
arena/layout and world/appearance formats remain. HUD logs identify casting actor,
recipients, saves and concentration loss; ordinary incoming damage feedback applies.

## Art and review

Use imagegen skill and built-in tool with inspected shipped goblin/archer masters.
Tribal staff caster: muted ragged robes, bone charms, gnarled staff; related goblin
anatomy/scale and consistent handedness. Review transparent four-direction reference
beside both goblins; obtain user reference acceptance before full animation sheets.

56 poses /25 sheets: four-direction idle 600/600, walk 300/300, hurt 150/150,
death 200/200/200/600, static prone; front melee 700/250/250/350 (impact 950);
four-direction casting 600/400 (release 600). Preserve transparent masters, prompts,
references/corrections and measured registration under assets/npcs/goblin-shaman.
Stable pivots, unclipped staff/falling bodies, no normalization to standing height.
Reuse extraction/cropping/mips/alpha/shadows/picking. Portrait 86 and normal arena
binding. Casting has no arrow; reuse prepared effect textures for restrained cue,
radiant impact and Bless feedback.

Unchanged limits: 512 MiB mip, 64 MiB alpha, 352 MiB tracked CPU,608 MiB GPU.
Current six-appearance publication is 260.95 MiB mip /46.60 MiB alpha. Measure all
seven appearances. No speculative budget increase.

## Verification and handoff

Small self-owned fixtures cover policy, targeting/formation/wards, mana/no refill,
range/LOS/same-cell/front blocking, support position, action economy, conditions,
windup/pause/hurt/cancel, simultaneous ordering, timestep subdivision and hidden rolls.
Cover NPC concentration on every damage path, source/recipient cleanup, overlap,
modifiers, zero/immune/lethal hits, reservations/full storage/pre-cost rejection,
invalid/foreign identities and allocation guards. Save/restore mana/cursor/RNG under
fresh worlds; test inactive ownership/travel, malformed sources and old-file rejection.

Use one Release configuration, focused affected suites and save smoke during work;
full campaign persistence once at integration. One production bake, one affected
hidden GPU integration check and strict small arena replays/captures: solo shaman,
goblin-supported shaman, concentration break, multiple casters. Ordinary parties
remain level one; record roster/seed/choices and limitations. User reviews interaction
feel, motion and fun. One intensive job, <=2 build workers/one test process, low
CPU/I/O priority; no OS input/focus automation or routine benchmarks/matrices.

Record exact checks/captures/totals/limitations in b05_verification.md. Align this
design/system overview/roadmap and deliver Ready for review; only user closes B05.

## Delivery checkpoint

Mechanics, catalog schema 13, tagged source ownership, pending-Bless capacity,
local mana/target cursors, campaign schema/simulation 4/7, protocol feedback, full
56-pose production art, native portrait 86 and ordinary arena binding are implemented.
Animation registration uses 800×720 canvases with ground anchor 696 to retain raised
and fallen staff detail; .86 logical canvas height includes margins while the body
matches goblin scale. No count/byte budget increase. The conservative combined
event allowance is 6,728 <8,192; source cleanup stays in the existing lifecycle bound.

Reference v1 is accepted. [Verification/gallery](b05_verification.md) records exact
checks, strict production arena recipes/captures, byte totals and limitations.
Completed-slice acceptance remains pending; B06 and campaign/PC07 stay unstarted.
