# Map icon/readability follow-up

Hidden application-owned captures, 2026-11-04. Fixed cells increased by 50%, from
18 to 27 native pixels. Markers use the built-in imagegen
[source artwork](../../../assets/ui/map/markers-v1.png) and
[exact prompt](../../../assets/ui/map/markers-v1.prompt.json), prepared once into
a padded atlas. Open/closed doors use the same warm stone/brass/wood design;
their leaf position and dark opening distinguish state. Only the party uses teal.

| Capture | Purpose |
| --- | --- |
| [Continued floor map](continued-floor-map.png) | Supplied save with larger cells and generated markers |
| [Reduced continued map](continued-floor-map-1280.png) | 1280×720 presentation |
| [Diagnostic full map](continued-floor-map-full.png) | Same scale/crop with remaining hidden terrain revealed |
| [Closed door](partial-closed-door.png) | Matching closed door; adjoining room stays hidden |
| [Open door](partial-open-door.png) | Matching open door; real LOS maps the adjoining room |
| [All features and both keys](feature-icons-and-keys.png) | Map/legend share artwork; quest keys remain readable |
| [Reduced features](feature-icons-and-keys-1280.png) | Small-size artwork/legend alignment |
| [Oversized map near edge](oversized-near-edge.png) | Party-centered crop, fixed cells and rotated party arrow |

All eight images were inspected. The hidden review passes GL error checks,
temporary/world resource release and zero engine-owned C++ allocations during
warmed map presentation. Original/copied save SHA-256 and continued simulation
digest are unchanged. The separate GPU smoke check passes owned raster-region
selection and party rotation tests. The `map` and `image_preparation` headless
checks pass; their fixtures do not depend on game artwork or saves.

Reproduce with a new output directory and isolated save copy:

```sh
timeout 120 nice -n 15 ionice -c 3 \
  cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets /path/to/new-captures --map-review /path/to/copied-save.json
```

The runner requires an available OpenGL context and creates a hidden window.
No OS input automation or window focus changes were used. Interactive acceptance
remains user review. See [system notes](../../map_revision.md).
