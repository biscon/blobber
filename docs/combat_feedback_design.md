# Painted combat feedback

Status: **Accepted and implemented (2026-11-05)**.
User approved [A — Painted splash](screenshots/combat-feedback-concepts/README.md)
and requested implementation. This extends the SE02 review work.

Replace the procedural starburst behind existing world and ordinary party weapon
feedback with authored crimson damage, charcoal outcome and slate immunity splashes.
Preserve SE02 portrait spell/healing/buff animation and its small number-only
readouts. Render labels separately in warm ivory, with no text baked into art.
Zero damage and saves/misses never use a blood backdrop. Existing label content,
recipient replacement and anchor/clipping rules remain authoritative.

`UiArt` owns one startup-loaded 768x128 RGBA atlas (three 256x128 cells), through
the Presenter's GPU registry. Only this runtime file is packaged. ImageGen source
and prompts stay in assets; deterministic offline packing provides transparent
gutters and consistent registration. Missing/invalid art rejects preparation.
Additional texture storage is 393,216 bytes, no new frame allocations or pools.
The packer removes near-invisible alpha residue below 16 before finding content
bounds, then fits the art within 224x112. Rendering samples that inner region.

`ui/combat_feedback.hpp` provides pure session-time sampling: full visibility on
the resolution frame, 400 ms readable hold, 240 ms fade, expired at 640 ms. Only
actual positive-damage numeric readouts receive a damped 100 ms tremor bounded to
two presentation pixels; background and other HUD elements remain steady. Repeated
timestamps freeze exactly. No new gameplay state, RNG, input, save data or camera
shake. In particular, the shared 1,000 ms combat hold/feedback retention stays
unchanged; this is presentation sampling only.

The draw path consumes the fixed feedback records, samples alpha/offsets, draws
the prepared backdrop and labels, and allocates nothing. Long outcome labels use
the available backdrop width, with smaller font if required. Keep the entire
backdrop inside the viewport or portrait card. Prepared splashes use a 44x26
art-unit footprint and the existing serif display font; portrait-only spell
readouts retain their 36x24 region and small font. Long special outcomes fall
back to small lettering and split at their space when needed.

Verification: focused CPU feedback classification, instant/hold/fade/expiry,
pause/repeat sampling, zero-damage/text-only shake suppression and allocation
checks; existing command/HUD/spell tests. Hidden application captures exercise
ordinary damage, saved/missed outcomes and party feedback using production art.
Inspect actual size and frame sequence; test OpenGL/resource lifecycle without
desktop input or focus changes. One low-priority job, max two build workers.
Promote a small selected gallery and clean the owned run. The user accepted this implementation with SE02; no automatic SE03 implementation.

## Verification and handoff — 2026-11-05

The normal application and GPU integration executable were rebuilt in the existing
Debug configuration, at low priority with at most two build workers. Focused CPU
checks passed (4/4, 0.30 seconds):

```sh
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug --output-on-failure -j1 \
  -R '^(combat|commands|hud|spell_effects)$'
```

These cover the changed presentation sampling and its combat/command callers:
immediate visibility, hold/fade boundaries, expiry, future/inactive records,
deterministic paused sampling, bounded damage-only motion and allocation freedom.
Gameplay timing remains covered by existing combat tests.

Three final hidden production runs passed using `--feedback-review` with
`poison-spray`, `scorching-ray` and `portrait-weapon`; see the
[review gallery and reproduction command](screenshots/combat-feedback/README.md).
Each checks OpenGL errors and repeats warmed presentation under the engine's
allocation watcher (zero allocations). This does not claim graphics drivers never
allocate. The new weapon fixture uses real End Turn commands and waits for a real
hit; the initial defending fixture produced only misses and was corrected before
the final captures. No fabricated damage outcomes are used.

Inspected native captures and phase sheets verify rounded silhouettes, readable
ivory labels, correct world/portrait placement, steady no-damage outcomes, visible
fade and removal. Scorching Ray hits once for 3 with two misses; the goblin hits
Alden for 9 after two misses. Poison saves and remains neutral. The final offline
packer reproduces `backdrops.png` byte-for-byte from the retained ImageGen source.

The new atlas adds 393,216 GPU bytes; dungeon application GPU is 281,327,132 bytes
(world 157,625,808), and the goblin scene application GPU is 278,798,944 bytes
(world 155,097,620). The world publication remains 409 textures with 695,273,524
GPU mip bytes and 67,648,613 CPU alpha bytes. These are resource-accounting figures.
SE02 plus this follow-up adds 23.71 MiB GPU storage, within the original 24 MiB
allowance. No runtime capacity ceiling was raised.

Immunity has CPU classification coverage but no separate production capture.
User acceptance is recorded; interactive desktop and cross-device verification were not performed. No broad
Release/headless/sanitizer matrix or benchmark was run for this visual change.
SE02 is Accepted; SE03 is next eligible and remains Planned.

Owned run `output/agent-runs/20261105T211147Z-c45c722dee9b4c8b8395031473fe7da8`
was cleaned after closing runners and promoting four review files. Cleanup was
previewed and applied: 589 disposable files / 554,966,090 bytes removed. No failed
iterations or temporary diagnostics retained. Source artwork/prompt, two concept
boards and build caches remain. Local Markdown links and `git diff --check` pass.

## Enemy melee lunge

Status: **Implemented; Ready for review (2026-11-06)**. The previously
accepted painted feedback remains accepted; this addition awaits its own review.

The user requests enemies to lunge forward and snap back during melee attacks.
`NpcAnimator` adds a bounded 0.14-cell offset along the actor's facing to its
existing presentation offsets only while a live, triggered melee attack clip wins
pose selection. Approach over the last 140ms before `npcAttackImpact`, reaching
maximum extension exactly on the existing impact frame; return over at most 120ms,
clamped to the remaining authored clip duration. Short clips scale these windows.
A squared approach/return gives a forceful strike and quick settling. No motion for
ranged attacks, casting, idle placement loops or absent/zero-impact attack clips.

Reuse the existing attack clock including accumulated hurt suspensions. Hurt,
prone, cancellation, death and removal suppress the lunge; resumption samples the
remaining attack timeline. Paused timestamps freeze exactly and large jumps require
no catch-up. No new mutable state, resources, allocations, gameplay displacement,
collision/range changes, damage timing, RNG calls or save data. The visibility
pipeline already consumes these offsets for billboards, depth, attachments, shadows,
badges and picking, keeping the entire creature coherent. The displacement stays
inside a subcell's quarter-cell margin on cardinal attacks.

Review correction (2026-11-06): the outer diagonal enemies crossed the renderer's
45-degree directional-art boundary during the lunge, selecting left/right artwork.
For a live triggered melee attack, visibility derives its facing-sector position
from `actorPosition(actor, actor.attackTimeMs)` for the entire selected attack clip.
Combat already holds actor facing and the party view through the attack. Keep the
animated foot position for geometry, depth, shadows, attachments and picking; only
directional texture selection uses the attack-start anchor. Other poses and travel
retain their existing direction selection. This needs no additional state or
allocation. Verify both diagonal sides in all four party headings across approach,
impact, recoil and clip completion.

Correction verification: rebuilt the Debug application and focused test targets.
`ctest --test-dir cmake-build-debug --output-on-failure -j1 -R '^(animation|world_visibility)$'`
selected the two affected suites: animation passed (0.01s); after correcting an
invalid optional-art test fixture, world visibility passed (1.22s). The regression
checks center/left/right enemies in all four headings at six attack timestamps,
front texture selection, peak displacement, return, unchanged authoritative state
and zero frame allocations. Existing walking and facing-sector checks also pass.
No new captures or interactive desktop verification; prior captures predate this
correction. Disposable build/test output was cleaned without retained exceptions.

Focused verification: animation profile/directions/interruptions, snapshots without
authoritative movement, allocation guards, existing impact and world-visibility
checks. Inspect deterministic hidden application captures with several production
creature sizes and attack timings, using real enemy attack timelines and ordinary
party commands. Keep one low-priority job at a time and clean the owned run. No OS
input or focus automation; interactive feel remains for user review.


Verification (2026-11-06): Debug application, animation/impact/world-visibility and
GPU review targets built at low CPU/I/O priority, at most two build workers, without
compiler warnings. Focused checks passed 3/3 in 1.30s:

```sh
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug --output-on-failure -j1 \
  -R '^(animation|impact|world_visibility)$'
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets RUN/melee --enemy-melee-review
```

Four production-art/rules capture cases pass: goblin, giant rat, bugbear and zombie.
They use seed 5, ordinary party Defend commands and actual enemy melee timelines.
Goblin/bugbear/zombie contact is at 950ms and rat contact at 450ms. Each sequence
captures -160 through +240ms around contact, in 20ms steps, checks one authoritative
impact, stable grid/subcell positions, continued picking, zero warmed engine-owned
presentation allocations and no GL errors. These deterministic attacks all miss;
the lunge is independent of hit outcome. Peak and return frames were inspected.
No desktop focus/input control was used. Interactive feel/cross-device rendering
remain unverified; no broader graphics/configuration/benchmark matrix was repeated.

The [motion comparison and native still](screenshots/combat-feedback/README.md#enemy-melee-lunge)
add two files to the accepted four-file feedback gallery, preserving its acceptance.
No new textures, saved fields or per-actor storage were added. All disposable output
in run `20261105T225947Z-91c533f59cf84d5bb15aac26844a8e72` was cleaned after promoting
these and the targeting evidence. No diagnostics or runners remain open.
