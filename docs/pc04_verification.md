# PC04 verification and handoff

**Stage / last updated:** **Accepted and closed**, 2026-10-30.
User: “slice accepted, record it in the plan doc”.
**Approved plan:** User: “Implement the plan.”
[Design](procedural_campaign_travel_design.md) · [Roadmap](procedural_campaign_plan.md) · [Gallery](screenshots/pc04/index.html)

## Delivered

Normal New Game uses the existing four-character picker, constructs one travelling
Party, eagerly prepares all three seeded floors and starts in an authored 9×9
masonry sanctuary hub. Its rest/arrival cell is (4,4); the entrance is (5,4).
F/click explicitly uses stairs; arrival never travels automatically. Ordinary
walking connects hub ↔ 1 ↔ 2 ↔ 3 for free, including with zero kits. Guardian
completion gates downward travel; floor 3's designated guardian completion wins
once pending consequences settle. Command admission closes immediately on final
objective completion; existing impacts/presentation drain without new AI, movement
or automatic turns. Defeat takes precedence. Terminal campaigns
freeze gameplay and offer Main Menu, with no restart/resume of that live run.
Arena rematches and standalone investigation scenarios remain independent.

`CampaignRun` owns Party, stable maps/definitions, active session and inactive
value records. `GameSession` has owned and borrowed Party construction paths.
Floor export/import retains actor lifecycle, discovery/doors/keys, AI intent/RNG,
loot/drop generations and death ledgers, objective/reward state, XP credit,
local clocks/effects and readiness. Inventory identity survives; runtime actor,
fixture, effect and loot identities change. Unfinished navigation work is rebuilt
within ordinary bounded requests, without restore-time simulation or RNG draws.

Travel stages both simulation and view before a nonthrowing commit. Source
mutation/actor/inventory/Party stamps invalidate stale tickets. UI intent is
consumed immediately at the same input boundary, before another simulation tick.
Old view borrowers die before the old session; maps, catalog, Party and prepared
banks outlive both. Held movement/confirmation/interact input must be released.
Failures preserve the live source and Party; there is no resource charge.

Portable party effects/concentration, conditions, temporary HP and readiness
travel with checked deadline rebasing. Local effects/RNG/clocks freeze. NPC/mixed
concentration and Web require explicit dismissal. Spiritual Weapon is
non-concentration and has a checked free safe-exploration dismissal action.
Independent Grease/NPC conditions remain local. Existing sanctuary/field recovery
and rest time skips execute through their existing authorities once; no arrival
healing, consumable refill, resurrection, respawn or reward duplication.

Automation adds schema-1 `campaign` scenarios, location/status/visited-entry/time
observations and transition/result events. Existing interact, preview, actions,
Python client, local MCP session tools and strict replay carry the feature.
Diagnostic integrity includes canonical field digests of inactive records and RNG;
player observations exclude those hidden records. Budgets remain bounded.
Protocol/scenario schema remain 1, generator/templates 3/3, prepared format/renderer
4/5. No new art bank or runtime image preparation. Normal New Game no longer uses
the static map/recovery launch; authored diagnostic assets remain because the
separate investigation modes still use them.

## Verification

| Final check | Result | Evidence |
| --- | --- | --- |
| Full headless Debug / Release | 39/39 each passed | `/tmp/pc04-final-headless-debug-tests.log`, `/tmp/pc04-final-headless-release-tests.log` |
| Full graphics Debug / Release | 58/58 each passed | `/tmp/pc04-final-debug-tests.log`, `/tmp/pc04-final-release-tests.log` |
| ASan/UBSan with leak detection | 3/3 passed | `/tmp/pc04-final-sanitize-tests.log` |
| Preparation allocation fault injection | 9 headless / 14 graphical points passed | `automation_allocations`, `/tmp/pc04-final-allocations.log` |
| Engine-owned steady-frame tracked C++ allocations | Zero in exercised post-travel simulation/HUD/prepared-frame paths | `automation_allocations`, `frame_allocations` |
| CLI/client and real local MCP session adapter | Campaign preview/travel/ascent/replay passed | `generation_cli` |
| Owned strict replay and corruption rejection | Passed, including fresh-process retained replay | `campaign`, [retained trace](screenshots/pc04/owned-travel.jsonl.gz) |
| Hidden GPU lifecycle | Passed, RTX 3060/OpenGL | `/tmp/pc04-gpu.log` |
| Hidden deterministic captures | 12 retained PNGs inspected, hashes/dimensions checked | [Gallery](screenshots/pc04/index.html), [manifest](screenshots/pc04/retained.json), `/tmp/pc04-captures-review.log` |
| Final repository checks | C++ formatting, Python syntax, whitespace and relative links passed | 40 C++ files; 187 relative paths checked at final closeout |

Final full runs include the final-objective closing-state regression and the
presentation/fault-injection corrections. They supersede intermediate/focused logs.

Owned campaign fixtures exercise checked movement/stairs through all three depths,
keyed descent, objective/reward/XP continuity, gear drops and exactly-once pickup,
advancement/formation, zero-kit travel, carried dead characters and terminal freeze.
All ten portable instance kinds and four condition kinds transfer. Other cases
cover local NPC Sleep/Grease freeze, concentration/mixed/Web restrictions, explicit
spectral dismissal, combined effect capacity, real spell cooldown and temporary-HP
rebasing/expiry, long-rest time once, actor and effect stale IDs, malformed
snapshot and landing-hazard rollback, stale staged Party/pose mutation, and view
allocation failure. Final completion is tested with unrelated actors still alive
and an accepted NPC walking tail: result waits for the tail, commands close and
AI time freezes while presentation drains. Existing PC03 tests continue to cover moved/pursuing enemies,
protected-room exclusions and ranged sight with doors open.

Strict owned-fixture hub→floor 1→hub replay passes in a fresh process. Reproduce
the durable 64,534-byte trace with:

```sh
gzip -dc docs/screenshots/pc04/owned-travel.jsonl.gz > /tmp/blobber-pc04-retained.jsonl
cmake-build-headless-debug/blobber_campaign_tests --replay /tmp/blobber-pc04-retained.jsonl
```
 Corrupting
an inactive-floor digest rejects replay. Real CLI/Python client and local MCP
Sessions adapter exercise start/preview/travel/ascent/replay through the engine.
Live Codex-host MCP interaction remains unverified.

Allocation checks run after preparation, including first post-travel simulation,
commands, HUD and prepared-frame refresh. They track ordinary/aligned C++ new;
they do not claim graphics drivers or C-library internals never allocate.
The preparation allocation-failure matrix checks **9 headless / 14 graphical**
allocation points and separately injects every allocation point in the exercised source/destination/view preparation and checks unchanged
source identity and canonical floor state.

Hidden GPU lifecycle passed on NVIDIA GeForce RTX 3060/OpenGL. Application-owned
hidden captures cover hub launch, floor-1 entry, hub return, sanctuary recovery,
defeat and a separately labelled victory HUD reconstruction. They use an isolated
temporary character library and in-process operations, never OS input/focus.
The victory image is presentation evidence, not a played final fight; owned CPU
fixtures establish the final-guardian lifecycle. Defeat uses diagnostic damage.

### Commands

From the repository root, with build/test output redirected to the logs above:

```sh
cmake --build --preset headless-debug -j 2
cmake --build --preset headless-release -j 2
cmake --build --preset debug -j 2
cmake --build --preset release -j 2
ctest --preset headless-debug
ctest --preset headless-release
ctest --preset debug
ctest --preset release
ctest --preset debug -R '^(campaign|automation_allocations|hud|menu|generation_cli|fixtures_render|automation_visual)$'
ctest --preset release -R '^(campaign|automation_allocations|hud|menu|generation_cli|fixtures_render|automation_visual)$'
cmake --build /tmp/blobber131-sanitize -j 2 --target blobber_campaign_tests blobber_fixtures_automation_tests blobber_guardians_tests
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /tmp/blobber131-sanitize -R '^(campaign|fixtures_automation|guardians)$' --output-on-failure
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /tmp/blobber131-sanitize -R '^campaign$' --output-on-failure
cmake-build-headless-debug/blobber_campaign_tests --retain /tmp/blobber-pc04-owned-travel.jsonl
cmake-build-headless-debug/blobber_campaign_tests --replay /tmp/blobber-pc04-owned-travel.jsonl
cmake-build-debug/blobber_automation_allocations_tests
cmake --build --preset debug --target blobber_gpu_smoke -j 2
cmake-build-debug/blobber_gpu_smoke
cmake-build-debug/blobber --assets cmake-build-debug/assets --development --seed 5 --capture-menu /tmp/blobber-pc04-captures-review
```

The reused sanitizer build has `-fsanitize=address,undefined
-fno-omit-frame-pointer -fno-pie` and linker `-no-pie`. No package/toolchain install.
Routine experiment-tool contract tests are fixture tests; no balance experiment,
smoke matrix, broad seed sweep or full expedition experiment was run.

Resolved verification issues: initial compilation exposed missing optional NPC
placement ownership and a mutable automation accessor; both were corrected.
New test namespace errors were corrected before executed verification. Capture
inspection found an obsolete hub depth label, unsupported dash glyph and leftover
encounter-result hints; final presentation uses plain labels, explicit sanctuary
recovery and terminal Main Menu text/actions. Final lifecycle inspection added the
closing-state guard so pending presentation cannot start new AI/automatic turns;
its regression passes in all four presets and sanitizers. No failing acceptance criterion is
intentionally deferred.

## Bounds and review

Four floor snapshots reserve at most **943,840 bytes** of fixed value payload
(`FloorState` is 235,960 bytes); generated definitions/maps and bounded immutable
placements are additional. Only active/staged sessions hold navigation/event/view
scratch. Preparation may allocate; normal frames do not grow storage. Existing
prepared banks are loaded once and shared through every travel: scene 201,348,816,
NPC 69,010,094, projectiles 1,012,160, loot/fixtures 2,503,928, total
**273,874,998 bytes** against the existing **536,870,912-byte** prepared budget.
These are engine accounting figures, not process RSS or driver memory.

User review:

1. Launch normally with `--development --seed 5`, choose four characters and verify
   the sanctuary hub, R recovery hint and F entrance destination.
2. Enter floor 1, explore/loot/fight normally, alter gear/formation and return via
   upstairs. Re-enter and verify discovered space, deaths, doors and resources persist.
3. Rest explicitly at the sanctuary arrival cell; verify free living-character
   recovery without kit refill or resurrection. Walking stairs remain free when kits
   are exhausted. Dismiss local concentration/summons when eligibility asks for it.
4. Defeat the designated guardians, descend to floor 2, obtain its Bronze key, then
   revisit floor 1 and return. Deeper ascent arrives at the previous objective-side
   landing rather than at a departure bookmark.
5. Review terminal lifecycle fixtures/captures and the explicit limits below.

## Limits and next-agent handoff

Interactive play coverage and a played full campaign remain unverified here. Easy
PC03 placeholder encounters remain unchanged; no balance confidence is claimed.
No recall, waypoint selector, disk save, content tuning, respawn or expedition
policy is delivered. A menu return discards the in-memory run. PC01–PC03 and P-A
stay accepted; PC04 was accepted by the user on 2026-10-30. PC05 is now eligible for detailed
planning only, when requested; its plan still requires review before implementation.

Entry points: `CampaignRun`, `GameSession::exportFloor/importFloor`, campaign
scenario handling and shared graphical/headless stair transaction adapters.
Floor import is preparation-only and never reconciles against the shared Party;
preserve this boundary when PC05/PC06 extend the state model. New snapshot fields
must enter canonical diagnostics and validation as well as transfer. Inactive
snapshots intentionally have no authoritative travelling-party effects: those
are overlaid from the active Party's effects/readiness at each transition.

The implementation-entry worktree was clean. No commit, reset, stash, agent
configuration change, desktop automation or balance experiment was performed.

**Acceptance:** User: “slice accepted, record it in the plan doc” (2026-10-30).
PC04 is Accepted and closed. Verification results and limitations are unchanged;
no additional interactive play-coverage details were supplied. This closeout edits
documentation only, preserves the worktree and runs no builds/tests/captures or
balance experiments. No commit was made. Next action: plan PC05 only for review
in a fresh context when requested; milestone P-B remains Planned.
