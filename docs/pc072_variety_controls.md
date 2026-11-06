# PC07.2 — campaign variety and responsive controls

Status: **Accepted and closed** as part of PC07.2, 2026-11-01. User: “slice accepted,
record it in the plan doc”. Implementation was approved with “Implement the plan”.
Parent PC07 is closed; P-C remains incomplete and PC08 is not started.

## Campaign and action ownership

New parties receive one camp kit total; existing floor rewards, potion budgets,
encounter recipes/counts and XP stay unchanged. New unseeded campaigns sample
entropy once at commitment, explicit seed overrides remain reproducible, and
Continue reports the saved seed without drawing another. Show that seed in launch
output and the gameplay log. Keyed door openings name the reusable bronze/silver
key in the log; closing, previews and failed interactions do not report key use.

Normal topology generation samples connected 6–8-junction masks on the existing
4×4 lattice with bounded enumeration and seeded reservoir sampling. Adjacent
junctions connect. Reserve the approach/objective chain; the sector junction has
two loop neighbours and its removal leaves all upstream junctions connected.
Attach the entrance and one/two optional leaves upstream, retain at most twelve
rooms, templates, protected landings, route population and final-tile validation.
Family 0 now denotes generated graphs, family 1 the checked fixed fallback. Keep
sixteen attempts and bounded scratch storage. This changes graphs, branches,
stair attachment and encounter positions rather than just rotating two layouts.

At the existing completed-action boundary, a conscious level 2+ rogue with no
normal action remaining applies unused Cunning Defend and advances initiative.
Do not wait for another UI command. Preserve attack/projectile resolution,
already-used allowances, existing defence and terminal combat phases. Level 3
quick potions are used before the last normal action. Manual early defence and
explicit skipping remain supported; other classes retain action sequencing.

## Shared pointer interaction

Use bounded button targets and one pointer capture per UI owner. Stable identities
and context validation prevent a held press from activating a replacement control.
Physical mouse-down arms an enabled button; holding shows a darker inset pressed
state; release over that same control activates once. Leaving its bounds cancels
the press permanently. Focus loss, cancel, panel/context changes and disabling
also cancel. No repeat while held. Feedback is ephemeral UI state, never saved or
part of simulation replay. Existing logical input callers remain supported.

Reuse layout definitions for hit tests and feedback in menu/creator/arena, HUD,
dialogs, inventory commands, spell/rest/advancement screens and selectable rows.
Keep raw pointer edges for inventory/party dragging, scrollbars, text focus and
world targeting. Keep keyboard activation timing. No heap allocation or resource
growth during steady frames; overflow cancels interaction rather than growing.

## Compatibility and verification

Generator 7/templates 5, campaign schema 5/simulation 10; strict rejection of old
campaigns without migration. Arena protocol remains unchanged. Document all
delivered changes and limitations here and in the system overview/checkpoint.

Focused owned fixtures cover seed selection/override/restore, one kit, keyed logs,
sixteen-seed structural diversity across depths plus fallback, key/sector access,
unchanged XP, pointer cancellation/context/drag/disabled cases and rogue action
resolution. Require eight non-rotational/reflection-equivalent complete layouts
and multiple junction degree profiles in the structural sample. Run relevant UI,
automation, persistence and allocation checks in Release, build the Debug app,
and inspect hidden application-owned pressed-state captures. One low-priority
intensive job at a time, bounded workers, no OS input/focus or balance experiments.


## Delivered boundaries and storage

`generation/topology.cpp` owns preparation-only graph sampling. It enumerates the
65,536 possible lattice masks with a single reservoir mask, then attaches seeded
entrance/optional leaves; it does not retain a candidate pool. The existing generator
owns templates, ports, fixtures, population, final validation and publication.
Sixteen attempts and a checked fixed fallback remain; at most twelve rooms are
admitted against the existing sixteen-room/twenty-four-connection storage. The
sector junction's two upstream edges still share the floor key on depths 2–3.
The authored hub and all population recipe/count/XP and reward budgets are unchanged.

`chooseCampaignSeed` chooses saved seed, explicit override, then entropy in that
order. New Game was already sampling entropy; the larger source of repetition was
the two authored topology families. This correction centralizes seed precedence,
avoids an unused entropy draw on Continue and logs the campaign seed. An explicit
`--seed` intentionally reproduces a campaign. The fixed authored hub always repeats.

`input/buttons.hpp` contains 160 fixed button targets and one captured target with
an eight-word context. Each menu, HUD or flow owner retains its own `ButtonPress`;
FlowControls retains its existing 128 controls. `ui/button_targets.cpp` builds HUD
targets from the shared panel layouts. Target identity, rectangle, enabled state,
context and topmost hit must still match on release. Inventory commands use semantic
action identities, with selected item/page/quantity in context. Menu library revision,
turn/panel transitions and flow state cancel stale presses. Registry overflow fails
closed. Trusted logical input remains supported; physical `UiInputEdges` sets
`pointerTracked` so mouse-down is routed through capture. No capture state is saved.

The painter overlays a dark fill, gold rim and teal inset on the captured rectangle.
It reuses the current composite without allocating textures, timers or containers.
Inventory/party dragging, text fields, scrollbars and world targeting retain their
raw pointer edges. A party-slot click is now committed on release; dragging still
uses its existing threshold/drop rules. Leaving the original slot cancels a click
even if the pointer returns without crossing the drag threshold.

Combat owns automatic rogue completion at `completeAction`, after melee/projectile
resolution. A shared helper also serves explicit End Turn. It checks consciousness,
unused allowance and existing defence, emits the ordinary Cunning Defend event once,
and advances initiative. Victory/defeat take precedence. Level 3 quick potions must
precede the final normal action; other classes retain optional action ordering.

## Executed verification and review evidence

All intensive commands ran serially at low CPU/I/O priority, with one build worker
and one test process. Release application, headless runner and affected test targets
built; the final Debug application build also completed. No toolchain/package change.

```sh
nice -n 15 ionice -c 3 cmake --build --preset release -j1 --target blobber blobber_headless
nice -n 15 ionice -c 3 cmake --build --preset debug -j1 --target blobber
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-release -R '^(campaign|creator|buttons|fixtures|generation_automation|fixtures_automation|generation_cli|campaign_save_cli|automation|options|menu|arena|campaign_save)$' -j1 --output-on-failure
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-release -R '^(buttons|commands|inventory|visual_review|creator|campaign_save_smoke)$' -j1 --output-on-failure
nice -n 15 ionice -c 3 cmake-build-release/blobber_world_integration_gpu_tests cmake-build-release/assets output/pc072-variety/captures-final --pc072-controls
```

The integration selection passed **13/13 in 9.52 s**, including full persistence
once (**4.49 s**). The final input/caller selection passed **6/6 in 1.16 s** after
adding the held-inventory-command context regression. Final menu/creator checks
passed **2/2 in 0.22 s**. Across the focused selections, **22 distinct CPU suites**
passed: options, buttons, progression_allocations, level_spells, visual_review,
inventory, menu, creator, arena, automation, automation_allocations, fixtures,
fixtures_automation, generation, generation_automation, generation_cli, population,
campaign, campaign_save, campaign_save_smoke, campaign_save_cli and commands.

Structural tests use seeds 0–15 at each depth: at least eight complete layouts
remain distinct after rotation/reflection/translation normalization, more than one
junction degree profile occurs, and no fallback is used in that sample. Existing
fallback, port/access/lock checks remain. These are structural correctness fixtures,
not a balance sweep. Four separate first-floor exports (seeds 0–3) are retained in
the gallery as diagnostic map evidence. Seed precedence uses an owned fake entropy
source; no probabilistic assertion about the platform RNG is required.

Owned regressions cover key open/close/reopen logs without key consumption; one
starting kit; press/hold/release, leave/reenter, disabled/overflow/blur/context and
drag cases; and level 2/3 rogue melee/ranged completion, prior manual defence,
interrupted attacks and terminal outcomes. Existing automation and allocation
checks pass. Early failures exposed obsolete three-kit/either-order test assumptions;
repeated-rest/split fixtures now explicitly acquire their extra owned stock. An old
generation mutation fixture now mutates a template actually used by its seed.

The hidden-window presentation helper passed with five retained captures. Twelve
pressed/released composed frames created **zero engine C++ allocations or additional
GPU textures/targets**, and the OpenGL error check passed. All five captures were
visually inspected. A first capture attempt correctly rejected the disabled
exploration Defend button; the helper was corrected to use the enabled Bag control.
Later item-context/close-overlay safety edits passed the final CPU checks and both
application builds; painting was unchanged, so the GPU capture workload was not repeated.
See the [retained gallery](screenshots/pc072-variety/README.md). Local build/test
logs are under `output/pc072-variety/`. `git diff --check` passes.

Human interaction/feel is left for user review. No OS input/focus automation,
new expedition runs, balance experiment, broad configuration matrix or benchmark
ran. The original eight-route results describe generator 5, three starting kits and
older potion budgets; they do not validate current balance. The implementation checks did not themselves constitute acceptance. The user later
accepted PC07.2 and parent PC07; P-C remains incomplete and PC08 is not authorized. Fresh campaigns are required;
incompatible saves reject without migration or modification.
