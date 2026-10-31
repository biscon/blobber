# Dungeon revision verification

2026-10-31. Implemented and ready for interactive review. This follow-up does not
change the accepted status of the older renderer/campaign roadmaps.

## Delivered behavior

- World-density door-frame UVs, generated rectangular timber/iron door art and a
  matching keyed variant. Ordinary doors toggle; keys and first-opening progress
  persist. Occupied/moving-through thresholds reject closing; gates stay automatic.
- Static eight-step stairwells with lined shafts, exterior shells, picking IDs and
  dark ends. Stairs block entry and use the designated adjacent landing. The visible
  descending flight is shallow enough to read over the fixed camera's landing lip;
  beveled noses distinguish its tread edges.
- One default map-selected asset pack with generated crest, transparent vine and
  overcast panorama. Prepared bindings and UV scale participate in publication
  identity, including changes that leave texture bytes unchanged.
- One loading worker with a bounded single task/result, exception/cancellation
  propagation, background reads and CPU scene/session preparation. GPU/font work
  and one-image decoding stay on the window thread. The 4ms soft preparation budget
  restarts after presentation so vsync does not force every small operation to wait
  another frame. Startup/loading continue presenting and pumping events.
- Whole-presentation 250ms fade out, published black frame, destination preparation
  and transactional commit, then 250ms fade in for stairs, recall and waypoints.
  Simulation clocks freeze; recall charges once on commit. Failed staging retains
  the source. Input requires release after transitions.

## Checks and outcomes

Used the existing **Release** configuration, no more than two build workers and
one test process, with `nice -n 15` and `ionice -c 3`. No parallel intensive jobs,
capacity stress, seed sweep or benchmark matrix was run.

The affected windowless checks passed: `fixtures_automation`, `fixtures`,
`generation`, `generation_automation`, `generation_cli`, `guardians`, `campaign`,
`campaign_save`, `campaign_save_cli`, `dungeon_refresh`, `world_cpu`,
`world_visibility`, `world_visual_cli`, `world_integration`, `world_environment`.
Full campaign persistence coverage was run at integration (final passing run 3.9s).
Initial failures were old fixture assumptions: permanently open doors, stepping
onto stairs, a hub forward step now blocked by stairs, and mutating a template no
longer used by the new generator version. Updated fixtures preserve each intended
invariant and exercise closing/reopening and explicit landing-facing travel.

The new CPU check covers both door directions and animation barriers, stale
requests, occupied thresholds, completion/open-state separation, snapshot roundtrip,
all stair orientations/directions, blocked entry, depth/picking, frame mapping modes,
prepared allocation guards, pack validation/identity, bounded files, worker errors,
cancellation during work and blackout ordering. A collected-key checkpoint guards
against accidentally recording keys as open doors.

The generated offline publication/build fixture passed with Ninja using
`tests/world_build_tests.py ... --single-config`: clean/no-op builds, source/effect
changes, missing output, added/removed inputs, touch dependencies, pack-manifest
changes and failed-bake retention. The baker's separate CPU asset target avoids
rebaking art after scene-geometry changes.

`blobber_world_integration_gpu_tests` passed in an application-owned hidden window
on NVIDIA GeForce RTX 3060, OpenGL 3.3, raylib 6.2. It checks doors in four directions,
opening motion, effects/depth, stairs in four directions at both quality settings,
CPU/GPU IDs, warm allocation/upload guards, preparation failure rollback and cleanup.
Final CPU/GPU ID comparisons reported zero edge/interior differences. Making open
leaves clickable exposed coplanar leaf/wall depth disagreement; moving the hinge
through the leaf thickness gives clearance and fixes that regression.

Production art was baked and loaded through the real renderer. Publication:
126 textures, 205,836,276 mip bytes, 36,108,944 alpha bytes, within existing budgets.
The game, headless runner and affected verification executables were rebuilt.

## Review captures

Application-owned 1920×1080 exports under `screenshots/dungeon-refresh/`:

- [Door and frame](screenshots/dungeon-refresh/door-closed.png),
  [open](screenshots/dungeon-refresh/door-open.png),
  [closing](screenshots/dungeon-refresh/door-closing.png).
- [Ascending stairs](screenshots/dungeon-refresh/campaign-stairs-up.png),
  [descending stairs](screenshots/dungeon-refresh/campaign-stairs-down.png),
  [descending world target](screenshots/dungeon-refresh/stairs-down-world.png).
- [Crest](screenshots/dungeon-refresh/hub-crest.png),
  [vine](screenshots/dungeon-refresh/hub-vine.png),
  [sky](screenshots/dungeon-refresh/hub-sky.png).
- [Half fade](screenshots/dungeon-refresh/travel-fade-half.png),
  [black](screenshots/dungeon-refresh/travel-black.png).

The black capture has RGB extrema exactly zero across the whole image. The half
fade's mean channels are approximately 0.496–0.497 of the unfaded source, including
HUD and frame. Source art and the final built-in ImageGen prompts are retained in
`assets/packs/dungeon/source/` and `assets/packs/dungeon/prompts.json`.

## Limits and compatibility

Older campaign saves reject unchanged: schema/simulation 3/3, generator/templates
4/4; publication format 2, appearance 4, visibility v4, terrain rules v2 and shared
300ms door-motion scheduling v2. No save migration or second biome was added.

The final normal production load recorded 166 presentations and a longest gap of
37.7ms between presentation starts. This includes real UI/font/art loading and
world texture uploads in the hidden application context. An earlier run presented
480 times; resetting the work budget after vsync reduced that overhead. This is a
responsiveness observation for this workload, not a portable frame-time guarantee.
Individual decoder/driver/I/O operations remain indivisible. No desktop ANR dialog
was deliberately provoked, and interactive desktop input/focus review was not
performed. All graphics checks and captures used hidden application-owned windows;
no OS mouse/keyboard or window-control automation was used.
