# R02 integration evidence

[Verification and limitations](../../r02_verification.md). These are application-owned
hidden captures; no OS input or foreign-window control was used. Eight selected PNGs
are retained from 81 repeat-identical world captures and the existing legacy menu
capture recipe. They are integration checks, not a new R01 appearance-review project.

- [Generated scene / native UI](0-ss-0-hud.png)
- [Authored scene / native UI](1-ss-40-hud.png)
- Turn midpoint: [native filtered](0-native-88.png), [supersampled](0-ss-88.png)
- Strafe midpoint: [native filtered](1-native-40.png), [supersampled](1-ss-40.png)
- [Active image retained after failed staging](retained-after-failure.png)
- [Normal legacy campaign path](legacy-campaign-floor1.png)

The world preview deliberately has no gameplay actors, picking or effects. Door
blockers are frozen static geometry. The native UI frame is the existing Presenter
layout; empty party/command areas are expected in the development preview.

[Native measurements](benchmarks/native.json) and
[supersampled measurements](benchmarks/supersampled.json) use fresh processes,
120 warmup frames, 600 samples, three trials, separate CPU submission and glFinish
completion. These are world-only costs, not complete game-frame or hardware GPU
query timings. [Decoded PNG hashes](captures.json) identify the retained images.
