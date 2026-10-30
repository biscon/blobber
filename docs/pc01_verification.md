# PC01 verification and handoff

Status: **Accepted and closed**, 2026-10-30. Implementation was authorized by “Implement
the plan.” User accepted: “slice accepted, record it in the doc”. PC02 has not started.

[Design](procedural_generation_design.md) · [Roadmap](procedural_campaign_plan.md) ·
[Capture gallery](screenshots/pc01/index.html) · [Protocol](headless_protocol.md#pc01-generated-floor-scenarios)

## Delivered

Normal New Game retains the party picker and independent starting builds/supplies,
then enters a seeded empty generated floor. Static placements, maze loading and
authored recovery are removed from this launch path. Existing arena and explicit
static diagnostics remain. Prepared assets, 1920×1080 presentation and discrete
movement are unchanged.

The bounded generator produces 36×36 maps with 10–12 rooms, two cycle families,
rotated chamber/pillar/bent/alcove interiors, optional leaves, and provisional
entrance/approach/objective metadata. Validation uses actual tiles, closed rims,
explicit ports, connectivity, useful-loop distance and role/branch constraints.
At most 16 attempts precede a validated authored fallback. Invalid templates and
fallbacks fail explicitly. No enemies, generated rewards, keys, doors, discovery,
campaign travel, saves or objective victory were added.

`blobber_generate_floor` provides ASCII/JSON inspection and an explicit bounded
streaming seed-report command. The game map overlay is labelled diagnostic with
cached generation information and provisional markers. Headless `start` accepts
`kind: generated_floor`; ordinary commands/reset/strict replay and MCP session
lifecycle are reused. Diagnostic forced fallback is rejected in visual mode.
Player observations omit topology/roles/rejections. Generated replay captures
include an explicitly diagnostic map companion.

## Verification results

| Check | Result | Evidence |
| --- | --- | --- |
| Full headless Debug / Release CTest | 35/35 each passed | `/tmp/blobber-pc01-headless-debug-tests.log`, `/tmp/blobber-pc01-headless-release-tests.log` |
| Full graphics Debug / Release CTest | 53/53 each passed | `/tmp/blobber-pc01-debug-tests.log`, `/tmp/blobber-pc01-release-tests.log` |
| Final added allocation accounting and reset-party assertions | 2/2 in all four presets passed | `/tmp/blobber-pc01-final-headless-tests.log`, `...final-debug-tests.log`, `...final-headless-release-tests.log`, `...final-release-tests.log` |
| ASan/UBSan, leak detection enabled | Focused 5/5; final strengthened 2/2 passed | `/tmp/blobber-pc01-sanitize-tests.log`, `/tmp/blobber-pc01-final-sanitize-tests.log` |
| Typed generated walking / prepared CPU visual path | Zero tracked steady-state C++ allocations | `automation_allocations` in headless and graphics builds |
| Actual direct Python client / MCP Sessions adapter | Fixture-owned launch, failure, reset, stale handle, close and replay passed | `generation_cli` |
| Strict shipped-asset walking replay and capture | Seeds 5, 8, 1 and forced depth-1 fallback passed | `/tmp/blobber-pc01-review/`; retained manifests in gallery |
| Normal party-picker launch / reconstructed launch | Passed; PNGs byte-identical | `/tmp/blobber-pc01-menu-captures/`; gallery |
| Existing arena graphical launch/capture | Passed, 54 PNGs | `/tmp/blobber-pc01-arena-captures/`, log beside it |
| Hidden GPU lifecycle | Passed on NVIDIA GeForce RTX 3060/OpenGL | `/tmp/blobber-pc01-gpu.log` |
| Whitespace / formatting / docs / Python syntax | Passed | `git diff --check`, clang-format on 33 changed/new C++ files, 169 relative links/anchors, `py_compile` |

Routine tests own their catalogs, characters, maps and materials. Generator fixtures
use ten fixed seeds, not a broad sweep. A two-record CLI test checks streaming and
summary semantics only. Render tests compare generated room/pillar pixels against
an independent ray/block oracle and check strafe endpoints/reversal. Structural
failure cases include bounds/capacity, duplicate sites/connections, unmatched ports,
unplanned cells, blocked anchors, disconnected interiors, dead-end “loops”, invalid
templates, bounded exhaustion and invalid fallback. Session tests retain resource/
formation state across reset, reject stale/invalid replacement, and replay in a fresh
process, including deliberate provenance corruption. Visual tests verify that
explicit inspection does not disable later player filtering.

The initial new CLI test failed because it used `recording_complete` rather than the
existing adapter's `trace_complete` field. The assertion was corrected; the final
focused and full suites above pass. No production adapter lifecycle behavior was
changed to accommodate the test.

### Executed build/test commands

From the repository root:

```sh
cmake --build --preset headless-debug -j 4
cmake --build --preset headless-release -j 4
cmake --build --preset debug -j 4
cmake --build --preset release -j 4
ctest --preset headless-debug
ctest --preset headless-release
ctest --preset debug
ctest --preset release
```

Focused checks during development included:

```sh
ctest --preset headless-debug -R '^(generation|generation_automation|generation_cli|automation_allocations|headless_cli|mcp_adapter_tools)$'
ctest --preset debug -R '^(generation|generation_automation|generation_cli|automation_visual|automation_allocations|render|menu|arena)$'
```

After adding preparation-allocation accounting and explicit reset-party comparison,
rebuilt `blobber_automation_allocations_tests` and `blobber_generation_automation_tests`
in each preset and ran `ctest --preset PRESET -R '^(automation_allocations|generation_automation)$'`.
These were test-only changes; the captured application/headless executables did not
change after their recordings.

Reused the existing sanitizer build, without installing/changing toolchains:

```sh
cmake --build /tmp/blobber131-sanitize --target blobber_generation_tests blobber_generation_automation_tests blobber_automation_allocations_tests blobber_arena_tests blobber_headless blobber_generate_floor -j 3
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /tmp/blobber131-sanitize -R '^(generation|generation_automation|generation_cli|automation_allocations|arena)$' --output-on-failure
```

That build uses `-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie` and
`-no-pie`. The final two test targets were rebuilt/rerun with the same flags.

```sh
cmake --build --preset debug --target blobber_gpu_smoke -j 2
cmake-build-debug/blobber_gpu_smoke
cmake-build-debug/blobber --assets cmake-build-debug/assets --seed 5 --capture-menu /tmp/blobber-pc01-menu-captures
cmake-build-debug/blobber --assets cmake-build-debug/assets --seed 5 --capture-arena /tmp/blobber-pc01-arena-captures
```

All windows above were application-owned and hidden. No mouse/keyboard injection,
window selection, focus change or desktop automation was used.

## Memory and bounded work

On this build, `sizeof(GeneratedFloor)` plus its 36×36 Map payload is **3,712 bytes**;
the hard 1,600-cell retained bound is **4,016 bytes**, below 16 KiB. The generator's
fixed search scratch plus two layout buffers totals **12,272 bytes**, below 128 KiB.
These are explicit data sizes, not an assertion about total process RSS, compiler
stack frames, allocator metadata or existing session/prepared-render storage.

Seed-5 preparation performs **39 C++ allocations totaling 3,817 requested bytes**
(row conversion and Map construction), measured separately before runtime guards.
This is cumulative requested allocation, not peak resident memory. Generation is a
preparation phase; ordinary simulation, movement, typed observations and prepared
CPU visual sampling allocate zero tracked C++ storage. JSON, export, driver/raylib
internals and C allocation are outside that guarantee. Diagnostic strings/markers
are prepared once in fixed storage. Sweep aggregation caps fingerprint payload at
800,000 bytes; broad sweep performance was not measured.

## Review artifacts and reproduction

Thirteen retained 1920×1080 PNGs are in the [gallery](screenshots/pc01/index.html).
The accompanying JSON files retain generation definitions/provenance, scenarios,
original capture manifests, filenames and image hashes. Representative entrances,
room/pillar occlusion, objective rooms, full diagnostic maps and normal menu launch
were visually inspected. Objective rooms are intentionally ordinary empty rooms.

| Example | Rooms / topology | Fingerprint | Replay capture operations |
| --- | --- | --- | --- |
| Seed 5 | 10 / six-room cycle | `8955331068716827512` | 1, 61, 121, 122 |
| Seed 1 | 12 / eight-room cycle | `8789648654042291559` | 1, 35, 177, 178 |
| Seed 8 | 11 / six-room cycle, two branches | `10972961193643475412` | 1, 69, 129, 130 |
| Seed 5 forced fallback | 10 / authored depth-1 layout | `15555746629466345479` | 1, 2 |

The short scripts used full diagnostic topology to compute paths, then submitted
normal checked turn/move/advance commands. They are diagnostic walking checks, not
player exploration policies. Every selected replay also resets and verifies the
same generation definition. Trace files and capture logs remain under
`/tmp/blobber-pc01-review/NAME/`; strict replay requires the matching executable and
content. Capture scripts are `/tmp/blobber_pc01_capture.py` and
`/tmp/blobber_pc01_capture_family1.py`; these temporary artifacts are not durable
project fixtures. Scenarios and generation JSON in the gallery are durable inputs
for fresh recordings after a rebuild.

Example executed replay/capture form (use a new output directory):

```sh
cmake-build-debug/blobber_headless --assets cmake-build-debug/assets --replay /tmp/blobber-pc01-review/seed-5/walk.jsonl --capture-dir /tmp/blobber-pc01-review/seed-5/captures --capture-operation 1 --capture-operation 61 --capture-operation 121 --capture-operation 122
```

For interactive review, launch `cmake-build-debug/blobber --seed 5`, select the
normal four-character party and enter New Game. Walk/strafe/turn, open M for the
labelled diagnostic map, compare the loop and branch, then repeat with `--seed 1`.
There is no endpoint win condition yet. Use the inspection tool's `--force-fallback`
for fallback diagnostics and `ctest --preset headless-debug -R '^generation$' -V`
for the fixture-owned invalid-template/fallback rejection contracts.

The menu launch/restart PNG SHA-256 is
`e3aabbf44c93650648475c0c3ad43d5e80a2af910d99bc5813e6a662461485d9` for both files.
Menu capture exported 49 PNGs in total; only the two relevant launch images are
retained here. Arena capture exported 54; its shared launch path remains operational.

## Limitations and next-agent handoff

- **No balance experiments, broad seed sweep, or full expedition was run.** A few
  deterministic structural fixtures/captures establish correctness cases, not
  distribution quality, pacing, combat safety or difficulty.
- User acceptance is recorded above. Interactive desktop execution remains unverified;
  acceptance does not establish that additional checks ran. The existing Codex-host
  MCP connection was not reconfigured or interactively tested;
  the actual Python/MCP Sessions adapter was exercised with a real fixture runner.
- Keys/doors/guardian gates and discovered navigation are deliberately absent.
  Structural metadata is provisional; PC02/PC03 must validate their actual geometry
  and state transitions rather than treating PC01 connectivity as progression proof.
- Depths 2–3 are identities with separate deterministic layouts/fallback orientations,
  not a playable multi-floor campaign. The hub and travelling Party ownership are PC04.
- Authored maze/placement content still serves explicit legacy diagnostics and offline
  baking. Normal New Game no longer reads that content; remove remaining exclusive
  dependencies as their consumers are migrated, without breaking investigation tools.
- All work remains uncommitted. The working tree was clean on entry; no pre-existing
  edits were overwritten. PC02 is next eligible for separate detailed planning; do not
  begin it automatically.

For later implementation, keep core generation independent of JSON/graphics; use
`Scenario` and existing transport adapters for automation additions. Preserve the
Map-before-session lifetime and arena behavior. Update generator/template versions
when layout semantics change, and retain explicit fallback/rejection evidence.


## Accepted closeout (2026-10-30)

The user explicitly accepted PC01: “slice accepted, record it in the doc”. The
checkpoint, tracker, execution record, system overview and detailed design now
record **Accepted and closed**. Previous verification results and limitations are
unchanged; no tests, builds, captures or balance experiments were rerun for this
documentation-only closeout. Existing working-tree changes are preserved; no commit
was requested or created.

PC02 is next eligible for detailed planning in a fresh context when requested.
Inspect current code and the accepted documents before presenting its plan for
review. This acceptance does not authorize automatic PC02 planning or implementation.
