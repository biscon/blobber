# SE03 prepared spell artwork

Built-in ImageGen generated the two transparent painted-pixel masters in `source/`.
The exact prompts are retained alongside each master. No CLI/API fallback was used.

- [Support master](source/support-master.png) and [exact prompt](source/support-prompt.txt).
- [Ground master](source/ground-master.png) and [exact prompt](source/ground-prompt.txt).

Rows use eight frames with a stable grid; no original icon/shape is procedurally drawn.

Regenerate from the repository root:

```sh
python3 tools/prepare_se03_effects.py
```

The approved offline packer registers each cell to 112×112 texels inside a padded
128×128 tile, rejects low-alpha generation residue using the accepted SE02 threshold,
and derives emission from authored bright cores. It does not create new silhouettes.
The portrait sheet reuses authored support and recovery rows. Runtime is prepared
textures only; no image generation/packing occurs during gameplay.

`support`: boon, curse, sleep, prism, restraint, faith, armour, sanctuary.
`ground`: grease spread, web unfurl, burning web, summon, movement, strike,
stabilization, cleanse.
`portraits`: stabilization, cleanse, boon, curse, faith, armour, sanctuary, sleep.

Existing Grease/Web/Spiritual Weapon artwork and SE01–SE02 masters are unchanged.
See [SE03 design](../../../docs/se03_design.md) for verification and acceptance status.
