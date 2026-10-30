# PC06 verification and handoff

**Stage / last updated:** **Accepted and closed**, 2026-10-30.
**Approved plan:** User: “Implement the plan.” Save + Save and Main Menu; a new
campaign keeps the old slot until its first confirmed save succeeds.
[Design](procedural_campaign_save_design.md) · [Roadmap](procedural_campaign_plan.md) · [Gallery](screenshots/pc06/index.html)
**Acceptance:** User accepted the save/load slice on 2026-10-30, including review corrections. Accepted PC04–PC06 complete milestone P-B; PC07–PC09 and milestone P-C remain Planned. Verification results and limitations are unchanged; no additional interactive play-coverage details were supplied.

## Delivered

Normal campaign pause offers Save, Save and Main Menu, recovery, and confirmed
unsaved Main Menu/Quit. Native close also produces a confirmation for an active
campaign or unresolved terminal marker. Save availability uses one shared stationary,
settled safe Exploration query; moving/queued commands, combat/approach, pending
impacts/presentation, party-cell hazards and terminal runs reject. Saving in place
preserves local concentration, Web and Spiritual Weapon, including their continued
departure restrictions. Opening, cancelling and previews never save or spend time.

The Main Menu validates the single slot and shows Continue with location and party
summary only when available; unavailable Continue and its explanation are hidden.
Continue bypasses the picker/library and creates no starting party or
supplies. A different run's first save requires confirmation; cancelling or failing
leaves the old file intact. Saved campaign builds remain usable after starting
library entries are deleted. Current levels 1–3, spells/portraits/names, resources,
formation and exact inventory placement/generations are independent values.

CampaignCheckpoint retains all four actual location definitions and provenance,
including unvisited layouts; live active state supersedes its older travel snapshot.
All actors/lifecycle, AI/RNG, loot/death/XP/objective ledgers, doors/keys, discovery,
waypoints, recall expenditure, effects/readiness, clocks, fractional time and active
pose survive. Navigation continuation retains bounded search progress/scheduling.
Load stages validation, Party, locations, active session and visibility/presentation
before publishing owners. Fresh runtime epochs reject old requests. Neither offline
time nor loading advances clocks, expires/casts effects, heals, or grants rewards.

Schema/simulation version 1 uses content names and catalog/NPC metadata identities;
unsupported versions/content reject without migration. Files are bounded to 16 MiB,
JSON nesting to 32 and collections to existing capacities. Integer/enum/float/RNG,
ownership/reference, layout/progression, key/guardian and clock validation precede
publication. The upper half of the uint64 clock range is reserved for deadline
arithmetic; other counters/generations keep lossless checked uint64 encoding.

CampaignStore validates a restorable candidate before replacing the slot. Locked
same-directory exclusive temp write, flush/close, atomic rename and directory flush
retain the old file before publication. Publication and durable completion are
separate results. Save and Main Menu stays in play on failure or uncertain durability.
Ordinary file corruption, including invalid UTF-8, is a recoverable rejection;
bounded ASCII-safe error text cannot break the JSON transport.

Terminal campaigns immediately deny matching Continue in process and attempt a
matching-run terminal marker once at an I/O boundary. Failures offer Retry and
explicit acknowledgement before leaving. A single denied slot association and an
unresolved-read flag bound this state: it does not accumulate ended-run history.
An unsaved unrelated run cannot invalidate the slot or forget a pending denial.
An acknowledged failed marker can leave an older active file after process restart;
the UI reports this limitation honestly. Manual copying/forced termination is outside
this guarantee, and no general autosave or anti-copying mechanism is added.

## Automation and integrity

Headless `--campaign-save FILE` explicitly enables save_status/preview_save/save/load/
retry_terminal. Without it, writes stay disabled. Current tokens and observed slot
revision bind saves; explicit live replacement binds loads. Success rotates session
identity; resumed sessions reject reset because they have no starting scenario.
Allocation failure while preparing replacement visibility returns a recoverable
response and retains the old session/tokens. Existing arena modes remain supported.

MCP sessions use isolated slots and bounded input/output artifacts; fresh resume
accepts a campaign_save_path. Successful load rotates the handle. Publication
failures return no newly published artifact; cancellation/transport failure can
still mean a mutation completed before its response. Reservation is 290 MiB for
trace, maximum slot/temp and metadata. No user campaign slot is implicitly written.

Strict traces retain external I/O/identity/results in 256 KiB raw-byte chunks,
hexadecimal encoded within existing record limits. This includes corrupt binary
inputs. Replay uses the recorded backend without touching the configured slot;
256 MiB trace and protocol line budgets remain. Saves/traces contain hidden diagnostic
state; player policies must not use artifacts as discovered navigation knowledge.
Protocol/scenario remain 1; generator/templates 3/3 and prepared format/renderer 4/5
are unchanged. Save compatibility is independent of strict executable-sensitive replay.

## Verification

| Check | Result / evidence |
| --- | --- |
| Full headless Debug / Release | 41/41 each; `/tmp/pc06-delivery-headless-debug-tests.log`, `/tmp/pc06-delivery-headless-release-tests.log` |
| Full graphics Debug / Release | 60/60 each; `/tmp/pc06-delivery-debug-tests.log`, `/tmp/pc06-delivery-release-tests.log` |
| Focused ASan/UBSan + leak detection | 3/3: allocation, campaign, campaign_save; `/tmp/pc06-delivery-sanitize-tests.log` |
| Final headless replacement-allocation follow-up | automation/campaign_save/campaign_save_cli 3/3 in all four presets; headless Release `/tmp/pc06-load-rollback-tests.log`, others `/tmp/pc06-final-rollback-*-tests.log` |
| Final sanitizer follow-up | 1/1 passed: final campaign_save rerun; `/tmp/pc06-final-rollback-sanitize-tests.log` |
| Engine-owned steady-frame C++ allocations | Zero after restored-run preparation, including simulation, save/menu previews and graphical composition |
| Restore/view allocation failure injection | 55 headless / 60 graphical allocation points; valid source owner retained at every exercised point |
| I/O fault injection | Read/write/flush/close/rename/directory flush, stale revisions, terminal failure/retry and unrelated-run isolation |
| CLI/Python/local MCP | Owned fresh-process resume, invalid UTF-8 rejection without losing the session, resource/unlock retention and strict replay pass |
| Optional SDK stdio | Existing legacy/native lifecycle contracts and new save/preview/artifact/load/rotated-handle/replay tools pass; `/tmp/pc06-delivery-sdk-legacy.log`, `/tmp/pc06-final-sdk-save.log` |
| MCP adapter contracts | 19/19; `/tmp/pc06-delivery-mcp-contracts.log` |
| Cross-build save | Owned Release save loads in Debug with equal party/resources/pose/unlocks; `/tmp/pc06-retained-verification.log` |
| Retained strict replay / corruption | Matches with retained owned assets and no original slot; rejects poisoned inactive fingerprint |
| Hidden GPU lifecycle | RTX 3060 / NVIDIA OpenGL 3.3; `/tmp/pc06-delivery-gpu.log` |
| Deterministic captures | All 19 images match independent repeat byte for byte; 14 retained and representative native/reduced views inspected |
| Final docs/format/artifacts | 209 relative links/anchors, 55 C++ formatting checks, 14 PNG and seven owned fixture hashes, current producer identities and Python syntax pass; `/tmp/pc06-document-validation.log` |

Owned cases include all depths before/after key and guardian transitions, non-entry
poses, level-2/3 advancement, recall/unlocks, every portable instance/condition,
local mixed concentration, positioned spectral weapon, nonempty Grease/Web/restraint
references, NPC Sleep/Guiding and frozen local versus rested party clocks. A small
checked encounter validates save after actual combat/XP and ordinary victory
acknowledgement; this is an invariant fixture, not a balance run. Navigation fixtures
retain a partially processed BFS, then compare result timing and bounded work.

Malformed cases reject truncation, excessive size/depth, duplicate keys, unknown
schema/content, duplicate inventory/formation ownership, invalid enums/coordinates,
RNG/clock/reference/capacity and definition mismatches without partial load. UI cases
cover keyboard/click Save and Save and Main Menu, overwrite cancel/confirm, blur,
native-close intent and terminal-warning acknowledgement. Existing input-edge and
travelling-party regression suites remain intact. The final replacement allocation
follow-up was added after the full suites; it changes tests plus the protocol's
bad_alloc response handling and is covered by the focused reruns above.

The allocation guard intercepts ordinary/aligned C++ new after explicit preparation;
it does not claim C libraries, raylib or graphics drivers never allocate. JSON,
filesystem work, preparation and capture/export may allocate. Fault counts cover the
selected owned restore/view workload, not every conceivable content combination.

### Executed commands

From the repository root, logs redirected under `/tmp/pc06-*`:

```sh
cmake --build --preset headless-debug -j 2
cmake --build --preset headless-release -j 2
cmake --build --preset debug -j 2
cmake --build --preset release -j 2
ctest --preset headless-debug
ctest --preset headless-release
ctest --preset debug
ctest --preset release
cmake --build /tmp/blobber131-sanitize -j 2 --target blobber_campaign_save_tests blobber_campaign_tests blobber_automation_allocations_tests
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /tmp/blobber131-sanitize -R '^(campaign_save|campaign|automation_allocations)$' --output-on-failure
cmake --build --preset release -j 2 --target blobber_gpu_smoke
cmake-build-release/blobber_gpu_smoke
.venv/bin/python tests/mcp_stdio_tests.py cmake-build-headless-release/blobber_headless
.venv/bin/python tests/campaign_save_mcp_tests.py cmake-build-headless-release/blobber_headless
cmake-build-release/blobber --assets cmake-build-release/assets --development --seed 5 --capture-campaign-save /tmp/blobber-pc06-delivery-captures
cmake-build-release/blobber --assets cmake-build-release/assets --development --seed 5 --capture-campaign-save /tmp/blobber-pc06-delivery-captures-repeat
```

Final protocol follow-up rebuilt campaign_save/headless targets and ran
`ctest --preset PRESET -R '^(automation|campaign_save|campaign_save_cli)$'` for each
of the four presets. The sanitizer follow-up rebuilt campaign_save and reran that
suite. The existing sanitizer directory is graphics-disabled Debug with
`-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie` and linker
`-fsanitize=address,undefined -no-pie`. No packages or toolchains were installed.
Neither SDK invocation uses `--playtest`. No balance matrix, broad seed sweep,
automated expedition or later-slice implementation ran.

## Retained evidence and storage

The [manifest](screenshots/pc06/retained.json) records PNG/fixture hashes and producer
identities. The [gallery](screenshots/pc06/index.html) shows actual save/load/recall,
injected rename failure and terminal-marker retry. Injury and defeat are diagnostic
setup. The overwrite panel is a presentation setup; owned CPU tests exercise the
actual distinct-run confirmation. A fresh separate run is saved after terminal
capture to show Main Menu Continue. No screenshot claims a fought expedition or
human play coverage, and no OS input/focus automation was used.

The owned saved campaign is 1,437,457 uncompressed bytes; the owned strict trace is
12,237,531 bytes. Compressed copies and small owned rules/NPC metadata are retained.
Reproduce strict replay with the matching headless Release executable:

```sh
gzip -dc docs/screenshots/pc06/owned-save.jsonl.gz > /tmp/blobber-pc06-replay.jsonl
cmake-build-headless-release/blobber_headless --assets docs/screenshots/pc06/owned-assets --campaign-save /tmp/blobber-pc06-unused-slot.json --replay /tmp/blobber-pc06-replay.jsonl
```

The configured replay slot is not opened or written. Binary/content identity
mismatch on a changed build is expected. For interruption testing, decompress
`owned-campaign.json.gz` into an isolated temporary file and use the normal load
operation against the retained owned assets. The retained trace is diagnostic and
has no prepared-bank assets for optional GPU replay capture.

Measured CampaignCheckpoint is 53,888 bytes excluding its owned dynamic payloads;
four FloorState payloads remain 943,840 bytes. Arrays/cells/actors/effects/loot keep
existing bounds, and only active navigation/view scratch is live. Prepared banks
remain shared: resident-accounted 273,874,998 bytes under the unchanged 536,870,912
budget in the hidden capture. These are engine accounting figures, not RSS or VRAM.
Save/restore JSON and staged owners are explicit temporary preparation storage.

## User review recipe

1. Use an isolated slot, for example `--campaign-save /tmp/blobber-pc06-review/campaign.json`.
   Start a normal four-person party in the hub. Pause, Save, and resume at the same pose.
2. Descend/explore, mutate resources/formation/gear and discovery, then Save and Main
   Menu. Exit/restart with the same slot and Continue; the picker must be bypassed.
3. Visit another depth, collect a key/guardian reward, recall once, save and restart.
   Verify exact kit count, unlocked entries, doors, discovered space and previous
   loot/deaths/XP on revisiting. Returning/Continue must not refill supplies or heal.
4. With a valid saved run, start another. The previous slot must remain until a
   confirmed save. Cancel replacement once, then confirm and Continue the new run.
5. Ordinary defeat/final victory must hide matching Continue. The retained
   failure panel shows Retry and acknowledged exit when terminal persistence fails.
   Owned failure fixtures cover the actual write faults; no permission changes or
   library deletion are needed for manual review.

## PC06 presentation review correction (2026-10-30)

Unavailable Continue is hidden. Confirmations and the write-failure panel use
compact, centered geometry shared with their click targets. The failure title is
“COULDN'T WRITE SAVE” with “Retry”; the body explains the failed write and the
possibility of an older save after restarting. Persistence behavior is unchanged.

Release `hud`, `menu`, `automation_allocations` and `campaign_save` checks pass
(4/4); the latter covers clicking the compact overwrite confirmation. Build/test
logs: `/tmp/pc06-ui-review-build.log`, `/tmp/pc06-ui-review-tests.log`.
Two fresh hidden application-owned capture runs produced 19/19 byte-identical
images; the 14 retained images and provenance are refreshed. The compact overwrite,
write-failure and main-menu captures were visually inspected. Capture logs:
`/tmp/pc06-ui-review-captures.log` and
`/tmp/pc06-ui-review-captures-repeat.log`. The headless Release executable identity
still matches the retained replay provenance. No balance experiments or desktop
input automation ran. Acceptance is recorded above; no additional interactive coverage details were supplied.

## Limits and next-agent handoff

Interactive human/native-close event review and live Codex-host MCP remain
unverified. Native-close intent has CPU coverage and the GLFW close-flag integration
was inspected locally; no OS close/input automation was attempted. Placeholder
encounters and recovery economics are unchanged. Save schemas require compatible
content; there is no migration, mid-fight save, general autosave or copy prevention.
A failed terminal marker may leave the previous file resumable after restart, with
an explicit warning and in-process denial. No balance confidence is claimed.

Ownership entry points are CampaignRun checkpoint/restore, Party/Inventory saved
values, GameSession validation/safe boundary, Navigation continuation, CampaignStore/
CampaignDisk, the app SaveFlow, and the existing protocol/client/MCP adapters. Do
not reconstruct saved runs through the starting-party or generator launch paths.
Preserve the active snapshot rather than its older travel record, the single Party,
logical references, portable/local clocks, pre-publication admission and borrower
lifetimes. Trace I/O is diagnostic and separate from campaign save compatibility.

Implementation began with a clean working tree. Existing changes were preserved;
no commits, resets, stashes, installed configuration changes or OS automation were
performed. PC06 is now accepted, completing P-B. PC07 is next for its own reviewed
detailed plan only, in a fresh context when requested; do not implement or advance
automatically. This acceptance closeout was documentation-only; no new builds, tests,
captures or balance experiments were run.
