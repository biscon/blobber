# Architectural ceiling review

2026-11-09: implemented and mechanically verified; **user visual acceptance pending**.
Six native 1920×1080 captures from the hidden production renderer, generator/templates
13/9. Seeds 0 (CompactWings) and 5 (ChamberNetwork), depth 3. Actors are removed only
from the owned review sessions to isolate architecture; this is not a combat playthrough.

| Capture | Case |
| --- | --- |
| [Small room](compact_wings-ceiling-5.png) | 1.25× ceiling, ordinary door proportions |
| [Narrow hall](compact_wings-ceiling-6.png) | 1.5× ceiling; narrow shape caps height |
| [Large chamber](compact_wings-ceiling-7.png) | 1.75× ceiling, pilasters reach ceiling |
| [Pillared chamber](compact_wings-ceiling-8.png) | 2× ceiling, full-height central pillar |
| [Open threshold](chamber_network-door-open.png) | Low doorway opens onto a taller chamber |
| [Stair approach](chamber_network-stairs.png) | Protected landing region keeps original height |

Reproduce with the Debug `blobber_world_integration_gpu_tests ASSET_ROOT NEW_OUTPUT
--ceiling-review` selector. ASSET_ROOT must contain current source assets and the Debug
`assets/prepared/` outputs. This run used a disposable asset view with symlinks to
checkout asset children and the Debug prepared directory; all links were removed at
cleanup. Put NEW_OUTPUT under a fresh `tools/agent_artifacts.py create` run.
Use `nice -n 15 ionice -c 3`; hidden display access is needed, never OS input.

The successful review exported twelve images and checked every room tier, both
profiles' stair regions, open/closed doors and thresholds, GL errors, interior CPU/GPU
picking agreement and return to the presenter's GPU resource baseline. Six selected
images remain; redundant captures and failed-run diagnostics were cleaned.
Commands, test outcomes and limitations are recorded in the
[design](../../procedural_layout_design.md#architectural-ceilings--2026-11-09).
