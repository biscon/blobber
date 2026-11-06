# Prop placement and rack size correction

2026-11-06: mechanically verified; user visual acceptance is pending.

- [Rack close view](rack-close.png): half-size geometry, centered on its one collision cell.
- [Rack side view](rack-side.png): the same prepared mesh from two cells away.

These two 1920×1080 application-owned captures use an isolated owned room with
production Underkeep assets. They establish rendering size, not generated placement
quality or interactive acceptance. CPU tests cover actual generated placement and
key/guardian progression. The hidden RTX 3060/OpenGL 3.3 run exported ten views,
checked draw errors, CPU/GPU interior IDs and resource return to the Presenter
baseline; only these two distinct views are retained.

Reproduce with a new disposable agent run, after building the runner:

```sh
nice -n 15 ionice -c 3 cmake --build cmake-build-release --target blobber_world_integration_gpu_tests -j2
# Substitute a new child directory of the run for NEW_CAPTURE_DIRECTORY.
timeout 60 nice -n 15 ionice -c 3 cmake-build-release/blobber_world_integration_gpu_tests \
  cmake-build-release/assets NEW_CAPTURE_DIRECTORY --rack-size-review
```

The window is hidden; input is scripted in-process. No OS input or window focus
control is used. See the [design and verification](../../underkeep_dressing_design.md#room-clearance-and-rack-correction--2026-11-06).
