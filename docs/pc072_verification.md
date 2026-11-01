# PC07.2 — Population, route evidence and handoff

2026-11-01. **Accepted and closed**, including subsequent review corrections. User:
“slice accepted, record it in the plan doc”. Implementation approval: “Implement the
plan”, including the eight-case bounded diagnostic study. The original delivery
evidence below remains historical; see the [current acceptance handoff](procedural_campaign_plan.md#pc072-accepted-closeout-and-next-agent-handoff-2026-11-01-current).
[Design](pc072_population_design.md) records the approved behavior and budgets.
PC07.1 and the earlier campaign, renderer and bestiary slices remain closed.

## Delivered behavior

Three depth bands use existing normal monsters: two goblin pairs/support guardian;
undead pair/support group/support guardian; goblin/undead/goblin pairs/bugbear-support
final guardian. Optional reserves are rats/goblins/undead. Intended direct-route XP
is 400 + 500 + 300 before the final guardian, then 300 more. Four survivors can
advance normally to level two before the final guardian. Optional XP totals 275;
clearing every optional group is not necessary. Ordinary fights remain bypassable;
the intended-route interpretation explicitly replaces compulsory XP gating.

Fixed pre-guardian caches contain 2 healing potions/4 mana potions/20 arrows; reserve
caches contain 1/2/10 respectively. Floors 1–2 retain one potion/one kit behind the
gate; final victory has no consumable cache. Starting resources and monster drops
are unchanged. Main caches are upstream of the guardian door. Closed ordinary
population-room doors use existing interaction, geometry, discovery and AI behavior.

Generation retains fixed records for at most five encounters/32 actors, route and
supplies. Shared bounded population resolution serves launch and disk validation.
Campaign schema/simulation are 5/8; generator/templates are 5/5. Older incompatible
saves reject unchanged. Arena schema 2/protocol v1 and asset formats are unchanged.
Only diagnostic campaign observations expose the active floor's hidden route and
population definitions. One Party and once-only reward/XP state persist normally.

## Checks performed

Resource checks before costly work: 19 GiB available RAM, 458 GiB free workspace;
/tmp is RAM-backed. Jobs ran serially at `nice -n 15 ionice -c 3`, at most two build
workers and one test process. Scratch uses `output/pc072/tmp`.

- Release application/headless/generation and affected test targets built. Existing
  application dependencies republished art; no source art or preparation format
  changed. Production totals remain 375 textures, 329,839,864 mip bytes and
  59,422,180 alpha bytes.
- `ctest --preset release -R '^(generation|population|campaign_route_tools|generation_automation|generation_cli|fixtures|fixtures_automation|guardians|campaign|campaign_save_smoke|campaign_save_cli|automation|automation_allocations|progression_allocations|arena_experiment_tools|headless_cli)$'`
  passed 16/16, 7.06 s. This selects changed generation/content, geometry/objectives,
  campaign persistence/transport/replay, typed allocation and Python tooling callers.
- Final population-table refactoring and fixture corrections:
  `ctest --preset release -R '^(generation|population|campaign_route_tools|automation_allocations)$'`
  passed 4/4, 0.54 s. Fast Python route tests now cover seven cases, including path
  occupancy/doors/gates, transfer accounting, clock epochs, reaction-cadence contact
  changes and explicit presentation-tail time advancement.
- Extended persistence checkpoint:
  `TMPDIR=/home/bison/Projects/blobber/output/pc072/tmp ctest --test-dir cmake-build-release -R '^campaign_save$' -j 1 --output-on-failure`
  passed 1/1, 4.59 s. Includes new encounter-range/route/cache tampering and old
  simulation rejection cases alongside existing effect/travel/failure coverage.
- The owned full route protocol integration check passed, finally 18.00 s:
  `TMPDIR=/home/bison/Projects/blobber/output/pc072/tmp ctest --test-dir cmake-build-release -R '^campaign_route_fixture$' -j 1 --output-on-failure`.
  It owns harmless one-HP enemies and generated party/content; it is correctness
  evidence, not a balance trial. It is explicitly labelled slow/integration and
  excluded from routine presets. Fast path/accounting tests remain ordinary tests.
- The allocation guard reported zero tracked C++ frame allocations and checked
  preparation rollback at 23 stairs, 25 recall, 23 waypoint and 85 restore failure
  points. It does not intercept C allocation or driver/third-party internals.

Development failures were corrected before the passing checks: owned content needed
seven appearances and an ammunition ID; older assertions assumed four actors and
three doors. Two ground-effect fixtures had become colocated when actors became
pairs, and now use distinct cells. The route fixture caught moving-target waypoints,
zero-time presentation-tail loops and management attempts during cooldown tails.
A cadence regression check ensures descriptors are rebuilt after contact begins.

## Study and captures

[Full costs, input scenarios, journals and retained traces](pc072_routes/README.md)
and the [nine-image gallery](screenshots/pc072/index.html) retain the evidence.

Executed separately for cases 1–8, inspecting each before starting the next:

```sh
nice -n 15 ionice -c 3 python3 tools/campaign_route_review.py \
  --runner output/pc072/bin/blobber_headless --assets cmake-build-release/assets \
  --output output/pc072/study --case N
```

The declared order was balanced seeds 5/8 direct/reserve, then no_wizard seeds 5/8
direct/reserve. All **8/8 won** with **32 observed downings**, **zero permanent deaths**,
**zero recorded encounter merges** and complete burst metrics. All reached level two
before the final guardian with three ordinary field long rests. Direct entries
had exactly 300 XP/member; reserve entries 368.75. No starting resource reset,
artificial XP, unlimited recovery or rare-drop prerequisite was injected.

Total trial wall time was **155.27 s**. All cases stayed below 120 seconds and 60,000
ticks; no request/decision/trace/output ceiling was raised or reached. Raw traces
total **1,322,479,568 bytes**. The complete study through both replays finished in
**679.48 s**, below thirty minutes. Retained study, captures and durable copies
occupied **1,500,517,481 bytes** at the report checkpoint, below three GiB. Eight
compressed traces total 142,502,683 bytes and retain uncompressed SHA-256 hashes.
No extra trial, repeated trial, sample expansion or broad seed sweep ran.

Strict replay plus hidden application capture used the pinned study binary, with
`--assets cmake-build-release/assets --mode diagnostic --replay TRACE` plus:

- balanced-8-reserve: `--capture-dir output/pc072/captures-balanced` and repeated
  `--capture-operation` values **464, 1341, 3387, 3844, 5227**.
- no_wizard-8-direct: `--capture-dir output/pc072/captures-no-wizard` and values
  **766, 1382, 1940, 2457, 3145, 3231**.

Both returned `{"replay":"matched"}` and closed their hidden OpenGL contexts/resources
successfully. Native first-person scenes, ordinary downings, all three guardian
approaches, level-two resources and the actual fought victory screen were inspected.
Each point also exported diagnostic/discovered maps; the gallery retains native
gameplay views and six initial floor definitions. These are current-state HUD
reconstructions, not historical hover/log reproduction; no repeat-identical PNG
claim or full GPU parity-suite claim is made.

Capture inspection found two obsolete map-help hints saying travel was unavailable.
Those strings now explain adjacent stair landings. Retained images/replays use the
pinned pre-correction binary; this text-only correction changes no campaign behavior.
The final Release application/headless build passed. A new player/diagnostic
regression check confirms public generation identity does not include hidden route
or encounters, and inspection does not teach discovery. Its first assertion was
too strict (it rejected already-public seed/version identity) and was corrected.
Final `ctest --preset release -R '^(population|generation_automation)$'` passed
**2/2**, 0.12 s. `generation` separately passed after the map-help correction.

## Findings and next-agent handoff

The intended route was viable in these four direct cases, with finite resources
carried through real rooms/doors and normal advancement. Reserve detours granted
extra XP/supplies but cost more mana potions and generally more downings under this
aggressive policy. They are not established as universally beneficial recovery.
Frequent downings remain an explicit human pacing/approach/rescue review point.

Preserve all eight retained cases, including the downing outliers. The matching
study runner is `output/pc072/bin/blobber_headless`; current builds deliberately
reject its traces by strict binary identity. The recorded tool and content hashes
are retained, and the current tool has the recorded script identity. Do not rerun
or expand the study without a new explicit opt-in. Parent PC07/P-C remain incomplete;
PC08 planning follows user acceptance, not this delivery record.

## Review and remaining limitations

Normal New Game starts in the hub at level one. Fight the intended ordinary route,
collect guaranteed supplies, use legal field recovery and explicit advancement, then
fight the visibly designated guardians. Compare a reserve detour, free walking
ascent/entry return and save/resume. Floor three's bugbear support is the final
challenge. Earlier floors contain no bugbears. All art uses the prepared-image path.

The focused tool uses diagnostic map/actor knowledge, aggressive-v2 tactics and
fixed reaction timing. It does not provide player exploration agents or measure
human fun. No broad seed sweep, new arena matrix, new monster statistics, benchmark,
Debug/Release matrix, new sanitizer run, package installation or OS input/focus automation is implied.

Manual review can use `cmake-build-release/blobber --seed 8 --campaign-save
output/pc072/manual-review.json` with a new party, without touching the normal save
slot. The normal creation/library flow remains the source of manual party builds.
Human interactive play/fun and live Codex-host MCP remain unverified. No commit,
reset, stash or installed configuration change was made. The implementation originally stopped at Ready for review; user acceptance is recorded above.

Final artifact checks passed: `git diff --check`, Python byte-compilation, 171
relative Markdown file links and 29 JSON files at the validation checkpoint, eight
complete route records with level-two final entry, trace byte counts and the two
matched replay results. Documentation-only closeout does not add gameplay evidence.
