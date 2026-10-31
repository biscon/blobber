# R03 review evidence

[Open the comparison gallery](index.html) · [Verification and limits](../../r03_verification.md)

The gallery retains 15 native HUD views, four selected actor poses, and all 192
logical motion images (six movement actions, 16 samples, two quality settings).
Supersampling and native filtered both enlarge the resolved 500×282 world with
nearest sampling. There are no raw-nearest production images.

All **403** exported PNGs match byte for byte between the Debug and Release hidden
capture runs. The full runs additionally exercise all 100 registered actor frames,
R02 geometry cases and failed staging. [Comparison](capture-comparison.json) and
[scripted interaction outcomes](capture-report.json) retain the results. Native HUD
views show formation fitting, a sleeping rear actor's clipped health/status art,
a still-occluding death animation, pickup/drop and key/instant-proxy completion.
The gallery's motion loops deliberately reset at the endpoint; this reset is not
an engine camera transition. Human interactive review is unverified.

```sh
cmake --build --preset release --target blobber -j 4
./cmake-build-release/blobber --development --world-preview
```

F1 resets the formation scene; F2 resets the loot scene; F3 resets the fixture scene.
F5 selects native filtered, F6 supersampling. WASD/QE and the current HUD handle
movement, selection and inventory; F interacts with the faced fixture or at-feet key.
Esc exits outside a modal. This is an isolated development preview: full projectile,
spell-image, animated-door, campaign and result-flow integration remains R04.
Normal game/menu launches and default headless visual mode remain legacy.

For new hidden evidence, choose a new empty output directory:

```sh
./cmake-build-release/blobber --development --capture-world /tmp/NEW-R03-CAPTURES
./cmake-build-release/blobber --development --benchmark-world /tmp/NEW-R03-NATIVE --supersampling off
./cmake-build-release/blobber --development --benchmark-world /tmp/NEW-R03-SS
```

The capture scripts use only application-owned hidden windows and in-process input
or presentation snapshots. No OS input, focus selection or desktop automation.
Benchmark reports are retained in [benchmarks](benchmarks/); world-only timings do
not establish full gameplay/UI performance. See verification for workloads and hardware.
