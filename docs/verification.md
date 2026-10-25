# Verification

Verified locally on Linux/X11 with GCC 16.2.1, CMake 4.4.4, raylib 6.2,
and an NVIDIA RTX 3060 using an OpenGL 3.3 context.

## NPC sprite animation

Verified 2026-10-25. Slow looping animation is implemented; NPC placements,
facing, occupancy and damage behavior are unchanged.

- Debug and Release builds pass with no compiler warnings in the build logs.
  All **six** headless CTest suites pass, including `npc_animation`: about
  12.0 seconds Debug and 1.8 seconds Release on this machine.
- Generated fixtures exercise exact timing boundaries, remainder/wraparound,
  uint64 maximum timestamps, phase offsets, backward diagnostic seeks, static
  compatibility, empty worlds, missing clips, malformed JSON and image failures,
  frame limits, ground anchors and combined prepared-memory exhaustion.
- Renderer coverage includes stationary animation updates, held-frame reuse,
  attack-to-directional-idle fallback, snapshot presence changes, all six movement
  effects with animation changes inside transitions, and transition completion.
  Independent pillar ray intersections and strafe-reversal tests now alternate
  animation frames as well as camera phases.
- The allocation guard runs multiple idle/walk/attack cycles and simultaneous
  movement effects: **zero engine-owned C++ frame allocations**. This is not a
  claim about C allocators, raylib internals, graphics drivers or loading/export.
- The selected nine imagegen sheets yield twenty 384x536 RGBA runtime frames.
  The offline preparer successfully checks every visible source pixel against
  output canvas bounds. Runtime loading/preparation succeeds within the existing
  128 MiB combined source/prepared-pixel budget; decoded source frames use
  15.70 MiB. Original static images remain available.
- Release GPU resource/presentation smoke check passed on the NVIDIA RTX 3060,
  OpenGL 3.3 (driver 615.78.08). Initial sandboxed X11 access failed; the same
  check and captures succeeded with approved desktop access. GPU log:
  `/tmp/blobber-animation-gpu.log`.
- Exported **98 deterministic logical captures plus one window screenshot** to
  `/tmp/blobber-animation-captures`. Animation captures include all four views at
  0/300/600/700/900/950/1200/1500/1550 ms and strafe views at selected attack/walk
  frames. Inspected prepared attack poses and desktop front/back/left/right and
  strafe captures for grounding, transparency, weapon visibility and clipping.
- [Clip previews](npc_design.md#visual-previews), an [in-game excerpt](screenshots/npc-animation/demo.gif),
  and representative [front](screenshots/npc-animation/demo-front.png),
  [back](screenshots/npc-animation/demo-back.png),
  [left](screenshots/npc-animation/demo-left.png), and
  [right](screenshots/npc-animation/demo-right.png) captures are saved in the repo.
  Individual GIF clips use their configured timing; the in-game GIF is a 1.55 s
  excerpt that restarts all clocks at its loop boundary, unlike continuous play.

Asset limitations: imagegen needed corrective iterations for alternating legs and
weapon handedness. Side walking uses a wide-stride/passing pair with steady arms.
Small face/clothing detail variation remains. This is a coarse sprite animation
proof of concept, not rigged motion. The original four occupants overlap from
cardinal views, so rear attacks may be partly hidden behind foreground idles;
strafe views and the isolated clips expose those poses. No NPC movement or combat
logic is attached. Other platforms were not exercised.

Commands:

```sh
cmake --build --preset debug
cmake --build --preset release
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset debug
env -u DISPLAY -u WAYLAND_DISPLAY ctest --preset release
cmake --build --preset release --target blobber_gpu_smoke
cmake-build-release/blobber_gpu_smoke
cmake-build-release/blobber --capture /tmp/blobber-animation-captures
```

## NPC contact shadows

- All five CTest suites pass headlessly in Debug and Release (about 8.1 and 1.3
  seconds locally). Allocation coverage includes a fully occupied NPC cell and
  retains zero engine-owned C++ frame allocations.
- Generated transparent-body fixtures isolate shadows and check neutral darkening,
  graded transparency, floor-projected bounds, per-row depth clipping, full hiding
  behind walls, and separate grounding for each occupant.
- Inspected native OpenGL captures from all four sides and during partial pillar
  occlusion in `/tmp/blobber-four-goblins`. Shadows remain beneath feet and respect
  NPC layering. No generated sprite artwork was changed.

## NPC first slice

Verified on Linux/X11, GCC 16.2.1, raylib 6.2, nlohmann/json 3.12.0 and the
NVIDIA RTX 3060/OpenGL 3.3 desktop.

- Debug and Release configure/build successfully. All **five** CTest suites pass
  with `DISPLAY` and `WAYLAND_DISPLAY` removed: about 8.3 seconds Debug and
  1.4 seconds Release locally. Run `env -u DISPLAY -u WAYLAND_DISPLAY ctest
  --preset debug` (or `release`).
- NPC tests own small generated sprites, maps and isolated JSON fixtures. They
  cover four subcells, movement in every heading/direction, queued/instant
  collision, rejected placement/type/capacity errors, relative paths, missing or
  corrupt images, malformed/oversized JSON, anchors, pixel budgets, direction
  sectors and diagonal ties, inverse-depth scaling, feet on the floor, alpha
  overlap and depth ordering, viewport clipping, and strafe reversal/endpoints.
- Wall occlusion agrees with independent ray/block slab intersections across
  all headings, four subcells and all eight strafe phases. The fixture explicitly
  requires fully hidden, partially hidden and visible sprites.
- The allocation guard now includes multiple NPCs, first composition, all six
  movement effects, idle reuse, NPC collision and queued movement. It reports
  **zero engine-owned C++ frame allocations**. Loading, decoder/preparation work,
  capture export, and raylib/driver internals remain outside this guarantee.
- A separate clean build in `/tmp/blobber-json-fallback`, configured with
  `BLOBBER_USE_SYSTEM_JSON=OFF`, downloaded and hash-verified the pinned 3.12.0
  archive, built the NPC suite and passed it headlessly. Existing raylib source
  was reused for this dependency-specific check.
- Release `blobber_gpu_smoke` passed on the desktop; log:
  `/tmp/blobber-npc-gpu.log`. Release also launched from `/tmp`, resolved its
  executable-relative assets, and exported **59 logical 1920x1080 captures**
  plus the window screenshot under `/tmp/blobber-npc-final`.
- Inspected the four goblin views, near/far scale, foot alignment, strafe midpoint,
  full pillar hiding, and a partial pillar reveal. Representative captures:
  [front](screenshots/npc-front.png), [back](screenshots/npc-back.png),
  [left](screenshots/npc-left.png), [right](screenshots/npc-right.png), and
  [partial occlusion](screenshots/npc-occlusion.png).
- With an explicit empty NPC manifest, all **35 original logical captures** are
  pixel-identical to `/tmp/blobber-foundation-review`. The comparison excludes
  the actual window screenshot. Empty-level captures are in `/tmp/blobber-npc-empty`.

The goblin is deliberately stationary. PNG generation is approximate artwork,
not a rigged model; small details can differ between directions. The selected
back view was regenerated to preserve weapon/pouch handedness and show boot
heels. No animation or AI behavior is claimed. Windows, macOS and CLion itself
were not exercised; existing platform limits below still apply.

## Foundation refactor

The refactor separates application, world, movement, input, assets, rendering,
UI, and platform ownership. Routine checks are now four CTest executables with
independent generated materials and `tests/fixtures/regression_maze.txt`; changes
to production art or maps no longer alter their inputs.

- Both Debug and Release pass `core`, `render`, `resource`, and
  `frame_allocations` with `DISPLAY` and `WAYLAND_DISPLAY` cleared. The routine
  suite takes about 5.3 seconds in Debug and 0.8 seconds in Release locally.
- The allocation guard starts after frame/cache construction and includes the
  first composition, idle frames, all six moving transitions, repeat input,
  collisions, buffered actions, and instant mode. It checks output against direct
  composition, and requires **zero C++ allocations**. This includes aligned
  allocation overloads, but does not intercept C allocations or graphics drivers.
- Resource tests cover invalid/foreign/stale handles, reuse, exhaustion, once-only
  cleanup, invalid materials, missing/corrupt files, CRLF maps, embedded NUL,
  dimension limits, and disconnected floors.
- The explicit `blobber_gpu_smoke` test runs outside CTest with a real desktop
  context. It checks texture upload, render-target drawing/readback, resource
  release/reuse, invalid inputs, stale lookups, capacity exhaustion, and shutdown
  order. With display variables cleared, window initialization fails cleanly with
  an error and exit status 1.
- Compared all **35** logical 1920×1080 captures to the pre-refactor strafe revision:
  **pixel-identical**, including all static poses, six transition midpoints, and
  both full strafe sequences. Captures are local diagnostic output under
  `/tmp/blobber-foundation-review`; the comparison baseline was
  `/tmp/blobber-strafe-review`. An intermediate room strafe was also inspected.

The GPU lifecycle test and capture run passed on the NVIDIA/X11 desktop. Existing
art, projection, collision, controls, and transition behavior are preserved. The
following sections also record earlier prototype checks; their explicit platform
and screenshot caveats still apply.

## Build and automated checks

- Debug and Release configure and build successfully.
- CTest passes in both configurations without a display or OpenGL context.
- Movement tests exercise all four headings, forward/backward/strafe mapping,
  turn wrapping, wall collisions, and out-of-bounds cells.
- Input/controller tests cover press timing, held-key repeat, one pending fresh
  command, dropping held repeats during animation, clearing the pending command,
  blocked moves, and instant mode.
- Aspect-fit checks cover native 1080p, 720p, 4:3, ultrawide, and zero-sized windows.
- Image-transition tests cover exact endpoints and opaque, fully initialized
  pixels at five intermediate phases for forward/backward and turns.
- Strafe tests cover exact endpoints, left/right reversal, and translated-eye
  geometry at intermediate phases in all headings with walkable destinations.
  Fixtures include `(8,4)` facing west, a junction, and the second large room.
  Wall coverage and opaque pixels are checked against the independent oracle,
  verifying sideways translation and newly exposed geometry rather than a turn.
- Map tests validate symbols, rectangular shape, dimensions, perimeter closure,
  and connectivity. Production maps are validated at application startup.
- Rendering checks compare wall coverage against an independent ray/block slab
  intersection oracle at nine locations in all four headings. This oracle is
  test-only; the game compositor uses prepared 2D sprites. Exact subpixel edge
  ties are excluded to allow independent floating-point rounding. Interior
  pixels are sampled at every column and every third row in the oracle loops;
  framing checks use their exact boundary strips. Distant fog is excluded from
  material-color classification.
- Framing tests require left and right passages to remain readable when directly
  beside the player, at least 32 art-grid pixels of central foreground floor
  when facing an adjacent wall, and more than twice as much floor as ceiling in
  an open-room fixture. Opposite walls must continue occluding the side passages.

## Desktop rendering

Launched the application successfully at 1920×1080, 1280×720, 1024×768, and
2560×1080. Inspected captures of corridors, open rooms, pillars, near walls,
corners, junctions, and movement effects. The 1024×768 window has centered top
and bottom bars; the 2560×1080 window has centered side bars. All logical scene
captures remain 1920×1080.

The Release build was also launched with `/tmp` as its working directory and
loaded its executable-relative assets successfully. The interactive Debug build
was opened at 1280×720 and keyboard movement was exercised. Automated tests
provide the deterministic checks of collision and input state behavior.

Representative captures:

- [Open room and pillars](screenshots/great-hall.png)
- [Long corridor](screenshots/corridor.png)
- [Approaching a right passage at (3,16)](screenshots/passage-approach.png)
- [Beside the right passage at (3,15)](screenshots/passage-beside.png)
- [Facing the adjacent wall at (3,14)](screenshots/adjacent-wall.png)
- [Right strafe and return at (8,4), west](screenshots/strafe-right.gif)
- [4:3 letterboxing](screenshots/letterbox.png)
- [Ultrawide pillarboxing](screenshots/pillarbox.png)

The perspective revision was rebuilt and tested in Debug and Release, then
captured at the exact three reported problem poses. The room, corridor, and
passage/wall captures above use the updated 90° view, 0.4-tile setback, and raised
horizon. The aspect-ratio captures retain the earlier projection; the fixed
render-target and window-fitting paths were unchanged by this revision.

The strafe correction was also built and tested in Debug and Release. Captured
both A/D sequences at `(8,4)` facing west and inspected their start, intermediate,
and end views. The animated preview is assembled from those actual game captures.
The earlier static/aspect captures predate the world-anchored texture parity fix.

## Limits of verification

Windows, macOS, CLion itself, fractional desktop DPI scaling, and a live minimized
window were not directly exercised. The project supplies portable CMake targets
and presets, executable-relative asset paths, framebuffer-aware presentation,
and a zero-dimension/minimized-window guard. Movement effects are intentionally
stylized 2D effects for forward/backward and turns. Strafes use geometrically
projected, precomputed 2D views at eight lateral substeps per tile.
