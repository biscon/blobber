# Stone dungeon distance lighting trials

Status: **C / Early accepted, implemented and verified (2026-11-05)**.
[Comparison viewer](screenshots/dungeon-lighting/index.html) and
[capture notes](screenshots/dungeon-lighting/README.md).

The existing camera-depth attenuation is `1 / (1 + .115*d + .015*d*d)`.
Keep its face shading, AO, fog, daylight, flash behavior and prepared-image pipeline.
Only the `underkeep` asset pack receives a trial lighting distance:

| Preset | Lighting distance | Intent |
| --- | --- | --- |
| Current (legacy reference) | `d` | Unchanged reference |
| A / Gentle | `d + .5*max(d-2,0)` | Preserve nearest two cells; moderate distant darkness |
| B / Strong | `d + max(d-2,0)` | Preserve nearest two cells; stronger distant darkness |
| C / Early (selected default) | `1.5*d` | Darken from the camera, including nearby objects |

Renderer-owned selection is an enum with a setter; no save, asset publication or
settings UI changes. A small graphics-independent profile helper resolves the enum
against the view's asset pack and supplies an onset and extra slope. Both surface
and billboard shaders consume these parameters, including normally lit effects.
Fog and geometry continue to use real camera depth. Shader uniforms are cached at
preparation; no new targets, uploads or frame allocations are needed. Each prepared
view records its effective preset, so color invalidates on a real lighting change
while unrelated packs retain cached images. Active and staged views resolve their
own pack independently. The user selected C; Early is the renderer default
while retaining Current as the neutral profile for other packs and review baseline.

## Accepted selection — 2026-11-05

The user chose **C / Early**. Change only the renderer's initial selection; retain
the approved attenuation formula and pack gate. Verify that a default stone view
matches explicitly selected Early and differs from the legacy reference. Existing
comparison tests must explicitly request Current before recording their baseline.
Rebuild the game and focused lighting GPU target, then run only that GPU suite.
Retain four accepted/reference images (two poses each), prune the unselected A/B
captures and update the viewer/index. No new captures or broader test matrix needed.

## Review and verification strategy

A hidden-window review executable uses production art and the normal 1920×1080
presenter. Fixed corridor and furnished-room poses produce baseline/A/B/C pairs
with frozen animation and AO enabled. The original eight-image gallery supported user choice; four accepted/reference
captures remain after selection. These are review evidence, not test fixtures.

Owned generated test fixtures exercise the two-cell boundary, monotonic attenuation,
surface and creature shading, unaffected packs, cache invalidation, diagnostic IDs,
staging/cancel, frame allocations and resource lifetime. CPU profile checks run
without a window; hidden GPU checks cover both presentation quality modes. Use
focused existing world renderer integration checks, one bounded low-priority build
at a time and at most two build workers. No benchmark or desktop input automation.

Disposable outputs belong to the task's agent-artifact run. Promote selected review
evidence, record commands/results here and clean the run before handoff.

## Implementation and verification — 2026-11-05

The renderer exposes `dungeonLighting(DungeonLighting)` and a matching getter.
The initial trial used `Current` as default; the accepted selection promotes `Early`.
Header-only profile resolution lives in
`src/render/dungeon_lighting.hpp`; a pair of cached uniform locations supplies
onset/slope to the existing shaders. Each view records its effective enum in the
prepared-color cache. The asset publication, saves and gameplay UI are unchanged.

One Debug build, with `nice -n 15 ionice -c 3` and at most two workers, built the
game, review tool, new CPU/GPU targets and existing world/AO GPU checks. No asset
rebake, benchmark, stress test or extra build configuration was required.

Passed checks:

- `ctest --test-dir cmake-build-debug -R '^dungeon_lighting$' --output-on-failure -j 1`:
  headless curve monotonicity, exact two-cell boundary, continuous transition,
  pack isolation and allocation-free profile resolution.
- `cmake-build-debug/blobber_dungeon_lighting_gpu_tests`: generated owned fixtures;
  wall lighting oracle at .9/1.9/2.9/5.9-cell depths in both quality modes,
  creature attenuation, unchanged visibility/IDs, cache reuse/invalidation,
  active/staged pack isolation and cancellation, zero engine C++ allocations or
  GPU uploads on preset switches, complete resource release and valid GL state.
- `cmake-build-debug/blobber_world_gpu_tests`: existing rendering/resolve,
  prepared-image caching, allocation and resource-lifetime checks.
- `cmake-build-debug/blobber_world_ao_gpu_tests`: existing environment AO integration.
- `blobber_review_dungeon_lighting`: eight production-art captures at fixed time
  and poses; production sanctuary city and inn output pixel-identical across all
  four presets. All six trial-vs-current differences are confined to the world
  viewport, with no brighter pixels. A/B leave the foreground unchanged; C changes
  the near field. All eight retained images were visually inspected.

The initial sandboxed GPU launch could not access X11. The authorized retry used
hidden application windows with display access, no focus control or OS input.
GPU checks/captures ran serially with `OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1
LP_NUM_THREADS=2`, low CPU/I/O priority. The owned inn test fixture needed its
hearth texture even for a plain room; the complete generated fixture passes.

Reproduce the review after creating a fresh artifact run:

```sh
nice -n 15 ionice -c 3 cmake --build cmake-build-debug \
  --target blobber_review_dungeon_lighting -j 2
OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2 \
  nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_dungeon_lighting \
  cmake-build-debug/assets RUN_DIRECTORY/captures
```

Four original screenshots are retained after acceptance of C. The local
HTML viewer switches between those unmodified captures and links full-resolution
images. Temporary logs and duplicate captures are cleaned with the artifact
helper. No failure diagnostics need retention. The user accepted the mood of C;
interactive movement review was not performed.

Default-promotion verification: rebuilt `blobber` and
`blobber_dungeon_lighting_gpu_tests` in Debug with two low-priority workers. The
focused hidden OpenGL suite passes, including the new default-vs-explicit-C image
comparison, legacy difference and existing pack-isolation checks. No broader
suites or captures were rerun for this default-only change. Viewer JavaScript
syntax and local image/document links pass checks. Four unselected A/B images
were removed; four original legacy/C images remain. Task build/test logs were
cleaned with the artifact helper; no diagnostics need retention.
