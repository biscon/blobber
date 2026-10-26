# Slice 8.2 verification and handoff

Status: **Accepted and closed**, 2026-10-26. Implementation plan and
[turnaround-v2 reference](../assets/npcs/goblin-archer/reference/turnaround-v2.png)
were approved separately. The user's “yes it looks great, approved :)” accepted
the reference before animation production. The user subsequently accepted the
completed slice with “slice accepted, close it up” on 2026-10-26. Verification
limitations remain recorded below. Slice 8.3 remains Planned.

## Delivered result

- Appearance `goblin.archer` has twenty master sheets / 48 poses: idle, walk,
  ranged attack, hurt and death in front/back/left/right views. Hood, exposed ears,
  left-hand bow and right-shoulder quiver distinguish it from the melee goblin.
- The two hybrid-demo archers use this appearance and its default name-based
  rules binding. Regular `goblin` has only its scimitar and no production bow clip.
  The original artwork, full sheet registration, extracted frames and bow masters
  are preserved; `sheets-melee.json` selects its production subset.
- Both roles use the accepted area/half-size preparation profile and existing
  immutable banks, animator and actor interfaces. No engine schema, gameplay
  timing, camera, movement, AI policy or public API changed.
- `src/app/archer_captures.cpp` owns the new diagnostics. Both focused preparation
  and hybrid commands invoke it. Tests use generated fixtures, not shipped art.

See [design](goblin_archer_design.md) and the
[asset README](../assets/npcs/goblin-archer/README.md). The masters, exact prompts,
five superseded corrections and measured splits/pivots/grounds are tracked under
`assets/npcs/goblin-archer/`. Runtime derivatives stay in the build tree.

## Verification

Executed from the repository root:

```sh
cmake --build --preset debug -j4
cmake --build --preset release -j4
ctest --preset debug
ctest --preset release
cmake-build-release/blobber --check-prepared
cmake --build --preset release --target blobber_gpu_smoke -j4
cmake-build-release/blobber_gpu_smoke
cmake-build-release/blobber --capture-preparation /tmp/blobber-82-captures
cmake-build-release/blobber_bake_assets assets /tmp/blobber-82-repro
cmake --build --preset release --target blobber_bake_runtime_assets
```

Debug **22/22 passed** (101.50 s), Release **22/22 passed** (11.43 s).
The focused changes cover name binding with reordered catalogs, missing second
bindings, explicit overrides, distinct clip tables, rejection of swapped timing
tables, loaded/direct pixel and picking equivalence, all four bow directions and
the 599/600/999/1000 ms boundaries, travel interrupted by hurt, corpse expiry and
occupancy. Existing melee-only, ranged-only and mixed-weapon AI tests still pass.
The allocation guard exercises both loaded appearances on first/subsequent frames
through attack, travel, hurt, death and body picking, with **zero tracked engine
frame allocations**. An early fixture attempt paired two-appearance rendering
with one-appearance actor worlds; its expected-count failure was corrected by
giving the two-role fixture its own renderer, without weakening the assertions.

The existing sanitizer build was rebuilt with the affected test sources:

```sh
cmake --build /tmp/blobber-81-sanitizers --target \
  blobber_prepared_assets_tests blobber_actors_tests blobber_npc_animation_tests -j4
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/blobber-81-sanitizers \
  -R '^(prepared_assets|actors|npc_animation)$' --output-on-failure
```

ASan/UBSan **3/3 passed** (170.56 s). Leak detection remains
unverified: the prior slice established the environment's tracing/ptrace limitation;
it is explicitly disabled here. Third-party raylib is not sanitizer-instrumented.

Hidden GPU lifecycle verification passed, including render/readback and resource
cleanup. The initial sandboxed capture could not open display `:0`; the authorized
hidden run with display access succeeded. No OS input or focus automation was used.

The preparation command completed **498 PNG captures**, including **102 archer
images**. The new suite samples every authored pose in four directions, near/far
idle, mid-strafe, five travel times per direction, all four six-slot mixed-role
front bands, bow release and wall occlusion. It asserts pose/frame selection,
living target counts and corpse expiration. `archer-trace.tsv` records type,
clip, frame, visibility and picking for each sample. All six front-band actors
remain pickable during idle and attacks; walls suppress hidden targets.

All twenty source sheets were visually inspected during authoring. Corrections
fixed left-profile hurt handedness, rear bow direction, front end-on arrow and
a duplicated front-death feather cluster. Extraction checked every visible alpha
footprint for clipping; the wide side bow required a pivot adjustment. Selected
native and reduced captures were inspected across all four directions, reactions,
bow poses, walking/travel, strafe, distance and mixed-role edges. Feet remain on
the registered ground, corpses retain collapse height, and shadows track the
displayed bodies. The user has accepted the slice; no agent-driven interactive
desktop review was performed.

A clean separate bake reproduced `scene.bin`, `npcs.bin` and `metadata.json`
byte-for-byte. The unchanged bake target reported no work. The surface bank is
also byte-identical to slice 8.1. The final NPC-bank SHA-256 is
`3a4f97faf8293c17a8b4070c1403ab2ada141284996953061284f58f09c19726`.
Original tracked PNGs are unchanged. Compiler logs have no warnings and
`git diff --check` passes.

A runtime-only tree in `/tmp/blobber-82-runtime-only` contained the two NPC
definition JSON files, level metadata, prepared banks, preparation profile,
maze, rules, UI and fonts. It contained **zero NPC PNGs and no texture directory**:

```sh
cmake-build-release/blobber --assets /tmp/blobber-82-runtime-only --check-prepared
cmake-build-release/blobber --assets /tmp/blobber-82-runtime-only \
  --capture-hybrid /tmp/blobber-82-runtime-captures
```

Both passed. All **256 runtime-only captures** match their normal-run counterparts
byte-for-byte, including all 102 archer images.

## Memory and preparation

The actual two-role production data, not duplicated placeholder art, measures:

| Allocation/accounting category | Bytes |
| --- | ---: |
| Archer selected PNG masters on disk (20) | 38,090,372 |
| Archer half-size PNG derivatives on disk (48) | 3,325,616 |
| Both roles' decoded derivative source pixels | 26,071,040 |
| NPC stationary prepared pixels | 57,716,268 |
| NPC travel prepared pixels | 8,785,480 |
| NPC shadow pixels | 25,594 |
| NPC metadata/capacity | 108,328 |
| NPC resident bank | 66,635,670 |
| Runtime surface resident bank | 114,664,792 |
| **Combined runtime resident banks** | **181,300,462 (172.90 MiB)** |
| Combined bake resident accounting | 189,242,838 |
| Derivative extraction tracked high water | 39,389,536 |
| Source decode tracked high water | 26,482,688 |
| NPC raster preparation tracked high water | 96,026,262 |
| **Overall bake tracked loading high water** | **126,080,672** |

Separate resident and tracked-loading limits remain **536,870,912 bytes each**.
Bake/runtime surface accounting differs because baked containers retain construction
capacity. The published binaries are 66,634,954 bytes (NPC) and 114,663,460 bytes
(surface). This accounting excludes allocator, codec, JSON/file-buffer, GPU/driver
and other game-state overhead as documented in 8.1.

A standalone Release bake took **4.91 s**, with process peak RSS **126,872 KiB**;
runtime-only `--check-prepared` took **0.206 s**, load report **197 ms**, process
peak RSS **241,396 KiB**. RSS was observed separately through Python child-process
`resource.getrusage`; it is not an assertion about tracked bytes. These are single
filesystem-cached observations, not performance guarantees.

## Review and reproduction

Selected durable screenshots are saved in **`docs/screenshots/slice82/`**:

- [Both roles together, 720p](screenshots/slice82/archer-roles-front-band-0-720.png)
- [Mixed-role release poses, 720p](screenshots/slice82/archer-roles-release-1-720.png)
- [Archer idle, native](screenshots/slice82/archer-idle-front-0.png) and
  [720p](screenshots/slice82/archer-idle-front-0-720.png)
- [Front draw](screenshots/slice82/archer-bow-front-0.png) and
  [release](screenshots/slice82/archer-bow-front-1.png)
- [Rear draw](screenshots/slice82/archer-bow-back-0.png),
  [left draw](screenshots/slice82/archer-bow-left-0.png),
  [right release](screenshots/slice82/archer-bow-right-1.png)
- [Corpse](screenshots/slice82/archer-death-front-3.png),
  [travel](screenshots/slice82/archer-travel-back-300.png),
  [strafe](screenshots/slice82/archer-idle-right-strafe-3.png),
  [far view](screenshots/slice82/archer-idle-left-far.png)

The complete disposable set is `/tmp/blobber-82-captures`; regenerate it with
the preparation capture command above. The traces are also preserved beside the
selected screenshots. To review live behavior, run:

```sh
cmake-build-release/blobber --hybrid-demo --seed 5
```

Watch the two hooded archers hold clear shooting positions, draw/release bows,
and create distance at contact while regular goblins close for melee. Use WASD
to move, Q/E to turn, and the existing party commands/target clicking to check
hurt and death. Inspect equipment continuity, proportions, ground placement and
role readability. Interactive desktop review is left to the user.

For separate registered-frame inspection:

```sh
cmake-build-release/blobber_prepare_npc_animation assets/npcs/goblin-archer/animation/sheets.json \
  --filter area --resolution 0.5 --output /tmp/blobber-82-derivatives
```

## Limitations and next-agent contract

The pre-existing broad `--capture` victory assertion failure from 8.1 remains a
known limitation; that command was not rerun or weakened here. Focused preparation
and hybrid captures passed. The historical `escape` capture now uses the accurate
name `wounded-melee`, since the production melee goblin no longer has a bow.

Slice 8.2 is accepted and closed. Plan 8.3 separately in a clean context and
obtain approval of its plan before implementation. Preserve
appearance IDs `goblin` / `goblin.archer`, shared profile C, typed lifetime ownership,
the front-band geometry/edge presentation and mixed-weapon fixture coverage.
Archer idle is 600/600 ms; walk 300/300; bow 600/400; hurt 150/150;
death 200/200/200/600. **The release boundary is 600 ms** and the release image
contains no arrow. Current damage, death and occupancy consequences still happen
immediately at attack calculation. The forthcoming impact-time slice must change
that deliberately; this artwork does not defer simulation events or add projectiles.
