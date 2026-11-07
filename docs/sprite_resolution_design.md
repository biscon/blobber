# World sprite preparation resolution

Status: **Implemented and mechanically verified; user visual acceptance pending
(2026-11-07).** [Six review images](screenshots/sprite-resolution/README.md).

The user approved reducing the loot bag, Grease, Web and Spiritual Weapon to
512×512, measuring actor coverage, trialling a half-resolution rat and selecting
appropriate reductions for other actors. Preserve all approved source artwork,
physical dimensions, discrete movement, frame timings and fixed world presentation.
Investigate the reported rat attack specks separately from resolution changes.

## Responsibilities and data

The world baker owns a small optional `assets/npcs/preparation.json` profile mapping
appearance IDs to a uniform resolution multiplier. Absent entries retain 1.0.
Supported profiles are 1, 7/8, 3/4 and 1/2; nonintegral canvas dimensions reject.
Unknown IDs, malformed fields and unsupported values fail before baking. This
file is preparation-only, included in bake dependencies/provenance but excluded
from NPC simulation definitions, so existing campaign saves remain compatible.

Reuse `prepareNpcAnimation` and its existing alpha-weighted area sampler for
reduced actors. Each actor's complete set uses one multiplier: registered canvases,
ground anchors and horizontal pivots scale together; normalized anchors, clip
world dimensions and attachments retain their existing values. Full-resolution
actors preserve the existing nearest-sampled extraction exactly. Final alpha
cropping, premultiplied mip generation and CPU picking masks remain shared with
the existing world pipeline. No runtime resampling, new residency owner, lazy
loading or frame allocations. Keep original masters and registrations unchanged.

A bounded preparation helper downsamples the four standalone sprites using the
existing alpha-weighted sampler, keeping aspect ratio and transparent RGB handling.
Only oversized inputs reduce; the shipped square sources become 512×512. Sampling
scratch stays within existing preparation bounds and is released between images.
Runtime publication format and 1 GiB world GPU ceiling remain unchanged; changed
prepared bytes naturally change the publication fingerprint.

## Measurement and review

An opt-in production review uses the shared camera/projection and cropped actor
geometry to report pixel coverage at 1000×564. Include adjacent near subcells,
the maximum cosmetic melee lunge and resident dialogue framing. Examine actual
animation/frame/direction sizes, with an explicit margin before reducing. Retain
full resolution where a lower profile would undersample the close view. This is
a small content-sizing review, not a performance benchmark or all-map guarantee.

Capture matched baseline/trial rat walking, attack and death views before choosing
the final profile. Then capture selected final near/far actors and dialogue views,
preserving the original full-size masters. Keep at most six comparison images
(twelve while awaiting review). User visual acceptance remains separate from
mechanical checks. Do not regenerate art or silently erase source pixels.

## Validation

Owned small image fixtures cover profile admission, alpha-weighted resizing,
normalized anchors, consistent frame dimensions, source preservation and failures.
Existing sprite crop/visibility tests cover shared picking geometry. Focused hidden
OpenGL checks cover actual presentation, picking and resource release. Reuse the
copied-save travel diagnostic for city/dungeon staging and unchanged save bytes;
run save smoke only, since persistence semantics do not change. Check steady-state
allocation guards in the affected existing GPU/CPU suites. One intensive job at
a time, low CPU/I/O priority, at most two build workers; no desktop input.

Temporary publications, captures and logs belong to the task's agent-run directory.
Record selected profiles, measurements, commands, results and limitations here at
handoff, retain selected review evidence and clean all disposable output.

## Selected settings and measured result

`assets/npcs/preparation.json` is the single production profile. Multipliers apply
to each axis, uniformly across every pose and direction for the appearance.
Loot, Grease, Web and Spiritual Weapon use the baker's fixed 512-pixel maximum;
their shipped square sources are 1254×1254. All source sheets and registrations
remain byte-identical. No source-art cleanup or generation was performed.

The opt-in `blobber_review_actor_resolution` tool uses a small room with the
adjacent actor in its nearest subcell. It measures every stored pose/direction,
including side tails and fallen frames, at depth 0.870 with the maximum 0.28-cell
melee lunge. This is conservative for the archer, which does not melee lunge.
Residents use the production dialogue camera, depth 0.600. Coverage is measured
in the 1000×564 supersampled target; the displayed world target is 500×282.

| Appearance | Resolution | Original max pixels/texel | Final max pixels/texel | Original/final mip bytes |
| --- | ---: | ---: | ---: | ---: |
| Giant rat | 1/2 | 0.357 | 0.714 | 62,042,260 / 15,731,292 |
| Goblin | 7/8 | 0.774 | 0.885 | 32,342,312 / 24,895,328 |
| Goblin archer | 7/8 | 0.774 | 0.885 | 36,793,944 / 28,319,584 |
| Zombie | 7/8 | 0.787 | 0.900 | 29,497,476 / 22,703,724 |
| Goblin shaman | 7/8 | 0.762 | 0.871 | 56,218,652 / 43,228,896 |
| Skeleton | 1 | 0.989 | 0.989 | unchanged |
| Bugbear | 1 | 1.131 | 1.131 | unchanged |
| Innkeeper, Mayor, Boric, Balin | 1 | 1.142 | 1.142 | unchanged |

Rat retains about 1.4 prepared texels per supersampled pixel at the measured
closest view; the other reductions retain at least 1.11. Larger actors and
dialogue residents keep their original detail. Slight differences in cropped
bounds after filtering do not change the registered world canvas or anchors.
Matched near, far, side, attack and death captures showed comparable detail;
all four resident dialogue captures were pixel-identical.

| Payload/accounting | Original | Final |
| --- | ---: | ---: |
| Full publication, GPU mip data | 742.55 MiB | 637.68 MiB |
| Full publication, CPU alpha masks | 79.43 MiB | 59.76 MiB |
| Copied-save city/dungeon overlap, textures | 561.32 MiB | 462.94 MiB |
| Same overlap, tracked world GPU | 589.27 MiB | 490.88 MiB |
| Same overlap, tracked application GPU | 715.99 MiB | 617.60 MiB |

The publication still contains 525 textures. GPU mip data falls by 104.87 MiB
(14.12%); CPU masks fall by 19.67 MiB (24.76%). These are engine payload/accounting
values, not whole-process memory or driver residency measurements. The 1 GiB GPU
limit is unchanged. Only 256 selected texture checksums changed; all other texture
checksums, appearance metadata, timing, attachments and normalized anchors match.
The simulation definition fingerprint is identical and the profile is recorded
in `sources.json` provenance.

## Verification and reproduction

Builds used the existing CMake presets, two workers, `nice -n 15 ionice -c 3`.
The Debug `blobber` executable and final prepared assets were rebuilt successfully.
Focused Release checks were selected for the preparation changes and their
visibility, residency and persistence callers; no unrelated full matrix or stress
benchmark was run.

- `ctest --test-dir cmake-build-release -R '^(image_preparation|sprite_crop)$' -j1
  --output-on-failure`: both pass (0.18 s), including new profile admission,
  alpha-aware edges, no upscaling, 3/4 and 7/8 registration and invalid-canvas checks.
- `ctest --test-dir cmake-build-release -R '^(world_visibility|campaign_save_smoke)$'
  -j1 --output-on-failure`: both pass (1.98 s).
- `blobber_world_visibility_gpu_tests`: hidden GPU/CPU picking parity passes with
  zero boundary differences; cropped animation allocation/upload guards and GPU
  lifecycle checks pass.
- `blobber_world_residency_gpu_tests`: hidden sharing, staging rollback, zero
  frame allocations/I/O/uploads and release checks pass.
- `blobber_review_actor_resolution ASSETS PUBLICATION NEW_OUTPUT`: baseline,
  rat trial and final captures/measurements pass, including pickability, GL errors
  and resource release. `--rat-only` and `--sprites-only` support focused comparisons.
  `ASSETS` was `cmake-build-debug/assets`; the final publication is its
  `prepared/world` subdirectory. Output directories must not already exist.
- `blobber_world_integration_gpu_tests ASSETS NEW_OUTPUT --campaign-travel-review
  COPIED_SAVE`: passes extra blocked forward movement, staging cancellation,
  dungeon entry, return to Sanctuary, canonical state, GL and resource checks.
  The copied save remains byte-identical. The actual user save was never written;
  its SHA-256 remains
  `88f51309aad27b25c6e2cd562af3a80254e12fcb105dab931078115ae75ff567`.

The first diagnostic assumed every actor had a melee attack; the archer correctly
rejected the absent clip. The diagnostic now selects its ranged clip, and the
completed comparisons use that corrected path. The initial rat side capture was
offscreen; its placement was corrected and that comparison replaced before review.

### Rat attack artifact remains unresolved

The original rat's widest crop is a side walk pose (1038×404), set by the main
body/tail silhouette. Front attack crops are roughly 506–519 pixels wide. Earlier
connected-component inspection found a few isolated internal pixels, but they
did not enlarge the crop bounds. Resolution reduction is not an artifact fix.

`blobber_world_integration_gpu_tests ASSETS NEW_OUTPUT --rat-attack-review` reuses
the actual combat/lunge diagnostic with the rat alone. The baseline run captured
21 moments from −160 to +240 ms around impact at 20 ms intervals, seed 5; it was
a miss. Picking, allocation guards, one authoritative impact and GL checks passed.
The detached specks were not reproduced in that sequence, so no source pixels
were removed. A selected six-moment contact sheet is retained for context.
An observed hit/other angle may still reveal the reported artifact; interactive
review, all camera poses and all GPU devices remain unverified.

Six selected comparisons/inspection images were promoted to the linked gallery.
All owned temporary publications, copied saves, raw captures, scripts and logs
were cleaned after the runners closed; no diagnostic-retention exception remains.
