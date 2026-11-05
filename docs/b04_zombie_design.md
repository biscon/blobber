# B04 — Zombie and atomic Undead Fortitude

2026-10-31. **Accepted and closed (2026-11-01).** The user approved the detailed
plan with “Implement the plan.” The decayed-villager visual direction is selected;
reference v1 accepted by the user with “reference accepted”. The completed slice
was accepted with “slice accepted, record it in the plan doc”.

## Delivery and ownership

Deliver `zombie`, full production art, native initiative portrait and normal Arena
Setup availability. Catalog owns immutable species/attack data; ActorWorld owns
checked identity, HP, occupancy and death markers; effects owns save modifiers;
combat owns accepted rolls and impact scheduling. A small shared NPC damage helper
borrows actors/effects and returns value results without owning state. Existing
prepared assets and UI atlas own artwork. No campaign population, guardian, PC07,
new footprint, condition framework or NPC caster work is included.

SRD 5.1 printed pp. 356–357: abilities 13/6/16/3/6/5, AC 8, fixed HP 22,
`zombie.slam` +3 for 1d6+1 bludgeoning (critical 2d6+1), 50 XP. Medium undead,
poison immunity, no vulnerability, sleep-immune, nonhumanoid, nonmetal clothing.
No loot/currency/potions. Existing ability-based NPC saves give Constitution +3;
omit tabletop Wisdom save proficiency, speed, senses, languages and the unsupported
poisoned condition. Existing one-subcell placement, formation protection, discrete
movement and six-second action economy apply.

## Atomic NPC damage and interfaces

Catalog schema 12 adds `undead_fortitude` to the byte trait mask, allowing all
three distinct traits. Older schemas retain their vocabulary/count limits.
Unknown/duplicate traits and unsupported programmatic bits reject.

The shared helper receives ActorWorld, EffectController, ActorId, amount, concrete
damage type, critical flag and timestamp. It validates identity/life/time before
save RNG, applies the existing damage profile exactly once and returns mitigation,
ActorResult and compact Fortitude metadata. Combat impacts and burning Web call
the same helper; party health remains on its existing path. Short-lived borrowed
actor references do not escape the call. No heap storage, callbacks or per-frame
trait scans are added.

Positive post-mitigation damage at least equal to current HP triggers Fortitude
for the trait only. Radiant damage or critical hits bypass it, with radiant taking
precedence in explanatory metadata. Otherwise effects' existing Constitution save
uses DC 5 + full post-mitigation damage, not HP removed or overkill alone. Saturate
the 64-bit DC on overflow. Ordinary save rules/modifiers apply; a natural 20 does
not automatically succeed. Even an impossible eligible save consumes its ordinary
roll. No Fortitude RNG occurs for miss/immune/zero/nonlethal/invalid/gone targets
or either bypass.

ActorWorld::damage gains a narrowly typed retain-one-HP disposition, defaulting
to ordinary lethal damage. On save success commit HP >=1, positive-hit timing and
revision together, including at 1 HP with zero HP removed. Never set hasDeath,
vacate occupancy, change actor identity or briefly enter death. On failure/bypass
commit ordinary death once. Positive surviving hits retain hurt/interruption,
wake and effect reconciliation. Reported damage remains the full adjusted amount;
ActorResult separately reports actual HP removed.

Fortitude is resolved at damage application, never during windup. Accepted attack
rolls remain saved; impacts and equal-time recipients keep existing ordering.
Magic Missile stays aggregated per target; separate Scorching Ray impacts each
resolve against current HP. Dead/removed later targets consume no Fortitude RNG.
XP/loot/objective/arena-clear continue to derive from final death markers only.

CombatOutcome gains Fortitude status and save roll/total/64-bit DC. Publish it only
on resolved attack events, with additive automation fields and no extra event or
capacity growth. Fixed HUD feedback shows “1 HP” on survival and a log explaining
Undead Fortitude, save total/DC, failure or radiant/critical bypass. Pending,
released and canceled results reveal no Fortitude information.

Campaign schema/simulation become 3/6 (was 3/5). No saved structure changes:
existing snapshots store final HP/life/markers and RNG, with no trait charge or
resurrection state. Old simulation/content rejects without migration or rewriting
files. Arena format stays 1, protocol v1, world/appearance formats 3/4. Normal
save/travel safety still requires settled safe exploration.

## Production art and arena

Use imagegen skill and built-in generation tool with inspected shipped goblin and
archer masters as references. First review a four-direction turnaround beside both
goblins; obtain reference acceptance before full animation sheets. Unarmed decayed
human villager in ragged muted clothes, gray-green skin, slack posture and restrained
wounds; shaded textured retro dungeon style and human scale. Target .45-cell body
width and .75-cell logical canvas height within existing publication bounds.

48 poses / 21 sheets: four-direction idle 600/600 ms, walk 300/300, hurt 150/150,
death 200/200/200/600 and static prone; front slam 700/250/250/350, impact 950 ms.
No unused ranged animation. Transparent masters, exact prompts/references/corrections
and measured registration live under assets/npcs/zombie. Keep uniform per-sheet
scale/ground anchors, unclipped limbs and falling poses at their anatomical scale.
Reuse offline extraction, full-resolution cropping, mip/alpha, shadows and picking.
Add native portrait at the next public art ID after giant rat (85), retaining party
portrait IDs 64–83. Extend fixed atlas/portrait selection without a new owner.

Register the appearance in the normal NPC definition list. Existing arena
selection, quantities/order, saved scenarios, seeds, editing and rematch apply.
Review one zombie and two zombies with ordinary level-one parties and available
radiant finishers. Record parties, spells, seeds and limitations; no new level UI,
automatic balance sweep or claim of fun. Keep all count/memory capacities fixed;
report production publication totals against 512 MiB mip/64 MiB alpha/352 MiB CPU/
608 MiB tracked GPU ceilings.

## Verification and delivery

Small self-owned fixtures cover save success/failure, exact-lethal/overkill DCs,
repeat saves at 1 HP, hurt/no death presentation, stable occupancy/identity,
zero/immune/nonlethal damage, both bypasses, invalid/foreign/dead/removed targets,
invalid time, modifiers, huge damage, deterministic RNG and no engine allocations.
Exercise real melee/delayed/spell/area/Spiritual Weapon/Web paths, existing
multi-hit granularity, simultaneous ordering, cancellation and timestep subdivision.
Assert no premature XP/loot/arena/guardian consequences and exactly-once real death.

Check loader versions, private pending metadata, resolved automation/feedback,
portrait/arena rematch, live-at-1/dead floor roundtrips under fresh world identities,
RNG continuation, unchanged save/travel admission and simulation-5 rejection with
unchanged bytes. Use one Release build, focused affected CPU tests and campaign
save smoke, one settled production bake, one affected hidden GPU integration check
and selected strict arena replays/application-owned captures. Inspect four views,
near/far/diagonal/overlap, picking, portrait and hurt/prone/death/survival feedback.

One intensive job, <=2 build workers, one test process, low CPU/I/O priority;
check memory/scratch and bound internal threads. No OS input/focus automation,
routine benchmark/stress/configuration matrix. Record executed evidence/limitations
in b04_verification.md, align designs/checkpoint and finish Ready for review only
when complete. Only the user accepts the slice; do not start B05 or campaign work.

## Delivery checkpoint

The approved mechanics, source ledger, schema 12 / simulation 6 identities, full
48-pose production set, native portrait and normal arena binding are implemented.
Reference v1 and the completed slice are accepted.
[Verification/gallery](verification/archive/b04_verification.md) records exact tests, four strict arena
traces, selected hidden captures, byte totals and limitations. No B05, campaign
population/guardian or PC07 work is started. B05 is next for separate detailed planning.
