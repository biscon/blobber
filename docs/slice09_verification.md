# Slice 09 verification and handoff

Status: **Accepted and closed**, 2026-10-27. Implementation was authorized by
“okay implement the plan”. The user accepted it with “I accept the slice, mark it
as such in the plan doc”. Recorded verification limitations remain unchanged.
Slice 10 is next eligible for planning and has not started.
See [implemented design](effects_design.md) and [headless trace](slice09_trace.txt).

## Delivered

GameSession owns a fixed 64-instance effect controller. Bless, Bane, Shield of
Faith, Mage Armor and Hold Person have bounded ownership, expiry, stacking,
initial/repeat saves, cleanup and checked application/removal. Effective combat
queries do not mutate base builds. Concentration is checked at positive damage
impact; downing/death/paralysis ends it immediately without a concentration roll.
Saved attacks retain their acceptance-time bonuses/results through flight.

Real-time effect deadlines drain chronologically alongside attack deadlines.
During turns, waiting and presentation grant no effect time; each completed full
round advances six gameplay seconds. Hold Person retries on that clock. Paralyzed
members cannot act or provide paired protection; mobile allies carry them. Held
NPCs retain occupancy and stop acting, walking and autonomously facing. Already
committed steps finish visually, and released arrows survive paralysis.

All review scenarios are generated, headless fixtures. No mana, casting commands,
spellbook, art, inventory or progression was added. Ordinary gameplay still has
no means to cast these spells; this slice supplies their consequences for later
validated spell execution.

## Checks executed

From the repository root:

```sh
cmake --build --preset debug -j4
cmake --build --preset release -j4
ctest --preset debug --output-on-failure
ctest --preset release --output-on-failure
cmake-build-release/blobber_effects_tests --stress
cmake-build-release/blobber_impact_tests --stress
cmake-build-debug/blobber_effects_tests --trace
cmake-build-release/blobber_effects_tests --trace
git diff --check
```

Final Debug **25/25 passed** (106.88 s); Release **25/25 passed** (11.90 s).
Both builds completed without compiler warnings. The extended frame guard passed
with **zero tracked C++ steady-state allocations**, including first application,
replacement, ordinary/concentration saves, expiry/retries, full capacity/rejection,
recipient death cleanup and session pause/update integration. Existing combat,
renderer, HUD and impact regressions also pass.

The separate capacity stress completes 100 full-pool application/replacement/
cleanup cycles in **32.4195 ms**. The existing full-population impact stress drains
all **260 attacks / 520 release-impact events**, without loss or replay, over 100
bursts: mean **281.638 microseconds**, observed maximum **490.215 microseconds**.
These measurements describe these bounded CPU checks on this machine, not total
frame-time guarantees. Debug and Release diagnostic traces compare byte-for-byte.

Focused sanitizer configuration reused the already downloaded raylib source:

```sh
cmake -S . -B /tmp/blobber09-sanitizers -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' \
  -DFETCHCONTENT_SOURCE_DIR_RAYLIB=/home/bison/Projects/blobber/cmake-build-debug/_deps/raylib-src
cmake --build /tmp/blobber09-sanitizers --target \
  blobber_effects_tests blobber_impact_tests blobber_combat_tests \
  blobber_hybrid_tests blobber_health_tests -j4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/blobber09-sanitizers \
  -R '^(effects|impact|combat|hybrid|health)$' --output-on-failure
```

Final ASan/UBSan **5/5 passed** (0.23 s). Leak detection was disabled, following
the preceding handoff's tracing restriction; it is not verified by these runs.
The focused checks are CPU-only. GPU lifecycle, screenshots and interactive
desktop checks were not run for this headless slice; they are not claimed as passes.

## Coverage and review findings

- All five effects; armor stacking versus base stats; Shield's ten-minute and
  Mage Armor's eight-hour boundaries; independent overlapping ownership.
- Initial complete/partial resistance, stable recipient order, duplicate/foreign/
  invalid recipients, invalid kind/DC/count, rejected-operation RNG preservation.
- Full pool, atomic concentration replacement at capacity, independent cleanup,
  invalidated/recycled handles, session restart and caster/recipient death.
- Bless and Bane independently affect saves; duplicate Bless adds only one die.
  Concentration includes active save modifiers; natural death-save 1/20 behavior
  survives modified totals. Paralysis auto-fails Strength/Dexterity without dice.
- Impact before expiry versus impact after expiry within a large update; the
  corresponding split updates preserve saved attacks and concentration dice.
  No premature HP/concentration changes or repeated damage/save events.
- Real-time six-second retries, pause, full-round retries, frozen player decision
  time, failed-save catch-up through expiry, victory retention and Continue.
- NPC/party paralysis, canceled windup versus released-arrow survival, no NPC
  movement, paired protection, rear attacks without occupying the held front slot,
  adjacent critical hits, caster unconsciousness and self-incapacitation cleanup.

Review caught that the existing movement-only AI lock still allowed `think()` to
turn a held NPC. A separate incapacitation mask now suppresses that behavior while
ordinary attack/travel locks retain perception. The added regression covers both
contact-facing and subsequent AI processing; all final suites include this fix.

The approved party-only caster scope makes a sustained all-paralyzed concentrating
party unreachable: incapacitating the last caster ends their Hold. Tests verify
self-incapacitation cleanup and skipped-turn behavior without adding NPC casters.
Defeat remains based on consciousness, not temporary action eligibility. This is
a fixture/scope limitation, not a claim to have exercised a legal all-paralyzed-party
scenario. Equipment-driven Mage Armor removal is implemented in reconciliation;
live equipment transactions remain slice 11.

## User review recipe

Run:

```sh
cmake-build-release/blobber_effects_tests --trace
```

The command first runs the effect checks, then prints the concrete scenarios:

1. **Real time and pause:** effects apply; a retry occurs at 6,000 gameplay ms;
   a 60-second paused update leaves the clocks unchanged.
2. **Turn-based:** waiting 60 seconds on a player decision leaves gameplay time
   at zero and produces no retry. Completing everyone's turns produces a retry
   at 6,000 gameplay ms and a full-round event.
3. **Impact:** at session 1,041 ms, the wizard still has 7 HP and concentration.
   At 1,042 ms, the arrow deals 4 damage, then the concentration save reports
   d20 6, modified total 10 against DC 10. The wizard has 3 HP and retains Bless.

The trace includes effect names, IDs, caster ownership, both clocks, save results,
end reasons and current stats. No window, user input injection or artwork is needed.

## Next-agent contract

- Effects are low-level simulation consequences. Future casting owns class,
  humanoid, range/visibility, resource/action costs and hostile provocation.
  An application is immediate at its caller-supplied authoritative operation time;
  future delayed spells must invoke it at their impact, not at visual disappearance.
- Consume **all** `CombatEvents`, including `CombatEvent::Effect`. A batch is
  borrowed only until the next session simulation operation, including synchronous
  apply/remove/submit/move. Read synchronous effect events through `events()`.
  `step` still describes the one automatic/phase step rather than drained impacts.
- Combat owns the gameplay clock; effects mirror the currently drained logical
  time. `occurredMs` uses session time; effect payload `gameplayMs` uses gameplay
  time. Expiry precedes retries and equal-time attacks. Round processing occurs
  before AI movement/next action. Preserve accepted attack outcomes.
- Apply concentration exactly once after authoritative positive damage. Direct
  diagnostic owner mutations only reconcile life/eligibility; they are not damage
  events. Cosmetic capacity and renderer playback never control effects.
- IDs are world/serial pairs, not pointers. The pool has 64 instances, each with
  up to three recipients; Hold, Shield and Mage Armor currently accept one.
  Future new kinds/upcasting/casters must revisit the 512-extra-event proof.
- Slice 09 is accepted and closed. Plan slice 10 in a clean context when requested;
  it remains unstarted. Acceptance does not turn unexecuted checks into passes.
