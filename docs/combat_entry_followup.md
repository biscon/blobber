# Settled contact and current-facing approaches

Status: implemented and verified, 2026-11-02. Design recorded before implementation;
interactive acceptance remains user review.

[Combat positioning](combat_positioning.md) supersedes the central convergence and
frontal-only turn eligibility policies. Earlier entry/facing fixes and historical
verification below remain recorded.

Keep the accepted frontal hybrid loop: real-time movement until genuine close
contact, locked party movement during initiative, automatic facing when the active
front clears, normal initiative/action costs and dedicated ranged retreat policy.

Contact must involve a living, arrived adjacent NPC and a settled player endpoint.
ActorWorld reserves destinations immediately for collision, but a walking actor's
reserved cell must not lock the player before its presentation arrives. GameSession
checks a movement endpoint before consuming queued input so continuous queued
walking cannot skip contact. Existing Contact waits for pending impacts and other
presentations before beginning initiative. Losing settled close contact returns to
Approach through the existing safe handoff without charging a partial round.

Front occupancy must include an exposed, in-view melee target. A visible rear body
whose protector is outside the view is not a sufficient reason to hold the camera.
Parameterize the existing paired-row exposure query by candidate heading and use
it for automatic-facing selection. Keep protection and incapacitated-front rules;
do not make invisible protectors attackable or globally remove rank protection.

Navigation requests identify from/target/adjacency/preferred-front. A request whose
fields change cancels an old active search or ready result and queues the latest
request. Preserve the current bounded cleanup and fairness budgets, checkpoint
layout, generation checks and allocation-free storage. Every current-party pursuit
prefers party.cell+forward(party.heading), including flank/rear approaches; legal
diagonal/front fallback remains when the centre is full or unreachable.

NpcAiController owns approach subslot choice: preserve a free near-row slot, try
the paired near slot then the other near slot before a free rear slot. Try visible
slots first, then the same order outside the viewport if necessary, so a hidden
near slot does not take priority over a usable rear slot at a diagonal. Derive the
row from current party facing. Only approach movement uses this policy; wandering,
patrols, investigation of an old location and retreat keep their own movement.
Combat uses the same approach path for eligible adjacent flank-to-centre moves and
distant healthy melee-capable enemies before a fallback shot. Movement spends its
normal enemy action, rather than granting a move plus an attack. Existing in-cell
rear advancement retains its existing action/paired-protection behavior. Dedicated
casters/ranged-only enemies retain their attack policies; wounded mixed enemies
retain Nimble Escape. No new formation manager, hidden teleport, heap allocation,
serialized cache or movement system.

Use owned fixtures to reproduce hidden-protector entry, far-only/retreated fronts,
walking arrivals, queued player endpoints, every heading/subslot, centre contention,
and stale ready/incremental routes after a turn. Verify front-target availability,
current-facing approach, normal action/round clocks, ranged-retreat exceptions,
pause and allocation bounds. Run focused CPU tests, then affected callers once;
use bounded hidden production-art views when useful. Preserve current save format
and data; strict traces retain executable identity. At most two low-priority build
workers, one intensive job, no desktop input/focus automation or balance sweep.

## Verification

Owned CPU regressions in `ai_tests`, `hybrid_tests` and `combat_tests` cover:

- Eight hidden-protector cases across four headings and both diagonals. Entry
  faces an exposed body and the fighter can validate a melee command.
- A cleared front releasing movement without charging a partial round; the player
  can move closer to the remaining distant enemy.
- NPC travel not starting contact before its 600 ms arrival; player travel not
  starting contact before its 160 ms endpoint. Pause remains frozen. Queued turns
  and escape steps cannot skip the contact endpoint.
- Four headings with four contending arrivals: both near slots fill first and
  all four reservations remain distinct. Healthy mixed goblins advance before
  shooting, join the central front and continue melee across rounds. Ranged-only
  holds and wounded retreat keep their existing coverage.
- Ready routes being replaced after a turn, and active incremental searches
  canceling/replacing without stale publication or exceeding the 4096-node budget.
  The 67-square owned fixture is just large enough to cross one search budget;
  it is not a capacity benchmark.

The three focused suites passed, followed by sixteen affected caller suites:
`actors`, `core`, `commands`, `effects`, `impact`, `spells`, `expanded_spells`,
`giant_rat`, `bugbear`, `shaman`, `zombie`, `automation`, `automation_allocations`,
`generation_automation`, `campaign_save`, `gameplay_startup`. The latter run took
7.21 seconds, including the full campaign save integration. Existing engine-owned
allocation checks pass. No sanitizer, stress, balance sweep or alternate build
configuration test matrix was run.

The opt-in hidden render diagnostic passed:

```sh
nice -n 15 ionice -c 3 cmake-build-release/blobber_world_integration_gpu_tests \
  cmake-build-release/assets docs/screenshots/combat-entry --combat-entry-review
```

It uses owned safe mechanics and production Underkeep/goblin visuals. Both fighter
decision frames contain a CPU-picked, exposed target accepting melee; GL error and
world resource-release checks pass. [Two inspected captures](screenshots/combat-entry/README.md)
show automatic entry facing and a turn-based near-row arrival. The initial
diagnostic setup failed an appearance-count precondition before rendering; it was
corrected to bind the complete production appearance list to owned definitions.
No desktop input/focus control or user save mutation was used.

The current campaign schema/simulation remain 7/11. The navigation checkpoint
layout is unchanged and the removed alternating-shot `closeNext_` flag was never
serialized. Strict traces continue to include executable identity. Release and
CLion Debug applications are rebuilt for this correction.

## Resume interrupted rounds — 2026-11-08 correction

Status: implemented and mechanically verified; interactive acceptance pending.
Approved design was recorded before implementation. A brief loss of melee contact
resumes the current initiative cursor, enemy-group cursor and spent actions. It
must not restart initiative or advance caster-turn effects a second time. Arrivals
join at the next round boundary, as during uninterrupted turns. If at least the
remaining six-second round interval passes in real time, contact starts a new
round with the retained initiative rolls. Initial real-time shots still debit the
first contact opportunity; contact changes never grant free actions.

CombatController owns one transient `roundEndMs_` deadline alongside its existing
initiative state. Actions taken in turns, including Cunning Action and
spiritual-weapon followups, become ready at that boundary; real-time actions keep
their six-second deadlines. At a round boundary only the remaining
time advances, so approach time is not charged again. Effects and nonparticipant
movement receive that remaining interval. These values require no save fields:
only settled exploration can be persisted, where initiative and the deadline are
empty. Work remains bounded by existing party/actor arrays, with no frame allocation.

Turn selection consumes unavailable conscious-party opportunities internally,
while still processing caster-turn effects and ground checks once. Dying actors
retain their death-save opportunity. Characters with Action Surge, Cunning Action
or a ready spiritual weapon remain selectable. Explicit log text distinguishes
cooldown, enemy surprise, voluntary end-turn and no available action. Recontact is
announced as a resumed round. Tests must cover short/long approach intervals,
spent/unused actions, exact remaining-time accounting, initial-shot cooldowns,
newcomers, death saves and existing allocation/persistence invariants.


### Verification of the 2026-11-08 correction

All twenty affected Release suites pass: `combat_movement`, `combat_positioning`,
`shaman`, `zombie`, `bugbear`, `level_spells`, `expanded_spells`, `spells`, `effects`,
`impact`, `hybrid`, `ai`, `combat`, `hud`, `commands`, `automation`,
`automation_allocations`, `campaign_save_smoke`, `world_visibility`,
`gameplay_startup`. The integration run took 6.03 seconds with nineteen passing;
after correcting an offscreen Poison Spray fixture, only `expanded_spells` was
rebuilt/repeated and passes in 0.01 seconds. No production change followed the
integration run. No broad alternate-configuration, sanitizer or stress matrix.

Build the corresponding `blobber_*_tests` targets with the configured CLion CMake,
then run its CTest with `--test-dir cmake-build-release -j1 --output-on-failure`
and a regex selecting the names above (`campaign_save_smoke` shares the
`blobber_campaign_save_tests` binary). New owned checks cover 0/1200/7000 ms breaks,
retained round identity and spent/unused opportunities, no false active-party
snapshots, next-boundary readiness before/after the break, deferred new monster
groups, one-shot enemy surprise and explicit log attribution. Existing death-save,
initial real-time shot, status, movement and allocation regressions continue to pass.

Surprise remains the existing adaptation: only an unaware, never-before-enrolled
NPC that cannot see the party at opening contact is surprised. It loses its first
opportunity once. Party members cannot be surprised; no stealth-versus-perception
contest or repeated surprise on recontact is introduced. The previous ambiguous
log was not evidence of party surprise.

Campaign schema/simulation remain 19/23. The new deadline is encounter-only and
safe exploration saves have no initiative to restore; save/load smoke passes.
Interactive play and exact visual doorway aperture behavior remain unverified.
The CLion Debug `blobber` application rebuilt successfully with the configured
CMake/Ninja, at two low-priority workers, without compiler warnings. Its normal
asset dependencies rebaked prepared atlases/world assets; these remain build
caches. No additional Debug test matrix or interactive desktop review was run.
The owned agent run was previewed and cleaned after all runners closed (14 files,
69,145 bytes); no captures or unresolved diagnostics were retained.
