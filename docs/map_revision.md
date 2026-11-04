# Floor map revision

Status: implemented and mechanically verified (2026-11-04). Visual acceptance
remains user review. [Approved layout reference](mockups/floor-map-approved.png)
and [current application captures](screenshots/map-icons-desktop/README.md).

### Readability/art follow-up (implemented and mechanically verified)

Increase cells by 50%, to 9×9 art units / 27×27 native pixels, preserving the
existing per-axis centering and character-centered crop. Replace all primitive
map/legend pictograms with imagegen artwork in the game's existing painted UI
style. Open/closed doors share the same frontal stone/brass frame, wood leaf,
palette and lighting; leaf position and the visible opening communicate state.
Only the party arrow uses teal. An eight-icon transparent 4×2 source sheet is
prepared once at startup into a small padded atlas owned by UiArt/GpuResources;
map, legend and quest keys share its regions, with rotation for party heading.
Source-cell alpha bounds are normalized during preparation, never during frames.
Missing/invalid artwork fails explicitly, with no procedural icon fallback.
This follows the persistent icon-art requirement in AGENTS.md.

The built-in imagegen source and exact prompt are retained in
[`assets/ui/map/`](../assets/ui/map/markers-v1.prompt.json). Startup preparation
accepts a bounded 2:1 RGBA sheet up to 4096×2048, splits fractional grid boundaries
(the generated master is 1774×887), and fits each alpha-bounded icon into a 48×48
region. Existing alpha-weighted area sampling excludes hidden RGB and faint
alpha noise; two-pixel gutters prevent neighbouring-region bleed. UiArt owns one
additional 208×104 RGBA GPU atlas (84.5 KiB) through the existing resource registry.
Source images and sampling scratch die after preparation. Party heading rotates
the shared raster region about its center. Map icons occupy 25×25 native pixels
inside 27×27 cells; legend samples are 36×36 pixels. No frame-time image processing
or allocation is introduced.

## Presentation

The map replaces exactly the 500×282 art-unit dungeon viewport, retaining the
1920×1080 presentation, portraits and bottom dock. The approved imagegen mockup
is a layout reference only; cell geometry is exact and marker artwork is generated.
The title is FLOOR N with M CLOSE at the right. A large left canvas uses fixed
9×9 art-unit (27×27 native-pixel) cells and one-native-pixel seams. Each axis
centers the map when it fits, otherwise centers the party without edge clamping.
Only whole cells inside the canvas are drawn. A right sidebar contains eight
aligned legend rows and all held quest keys, or None collected. Shared prepared
artwork draws padded cell-centered markers and larger legend samples. The party
arrow draws last. No explanatory copy, zoom, panning or gameplay changes.

## Knowledge and ownership

A graphics-independent MapOverlayModel belongs to each GameView. Preparation
copies the generated floor's bounded room membership (at most 1,600 cells / 16
rooms); it never retains a pointer into temporary generation data. Application
orchestration supplies the active generated layout at both Continue and travel.
Authored views without membership retain sight-based floor discovery.

Existing FloorGeometry discovery remains authoritative and unchanged. A room
becomes mapped when an interior non-solid, non-door/non-gate cell in that room
has already been discovered through line of sight. A seen blocking threshold is
not a room seed. Expand only that room's exact membership, never a flood fill
through connected floors. Corridors retain direct discovery. Actual solid tiles
in the eight-neighbour ring around mapped floor supply complete boundary walls
and corners, without exposing other floors. Directly discovered walls remain
visible. Full-map mode uses all floors and their boundary walls; deep exterior
solid space remains dark.

MapOverlayModel caches mapped floors/walls against world identity and discovery
revision, with fixed-capacity bitsets. Discovery refresh is bounded by the
existing discovery capacity. Full/non-discovery maps may be larger and use local
terrain queries for visible cells rather than a capacity-limited mask. All map
frame processing reuses storage and performs no engine-owned heap allocation.
Invalid/missing room metadata falls back to sight-based discovery. No capacity
growth or unbounded traversal occurs during frames.

Fixture markers retain existing known/remembered rules, open/completed state and
collected-key hiding. Derived room revelation does not change interaction
knowledge, enemy perception, simulation digests or save bytes. Existing saves
benefit immediately from their recorded discovery; no schema migration is needed.
Presenter borrows the model only during drawing, before its GameView is destroyed.
Development reveal remains inside the panel; the legend stays visible in full-map
mode rather than replacing it with generation text. Map pause/input isolation is
retained. Standalone maps without a depth use FLOOR 1; the campaign hub uses FLOOR 0.

## Verification

Owned small fixtures cover LOS/closed thresholds/opening, exact irregular room
membership, room isolation, corners, islands, corridor bends, persisted knowledge,
full-map equivalence, remembered icons, fixed-size axis centering/cropping and
zero allocations. Run the map, fixture, HUD/input and campaign-save smoke checks
in one development configuration. Hidden application captures verify native and
reduced presentation, partial/full/oversized maps, icon placement, OpenGL errors
and resource release. Production review loads an isolated copy of the user's
save and verifies original bytes unchanged; it is separate from routine tests.
Use low CPU/I/O priority, at most two build workers and one heavy job at a time;
never inject desktop input.

### Executed checks

Readability/art follow-up:

- Debug application and affected targets built with two low-priority workers;
  prepared-art build dependencies refreshed once. No alternate configuration,
  save-suite rerun or benchmark was needed for this presentation-only change.
- `map` (0.01 s) and `image_preparation` (0.02 s) pass. Coverage includes 27-pixel
  framing/cropping, odd-sized sprite sheets, alpha-bound centering/aspect, hidden
  RGB isolation and explicit rejection of missing/opaque/empty icon cells.
- The owned hidden GPU smoke check passes raster-region selection for open/closed
  doors and texture rotation about the party's cell center, plus existing native
  artwork and resource lifecycle checks. Its generated fixture images include
  the newly required map sheet and the existing creature portraits.
- The hidden map review passes OpenGL/resource-release checks, unchanged continued
  simulation and zero engine-owned C++ allocations in warmed map frames. All eight
  new [native/reduced captures](screenshots/map-icons-desktop/README.md) were inspected,
  including the supplied save, both door states, all markers/quest keys and a map
  cropped around the party near an edge. Both original and copied saves retain
  their original SHA-256. Interactive acceptance remains user review.

Initial 18-pixel/procedural-icon delivery (superseded by the follow-up above):

- Built the Debug application, map/fixture/HUD/command/save test targets and the
  opt-in GPU review runner with two low-priority workers. Existing prepared-art
  build dependencies refreshed once; no alternate build matrix or benchmark ran.
- `map` passes (0.01 s), including persisted discovery, threshold seeding, wall
  completion, full-map equivalence, hidden/remembered/current fixture markers,
  oversized axes and zero C++ allocations on both refresh and cached queries.
- `fixtures` (0.28 s), `commands` (0.01 s), `hud` (0.02 s), and
  `campaign_save_smoke` (17.97 s) pass. Passing checks were not rerun after
  unrelated fixture/test-only edits.
- The initial HUD failure reproduced with HEAD's HUD test, layout header and
  combat implementation compiled separately. Its old assertion excluded waiting
  enemies from initiative, contrary to the already accepted combat-positioning
  system. Only the stale test expectation was corrected; combat code is unchanged.
- Eight hidden application captures cover native/reduced display, complete
  boundaries, all pictograms, both quest keys, closed/open doorway revelation,
  oversized clipping and the continued production save. All were inspected.
  The review checks resource release, OpenGL errors, unchanged simulation digest
  and zero engine-owned C++ allocations during warmed map presentation. It does
  not claim that third-party/driver C allocations never occur.
- X11 was inaccessible in the sandbox; the authorized hidden review ran outside
  it successfully. No OS input, foreign-window control or focus changes occurred.
- Original and copied saves retain their original SHA-256. The supplied floor
  has recorded interior LOS in 9 of 11 rooms; two rooms beyond the left closed
  door have no interior discovery. They correctly remain hidden. The full-map
  comparison shows those rooms, so the supplied discovered/full captures are
  intentionally different. The owned fully explored fixture matches full-map
  terrain exactly.

Interactive input/visual acceptance remains the user's review; no automated
desktop interaction was attempted.
