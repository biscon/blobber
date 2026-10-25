# Slice 06 verification and user review

2026-10-26 — Goblin hurt/death animations accepted and closed, including
attack-impact timing and diagonal front-slot review corrections.
See [design](goblin_reactions_design.md) and
[roadmap record](dnd5e_demo_design.md#slice-06-goblin-hurt-and-death-animations).
No slice 07 work is included.

## Builds and CPU checks

```sh
cmake --build --preset debug -j4
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset debug --output-on-failure
cmake --build --preset release -j4
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset release --output-on-failure
```

Debug and Release builds passed without compiler warnings. All **18/18 suites**
passed in each configuration (approximately 28.1/4.0 seconds). The new
`npc_reactions` suite and expanded `npc` loader/preparer and `frame_allocations`
checks use generated, test-owned data. No shipped artwork is needed for CTest.
After strengthening the opaque replacement, canvas-registration and malformed-row
checks, the affected suites were rerun in both configurations.

Coverage includes:

- Optional legacy compatibility; four required reaction directions; illegal
  placement loops; clip canvas widths and prepared-memory exhaustion.
- Hurt/flash/death boundaries; repeated damage; attacks interrupted by hurt/death;
  damage winning equal timestamps; a newer attack interrupting hurt; canceled
  older attacks never resuming; zero damage; large time jumps and explicit seeks.
- Hidden death behind walls, immediate removal, dead targeting exclusion,
  stationary refresh and all six movement transitions sharing the same sample.
- Death unblocking immediately, a living actor reusing a corpse's subslot, opaque
  living pixels drawing over that corpse regardless of slot order, and removal
  preserving the replacement's occupancy.
- Pause, victory before animation completion, Continue without clock reset, corpse
  expiry in exploration, and fresh restart identities/presentation.
- Wider transparent canvases retaining body scale, pivot and ground (one-pixel
  tolerance only for horizontal raster rounding); explicit ground registration,
  uneven sheet rows, preserved alpha/pose height, empty frames and clipping errors.
- **Zero engine-owned C++ frame allocations** after preparation, including hurt,
  interrupted attack, collapse stages, final hold, expiry, composition and picking.
  This does not claim that raylib/driver internals or capture/export never allocate.

## Assets and reproduction

Eight selected transparent sheets contain 24 new poses: two hurt and four death
frames in each direction. All 20 original runtime PNGs are byte-identical to HEAD
at the start of this slice. The selected sheets and runtime frames are in
[the animation directory](../assets/npcs/goblin/animation/); exact initial and
correction prompts/reference paths are in
[reaction_prompts.json](../assets/npcs/goblin/animation/reaction_prompts.json).
The built-in imagegen tool generated the art; no CLI/API fallback was used.

The first left hurt candidate placed the dagger in the near left hand. A targeted
imagegen correction restored the far anatomical right hand. Death sheets had
unequal rows, requiring measured `split_y` rather than a midpoint cut. Those
choices are preserved in [sheets.json](../assets/npcs/goblin/animation/sheets.json).
One scale per sheet preserves the different silhouettes without individual pose
resizing. Body identity, equipment, source alpha and all four final corpse views
were visually inspected at source and runtime resolution.

```sh
cmake --build --preset debug --target blobber_prepare_npc_animation -j4
cmake-build-debug/blobber_prepare_npc_animation assets/npcs/goblin/animation/sheets.json
```

Repeated extraction reproduces all 44 PNGs and `frames.json` byte-for-byte.
Existing sheets retain their old extraction defaults. New death sheets use
768×536 canvases and 0.9-cell width; living frames remain 384×536/0.45 cells.

Shipped startup validation measured:

| Pixel storage | Bytes | MiB |
| --- | ---: | ---: |
| Decoded source frames | 49,397,760 | 47.11 |
| Prepared depth colors and shadow alpha | 69,065,872 | 65.87 |
| Combined | 118,463,632 | 112.98 |
| Enforced limit | 134,217,728 | 128.00 |

Metadata, temporary decoder storage and graphics-driver internals remain outside
this existing pixel-array budget. Source frames are released after preparation.

## Hidden GPU and visual checks

```sh
cmake --build --preset debug --target blobber_gpu_smoke -j4
./cmake-build-debug/blobber_gpu_smoke
./cmake-build-release/blobber --capture-reactions /tmp/blobber-slice06-review
./cmake-build-release/blobber --capture /tmp/blobber-slice06-full
```

The initial sandboxed capture could not access X11 display `:0`. The same command
succeeded with approved display-server access. GPU lifecycle checks passed in a
hidden window on Linux/X11, NVIDIA GeForce RTX 3060, OpenGL. Both capture runs
exited successfully and unloaded application-owned resources: **97 focused PNGs**
and **252 full-regression PNGs**. Full mode includes the existing UI and combat
cases; this is not a claim that every old image is pixel-identical.

Inspected images include white flash, normal hurt/recovery, intermediate collapse,
all four corpse directions, replacement overlap, transitions, and the final
kill/victory/Continue flow. Selected captures at 1920×1080 and 1280×720 were
inspected. The unchanged victory panel can cover part of the last corpse; it does
not pause or delay the reaction. Old attack views remain front-only as approved.

Retained examples:

- [Hurt, front](screenshots/slice06/reaction-hurt-front-100-720.png)
- [Corpse, front](screenshots/slice06/reaction-death-front-600-720.png)
- [Corpse, back](screenshots/slice06/reaction-death-back-600-720.png)
- [Corpse, left](screenshots/slice06/reaction-death-left-600-720.png)
- [Corpse, right](screenshots/slice06/reaction-death-right-600-720.png)
- [Living replacement over corpse](screenshots/slice06/reaction-corpse-replacement.png)
- [Victory during final corpse hold](screenshots/slice06/combat-victory-corpse.png)

No OS input, mouse/keyboard automation, window selection, focus change, desktop
switching or foreign-window capture was performed. Physical interactive review,
Windows/macOS, high-DPI hardware and production-scale performance remain unverified.
The generated animation deliberately uses a small number of discrete poses;
minor detail variation between poses remains visible at source resolution.

## User review

1. Run `./cmake-build-release/blobber --combat-demo --seed 5`.
2. Attack a reachable goblin. A nonlethal hit flashes white for 100 ms while a
   two-pose recoil/recovery plays for 300 ms, then returns to idle. Misses keep
   their existing feedback and do not trigger hurt.
3. Kill a goblin. It stops blocking/being targetable immediately, collapses for
   600 ms, holds its final corpse pose for 600 ms, then disappears. A rear goblin
   can still advance into the vacant front slot on its next turn.
4. Pause with the map or an overlay during a reaction; resume from the held stage.
   Finish the fight and Continue; the last death can finish without delaying the
   result or resetting its clock. Restart after defeat restores fresh actors.
5. Run the focused capture command above to inspect side/back reactions that the
   current frontal encounter does not normally expose. `reaction-*` names include
   clip, direction and elapsed milliseconds since the hit. `-baseline` is before
   damage; `-720` is a reduced-size export. Final hold begins at `death-*-600`;
   `death-*-1200` must be empty. `hurt-*-300` has returned to idle.

## Attack-impact review correction (2026-10-25)

The user reported enemy damage/MISS preceding the attack animation. The HUD now
starts those markers and the portrait hit highlight at the strike pose (frame 2,
950 ms into the shipped four-frame attack). Timing comes from prepared metadata.
Player attacks remain immediate with monster hurt/flash; HP, logs and combat
resolution still update immediately. Pending impacts use fixed storage and paused
session time, and preserve successive attacks on the same portrait.

Both builds and all 18 CTest suites passed again (`ctest --preset debug -j4` and
`ctest --preset release -j4`, 8.34 / 1.18 seconds). New checks cover guard/wind-up/
strike boundaries against NpcAnimator, hit/MISS, highlight and splash lifetime,
multiple pending impacts, pause, skipped/expired intervals, movement/Continue
cancellation, simultaneous player feedback, shortened clips and missing/single
frame fallback. The allocation guard now supplies prepared attack timings and
still reports zero engine-owned C++ allocations.

`./cmake-build-release/blobber --capture-hud /tmp/blobber-impact-review` passed
with authorized display-server access, exporting 45 images in a hidden window.
New `hud-impact-{hit,miss}-{guard,windup,strike}` captures use real seeded enemy
outcomes with an isolated production HUD controller. Samples are at 0, 949 and
950 ms; native and 1280×720 exports are available. Inspected both hit and MISS
wind-up/strike comparisons, guard and player-hit feedback at reduced size. The
recipient splash is absent during wind-up and present with the forward strike;
player damage still coincides with the monster's white hurt flash. No desktop
input/control was used. Interactive review remains for the user.

Retained timing examples:

- [Wind-up, no damage splash](screenshots/slice06/hud-impact-hit-windup-1280.png)
- [Strike with damage](screenshots/slice06/hud-impact-hit-strike-1280.png)
- [Strike with MISS](screenshots/slice06/hud-impact-miss-strike-1280.png)

Review with `--combat-demo --seed 5`: let enemies act, and check portrait numbers/
MISS begin as the dagger strikes. Pause during wind-up and resume; feedback should
wait for the remaining wind-up. This correction does not accept slice 06 or start
slice 07.

## Front-slot review correction (2026-10-26)

The screenshot exposed a rear goblin skipped forever behind its own front ally,
although the other front slot was empty. Combat now selects a usable front slot
separately from current target exposure: prefer straight ahead, otherwise use the
opposite vacancy. On its own turn the goblin performs a checked same-cell move and
attacks once. Full front rows remain blocked. Initiative preview uses a local
four-bit occupancy mask to predict claims in order without mutating actors.

Generated tests cover both lanes from all four approaches, diagonal fallback,
straight preference, full rows, two rear actors competing for one gap, occupied
destinations, preserved identity/cell, one attack per round, attack cue timing,
pauses and pure preview. The HUD regression also checks that a later rear enemy
is omitted when an earlier enemy claims the opening, and included when that
opening remains available. The allocation guard executes diagonal advancement.

Debug and Release builds passed. Final `ctest --preset debug -j4` and
`ctest --preset release -j4`: **18/18** each (8.64 / 1.33 seconds), including zero
engine-owned C++ frame allocations. An initial HUD regression exposed competing
preview entries; local occupancy reservations fixed it before the final runs.
`git diff --check` passed.

`./cmake-build-release/blobber --capture-hud /tmp/blobber-diagonal-review` passed
with authorized display access: **51 PNGs**, including native, 1280×720 and 4:3
before/after diagonal advancement. Diagnostic setup removes the opposite column;
production turn execution moves the remaining protected rear goblin and attacks.
Inspected final reduced images confirm the hidden rear goblin appears in the
vacant front-left slot, with the front-right ally unmoved:

- [Before: rear goblin hidden behind its ally](screenshots/slice06/hud-diagonal-before-1280.png)
- [After: rear goblin fills the front-left opening](screenshots/slice06/hud-diagonal-after-1280.png)

No OS input/window control was used. Agent-run interactive desktop testing was
not performed. This is a same-cell combat correction, not the moving-AI slice.
The user subsequently accepted slice 06; slice 07 remains Planned.

## Acceptance — 2026-10-26

The user said: “I accept the slice, finish it in the document”. Slice 06 is
accepted in full, including the attack-impact feedback and diagonal front-slot
corrections. No additional conditions were requested. Historical verification
results and unexecuted platform/hardware checks remain as recorded. This closure
updates documentation only; no code, assets or slice 07 work is included.

## Next-agent handoff

Slice 06 is accepted and closed. Slice 07 is next eligible, but planning and
implementation have not started. In a new planning task, inspect the code and
prepare its plan for user review. Preserve the existing
session/actor authority, immediate dead occupancy release, latest-cue ordering,
100 ms flash overlay, optional legacy behavior and fixed memory limits. Hurt/death
may never become placement loops or use directional idle fallback. Rebuild
animator/pipeline/controller with a restarted session, and sample after mutations
before rendering. The underlying session clock advances through victory and
Continue but freezes for focus/map/modal pauses. No loot or AI lifetime is tied
to the corpse. Preserve impact timing and separate front-slot turn eligibility
from current target exposure when designing moving AI.
