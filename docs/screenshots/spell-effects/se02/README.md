# SE02 review evidence

Status: **Accepted (2026-11-05)**, including colour corrections and the
[painted-feedback follow-up](../../combat-feedback/README.md).
See the [implementation, checks and memory totals](../../../se02_design.md),
[roadmap](../../../spell_effects_plan.md) and [accepted SE01 references](../README.md).

Fifteen cases use production rules/art, seed 5, legal spell-party presets and real
commands. The incoming-damage case runs a goblin shaman through its actual turn.
Other scenes hold enemy AI still to isolate each spell. Captures come from hidden
application-owned render targets; no desktop input or focus automation.

## Motion

Each GIF samples 0–1,320 ms in 60 ms steps, then holds the final frame for 600 ms
before looping. World previews combine four resized world crops; portraits retain
native-size party crops. Labels/timestamps are review annotations, not game UI.
GIF palette quantization and reduced world size limit fine colour/pixel assessment;
the two PNGs below retain the complete native 1920×1080 output.

- [Projectiles](projectiles.gif): Magic Missile across three recipients and three
  darts onto one recipient; Scorching Ray with one hit and two misses; Guiding Bolt
  travel, impact and quiet mark.
- [Contacts](contacts.gif): saved Sacred Flame, Shocking Grasp, inward Inflict
  Wounds, and Acid Splash with one save and one hit. The two touch spells retain
  their existing 200 ms contact deadline.
- [Cloud, areas and daylight](areas.gif): saved Poison Spray's dense green cloud,
  Thunderwave's broken pressure front across a sparse footprint, Shatter's
  cast-centred fracture and Guiding Bolt against bright city surfaces.
- [Portraits](portraits.gif): Cure Wounds heals Alden by 5 with an emerald-green shimmer; Bless shimmers blue
  across three selected allies; enemy Sacred Flame deals 2 radiant damage to
  Brunna. Small numbers remain, names stay in the log/status tooltips, and
  unaffected portraits remain untouched. Effects finish without looping.

The motion panels retain the earlier world-enemy badges. These have since been
replaced by approved option A; see the current [painted feedback previews](../../combat-feedback/README.md).
White hit flashes remain. The native poison still below includes the new backdrop. A successful save receives short
cosmetic dissipation, without damaging aftermath or a new condition. Poison
retains a dense initial cloud on saves but expires sooner than a damaging hit.

## Native stills

- [Poison Spray at 120 ms](poison-spray-120.png): stronger green and denser cloud,
  following user review; the save outcome remains visible.
- [Healing at 240 ms](portrait-healing-240.png): emerald-green shimmer, distinct
  from radiant damage, with the small +5 readout and portrait borders intact.

The poison and healing motion panels were recaptured after the colour/density
correction; all other panels retain their previous captures. The earlier
Thunderwave/Bless native stills were replaced to keep six selected files.

These six files cover all ten offensive identities, multi-delivery variants,
daylight and the three portrait treatments. Superseded iterations and individual
frames were removed with the owned run. Source masters, cleanup prompts and the
deterministic packer remain [durable artwork inputs](../../../../assets/effects/se02/README.md).

## Reproduce

Build the normal `blobber` target and `blobber_world_integration_gpu_tests` so the
production publication is current. Create an owned run, then run:

```sh
python3 tools/agent_artifacts.py create
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets RUN/production --se02-review
```

Output must be new or empty. An optional final argument selects one of:
`magic-missile`, `missile-single`, `scorching-ray`, `guiding-bolt`, `sacred-flame`,
`shocking-grasp`, `inflict-wounds`, `acid-splash`, `poison-spray`, `thunderwave`,
`shatter`, `radiant-bright`, `portrait-healing`, `portrait-buff`, `portrait-damage`,
`portrait-weapon` (added by the painted-feedback follow-up; not in these older GIFs).
Each sequence contains 23 native frames; `checks.txt` records impact offsets and
resource-accounting totals. Use `--se02-check` without asset/output paths for the
small owned GPU fixture at both quality settings. Clean disposable output after
selecting evidence.

The agent inspected stills and phase filmstrips; interactive desktop and cross-device
verification were not performed. The user explicitly accepted the completed slice.
