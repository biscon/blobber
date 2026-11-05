# Environment ambient occlusion

Status: implemented and mechanically verified (2026-11-05). Visual tuning awaits
user review. [Comparison captures](screenshots/environment-ao/README.md).

Environment-only SSAO supplies restrained contact shading to world meshes and doors.
It is on by default in city and dungeon; `--ambient-occlusion on|off` controls the
trial. NPCs, all other billboards, decals, ground shadows, effects and sky neither
contribute nor receive AO. Existing creature ground shadows/compositing remain.
No save, publication, movement, presentation-size or persistent-settings changes.

## Passes and interfaces

WorldRenderer owns an AO helper with three shared 1000x564 targets: sampled depth,
AO output and blur scratch. A depth-only branch of the world shader shares static
range selection, UV/cutout and current door transforms with color submission.
The prepass excludes billboard drawing. This extra geometry pass isolates AO from
the creature layer's deliberately leaf-free depth and applies shading before fog.

The helper reconstructs view positions with the inverse of the actual asymmetric
projection and normals from the nearer depth neighbors. It uses 32 fixed hemisphere
samples, fixed repeating rotation, a .30-cell radius, .01-cell bias, .60 strength
and a .30 maximum darkening. A separable five-tap depth/normal-aware blur smooths
noise without crossing discontinuities. Background and offscreen samples add no
occlusion. No temporal state or frame-varying noise. Both quality modes sample the
same high-resolution AO field; normal supersampling resolve remains unchanged.

The surface shader applies the AO factor to textured lighting before fog and the
existing vertex shade. AO enablement invalidates each view's cached image. Existing
world invalidations also regenerate AO; unchanged views reuse completed color.
Diagnostic IDs bypass AO. Latest AO scratch is accessible for diagnostic captures,
not a published view: rendering a staged view can overwrite it.

## Ownership and bounds

Resources are prepared transactionally with existing typed handles/RAII; no frame
allocations, uploads or readbacks. Three existing-format targets consume 12.91 MiB
(below 16 MiB), included in world GPU budgets. Target registry grows from 8 to 12
for presentation, active/staged views and diagnostics; shader capacity remains 8
(five existing shaders plus AO and blur). Existing world GPU ceiling remains.
Preparation failure reports an error without replacing the playable view.
An allocation-free renderer toggle supports tests and future controls. Application
options propagate the default-on choice through normal and city-review launches.
Scratch is prepared even when AO starts disabled, so enabling it cannot allocate.
The helper lives in `src/platform/environment_ao.*`; shared mesh/door submission is
in `world_geometry_draw.cpp`, with the actual projection in `world_projection.hpp`.
The latter also makes projection checks runnable without creating a window.

## Verification

Owned small fixtures cover flat surfaces, corners, object contacts, disconnected
depths, cutouts, near plane/screen edges and native/supersampled output. Check actual
projection reconstruction, deterministic output, creature exclusion, fog/sky,
diagnostic IDs and pre-existing creature/door clipping. Lighting-oracle tests use
AO off explicitly. Test caching, toggles, staged cancel/failure, cleanup and frame
allocation/resource bounds. Use one build configuration, two workers and serial
low-priority checks. Hidden application-owned captures compare dungeon corners,
furniture/chests, doors/creatures and city architecture/fountain, including turns.
No OS input automation, stress suite or unrequested benchmark. Hardware-unavailable
checks are unverified; visual tuning remains provisional pending user review.

## Executed verification

Debug graphical build, 2026-11-05, NVIDIA RTX 3060 / OpenGL 3.3 (driver 615.78.08).
One build job at a time, at most two workers; checks and captures ran serially at
low CPU/I/O priority. All graphical windows were hidden; no desktop input was used.

- Built `blobber`, the focused CPU/GPU targets and `blobber_review_environment_ao`.
- `options`, `world_cpu`, `world_environment`: passed (0.13 seconds combined).
- `blobber_world_ao_gpu_tests --cpu-only`: asymmetric projection and inverse checks
  passed without a window, including near-plane and 16/40-cell far ranges.
- `blobber_world_ao_gpu_tests`: passed. Bounded neutral factors, flat-wall and contact
  shading, deterministic enable/disable, cached frames, identical AO across quality
  modes, staged failure/cancel, zero C++ frame allocations/uploads, opaque creature
  exclusion, identical AO after creature animation/removal, unchanged diagnostic IDs,
  sky exclusion, cutout/turn/near-plane bounds, fully fogged surfaces unchanged,
  resource rollback/cleanup and final GL state.
- Existing `blobber_world_gpu_tests`, `blobber_world_visibility_gpu_tests`,
  `blobber_world_environment_gpu_tests`, `blobber_world_residency_gpu_tests`: passed.
  These retain lighting/resolve oracles (explicitly AO off), creature/door clipping,
  CPU/GPU IDs, environment/foliage integration, asset residency and allocation checks.
- Native-start diagnostics exposed an existing unconditional lookup of absent
  supersampling scratch. The lookup is now guarded and the AO native ID test covers it.
- Exported eight deterministic production poses, each with AO on/off and AO-only:
  fountain, facade, facade mid-turn, closed door, open door with creature, chest,
  chest mid-turn and dungeon corner. Captures include the normal 1920x1080 presenter.
  All on/off differences are confined to the dungeon viewport; see the capture index.
  Capture residency is 9 targets / 7 shaders, below the 12/8 capacities. Shared AO
  adds 13,536,000 bytes (12.91 MiB), within existing overall world budgets.

No benchmark, stress suite or additional build configuration was run. Captures are
static evidence, not interactive motion acceptance. Screen-edge/hidden-geometry
limitations remain inherent to screen-space AO; the fixed pattern is visible in
some AO-only diagnostics but subdued in composed artwork. All billboards (including
non-creature scenery/decorations) remain outside AO in this prototype.

Reproduce captures with a new output directory:

```sh
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_environment_ao \
  cmake-build-debug/assets NEW_OUTPUT
```
