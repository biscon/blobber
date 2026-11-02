# Settled contact and current-facing approaches

Status: implemented and verified, 2026-11-02. Design recorded before implementation;
interactive acceptance remains user review.

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
