# PC03 verification and handoff

Status: **Accepted and closed**, 2026-10-30. Implementation approved by “Implement the
plan.” The user accepted after playing through the accessible floor, defeating the
guardian and collecting the loot: “I played through the floor that I can access and defeated the guardian, got the loot yay :) slice accepted, record it in the doc”.
Milestone P-A is Accepted. PC01/PC02 remain accepted; PC04 has not started.

[Design](procedural_guardians_design.md) · [Roadmap](procedural_campaign_plan.md) ·
[Capture gallery](screenshots/pc03/index.html) · [Protocol](headless_protocol.md#pc01-generated-floor-scenarios)

## Delivered behavior

Generated floors have one ordinary melee goblin, one optional melee goblin and a
designated guardian pair: a 14-HP placeholder melee lead plus an ordinary archer.
The lead retains the ordinary goblin's other stats, attacks, drops and 50 XP.
All three independent depths use this deliberately easy proof set. Optional supplies
are a healing potion and mana potion. No difficulty scaling or balance experiment.

The guardian approach still has its normal depth-1 door or depth-2/3 Bronze/Silver
keyed door. Its prompt warns of guardians and movement-locked combat before opening.
A separate sealed gate opens only after every designated guardian has a permanent
death record. Removing a living actor, defeating other enemies or losing awareness
does not count. Completion activates a reserved healing potion and camp kit beyond
the gate once. Full inventory and full player-drop storage cannot consume or block
those reservations. Gate/loot revision exhaustion rejects completion atomically.
Visible guardian hover text and the completion log explain the objective.

Both stairs rooms have initially closed, unlocked ordinary doors. Opened doors stay
open. Bent walls prevent sight/contact at their arrival cells even with every door
open; actors cannot enter the reserved room interiors. The shared passability rule
covers construction, checked moves, navigation and AI. Player drops are rejected in
those interiors. A checked read-only arrival query is available for PC04, which must
still implement transactional travel. Stairs themselves remain inert and say so.
Normal New Game is depth 1; other depths remain independent scenarios. There is no
hub, recall, persistent travel, campaign victory or save/resume in this slice.

Graphical/headless launch share generated population preparation. Player observations
expose objective state only after discovery and mark visible guardians; diagnostics
include exact designated member slots and reward ledger without teaching discovery.
Existing interact, actions, preview, client/MCP Sessions and strict replay are reused;
there is no objective-completion command or new MCP tool.

Generator/template versions are **3/3**; older generated scenarios/traces reject.
Protocol/scenario schema remain **1**. Prepared format is **4**, renderer revision
**5**; rebuild assets normally. Gate imagery shares existing door/threshold pixels;
no new bank, model, sprite or renderer redesign. The placeholder appearance alias
also works in the existing arena.

## Verification

| Check | Result | Evidence |
| --- | --- | --- |
| Full headless Debug / Release | 38/38 each passed | `/tmp/blobber-pc03-headless-debug-final-tests.log`, `/tmp/blobber-pc03-headless-release-final-tests.log` |
| Full graphics Debug / Release | 57/57 each passed | `/tmp/blobber-pc03-debug-final-tests.log`, `/tmp/blobber-pc03-release-final-tests.log` |
| Final diagnostic-caption correction | Focused generation/fixture-render checks passed in Debug/Release; generation passed in both headless presets | `/tmp/blobber-pc03-caption-*-tests.log` |
| ASan/UBSan, leak detection | 5/5 passed | `/tmp/blobber-pc03-sanitize-final-tests.log` |
| Engine-owned tracked runtime C++ allocations | Zero in exercised simulation/observation/objective/HUD/prepared-frame paths | `automation_allocations`, `frame_allocations` |
| Direct CLI/client and real local MCP Sessions adapter | Owned-fixture preview/open/repeat/replay and objective metadata agree | `generation_cli` |
| Checked guardian combat and strict replay | Passed fresh-process replay; quest/reward-ledger corruption rejected | `fixtures_automation`; retained compressed trace below |
| Hidden GPU lifecycle | Passed, NVIDIA GeForce RTX 3060/OpenGL | `/tmp/blobber-pc03-gpu.log` |
| Hidden application-owned captures | 21 retained PNGs, final images inspected | [Gallery](screenshots/pc03/index.html), [hash/dimension manifest](screenshots/pc03/retained.json) |
| Final repository checks | 49 C++ files formatted; changed Python syntax, whitespace, 211 local links/anchors, source/trace hashes and 21 PNG hashes/dimensions passed | `clang-format --dry-run --Werror`, `git diff --check`, local artifact validation |

Owned tests cover all depths and fallbacks, rotated definitions, bounded generation,
normal/key/guardian dependencies, optional paths, invalid placement/gate/exclusion
metadata and shared preparation failures. Guardian tests cover unrelated/partial
and multiple deaths, death then removal, living removal, foreign IDs, duplicate or
invalid bindings, full bags/drop pool, reserved rewards, repeated reconciliation,
pickup preview/repeat, normal contact movement lock and revision exhaustion.

Landing tests exhaustively compare both arrival cells against every permitted NPC
cell with all doors abstractly open. They also run initial session AI, then actual
melee/ranged pursuit toward a remembered arrival after opening the entry door,
checking exclusion and sight throughout. Existing recovery/combat suites continue
to exercise hazards, unresolved actions and ability checks. These are focused
correctness workloads, not a balance expedition or proof of production-scale speed.
The prepared fixture tests verify gate imagery/picking and stationary frame refresh
in four headings. Existing authored-map, arena, combat, effect and rendering suites
remain passing.

Resolved failures: early runs found obsolete empty-floor fixture counts/version-2
CLI data, an off-route unlocked-door validation gap, and two graphics-owned fixtures
still declaring one appearance after adding archer support. The fixtures/validator
were corrected; final full runs above pass. New test compile errors (XP member access
and a missing perception include) were corrected before those runs. Final image
review found an obsolete diagnostic-map caption claiming objectives unavailable;
that caption and the missing G legend were corrected, both graphics builds and
focused tests rerun, and all retained screenshots regenerated. No known unresolved
failure is being deferred.

### Commands executed

From the repository root; output was redirected to the logs above. No packages or
toolchains were installed.

```sh
cmake --build --preset headless-debug -j 3
cmake --build --preset headless-release -j 3
cmake --build --preset debug -j 3
cmake --build --preset release -j 3
ctest --preset headless-debug
ctest --preset headless-release
ctest --preset debug
ctest --preset release
cmake --build /tmp/blobber131-sanitize -j 3 --target blobber_fixtures_tests blobber_fixtures_automation_tests blobber_generation_tests blobber_generation_automation_tests blobber_guardians_tests
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /tmp/blobber131-sanitize -R '^(fixtures|fixtures_automation|generation|generation_automation|guardians)$' --output-on-failure
cmake --build --preset debug --target blobber_gpu_smoke -j 2
cmake-build-debug/blobber_gpu_smoke
cmake-build-headless-debug/blobber_generation_tests
cmake-build-debug/blobber_automation_allocations_tests
cmake-build-headless-debug/blobber_fixtures_automation_tests --retain /tmp/blobber-pc03-guardian-review.jsonl
cmake-build-headless-debug/blobber_fixtures_automation_tests --replay /tmp/blobber-pc03-guardian-review.jsonl
cmake-build-debug/blobber --assets cmake-build-debug/assets --development --seed 5 --capture-menu /tmp/blobber-pc03-captures-review
```

The reused sanitizer build uses `-fsanitize=address,undefined -fno-omit-frame-pointer
-fno-pie` and linker `-no-pie`. After the final caption-only change all four presets
were rebuilt; ran `ctest --preset debug -R '^(generation|fixtures_render)$'` and the
same in Release, plus `ctest --preset headless-debug -R '^generation$'` and the same
in headless Release. No simulation changed after the full/sanitizer runs.

## Bounds and memory

Retained generated definition plus 36×36 map payload is **5,264 bytes**, **5,568** at
the 1,600-cell bound, below 16 KiB. Accounted scratch is **34,576 bytes**, below
128 KiB. Generation retains caps of 32 population records, 32 fixtures, 16 attempts
and a validated fallback. The bounded progression search has four states per cell
(key obtained × guardians defeated). Session objective storage binds at most 32
nonrecycled member slots and two reward reservations. NPC exclusion is a fixed
1,600-bit mask with constant-time lookup, allocated outside frame processing.

Generation preparation requests **39 C++ allocations / 3,817 bytes** in the owned
allocation workload. Normal launch accounts scene **201,348,816**, NPC **69,010,094**,
projectiles **1,012,160**, loot/fixture markers **2,503,928**: **273,874,998 bytes**
against **536,870,912**, unchanged from PC02 because art is reused. These are engine
accounting figures, not process RSS or driver memory. JSON/export, C libraries and
graphics driver allocations are outside the tracked runtime C++ guarantee.

## Review and reproduction

The gallery includes closed/open stairs door, guardian warning/pair/hover identity,
sealed gate, partial defeat with gate still sealed, completed gate/reward and reward
closeup. Nine PC03 views have 1920×1080 and application-exported 1280×720 versions;
three normal launch/discovered-map/diagnostic-map views are 1920×1080.

**Capture provenance:** seed 5, depth 1, generator/templates 3/3, fingerprint
`1178095383652031389`, one attempt, no fallback. The PC03 views are independent
scene/HUD diagnostic reconstructions from explicit poses. The capture routine applies
authoritative actor damage to demonstrate partial/full objective presentation; it is
not a continuous fought expedition. Normal launch views use the party-picker capture
flow. Every window and scripted interaction belongs to the application; there was no
OS input injection, focus change or desktop automation.

The separate [compressed owned-fixture trace](screenshots/pc03/guardian-review.jsonl.gz)
records normal checked movement, key/door interaction and combat through guardian
completion. It uses test-owned rules/appearances, not shipped-content difficulty.
Replay it with the matching test executable:

```sh
gzip -dc docs/screenshots/pc03/guardian-review.jsonl.gz > /tmp/blobber-pc03-retained.jsonl
cmake-build-headless-debug/blobber_fixtures_automation_tests --replay /tmp/blobber-pc03-retained.jsonl
```

For interactive user review, start the normal game with `--development --seed 5`,
choose four characters, and explore from the bent entry alcove. The first door needs
no key; opening it must not immediately start contact at the arrival cell. Inspect
optional routes, read the guardian warning, and fight the designated pair. The gate
must remain closed after unrelated or partial kills, then open and reveal supplies
after both guardians die. The next stairs-room door is also unlocked; stairs travel
remains unavailable. M shows discovered space; development reveal is diagnostic.
Depths 2/3 and forced fallback are covered by owned tests as separate floors.

## Limits and next-agent handoff

The user completed the accessible floor through guardian defeat and loot collection
and accepted the slice on 2026-10-30. This does not establish interactive coverage of
the other depths or actual floor travel. Live Codex-host MCP remains unverified.
Hidden application captures and local adapter tests passed. No balance experiments, broad
seed sweep, complete expedition, actual floor revisit/travel transaction or campaign
save/load was run or implemented. Easy placeholder encounters are intentional; more
monsters and tuning belong to later approved work. Basic reused gate/door art is not
final visual quality.

Preserve this worktree and the linked implemented design. The implementation entry
tree was clean, and no commit/reset/stash was made. Core ownership
is in `GameSession`/`FloorGeometry`; generated preparation is shared by graphical and
headless launch. `landingSafety` is a query, not travel authorization by itself:
PC04 must prepare destination ownership and commit the party/floor change atomically.
Do not reopen accepted PC01–PC03 or begin PC04 automatically. PC03 and P-A are
accepted; next action is to plan PC04 only for review in a fresh context when requested.
This closeout changed documentation only and preserved all existing working-tree
changes. No builds, gameplay tests, captures, balance experiments or commits were
made during the acceptance update; recorded verification results remain unchanged.
