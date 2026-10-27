# Slice 09: effects and concentration

Status: implemented, **accepted and closed** (2026-10-27). The user approved
implementation with “okay implement the plan” and accepted it with “I accept the
slice, mark it as such in the plan doc”. Slice 10 is next eligible for planning
and remains unstarted.

## Scope and rules

Implement the effect consequences of Bless, Bane, Shield of Faith, Mage Armor,
and Hold Person. No casting UI, mana, class eligibility, spell range/area targeting,
progression, new artwork or general condition language. Future validated spell
execution calls the same checked application API at the authoritative application
time. Party members are the only casters; recipients can be party or NPC identities.
Mage Armor accepts only unarmored party recipients. Base character stats stay intact.

Bless/Bane add/subtract one d4 on attacks and saving throws, including death and
concentration saves. Bane initially allows Charisma; Hold Person allows Wisdom.
Bless, Bane and Hold Person concentrate for 60,000 gameplay ms; Shield of Faith
concentrates for 600,000 ms and adds 2 AC; Mage Armor lasts 28,800,000 ms and sets
unarmored base AC to 13 + Dexterity, retaining shield bonuses. Same-kind instances
coexist without stacking; different kinds combine, including separate Bless/Bane
dice. All adopted rules derive from the existing official SRD 5.1 ledger source.

One concentration instance per caster. Replacement validates/reserves before
changing anything or rolling; accepted replacement ends old concentration before
initial saves. No remaining recipient means no concentration. Positive damage
checks Constitution against max(10, floor(damage/2)) at impact, once per damage
source, using damage before HP clamping. Incapacitation/death ends concentration
without rolling. Non-concentration effects survive their caster's death. Remove
dead/invalid recipients; effects on downed living recipients otherwise persist.

Hold Person retries every 6,000 gameplay ms. **Outside turns this means every six
seconds; during turns this means at the end of a full round, after everyone's
turns.** Partial rounds and presentation time grant no retry. This deliberately
adapts the SRD's target-turn-end save. Paralysis prevents actions and further NPC
steps, cancels unreleased attacks, clears Defend, removes paired protection, fails
Strength/Dexterity saves, grants incoming advantage and critical hits on successful
close attacks (existing conservative adjacent-contact geometry). Released attacks
keep saved outcomes. Already committed steps finish visually. Mobile conscious
allies carry paralyzed party members; no mobile member blocks manual movement,
not simulation. An all-paralyzed conscious party skips turns until saves/expiry;
it is not defeated merely because it cannot act. NPC occupancy remains intact;
rear NPCs exposed by paralysis can attack from their existing subslot.

## Ownership, API and storage

GameSession owns EffectController after party/actors/RNG and before combat.
The controller borrows those owners; CombatController borrows effects. Use 64
contiguous instance slots, each with up to three recipients, caster, kind, DC,
expiry, retry deadline and serial. One instance represents one application/cast.
Shield, Mage Armor and Hold accept one recipient; Bless/Bane accept up to three.
No upcasting is introduced. Bounded scans favor simple ownership over heap queues.

Effect IDs contain world identity and monotonically increasing serials; recycling
a slot or restarting cannot validate an old ID. Serial/storage exhaustion rejects
before RNG/mutation; replacement may reuse the old concentration slot. Public
session operations apply/remove/end concentration and return checked outcomes;
read-only effect queries expose effective AC, modifiers, paralysis and can-act.
Requests include session identity even for party-local recipient IDs. Invalid
requests, duplicates and unsuitable recipients reject the whole request.

The existing CombatEvents stream gains typed effect payloads and 512 additional
entries (1,036 total). A borrowed batch remains valid until the next session
simulation operation, including apply/remove/submit/move; consume it first. For
one clamped update there are at most four Hold instances (one target each) and ten
retry deadlines per target: 40 retries, at most 260 damage concentration checks,
64 instance endings, and 72 recipient removals (60 non-concentration plus at most
12 concentration recipients). The conservative sum is 436, below 512. Application
batches are separate operations and fit this bound too. This fits 512 additional entries; overflow aborts as an invariant violation rather
than dropping outcomes. Events carry session and gameplay timestamps, raw roll,
modified total/DC and lifecycle reason. No string formatting occurs in simulation.

## Ordering and integration

Merge effect deadlines with existing release/impact deadlines. In real time,
advance gameplay incrementally through each due attack timestamp, processing
effects before attacks at a tie, then advance the remainder. Effect expiry precedes
retry saves, ordered by application serial then recipient identity. Existing flying
impact/windup/attack-serial precedence remains. During turns gameplay is frozen;
completed rounds advance 6,000 ms and drain effects before AI/next action. Pauses
freeze both clocks. Victory retains effects with frozen deadlines; Continue resumes.

Attack modifiers are captured at acceptance and never rerolled at impact.
Damage commits first, then concentration/invalid-recipient cleanup, before later
impacts. Effect application immediately reconciles Defend and pending windups.
Raw diagnostic party/actor mutations are reconciled before the next simulation
operation; they are not combat damage notifications. NPC AI receives a separate incapacitation mask: unlike ordinary travel/attack
locks, it suppresses autonomous facing as well as pathfinding. No effects means unchanged
RNG consumption and existing combat behavior. Death-save modifiers are separate
from the natural face so natural 1/20 exceptions survive.

## Verification and delivery

Generated headless fixtures cover each effect; partial and complete saves;
stacking/replacement; stale/foreign IDs; full capacity; cleanup and restart;
exact expiry/retry/impact ties; large versus subdivided updates; pauses and phase
changes; all-paralyzed turn progress; protection/occupancy/movement; accepted-roll
preservation; damage/concentration and death saves. Extend the frame allocation
guard; test maximum capacity separately. Run Debug/Release and focused sanitizers.
Provide `blobber_effects_tests --trace` for readable deterministic review; no OS
input/window automation or new graphical review. Record executed checks and limits
in slice09_verification.md. Implementation stopped at Ready for review; the user
has since accepted the slice. Recorded verification limitations remain.


## Implemented interfaces and handoff

`GameSession::effects()` is const. `applyEffect(request)`, `removeEffect(id)` and
`endConcentration(world, caster)` return `EffectResult`; consume `events()` after
synchronous operations. They are low-level simulation operations, not player
commands: they do not charge mana, validate spell class/range/humanoid eligibility,
provoke enemies, or grant a cast action. Those responsibilities stay in slices
10/13. Request DC is bounded to 1..1000; target counts are checked before accessing
arrays. An accepted application can return an already-ended ID when it incapacitates
its own caster. Every initial save succeeding returns an empty ID.

`EffectController` owns no dynamic storage. This compiler reports 8,744 bytes for
it and 190,816 bytes for `CombatEvents`, including the extra payload/capacity.
Bounded scans are O(64) for queries and O(64 squared) for serial-ordered cleanup;
there is no per-effect heap object or timer queue. `CombatSnapshot::gameplayMs`
remains the clock authority; effects mirror the time currently drained, including
intermediate impact timestamps. `resolveImpacts` now receives elapsed session time
from GameSession; round advancement passes the event sink into combat update.
Direct controller callers must preserve these clock/event contracts.

`CombatEvent::Effect` carries lifecycle/save details in `effect`, with session time
in `occurredMs` and logical effect time in `effect.gameplayMs`. Initial and repeated
saves use caster-supplied DC; ordinary NPC saves use current catalog ability modifiers
(the supported goblins have no save proficiency). Concentration uses full incoming
damage before HP clamping, with no die roll after incapacitation. Natural death-save
faces remain separate from a default-zero modifier in the pure health API.

Diagnostic direct health changes are reconciled for life/eligibility, but do not
pretend to be combat damage notifications. Future authoritative damage paths must
call the damage/concentration integration exactly once, just as existing impacts do.
Same-kind overlaps retain all instances and their independent deadlines; Hold
instances each retain their own saving throws, with paralysis lasting while any
survives. Armor eligibility is checked again during reconciliation.

With party-only concentration casters, a sustained all-paralyzed party cannot be
constructed legally: incapacitating the last caster ends their Hold. Tests cover
this self-incapacitation cleanup and individual skipped turns; the simulation
keeps defeat based on consciousness and can keep advancing rounds without actions.
No NPC-caster mechanism was added solely to manufacture that test. Actual spell
casting, upcasting, rest-time advancement, equipment transactions and persistence
remain later slices. No interactive or graphical review is claimed for this slice.

See [verification and handoff](slice09_verification.md) and the retained [headless trace](slice09_trace.txt).
