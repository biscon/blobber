# Floor map revision review

Application-owned hidden captures, 2026-11-04. The map fits the dungeon viewport,
retains fixed 18-pixel cells at native 1920×1080, and uses shared pictograms.
The [approved mockup](../../mockups/floor-map-approved.png) guides layout only.

| Capture | Purpose |
| --- | --- |
| [Continued floor map](continued-floor-map.png) | Current save, completed room walls, normal player knowledge |
| [Continued map at 1280×720](continued-floor-map-1280.png) | Reduced presentation legibility |
| [Full-map comparison](continued-floor-map-full.png) | Diagnostic view of terrain still hidden from the saved party |
| [Closed doorway](partial-closed-door.png) | Seen threshold does not reveal the adjoining room |
| [Opened doorway](partial-open-door.png) | Real LOS after opening maps the adjoining room |
| [Features and keys](feature-icons-and-keys.png) | All legend/feature symbols and both held quest keys |
| [Features at 1280×720](feature-icons-and-keys-1280.png) | Small symbol scaling and legend alignment |
| [Oversized map near an edge](oversized-near-edge.png) | Fixed-size crop keeps the party centered; outside-map cells stay dark |

The saved level contains two rooms with no recorded interior line of sight,
beyond the closed door on the left. They remain hidden intentionally. The
fully explored owned CPU fixture has identical discovered/full-map terrain.

The opt-in runner uses production UI art but owned synthetic map cases; normal
CPU tests do not depend on production assets or this save. Hidden GPU checks
passed resource release, GL error checks, unchanged continued simulation/save
bytes, and zero engine-owned C++ heap allocations during warmed map frames.
The original user save was never written; both its SHA-256 and the copied slot's
SHA-256 remained unchanged.

Reproduce using a new output directory and an isolated copy of a current save:

```sh
timeout 120 nice -n 15 ionice -c 3 \
  cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets /path/to/new-captures --map-review /path/to/copied-save.json
```

Omit the copied-save argument to capture only the synthetic cases. The runner
requires an available OpenGL/X11 context and always creates a hidden window.
Interactive input and final visual acceptance remain user review.
