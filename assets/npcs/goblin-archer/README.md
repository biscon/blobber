# Goblin archer appearance

The user approved [reference/turnaround-v2.png](reference/turnaround-v2.png)
on 2026-10-26. The muted green hood, short shoulder covering, exposed face/ears,
brown leather, left-hand shortbow and right-shoulder quiver distinguish this
bow-only role from the original melee goblin.

`goblin-archer.json` binds appearance `goblin.archer` to the catalog monster of
the same name. Twenty original imagegen master sheets contain 48 poses:
four directions each of idle, walk, bow draw/release, hurt and death/corpse.
Four additional single-frame prone masters bring the total to 24 masters and
52 poses. Their exact requests are in `animation/sheets/prone-*.prompt.json`;
sleeping and knocked-down living archers share this static pose.
`animation/prompts.json` records the twenty requests and five correction requests;
`animation/revisions/` preserves superseded sheets. The final front bow points
its nocked arrow toward the camera; its release pose contains no flying arrow.
Reference requests live separately in `reference/prompts.json`.

`animation/sheets.json` is the measured registration: one uniform scale per
sheet, body-centered pivots, explicit local ground rows and unclipped canvases.
Death sheets use a 750-pixel row split to preserve the taller top poses.
Prone sheets use `columns: 1`, one registered frame and a 768×536 canvas.
Bow and corpse canvases are wider, with proportional world widths; every canvas
uses the same 512/536 vertical anchor. Collapsing poses are not individually
normalized to standing height. The extractor rejects visible footprint clipping.

Runtime derivatives are generated in the build tree using the shared area/half-size
profile. They are not hand-edited or required in this source directory:

```sh
cmake --build --preset release
cmake-build-release/blobber_prepare_npc_animation assets/npcs/goblin-archer/animation/sheets.json \
  --filter area --resolution 0.5 --output /tmp/blobber-archer-derivatives
```

See [design](../../../docs/goblin_archer_design.md) and
[verification](../../../docs/slice82_verification.md) for timings, validation,
review captures and the next-slice contract.
