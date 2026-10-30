# Arena playtesting (explicit commands only)

These are playtesting and balance experiments, **not ordinary tests**. No matrix,
including `smoke.json`, is registered with CTest, build presets or CI. Do not run
these as verification for unrelated renderer/UI changes. The small
`arena_experiment_tools` CTest entry checks owned policy/transport/report fixtures
without starting the game or playing a fight.

The initial comparison uses competent level-one starter builds:

- `balanced`: the accepted Fighter/Cleric/Rogue/Wizard preset.
- `no_wizard`: Fighter/Cleric/Rogue/Rogue, without racial arcane spells.
- `no_spells`: Fighter/Fighter/Rogue/Rogue, without any spells.

The alternatives retain the preset fighter/rogue builds with distinct character
identities. Their gear, point buy and spell grants were exported through the
existing C++ arena/build codecs. They are standalone snapshots, independent of the
user's character library. The headless runner revalidates them on every start.
These are whole-party packages, not controlled changes to one isolated class.
There is no goal that every composition beats every roster, and no automatic tuning.

From the repository root:

```sh
cmake --preset headless-release
cmake --build --preset headless-release --target blobber_headless -j 6
python3 tools/arena_experiment.py validate experiments/arena/smoke.json
python3 tools/arena_experiment.py run experiments/arena/smoke.json \
  --output /tmp/blobber-arena-review
python3 tools/arena_experiment.py report /tmp/blobber-arena-review
```

Run output directories must be new; `report` regenerates the tool's own report
files in an existing run. `validate` checks matrix
shape and starts each party/roster through C++ without playing. It also checks policy
coverage. Use `--structure-only` for JSON/matrix checks without a runner; that does
not establish gameplay validity. All commands are stdlib Python; no pip packages.
The default runner is `cmake-build-headless-release/blobber_headless` with `assets`.
Pass `--runner` and `--assets` explicitly when using another build.
For repository-local runs, put generated reports/traces under the ignored
`output/` directory (for example `--output output/arena-review`), or use `/tmp`.

`smoke.json` runs 18 trials: three parties, the two-goblin roster, three policies,
and two seeds. `exploratory.json` runs 216: the same parties/policies, three ordered
rosters, and eight seeds. Run the exploratory file only when requested for
playtesting/balancing. Eight seeds give broad uncertainty, not a balance verdict.
Larger seed lists are an explicit follow-up after policy competence is reviewed.
Do not sample until a desired result appears.

To use visual observations, copy a manifest, set `mode` to `visual`, keep its party
paths valid relative to that file, and pass the normal graphics build's runner
and prepared asset directory:

```sh
python3 tools/arena_experiment.py run /path/to/visual-manifest.json \
  --runner cmake-build-release/blobber_headless \
  --assets cmake-build-release/assets --output /tmp/blobber-arena-visual
```

Visual mode uses prepared CPU body picking and opens no window. Missing assets
fail explicitly; there is no automatic diagnostic fallback. Diagnostic mode knows
all enemy positions/HP and is not player-equivalent. Neither policy sees pending
rolls or AI plans. Both use a zero-time preview legality oracle and know layout-v1's
room geometry. Default real-time decisions occur on a shared 25-tick/250 ms grid;
initiative has no artificial thinking delay. Wall-clock speed cannot change that
schedule. `reaction_ticks: 0` is an explicitly labeled instantaneous sensitivity
profile and is never pooled with the default.

Policies are small deterministic priorities, not optimal players:

- `focus`: focus a legal target; weapons for martial characters, cantrips for
  casters; offensive mana retained, emergency healing at one-quarter HP.
- `conservative`: useful preparation/support, one healing cast's mana reserve,
  and healing at half HP.
- `aggressive`: safe frontal areas, compound attacks and paid damage, with
  emergency healing at one-quarter HP.

All prioritize rescuing unconscious allies and use ordinary potions/features.
Non-focus policies can use Arcane Recovery after preparation, before approaching
threats, to recover a spent cast. This is normal checked gameplay, not a free reset.
No profile rests, loots, changes gear/preparation or advances levels during a trial.
Available unsupported spells stop preflight, rather than quietly handicapping a
party. Learned-but-unprepared spells do not require tactical support. Full coverage
and limits: [design](../../docs/automated_players_design.md#policies-and-declared-coverage).

Read `report.md` first. Other outputs are:

| File | Use |
| --- | --- |
| `manifest.json`, `provenance.json`, `preflight.json` | Resolved inputs, code/executable/content identity and checked coverage |
| `batch.json` | Complete/incomplete/cancelled status, started/unstarted counts and reason |
| `trials.jsonl`, `trials.csv` | One row per recorded trial, including noncombat statuses, resources and timing |
| `summary.json` | Group counts, valid-completion denominators, casualty/resource/duration summaries and uncertainty |
| `trial_index.json` | Retention status and exact replay/rerun commands; created after the first trial |
| `scenarios/`, `decisions/`, `traces/` | Actual scenarios, deterministic decision explanations and retained strict engine traces |

`victory` and `combat_loss` require the engine's terminal result. A partial local
victory continues. Stalls, simulated/wall timeouts, unsupported decisions, policy
failures, runner failures and cancellation remain separate. A confirmed combat
outcome with an artifact error remains that outcome but is excluded from the
verified balance denominator. No-start preflight failures are not combat trials.
An orderly trace footer means the recording ended correctly, not that the party won.

Victory rate uses valid wins/(wins+losses); completion rate and wins/started are
also reported. Resource and duration summaries cover valid wins and losses;
unfinished trials remain separate. Downing and HP-transition counters are observed
lower bounds. Mana expenditure/recovery and item consumption are tracked separately
from final quantities. Per-group 95% Wilson intervals and paired-seed differences
show uncertainty; paired bootstrap intervals need at least 20 valid pairs.
Shared seeds do not promise shared roll sequences under different actions.

Every trial is initially recorded. Default retention keeps all defeats/failures,
first victories and fastest/slowest victorious outliers per group. Unselected
successful engine traces are removed only after their summary and retention index
are committed. Scenarios and decision records remain. Set `retention` to `all` to
keep every trace. Capacity exhaustion stops explicitly; it never silently discards
failures. Ctrl-C/SIGTERM cancels locally, reaps the child, and leaves partial results.
The CLI returns success when all scheduled rows were recorded, not when all parties
won. A fatal tooling failure, invalid setup or incomplete batch returns nonzero.

Use the commands from `trial_index.json`, or:

```sh
python3 tools/arena_experiment.py rerun /tmp/blobber-arena-review \
  --trial TRIAL_ID --output /tmp/blobber-arena-rerun
cmake-build-headless-release/blobber_headless --assets assets --mode diagnostic \
  --replay /tmp/blobber-arena-review/traces/TRIAL_ID.jsonl
```

`rerun` checks code/binary/content identity, reconstructs from the initial scenario,
then compares decisions, outcome and final state; it does not resume a saved fight.
Engine replay is the existing strict 13.1 replay and requires no policy. A killed
runner may leave a non-replayable incomplete trace; it stays labeled as such.
Wall-clock failures are not guaranteed reproducible. Rebuilding or changing content
can invalidate old engine traces. Use fresh output paths for every command.

Separate developer checks and benchmarks (also explicit):

```sh
python3 tests/arena_experiment_tests.py
python3 tests/arena_experiment_playtest.py cmake-build-headless-release/blobber_headless
python3 tools/arena_benchmark.py --runner cmake-build-headless-release/blobber_headless \
  --assets assets --trace /tmp/blobber-arena-review/traces/TRIAL_ID.jsonl \
  --output /tmp/blobber-arena-benchmark
```

The first command is the short routine tooling contract check. The second is the
manual generated-fixture integration playtest, not registered in CTest. The third
compares recording on/off for identical requests, alternates order, verifies
visible-output hashes and reports startup/IPC/recording-inclusive timings and peak
runner memory. For a bounded capacity exercise, use `--scenario FILE --ticks 2000`
in place of `--trace`; this need not finish a battle and provides no balance evidence.
Use Release for performance results. OS caches are not controlled; no pure simulation
or frame-rate speedup claim follows. No worker pool or desktop automation is involved.

The optional [local MCP adapter](../../docs/mcp_usage.md) exposes these same explicit
commands as bounded jobs with status/cancellation/results and replay investigation.
Connecting an agent never runs a matrix automatically.
