# World-window screenshake

Status: implemented and mechanically verified; awaiting user timing/feel review (2026-11-05).

## Behavior and ownership

The prepared 500×282 world image receives translation and temporary center zoom;
the fixed 1920×1080 presentation, world camera and discrete movement do not change.
One code tuning value defines little (1 px / 140 ms), medium (2.5 px / 230 ms)
and big (5 px / 360 ms) presets, 14 Hz noise, an overall multiplier (zero disables)
and a 7 px per-axis summed limit. Units are logical world-image pixels.

Burning Hands, Thunderwave and Shatter produce one big shake per committed cast,
even for saves or immunity. Existing rules reject empty areas before committing a
cast; rejected requests never shake. The cosmetic classifier also supports a
committed cast without recipient events, without changing those admission rules. Their existing Cast event occurs at the
same session timestamp as immediate resolution; individual recipients never trigger
big shakes. If these spells later acquire travel time, move this trigger to their
single landing event. Acid Splash, missiles, rays, support and ongoing damage do
not qualify. Resolved damaging criticals on either side (including spells) produce
medium shake. Noncritical enemy melee weapon damage to a party member produces
little shake. Misses, cancellation, missing targets, mirror images and zero damage
are excluded. Critical classification takes precedence over incoming melee.

CommandController owns a graphics-independent ScreenShakePlayer beside its combat
feedback, recording the existing consume-once outcome stream. HudSnapshot publishes
a small WorldViewTransform sampled at absolute paused session time; rendering never
triggers impulses. Sixteen contiguous records retain preset, timestamp and cosmetic
seed, without actor pointers. Expired records are reclaimed before insertion;
overflow drops only the new cosmetic request. Session/world replacement clears
transients. No save, simulation, protocol or asset format changes.

Adapt ../engine's hashed gradient noise and impact envelope locally: smooth attack
up to 10 ms, then quadratic release. Noise uses a private deterministic sequence,
never gameplay RNG. Sample absolute time so frame subdivision cannot change output;
skip already-expired late events. No frame allocation, logging or external dependency.

## Composition and input

Presenter enlarges/translates the existing world target inside a fixed viewport
scissor. Zoom uses the summed, capped amplitude envelope to cover both axes smoothly,
including noise extrema; at rest it is exactly one. No extra GPU target, world
re-render, upload or picking regeneration is needed for shake alone.

World-attached outlines, enemy statuses/health, spell markers and enemy feedback use
the same transform. Their depth clipping rectangles transform and intersect the
fixed viewport. Party feedback, HUD frame, initiative, tooltips, prompts, menus and
other controls remain stationary. Existing world markers have no separate hit targets.
Presenter retains the last displayed transform; application picking inverse-maps
world pointer coordinates before querying creature, loot, ground, fixture or container
IDs. Physical HUD coordinates stay unchanged; moving images also refresh pointer hover.

## Verification strategy

Small owned CPU fixtures cover event classification, one shake per area cast,
critical precedence, exclusions, overlap/capacity, deterministic time, pause, expiry,
late events, resets, disabled/invalid tuning, forward/inverse mapping, clipping,
covered edges and allocation-free engine frame processing. Focused command/HUD/
targeting/spell/impact suites protect callers. Hidden OpenGL verification covers
both quality settings, fixed HUD pixels, transformed scene/overlays, cache reuse
and resource release. Selected application-owned captures support user review;
no OS input or focus control. Initial strengths and durations await user trial.

Use one Debug build, at most two workers, one costly job at a time and low CPU/I/O
priority. Record results here; disposable output belongs to the task's agent run,
with at most six selected review images promoted before preview/apply cleanup.

## Delivered interfaces and verification

`src/ui/screen_shake.hpp` contains the editable `screenShakeTuning` value. Presets
are ordered Little / Medium / Big; changing pixels, duration, frequency, multiplier
or limit requires rebuilding. The player exposes consume-once `record`, read-only
`sample`, reset and bounded activity diagnostics. `WorldViewTransform` maps logical
art coordinates and computes native viewport-intersected depth clips. HudSnapshot
publishes it; Presenter retains the displayed value separately from newly sampled
state. Gameplay compares consecutive input transforms, including the last return
to identity, so an unmoved pointer cannot retain stale hover when shaking stops.
World markers remain decorative; their underlying creature targets use inverse
mapping. `drawWorldViewport` and the scoped world-overlay transform share composition
with the hidden GPU checks. No extra GPU resources are retained by this feature.

Final verification (Debug; all listed final checks pass):

- Built `blobber`, `blobber_screen_shake_tests`, affected HUD/command/targeting/button/
  spell/level-spell/impact targets, and `blobber_world_integration_gpu_tests` with
  `nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target ... -j 2`.
  Existing build dependencies regenerated prepared UI/world output as necessary;
  no source artwork or publication format changed.
- `ctest --test-dir cmake-build-debug -R
  '^(screen_shake|hud|commands|targeting|buttons|spells|level_spells|impact)$'
  --output-on-failure -j 1`: all eight suites pass in 0.25 s. The new suite covers
  every spell's classification, critical precedence/exclusions, real multi-target
  area casts, rejected empty requests, disabled/invalid tuning, time subdivision,
  pause, expiry, world reset, full capacity/reuse, transformed coordinates and
  edge coverage. Engine C++ allocation guards pass for shake and HUD refresh.
- After the final hover-settling fix, rebuilt the application and shake test only;
  `ctest --test-dir cmake-build-debug -R '^screen_shake$' --output-on-failure -j 1`
  passes, including stationary-pointer hover refresh and selection preservation.
- Hidden `blobber_world_integration_gpu_tests`: passes on NVIDIA RTX 3060 / OpenGL
  3.3. Generated fixtures cover both supersampling settings, viewport edge coverage,
  transformed depth clipping/enemy feedback, unchanged exterior HUD/party feedback,
  pixel-identical restoration, cached world/picking reuse, no uploads or engine C++
  frame allocations, GL errors and GPU resource lifetime. Graphics-driver allocation
  behavior is outside that assertion. Initial sandbox execution could not open X11;
  authorized hidden execution succeeded without OS input or focus control.
- `blobber_world_integration_gpu_tests cmake-build-debug/assets RUN/captures
  --screen-shake-review`: passes. Inspected rest and big-shake framing; compared all
  six captures programmatically. Every changed pixel is inside the world viewport;
  expiry is identical to rest. [Selected evidence](screenshots/screenshake/README.md)
  is an isolated preset preview, not a recorded end-to-end combat animation.
- `git diff --check`: passes. No benchmark, stress test, sanitizer or second build
  configuration was needed. Earlier fixture failures were corrected to respect
  prepared spell access and to keep the party outside an intended empty Shatter
  area. The GPU diagnostic readback was corrected to retain noncopyable image RAII.

Interactive gameplay and subjective strength/timing acceptance remain user-owned.
The replay still-capture tool constructs a fresh HUD at each selected boundary, as
before, and does not reconstruct historical cosmetic impulses. Saves contain no
shake state. Empty area cast admission remains unchanged; no spell rules were altered.

Six selected images were promoted from agent run
`20261105T170121Z-77e64f0e4bfb49faac8e2414abb81f73`. Disposable logs/captures are cleaned
through preview/apply after runner shutdown; build caches and prepared assets remain.
No unresolved diagnostic exception is retained.
