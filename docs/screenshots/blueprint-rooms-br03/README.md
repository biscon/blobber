# BR03 generated-room review

Status: **mechanically verified; user visual/play acceptance pending**, 2026-11-10.
BR01 and BR02 remain accepted. This gallery does not imply BR03 acceptance.

Six selected images from the hidden application-owned review. Native game captures
are 1920×1080; the map sheet is a diagnostic rendering of generated JSON, with
physical furniture blocking marked. No desktop input or focus automation was used.

- [Floor maps](floor-maps.png): seed 5, depths 1/3, both profiles and both frozen
  fallbacks. Every floor retains all four purposes and surrounding ordinary rooms.
- [Kitchen](kitchen.png): normal compact wings, depth 1, rotation 180°.
- [Mead hall](mead-hall.png): normal compact wings, depth 1, rotation 0°.
- [Barracks](barracks.png): normal chamber network, depth 3, rotation 90°; six
  blocking beds, center aisle and middle cross aisle.
- [Armory](armory.png): normal chamber network, depth 3, rotation 0°.
- [Kitchen/hall transition](pair-transition.png): chamber-network fallback,
  depth 3, opened door and low passage ceiling before the taller hall.

The visual harness removes actors to isolate furnishing/architecture review; owned
CPU campaign tests retain real encounters and progression. All twenty captured views
passed GL checks; all four pair passages passed checked in-process movement. Failed
staging preserves the active image, and released floors return resource ownership.

Reproduce with the Debug `blobber_review_blueprint_campaign` target:

```sh
cmake-build-debug/blobber_review_blueprint_campaign assets \
  cmake-build-debug/assets/prepared NEW_AGENT_RUN_DIRECTORY/review
```

Use low CPU/I/O priority and a new owned output directory. The tool emits four JSON
maps and twenty native captures. Only these six selected images are retained; scratch
and superseded captures are cleaned. [Design, checks and limitations](../../br03_blueprint_rooms_design.md).
