# Slice 13.2 verification and handoff

Status: implemented, **Accepted and closed**, 2026-10-30. User authorized implementation
with “implement the plan” and accepted the result with “close up the slice”. Slice 13.1 remains accepted and
closed; MCP/capture integration in 13.3 and save/resume in 14 remain unstarted.

[Design](automated_players_design.md) · [Usage](../experiments/arena/README.md) ·
[Automation roadmap](automation_plan.md#slice-132-automated-players-and-balance-experiments) ·
[Demo checkpoint](dnd5e_demo_design.md#current-checkpoint)

## Delivered

A stdlib Python CLI validates, runs, reports and strictly reruns bounded arena
experiments through the accepted headless commands. The existing script client
retains its public request/context-manager interface, adding nonblocking bounded
I/O, deadlines, cancellation, stderr-tail capture and child cleanup. No gameplay,
renderer, command schema, arena format, or engine replay implementation changed.

Three deterministic policies share checked candidate generation and navigation:
focus, conservative and aggressive. Casters prefer supported cantrips over backup
weapons; martial characters prefer normal weapon attacks. Healing/stabilization,
owned consumables, compound recipient/dart payloads, frontal areas, concentration,
Second Wind, safe Arcane Recovery and partial-victory continuation are covered.
Preparation policies wait through ordinary cooldowns before approaching; they do
not gain free recovery or reset resources. Decisions occur on the recorded 250 ms
party-wide real-time grid, with ordinary initiative decision boundaries.

Three sensible level-one party packages use validated preset-derived builds:
balanced Fighter/Cleric/Rogue/Wizard, wizard-free Fighter/Cleric/Rogue/Rogue, and
spell-free Fighter/Fighter/Rogue/Rogue. Their distinct identities are independent
snapshots. There is no weak-build sweep, universal-win target, or automatic balance
tuning. Trial setup preserves ordered rosters and explicit seed lists.

The runner streams engine traces and deterministic decision explanations, with
exclusive output directories, strict input/code/content identity, resource/work/time
budgets, cancellation and selected trace retention. Reports expose all outcome
categories, actual denominators, uncertainty, casualties, remaining/consumed
resources, observed downings, rounds, simulation/gameplay clocks and wall costs.
An incomplete recording is never labeled replayable. A confirmed combat outcome
with an artifact failure keeps its outcome but is excluded from verified aggregates.

**All balance matrices, even the 18-trial smoke profile, are explicit playtesting
commands.** The only new CTest registration is the short fixture-owned
`arena_experiment_tools` contract check; it does not launch the game or play a fight.
The separate generated-fixture integration playtest and benchmark are not CTest
entries. Unrelated renderer/UI work must not automatically rerun these experiments.

## Executed checks

Commands ran from the repository root on Linux. `/tmp` locations are review artifacts
on this machine, not permanent test fixtures. Source recipes remain in the repository.
Final documentation checks passed 276 local links/anchors, synchronized slice
tracking, changed/new-file whitespace and final newlines, and `git diff --check`.

| Check | Result | Evidence |
| --- | --- | --- |
| New tooling contracts | 27 checks passed, about 1.7 s | `python3 tests/arena_experiment_tests.py`; `/tmp/blobber132-tooling-final.log` |
| Focused graphics-disabled Debug CTest | 5/5 passed | `/tmp/blobber132-debug-tests.log` |
| Focused graphics-disabled Release CTest | 5/5 passed | `/tmp/blobber132-release-tests.log` |
| Existing scripted client compatibility | Complete fixture fight/reset/replay and failure checks passed | Included `headless_cli` in both focused suites |
| Separate generated-fixture integration playtest | Passed | `/tmp/blobber132-playtest-final.log` |
| Explicit 18-trial diagnostic smoke | 18 completed victories, no other outcomes/artifact failures | `/tmp/blobber132-review-smoke/report.md` |
| Explicit 216-trial diagnostic exploratory run | 216 completed victories, no other outcomes/artifact failures | `/tmp/blobber132-review-exploratory/report.md` |
| Prepared visual mode | One balanced/aggressive, two-goblin, seed-5 victory; no inspection contamination | `/tmp/blobber132-review-visual/report.md` |
| Deterministic policy rerun | Three diagnostic cases spanning all parties/policies, plus visual case matched decisions/outcome/final state | `/tmp/blobber132-reproduction.json`; `/tmp/blobber132-policy-repeat-*` |
| Engine strict replay | Selected diagnostic/visual replays matched; generated defeat and injected policy-failure prefix also matched | Reproduction record and explicit integration playtest |
| Visual replay without display environment | Matched with DISPLAY/WAYLAND_DISPLAY unset; no window | Existing `blobber_headless --mode visual --replay` |
| Existing separate 256-actor allocation stress | Zero tracked C++ allocations; 71 accepted actions, 698 events | `cmake-build-headless-release/blobber_automation_allocations_tests --stress` |
| Explicit Release recording/performance measurements | Matching visible output for record on/off, three pairs/workload | `/tmp/blobber132-bench-*/benchmark.json` |

Focused suite command, run with each `headless-debug` / `headless-release` preset:

```sh
ctest --preset headless-release \
  -R '^(automation|headless_cli|arena|arena_experiment_tools|automation_allocations)$'
```

The manual integration recipe owns a generated catalog, metadata, scenarios and
isolated temporary directories. It verifies a real victory and defeat; strict
engine replay; policy rerun; rejection after content changes; simulation limits;
sidecar exhaustion; cancellation after play starts; injected policy failure with a
complete replayable prefix; and unsupported candidate-search exhaustion. These
failures do not become combat losses. It also checks real-time actions occur on the
25-tick grid and at least 25 ticks apart. Run only when relevant:

```sh
python3 tests/arena_experiment_playtest.py \
  cmake-build-headless-release/blobber_headless
```

Fast checks cover healing versus death/stabilization, concentration, dart totals,
area direction, racial/active spell coverage, caster/weapon priorities, cooldown
waiting and Arcane Recovery preparation, optional End Turn, deterministic navigation,
private-data filtering, gross mana expenditure/recovery, stale-safe item payloads,
strict inputs/limits/identity, report denominators, missing pairs and repeatable
uncertainty, trace retention, exclusive/bounded sidecars, blocked writes, partial
reads, oversized responses, event IDs, stderr draining, cancellation and child reaping.

No new C++ implementation was introduced. New sanitizer runs and full unrelated
renderer/gameplay suites were not run; the focused scope above is the claimed scope.
The existing typed C++ allocation guard excludes Python, JSON/trace I/O, C allocations
and third-party internals. Interactive desktop and GPU/presentation review were not
performed or needed for this Python-only tool change. No OS input, focus switching
or desktop automation occurred.

## Experiment interpretation

The final exploratory run used seeds 5, 17, 29, 43, 67, 89, 113 and 151, with three
parties × three ordered rosters × three policies. All 27 cells had eight victories.
A cell's 8/8 result has a 95% Wilson interval of approximately **67.6%–100%**; there
are too few paired seeds for the configured bootstrap interval. This is evidence
that the selected fixtures/policies run competently, not a precise viability ranking
or a promise that these parties beat harder encounters.

Victories were not uniformly casualty-free. The balanced conservative/aggressive
mixed-roster groups each included a victory with one permanent death. Resource use
and durations differ; for example, median remaining party HP against the mixed
roster was 28.5/33/29 for balanced focus/conservative/aggressive and 29/33.5/29 for
the spell-free party. These small samples do not justify tuning conclusions. No
monster, class, item, resource amount or combat rule was changed.

The final run exercised 48 safe Arcane Recovery activations across the balanced
non-focus policies. Earlier development pilots preceded the preparation/caster
priority corrections and are superseded by the final review artifacts above.
Failure classification is independently established by owned fixtures, not by hoping
the shipped-content matrix happens to lose or stall.

## Performance measurements

Machine: AMD Ryzen 9 7900, 12 cores / 24 logical CPUs, x86-64 Linux. One trial/runner
at a time; Release binaries. Full interpreter/platform/build/content identities are
in each provenance/benchmark JSON. OS caches and background load are uncontrolled.
Each benchmark used three record-off/on pairs, alternating order, and required the
same visible-output hash for all six executions. These timings include process
startup, fingerprinting, JSON, Python/IPC and (when enabled) engine trace I/O.
They are **not** pure simulation timings or frame-rate claims.

| Workload | Median wall, recording off | Median wall, recording on | Recorded trace bytes | Largest observed runner VmHWM |
| --- | --- | --- | --- | --- |
| One-goblin complete trial requests | 0.0365 s | 0.1010 s | 1,185,628 | 8,172 KiB |
| Two-goblin complete trial requests | 0.0844 s | 0.1214 s | 3,152,678 | 8,220 KiB |
| Six-enemy mixed complete trial requests | 0.1646 s | 0.2141 s | 4,602,705 | 8,308 KiB |
| Two-goblin prepared visual trial requests | 0.5396 s | 0.6161 s | 2,838,030 | 240,960 KiB |
| 32-actor bounded stepping, 3,200 actual ticks | 0.0407 s | 0.0587 s | 1,389,208 | 8,544 KiB |
| 256-actor bounded stepping, 3,200 actual ticks | 0.1989 s | 0.2379 s | 8,767,677 | 11,508 KiB |

Diagnostic startup medians were approximately 4–13 ms; prepared visual startup was
about 452 ms. Capacity scripts are fixed movement/advance requests and need not
finish a fight; they establish bounded workload behavior, not 256-enemy balance.
Full phase/actual-tick/request/byte measurements are in the benchmark JSON files.

The 216-trial recorded exploratory batch took **23.55 s** overall; summed trial
wall time corresponds to approximately **9.35 completed trials/s**. The run directory
held about 192 MB after selected trace retention. These are observed local results,
not a promised throughput multiplier. Recording and visual startup are material
costs, but serial performance is sufficient for the initial scope; no worker pool
or timing shortcut was added. Report generation time is recorded separately from
trial timing; protocol timing overlaps startup/play-loop timing and must not be
summed as independent CPU stages.

## Review recipe and next-agent handoff

Read the existing final reports above, or use a **new** output directory:

```sh
python3 tools/arena_experiment.py validate experiments/arena/smoke.json
python3 tools/arena_experiment.py run experiments/arena/smoke.json \
  --output /tmp/blobber-132-user-review
python3 tools/arena_experiment.py report /tmp/blobber-132-user-review
```

Inspect `report.md`, `summary.json` and `trials.csv`; choose a `trial_index.json`
entry for its exact engine replay and policy rerun commands. Rerun reconstructs
from the beginning and compares normalized decisions and outcome. It is not a run
save. Runs require new output directories; report regeneration replaces only the
tool's own report files. Strict code/binary/content identity can invalidate
historical artifacts after changes.

Useful review artifacts:

- `/tmp/blobber132-review-exploratory`: complete comparison, selected traces,
  resolved inputs, coverage and provenance.
- `/tmp/blobber132-policy-repeat-0`: balanced/aggressive mixed-roster seed 5,
  trial `774224fd0e98dbab01985bd3`, matched fresh policy run and engine replay.
- `/tmp/blobber132-policy-repeat-1`: wizard-free/conservative mixed-roster seed 17,
  trial `371819029f1c1ff6a3376c4a`, matched from retained inputs even though the
  original successful engine trace was not selected for retention.
- `/tmp/blobber132-policy-repeat-2`: spell-free/focus mixed-roster seed 29,
  trial `d15cd145ec1036957631d8af`, matched from retained inputs.
- `/tmp/blobber132-review-visual` and `/tmp/blobber132-policy-repeat-visual`:
  player-visible prepared-mode case `e0489cdb189cbfb35618846e`, matched replay/rerun.

Policy coverage remains intentionally limited to the selected level-one loadouts.
Sleep, Grease, False Life, Color Spray, Poison Spray, Resistance and Sanctuary are
unsupported policy choices, explicitly rejected when active. Later-level tactics,
rest/loot/advancement automation and full dungeon expeditions remain outside this
profile. No implication that the engine itself lacks those accepted commands.

For 13.3, reuse `Client`, the experiment runner's bounded callback/cancellation
interface, immutable manifests, reports and existing engine replay. Keep policy
inputs isolated from diagnostic traces. Do not make MCP a second rules engine or
make the experiment commands ordinary test prerequisites.

## Accepted closeout and next-agent handoff

The user accepted the executed verification as sufficient and instructed “close up
the slice” on 2026-10-30. The results, coverage limits and unexecuted checks above
remain unchanged; acceptance does not establish future gameplay balance.

The user requested open-ended use of this general-purpose project tooling wherever
relevant. Feature development, investigation, regression diagnosis and playtesting
are examples, not an exclusive list or conditions for use. Assess coverage and
assumptions for the task and extend them when needed. Human-readable Markdown
reports already exist; agents can explain findings without requiring the user to
inspect raw traces. Illustrative future examples neither restrict the purpose nor
create new feature requirements. Balance experiments remain opt-in, not automatic
checks for every change.

**Next action:** plan 13.3 (MCP play and investigation) in a clean context, inspect
the implemented interfaces and present its plan before implementation. 13.1/13.2
remain closed; 13.3 has not started and save/resume remains slice 14. Preserve the
original planning-document changes and all current implementation work; no commit
was requested. This closeout changes documentation only and does not rerun balance
experiments.
