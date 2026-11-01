# Goblin shaman — B05 production appearance

Reference [turnaround-v1](reference/turnaround-v1.png) was accepted on 2026-11-01
with “reference approved”. The tribal caster wears a muted plum-brown ragged robe,
bone/tooth charms and worn leather, and holds its gnarled staff in anatomical right
hand. Its empty left hand casts. Shipped melee goblin and goblin archer masters
were inspected and supplied as the reference's image inputs.

The built-in `image_gen` tool produced the accepted reference, **25 transparent
master sheets / 56 registered poses**, five retained corrections and a native
portrait at `assets/ui/goblin-shaman.png` (public initiative art ID 86). Exact tool
requests, input references and corrections accompany the images as `.prompt.json`.
Superseded sheets/prompts remain in `animation/revisions/`. No placeholder artwork
or runtime image generation is used.

| Clip | Views / frames | Duration (ms) |
| --- | --- | --- |
| Idle | Four / two each | 600 / 600 |
| Walk | Four / two each | 300 / 300 |
| Staff melee | Front / four | 700 / 250 / 250 / 350; strike 950 |
| Casting | Four / two each | 600 / 400; release 600 |
| Hurt | Four / two each | 150 / 150 |
| Death/corpse | Four / four each | 200 / 200 / 200 / 600 |
| Prone | Four / one each | Static |

`ranged_attack` is the existing animation slot for casting, not a hidden bow attack.
The concrete spell timeline uses these measured timings and no arrow flight.

`animation/sheets.json` retains measured gutters, body pivots and local ground rows;
`registration_audit.json` retains source bounds and audited crown-row corrections
where staff highlights resemble skin. Each sheet has one uniform scale. Falling
frames retain that scale, and single prone poses use anatomical length. Logical
800×720 canvases with anchor (400,696) leave room for raised and fallen staff tips;
height .86 includes that margin, while the visible goblin body matches the shipped
goblins. Clip width .95556 stays inside the existing one-cell publication bound.
Occupancy remains one subcell. No silhouettes touch source-image edges.

The ordinary offline extractor, full-resolution transparent-margin crop, prepared
mips/alpha masks, shadows, attachments and picking are reused. Derivatives are
prepared in the build tree, not hand-edited in this source directory:

```sh
cmake-build-release/blobber_prepare_npc_animation \
  assets/npcs/goblin-shaman/animation/sheets.json \
  --filter nearest --resolution 1 --output scratch/blobber-shaman-derivatives
```

Seven-appearance production publication: 375 textures, 329,839,864 mip bytes and
59,422,180 alpha bytes. Existing 512/64 MiB payload ceilings and 352/608 MiB tracked
world CPU/GPU ceilings remain unchanged.

See [design](../../../docs/b05_goblin_shaman_design.md),
[verification](../../../docs/b05_verification.md) and
[gallery](../../../docs/screenshots/b05/index.html). Reference approval does not
constitute completed-slice acceptance.
