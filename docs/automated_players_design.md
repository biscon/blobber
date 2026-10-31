# Slice 13.2: Automated players and balance experiments

Status: **Implemented; Accepted and closed**, 2026-10-30. The user authorized
implementation with “implement the plan” and accepted the result with “close up the
slice”. Verification scope and limitations remain unchanged.
[Verification/handoff](slice132_verification.md) and [usage](../experiments/arena/README.md)
record the delivered tools and executed checks. [Roadmap/checkpoint](dnd5e_demo_design.md#current-checkpoint) ·
[Automation scope](automation_plan.md#slice-132-automated-players-and-balance-experiments) ·
[Accepted headless contract](headless_protocol.md).

## Purpose and scope

Build a modest local playtesting tool: a few understandable player policies, a
bounded matrix runner, readable comparison reports, and reproducible individual
trials. Reuse the actual game and slice 13.1 commands/replay. No alternative combat
simulator, enemy AI changes, balance changes, or optimal-play claim.

This is general-purpose project tooling, available wherever agents find it useful.
Feature development, investigation and playtesting are examples, not an exclusive
list. Reports help explain findings without requiring the user to inspect traces.
Assess and extend coverage as needed for the task. Examples do not create new
requirements or restrict use; balance matrices still stay outside automatic routine
verification.

The user's clarification is part of this design: compare **sensible parties with
competent builds**, not every possible composition. Viability does not mean every
party wins every encounter. Four rogues, deliberately poor point allocations and
other intentionally weak builds are outside the initial comparison. Do not weaken
encounters or flatten class strengths to meet a universal win-rate target.

Experiments are **opt-in playtesting and balancing**, including the smallest smoke
matrix. They are not CTest tests, default build targets, CI prerequisites, or
verification to run whenever unrelated code changes. Renderer work must not
implicitly launch headless balance playthroughs. Short tooling correctness checks
are a separate category, selected when relevant.

Starts remain valid level-one arena scenarios. Trials keep that level, starting
equipment and spell preparation, use consumables/normal combat recovery, and
continue partial victories until the entire roster is resolved. No rests, looting,
equipment management or advancement between local encounters in this initial
fixed-loadout comparison. Those protocol operations remain available to scripts;
their automation and expedition/rest pacing are outside this experiment profile.
No arbitrary state changes, direct level-two starts, or development XP shortcuts.

MCP and capture tools remain 13.3; player save/resume remains 14. No desktop input,
window/focus control, screenshot work or presentation change is needed for 13.2.

## Findings from the accepted implementation

- `src/automation/session.*` owns scenario/map/placements/session in lifetime order
  and routes commands to normal validation. Full victory/defeat is terminal;
  partial victory needs `continue`. The driver must check both phase and terminal.
- `protocol.cpp` advances fixed 10 ms ticks, with ceilings of 1,000 ticks/request,
  360,000 ticks/run and 100,000 mutations/run. Decision advancement can return zero
  in settled exploration. Reads/previews freeze time; rejected acts still consume
  a revision. Policy code must use the client's latest tokens.
- `actions.cpp` describes candidates, not complete legal actions. Spell descriptors
  may say a recipient is missing even when a fully specified cast is legal.
  Complete payloads must be previewed; recipient Cartesian products are unnecessary.
- `observe.cpp` exposes health/resources, grants, formation, cooldowns and statuses.
  Visual observations use body picking, omit hidden enemies and pre-impact results,
  and suspend picking during movement. Diagnostic observations expose private data.
  Life enums need one explicit v1 mapping in the client-side model.
- `tools/headless_client.py` is a useful stdlib transport but has blocking reads and
  no request deadline. Its existing script/context-manager API should keep working.
- `trace.*` streams an exclusive, bounded 256 MiB recording for the lifetime of a
  process. There is no per-trial trace rotation command. Every recorded request,
  including preview/discovery, adds response and diagnostic state records. A fresh
  runner per trial is the simplest honest initial ownership/performance choice.
- `game/arena.*` fixes the 21×25 room, spawn and BFS placement. Roster **order** affects
  placement; seeds do not randomize layout. Preserve ordered rosters in identities.
  Four distinct build IDs do not require four distinct classes.
- Starting codecs revalidate point buy, spell grants and exact curated equipment;
  Python must not recreate derivation. High elves have a racial wizard cantrip:
  “no wizard” and “no spells” are different comparisons.
- Existing automation/CLI tests already cover tiny complete fights and replay.
  Existing allocation stress is a separate command. Keep these accepted regression
  checks; do not turn the new experiment configurations into another default suite.

Read: AGENTS.md, system overview, demo roadmap/workflow, automation plan, headless
design/protocol/13.1 handoff, arena design, actual host/client/arena/creation/spell
code, CMake registration, automation/CLI/arena fixtures and tests. Working tree was
clean at planning start; the planning pass changed documentation only. Implementation approval and delivery
are recorded above; existing planning changes were preserved.

## Implementation boundaries and ownership

Use Python's standard library and small cohesive modules, with no package service
or new dependency. Implemented layout:

| Location | Responsibility |
| --- | --- |
| `tools/headless_client.py` | Existing transport, plus bounded I/O deadlines, cancellation, response validation and reliable child cleanup |
| `tools/arena_experiment.py` | Thin CLI: validate, run, report and rerun one trial |
| `tools/arena_experiments/config.py` | Strict manifest parsing, matrix expansion, scenario assembly and stable trial identity |
| `tools/arena_experiments/policies.py` | Shared candidate builders plus three deterministic priority policies; split spell targeting if this grows |
| `tools/arena_experiments/trial.py` | Trial state machine, reaction scheduling, outcome classification, streaming decision records and metrics |
| `tools/arena_experiments/report.py` | Aggregation, uncertainty, JSON/CSV/Markdown output and deterministic outlier selection |
| `experiments/arena/` | Curated party snapshots, ordered rosters and explicit small/example experiment manifests; no ordinary test fixtures |
| `tests/` | Small owned fixtures for policy, transport, classifier and report contracts, separate from the above experiments |

No framework, plugin registry, generic strategy language or automatic policy tuning.

The coordinator owns one child process at a time and immutable resolved inputs.
The trial driver owns a small policy state (focus target, navigation visits, opening
buffs, last decision and counters). Reset that state for every trial. A policy takes
an allowed observation, descriptors and its local state, and proposes ordered
candidates or a typed Wait/Continue/Unsupported decision with a short reason code.
Only the driver touches the client, clocks, files and protocol tokens. Policies
cannot read traces, invoke `inspect`, open content files or peek at future rolls.

Current policies are deterministic; ties use documented stable indices/IDs, never
unordered container iteration or wall time. Reserve a separately recorded policy
seed only if randomized policy behavior is later added; do not consume combat RNG.

## Initial experiment set

Use three reviewed party packages, all level one, with normal 27-point builds,
class-appropriate key abilities, legal starter equipment, normal supplies and
sensible front/back placement:

| Party | Intent |
| --- | --- |
| Fighter / Cleric / Rogue / Wizard | Accepted balanced preset, unchanged |
| Fighter / Cleric / Rogue / Rogue | Wizard-free martial-heavy alternative; two independently identified, competent rogues and no racial arcane cantrip |
| Fighter / Fighter / Rogue / Rogue | Strictly spell-free alternative; no cleric, wizard or racial spell grants; front fighters and rear rogues retain ordinary healing potions/Second Wind |

The third party is an exploratory comparison, not a promise that losing healing
and arcane tools should have no cost. No four-of-one-class or deliberately weak
build matrix. Use existing creation/build functions and arena encoding to prepare
the checked-in snapshots during implementation; no permanent general character
generator is needed. The runner validates resolved scenarios through normal `start`.
Inputs also accept other already-valid exported arena party snapshots, with explicit
coverage validation, without accessing or editing the user's character library.

Start with three ordered rosters: two melee goblins, four melee goblins, and four
melee goblins followed by two archers. These cover a light case, melee pressure and
mixed pressure, not calibrated difficulty categories. Add no new monsters.
Report formation, race, key stats, spells, equipment, ammunition and consumables
beside each package. Starter gear varies by class; describe whole-party outcomes,
not a controlled causal estimate of changing just one class. Shared roster order,
level, seeds, timing and preparation rules are held constant within comparisons.

A versioned JSON manifest contains named parties, ordered rosters, policy IDs and
versions/parameters, an explicit unique uint32 seed list, mode/reaction profile,
budgets and trace retention. Paths resolve relative to the manifest, then snapshots
are copied into the run directory. Reject unknown fields, duplicate IDs/seeds,
invalid numbers and oversized products before playing. Never silently prune cases.
Stable expansion order is party × roster × policy × seed; preserve roster order.

Provide an explicit 18-trial smoke profile (3 parties × 1 roster × 3 policies ×
2 seeds) and a 216-trial exploratory profile (3 × 3 × 3 × 8 seeds). Neither runs
automatically. Eight seeds are preliminary evidence, not a balance verdict. Larger
seed sets are a deliberate follow-up, chosen after reviewing competence and pilot
results. Do not keep sampling until a desired conclusion appears.

## Information and reaction contracts

Two separately labeled modes reuse the accepted headless modes:

- **Diagnostic:** default for fast graphics-free investigation. Policy input may
  include all enemy positions and exact HP, with core targeting rules. It is
  omniscient about enemies and is not player-equivalent. Even here, strip AI plans,
  private RNG and pending attack results from the policy input; feed only resolved
  events to the policy, excluding diagnostic attack-start/release roll payloads.
- **Visual:** prepared CPU visibility, ordinary target filtering, health fractions
  and resolved events only. No hidden-position inference from the manifest or
  diagnostic trace. Missing prepared assets is a setup failure, with no fallback.
  It is structured player-visible information, not a simulation of human skill.

Both policies know layout-v1's arena geometry and their own party, but visual
policies do not know roster placements. Discovery/preview is a zero-time legality
oracle; disclose that assumption. Cap preview work rather than enumerating every
possible selection. Any diagnostic inspection contaminates a visual trial's
provenance and makes it ineligible for the player-visible aggregate.

Default real-time reaction profile: one party-wide action opportunity every **250 ms
(25 ticks)**, including the first action after start; all four characters share
that cadence. Movement duration/cooldowns still apply. No four same-tick opening
shots. At an opportunity, refresh state, rank candidates, preview and submit at
most one action. If waiting, advance explicitly toward the next opportunity.
Movement and intervening automatic events do not queue a stale action. Reobserve
and recompute after any advancement or phase transition.

During initiative, choose at the awaiting character's decision boundary with no
extra artificial delay; the game already freezes tactical decision time. Resolve
optional allowances and explicitly End Turn when appropriate. Normal impact and
presentation tails still advance through 10 ms simulation steps. `until_decision`
is used for resolution, not repeatedly polled at a zero-progress settled boundary.
An explicit zero-delay profile may be used for sensitivity experiments, labeled
as such and never pooled with the 250 ms profile. Wall-clock policy speed does not
change simulated reaction time; no sleeps are used to pace simulation.

## Policies and declared coverage

Three small priority policies share legal payload construction, navigation and
emergency handling. Parameter values and tie-breaking order are frozen in the
manifest and versioned. Casters prefer supported cantrips over backup melee
weapons; martial characters prefer weapon attacks. They are examples of reasonable tactics, not optimizers.

| Policy | Initial priorities after emergency care |
| --- | --- |
| `focus` | Keep attacking the current legal target; otherwise lowest health fraction, then distance and actor index. Prefer normal weapons/cantrips; preserve mana except for healing. This is an intentionally simple baseline. |
| `conservative` | Use a useful long-duration armor buff once, maintain a useful defensive/support concentration, heal a conscious ally at or below half HP, prefer renewable damage and retain one healing cast's mana where available. Recover mana when it unlocks needed healing. |
| `aggressive` | Use the same emergency rescue, then prioritize safe multi-enemy damage/control and focus-fire paid damage; spend offensive mana without a reserve and recover it when another useful cast is available. Heal conscious allies at or below one-quarter HP. |

All three prioritize a legal heal/potion for a recoverable unconscious ally, then
stabilization/Aid when healing is unavailable; never heal dead/full-health targets.
`focus` uses one-quarter HP as its conscious emergency threshold. Prefer the most
endangered ally, then formation slot. Second Wind is considered for an injured
fighter before a potion at the policy's healing threshold. Arcane Recovery and
mana potions require missing mana and an actionable reason, not a repeated loop.
Preparation policies wait for normal buff/recovery cooldowns before advancing
into threat range. Safe Arcane Recovery may restore preparation mana once to
fund another useful cast. No infinite prebuff/recovery cycle or voluntary rest. Base costs/thresholds come
from discovery and versioned policy preferences; legality and effects stay in C++.

Shared approach: use legal ranged attacks/casts while closing toward observed
enemies; do not kite indefinitely. When no enemy is known, a deterministic sweep
of the known open arena finds guards. Use bounded BFS/visit storage for the 525-cell
layout, stable direction ties and previewed steps; no unknown wall/map support.
Visual movement's empty target list is temporary, not evidence of victory.
Continue partial victories, then resume the same bounded search. Never declare a
win by counting the visible enemies.

Coverage is explicit per policy and active loadout:

| Decision family | 13.2 contract |
| --- | --- |
| Melee, Rogue back-row melee, ranged, ammunition, Defend, Aid, End Turn | Supported through candidate eligibility and complete preview; fallback only when it is a deliberate legal tactic |
| Second Wind, Arcane Recovery, dismiss concentration | Supported when useful; automatic Sneak Attack/Life/Defense behavior remains engine-owned |
| Single-enemy spells | Support Sacred Flame, Guiding Bolt, Inflict Wounds, Fire Bolt, Ray of Frost and racial Shocking Grasp in the initial party loadouts |
| Healing/stabilization and ally buffs | Cure Wounds, Spare the Dying, Mage Armor, Bless, Shield of Faith and Guidance; choose legal nonredundant recipients, do not replace useful concentration blindly |
| Compound hostile targeting | Bane uses up to three eligible enemies; Acid Splash selects same-cell pairs; Magic Missile allocates exactly three positive darts across at most three recipients, with focused and split candidate patterns |
| Frontal areas | Burning Hands and Thunderwave; score visible/known affected enemies in the current heading and preview the complete cast |
| Consumables | Healing/mana potions using current generation-checked handles and legal recipients; missing supplies trigger a tactical fallback |
| Other level-one spells | Sleep, Grease, False Life, Color Spray, Poison Spray, Resistance and Sanctuary are initially **unsupported policy choices**, not missing engine commands. Report them explicitly if available in a requested active loadout. Avoid expanding this slice into every possible spell tactic. |
| Level-two/three decisions | Aid spell, Hold Person, Lesser Restoration, Spiritual Weapon and follow-up, Scorching Ray, Shatter, Mirror Image, Web/Escape, Action Surge, Preserve Life and Cunning Action are outside the fixed-level-one experiment. Unexpected availability requiring these choices is unsupported, not a silent End Turn. |
| Inventory management, loot, rest, preparation changes, advancement | Deliberately disabled by this fixed-loadout encounter profile; listed in every report, not claimed as policy coverage |

The initial loadouts require only the supported subset. Learned-but-unprepared
spells do not demand tactical coverage. Preflight records each accessible ability
as implemented, intentionally unused by a named policy (e.g. focus's offensive
mana reserve), unavailable at this level, or unsupported. Unsupported active
loadouts are rejected for balance use; users must choose a supported snapshot or
request a future policy extension. Unknown IDs are never silently discarded.

At runtime, generate a bounded ranked shortlist, use `preview` for complete
payloads and keep rejection reasons. An expected rejected candidate is ordinary
search, not a failed trial. No useful action due to cooldown/transition calls for
Wait; no ordinary action with a legal remaining allowance calls for explicit
End Turn. An unhandled required decision is `unsupported_decision`. An accepted
preview followed immediately by a rejected act on the same state, an illegal
policy payload or an exception is `policy_failure` with subtype and last request.

## Bounds, termination and cancellation

Initial safety limits below may be lowered in a manifest. Keep the first version's
ceilings explicit rather than adding an unbounded override:

| Resource | Default / ceiling and behavior |
| --- | --- |
| Manifest and expanded cases | 1 MiB manifest; at most 16 parties, 16 rosters, 8 policy configurations, 1,024 seeds and 10,000 total trials; validate product before expansion |
| Concurrent trials | One; no worker pool in the initial delivery |
| Trial work | Default 60,000 ticks (10 minutes of simulation time), configurable up to the engine's 360,000; at most 20,000 decisions, 20,000 mutations and 100,000 total requests |
| Decision search | At most 256 ranked candidate previews per decision, generated incrementally; at most 256 enemy records and four party records; bounded target combinations rather than a Cartesian search |
| Wall time | 120 s/trial, 10 s/request, 30 s/startup, 60 min/batch; all recorded, with trial/batch deadlines taking precedence |
| Process shutdown | Try orderly close when responsive, then terminate/kill within a 2 s grace period and reap the child |
| Files | Existing 256 MiB engine trace ceiling; 16 MiB decision sidecar/trial; 4 GiB total run output; reserve room for the next trial and final status before launch |
| Memory | Keep current observations, candidate shortlist, 525 navigation visits, counters and bounded stall history; stream events/decisions instead of collecting run histories |

Client writes as well as reads must honor deadlines and cancellation. Bound decoded
response lines (4 MiB initial client cap), stderr capture (1 MiB retained tail) and
event streaming. Never block indefinitely waiting for a newline or leave an unread
stderr pipe to deadlock a child. Retain error categories rather than parsing all
failures into one generic exception. Current policies contain no user-loaded code;
bounded loops plus batch/trial supervision protect progress.

Stop the batch on cancellation, output-budget exhaustion or systemic transport/
protocol incompatibility. Flush completed rows and mark the active trial, planned
but unstarted count, and batch reason. Do not invent results for unstarted trials.
An isolated policy failure is recorded; the default stops after the first such
failure so hundreds of broken-policy runs do not waste time. Continuing errors
must be an explicit option with a bounded failure count. No automatic retries.

Every started trial has one primary status, plus independent artifact/provenance
quality flags:

| Status | Meaning |
| --- | --- |
| `victory` | Authoritative terminal victory, entire arena cleared |
| `combat_loss` | Authoritative terminal defeat, regardless of how many are dead versus unconscious |
| `stall` | Deterministic progress watchdog detects repeated nonproductive behavior; not an inferred defeat |
| `timeout` | Simulation, decision, request/mutation or wall-clock budget exhausted; record which bound and actual progress |
| `unsupported_decision` | Required ability/loadout/phase cannot be handled by this policy/profile |
| `policy_failure` | Invalid action construction, stale policy use, inconsistent selection, exception or internal invariant failure |
| `runner_failure` | Crash, fatal protocol/I/O error, incompatible response or startup/content failure; distinguish subtype |
| `cancelled` | User interrupt or batch stop before completion |

Preflight invalid manifests/configurations are reported separately from started
trials. Candidate-search exhaustion is an unsupported decision with budget detail;
it does not prove no legal action exists. Trace/report write problems set artifact
failure independently: a confirmed defeat remains a defeat, but it is not a fully
verified reproducible trial. Never turn an incomplete trace into a successful one.

Watchdogs use substantive progress, not ever-increasing revision/tick numbers:
HP/life/resource/status-membership changes, new navigation cells or a new encounter.
Action acceptance, refreshed durations, cooldown countdowns and cosmetic events
alone are not progress. Flag 256 repeated decision signatures with no substantive change;
also flag 60 real-time simulation seconds or 12 completed combat rounds without
substantive progress. Mere misses, round changes or revisiting the same cells must
not reset this indefinitely. Waiting through known cooldowns/movement/resolution
is allowed; it cannot bypass global budgets. Retain signatures/reasons and last
progress so false stall detections can be investigated. A stall is censored evidence,
never a balance loss; watchdog parameters are part of reproduction identity.

## Reports and reproduction artifacts

Each run uses a fresh, exclusively created output directory. Resolve and hash the
manifest and snapshots before playing. Trial ID hashes party snapshot, **ordered**
roster, combat seed, policy implementation/version/parameters, mode, reaction and
budget profile. Run provenance also records executable/content/asset fingerprints,
protocol/timing versions, Python/platform and source revision plus dirty state/code
hashes. A Git commit alone is insufficient for a dirty working tree.

Stream one `trials.jsonl` row per started trial, an indexed decision sidecar, and
the existing engine `--record` trace. Store actual scenario/seed and policy inputs
needed to rerun from the beginning. Decision records contain sequence, observed
tick, policy reason, selected action, bounded preview/rejection summary and actual
advancement; they are explanations, not a second replay format. Trace files remain
diagnostic and are not fed back to player-visible policies.

Record every trial initially. Default retention keeps every defeat and noncombat
failure, the first victory per comparison group, and fastest/slowest completed
victories by ticks (stable trial-ID ties). An explicit retain-all option is available.
Only discard an unselected successful trace after its summary and retention decision
are committed. No deletion of user files or previous runs. If retained failures
exceed the disk budget, stop with an incomplete batch, not silent eviction. Reserve
one full trace plus sidecar and a small report/status margin before each launch.

Outputs: resolved manifest/scenarios, trial index, JSON summary, CSV trial rows,
and a Markdown report. Per trial include:

- Status/subreason, coverage/provenance flags, terminal state and completeness;
  permanent deaths, surviving unconscious/stable members, and observed downings.
  If an intermediate transition is not recoverable from existing events/boundaries,
  label that counter a lower bound rather than inventing an exact total.
- Per-character starting/final HP and mana, death-save state, remaining ammunition,
  potions, feature charges and concentration; resources grouped by party/policy.
- Gross expenditure and recovery separately where derivable from accepted commands
  and consecutive observations. Net initial-minus-final mana is not total spending.
  Track consumable quantities/feature use; unavailable metrics are null with reasons.
- Rounds completed from round events across partial encounters, encounter count,
  simulation ticks/time, gameplay-clock duration, actions/previews/rejections,
  simulated reaction profile and maximum observed decision-search work.
- Startup, play-loop, policy, protocol request and report/trace costs as measurable,
  total wall duration, bytes written and peak process memory where available.
  Report overlapping timings honestly; IPC wait is not pure C++ compute time.
- Trace/sidecar paths, trace completion and replay verification state, and exact
  rerun/replay commands. Full strict replay is available without running the policy.

Aggregate separately by party, roster, policy, mode and timing/coverage profile.
Refuse to pool different executable/content or policy implementation fingerprints.
Always show planned, started, valid combat completions, each noncombat status,
artifact failures and unstarted counts. Primary conditional victory rate is
`wins / (wins + combat_losses)` among compatible valid completions, with its actual
denominator; zero denominator is N/A. Also show completion rate and wins/started.
Do not hide stalled/unsupported/failed trials by presenting only a win percentage.
Summaries of casualties/resources/duration specify whether they include victories,
defeats or both; unfinished data is separate, never padded with zeros.

Use 95% Wilson intervals for per-group victory proportions. For predeclared paired
comparisons on shared seeds, show paired counts and a deterministically seeded
paired bootstrap interval for victory-rate difference (2,000 resamples, at least
20 valid pairs); below that, report the observed difference and insufficient paired
sample size. Missing pairs and small samples are explicit. No significance ranking across
many exploratory comparisons. Matching seeds does **not** imply that different
actions consume the same rolls. Medians/ranges or percentiles describe duration and
resource outcomes without claiming high precision from eight trials.

Reproduction has two separate checks: engine replay must match its original trace
under strict 13.1 compatibility; rerunning the policy must reproduce normalized
decisions/outcome under the same policy/configuration/build. Ignore wall durations,
output paths and process identities in policy repeatability comparisons. A killed
runner may leave an incomplete trace: retain it for diagnosis, explicitly mark
non-replayable, and offer a fresh-from-start rerun. Do not promise to reproduce an
external wall-clock timeout or resume an interrupted fight.

## Validation and performance measurement

Keep three visibly separate activities:

1. **Fast tooling correctness:** owned observation/descriptor fixtures check legal
   target selection, healing versus stabilization/death, concentration preservation,
   racial grants, cooldown waiting, compound darts/recipients, feature use, optional
   End Turn and unsupported coverage. Scripted transport fakes check timeout,
   partial lines, cancellation, child cleanup and disk/trace errors. Tiny synthetic
   trial rows check all denominators, outcome precedence, missing data, uncertainty,
   stable IDs and retention. These can be registered as a fast tooling test; no
   autonomous full fight or balance matrix belongs in that test.
2. **Explicit integration/competence playtesting:** a small generated-fixture recipe
   exercises real previews/actions, approach, victory/defeat and strict fresh-process
   replay, plus targeted healing, resource exhaustion, compound targeting and
   partial continuation. Run manually when developing this tool. Test fixture
   setup can isolate decisions; experiment runs never mutate engine internals.
   Separately run the shipped-content smoke and exploratory comparisons for this
   slice's review, with the exact budget and results in the handoff. Do not assert
   that any sensible party must beat a specified roster.
3. **Explicit performance experiments:** Release only for throughput claims; measure
   cold startup/asset fingerprinting, representative 1–6 enemy encounters and
   heavier 32/256-actor bounded exercises separately. Maximum-capacity work need
   not finish a battle and is not evidence of balance. Compare diagnostic/visual
   and recording overhead on matched requests; report machine, build, workload,
   repetitions, wall time, simulated seconds/s, completed trials/minute, memory,
   request count and trace bytes. Include failed-trial time in total throughput.

Run relevant existing Debug/Release automation/CLI/arena checks if their paths are
changed, and the new fast tooling checks. Existing simulation allocation guards
remain the authority for engine-owned frame behavior; Python, JSON and report I/O
may allocate outside that boundary. If C++ paths change, run focused sanitizers and
allocation checks appropriate to that change. Do not claim a full suite was run
unless it was; do not rerun balance experiments for unrelated renderer fixes.

The first implementation is serial. Benchmark before proposing optimization or
bounded worker processes. Record startup/replay/trace costs honestly rather than
promising a speed multiplier. No event skipping or new timing schedule. If serial
performance is inadequate, bring a measured, small follow-up proposal for review;
do not silently add a pool or weaken recording/observation guarantees.

Visual mode validation is command-line CPU work with prepared world alpha masks/depth/IDs and no window.
If those assets are unavailable, report the check unexecuted. No desktop automation
or new GPU/presentation verification is needed for this Python tooling scope.

## Implementation sequence and review gate

1. **Inputs and transport:** curate the three sensible party packages/manifests;
   add strict bounded expansion and identity; extend the existing client for
   deadlines/cancellation/cleanup without changing headless command semantics.
2. **Policies and trial lifecycle:** implement allowed-observation projection,
   shared targeting/navigation, three priority policies, reaction scheduler,
   coverage checks, progress watchdogs and explicit classification. Validate with
   small owned contracts and targeted real-runner decisions before long runs.
3. **Artifacts and reports:** stream engine traces plus decision explanations;
   implement bounded retention, reproducible single-trial rerun/replay recipes,
   per-trial metrics and correctly denominated aggregate reports with uncertainty.
4. **Opt-in review evidence:** manually exercise cancellation/failure handling,
   run the small comparisons, replay retained representative victories/defeats and
   reproducible policy errors, check deterministic policy reruns, and measure
   serial performance separately. Explain policy limitations before interpreting
   any apparent balance issue. No game tuning is part of this slice.
5. **Handoff:** write `docs/slice132_verification.md` with exact commands, results,
   artifact locations, measured costs, coverage/unsupported decisions and skipped
   checks. Update this design, system overview, automation execution record and
   roadmap together. Mark **Ready for review** only after implementation and
   required available checks; stop for user acceptance before 13.3.

Review commands:

```sh
python3 tools/arena_experiment.py validate experiments/arena/smoke.json
python3 tools/arena_experiment.py run experiments/arena/smoke.json \
  --runner cmake-build-headless-release/blobber_headless --assets assets \
  --output /tmp/blobber-132-review
python3 tools/arena_experiment.py report /tmp/blobber-132-review
python3 tools/arena_experiment.py rerun /tmp/blobber-132-review --trial TRIAL_ID \
  --output /tmp/blobber-132-one-trial
cmake-build-headless-release/blobber_headless --assets assets --mode diagnostic \
  --replay /tmp/blobber-132-review/traces/TRIAL_ID.jsonl
```

Acceptance is tooling usefulness and honesty: one explicit command compares the
three sensible party packages across multiple policies/seeds, including wizard-free
and spell-free variants; reports separate combat outcomes from tooling/coverage
limits; retained complete traces replay; policy choices repeat; budgets/cancellation
work; and no balance playthrough is pulled into ordinary tests. It is not a win-rate
target, universal party viability, or automatic approval of future balance changes.

Planning verification (2026-10-30): passed 261 local Markdown links/anchors across
the four changed planning documents, synchronized slice tracking, whitespace/final
newline checks and `git diff --check`. No gameplay tests, experiments, benchmarks
or desktop work ran for this documentation-only planning pass.


## Delivered implementation and verification

The approved architecture is implemented in the source locations above, with serial
execution and no C++ gameplay/protocol/rendering changes. The existing Python client
now bounds I/O/deadlines/cancellation; the separate `tools/arena_benchmark.py` measures
identical requests with recording on/off. Manifest validation checks all party/roster
combinations through `start` before play and rejects unsupported active loadouts.
Inputs that cannot reserve one whole trace/sidecar/report allowance reject before
launch. Request budgets reserve one request for orderly trace closure. Failure to
publish a trial row leaves its active trial identity and attempted count in batch
status; reports distinguish recorded rows from unrecorded started work.

All initial supported level-one policies, report categories, reproduction paths and
bounded retention are delivered. Routine CTest adds only `arena_experiment_tools`:
27 owned contracts, about 1.7 seconds, without the game or any full fight. The manual
`tests/arena_experiment_playtest.py`, all balance manifests, and benchmarks remain
outside CTest/default builds/CI. The requested separation is retained even for the
smallest smoke matrix. No accepted 13.1 regression test was removed.

[Verification](slice132_verification.md) records focused Debug/Release 5/5, fixture
win/loss/failure/cancellation/replay, an explicit 18-trial smoke and 216-trial
exploratory comparison, visual CPU play/replay without a window, deterministic policy
reruns, allocation stress and separate Release measurements. All final exploratory
trials won; some had casualties. No balance conclusion or tuning follows from eight
seeds per configuration. Later-level tactics and the listed unsupported spells stay
explicit limitations. No worker pool was warranted by the measured initial workload.

The user accepted and closed this slice on 2026-10-30.
[Accepted closeout](slice132_verification.md#accepted-closeout-and-next-agent-handoff).
13.3 is next eligible for separate planning; 13.3 and 14 remain unstarted.
