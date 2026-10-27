# Slice 8.3: projectiles and impact-time consequences

Status: implemented, **Ready for review** (2026-10-27). The user
approved the plan with “Implement the plan.” Acceptance remains pending.

## Contract and ownership

CombatController owns 260 bounded pending attacks, one per possible attacker.
World-scoped monotonic attack IDs identify accepted attacks across reused slots;
restart invalidates them. Validation, costs, alerts and rolls happen once at
acceptance. HP, life, occupancy, defending state, reactions and result feedback
change together at impact. Renderers never resolve attacks or perform hit tests.
An unfinished attacker cannot overwrite its record; rejection precedes costs/RNG.

Updates publish bounded timestamped batches (524 events), separating acceptance,
release, impact and cancellation. All existing due attacks drain chronologically
before movement/AI/phase handoff. Flying impacts precede new releases on ties,
then attack serial orders ties. New automatic actions retain the existing
one-step-per-update policy. Cooldowns use gameplay time; deadlines use paused
session time. Contact, facing and results wait for pending attacks and their tails.

Party melee strikes at 200 ms with a brief slash; party arrows release immediately.
NPC strikes/releases use authored clip markers (zero when missing), including the
600 ms archer release. Flight is clamp(round(180 + 54*(distanceCells-1)),180,450)
ms, fixed at release. Movement/turning and later cover do not change saved rolls.
Targets are never replaced. Dead/removed targets cancel before release; after
release the arrow finishes without damage. Downed living party members still
receive existing health/death-save damage rules. Defeat wins mutual elimination.

Incapacitation/removal before release/strike cancels without a refund. Released
arrows survive shooter death. Nonlethal damage starts hurt/flash immediately,
suspends an unreleased NPC attack for the hurt interval, then resumes its remaining
windup without rerolling. Repeated hits extend the union of hurt intervals. Hurt
after release does not delay arrival. Holds include playback, 1000 ms feedback,
and victim reaction/corpse duration; turn handoff occurs in a separate update.

## Presentation and storage

A separate bounded cosmetic owner reads copied attack snapshots. Its 260 slots
use deterministic oldest replacement if exhausted; gameplay is unaffected.
NPC attack cues expose their paused playback timeline to NpcAnimator. HUD result
logs and feedback consume impact events directly; no UI-owned delayed damage
queue. Portrait hit highlighting lasts 150 ms; incoming target indication is
neutral. Movement clears selections, not pending consequences. Expired feedback
is not replayed after large time steps.

Projectiles use a slight arc, perspective sizes, frozen launch origins and tracked
target endpoints. Hits end in a glint; misses pass beside the target. Party launch
points are near the lower viewport by formation position. NPC attachments share
body projection, travel, strafe and inward front-band correction. Optional four-
direction release/torso metadata defaults centrally for older/generated assets;
metadata remains source-free and covered by definition fingerprints.

A compile-time immutable raster bank (at most 1 MiB) supplies arrow, slash and
impact variants, counted in prepared-memory reporting. No runtime preparation,
image decoding or frame allocation. Projectiles compose into CPU scene frames,
with preallocated depth scratch, wall/body occlusion and no picking contribution.
Movement endpoints and strafe compose the same shot timeline. Existing projection,
1920x1080 presentation, appearance IDs, profile C and surface policy remain.

## Validation and delivery

Generated fixtures cover exact strike/release/impact/hold boundaries; all attack
roles, hits/misses/criticals/lethal attacks; overlap and tie ordering; interruptions,
repeated hurt and resumption; invalid/dead/downed targets; pause, movement, contact,
facing, large steps, mutual elimination, Continue and restart. Assert exactly-once
RNG/cost/damage, no premature HP/life/log/occupancy, no early handoff, bounded storage
and visual exhaustion independence. Extend zero-allocation frame checks; keep
population stress separate. Run Debug/Release, focused sanitizers, prepared/source-
free checks, hidden GPU lifecycle and application-owned captures. No OS input or
window/focus automation. Recheck inherited broad-capture victory failure without
weakening its assertion. Record commands, limitations and selected native/reduced
captures in slice83_verification.md. End at Ready for review; no slice 09 work.

The final handoff must specify the authoritative impact/phase-clock contract and
small visual extension points for future spell projectiles, without implementing
spells/effects, ammunition, physics, atlases or a general VFX framework.

## Implemented interfaces and handoff

`combat_impacts.cpp` owns queuing, chronological draining, interruption and holds.
`GameSession::update` returns `const CombatEvents&`, valid until its next update;
consume the entire batch before updating again. `step` is only the one automatic
or phase event and excludes drained impacts. `CombatEvent::Attack` is the committed
result; `AttackStarted`, `ProjectileReleased` and `AttackCanceled` are separate.
`occurredMs` is the authoritative session timestamp. The legacy `impactMs` outcome
field is a strike/release offset, while `AttackVisual::impactMs` is an absolute
arrival deadline. Do not confuse either with the hybrid gameplay clock.

Records remain busy through their presentation tail, preventing overwrite even
when a cooldown expires first. Event capacity covers two events per record plus
four spare slots; violating that internal invariant aborts rather than losing
consequences. Contiguous rescans cost O(A²) for a whole due burst, bounded at 260
attacks. This avoids heap queues and permits hurt to change deadlines in place.
The separate stress check drains all 520 release/result events; measurements and
normal allocation checks are in the verification report.

`FramePipeline` owns `ProjectilePlayer` and two reusable picking buffers. Rendering
uses `PreparedNpcs::attachment`, sharing `projectNpc` with body placement; normalized
release/torso coordinates are immutable runtime metadata. Attachment changes
invalidate definition fingerprints without changing the bank layout. The arrow
rasters are compiled into the executable, not generated or resampled at startup.
NPC sources and materials are unnecessary at runtime. Actual prepared accounting
adds 2,048 attachment bytes and 1,012,160 projectile bytes to slice 8.2.

Future spell work can add a concrete visual kind/raster to the copied visual
record and compositor, and a typed consequence to the authoritative pending record.
Keep consequences, spell target validation and effect durations outside rendering;
never infer arrival from a disappeared cosmetic. Respect the two clocks and apply
round-based effects at the hybrid boundary defined by slice 08. No spell, effect,
concentration, ammo or general VFX abstraction has been added in this slice.

See [verification, captures and review recipe](slice83_verification.md).
