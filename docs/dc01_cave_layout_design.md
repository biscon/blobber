# DC01 · Walkable cave geometry

**Accepted and closed by the player on 2026-11-19** after review of the expanded
layouts and revised pool. In the first blockouts the player found depths 4–6 small and linear and reported bright animated lines at the
Galleries pool's cell edges. The original 36×36 canvas was a prototype choice.
[Expansion checkpoint](damp_caves_plan.md) · [Current plans and game views](screenshots/damp-caves-dc01/README.md)

## Exploration revision

| Seed 5 | Earlier walkable terrain cells | Revised cells | Revised extent | Rooms excluding entrance and objective |
| --- | ---: | ---: | --- | ---: |
| The Seep | 174 | 543 | 52×30 | 14 |
| The Drowned Galleries | 157 | 561 | 53×30 | 14 |
| The Brood Vault | 196 | 604 | 53×30 | 14 |

Each reviewed floor has 16 rooms and 18 passages, giving three independent
circuits. The prototype had six rooms, five passages and zero room-to-room
circuits. Cell counts exclude solid rock and blocking scenery. Door and stair
fixtures are counted as their underlying terrain. The baselines were read from
the previous seed plans.

The nine exploration chambers vary in size, ceiling, socket position and
connectivity. A tall chamber with twin stone pillars marks the northern route.
A low gallery marks the southern route. The seed selects passages between
staggered chambers. During preparation the builder removes candidate links in
seeded order while preserving access to every chamber and an alternative route
from the network entrance to the exit. Final-map checks close individual passage
cells and verify that the party can still reach the objective.

The Seep's webbed cache connects the teaching arch back to the northern gallery.
The Galleries keeps both dry approaches through the pool room, then opens onto
the exploration network. The Vault forks in its first chamber. A party can head
north through the survey cache or east to the main junction. All three depths
have a second optional destination marked with a brazier. The objective remains
reachable with both optional destinations cut off.

## Player review

Run from the checkout root. **W/S** step, **A/D** strafe, **Q/E** turn, **F**
opens the faced door, **M** opens the discovery map and **Esc** opens the menu.
Seed 5 starts facing west with the up stair behind the party. Open the door and
follow the bent passage north. Coordinates below match the linked seed plans.

| Command | Walkthrough |
| --- | --- |
| `./review-caves --depth 4 --seed 5` | From the camp, head east through the teaching arch. Turn north for the webbed cache or continue east into the junction. Find the high pillar gallery at (31,5), then return by the low gallery at (31,25). The southeast lookout is a separate detour. |
| `./review-caves --depth 5 --seed 5` | Pass the solo-newt barrel. At (11,15), take the north bank to the far side and come back by the south bank. Stand at (14,13) facing south to check the strip joins. Cross to (13,17), face north and watch a pale ripple beside the rib for four seconds. It should travel right by one eighth of a cell. Head east from the pool to choose an onward route. The lookout is at (47,5) and the objective landing at (47,26). |
| `./review-caves --depth 6 --seed 5` | At the first chamber, choose the north survey route or turn east. Explore the loops before entering the vault. Walk to the two melee stances (45,14) and (49,14), circle to the hatch spaces one cell south of each clutch, then return via a different gallery. |

Add `--fallback` for the fixed fallback or `--check` to walk headlessly.
The launcher builds with the configured Debug toolchain and owns its temporary
output. Player review covers exploration and route readability. Hostile contact,
XP, final art and encounter pacing belong to later slices.

## Generator and artist handoff

`generateCave(seed, depth, forceFallback)` returns an owned immutable Map,
FloorLayout, typed reservation cells and generation diagnostics. It uses actual
chamber membership, outward-facing sockets, ordered passage cells and protected
landings from the [shared layout design](procedural_layout_design.md). A floor's
review launcher borrows this result for one ordinary session using the
[standalone walkthrough precedent](br01_blueprint_rooms_design.md). Campaign
generation, save admission and the floor-three ending retain their existing owners.

These depth profiles use rectangular footprints. Shared validation checks the
area against the backing arrays before indexing either axis. Current bounded
storage holds 1,600 cells, 16 rooms, 24 connections and 1,600 passage indices.
The 36-cell Underkeep profile is local to stone generation. A candidate can
fail at placement, structural, fixture or final movement validation. Sixteen
attempts precede a seed-independent fallback per depth, checked by the same
validators. A failed fallback reports failure before session construction.
Preparation uses an 11,200-byte shared structural workspace. Route checks use a
3,200-byte index queue and 200-byte masks. The small nine-node network uses
12 candidate edges. Generation, model binding and exports run outside frame processing.

The three SVG plans give global occupied cells, named anchors, sockets and their
facings. Model pivots use cell-floor centers. Keep the entry dogleg, level door
approaches and the 3×3 NPC exclusion around each landing. Thresholds have four
quarter-cell ticks of headroom. The tall pillar gallery has nine ticks, the low
gallery five and the guardian approach six. Overhead art should leave one full
cell of vertical clearance above walking lanes.

- **Seep.** The bed occupies one cell off the camp aisle. The crate is walkable.
  Both wall webs need supported corners. Passing the teaching threshold precedes
  access to either the future spider or the webbed cache.
- **Galleries.** The 9×5 room enters west and exits east. The 5×3 pool surrounds
  a solid 3×1 east/west rib. Two 5×1 bank strips and two 1×1 end strips reserve
  all twelve water cells for movement and NPC occupancy. The rock blocks sight.
  Water permits sight, and the walkable shoulders connect both banks. Both contacts,
  the rope and the solo-newt cell are reserved for dressing and encounter placement.
- **Vault.** Enter the 9×9 vault from the north. Two centered, 0.88-cell-wide
  clutch proxies stand two cells either side of the mother. Reserve each cell
  immediately north for melee and each cell immediately south for a hatchling.
  The vault and the divided pool chamber both have eight ticks of ceiling clearance.

The former one-cell water meshes restarted the fountain atlas at each boundary.
Each animation frame contains a painted rim. The first correction mapped all four
strips into a single interior portion of each frame, with matching interpolated
UV coordinates where strips meet. The water stays 0.07 cell below the bank.
The town fountain retains its own mesh and material mapping.

## Pool density and current revision

The player confirmed that the expanded layouts suit the intended exploration and
the cell-edge seams are gone. From the south bank facing north, the water looked
coarse and slid back and forth. The seam fix spread 70% of one 512-pixel fountain
frame across five by three cells, about 72 by 119 source texels per cell. The wall
uses 256 by 256 per cell. The fountain frame offsets oscillate between −4 and +4
source pixels on a one-second cycle.

The revised blockout takes a seamless 256-square tile from the original water
painting before the fountain rim, stream strip and oscillating frames were added.
The review loader resamples the painting to 288×288, overlaps 32 pixels at each
edge, and prepares nine RGBA mip levels (349,524 bytes) once. The review
publication owns that data. Its GPU material uses the ordinary scene texture lease.
All four strips sample world X/Z once per cell.
The renderer translates that field east by one cell every 32 seconds and uses
periodic wrapping and linear filtering for subtexel motion. Visible current uses
the existing color cache with a separate scrolling phase. Depth, picking and
collision depend only on the fixed basin geometry.

The prepared Underkeep wall is 256×256 at world scale 1. A hidden OpenGL walkthrough
captured the fork and both banks, including 33 north-facing samples spaced
125 ms apart. On framebuffer row 785 the painted pattern repeated every 687
screen pixels. Matching RGB strips between the first and last frame measured
87 pixels of rightward travel in four seconds, approximately one eighth of a cell.
All 32 consecutive matches moved right, including the transition through the
32-second wrap. The scene released its GPU resources and the revised plan
matched the prior seed-5 plan byte for byte. The gallery retains eight plans or
stills and one four-second clip. The clip samples motion at 8 fps. The in-game
current updates at up to one step per 16 ms.

Focused Debug checks for this revision used `ctest --test-dir cmake-build-debug
-R '^(cave_layout|world_residency)$' -j1 --output-on-failure`. Both passed in
0.36 seconds. The hidden `cmake-build-debug/blobber_architecture_gpu_tests`
also passed its existing fountain atlas, cache, picking, allocation and teardown
checks.

## Verification and limitations

Used the Debug tree's cached CLion CMake and Ninja, at most two build workers,
one test process and one intensive job at a time. Seeds 5 and 6 cover repeatable
layouts and distinct room graphs. Two forced seeds per depth reconstruct the
same fallback. The tests also cover oversized rectangular input, blocked corners,
door and landing clearance, prohibited pool entry, both independently usable
banks, optional bypass, camp order and distinct clutch and hatch spaces. An
owned art fixture checks the twelve-cell footprint, the binding of every strip
to the current, native one-texel features, both periodic joins and the mip chain.

```sh
cmake_dir=/home/bison/clion/bin/cmake/linux/x64/bin
nice -n 15 ionice -c 3 "$cmake_dir/cmake" --build --preset debug --parallel 2 \
  --target blobber_review_caves blobber_cave_tests blobber_generation_tests
nice -n 15 ionice -c 3 "$cmake_dir/ctest" --test-dir cmake-build-debug \
  -R '^(cave_layout|generation)$' -j1 --output-on-failure
```

Both suites passed in 3.70 s. The final cave suite with the added water regression
passed in 0.06 s. For each depth, `./review-caves --check --depth DEPTH --seed 5
--fallback` walked both galleries, the lookout, the cache, the objective and the
return landing. The walker also returned to the network entrance with an outbound
passage excluded. All three revised normal floors passed on the first candidate.
The three forced floors have 529, 514 and 563 walkable terrain cells respectively.

```sh
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_caves capture DEPTH 5 \
  NEW_OUTPUT assets cmake-build-debug/assets/prepared
```

Three hidden OpenGL runs exercised normal game commands and discovery, checked
GL errors, released scene assets and returned renderer resources to the presenter
baseline. The Galleries run photographed the south-facing bank at all eight
animation phases. Inspection showed continuous ripples without the repeated
cell-edge lines. The final plans reproduced byte for byte after rebuilding.
These eight phases belong to the earlier fountain-based correction. The player
confirmed the seams had disappeared and liked the larger, less linear layouts.
The native discovery map still uses floor and wall symbols.

The original DC01 extraction also passed the fixture, cave and generation suites
and preserved Underkeep seed-5 fingerprints 4925727464932174507 (normal depth 1)
and 6714332330039686772 (forced depth 3). The player rejected the earlier cave
layouts. Their former gallery has been replaced.
