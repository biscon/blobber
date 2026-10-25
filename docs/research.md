# Rendering research

## Historical references

The following are primary sources from the ScummVM project's implementations of
these game engines. They provide inspectable evidence of the techniques, rather
than an assumption that all early first-person games used raycasting.

1. [Eye of the Beholder / Lands of Lore shared scene renderer](https://github.com/scummvm/scummvm/blob/master/engines/kyra/engine/scene_rpg.cpp).
   `generateBlockDrawingBuffer` assembles floor/ceiling data and wall tiles using
   direction-dependent visible blocks. Its documented visible-block stencil and
   fixed destination offsets show how nearby grid cells become a perspective
   image. Occlusion and clipping rules prevent foreground walls from exposing
   shapes behind them.
2. [Might and Magic: Xeen scene implementation](https://github.com/scummvm/scummvm/blob/master/engines/mm/xeen/interface_scene.cpp).
   The draw lists contain positioned image frames with scaling/mirroring flags.
   This supports a view-slot/sprite composition model for Might and Magic IV–V,
   including the indoor/outdoor variants, rather than a continuously movable camera.
3. [Lands of Lore screen effects](https://github.com/scummvm/scummvm/blob/master/engines/kyra/graphics/screen_lol.cpp).
   `smoothScrollZoomStepTop`, `smoothScrollZoomStepBottom`,
   `smoothScrollHorizontalStep`, and the three `smoothScrollTurnStep` routines
   manipulate bitmap regions for movement. This establishes that animated motion
   can be achieved while keeping the underlying renderer and poses discrete.

These implementations were consulted to understand the architecture. No ScummVM
code, original game data, or original game art was copied into the prototype.

## Prototype adaptation

The prototype uses a 640×360 art grid and a 90° horizontal field of view.
It prepares its own wall view slots from generated textures at startup,
rather than requiring artists to hand-paint every front/side/distance variation.
Twelve visible depth rows and their lateral slots support longer corridors and
open rooms. Cropped sprite columns retain depth for correct overlap.

The virtual viewpoint is 0.4 tiles behind the player center, inside the same tile.
This brings immediately adjacent side passages into view and preserves a strip
of foreground floor when facing the nearest possible wall. Moving the projection
horizon to 36% of the viewport height gives more space to the floor while keeping
wall edges vertical. These are usability choices for this prototype, not measured
reconstructions of the original games' perspective settings. The offset is applied
to wall depth and floor/ceiling sampling consistently. Lateral sprite coverage
is derived from the field of view so wide rooms remain fully covered.

Floor and ceiling projections are also prepared once. Four independent X/Y
parity combinations keep the repeating materials attached to the map during
side steps. Wall strips also retain both material parity variants so that
changing the base tile at a strafe endpoint does not flip their artwork.

Forward/backward zoom/inset and turn-strip effects are a new implementation of
the broad image-space idea, not a reproduction of every Westwood animation frame.
They are deliberately brief and can be disabled with Tab.

### Strafe correction

Tracing [the scene-level movement routines](https://github.com/scummvm/scummvm/blob/master/engines/kyra/engine/scene_lol.cpp)
shows that Lands of Lore dispatches side steps separately from turns. Its strafe
effects combine partial, unstretched bitmap regions, while turns use the separate
stretched-strip routines. It does not use the full-viewport slide originally
implemented in this prototype. The source was inspected as a behavioral reference;
its code was not copied.

For this prototype's wider viewport and open rooms, strafes now use prepared
views at eighth-tile lateral offsets, maintaining the same heading. This gives
depth-dependent parallax and correct newly exposed geometry. It is an intentional
extension of the prepared 2D-image approach, not a claim that Lands of Lore used
this exact implementation. The grid-based player state and collision are unchanged.

## Library version

[raylib 6.2 release notes](https://github.com/raysan5/raylib/releases/tag/6.2)
identify a CMake issue in the previous release as a reason for the update.
The project therefore uses 6.2 within the requested raylib 6 series. CMake pins
the release archive and its SHA-256; it does not track a moving branch.
