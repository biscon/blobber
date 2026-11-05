# Procedural melee swipe

Status: implemented and mechanically verified; awaiting user visual review (2026-11-05).

Outgoing party melee weapons receive a screen-space ivory Bézier ribbon. Existing
200 ms authoritative impact, damage, reaction, textured impact and damage/MISS text
remain unchanged. The ribbon begins at impact minus 100 ms and lasts 185 ms.
Reveal occupies 0–70 ms, peak 70–130 ms, and retreat/thinning/fade 130–185 ms.
Twenty segments sample the visible interval; both ends taper to points. Defaults
are RGB 245/240/220, 135% body width span, 8.5% body height full thickness, 35-degree
diagonal and 30% span control offset, with feathered edge opacity. The user made
the screenshot the visual authority after rejecting the first numerical defaults.

Prepared per-facing body-only bounds accompany torso attachment metadata, excluding
weapons and transparent margins. Bounds use normalized registered idle-canvas coordinates. The billboard retains
the idle canvas width separately, so wider attack art cannot inflate the stroke.
The prepared appearance revision changes, but cosmetic metadata does not change
the legacy NPC definition fingerprint or save compatibility. Existing omitted
bounds default to the full canvas for external/test definitions.

A CPU melee-swipe player belongs to each EffectsPipeline. Four fixed records keyed
by world/attack identity capture projected torso and body dimensions on acceptance,
before recoil, with no retained actor pointers. One record per character follows
the existing one-pending-attack ownership. Cancellation/world changes clear records;
invalid initial geometry suppresses cosmetics without retrying against recoil art.
Accepted strikes alternate diagonal, with identity-derived ±10-degree variation
independent of gameplay RNG. Misses offset 105% of body width to the chosen side.
The renderer never emits combat events. Pause uses the existing session clock.

Fixed ribbon arrays in EffectFrame are drawn with ordinary alpha blending after
world resolve into the 500×282 output, before HUD presentation. The target clips
geometry; picking is untouched. Active animation and expiry invalidate the cached
image. No new textures, frame-path heap allocation or general effect scheduler.

## Verification strategy

Owned CPU fixtures cover geometry, phase boundaries, misses, lethal recoil,
cancellation, repeated snapshots, skipped time, pause, world reset and allocation.
Metadata round trips validate bounds and fingerprint compatibility. Existing impact
and combat tests protect exactly-once damage and handoff timing. Hidden OpenGL checks
cover supersampling modes, clipping and resource lifetime; application-owned captures
provide review evidence without desktop input. Use focused tests, two build workers
at most and one costly job at a time. Record results here and retain at most six
selected captures; interactive/aesthetic acceptance remains user-owned.


## Delivered implementation and verification

`render/melee_swipe.*` owns pure ribbon sampling and four retained attack records;
`EffectsPipeline` publishes fixed ribbon arrays beside existing world effect packets.
`WorldRenderer` draws their consistently wound triangles after resolve into its existing
500×282 output. No additional GPU resource or combat event is introduced. All tuning
is in `meleeSwipeTuning`; cosmetic variation uses attack serials, never DiceRng.
Prepared appearance revision is now 5; production assets were rebuilt through
`blobber_bake_world_assets`. NPC body-bound metadata is validated at load, round-tripped
through publication, and excluded from the legacy definition fingerprint.

Checks run (Debug, two build workers maximum, one test process; all final checks pass):

- `cmake --build cmake-build-debug --target blobber blobber_melee_swipe_tests
  blobber_npc_metadata_tests blobber_world_integration_tests blobber_world_integration_gpu_tests -j 2`
  plus the focused impact/combat targets and production asset bake: pass.
- `ctest --test-dir cmake-build-debug -R '^(melee_swipe|npc_metadata|impact|combat|world_integration)$'
  --output-on-failure -j 1`: impact, combat, metadata and world integration passed.
  The first swipe fixture lacked a valid billboard depth; after correcting it and
  extending lifecycle coverage, `blobber_melee_swipe_tests` passes directly.
- Swipe CPU cases include reveal/peak/retreat boundaries, pointed ends, maximum
  thickness, mirror shape, stable contact through removal/recoil, misses, cancellation,
  pause, expiry, large time jumps, late observation suppression, teardown, four-record
  capacity, weapon/padding canvas independence, and excluded attack types. Spawn,
  animation and full-capacity processing report zero engine C++ allocations.
- NPC metadata checks cover normalized-bound validation and unchanged legacy hashes;
  a generated publication fixture verifies body-bound round trip. Existing impact
  and combat checks retain authoritative ordering, exactly-once consequences and
  turn handoff behavior.
- `blobber_world_integration_gpu_tests`: pass on NVIDIA RTX 3060 / OpenGL 3.3.
  New assertions check opaque ivory pixels, mirrored winding, expiry, both
  supersampling modes, art-target dimensions, unchanged picking and zero engine C++
  frame allocations. Existing resource/parity/rollback checks also pass. This does
  not assert that graphics drivers never allocate.
- `blobber_world_integration_gpu_tests cmake-build-debug/assets RUN/captures
  --melee-swipe-review`: pass. Real hit and miss commands each produce exactly one
  authoritative impact at acceptance +200 ms; GL error and GPU release checks pass.
  The initial miss fixture used an out-of-range armor class; correcting it to the
  catalog's valid maximum of 30 completed the check without gameplay changes.
- `git diff --check`: pass.

All lengthy commands used `nice -n 15 ionice -c 3`; internal library threads were
bounded. The first sandboxed GPU attempt could not open the display; authorized
hidden execution succeeded. No desktop input or focus control was used. Production
visual inspection covers the goblin; seven creature types have authored body bounds,
with their registered idle appearances inspected during preparation. Interactive
review and final aesthetic acceptance remain pending. No broad Release/sanitizer
matrix or benchmark was run for this rendering-only change.

[Six selected captures](screenshots/melee-swipe/README.md) show reveal, peak, contact,
retreat, expiry and miss. The existing white hit flash reduces ivory contrast at
contact and is deliberately retained. Late first observation after contact suppresses
the swipe rather than inventing an acceptance-time anchor. New renderer/view ownership
clears transient records; saves contain no swipe state.

Disposable captures, inspection sheets and logs were owned by run
`20261105T160455Z-e04eed09cd37434aa514977b79e8f64c`; after evidence selection its cleanup
was previewed and applied. Build caches and prepared assets are retained. No unresolved
diagnostic exception remains.

## Visual correction — 2026-11-05 (implemented; awaiting visual review)

The user rejected the thin, flat first version and made the supplied screenshot
supersede the earlier numerical size instructions. Increase the sweep span to
135% of prepared body width, full thickness to 8.5% of body height and curvature
to 30% of span. Keep a solid ivory centre with two narrow interpolated-alpha bands
at each edge, falling from opaque through 45% to transparent. Six vertices per
curve sample describe five adjacent bands; they share boundaries, with no layered
alpha overdraw. Use per-vertex colour in the existing raylib batch, retaining the
art-resolution render target and ordinary blending. Shift misses 105% of body
width to keep the broader sweep beside the target. Timing and combat remain intact.
Fixed geometry grows from 42 to 126 vertices per ribbon, still four bounded records.
Verify the existing CPU/allocation suite and hidden GPU paths; replace the previous
six captures with the corrected appearance. Prior visual acceptance was not granted.


Correction verification: rebuilt `blobber`, `blobber_melee_swipe_tests` and
`blobber_world_integration_gpu_tests` with two workers and low CPU/I/O priority.
The CPU geometry/lifecycle/allocation suite passes with the broader geometry.
Hidden OpenGL integration passes in both supersampling modes, including a new
pixel assertion that edge colours interpolate between actual scene pixels and
ivory while the core stays opaque. Mirrored winding, expiry, unchanged picking,
resource cleanup and allocation checks pass. The real-command hidden capture run
passes, with exactly one authoritative impact for each hit/miss. No gameplay or
asset republication was needed; no broad test matrix or benchmark was rerun.

Inspected updated peak, reveal and miss captures against the supplied screenshot;
the sweep is now roughly twice as long, with a fuller core and translucent edges.
All six prior gallery images have been replaced, preserving pending acceptance.
The initial thin treatment was rejected by the user. The run
`20261105T163045Z-4c729e5b113b48919ac10de4eca82c7d` was cleaned after preview; only
six selected gallery captures remain, with no retained diagnostic exception.

## Longer display — 2026-11-05 (implemented; timing awaiting user trial)

The user accepts the revised shape and requests twice the display time. Scale all
visual phase durations by two: 240 ms total, 70 ms reveal, retreat from 130 ms,
and contact at visual age 100 ms. Anchor this to the unchanged authoritative
200 ms impact: the swipe now runs from acceptance +100 through +340 ms.
Appearance and combat rules remain unchanged. Update elapsed-time regression
boundaries and the optional capture runner. Existing stills document the accepted
shape; they do not establish acceptance of the new timing.


Longer-display verification: Debug application and focused test targets rebuilt with
at most two workers and low CPU/I/O priority. `ctest --test-dir cmake-build-debug
-R '^(hud|targeting|melee_swipe)$' --output-on-failure -j 1` passes all three suites.
The updated geometry tests exercise doubled phase boundaries and +340 ms expiry;
allocation, capture anchoring and lifecycle coverage remain passing. Hidden
`blobber_world_integration_gpu_tests` passes at both quality settings with updated
expiry boundaries, blending, resource and allocation assertions. No new captures
were exported for a duration-only change; retained stills show the accepted shape
at the previous cadence. Shape is user-accepted; the new timing awaits user trial.
See [delayed creature hover](target_selection_design.md#delayed-creature-hover--2026-11-05-implemented)
for the accompanying tooltip change. Run `20261105T164048Z-af4c87c526ce4a8e8cdac1f74ab09bd1`
was cleaned after preview, with no new evidence or diagnostic exceptions retained.


## Duration tuning — 2026-11-05

The 240 ms version felt too long. Set total duration to 185 ms, preserving the
existing reveal/contact markers and shape; the retreat fades out sooner. This is
a one-value tuning change for immediate user trial. Rebuild the application only;
do not rerun tests or captures, as requested.
