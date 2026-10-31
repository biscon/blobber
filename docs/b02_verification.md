# B02 — Bugbear delivery and review

2026-10-31. **Accepted and closed (2026-10-31).** Detailed plan approved with “Implement the plan.”
User accepted the four-direction reference with “reference accepted, looks great”.
User accepted the completed slice with “slice accepted, record it in the plan doc”.
B03 is next for detailed planning; its implementation and campaign integration
remain unstarted/paused.

[Review gallery](screenshots/b02/index.html) · [design](b02_bugbear_design.md) ·
[art provenance](../assets/npcs/bugbear/README.md).

## Delivered behavior

Normal Arena Setup now exposes Bugbear through the existing catalog/appearance
binding. It occupies one subcell, uses ordinary front protection, approach and
reach, and makes one +4 morningstar attack for 2d8+2 piercing. Brute is included
once; ordinary critical damage is 4d8+2. AC 16, fixed HP 27, abilities
15/14/13/8/11/9, 200 XP, one existing shield drop. Humanoid, sleep-susceptible,
nonmetal hide armor, normal damage profiles. No Surprise Attack, javelin,
Nimble Escape, new footprint, hidden protection or campaign guardian assignment.
See the source/adaptation ledger for adopted and omitted SRD content.

Built-in imagegen supplied the accepted turnaround, native initiative portrait,
21 master sheets / 48 poses, and 12 corrections/regenerations. All prompts,
references, superseded masters and measured registration are retained. Four views
of idle/walk/hurt/death/prone plus front melee follow existing timing contracts;
impact remains 950 ms. Corrections address repeated strides, touching figures and
back-prone orientation. Extractor clipping checks passed for every pose. Registered
canvases are 536 px tall with a 512 px ground anchor and 416–560 px clip widths.
The final logical canvas height is .95 cells for raised-weapon room; body width is
.5 cells. Collapse frames remain low, with stable ground registration. This is a
presentation adjustment from the plan's .85-cell starting height, not a new footprint.

## Storage and compatibility

World mip payload capacity is **512 MiB**; tracked world GPU ceiling is **608 MiB**.
CPU world ceiling remains 352 MiB, alpha 64 MiB, and scene/scratch/count limits
remain unchanged. No atlas, packer dependency, streaming, texture compression or
additional prepared-resolution reduction was introduced.

Offline NPC cropping retains visible base texels, two transparent padding texels
where available, original canvas dimensions and integer crop offsets. Logical
anchors, attachments, shadows and edge fitting remain stable; CPU picking and GPU
models share the cropped corners. Prone badges convert stored bounds back to
logical canvas coordinates. Non-NPC textures use identity crops.

Measured publication, including all four shipped appearances:

| Publication | Textures | RGBA mip bytes | Alpha bytes |
| --- | ---: | ---: | ---: |
| Cropped production | 223 | 182,081,476 (173.65 MiB) | 31,686,926 (30.22 MiB) |
| Same art, uncropped comparison | 223 | 331,414,072 (316.06 MiB) | 59,656,848 (56.89 MiB) |

Cropping saves **149,332,596 mip bytes (142.41 MiB)** and **27,969,922 alpha bytes
(26.67 MiB)** on this publication. Before B02, the three-appearance B01 publication
used 263,362,808 mip bytes / 46,895,760 alpha bytes; the four-appearance result is
smaller. These are payload/accounting totals, not measured driver VRAM residency
or total process RSS. Original masters and preparation resolution remain intact.

World publication is format **3**, appearance revision **4**, visibility
**world-visibility-v6**. Old prepared format 2 rejects with rebuild guidance; crop
metadata participates in publication identity. Rules schema stays 10, campaign
schema/simulation 3/4, arena format 1. Updated catalog/definition fingerprints
reject incompatible previous campaign saves without rewriting them. No migration
or campaign population change. Strict traces remain executable/content-sensitive.

## Executed checks

One Release engine configuration, <=2 build workers, one test process and one
intensive job at a time, low CPU/I/O priority. No balance sweep, capacity stress,
benchmark, sanitizer or full configuration matrix.

- Application, headless runner, production bake and affected targets built.
- **16 focused CPU CTests passed in 1.66 s:** bugbear, sprite_crop, world_cpu,
  world_visibility, rules_content, party_content, combat, impact, commands, hud,
  animation, effects, arena, automation, npc_metadata and campaign_save_smoke.
- Crop fixtures cover asymmetric margins/anchors, thin translucent edges, holes,
  below-cutoff pixels, odd/edge-touching identity crops, empty-art rejection,
  malformed/overflow/out-of-bounds crop metadata, and binding-only identity changes.
  All retained prepared base texels/masks match; cropped/uncropped CPU picking is
  pixel-identical over the selected clips, frames and near/far/offset views.
  Logical attachments, shadows and standing/prone badge coordinates match.
- Bugbear fixtures use one legal party target and a copied deterministic RNG to
  verify normal/miss/critical results through the real delayed combat path:
  2d8+2 / 4d8+2, piercing, hidden pending roll, 950 ms impact, HP application and
  allocation-free resolution. Additional checks cover paired-slot protection,
  species portrait selection and exactly-once 200 XP sharing.
- After the explicit retired-format assertion, sprite_crop passed again; the
  affected **world_visual_cli** check passed too (combined 0.18 s). These use owned
  data and no window.
- Generated **world_build_tests.py --single-config** passed clean/no-op/source
  change/touch/missing/add/remove/failure publication checks. Its Ninja fixture is
  a generated CMake project, not another Debug engine build.
- Hidden **world_visibility_gpu_tests** passed CPU/GPU parity for identity and
  cropped fixtures, all clips, quality modes, pivot/direction/near-plane behavior,
  resource cleanup and preparation rollback; reported boundary differences **0**.
  The final cropped-frame allocation/upload guard also passed.
- Hidden **world_integration_gpu_tests** passed lifecycle, allocation, picking,
  effect/door integration and travel rollback; reported edge differences **0**.
- Six retained production-asset traces replayed strictly with **matched**, including
  the uncropped comparison and the existing skeleton scenario. Separate selected
  hidden application-owned captures inspect standing, approach, mixed overlap,
  weapon windup/strike, Sleep/prone badge, lethal flash and final low corpse.

## Arena evidence and review

Seed-5 scenarios and exact event/checkpoint JSON are beside each trace in the
[gallery directory](screenshots/b02/). No installed character library, scenario
folder or user save was changed.

- **bugbear-one:** ordinary level-one party wins in round six. A normal bugbear
  hit deals **11 piercing**, leaving Alden at **1/12 HP**. Later Brunna also reaches
  1 HP. The victory run resets to time zero and full party resources through normal
  arena rematch/reset.
- **bugbear-goblins:** one bugbear plus two melee goblins ends in party defeat.
  Ordinary front protection and approach apply; normal rematch/reset succeeds.
  This is deliberately a dangerous review recipe, not a campaign difficulty claim.
- **bugbear-sleep:** a legal starting snapshot swaps Magic Missile preparation for
  already-learned Sleep. The wizard legally aims beyond the party to exclude allies;
  Sleep applies to the bugbear and selects its separate prone pose/status anchor.
- **bestiary-comparison / bestiary-uncropped:** same scenario, seed, actions and
  selected checkpoints compare all four appearances. The baker's explicit diagnostic
  `--uncropped-npcs` option creates the baseline; normal builds always crop.
- **skeleton-crop:** repeats accepted B01 paired-skeleton actions with current
  identities. Captures retain the accepted size and right-aligned initiative strip.

The gallery's comparison slider and crop-comparison.json retain pixel-difference
measurements. Regenerating mip chains on cropped dimensions changes sample phase;
images are not bit-identical. Selected captures show stable geometry and retained
style/detail, without apparent clipping or resolution loss. This is selected visual
inspection, not proof of identical filtering on every GPU/distance.

Human interactive motion, hover/readability and fun review remain unverified.
No OS input, focus switching or desktop automation was used. Replay captures omit
historical mouse interaction, floating feedback and past combat logs. The source
sheets and gallery cover every pose; dungeon captures are selected views, not an
exhaustive every-direction/diagonal/status matrix. Two-frame strides are restrained
and await human motion review. Full slow campaign persistence and stress suites
were not run because this slice changes content/publication, not saved structures.

For review, run the rebuilt game, open Arena Setup and add one Bugbear; then try
one plus two melee Goblins. Retained scenario JSON may be copied into your existing
scenario folder to reproduce the setup. Review anticipation, scale beside goblins,
control, ground contact, selection, damage danger and rematch. B02 acceptance is recorded below; B03 is next for separate detailed planning.

## Acceptance — 2026-10-31

User: “slice accepted, record it in the plan doc”. B02 is **Accepted and closed**.
This includes the delivered bugbear art/mechanics, arena integration, crop-offset
preparation/rendering and revised memory ceilings. The executed checks and their
limitations above remain unchanged. B03 — Giant rat and Pack Tactics is next for
detailed planning; no B03 implementation or campaign integration is started.
