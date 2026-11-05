# Native-resolution HUD and impact alerts

Status: implemented, **accepted and closed**, 2026-10-27. The user explicitly
accepted slice 10 including this correction. Slice 11 remains Planned/unstarted.

## Rendering and ownership

Presenter owns a point-filtered dungeon texture/base target and a transparent
1920×1080 native UI target, fitted with bilinear sampling. Existing logical UI
coordinates and input mapping remain unchanged. HudPainter converts geometry to
native pixels; fonts already use native coordinates. One ordered UI pass draws
artwork, geometry and text together, so modal backgrounds cover all three.
World selection contours retain picking-mask alignment with native-width strokes.
The dungeon's prepared pixels, projection and discrete movement do not change.

UiArt owns one startup-prepared 2048×1024 RGBA atlas through GpuResources. Twenty-nine
256-pixel cells hold portraits up to 216×252 and icons up to 108×108, with two-pixel
edge extrusion. Sources remain intact. Portraits crop centrally to destinations;
icons aspect-fit. Source decoding/resampling occurs only at startup. The atlas is
8 MiB, with no retained decoded source images or per-frame allocations. The old
640×360 HUD target is removed. Code-native shapes supply thin borders, checkmarks,
darts and a clean eight-tooth gold cog; spell icons get a separate dark frame.
The Class Features correction adds one shared laurel emblem in the existing atlas;
its size and resource ownership are unchanged.

## Combat timing

Delayed outgoing attacks alert only at authoritative impact, before damage, for a
valid living target. Misses and zero damage alert; canceled/gone targets do not.
The existing radius-three local flood uses the current target cell and current
party position. Lethal hits therefore alert surviving neighbors. Different missile
recipients alert at their own deadlines, subject to normal local propagation.
Instant hostile effects still alert immediately. Normal sight detection remains
independent. Costs, saved rolls, flight, effect ordering and turn clocks are retained.

## Verification

Generated GPU artwork checks native detail, fitting/cropping, padding and cleanup.
Hidden native/reduced captures cover the picker, dock, portraits/inspector,
initiative, targeting, cog and overlapping modal layers. No OS input automation.
CPU regressions cover awareness before/at impact, misses, lethal hits, cancellation,
gone targets, staggered missiles, normal sight, pauses and coarse updates. Run full
Debug/Release, focused sanitizers and allocation guards; retain review evidence.

Actual execution results and retained captures: [verification](verification/archive/native_hud_verification.md).
The atlas replaces the former 256×448 atlas (8,388,608 versus 458,752 bytes).
Removing the 640×360 render target saves its 921,600-byte color attachment plus
its depth storage. GPU driver allocations are outside engine allocation claims.
GameSession retains ordinary sight/contact detection while flights are pending;
approach-only death-save scheduling begins when the impact actually starts approach.

## B01 review adjustment — 2026-10-31

The initiative strip now anchors to the dungeon viewport’s right edge with a
six-logical-unit inset (18 native pixels), retaining its existing top edge, bounded
queue and portrait ordering. Its width follows the remaining queue, so the right
edge remains fixed as turns advance. This clears the central monster/health-bar
area. [B01 review evidence](verification/archive/b01_verification.md#user-review-adjustment--size-and-initiative-2026-10-31)
records the smaller skeleton metadata and fresh GPU captures. No per-frame storage,
input regions, render targets or resource ownership changed.

The B01 health-bar centering correction uses `TargetAnchor::badgeX`, the existing
projected body/ground axis, rather than the visible alpha-bound center (`x`), which
includes outstretched weapons and partial occlusion. Keep the current visible-top
height, viewport clamping and depth clipping. This shares the stable horizontal
anchor already used by status indicators; no new per-frame data or rendering pass.
Verify on a hidden production skeleton hover capture and the existing HUD and
visibility checks.
