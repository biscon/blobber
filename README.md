# Stone & Shadow

A C++17 / raylib 6.2 prototype of a 1990s grid-based dungeon crawler. Rendering,
six-direction keyboard movement, wall/NPC collision, and a small exploration level
with a stationary four-view goblin.

![The great hall](docs/screenshots/great-hall.png)

## Build and run

Requirements: a C++17 compiler, CMake 3.24+, Ninja, and desktop OpenGL 3.3 support.
CMake first looks for an installed raylib **6.2** package. If absent, it downloads
the pinned release through FetchContent and verifies its SHA-256. Subsequent
builds can run offline. Set `BLOBBER_USE_SYSTEM_RAYLIB=OFF` to force the pinned fallback.
NPC JSON uses **nlohmann/json 3.12.0** with the same installed-package preference
and a SHA-256-pinned download fallback (`BLOBBER_USE_SYSTEM_JSON=OFF` forces it).

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./cmake-build-debug/blobber
```

For optimization, substitute `release` for `debug` in all commands. To use a
different generator, configure normally, for example
`cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`, then `cmake --build build`.
Multi-configuration generators also work; select the desired configuration when
building and running tests.

On Debian/Ubuntu, the native build dependencies are typically:

```sh
sudo apt install build-essential cmake ninja-build libx11-dev libxrandr-dev \
    libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

Audio is disabled. On Windows use a configured MSVC or MinGW toolchain; on macOS,
Xcode command-line tools plus CMake and Ninja. Linux/X11 is the verified platform.

### CLion

Open this directory or its `CMakeLists.txt`. Enable the `debug` or `release`
CMake preset, wait for configuration, select the **blobber** executable, and Run.
CLion's ordinary CMake profiles also work. No machine-specific `.idea` files or
working-directory setup is required: assets are copied beside the executable.

## Controls

| Key | Action |
| --- | --- |
| W / S | Forward / backward one tile |
| A / D | Strafe left / right one tile |
| Q / E | Turn left / right 90 degrees |
| Tab | Toggle animated / instant steps |
| F1 | Toggle the development map overlay |
| Esc | Exit |

The first action occurs on keypress; holding a key repeats after 300 ms and then
every 180 ms. An animated action lasts 160 ms. At most one fresh keypress is
buffered during an animation. Losing focus clears the buffer and held-key state.
Collisions block the entire move, including its visual effect.

The player starts at `(3, 21)`, facing north. The enclosed 25×25 test map contains
two large chambers, pillars, loops, dead ends, and long corridors. Press F1 to
inspect the layout. `assets/maze.txt` is plain ASCII: `#` is solid, `.` is floor.
All floor tiles must be reachable from the spawn.

## NPCs

The test goblins stand at **(18,17)** in the southeast great hall, facing south.
The player still starts at (3,21). Use F1 and the coordinate readout to navigate;
stand at (18,18), (18,16), (19,17), or (17,17) and face its cell to inspect all four
views. Any occupant blocks player entry into the entire cell.

![The goblin in the great hall](docs/screenshots/npc-front.png)

Edit `assets/npcs/level.json` to list definition files and placements. Each
placement has a string `type`, integer `cell: [x,y]`, `subcell: [x,y]` with each
component 0 or 1, and `facing: "north"`, `"east"`, `"south"`, or `"west"`.
The four subcells are world-aligned quarter/three-quarter positions; at most one
NPC occupies each slot. The same entity representation supports future friendly
and hostile types; this slice has no behavioral distinction.

A definition such as `assets/npcs/goblin/goblin.json` contains `id`, `width`,
`height` (full canvas dimensions in cell units), and four `views` named `front`,
`back`, `left`, `right`. Left/right refer to the NPC's anatomical sides. Each view
is a PNG path or `{ "image": "front.png", "anchor": [0.5, 1.0] }`; anchors are
normalized foot positions, defaulting to bottom center. Paths are relative to
the JSON that declares them. An optional `alpha_cutoff` (default 0) removes pixels
at or below that alpha during preparation. Source PNGs remain unchanged.

Rebuild to copy edited assets beside the executable. Custom `--assets` directories
must include `npcs/level.json`; use `{ "definitions": [], "instances": [] }` for
an empty level. Invalid placements, unknown types, malformed assets, and exceeded
limits fail at startup with an error. See the [NPC design](docs/npc_design.md) for
ownership, limits, and future extension points. NPCs are frozen: no AI, animation,
combat, stats, or runtime spawn/despawn is implemented yet.

## Rendering

This is a **2D image compositor**. At startup it prepares perspective wall sprites
for fixed cardinal headings, covering twelve tiles of depth and eight lateral
fractions of a tile. Each fraction has sixteen floor/ceiling backgrounds (four
headings × four world X/Y parity combinations). It then selects and layers
those sprites for the current view. Each prepared sprite stores cropped vertical
pixel strips and their depth; a column coverage buffer resolves occlusion at
corners and pillars. Equal-height walls make this coverage representation exact.
NPCs use pre-scaled, shaded alpha images at the same subcell depths and strafe
phases. They are blended far-to-near and clipped against wall column depths,
including partial pillar occlusion. Soft semitransparent contact shadows are
prepared as floor-projected blobs and blended beneath the bodies. The runtime
scene compositor uses neither
ray traversal nor a 3D camera/mesh.

The prepared projection uses a 90° horizontal field of view and a viewpoint 0.4
tiles behind the player's tile center, still inside that tile. This exposes side
passages while standing beside them and keeps an adjacent wall far enough away
to leave foreground floor visible. The horizon is 36% down the viewport, giving
more space to the floor. This is an off-center projection, keeping vertical walls
upright rather than pitching them. `ViewProjection` in `src/render/projection.hpp` holds
these settings; wall sprites, floor/ceiling sampling, and movement effects share
them. The player remains at the tile center for movement and collision.

The textures are original imagegen artwork, normalized to 256×256 PNGs with
nearest-neighbor sampling. Mirrored tile addressing joins texture edges.
Side faces, ceilings, distance shading, and a twelve-tile fade give depth cues.
Floor, ceiling, and wall texture parity is anchored to the map so materials remain
attached to their surfaces throughout a side step and at its endpoint.

Grid moves and headings remain discrete. Forward/back movement uses short
image-space zoom/inset effects; Q/E turns use stretched strips. A/D strafes use
the prepared fractional views at the same heading: nearby objects move farther
than distant walls, the horizon stays fixed, and newly revealed surfaces are
composed with proper occlusion. Left strafes reuse the same eight-step cache in
reverse. The game still draws 2D images; no runtime 3D scene or raycaster is used.
Tab switches transitions off.

The full presentation uses a 640×360 pixel-art grid. The viewport is 500×272 at
`(132, 8)`, with the left and bottom areas available for future UI. That composition
is enlarged exactly 3× into a **1920×1080 render texture**. Finally that texture
is fitted to the actual framebuffer, centered with black bars and point filtering.
Window resizing never changes the logical resolution, perspective, or UI layout.
At noninteger scales, pixel widths necessarily vary slightly to fill the fitted area.

## Assets and screenshots

Keep disposable generation experiments, unselected sheets, and intermediate
files in `/tmp` outside the repository or in the ignored repository directories
`tmp/`, `temp/`, `scratch/`, or `output/`. Asset-local directories named `tmp`,
`temp`, or `scratch` are also ignored. Use `captures/` or `captures-*/` for
disposable diagnostic captures.

Selected source artwork, animation sheets, runtime textures/frames, manifests,
prompts, and curated previews in `docs/screenshots/` are intentional versioned
assets, even when generated by a tool. Do not put them in the scratch directories.

The three original images are in `assets/originals/`; the runtime textures are in
`assets/textures/`. Each exact generation prompt is saved as
`assets/{wall,floor,ceiling}-prompt.txt`. All three were made using the built-in
imagegen tool; no original game artwork is included. The four goblin views were
also generated with the built-in imagegen tool. Selected PNGs are in
`assets/npcs/goblin/` (`front.png`, `back-v2.png`, `left.png`, `right.png`), alongside
the exact prompt history in `prompts.txt` and an unused first back-view variant.

To repeat the deterministic normalization of the generated originals:

```sh
cmake --build --preset debug --target blobber_prepare_assets
./cmake-build-debug/blobber_prepare_assets assets/originals assets/textures
cmake --build --preset debug
```

The developer capture mode saves eleven static poses, the midpoint of all six
transitions, both strafe sequences at `(8,4)` facing west, plus six NPC poses and
two nine-frame NPC strafe/reveal sequences (**59 logical captures** total). It
also saves a screenshot of the actual window, then exits:

```sh
./cmake-build-debug/blobber --size 1024 768 --capture /tmp/blobber-captures
```

`--assets DIRECTORY` overrides the default executable-relative asset directory.
Missing assets or malformed maps produce a clear error and a nonzero exit status.
Representative verified screenshots are included in `docs/screenshots/`.

## Code and checks

The [system design](docs/system_design.md) describes module boundaries, ownership,
capacities, and extension points. The project is organized into world, movement,
input timing, assets, CPU rendering, UI, platform, and application modules.
`main.cpp` only calls the application and reports errors.

The CPU frame path reuses three fixed-size buffers. Prepared wall data uses
contiguous column/pixel arrays, and unchanged views skip viewport uploads.
GPU textures and render targets are owned by bounded registries with typed,
generation-checked handles and RAII cleanup before the window closes.

Five routine CTest suites cover core behavior, rendering against an independent
geometric oracle, resource lifetime/loading failures, NPC loading/rendering/collision,
and zero C++ allocations in engine-owned CPU frame processing. They own their fixtures and need no display.
Loading, preparation, diagnostic exports, and raylib/driver internals are outside
the allocation guarantee. No performance benchmark framework is required.

The separate desktop GPU lifecycle check is explicit:

```sh
cmake --build --preset debug --target blobber_gpu_smoke
./cmake-build-debug/blobber_gpu_smoke
```

C++ formatting is defined by `.clang-format`. See [research notes](docs/research.md)
for historical sources and [verification notes](docs/verification.md) for checks
and remaining platform limitations.

The prototype has no combat, NPC behavior, inventory, interactive doors, audio,
save system, or procedural level generation. Walls have one uniform height;
transparent walls and variable elevations would require a richer coverage model.
