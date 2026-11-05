# SE03 — review evidence

Status: **Ready for review (2026-11-05)**; user acceptance is pending.
See the [implemented design, checks and memory totals](../../../se03_design.md)
and [roadmap](../../../spell_effects_plan.md). SE01–SE02 remain Accepted.

These application-owned captures use production rules and artwork, legal party
preparation/advancement and ordinary commands in a hidden 1920×1080 window. No OS
input or focus control was used. Enemy AI is disabled except for the actual shaman
Bless case. Seed 5 is used except seed 3 for the Web restraint/Fire Bolt ignition
case. Movement follows the ordinary post-victory transition to exploration.

| Evidence | Distinct cases |
| --- | --- |
| [Portrait motion](portraits.gif) | Stabilization without invented HP, condition cleansing, Aid vitality, actual Mirror Image echoes, Shield of Faith and daylight Sanctuary |
| [Condition motion](conditions.gif) | Sleep footprint/motes, one Color Spray fan, Hold Person bands and Bane application/settled marks |
| [Persistent motion](persistent.gif) | Grease, Web placement, one actually burning Web cell, Spiritual Weapon summon/strike, movement endpoints and shaman Bless on actual recipients |
| [Burning Web, native resolution](burning-web-native.png) | Actual burning strands and restrained local light at capture offset 960ms; other Web cells remain unburned |
| [Mirror Image, native resolution](mirror-native.png) | Settled three-image portrait state at 960ms; names, readouts and status icons remain readable |
| [Color Spray, native resolution](prism-native.png) | Footprint-clipped fan at 240ms with creature silhouettes and targeting markers |

GIFs combine labeled crops from seventeen frames sampled every 80ms through
1280ms, then hold the last frame an additional 600ms. The encoder merges identical
frames while preserving their duration. Labels are review annotations,
not game UI. Portrait tiles retain native resolution; world comparisons are reduced
palette previews. The PNGs preserve exact 1920×1080 application output. Offsets
refer to each capture's start: Web ignition and NPC casting use their existing real
resolution delays. Six files retain distinct support, condition, ground, weapon,
NPC and native-resolution comparisons. Superseded/intermediate output was cleaned.

## Reproduce

Build `blobber` and `blobber_world_integration_gpu_tests` to prepare publication 13,
then create an owned run with `python3 tools/agent_artifacts.py create`:

```sh
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets RUN/production --se03-review
```

`RUN/production` must be new or empty. Optional final argument selects one of:
`stabilize`, `cleanse`, `aid`, `mirror`, `faith`, `sanctuary`, `sleep`, `prism`, `hold`,
`bane`, `grease`, `web`, `web-burn`, `weapon`, `weapon-move`, `npc-bless`.
The runner writes frame PNGs, a selected still and `checks.txt` with timing/resource
totals. `--se03-check` without paths runs the small owned GPU fixture at both quality
settings. Clean the run after selecting evidence.

[Built-in ImageGen masters and exact prompts](../../../../assets/effects/se03/README.md)
are durable artwork inputs. Mechanical checks and this visual inspection do not
replace user review of interactive feel or establish cross-device appearance.
