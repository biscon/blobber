# R05 verification and review handoff

Status: **Accepted and closed** (2026-10-31).
The user approved the detailed plan with “Implement the plan.” R04/R-B stay accepted;
the user accepted R05 with “slice accepted, record it in the plan doc”.
Acceptance changes no verification results or coverage limitations. At the R05
delivery checkpoint, R06 remained Planned and required its own reviewed plan.
[Design](renderer_environment_design.md) · [Gallery](screenshots/r05/index.html).

Current closure: [R06 and the full renderer roadmap](renderer_plan.md) are Accepted
and closed. Rollout/next-slice notes below record the R05 delivery checkpoint and
are superseded by that closure; R05 evidence and limitations are unchanged.

## Delivered behavior

Immutable maps own eight-byte quantized surface records and bounded decorations.
Shared edge clearance/step and rational supercover sight rules feed player/NPC
movement, BFS, melee, fixture/loot reach and discovery. Floors remain single-layer;
solid cells remain full-height blockers. Ranges and spell footprints remain 2D.

Chunk meshes emit risers/lintels and optional ceilings with actual vertical bounds.
Cameras and actors interpolate elevation; loot, attachments, HUD anchors, effects,
shadows and fitted door frames follow the surface. Ground picking uses a bounded
64-cell ray walk and current opaque depth. Daylight/fog and a procedural sky support
indoor/outdoor adjacency. Prepared alpha cutouts, low benches and urns add decoration.
The authored hub keeps its topology, floor heights, sanctuary and stairs; generated
floors remain flat. Legacy is still default, and rejects non-flat scenes.

`--development --renderer-fixture r05-heights-v1` opens the shared height review;
`--capture-environment DIR` exports hidden captures and a frame-time manifest.
The additive `renderer_fixture` headless scenario supports windowless world replay.
Campaign locations retain full world definitions/fingerprints through travel and
Continue. Schema/simulation **2/2**, appearance **4**, visibility **world-visibility-v3**
and **terrain-rules-v1** explicitly reject incompatible old files/traces.

## Focused final checks

The post-restart development check is deliberately limited to changed behavior:

- Headless Release `world_environment`, `campaign_save_smoke`, `world_visual_cli`:
  **3/3 passed in 0.65 seconds**, serial execution.
- Final Release game/GPU/NPC target build: pass, at two workers and low CPU/I/O priority.
- Hidden generated-art environment GPU check: pass; **zero CPU/GPU ID differences**,
  **zero tracked warmed C++ allocations** and no frame resource uploads; rollback and cleanup pass.
- Optimized NPC animation fixture executable: pass; three identical animated scene
  preparations now share one immutable prepared owner.
- Preset/schema listing, slow-test classification and whitespace checks: pass.
- No repeat benchmark, stress run, complete four-configuration suite or exhaustive
  capture comparison is required for the final local corrections.

The focused CPU suite covers step/clearance rejection, player/actor/BFS agreement,
height and ceiling/corner sight, raised camera/door/effect transforms, ground picks,
invalid world/material references, outward prop faces and warmed allocation guards.
Save smoke covers a nonflat environment through Continue and inactive locations,
ordinary save/continuation/terminal behavior and save UI. The CLI test uses owned
assets and strict elevated-fixture replay without a display.

## Earlier evidence and interruption

Earlier in this implementation, Release graphics 66/66 and headless Debug/Release
47/47 suites passed, along with generated publication incrementality and local MCP
save/resume/replay checks. Generated-art GPU environment checks reported zero CPU/GPU
ID differences and zero tracked warmed C++ allocations; prior world/visibility/
integration GPU regression executables also passed. Capacity checks retained 256
decals, 128 props and the existing geometry limits. These are earlier-run evidence,
not claims of a fresh complete matrix after every correction.

A later Debug run failed writing a large prepared fixture when RAM-backed `/tmp`
filled; it is not counted as a passing full Debug suite. Subsequent verification was
interrupted by the machine restart. Sanitizer completion and a final full-matrix run
are **not claimed**. Temporary logs on `/tmp` were lost at restart; final focused
logs are under `/home/bison/.cache/blobber-r05/`.

The original verification strategy was excessive. The user's resource/testing
instructions supersede its repeated matrix/benchmark recipe. See
[verification workflow](verification_workflow.md): two build workers total, one
intensive job, low CPU/I/O priority, one test process, focused tests during iteration.
Full campaign cases and complete legacy prepared-bank fixtures remain available as
explicit `slow` integration tests. Save smoke is separate; identical animated-art
fixtures are prepared once instead of three times. Fixtures honor TMPDIR. The new
height capacity option no longer runs a repeated timing benchmark.

## Captures and limits

The gallery retains 61 selected application-owned Release PNGs and actual frame
clock/transition data, including both approved quality modes. Static inspection
found and corrected inward prop faces; the captures show the corrected bench/urn.
Covered-walkway exit and quarter-step sequences use actual session updates; other
views are labelled diagnostic poses. They predate final material/reference and
resource-workflow guards that do not change these images. No repeated full
Debug/Release capture matrix was run.

No desktop mouse/keyboard/focus automation was used. Human interactive review,
other GPU/vendor coverage and cross-device pixel equivalence remain unverified.
Allocation claims cover tracked engine C++ allocation, not C allocation or driver
internals. No whole-process memory, GPU timing or 60 Hz guarantee is claimed.

## Next action

R05 is accepted and closed. R06 is eligible for separate planning only, when
requested; R-C remains Planned. Legacy stays default. No new monsters, campaign
balancing, PC07, implementation work or commits are implied.
