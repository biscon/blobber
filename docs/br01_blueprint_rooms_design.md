# BR01: blueprint foundation and playable blockouts

Status: **Accepted and closed by the user**, 2026-11-09.
The user approved the detailed BR01 plan and requested implementation. Compact
rooms, a developer launcher and optional fixed combat were selected. The user then
approved the completed slice: “slice approved, update the plan doc”. BR01 layouts,
circulation and foundation contracts are accepted; BR02 and BR03 are not authorized here.

## Responsibilities and contracts

An immutable, preparation-only blueprint catalog owns room layouts and kit contracts.
Core blueprint validation/transformation/assembly has no graphics or JSON dependency.
Asset I/O parses strict schema 1 JSON; review tools export diagrams and orchestrate
ordinary GameSession/GameView/WorldRenderer/Presenter owners. Runtime consumes an
ordinary Map, scenery, fixtures and chest decorations, not live blueprint objects.
Purpose is independent of generation Role. Campaign generation and save versions
remain unchanged. No frame-time searches, mesh creation or new runtime furniture type.

Local (0,0) is the northwest interior cell; X east, Y south. The wall ring is outside
the footprint. Quarter turns are clockwise and rotate positions, headings, masks,
sockets and regions together; no mirroring/stretching. Models use cell units, scale
1, and an anchor-cell floor-center pivot: +X right, +Y up, +Z backward. Mesh bounds
must remain inside the kit footprint and declared height. Scenery blocks movement
and NPC occupancy but is not a terrain sight wall. Chests retain current walkable
party-cell/blocked-NPC semantics. Thin props have full-footprint blockout bases.

| Room | Interior / ceiling ticks | Occupied rectangles (x,y; width×depth) |
| --- | --- | --- |
| Kitchen | 5×5 / 6 | Hearth (0,0;2×1) south; work (4,0;1×2) west; provisions (0,3;1×2) east |
| Mead hall | 7×7 / 8 | Table groups (0,1;2×3), (5,1;2×3); solid pillars (1,5), (5,5) |
| Barracks | 5×7 / 7 | Beds (0,0), (4,0), (0,4), (4,4), each 1×2 south; lockers (0,2), (4,2), (0,6), (4,6) face aisle |
| Armory | 5×5 / 6 | Weapons (0,0;2×1) south; work (4,0;1×2) west; armor (0,4;1×1) east |

Kitchen/armory preserve column 2 and row 2; hall columns 2–4 and row 4;
barracks column 2 and row 3. Floors are zero; thresholds/vestibules use 4 ticks.
Kitchen/hall/barracks have cardinal sockets at the middle of each wall, except
hall side sockets use row 4. Armory has south-middle and west-middle sockets.
All nonempty socket subsets are supported. Unselected sockets remain solid.

Encounter rectangles: kitchen (2,3;2×2), hall (2,4;3×2), barracks (1,2;3×2),
armory (1,3;2×2). Reward/access pairs respectively (4,4)/(3,4), (6,6)/(5,6),
(3,6)/(2,6), (4,4)/(3,4). A review chest is empty, never an extra campaign reward.
The pair uses kitchen origin (1,3), hall (7,1), shared threshold (6,5), and both
south sockets. Assembly adds outer vestibules and border padding, then rotates
the complete result. Standalone spawn is the selected south vestibule by default.

Eight kit entries: hearth 2×1×1.0, work-surface 2×1×.35, provisions 2×1×.45,
table-and-benches 2×3×.4, sleeping-bay 1×2×.6, footlocker 1×1×.2,
weapon-storage 2×1×.8, armor-display 1×1×.8 (width×depth×height).
Pillars use ordinary terrain; chest art is reused. These bounds/pivots are BR02's
handoff contract, not finished art approval.

## Data, bounds and failure

Schema 1: kits (id, footprint, height); rooms (id, purpose, size, ceiling,
rotations, solid cells, placements with kit/anchor/facing, lane rectangles,
sockets with id/cell/outward heading, encounter rectangles, reward/access pairs).
Hard limits: 256 KiB JSON, 8 rooms, 16 kits, 9×9 interiors, 16 placements,
4 sockets, 2 encounter rectangles, 2 reward candidates, 8 lane rectangles.
IDs use bounded stable text; parsed references become typed nonrecycled indices.
All storage is bounded; strings/JSON/Map/prepared assets allocate only in preparation.
Catalog owners outlive validation borrows; published maps copy their values.

Validate the combined occupancy with fixed masks and flood-fill scratch: bounds,
overlaps, ceiling fit, wall backing, reserved paths, socket approaches, encounters,
reward access, and connectivity of every unblocked room cell without outside detours.
Every allowed rotation and socket subset is checked. Threshold cells reserve the
existing door sweep. Pair assembly checks opposing sockets and one shared door.
Bad input/exhaustion throws a contextual error before publication or session startup.
Existing scenery/world/GPU capacities are retained. No production fallback changes.

## Review and assets

An explicitly built developer launcher supports validate/plans/walk/capture, room
or kitchen-hall selection, rotations, socket IDs, blueprint path, prepared root,
output directory and optional combat. CPU modes work without a window. Walk uses
normal controls and never opens automatically during agent verification. Capture
uses a hidden application-owned window and in-process checked commands only.
Optional combat uses the existing preset party, seed 5, goblin and goblin archer
at opposite ends of the encounter's north row; pair combat is in the hall only.
It is spatial review, not balance calibration. No campaign/save owner is installed.

The world's opt-in blockout bake writes a separate review publication with a cloned
Underkeep binding, one small color atlas and prepared simple meshes. Production
pack sources and ordinary publication output stay unchanged. Generated review
assets are preparation output in the owned run, not shipped production artwork.

## Verification and handoff

Owned small CPU fixtures cover strict loading, capacities, transforms, connectivity,
socket subsets/pairing, blocked lanes/rewards and publication bounds. Integration
checks exercise movement, doors, empty chests and combat space with owned fixtures.
Prepared rendering checks pivots, collision bases, allocation-free CPU frames and
resource rollback; hidden review checks GL errors, picking and GPU lease cleanup.
Focused Debug regression selection: blueprints, scenery, fixtures, underkeep_render,
generation. Compare representative normal/fallback generation identities before/after.
No broad seed sweep, benchmark, second configuration or desktop automation.

Use configured CMake/Ninja, <=2 build workers, one test process and one expensive
job at a time at low CPU/I/O priority. Generate diagrams/captures/logs in an owned
agent run. Retain five diagrams and five native views (<=12 pending review), record
commands/results/limitations here, close runners and clean disposable output.
User interactive/visual acceptance is separate from mechanical verification.

## Execution record

Core implementation lives in `src/world/blueprints.*`; strict JSON loading and
offline mesh preparation live in `src/assets/blueprint_*`. The authored input is
`assets/blueprints/underkeep.json`. The CPU-only `blobber_blueprints` tool is also
available when graphics are disabled; `blobber_review_blueprints` adds the ordinary
gameplay and hidden capture paths. Neither executable is in the default build.
The review-only rules copy zeros monster loot/XP; all other combat rules are kept.

Catalog bounds are fixed arrays (including 33-byte IDs), with 81-cell masks and an
81-cell flood queue. Kit footprints are bounded to 4×4 to fit the existing positive
anchor-relative scenery contract. A review contains at most two rooms/32 scenery
placements; selected doorways get one-cell exterior vestibules within an enclosed,
padded canvas. The authored pair has one shared door, not overlapping fixtures.
Validation of all socket combinations is performed by the validation tools/tests;
individual preparation validates its chosen assembled thresholds and connectivity.

The eight listed kit IDs additionally define the concrete blockout forms. Hearth,
work-surface, provisions, weapon-storage and armor-display require backing terrain;
table groups, beds and footlockers are free-standing. Models have footprint origin
(0,0) relative to the anchor and local bounds X [-.5,width-.5], Z [-.5,depth-.5],
Y [0,height]. There are no visual overhangs in BR01. Rotating the instance rotates
the mesh and occupied cells around the same floor-center pivot.

### Reproduction

Use the configured CMake from the selected build's `CMakeCache.txt`; this checkout
uses `/home/bison/clion/bin/cmake/linux/x64/bin/cmake` and its configured Ninja.

```sh
BR_RUN=$(python3 tools/agent_artifacts.py create)
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/cmake \
  --build --preset debug --target blobber_blueprints blobber_review_blueprints \
  blobber_bake_world blobber_blueprint_tests blobber_blueprint_render_tests

cmake-build-debug/blobber_blueprints validate assets/blueprints/underkeep.json
cmake-build-debug/blobber_blueprints plans assets/blueprints/underkeep.json "$BR_RUN/plans"

# Separate publication; requires a new path. This includes normal NPC preparation
# and can take several minutes. Reuse it for every room/orientation in the run.
nice -n 15 ionice -c 3 cmake-build-debug/blobber_bake_world assets \
  "$BR_RUN/prepared/world" --blueprint-blockouts

# User-operated walk: normal controls, optional --combat, no campaign save owner.
cmake-build-debug/blobber_review_blueprints walk \
  --catalog assets/blueprints/underkeep.json --room kitchen-hall \
  --assets assets --ui-prepared cmake-build-debug/assets/prepared \
  --prepared "$BR_RUN/prepared" --output "$BR_RUN/walk"

# Hidden in-process script; does not inject desktop input or select windows.
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_blueprints capture \
  --catalog assets/blueprints/underkeep.json --room kitchen-hall \
  --assets assets --ui-prepared cmake-build-debug/assets/prepared \
  --prepared "$BR_RUN/prepared" --output "$BR_RUN/capture"
```

Room choices: kitchen, mead-hall, barracks, armory, kitchen-hall. `--rotation` accepts
0/90/180/270. For individual rooms, `--sockets south,west` selects authored socket
IDs before rotation; the pair fixes its join and south entrances. Default mode is
an empty walk; `--combat` enables the fixed two-actor encounter (hall only in the
pair). `plans` and `validate` modes do not create a graphics context. Each output
directory must be new. Runtime source assets and prepared UI/world roots are
independent, avoiding stale Debug asset copies. The UI root defaults to the
executable's `assets/prepared`; `--prepared` selects the separate review world root.

After review, close the launcher, select evidence, then preview and apply cleanup:

```sh
python3 tools/agent_artifacts.py clean "$BR_RUN"
python3 tools/agent_artifacts.py clean "$BR_RUN" --apply
```

### Mechanical verification

Configured Debug targets built with two workers. Six focused suites passed in
2.20 seconds: blueprints, blueprint_render, scenery, fixtures, generation and
underkeep_render. The blueprint suite additionally passed after a subsequent
bounded error-diagnostic guard. No unrelated suite or second configuration ran.

```sh
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest \
  --test-dir cmake-build-debug \
  -R '^(blueprints|blueprint_render|scenery|fixtures|underkeep_render|generation)$' \
  -j 1 --output-on-failure
```

Owned fixtures check malformed input, footprint transforms, local circulation,
240 fixture socket/rotation assemblies, checked door traversal/opening/closing,
empty chest access and bounded melee/ranged combat. CPU rendering checks 36 owned
kit orientations, raised floor pivots, publication/scene failure, lifetime and idle
caching. Actual door-box sweeps stay inside reserved threshold cells in all four
orientations. Allocation guards cover engine-owned C++ simulation and CPU drawing;
they do not claim to intercept C allocation, graphics drivers or other internals.

The shipped catalog separately validates 192 room/socket/rotation combinations and
four connected-pair rotations. Four depth-2 generation reports (seeds 0 and 5,
normal and forced fallback) match pre-change reports exactly apart from timing;
these particular samples are ChamberNetwork. Existing generation tests cover both
profiles; no additional seed sweep or performance claim is made. Campaign versions
and production pack source files were not changed. The normal application target
was not rebuilt, to avoid an unnecessary normal world-asset bake; the shared
application library is linked and exercised through the new review executable.

The initial full review bake exposed overhanging blockout supports/pillows. Bounds
were corrected and all primitive branches moved into small owned render fixtures.
A subsequent optional-mode provenance comparison omitted the new blueprint input
on its second scan; both scans now include it consistently. Final review publication
succeeded: 556 textures, 861,266,744 mip bytes and 64,967,422 alpha bytes. The extra
pack reuses Underkeep textures and adds one 64×8 color atlas; installed publication
size is not the active scene's resident GPU usage. Ordinary bake behavior is unchanged.

### Hidden review and acceptance

Seven successful hidden reviews produced 25 native views: each standalone room,
the connected pair, its 90° rotation and the optional production goblin/archer
preview. All views had zero nonedge CPU/GPU ID disagreements and no OpenGL errors.
Checked in-process commands traversed every selected door and opened empty chests.
Injected GPU acquisition failure preserved the active image and resource counts.
View destruction released model/texture leases; renderer destruction returned GPU
bytes to the persistent Presenter baseline. Font accounting returned to zero when
Presenter ended; its registry-owned UI targets/atlases were destroyed by the registry
before the hidden window closed.

The initial sandbox could not access X11. The authorized hidden display invocation
was used, with no mouse/keyboard injection, focus switching or desktop control.
The first capture harness incorrectly expected lazy renderer resources and then
registry-owned UI resources to end at shorter lifetimes. Checks now follow the
existing owners as described above; no renderer ownership changes were needed.
The optional actor binding uses the existing `goblin.archer` content ID, not its
hyphenated source-directory name. Only affected capture paths were rerun.

[Review gallery](screenshots/blueprint-rooms-br01/README.md) retains ten images:
five top-down plans and five selected 1920×1080 views. Blockout colors are temporary;
these images establish layout, scale and whole-cell reservations, not finished art.
Agent visual inspection is complete; user acceptance of BR01 was recorded on 2026-11-09.
The agent did not perform interactive desktop play. There was no benchmark, balance run, sanitizer matrix,
second build configuration, campaign integration or save migration.

All runners are closed. Disposable captures, logs and the isolated review publication
are cleaned after selecting evidence; no retained diagnostic exception. Durable
JSON, code-authored mesh inputs and reproduction commands remain. Rebuild the
separate publication for another walkthrough. Next action: separately plan BR02 against
the accepted layouts and kit contracts. BR01 approval does not authorize the next slice.

Acceptance closeout changed documentation only. `git diff --check` and local link
checks passed; no build, gameplay or test rerun was needed. The empty closeout
artifact run was cleaned. Ten reference images remain under the gallery's documented
retention exception for the BR02 handoff.
