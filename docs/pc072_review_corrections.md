# PC07.2 review corrections

Status: **Accepted and closed** as part of PC07.2 (2026-11-01). User: “slice accepted,
record it in the plan doc”. The review plan was approved earlier the same day.
This does not authorize PC08. Keep existing rooms,
encounters, placements and XP; quiet rooms remain for now.

The later [variety and controls correction](pc072_variety_controls.md) supersedes
the fixed room graphs, starting kit count, End-Turn-only rogue behaviour and version
identities below. Other delivered corrections and historical evidence remain.

## Gameplay and bounded data

Floors 2–3 lock both loop ports of the junction immediately before the guardian
approach with the existing reusable floor key. The approach door becomes ordinary.
Derive that junction from room connections; no additional saved metadata. The key
and optional reserve stay upstream, the objective still requires guardian deaths.
Validate this on final tiles, both graph families, rotations and fallback. One key may match multiple locks;
duplicate keys and unmatched lock/key IDs still reject.

Starting supplies become one healing potion per character and one mana potion per
cleric/wizard. Each main cache has 1 healing/2 mana; each reserve has 1 healing/1 mana.
Guardian rewards retain kits but lose healing potions. Random drops/ammunition/kits
are unchanged: balanced full-clear guaranteed totals are 10 healing/11 mana.
Unused rogue Cunning Defend applies on explicit End Turn through the existing
allowance/event path, before advancing; quick potions share that allowance at level 3.
Healing stable/dying allies with administered potions remains legal; dead allies reject.

## Presentation and saving ownership

Present a centered blocking Saving… dialog over the dimmed cached composite before
inspection. Use the app-owned preparation worker for inspection/save work, joining
before returning. Main-thread pumping only touches cached presentation and window
events, never borrowed run/commands/store. Gameplay and UI commands stop; close is
deferred until completion, never cancels publication. Retain synchronous headless
execution and all existing atomic/revision/durability protections.

Compute badge anchors from canonical idle alpha bounds, using the same fitted
billboard geometry as rendering/picking. Bounded layout searches avoid initiative,
badges and depth occlusion; draw complete badges or omit them, never fragments or
indicators for fully hidden actors. Fit billboards against solid geometry and the
viewport using alpha extents, scaling about ground anchors instead of moving across
subslots. Reuse prepared bounds, fixed scratch arrays and allocation-free frame work.
Preserve natural front/rear overlap; test logical uniqueness through placement,
movement, death/removal and restore rather than flattening perspective.

## Compatibility and verification

Generator 6/templates 5 and campaign schema 5/simulation 9; strict old-save rejection,
no migration. Shared population resolution remains authoritative for launch/admission.
Run focused owned gameplay/generation/fixture/rogue/save/render/allocation checks,
one persistence integration pass, and small hidden application-owned captures when
available. No OS input/focus automation and no expedition-study rerun. One intensive
job, low priority, bounded workers. Record actual commands/results/limitations here
before marking Ready for review. Prior route results remain historical, not evidence
for the new potion budget.

## Delivered verification and handoff

Release application/headless and the user's Debug application build with one
low-priority worker. Logs live in `output/pc072-review/`. No installs, OS input,
focus changes, expedition trials or balance-study reruns. No commits made.

Twenty-six distinct affected CPU suites passed across focused selections (passing
suites were not rerun without a subsequent relevant change): `actors`, `automation`,
`automation_allocations`, `campaign`, `campaign_route_tools`, `campaign_save`,
`campaign_save_cli`, `campaign_save_smoke`, `expanded_spells`, `fixtures`,
`fixtures_automation`, `generation`, `generation_automation`, `generation_cli`,
`guardians`, `inventory`, `level_spells`, `population`, `progression`,
`progression_allocations`, `spells`, `visual_review`, `world_environment`,
`world_integration`, `world_visibility`, `world_visual_cli`.

- `ctest --test-dir cmake-build-release -R '^(generation|population|fixtures|level_spells|visual_review|inventory|actors|campaign|guardians|world_visibility|campaign_save_smoke|progression_allocations|automation_allocations)$' -j 1 --output-on-failure`
  selected core review behavior (`focused-tests.log`). Later corrected checks and
  caller selections are in `integration-tests.log`, `caller-tests.log`,
  `final-caller-tests.log`, `sector-caller-tests.log` and `fixtures-final.log`.
- Final malformed-sector-door facing validation: `ctest --test-dir cmake-build-release
  -R '^(generation|population|fixtures)$' -j 1 --output-on-failure` passed 3/3
  in 0.05 s (`final-generation-tests.log`). Final application rebuilds are recorded
  in `final-build.log` and `debug-final-build.log`.
- One full persistence integration run: `ctest --test-dir cmake-build-release -R
  '^(guardians|world_visibility|world_environment|campaign_save)$' -j 1 --output-on-failure`.
  `campaign_save` passed in 4.42 s: duplicate live subcells reject on restoration,
  corruption/atomic publication and rollback protections remain covered.
- Hidden `blobber_world_visibility_gpu_tests` passed, zero boundary differences;
  hidden `blobber_world_environment_gpu_tests` passed. Hidden
  `blobber_world_integration_gpu_tests` passed: CPU/GPU parity, allocation/resource
  lifetimes and travel rollback (17 preparation failure positions; zero edge differences).
- `blobber_world_integration_gpu_tests cmake-build-release/assets
  output/pc072-review/captures-verified --pc072-review` passed. Inspected close-range
  wall/target/Bless and centered saving captures at 1920×1080 and 1280×720.
  Eight cached saving frames allocated no engine C++ or GPU resources. An injected
  close probe confirmed save work joins without worker cancellation, without OS events.
  [Retained gallery](screenshots/pc072-review/README.md).

Initial failures were retained in logs and resolved: runtime admission assumed one
lock/key, old tests assumed potion rewards and fixture indices, transport fixtures
turned during unresolved contact, and GPU integration declared seven species but
prepared two. One new rendering test initially used an out-of-format appearance
width; its corrected owned clip exercises legal wide sprites. The alpha oracle now
checks visible pixels, allowing transparent filtering borders. An intermediate
asset copy with mismatched old preset/new creation stock rejected; refreshing both
stock definitions and copied assets fixed production admission. No admission checks
were weakened to accept inconsistent data.

No duplicate-subcell gameplay bug was reproduced. Existing placement/movement/import
rejection remains intact; mixed-undead movement and duplicate-live restore regressions
pass. Natural front/rear overlap is intentional per review. The billboard fit uses
at most two 16-step DDA rays on visibility updates, has conservative bounded exhaustion,
and reuses prepared alpha bounds. Whole-badge searches are bounded and allocate no heap.
Topology/encounter selection rules are unchanged, but generator versioning also
changes deterministic seed streams. Generator/templates are 6/5, campaign
schema/simulation 5/9, visibility v7; fresh runs
are required. Library drafts remain independent and new launches receive current stock.

The potion budget has no new balance evidence. Existing eight-route results describe
generator 5 and the previous supplies, not this revision. Interactive expedition/save
feel remains user review. PC07.2 and parent PC07/P-C are not accepted; PC08 remains
unapproved. Next action is user review of these corrections.
