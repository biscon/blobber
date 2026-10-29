# Prepared ground effect art

`grease.png` is a transparent, top-down oily pool generated with the imagegen
skill for the slice 12.1 visual review correction. `grease.prompt.json` preserves
the exact request and generation provenance. Keep this master unmodified;
projection, lighting and alpha blending belong to offline scene preparation.

The baker fingerprints this directory and requires the pool for production.
Four heading banks in each prepared view/strafe phase anchor it to world cells;
wall depth clips it and actors/loot draw afterward. Runtime performs no image
preparation. See the [design](../../docs/spell_visual_review_design.md) and
[verification](../../docs/slice121_verification.md).
