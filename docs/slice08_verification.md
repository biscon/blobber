# Slice 08 verification and handoff

Status: Accepted and closed, 2026-10-26. Implementation was authorized by “Implement
the plan.” Slice 07 and its two corrections were accepted before this work.
The user accepted slice 08 and milestone B, including all review corrections.
Slice 09 remains Planned; discuss graphical changes before starting it.
The verification/review notes below record the checks and limitations at delivery;
the acceptance record at the end supersedes their pending-review instructions.

## Delivered

The [approved design](hybrid_encounter_design.md) is implemented: shared NPC AI,
typed Nimble Escape, schema-3 ranged equipment, six-cell bows on both sides,
local alerts, NPC surprise and boundary-enrolled reinforcements. The approved
review correction replaces shared maneuvers with direct initiative and automatic
facing after the adjacent front group clears. Facing preserves the active action;
offscreen enemies cannot attack, and the front row always protects its paired
back row. No Hold/Exit, retreat command or party surprise penalty remains. Gameplay
and presentation clocks are separate; cooldowns, Defend and saves survive phase
changes. Existing travel, immediate death occupancy release and attack holds remain.

The HUD offers ranged selection, cooldown/spent states, threat directions,
arrival count and direct action controls. Automatic turns show the destination
heading and explain that the action is preserved. Portraits select during real-time play;
[i] opens details. Four prepared bow views each have draw/release poses, with
release/impact at 600 ms and a 1000 ms feedback marker. Legacy clips remain readable.

Implementation is split across `combat.cpp`, `combat_actions.cpp`, `combat_loop.cpp`,
`npc_ai.cpp`, `perception.cpp` and the session/UI boundaries. No later spells,
effects, ammo spending, loot or persistence systems were introduced.

## Automated checks

Commands run from the repository:

```sh
cmake --build --preset debug -j4
ctest --preset debug
cmake --preset release
cmake --build --preset release -j4
ctest --preset release
```

Both configurations: **20/20 passed**. Builds reported no compiler warnings.
Routine coverage includes exact six-cell range, blocked/corner LOS, invalid and
stale commands, no-cost rejection, non-chaining alerts, paused clocks, contact
presentation holds, approach cooldown spending, six-second round advancement,
all four contact directions, steady front facing, clockwise tie/rear turns,
attack-marker and pending-arrival travel holds, paused/stale input during facing,
preserved fighter decisions, offscreen attack suppression, partial-round re-entry,
NPC surprise, fixed paired protection,
trait-controlled/blocked escape, bow timing, reinforcement entry and pending
reinforcement victory prevention. Save deadlines survive real-time-to-turn entry.
The maximum 256-actor encounter enrolls without truncation and has an eight-entry
HUD preview. Schema 1/2 compatibility and schema 3 trait/ranged references,
invalid hands/slots, four-view bow loading and forbidden placement cues are tested.

All combat tests and captures use GameSession directly; maneuver fixture adapters
were removed. The frame allocation guard includes cold bow/alert/contact, a new
arrival, round transitions, automatic side-facing and HUD refresh: **zero engine-owned C++ allocations** after preparation. This is
not a claim about driver allocations or an exhaustive allocator interception.

Focused AddressSanitizer/UndefinedBehaviorSanitizer check passed:

```sh
c++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -Isrc -Itests tests/hybrid_tests.cpp src/game/npc_ai.cpp \
  src/game/navigation.cpp src/game/perception.cpp src/game/session.cpp \
  src/game/combat.cpp src/game/combat_actions.cpp src/game/combat_loop.cpp \
  src/game/party.cpp src/game/movement.cpp src/game/npc_animation.cpp \
  src/game/actors.cpp src/game/commands.cpp src/game/hud_state.cpp \
  src/ui/interaction.cpp src/world/map.cpp src/world/npc.cpp \
  src/rules/resolve.cpp src/rules/health.cpp src/rules/character.cpp \
  src/rules/content.cpp -o /tmp/blobber-front-sanitized
ASAN_OPTIONS=detect_leaks=0 /tmp/blobber-front-sanitized
```

LeakSanitizer was not run; prior environment limitations remain applicable.

Separate Release stress runs from the initial slice 08 implementation (the
review correction does not change navigation or sprite preparation):

```sh
cmake-build-release/blobber_ai_tests --stress
cmake-build-release/blobber_npc_animation_tests --stress
```

Navigation: 256 actors, 25×25 map, 63 ticks/3,750 scratch bytes/2 ms; 1024×1024,
130,306 ticks/6,291,456 scratch bytes/5,410 ms. Work is bounded, but large-map
latency is not suitable evidence of responsive gameplay. Generated-art travel and
picking: 256 actors, 120 compositions, 91.05 ms total (0.759 ms/frame, CPU only).
These do not establish production-art throughput at maximum population.

## Art and presentation

Selected generated sheets and prompts live under
`assets/npcs/goblin/animation/`; `sheets.json` records registration. The existing
preparer produced all eight bow frames:

```sh
cmake-build-debug/blobber_prepare_npc_animation assets/npcs/goblin/animation/sheets.json
```

Four 512×536 bow canvases use width 0.6 to retain the 384×536/0.45 idle scale,
shared vertical scale and grounding. No collision size changed. Shipped source
plus prepared NPC pixel storage measured **160,526,125 bytes** (153.09 MiB), below
**167,772,160 bytes** (160 MiB); the cap was not raised. Application startup reports 58,179,584 source bytes
and 102,346,541 prepared bytes including shadows; hidden captures confirm the
same combined total as the headless preparation preflight.

Hidden application-owned captures and GPU lifecycle checks completed:

```sh
cmake --build --preset debug --target blobber_gpu_smoke -j4
cmake-build-debug/blobber_gpu_smoke
cmake-build-debug/blobber --capture-hybrid /tmp/blobber-front-captures
cmake-build-debug/blobber --capture-hud /tmp/blobber-front-hud
```

Hybrid output contains 119 PNGs plus a phase/clock/actor trace. Cases include
opening shot/pause, side/rear contact, the front-to-side fighter handoff, wounded
escape, reinforcement and four bow orientations at 0/599/600/999/1000 ms.
The handoff fixture removes the front goblin directly while Alden is ready to
isolate the reported active-decision problem; it is not a simulated killing attack.
Separate CPU regressions verify that real attack/impact holds finish before turning.
Selected handoff and rear-contact captures were inspected for action preservation,
facing state and layout. HUD captures also cover impact/turn handoff, reduced
1280×720 and letterboxed 4:3 presentation. Original bow views and grounding were
visually inspected during initial implementation; their art has not changed.

The sandbox could not open X11; the hidden runs succeeded with approved escalation.
No OS input, window focus changes or desktop automation were used. Interactive
feel, physical keyboard/mouse operation and final artistic acceptance remain for
the user. CLI help and incompatible hybrid-capture/AI-demo arguments were checked;
`git diff --check` passed.

## Review recipe and next agent

Run `cmake-build-debug/blobber --hybrid-demo --seed 5`. Select Merrin's portrait,
press 3 for Ranged, and click a visible goblin. On contact, use the normal action
dock immediately. Facing stays steady while the front group lives; when it clears,
the camera turns to an adjacent group and resumes initiative without a maneuver
prompt or action cost. Side/rear enemies wait until faced. Use [i], menu or F1 to
check pause behavior. Player movement/turning is locked during combat.

Inspect `/tmp/blobber-front-captures/hybrid-front-handoff-fighter-before.png`,
`hybrid-front-handoff-turn-start.png`, `hybrid-front-handoff-fighter-after.png`
and `hybrid-front-handoff-fighter-attacks.png` for the reported fighter case.
These are local diagnostic exports, not repository assets. The preserved active
fighter can attack immediately once the camera settles. Rear-contact captures
exercise two quarter turns without an intermediate player decision.

The original 1920×1080 presentation, 84-degree FOV, discrete occupancy and 600 ms
visual NPC travel remain. Nimble Escape, NPC surprise, no creature cover and
front-focused combat are explicit project adaptations recorded in the
[SRD ledger](srd_content_ledger.md).
No material ambiguity remains from the approved plan. Review corrections belong
to slice 08. Do not begin slice 09 until the user accepts slice 08 and milestone B.

## Single-weapon AI review correction

Implemented at the user's request: melee-only actors close and attack at either
health level; ranged-only actors hold legal shots, approach when blocked/out of
range, and try to escape adjacency at any health. Trapped archers shoot. Mixed
weapon goblins retain their current approach, melee preference and wounded escape.
No random preferences, caster policy or later-slice systems were added.

Two bow-only goblins now stand at (2,7) and (4,7), ahead and to either side of the
starting party in `--hybrid-demo`. They use `goblin.archer` rules and the original
appearance, including the existing idle/walk equipment imagery. Optional typed
placement overrides separate rules from appearance; invalid overrides fail during
ActorWorld construction. No placement JSON schema or prepared art changes.

Validation for this correction:

- Debug and Release: **20/20 passed**, no build warnings.
- Generated hybrid fixtures cover repeated ranged shots without closing in real
  time and initiative, pursuit from blocked LOS/outside range, healthy escape
  without a trait, blocked escape shooting, persistent escape action costs,
  melee pursuit/attacks at both health levels and rules-variant validation.
- Existing mixed-weapon, pause, reinforcement and front-facing regressions pass.
- Frame allocation guard includes cold ranged-only pursuit, repeated shooting,
  adjacent escape and phase transitions: **zero engine-owned C++ allocations**.
- Focused ASan/UBSan hybrid executable: **passed** (`detect_leaks=0`). Same compile
  command as above, output `/tmp/blobber-weapons-sanitized`.
- `cmake-build-debug/blobber --capture-hybrid /tmp/blobber-weapons-captures` exercises
  shipped catalog binding and hidden rendering. New `hybrid-demo-initial.png` and
  `hybrid-ranged-only-100.png` were visually inspected. The ranged-only script
  asserts at least two shots while both archers remain at their original cells.
  Combined source/prepared pixels remain **160,526,125 bytes** below the 160 MiB cap.
- Documentation file links and `git diff --check` pass. No OS input/focus automation.

Review with `cmake-build-debug/blobber --hybrid-demo --seed 5`: the two new archers
shoot from their starting positions; approach one to exercise its attempt to back
away. The original four goblins keep their mixed-weapon behavior. Gameplay feel
and interactive review remain with the user; slice 09 is still unstarted.

## Front-approach routing and melee range correction

Reproduced the user's demo report with seed 5 and automatic Defend decisions:
healthy goblin #2 moved (4,9) → (4,10) → (4,11), selecting an ordinary adjacent
flank through a tied shortest path. No wounded escape was involved. Pursuit now
prefers the front tile from ahead, including lateral alignment on ties. In the
same headless replay #2 stays in front and eventually joins (3,10), attacking
there. A full, solid or unreachable front still permits ordinary adjacency.

Living melee targets outside the adjacent front tile now return OutOfRange;
hover and log say “Target is out of range for this attack.” Dead/removed/foreign
targets keep InvalidTarget. Rejection leaves the active action and readiness
unchanged. The initiative strip is explicitly unchanged at the user's request.

Checks for this correction:

- Debug and Release suites: **20/20 passed**; builds without warnings.
- Four-heading encounter regressions keep the approaching healthy actor in view
  across rounds and require repeated melee attacks from the shared front tile.
- Navigation tests cover tied routes, blocked/full/unreachable preferred goals,
  canceled results, and incremental fallback on an 80×80 fixture. Work remains
  capped at 4096 nodes and eight completed requests per update.
- HUD regression checks hover, rejection log, unchanged action/readiness, truly
  dead target rejection and a subsequent valid melee attack.
- Frame allocation guard now includes preferred-goal navigation: **zero
  engine-owned C++ allocations** during the guarded run.
- Focused AI/navigation ASan/UBSan passed (`detect_leaks=0`). Compile using the
  earlier sanitizer command with `tests/ai_tests.cpp` instead of hybrid tests;
  output `/tmp/blobber-routing-sanitized`.
- Release navigation stress: 256 actors; 25×25 map, 63 ticks, 3,750 scratch bytes,
  3 ms; 1024×1024 map, 130,306 ticks, 6,291,456 scratch bytes, 6,080 ms. These
  existing stress cases use ordinary goals; preferred fallback is separately
  covered by the bounded 80×80 test. Large-map latency remains a limitation.
- Hidden `--capture-hybrid /tmp/blobber-routing-captures` now runs the actual demo
  through automatic Defend rounds and asserts that its first two healthy goblins
  remain in view. It also rejects an Alden melee attack against a distant archer
  without spending his turn. `hybrid-demo-melee-out-of-range.png` was inspected.
  Prepared storage remains 160,526,125 bytes; no art or projection changes.
- Documentation links and whitespace checks pass. No OS input/focus automation.

Review with `cmake-build-debug/blobber --hybrid-demo --seed 5`. Let enemies close,
try Alden's melee against a distant archer, then play several rounds. The front
pursuers should join the front group instead of voluntarily walking onto a flank.
Interactive review remains with the user; slice 09 is unstarted.

## Three-cell melee front and diagonal visibility correction

Implemented the approved three-cell melee front: center, front-left and front-right
share melee reach in both directions, with paired front/back protection relative
to party heading. Clear diagonal contact starts initiative; walls and blocked
corners prevent contact. Facing stays fixed until the whole front band clears.
Ranged disadvantage and archer escape use the same adjacency; an escape must
actually leave that melee distance. Mixed-weapon policy otherwise remains intact.

Pursuit prefers the center front cell, then reachable diagonal front cells when
the center's four slots are full or unreachable, before falling back to a flank.
Navigation keeps its existing bounded BFS and allocation-free runtime storage.
The initiative strip's presentation and remaining-turn behavior are unchanged.

Shipped-art preflight found the two outer nearest-row slots completely clipped.
With the user's approval, the close row now compresses laterally enough to keep
its outer pose canvases visible. All neighboring slots retain distinct positions;
bodies, shadows, picking and feedback share the adjustment. Interpolated movement
blends it at row edges. World occupancy, depth/scale, FOV and camera stay unchanged.

Checks for this correction:

- Debug and Release: **20/20 passed**, no build warnings.
- Combat regressions cover all four headings and both diagonals, party/enemy melee,
  paired protection, whole-front facing retention, blocked corners and healthy
  ranged-only escape beyond diagonal melee reach.
- Navigation regressions cover all four headings with a full center, plus a fully
  occupied front band falling back to other adjacency. Existing bounded fallback,
  cancellation and mixed-weapon behavior checks pass.
- Generated rendering fixtures cover all 48 heading/cell/subslot combinations,
  six simultaneous nearest-row targets in every heading, wall occlusion and
  outer-slot retreat at five interpolation times. Fixtures own their content.
- Frame allocation guard: **zero engine-owned C++ allocations** in guarded work.
- Focused hybrid ASan/UBSan: **passed** (`detect_leaks=0`), executable
  `/tmp/blobber-band-sanitized`, log `/tmp/blobber-band-sanitizer.log`.
- Hidden `--capture-hybrid /tmp/blobber-band-captures` asserts six shipped-art
  targets and legal melee in all four headings, then resolves an outer diagonal
  attack. North/east `diagonal-melee-ready` captures were visually inspected.
- Hidden GPU lifecycle smoke and `--capture-hud /tmp/blobber-band-hud` completed.
  Combined source/prepared pixels remain **160,526,125 bytes** under 160 MiB;
  no new raster banks or GPU resources. Captures use the final stationary layout;
  the subsequent travel-edge smoothing is covered by the CPU rendering checks.
- No OS input or focus automation. Interactive feel remains unverified and is
  left to the user.

Review with `cmake-build-debug/blobber --hybrid-demo --seed 5`: Alden and Brunna can
melee exposed enemies in all three cells immediately ahead. Bow-only goblins try
to back out of that band; incoming melee enemies can fill diagonal front cells
instead of taking a flank when the center is full. Slice 08 and milestone B remain
Ready for review; slice 09 is unstarted.

## Acceptance and closure — 2026-10-26

The user said “yes I accept this slice so close it”, following positive review of
the broader melee front and moving combatants. Slice 08 and milestone B are
Accepted and closed, including all review corrections above. No additional
implementation or testing was performed for this documentation-only closure;
recorded automated checks and their technical limitations remain unchanged.

The user wants to discuss graphical work before the next slice. Slice 09 is now
eligible but remains Planned and unstarted. Do not plan or implement it until
the user resumes that work.
