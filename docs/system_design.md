# System design

Implemented and accepted by the user (2026-10-25): slice 04
[party HUD and command design](party_hud_design.md). Four portraits, command
previews, visible-body targeting and a bounded log read the existing party/actor
state; no combat resolution or resource spending is integrated. Explicit UI demo
scenarios keep sample mana/status values separate from authoritative party state.
See [slice 04 verification](slice04_verification.md). The typography correction uses
size-specific font atlases and native 1920×1080 text over the enlarged art,
with rounded text positions. The HUD design records the live-animation correction:
interactive demo clocks advance; capture clocks remain explicit, and automatic
idle phase offsets prevent default-placed actors from moving in lockstep. The UI
demo prepares its own idle-only placement copy, shared by interactive and
capture scenarios, so stationary actors do not inherit showcase walk/attack loops.
Expanded log layout packs entries by their measured wrapped line height,
with a small inter-entry gap and bounded drawing inside the panel.

Implemented and accepted by the user (2026-10-25):
slice 03, [mutable monster world](actors_design.md).
Runtime actors now supply live collision/rendering state, health and lifecycle.
Prepared appearances and asset formats remain unchanged; scripted diagnostics
exercise mutation and the 100 ms white damage/death flash.

Gameplay extension: [SRD 5.1 demo design and self-tracking roadmap](dnd5e_demo_design.md).
It defines sixteen individually reviewed slices toward a four-character goblin
combat demo. Slice 01's [rules foundation](rules_design.md) is implemented and
accepted by the user (2026-10-25): explicitly seeded dice, pure attack/save/damage rules,
and an immutable catalog loaded from bounded JSON before image preparation.
The application owns the catalog, starting party and runtime actors for the session;
combat resolution is not integrated yet. Rules are graphics-independent, JSON stays in a separate I/O library,
and typed catalog indices are stable for one non-recycled catalog lifetime.
The [source ledger](srd_content_ledger.md) records adopted SRD data and adaptations.
The roadmap owns delivery
status, user decisions, verification records, and handoff notes; this overview
continues to describe implemented systems and must be updated as slices land.

Implemented and accepted by the user (2026-10-25):
[characters and preset party](characters_design.md) is slice 02. Independent race/class/item definitions and an editable preset feed
pure stat derivation and a bounded four-character owner. It initializes HP, Hit
Dice, XP, and relative formation before world/image preparation. The party exposes
const state and checked formation swaps; combat, resource spending, inventory
transactions, and mana are deferred. See [slice 02 verification](slice02_verification.md).

NPC extension: [directional sprites and slow animation](npc_design.md) is implemented.
It adds immutable level NPC placement data, JSON assets, and prepared alpha animation
frames. Runtime actors initialize from those placements; a separate animation
player selects idle, walk-in-place, or frontal attack frames and transient hit feedback.
There is no autonomous movement or combat yet.
The [NPC design](npc_design.md#rendering) also covers prepared, floor-projected
contact shadows, blended beneath bodies and clipped against wall depth.

Status: implemented. The foundation refactor preserves the established renderer,
projection, movement, and art. See [verification](verification.md) for executed
checks and limits.

## Boundaries and data flow

- **World:** compact immutable row-major solid-cell storage and immutable NPC
  loading/placement data; grid directions and coordinates are value types. Loading and connectivity validation happen before
  play. A map is limited to 1024×1024 cells and rejects malformed input explicitly.
- **Party/rules:** immutable catalog values feed pure, allocation-free character
  derivation. A four-character party owns starting builds/gear, derived caches,
  resources, and a formation permutation. Typed character/item indices never
  recycle; formation swaps preserve identities and resources. JSON/loading stays
  outside game code. Schema 2 adds party content; strict schema 1 remains readable
  as rules-only content, but startup requires a preset.
- **Actors:** a fixed-capacity runtime owner initializes monster HP and transforms
  from placements and the catalog. Checked movement, facing, damage and removal
  atomically update records, occupancy and revisions. Typed IDs include a unique
  world identity; slots never recycle. Dead records remain readable, removed IDs
  fail lookup, and only living actors block movement.
- **Movement:** owns player pose, one pending command, transition state, and a
  revision counter. Exposes a const state snapshot; consumers cannot mutate its
  invariants. Key repetition is a separate graphics-independent input component.
- **Assets and preparation:** file loading produces owned CPU materials and JSON
  NPC definitions/placements. NPC source images become bounded prepared RGBA
  clip/direction/frame/depth sprites and are released before play. A scene cache baker prepares eight lateral view phases, wall strips, and sixteen
  floor/ceiling variants per phase. CPU composition accepts generated materials
  as well as file-loaded ones, so tests do not require game artwork.
- **CPU rendering:** immutable prepared data plus caller-owned fixed-size frames.
  Scene composition and image transitions write into supplied buffers. A frame
  pipeline owns source/destination/display buffers and only updates changed views.
- **Platform:** raylib window/input adaptation and GPU ownership. Typed texture
  and render-target handles refer to fixed-capacity, generation-checked slots.
  Borrowed raw raylib values stay inside short platform calls. RAII unloads GPU
  resources before the window closes, including exception paths.
- **Commands/UI:** a graphics-independent controller validates typed preview
  commands against borrowed party/actor state and current presentation eligibility.
  It owns selection, bounded target membership, a 32×192-byte log and a fixed HUD
  snapshot. Focus/edge handling and aspect-fit hit testing are CPU-testable.
- **Presentation:** the HUD draws from movement, map and command snapshots; the presenter
  owns handles for the viewport texture and three targets. It retains the 640×360
  art grid → 1920×1080 texture → aspect-fit framebuffer path. HUD geometry stays
  on the art grid; text is drawn at native resolution onto a separate transparent
  1920×1080 target. Final aspect-fit sampling uses point filtering for art and
  bilinear filtering only for premultiplied text, preserving strokes when reduced.
  Captures temporarily composite both layers into an owned diagnostic target.
  Four startup-loaded RAII font atlases provide Plex Sans regular 21/24 px, bold
  27 px, and Inconsolata 24 px for logs; every draw/measurement uses its atlas
  base size. Font ownership is separate from the generic texture registry and
  ends before window shutdown, including partial loading failures.
- **Application/tools:** option parsing, loading, scheduling, and lifecycle only.
  Captures are an explicit diagnostic mode with startup-prepared cases; output paths, filesystem work and image export are not normal gameplay frame work.

Dependencies flow from application/presentation toward platform, CPU rendering,
movement, and world values. World/movement/input timing have no raylib dependency.
There is no ECS, generic event bus, virtual renderer hierarchy, or service locator.

## Ownership, storage, and limits

Maps own contiguous cell bytes. The application owns the map and renderer for
the session. Movement and UI borrow them only during calls. A scene change would
prepare a replacement outside the frame loop; live streaming is not implemented.

Prepared sprites refer to ranges in contiguous column and color arrays, replacing
per-column pixel vectors. These are immutable offsets with cache-local lifetime,
not recyclable object IDs. Preparation may grow containers; rendering cannot.
The projection and eight phases bound the cache. Each phase permits at most 1024
sprites, 512,000 columns, and 16 million colors; exceeding a limit fails loading.
No eviction or hidden runtime growth occurs.

Runtime actors retain up to 256 stable records and one byte of live occupancy
per map cell. The immutable placement mask is loading data only. The map and
catalog outlive the noncopyable/nonmovable actor owner. Runtime operations return
fixed result values without allocation. A death unblocks immediately; animation
shows a white frozen pose for 100 ms before hiding body/shadow.

Each 500×272 RGBA frame allocates once, is move-only, and cannot resize. The live
pipeline owns three frames (about 1.56 MiB), reusing them across idle, movement,
collision, NPC alpha composition, and transitions. Composition copies pixels but
never allocates output storage. Input buffers at most one fresh movement command. UI confirmation is edge-triggered
and never queued. UI formatting uses stack buffers. Each frame pipeline also owns
one fixed 500×272 uint16 picking buffer (~266 KiB), written alongside prepared body
pixels. Living target membership is scanned after composition; transitions expose
no picking. Transparent holes/walls/shadows do not select, and dead flashes occlude
without becoming targets. Revisions invalidate picking with the image.

Party storage is four records with up to 16 owned starting item entries each,
not the future inventory grid. Definitions allow 16 races, 16 classes, and 64
items; exceeding a bound rejects preparation. Equipment borrows owned entries,
weight uses integer hundredths of pounds and int64 totals, and derived stats use
fixed arrays. Party construction may copy strings; steady-state callers borrow
its const snapshot. The application keeps the catalog alive and unmoved until
after party destruction. Character IDs are party-local, owned-item IDs are
character-local, and catalog IDs remain catalog-local.

The GPU registry holds up to eight standalone textures and four render targets.
Handles include a typed slot index and generation. Invalid/stale lookup returns
no resource; checked platform operations reject it. Exhaustion is an explicit
startup error. Releasing a slot invalidates its generation; exhausted generation
counters retire that slot rather than allowing old handles to become valid again.
Registry identity prevents handles from accidentally crossing registries. Slots
contain resource values, not owning raw pointers. No extra handles are introduced
for ordinary grid coordinates or transient references.

The allocation guarantee covers engine-owned CPU gameplay processing. The test
intercepts ordinary and aligned C++ `new`/`new[]`; it does not intercept C
`malloc` calls. Engine-owned frame code uses no C allocation APIs. Loading,
cache preparation, exceptional error reporting, captures, and driver/raylib
internals are outside it. A targeted allocation guard is more useful here than
a Vulkan-style benchmark or stress-testing framework.

## Interfaces and preservation

The composition API writes to `Frame&`; callers must supply distinct buffers for
image transitions. The frame pipeline consumes a read-only movement snapshot,
tracks movement, actor-world identity/revision and NPC animation revisions, and publishes a const display frame plus a changed flag so idle
frames do not trigger texture uploads. Prepared cache configuration and content
remain immutable after construction. A pipeline belongs to one map, scene cache,
actor world, animation player, and controller lifetime: construct a new pipeline when replacing those systems,
because revisions are local to that controller. Moved-from frames may only be
destroyed or assigned another frame before use.

Keep controls, collision, queued-command behavior, the 90° field of view,
0.4-tile viewpoint setback, raised horizon, strafe parallax, and existing image
transition effects. Capture filenames remain stable for before/after comparisons.
Prefer an installed compatible raylib 6.2 package, otherwise use the current
hash-verified FetchContent fallback. nlohmann/json 3.12.0 likewise prefers the
installed package with a hash-pinned download fallback; only asset I/O uses JSON.
Debug/Release presets stay CLion-compatible.

## Validation strategy

- Independent core tests: movement, malformed/bounded maps, input repeat, focus
  reset, state transitions, and aspect-fit behavior.
- Renderer tests: generated CPU materials and a dedicated test-owned regression
  map; preserve the independent geometric oracle, side-passage/floor framing,
  strafe reversal, and transition endpoint checks.
- Resource tests: typed handles, stale IDs, slot reuse, capacity, registry lifetime,
  and CPU material loading failures using isolated test-owned files.
- NPC tests: bounded JSON loading, generated sprites, 2x2 occupancy and collision,
  four view sectors, scale/grounding, transparent overlap, viewport clipping,
  and wall occlusion against independent ray/block intersections.
- Party/rules tests: deterministic dice, strict versioned content, independent
  race/class builds, armor/weapon/weight calculations, initialized resources,
  formation identity, and rejected invalid input.
- Actor tests: generated catalogs/maps, capacity and binding failures, stale/foreign
  handles, transactional occupancy, queued movement, exactly-once death/removal,
  stationary view refresh, hit-flash boundaries and all movement transitions. Routine fixtures own their data;
  shipped content and startup validation are checked separately.
- Command/UI tests: preview validation without costs, live formation, foreign and
  stale identities, 256-target paging, bounded logs, development-only statuses,
  focus/held input, letterbox/pillarbox/high-DPI inverse mapping, alpha/depth-aware
  picking, transition suspension, and stationary invalidation.
- Allocation regression: enable an allocation counter after preparation, exercise
  first and subsequent moves, all transitions, blocked/queued input, and held/advancing NPC
  frames, actor mutations/flash expiry/removal, rules/party queries and formation swaps, HUD refresh/commands/log rollover/input/picking, and require zero C++ allocations in the normal CPU frame path.
- Desktop integration: check actual GPU handle creation/release and presentation,
  produce the same deterministic captures, and compare them with the baseline.
  Capture/export allocations are intentionally excluded from the frame guarantee.
- Build and run CTest in Debug and Release. Record executed checks and limitations
  in [verification notes](verification.md).

Historical techniques and visual choices are documented in [research](research.md).

## Source map and extension points

| Location | Responsibility |
| --- | --- |
| `src/world/` | Grid values, immutable map and NPC placements/occupancy |
| `src/rules/` | Deterministic dice, pure attack/save/damage and character derivation, immutable typed content catalog |
| `src/game/` | Runtime actors/occupancy, discrete movement, animation state, bounded party/resources/formation |
| `src/input/` | Platform-independent key repeat timing |
| `src/assets/` | CPU image ownership, materials, NPC JSON/images, and separate rules JSON loading |
| `src/render/` | Projection, prepared scene data, composition, transitions, frame reuse |
| `src/core/` | Fixed-capacity typed resource slots |
| `src/platform/` | Window lifetime, keyboard adapter, GPU registry and presentation |
| `src/ui/` | Portrait/action HUD, shared layout/input routing, targeting and bounded-log presentation |
| `src/app/` | Startup/options, frame scheduling, diagnostic capture cases |
| `tools/` | Offline texture normalization |
| `tests/` | Core, renderer, resource, NPC, allocation, and explicit desktop checks |

NPC appearance indices remain stable for a prepared level lifetime. Runtime actor
slots never recycle; removal leaves a terminal record. Runtime spawning is deferred. Other future interactive objects should have their own
owner and lifetime model before introducing handles for them. They should not be
added to the GPU resource pool.
Future overlays belong in the UI; new movement rules belong in the controller or
world queries; material loading remains outside composition. Preparing a new
level or material set is an explicit loading operation. These boundaries do not
promise live streaming, asynchronous loading, or arbitrary wall heights.

Implemented extension: [slow NPC sprite animation](npc_design.md#sprite-animation)
adds bounded clip timing and prepared animation frames, preserving immutable
placements and the current image-composition/presentation pipeline. The application
owns a graphics-independent animation player with fixed-capacity snapshots. Source
images are released after preparation; timing metadata survives independently.
No AI or combat is attached to these visual loops. Controlled actor movement and
health/lifecycle mutation are described in the actor design; prepared caches stay
immutable and are reused at each runtime position.
