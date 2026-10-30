# PC05 verification and handoff

**Stage / last updated:** **Accepted and closed**, 2026-10-30.
**Approved plan:** User: “Implement the plan.” Dedicated Travel panel selected in review.
[Design](procedural_campaign_recall_design.md) · [Roadmap](procedural_campaign_plan.md) · [Gallery](screenshots/pc05/index.html)
**Acceptance:** User: “slice accepted, record it in the plan doc” (2026-10-30).
PC04 and P-A remain accepted. PC06 and milestone P-B remain Planned.

## Delivered

T or the labelled header control opens a paused dungeon recall panel, including
at full health. It identifies the sanctuary destination, one-kit charge and current
kit count, explains entry-only return and separate recovery, and requires Enter or
an explicit confirmation click. Cancel, opening and previews spend nothing. Missing
supplies or unsafe conditions disable confirmation with an authoritative reason.
Inventory retains its contextual T transfer action; other overlays/targeting keep
exclusive input. Held input is suppressed after owner replacement until release.

At the hub, F/click on the reachable entrance opens the selector. T requires the
same entrance reach/facing. A fresh run offers ordinary first descent; otherwise
only unlocked entry depths appear. Selection and confirmation are separate, with
three bounded rows. Dungeon stairs retain immediate checked interaction.

Recall consumes exactly one existing camp kit only when travel commits. It reaches
the hub's sanctuary landing without healing, resurrection, supply refill or clock
advance. Free waypoint return uses the original floor entry/facing, never a stored
departure portal. Only successful stairs arrival unlocks a depth. Zero-kit walking
and unlocked entry return remain free. Field rest still spends a kit for recovery
in place; sanctuary recovery remains explicit and advances time once.

The same CampaignRun staged transaction serves all three travel kinds. Requests
bind campaign revision/source world; recall additionally captures a kit handle and
inventory revision. Preparation stages an Inventory value copy and supplier derived
stats, source snapshot, destination session, portable effects/deadlines and view.
Commit rechecks source/Party/actor/inventory stamps, eligibility and destination
pose/landing before nonthrowingly publishing charge and ownership together. Import
never reconciles against the travelling Party during preparation. Old view borrowers
die before their old session. Any rejected/preparation-failed transaction leaves
the valid source and resources intact.

Floor-local clocks/RNG and mutations remain persistent/frozen while absent. Party
buffs, conditions, readiness and temporary-HP deadlines reuse PC04 transfer rules.
Local concentration, restraint, Web and Spiritual Weapon retain explicit departure
restrictions. Keys, doors, XP, loot/death/reward ledgers and discovery do not reset.

## Commands and integrity

Automation adds `{"kind":"recall"}` and `{"kind":"waypoint","depth":1}` through
normal preview/act tokens; explicit act confirms. `interact` stairs, including the
hub's ordinary first descent, remain available to tooling. Existing CLI/Python and
local MCP Session tools pass the new actions without a new service or policy.

`campaign.travel` and action descriptors show recall readiness/reason/cost/kits and
only unlocked waypoint depths with eligibility and zero cost. No inactive geometry
or actor state leaks into player output. Transition events add travel kind and
actual kit charge; `entry_unlocked` appears only on committed first stairs arrival.
Strict replay compares these events, resources, unlocks and inactive digests.
Protocol/scenario remain 1, generator/templates 3/3, prepared format/renderer 4/5.
Old traces remain executable-sensitive; no cross-build replay or save migration is
promised. Save/resume remains PC06.

## Verification

| Check | Result / evidence |
| --- | --- |
| Full headless Debug / Release | 39/39 each passed; `/tmp/pc05-headless-debug-tests.log`, `/tmp/pc05-headless-release-tests.log` |
| Full graphics Debug | 58/58 passed; `/tmp/pc05-debug-tests.log` |
| Full graphics Release | 58/58 passed; `/tmp/pc05-release-tests.log` |
| Focused ASan/UBSan, leak detection | 3/3 passed: campaign, recovery, hud; `/tmp/pc05-sanitize-tests.log` |
| Engine-owned steady-frame C++ allocations | Zero, including Travel controller/preview and post-travel frames; headless and graphics allocation logs |
| Preparation failure injection | Every exercised allocation: stairs/recall/waypoint each 9 headless and 14 graphical points; source, world, kit count and inventory revision retained |
| CLI/Python/local MCP session adapter | New actions, unavailable depth, preview, one-kit recall, entry return and strict replay passed in `generation_cli` |
| Owned strict replay | Passed, including a fresh process; `/tmp/pc05-owned-replay.log`; retained fixture trace below |
| Hidden GPU lifecycle | Passed on NVIDIA GeForce RTX 3060/OpenGL; `/tmp/pc05-gpu.log` |
| Deterministic hidden captures | 14 retained PNGs, native/reduced dimensions and exact second-run byte hashes match; representative panels and arrivals visually inspected |
| Final formatting, whitespace and links | 31 C++ files, Python syntax, whitespace, 223 relative links/anchors/gallery references and 14 retained hashes checked |

Owned campaign cases cover all three depths, field rest in place versus recall,
multiple/split supplier stacks, partial/last-stack consumption and derived weight,
stale worlds/handles/inventory/tickets, repeat/preview/cancel, unsafe phases and
presentation, local hazard, leadership and mixed concentration, summon/movement
restrictions, malformed destinations, unsafe landings and mutated staged poses.
Canonical floor digests match after recall and waypoint return; all ten portable
instance kinds and all four party condition kinds retain duration/state through
both new modes. Existing PC04 cases retain cooldown/temporary-HP and local-clock/RNG
coverage. Three-row selector browsing, confirmation, modal T routing, physical-T
Inventory transfer, focus/held-edge behavior and zero-kit ascent/return have owned
CPU regressions. These are functional fixtures, not encounter/balance experiments.

The final physical-T Inventory regression was strengthened after the broad runs;
`campaign` was rebuilt and passed again in all four presets and under ASan/UBSan.
See `/tmp/pc05-*-final-focused.log`. The final full runs above include the complete
feature, rollback, effect and automation cases; the follow-up changes tests only.

The allocation guard intercepts ordinary/aligned C++ new after preparation. It does
not claim C-library, raylib or graphics drivers never allocate. JSON/trace output,
transition preparation and image export are explicit allocation phases. Failure
injection covers all allocations in the selected owned preparation workload, not
all possible content combinations.

The retained [fixture trace](screenshots/pc05/owned-travel.jsonl.gz) is 136,677
uncompressed bytes. It records hub entry, paid recall, rejected repeat recall,
waypoint return and ordinary ascent using an owned catalog. Reproduce with:

```sh
gzip -dc docs/screenshots/pc05/owned-travel.jsonl.gz > /tmp/blobber-pc05-retained.jsonl
cmake-build-headless-debug/blobber_campaign_tests --replay /tmp/blobber-pc05-retained.jsonl
```

The [gallery](screenshots/pc05/index.html) uses real application-owned entry,
recall, waypoint and zero-kit ascent transactions. Injury and supply depletion are
explicit diagnostic setup. Recall is captured from the floor-1 entry, not a fought
expedition or a demonstration of pacing. No victory reconstruction is retained in
this gallery. [Manifest](screenshots/pc05/retained.json) contains provenance,
dimensions and image/trace SHA-256 hashes. No OS input/focus automation was used.

### Commands

From the repository root; output redirected to `/tmp/pc05-*.log`:

```sh
cmake --build --preset headless-debug -j 2
cmake --build --preset headless-release -j 2
cmake --build --preset debug -j 2
cmake --build --preset release -j 2
ctest --preset headless-debug
ctest --preset headless-release
ctest --preset debug
ctest --preset release
cmake --build /tmp/blobber131-sanitize -j 2 --target blobber_campaign_tests blobber_recovery_tests blobber_hud_tests
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /tmp/blobber131-sanitize -R '^(campaign|recovery|hud)$' --output-on-failure
cmake --build --preset debug -j 2 --target blobber_campaign_tests
cmake --build --preset release -j 2 --target blobber_campaign_tests
cmake --build --preset headless-debug -j 2 --target blobber_campaign_tests
cmake --build --preset headless-release -j 2 --target blobber_campaign_tests
ctest --preset debug -R '^campaign$'
ctest --preset release -R '^campaign$'
ctest --preset headless-debug -R '^campaign$'
ctest --preset headless-release -R '^campaign$'
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /tmp/blobber131-sanitize -R '^campaign$' --output-on-failure
cmake-build-headless-debug/blobber_campaign_tests --retain /tmp/blobber-pc05-owned-travel.jsonl
cmake-build-headless-debug/blobber_campaign_tests --replay /tmp/blobber-pc05-owned-travel.jsonl
cmake-build-headless-debug/blobber_automation_allocations_tests
cmake-build-release/blobber_automation_allocations_tests
cmake --build --preset release --target blobber_gpu_smoke -j 2
cmake-build-release/blobber_gpu_smoke
cmake-build-release/blobber --assets cmake-build-release/assets --development --seed 5 --capture-menu /tmp/blobber-pc05-captures
cmake-build-release/blobber --assets cmake-build-release/assets --development --seed 5 --capture-menu /tmp/blobber-pc05-captures-repeat
```

The reused sanitizer build is Debug, graphics-disabled, with
`-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie` and linker `-no-pie`.
No package/toolchain installation. Routine experiment-tool contract tests use owned
fixtures; no balance matrix, smoke experiment, broad seed sweep or full expedition
experiment was run.

## Storage and review

PreparedTravel is 10,680 bytes on this build, including optional copied Inventory,
supplier stats and existing stamps/deadlines; it is transient preparation storage.
Four floor snapshot payloads remain 943,840 bytes. Prepared banks remain shared:
273,874,998 resident-accounted bytes against the unchanged 536,870,912-byte budget
in the shipped hidden capture. These are engine figures, not process RSS/VRAM.

User review:

1. Launch with `--development --seed 5`, choose four characters, and use the hub
   entrance. Verify the fresh selector offers floor 1; Cancel stays in the hub.
2. Enter and explore normally. After taking damage and settling combat, use R for
   a field long rest: one kit, recovery in place and normal rest time.
3. Later use T or the header Travel control. Verify destination/cost/count, cancel
   once, then confirm recall: one kit spent, sanctuary arrival, no healing.
4. Rest explicitly at the sanctuary. Use F on the hub entrance and confirm an
   unlocked entry. Verify lost departure position and persistent deaths, loot,
   doors, keys, discovery, gear, formation and resources.
5. Unlock a deeper floor through normal guardian/key/stairs play, then recall and
   choose that entry. It must not reveal or unlock unvisited depths.
6. At zero kits, recall is unavailable while safe ordinary stairs and unlocked
   entry returns remain free. Inventory T continues to transfer selected items.

## Limits and next-agent handoff

Interactive human play coverage and live Codex-host MCP remain unverified. The
real local MCP session adapter is covered; it is distinct from host integration.
Placeholder encounters/content and accepted recovery economics are unchanged; no
balance or full-expedition confidence is claimed. No disk save, shop, refill,
respawn, departure portal, new expedition policy or PC06 work is delivered.

Entry points are CampaignRun's three typed request factories and shared travel
transaction, Party's shared campSupplies query/staged charge, bounded TravelHud and
existing graphical/headless replacement adapters. Keep floor import preparation-only
and preserve the single travelling Party. Future disk saves must retain actual
inventory/unlocks and cannot reconstruct them from seed or a trace.

Implementation entered with a clean working tree. Existing changes were preserved;
no commit, reset, stash, installed agent configuration change, desktop automation
or balance experiment was performed. PC05 was accepted by the user on 2026-10-30.
PC06 is next eligible for detailed planning only, in a fresh context when requested;
its plan still requires review before implementation.

Initial compile verification caught test-only namespace/reset API mistakes, corrected
before the passing runs. Existing missing-aggregate-field warnings in
commands_tests are unchanged. No failing acceptance criterion is intentionally
deferred. Interactive human and live-host checks remain unavailable evidence,
not passing tests.


## Acceptance closeout (2026-10-30)

User: “slice accepted, record it in the plan doc”. PC05 is Accepted and closed.
Verification results and limitations remain unchanged; no additional interactive
play-coverage details were supplied. This closeout changes documentation only and
preserves all existing working-tree changes. No builds/tests/captures, balance
experiments, commits, resets or stashes were performed. Next action: plan PC06 only
for review in a fresh context when requested; milestone P-B remains Planned.
