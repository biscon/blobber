# Underkeep review corrections

Status: **Implemented and verified; awaiting user review**, 2026-11-01.
The design below was recorded before implementation.

Chest instances use a uniform 0.5 scale at scene preparation, about their floor
anchor, for both closed/open triangles and CPU/GPU picking. Preserve source mesh
units, occupied cells, container access, generated records and saves. Moss uses
prepared alpha bounds to put the visible bottom at the local floor, accounting for
transparent source padding. Other decals retain their authored elevation.

Reusable doors use 65% of the old leaf width and 87.5% of the old leaf height.
Widen full-height stone jambs and deepen the lintel to fill the entire old aperture;
retain floor anchoring, fitted door artwork, hinge animation, IDs and collision.
Derive dimensions from the actual local floor/ceiling, including raised floors.

The combat lock appears when a front-diagonal cell contains a body beyond the
horizontal viewport. Cell-only occupiedFront suppresses the automatic facing
handoff, while the UI correctly excludes the invisible body from targeting.
Define horizontal camera setback/tangent in a small shared world-view header.
Perception adds an allocation-free body-axis viewport query using NPC subcells,
without borrowing the renderer. Combat uses it for front occupancy, frontal enemy
eligibility and awareness; candidate melee subslot moves must also stay in view.
Keep cell-based reach, rank protection and line of sight. Once the visible front
empties, use the existing automatic Facing barrier, turn-token invalidation and
action/round preservation. Keep visible diagonal combat; do not unlock manual
movement or charge a turn for the handoff. No GPU visibility readback in simulation.

All changes reuse existing prepared storage; no new per-frame containers or
allocations. Generator and save layout stay unchanged, so current Underkeep saves
remain compatible. Corrected behavior applies on load; strict trace binary identity
rejects recordings from an older executable. Update visibility identity for changed
prepared views. MCP owns separate headless clients and cannot attach to a running
graphical session; no desktop input or process-memory inspection is used.

Verify owned regression fixtures across four headings, diagonal sides/subslots,
front death, relocation, presentation holds and resumed legal attacks. Check both
chest variants and padded moss on a raised floor, door aperture coverage and swing
from both sides. Run focused combat/hybrid/render/allocation/automation/save checks,
and bounded hidden application-owned production captures. Keep one intensive job,
at most two low-priority build workers, and leave interactive acceptance to the user.

## Delivered verification

`world/view.hpp` shares the fixed 84-degree horizontal projection's tangent and
0.4-cell camera setback. `inPartyViewport` evaluates the settled body axis and
leaves a 0.05-cell lateral margin; the existing cell sight/range query still applies.
Front occupancy scans only actual living actors. Enemy eligibility, observation
and vacant melee subslot candidates use the same predicate. This fixes both the
unseen attacker and the suppressed facing handoff without borrowing GPU results.
The existing Facing barrier owns animation, stale tokens and resuming the held turn.

`world_scene` applies chest instance scaling and visible-ink moss anchoring during
preparation. `world_doors` fills the entire local aperture with three stone boxes
and the resized hinged leaf. Source art and generated world fingerprints remain
unchanged; generator/catalog 9/7 and campaign schema/simulation 7/11 are retained.
Visibility is now `world-visibility-v9`; older executable traces reject by binary
identity. Current Underkeep campaign saves can Continue normally.

Release application/headless and the user's Debug application built successfully
with low-priority workers limited to two. Fifteen distinct affected CPU suites passed:
`hybrid`, `combat`, `underkeep_render`, `dungeon_refresh`, `world_visibility`,
`world_environment`, `world_cpu`, `automation_allocations`, `automation`,
`campaign_save`, `ai`, `expanded_spells`, `shaman`, `generation_automation`,
`world_visual_cli`. The initial four-suite selection passed in 0.17 s; the caller
selection passed 11/11 in 7.70 s, including save integration at 5.74 s. After adding
the full-round offscreen attack regression, only `hybrid` was rerun (0.03 s).

Hybrid regressions cover 64 heading/side/subslot/animation combinations, retaining
visible diagonal combat, rejecting offscreen enemy attacks for a full round,
relocation/front death, stale commands, preserved action/round clocks, immediate
legal melee and eventual victory. Eight rendered-target cases verify the surviving
body enters the actual CPU picking list and CommandController accepts it. Automatic
camera changes and visibility updates allocate no engine C++ storage in the owned
guard. Mesh/decal fixtures cover both chest variants, padded moss on flat/raised
floors, door aperture coverage and existing swing/raised-floor integration.

The hidden application-owned review command passed:

```sh
nice -n 15 ionice -c 3 cmake-build-release/blobber_world_integration_gpu_tests \
  cmake-build-release/assets docs/screenshots/underkeep-corrections/first-person \
  --underkeep-corrections-review
```

[Nine reviewed captures](screenshots/underkeep-corrections/README.md) use controlled
small rooms, production art/catalog and checked chest/door actions. Diagnostic
relocation and front death reproduce the diagonal case; automatic facing and the
resumed attack eligibility use ordinary rules. GL errors, CPU/GPU interior picking
agreement, chest/enemy IDs and resource release passed on NVIDIA RTX 3060/OpenGL
3.3. Existing interior-ID tolerance is fewer than 100 nonedge pixels per 500×282
view; no exact cross-device raster claim. Reports of 60,488,291 CPU / 352,749,736
GPU bytes are shared estimates after view release, not process or driver peak.

Logs: `~/.cache/underkeep-corrections-{build,integration-build,hybrid-build,debug-build,gpu}.log`.
`git diff --check` passed. Initial compilation found an unsupported CharacterId
optional equality in a new test; comparing checked character indices fixed it.
No engine verification failure was hidden. No benchmark, balance route, unrelated
configuration test matrix or interactive desktop automation ran. The actual user's
live encounter was not inspected: MCP cannot attach to the graphical process,
and the user closed it before verification. Visual acceptance remains pending.
