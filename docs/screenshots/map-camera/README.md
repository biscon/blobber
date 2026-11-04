# Map camera correction

Hidden application-owned captures, 2026-11-04, using owned maps and production art.
No saves were accessed, and no OS input or window-focus automation was used.

- Small padded map: [top-left party](small-padded-top-left.png),
  [bottom-right party](small-padded-bottom-right.png). Terrain remains centered
  at the same coordinates; only the party marker moves.
- Large map: [top left](oversized-near-edge.png), [top right](oversized-top-right.png),
  [bottom left](oversized-bottom-left.png), [bottom right](oversized-bottom-right.png).
  The camera stops at each map edge and fills all 41×28 available whole cells.
- [Large-map center](oversized-center.png): the camera tracks the party away from edges.

All seven captures were inspected. The Debug map regression suite passes (0.01 s).
The hidden map review passes OpenGL, resource-release and warmed-frame C++ allocation
checks. The Debug application was rebuilt. Interactive acceptance remains user review.

Reproduce with a new output directory:

```sh
nice -n 15 ionice -c 3 cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets /tmp/new-map-review --map-review
```

See [design and test coverage](../../map_revision.md#camera-bounds-correction).
