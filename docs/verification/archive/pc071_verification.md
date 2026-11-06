# PC07.1 — Delivery, calibration and next-agent handoff

Historical checkpoint; raw run artifacts were pruned on 2026-11-05.
The pinned study runners, trace bodies and journals mentioned below are no longer
retained. Recorded hashes and outcomes remain provenance; exact local replay is unavailable.
[Archive index](README.md) · [Current workflow](../../verification_workflow.md).

2026-11-01. **Accepted and closed.** The user approved the
reviewed plan with “Implement the plan”, including the bounded experiment study.
PC07.1/PC07.2 splitting, a level-2 opening target and focused level 1–2 tactics
were explicitly selected. PC01–PC06/P-A/P-B and the renderer/bestiary remain closed.

[Implemented design](../../pc071_arena_calibration_design.md) ·
[Measured costs and recommendations](../../pc071_calibration/README.md) ·
[Capture gallery](../../screenshots/pc071/README.md) ·
[Six replay results and retained trace hashes](../../pc071_calibration/index.json).

## Delivered

Arena Setup now prepares uniform level 1–3 parties through a staged level/spell
flow with shared selection controls, optional empty preparation, recommendations,
review and cancel. Four level-one library snapshots stay independent. Actual
identity/build/formation changes reset preparation; returning the unchanged party
preserves it. Name, seed and roster edits retain preparation. Loading rejects
invalid choices before replacing setup; headless failed start preserves its owner.

Arena schema 2 / layout 1 records a target and incremental per-character spell
choices by content ID. Schema 1 remains supported with level-one defaults.
`ArenaPreparation` is fixed two-by-four storage. Pure preview validates transitions;
`prepareArenaParty` constructs starting gear/supplies once and uses Party advancement
to obtain full ordinary resources at the target threshold. Manual launch, headless
start and rematch/reset use this factory and immutable scenario. The existing
GameSession owning constructor is public and rejects a missing Party. Campaign
save, generator, monster stats and campaign population are unchanged.

Aggressive policy v2 adds contact-turn Action Surge, Preserve Life and Cunning
Action. V1 priorities remain unchanged. Unsupported active loadouts fail preflight;
unsupported decisions and timeout now stop a batch with partial evidence. Reports
add per-character resolved damage/healing, single-hit and bounded six-second/round
bursts, and distinguish sampled HP/downings, approach duration and total simulation.

## Executed checks

One Release configuration; at most two build workers and one test process.
All lengthy commands used `nice -n 15 ionice -c 3`. Before costly work, `free -h`
reported 19 GiB available and `df -h . /tmp` reported 459 GiB free on the workspace
disk; /tmp is RAM-backed. Study/captures use disk-backed `output/pc071/`.

- Built `blobber`, `blobber_headless`, arena/automation/allocation/progression/menu
  tests and the existing optional GPU review target. The final application build
  passed. Its existing dependencies regenerated portrait, creator and world
  publications; world totals remain 375 textures, 329,839,864 mip bytes and
  59,422,180 alpha bytes. No source art or publication format changed.
- `ctest --preset release -R '^(arena|progression|progression_allocations|menu|automation|automation_allocations|arena_experiment_tools|headless_cli)$'`
  passed **8/8**, 3.21 s. This selects preparation/advancement, setup, transport,
  replay callers and engine-owned allocation paths, without a campaign matrix.
- After focused setup/search/burst corrections,
  `ctest --preset release -R '^(arena|automation|automation_allocations|arena_experiment_tools)$'`
  passed **4/4**, 3.00 s. Python tooling contracts pass **30 tests**, including
  versioned coverage, feature priorities and burst boundary/capacity behavior.
- Final `ctest --preset release -R '^arena$'` passed **1/1**, 0.04 s, after adding
  a guard for the shared preparation flow. The arena check additionally guards the complete routed level-3 selection
  and commit for zero ordinary/aligned C++ allocations. The automation allocation
  guard covers pure arena validation and a prepared level-2 session. These guards
  do not intercept C allocation or graphics-driver/third-party internals.
- Owned CPU fixtures cover level 1–3 resources, exact starting inventory/three kits,
  earned charges, thresholds, portable choices, schema-1 defaults, malformed/
  missing/duplicate/wrong-class choices, stage cancellation/search-backspace,
  unchanged-party preservation, changed-formation reset, immutable inputs and
  rematch/reset reconstruction. Protocol tests start level 3 and compare exact
  state after rejected replacement and fresh reset.
- Hidden production UI review:
  `cmake-build-release/blobber_world_integration_gpu_tests cmake-build-release/assets output/pc071/preparation-ui --arena-preparation`
  passed and exported seven views. This new explicit mode returns before the
  ordinary GPU integration suite; it is not a claim that that full suite ran.
- Six selected retained arena traces replayed with `{"replay":"matched"}` using
  `output/pc071/bin/blobber_headless --assets cmake-build-release/assets --mode diagnostic --replay TRACE`.
  Selections include both retained downing cases. The binary was copied before the
  study and remains available locally with its recorded fingerprint.
- A further strict reconstruction of balanced L2 bugbear support, seed 8, exported
  operations **1, 196, 199, 246** through `--capture-dir` and repeated
  `--capture-operation`. Native UI, the 0/21-HP cleric and ordinary rescue to 10/21
  were inspected, alongside learning/empty-slot/final-review and arena setup.
  The UI gallery includes a 1280×720 export. Captures reconstruct current HUD/logs,
  not historical hover/log/feedback. No repeat-identical PNG claim is made.

Documentation/file checks passed: `git diff --check`, Python byte-compilation,
159 affected Markdown file links, all 16 study JSON files and uncompressed SHA-256
verification of 26 retained traces. Two obsolete renderer links in the dated
roadmap findings were converted to explicitly historical names.

Development builds caught a missing forward declaration and a misplaced test
fixture insertion; both were corrected before the passing checks. Existing arena
version-rejection tests now use an actually unsupported version, since schema 2
is deliberately supported. No unresolved build/test failure remains.

## Explicit study and results

Each recipe ran separately through the existing serial CLI, with inspection before
the next. Exact command pattern:

```sh
nice -n 15 ionice -c 3 python3 tools/arena_experiment.py run \
  experiments/arena/pc071/RECIPE.json \
  --runner output/pc071/bin/blobber_headless --assets cmake-build-release/assets \
  --output output/pc071/RECIPE
```

Executed recipes, in order: `goblin_pair`, `rat_pack`, `undead_pair`, `bugbear_solo`,
`goblin_support`, `bugbear_support`. Each used balanced/no_wizard × seeds 5/8/12 ×
aggressive v2: **36/36 completed, 36 victories, zero permanent deaths, three observed
downings**. No stalls, timeouts, unsupported decisions, policy/runner failures,
artifact errors or incomplete burst metrics. Action Surge was used in all six L2
trials; Cunning Action and Preserve Life were exercised through checked commands.

Limits: 60 s/trial, 450 s and 512 MiB/recipe, 45 minutes/3 GiB overall. Sum of batch
wall times **6.824 s**, sum of trial wall times **5.851 s**. Study/UI/battle captures
occupied **96,821,410 bytes** at the checkpoint. No limit was changed or exhausted;
no simultaneous build/test/experiment, extra trial, optimizer or broad sweep ran.

The [candidate table](../../pc071_calibration/README.md) is the decision artifact.
Notable evidence: a normal noncritical 14-damage lone-bugbear hit downed the L1
cleric; L2 bugbear support caused two downings, up to 29 total incoming damage,
up to two healing potions, and cleric six-second bursts of 14/15. Other recipes
were often cheap in incoming damage but could consume substantial mana/potions.
These are cost/risk observations, not a population acceptance or win-rate estimate.

Durable evidence retains every trial's scenario, decisions, resources and summary,
26 compressed engine traces selected by the existing first/fastest/slowest policy,
and six strictly matched replays. There were no combat defeats to retain. One
middle-duration successful downing trace (no_wizard bugbear_support seed 8) was
not selected by that policy; its metrics/decisions remain, and no extra fight was
run to reconstruct it. Strict replay requires the recorded binary/content identity;
a new build supports fresh experiments rather than translating old traces.

## Review and next-agent handoff

In ordinary Arena Setup, choose a party, select Prepare level/spells, choose level
2 or 3, complete required wizard learning, leave or fill new preparation slots,
review and commit. Save/load a schema-2 scenario, launch and rematch; resources
should reproduce the chosen level without new starting supplies. Changing party
formation resets to level one; canceling preparation leaves the prior setup intact.
The six ordinary scenario files under `experiments/arena/pc071/scenarios/` can be
copied into the UI's displayed scenario folder for manual review; this task did not
install them or touch the user's library/save files.

PC07.2 remains **Planned** and is next eligible for detailed planning when requested.
Plan population from the measured
costs, normal XP and real geometry. Four living members require **1,200 compulsory-
route monster XP** to reach level 2 before the final guardian. Retain finite easier
optional reserves and free walking recovery. Carry actual HP, mana, ammunition,
charges, supplies, effects and advancement between fights; fresh-party arena wins
cannot establish cumulative affordability. Budget supply rewards and then test
selected generated approaches, alerts/merges and direct/optional routes. Do not
select floor number as party level or silently introduce milestone XP/respawns.

Limitations: diagnostic knowledge, three seeds and one focused policy, empty new
preparation slots, open-arena approaches and fresh initial resources. Aggressive
tactics do not use Bless/Shield of Faith; automated level-3 tactics are unsupported.
No generated-route experiment, campaign population integration, depleted-entry
study, full graphics/headless Debug/Release matrix, sanitizer sweep, benchmark or
full save suite was run. Human interactive feel/fun and live-host MCP remain
unverified. No OS input/focus automation, installed configuration change, commit,
reset or stash was performed. PC07.1 is Accepted and closed; do not advance automatically.

## Accepted closeout (2026-11-01)

The user explicitly accepted the slice: “slice accepted, record it in the plan doc”.
PC07.1 is **Accepted and closed**. Verification results, measured costs, retained
evidence and limitations above are unchanged; no additional interactive coverage
was supplied. PC07.2 is next for its own reviewed detailed plan when requested.
Parent PC07 and milestone P-C remain incomplete. This documentation-only closeout
preserves all existing working-tree changes and runs no builds, tests, captures or
experiments. No commit, reset or stash was made.
