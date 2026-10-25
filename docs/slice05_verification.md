# Slice 05 verification and review

Implementation and acceptance date: 2026-10-25. Slice 05 and milestone A are
accepted and closed, including the revised UI. Historical results and limitations
below remain unchanged.

Current evidence and controls: [dungeon-first UI verification](dungeon_ui_verification.md).
That revision passed 17 Debug/Release suites and authorized hidden-window GPU/capture
checks. Earlier results below remain historical; their desktop restrictions and
invalid input test must not be mistaken for current rendering evidence.
See the [melee combat design](melee_combat_design.md) and
[roadmap execution record](dnd5e_demo_design.md#slice-05-first-melee-combat).

## Builds and headless correctness

From the repository root:

```sh
cmake --build --preset debug -j 4
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset debug
cmake --build --preset release -j 4
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset release
env -u DISPLAY -u WAYLAND_DISPLAY cmake-build-release/blobber_rules_content_tests --validate cmake-build-release/assets/rules/catalog.json
git diff --check
```

Both builds succeeded without compiler warnings. All **16/16 CTest suites passed
in Debug and Release**. Tests ran with display variables removed and did not open
windows. The two new suites are `health` and `combat`; existing preview, picking,
actor, movement, rendering, resource and catalog checks remain included.

Coverage includes:

- HP boundaries, negative/zero/huge amounts, massive-damage thresholds, healing
  from zero, stable-state damage, natural 1/20, three nonconsecutive saves/failures,
  invalid operations, terminal death and exactly-once lifecycle changes.
- Individual/group initiative ordering and ties; one action per participant per
  round; protected rear enemies skipping attacks; both row collapses from all four
  approaches; missing weapons, dead/removed/foreign targets and duplicate tokens.
- Rejected requests preserving both action and RNG sequence; target-specific
  Defend disadvantage and expiry; successful/failed Aid, ally-selection hotkeys,
  death saves and natural-20 recovery permitting the same turn's action.
- Queued/instant/animated movement entering contact without escaping it; focus/map/log
  pause inputs represented by the session pause flag; large frame gaps without
  catch-up turns; real fights reaching victory and defeat with preset mechanics.
- Victory preserving HP/deaths and stabilizing survivors; defeat with an entirely
  stable but unconscious party; restart restoring resources and rejecting old IDs.
- Current visibility/revision gating, log confirmation suspension, and input sampled
  before a new turn/result becoming ineligible to confirm it. Natural-20 recovery
  also must be presented before its newly available action can be confirmed.

The extended allocation guard passed with **zero engine-owned C++ allocations**
after preparation, including first encounter entry, automatic attacks, player
melee, Aid/Defend, healing, animation/composition/picking, HUD/log rollover, rejected
requests and victory/Continue. Fixtures use test-owned maps, catalogs and sprites.
Restart/setup/capture work and third-party/driver internals are outside this guard.

CLI checks accepted seeds `0` and `4294967295`, rejected negative, overflowing,
empty and trailing-character seeds, and rejected simultaneous `--ui-demo` and
`--combat-demo`. These checks used `--help` and confirmed no window initialization.
Shipped/copied catalog validation passed separately from fixture-owned CTest.

A focused AddressSanitizer/UndefinedBehaviorSanitizer combat build also passed:

```sh
c++ -std=c++17 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc \
  tests/combat_tests.cpp src/rules/resolve.cpp src/rules/health.cpp \
  src/rules/content.cpp src/rules/character.cpp src/world/map.cpp src/world/npc.cpp \
  src/game/movement.cpp src/game/party.cpp src/game/actors.cpp src/game/commands.cpp \
  src/game/combat.cpp src/game/session.cpp src/game/hud_state.cpp src/ui/interaction.cpp \
  -o /tmp/blobber-slice05-combat-sanitized
env -u DISPLAY -u WAYLAND_DISPLAY ASAN_OPTIONS=detect_leaks=0 /tmp/blobber-slice05-combat-sanitized
```

LeakSanitizer was disabled; this is an address/undefined-behavior check, not a leak
or production-scale performance claim.

## Rendering evidence and explicit limits

Before the desktop-automation restriction, the existing Release build generated
123 baseline PNGs at `/tmp/blobber-slice05-before`. An initial implementation build
then generated 130 PNGs at `/tmp/blobber-slice05-first`: the existing captures plus
seven combat cases. Both capture runs exited successfully. All **122 existing
logical viewport crops were pixel-identical** (RGB crop `(396,24,1896,840)`);
the window screenshot was excluded. Initial combat start, front-collapse and
victory images were visually inspected.

Those captures predate the final rear-target labels, always-visible death-save
counters, victory-log correction, three additional Aid/Defend capture cases and
confirmation gate. They are **initial rendering evidence, not final visual QA**.
Final captures and final standalone GPU lifecycle execution were not performed.
No virtual display is installed, and no package/toolchain changes were made.

The attempted interactive desktop test selected CLion instead of the game because
its window selector was incorrect. Its mouse/keyboard, pause, scaling, defeat and
restart observations are **invalid and must not be reported as passing**. The
input script was stopped; cleanup found no matching test processes remaining.
The script and invalid screenshots were removed. The user explicitly forbids
mouse/keyboard/window-focus/desktop automation; this is now recorded in AGENTS.md.
All subsequent checks were command-line/headless only. Interactive review is left
to the user. Windows/macOS, physical high-DPI, scale benchmarks and manual traversal
are also unverified.

## User review

1. Run the focused checks:
   `ctest --preset debug -R '^(health|combat|commands|frame_allocations)$'`.
2. Launch `./cmake-build-release/blobber --combat-demo --seed 5` yourself. The banner
   names the active character and round. Movement/turning are locked during combat.
   Ordinary play also enters combat when facing an adjacent occupied cell.
3. Hover a visible goblin to see reach and equipped damage. Click a reachable
   body to attack immediately, or cycle with Left/Right and press Enter. An
   unreachable click explains why and spends no turn. Inspecting a rear portrait
   opens read-only details; it does not replace the active character.
4. Click the shield/press 2 to Defend immediately. Click Aid/press 6, then click
   a dying ally's portrait to attempt stabilization. Aid can fail and never heals.
   Escape/right-click cancels targeting. Actions reset to the next actor's melee.
5. Kill one front goblin: its paired rear goblin becomes targetable immediately,
   while the other rear remains protected. On its next turn, the exposed goblin
   moves into the vacant front slot and attacks once. A downed party front member
   likewise exposes only its paired rear; party formation does not move. Check unconscious/dead/stable statuses and death-save counters. A
   natural 20 restores 1 HP and allows that character to act after the result is shown.
6. Defeat all goblins to see Victory. Continue preserves remaining HP and deaths
   and leaves surviving downed members stable. Alternatively, repeatedly Defend
   until actual enemy attacks cause defeat. Restart creates a fresh party/world
   using the same seed. A held Enter must not duplicate actions or restart unseen.
7. Manually check focus loss/return, F1 map pause, expanded-log pause, letterboxing
   and pillarboxing if desired. These desktop behaviors have headless coverage,
   but successful final desktop interaction has not been established.
8. To review deterministic images yourself, run:
   `./cmake-build-release/blobber --capture /tmp/blobber-slice05-review`.
   The shipped level adds ten `combat-*` cases: start, exchange, victory, defeat,
   front-collapse, healed-from-zero, log, aid, aid-result and defend. Healing and
   forced-health setup in capture scenarios use the real APIs but are diagnostic
   fixtures; normal play has no temporary healing command.
9. `--ui-demo` retains its isolated preview-only scenarios, sample mana/status
   values, and F2/F3 controls. Production combat never consumes those sample values.

## Delivery boundary

This is the stationary first-fight checkpoint, not the full demo. Shared maneuvers,
retreat/reinforcements, side/rear threats, cross-cell moving AI, new hurt/death
clips, reach, Rogue exceptions, ranged weapons, spells, inventory, XP/loot, resting
and saves remain in later slices. The approved temporary recovery rules are
immediate defeat when nobody is conscious and automatic post-victory stabilization
without HP restoration. Milestone A and slice 05 await user review; slice 06 has
not been started.

## Review corrections — 2026-10-25

Implemented after the user's screenshots: paired-slot protection on both sides,
melee goblins stepping into their own vacant front slot on their next attack turn,
one-shot existing attack playback on both hits and misses, and one-second red
damage/white MISS splats over monster bodies and party portraits.

Headless checks add single gaps in either lane from all four approaches; unchanged
blocked lanes, occupancy and exactly one attack per turn; miss cues changing actual
prepared pixels without HP mutation; clip completion/retrigger/fallback/death;
body-derived anchors, lethal feedback after body disappearance, correct portraits,
viewport bounds, same-recipient replacement, pause, exact expiry and restart. The
allocation guard explicitly observes both kinds of feedback and attack playback.

Review-correction verification: Debug and Release builds succeeded without compiler
warnings; all 16/16 CTest suites passed headlessly in each configuration. After
strengthening the allocation test to require observed attack playback and both
feedback destinations, it passed again in Debug and in the final Release suite,
with zero engine-owned C++ allocations. `git diff --check` passed. These results
cover the review corrections; earlier sanitizer/catalog/capture evidence above
belongs to the preceding delivery and was not rerun for these corrections.

Visual review additions: observe an enemy miss still swinging and white MISS on
the targeted portrait; observe your misses over monsters; compare red damage splats
on both sides with the supplied references. Killing blows retain their last body
position for the remaining splash lifetime. No new desktop captures or interactive
checks were run: final GPU appearance is unverified and must be reviewed by the user.
Slice 05 and milestone A remain Ready for review, not accepted; slice 06 is unstarted.


## Dungeon-first UI follow-up — 2026-10-25

The user accepted the mechanics and requested the approved UI redesign. It is
implemented and Ready for review. See [current verification](dungeon_ui_verification.md)
for 17/17 Debug/Release results, hidden GPU lifecycle execution, application-owned
captures at three sizes, retained screenshots and the updated review recipe.
The prior invalid CLion test remains invalid. The new checks never select/control
windows, inject OS input, capture the cursor or switch desktops. Overall slice 05
and milestone A still await acceptance; slice 06 remains unstarted.


## Acceptance and closeout — 2026-10-25

User: “much better, this slice is accepted, close it in the document”. Slice 05,
including combat corrections and the dungeon-first UI revision, and milestone A
are accepted and closed. This supersedes earlier pending-review statuses in this
historical verification record. No additional conditions were requested and no
unexecuted checks are reclassified as passing. Slice 06 is eligible for planning
in a clean context; it remains Planned and unstarted. See the
[roadmap checkpoint](dnd5e_demo_design.md#current-checkpoint).
