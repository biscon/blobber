# PC02 verification and handoff

Status: **Accepted and closed**, 2026-10-30.
Implementation approved by “Implement the plan.” The user accepted on 2026-10-30:
“okay in that case I accept the slice, ive tested as far as I could and it is working,
update the doc”. PC03 has not started.

[Design](procedural_fixtures_design.md) · [Roadmap](procedural_campaign_plan.md) ·
[Capture gallery](screenshots/pc02/index.html) · [Protocol](headless_protocol.md#pc01-generated-floor-scenarios)

## Delivered

Generated floors now have one permanent-open door. Depth 1 opens without a key;
depths 2 and 3 use an upstream Bronze/Silver seal key. Keys are party quest bits,
independent of inventory capacity and weight, and remain acquired after opening.
F or a visible fixture click uses the checked command: nearby/faced, stationary,
settled safe exploration, no party-cell area hazard, and an able companion. Preview,
rejection, stale handles, repeated requests and reset preserve their contracts.
NPCs do not open doors. Upstairs, downstairs and the final endpoint are inert markers.

Session-owned effective geometry is shared by movement, actor placement, incremental
BFS, AI, sight, attacks/areas, prepared rendering and picking. Geometry revisions
invalidate pending/cached paths and stationary frames. The renderer shares prepared
wall columns with unlocked/sealed door colour planes; six fixture marker sprites
extend the ordinary loot bank. Editable SVG masters and raster inputs are retained.

Six-cell facing sight records discovered terrain and remembered fixture state at
settled poses and interactions. Blocking endpoints are visible; cells beyond closed
doors and solid corners remain hidden. Queued movement discovers the completed pose
before starting the next transition. M shows this knowledge. Development mode has a
labelled full-map toggle that never writes discovery. Player observations expose the
same knowledge; explicit diagnostic inspection remains isolated. Headless actions,
Python client, MCP Sessions and strict replay all support `interact`.

Generator/template versions are **2/2**, scenario schema and protocol remain **1**.
Prepared format/renderer revisions are **4/4**. Rebuild prepared assets normally;
old generated version-1 scenarios/traces are intentionally incompatible. Arena
scenarios and authored empty-fixture maps remain supported.

## Verification results

| Check | Result | Evidence |
| --- | --- | --- |
| Full headless Debug / Release | 37/37 each passed | `/tmp/blobber-pc02-headless-debug-tests.log`, `...headless-release-tests.log` |
| Full graphics Debug / Release | 56/56 each passed | `/tmp/blobber-pc02-graphics-debug-tests.log`, `...graphics-release-tests.log` |
| Test-lifetime and diagnostic-label correction | 3/3 passed in each of the four presets | `/tmp/blobber-pc02-final-*-tests.log` |
| Final threshold-prompt correction | Focused headless 4/4, graphics 5/5 per configuration; sanitizer 2/2 | `/tmp/blobber-pc02-threshold-*-tests.log` |
| ASan/UBSan with leak detection | 4/4 passed after correcting test-owned JSON lifetime | `/tmp/blobber-pc02-sanitize-final-tests.log` |
| Engine-owned steady-state C++ allocations | Zero tracked in headless and prepared graphics paths | `automation_allocations`, `frame_allocations` |
| Direct Python client and real MCP Sessions adapter | Preview/open/repeat/traverse, stale guards and replay passed | `generation_cli` |
| Shipped-asset strict walk/replay and hidden captures | Depths 1, 2 and forced depth-3 fallback matched | `/tmp/blobber-pc02-review-delivery-depth{1,2,3}/` |
| Normal party-picker launch and development maps | Hidden captures exported and inspected | `/tmp/blobber-pc02-menu-final/` |
| Hidden GPU resource lifecycle | Passed, NVIDIA GeForce RTX 3060/OpenGL | `/tmp/blobber-pc02-gpu.log` |
| Formatting, whitespace, Python syntax and document links | Passed: 217 relative links/anchors, 22 PNG hashes/dimensions | 71 PC02 C++ files; `git diff --check` |

New owned fixtures cover closed/open movement, reciprocal sight, melee/ground spell
geometry, actor movement, pending BFS cancellation within its work budget, actual AI
perception across the door, stationary frame/pick refresh, all four rotations,
strafe endpoints/reversal, marker/body/wall occlusion, full bags, no able companion,
contact/hazard rejection, preview/repeat/stale/reset and queued-turn discovery.
Generation cases cover depths 1–3, representative fixed seeds and forced fallbacks,
misplaced doors/keys and no-bypass reachability. Observation tests probe hidden IDs,
inspect without contaminating knowledge, and replay in a fresh process with deliberate
quest-state corruption. Existing combat/projectile/area/render/arena suites also pass.
No seed sweep or balance trial was run; the existing two-record CLI stream test is
only a structural/tool contract test.

During development a new actor test initially attempted an illegal two-cell move;
its fixture was corrected. Compiler errors from missing test includes and a HUD
conditional scope were corrected before final builds. Sanitizers then caught a
range-for over a JSON subobject of a destroyed temporary in the new automation test;
keeping the response alive fixed it. The sanitizer rerun passes. Capture review
corrected a past-tense preview prompt, unsupported punctuation, a hard-coded depth-I
header, an obsolete diagnostic legend and a misleading prompt on an already-open
threshold. The last fix restricts the nearby prompt to geometrically reachable
fixtures; the checked command rules are unchanged. These are resolved failures.

### Executed commands

From the repository root (logs redirect output only):

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

After the test-owned JSON fix, rebuilt `blobber_fixtures_automation_tests` in all
presets. After the diagnostic legend correction, rebuilt all presets with `-j 3`
and ran `ctest --preset PRESET -R '^(fixtures_automation|generation|automation_allocations)$'`.
The later clang-format adjustment changes whitespace only. The capture-only replay
from a prior executable correctly rejected its binary fingerprint after rebuilding;
all final review walks were then recorded/replayed with the final executable.
After the threshold-prompt correction, rebuilt all four presets and ran
`ctest --preset PRESET -R '^(fixtures|fixtures_automation|commands|automation_allocations)$'`
(headless) or the same expression plus `fixtures_render` (graphics). Rebuilt the
fixture sanitizer targets and ran `ASAN_OPTIONS=detect_leaks=1 ctest --test-dir
/tmp/blobber131-sanitize -R '^(fixtures|fixtures_automation)$' --output-on-failure`.
The final capture walks were regenerated again with that executable.

```sh
cmake --build /tmp/blobber131-sanitize -j 3 --target blobber_fixtures_tests blobber_fixtures_automation_tests blobber_generation_tests blobber_generation_automation_tests
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /tmp/blobber131-sanitize -R '^(fixtures|fixtures_automation|generation|generation_automation)$' --output-on-failure
cmake --build /tmp/blobber131-sanitize -j 3 --target blobber_fixtures_automation_tests
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir /tmp/blobber131-sanitize -R '^(fixtures|fixtures_automation|generation|generation_automation)$' --output-on-failure
cmake --build --preset debug --target blobber_gpu_smoke -j 2
cmake-build-debug/blobber_gpu_smoke
cmake-build-debug/blobber --assets cmake-build-debug/assets --development --seed 5 --capture-menu /tmp/blobber-pc02-menu-captures
```

The reused sanitizer build has `-fsanitize=address,undefined -fno-omit-frame-pointer
-fno-pie` and linker `-no-pie`. No toolchain/packages were installed. All graphics
windows were application-owned and hidden; no OS input/focus/desktop automation.

## Memory and bounds

On this build, generated retained definition plus 36×36 Map payload is **4,240 bytes**
(**4,544** at the hard 1,600-cell bound), below 16 KiB. Accounted generation scratch
is **32,528 bytes**, below 128 KiB, including the 6,400-state key/open search.
`FloorFixtures` is 524 bytes; `FloorGeometry` is 888 bytes plus its cell lookup
(1,296 bytes for generated maps), totaling **2,184 bytes**. It has 32 fixed slots,
1,600 discovery bits and nonrecycled floor identity. Discovery scans at most 169
candidate cells with bounded rays per settled pose or fixture change.

Normal launch reports scene **201,348,816**, NPC **69,010,094**, projectiles
**1,012,160**, and loot/fixture markers **2,503,928** bytes, totaling **273,874,998**
(~261.2 MiB) against **536,870,912**. Bake accounting reports resident scene/NPC
**285,009,262** and tracked loading high-water **236,194,664** bytes, also within
limits. These count different preparation/runtime capacities; neither is process
RSS or driver memory. The marker loader budget is 4 MiB. Door pixel planes share
wall column metadata and are included in preparation accounting.

Seed-5 generation still requests **39 C++ allocations / 3,817 bytes** during explicit
preparation. Runtime fixture interaction, discovery, typed observations, simulation,
HUD refresh and prepared composition perform zero tracked C++ allocations in the
owned workloads. JSON/export and C/third-party/driver allocations are outside that
guarantee. No production-scale performance or balance claim follows from these tests.

## Review and reproduction

Use the 22-image [gallery](screenshots/pc02/index.html) for the initial key, missing-key door,
collected key, opened passage/threshold, stairs/endpoint, and discovered-versus-full
maps. Accompanying scenarios, generation metadata, operation samples, capture manifests
and image hashes retain provenance. Captures are scene/HUD reconstructions; historical
mouse position and combat-log interaction are not reconstructed.

The durable script walks an empty floor with explicit diagnostic BFS and records
ordinary checked moves/interactions. It is a fixture review tool, not a balance agent.
Use a new output directory:

```sh
python3 docs/screenshots/pc02/reproduce.py --runner cmake-build-debug/blobber_headless --assets cmake-build-debug/assets --scenario docs/screenshots/pc02/depth-2-scenario.json --output /tmp/blobber-pc02-review-depth2
```

The same command was executed with the retained depth-1 and depth-3-fallback scenarios.
The menu command was repeated with output `/tmp/blobber-pc02-menu-final` after the
diagnostic legend correction.
It samples 5/8/8 operation boundaries respectively and strict-replays each trace with
three PNGs per boundary. `walk.jsonl` and complete logs remain in the corresponding
`/tmp/blobber-pc02-review-delivery-depthN` output; reproduce instead of depending on those files.

For user-driven interactive review, launch `cmake-build-debug/blobber --development
--seed 5`, select New Game and a party, move/turn with the existing controls, use F
or click the door, and compare M before/after using its diagnostic reveal toggle.
Normal New Game is depth 1; the retained scenarios provide separate depth-2/3 review.
Stairs/objectives explain that travel is not yet available. Keys never consume bag slots.

## Limitations and next-agent handoff

The user tested the available gameplay and reported it working when accepting PC02.
This is user-reported interactive evidence, not a claim of exhaustive coverage. Live
Codex-host MCP interaction remains **unverified**. Prior headless, direct adapter and
hidden GPU/capture results are unchanged.
There is no travel, encounter population, guardian, reward, victory, hub, save,
respawn or campaign persistence in PC02. Bronze and Silver locks share the sealed
door image; prompts and key names identify the required key. NPCs cannot open doors.
The user confirmed that separate gallery floors and inert stairs are expected here:
traversal remains PC04. The user criticized the basic door visuals but accepted the
functional slice; visual quality is not considered final. No renderer redesign was
authorized or started by this acceptance.

`GameSession` must outlive all borrowed geometry/HUD views. Keep base Map and fixture
definitions immutable. PC04 will transfer quest-key ownership into campaign state;
per-floor discovery/completion must persist with each floor then. Preserve the checked
command boundary and revision invalidation when adding guardian gates in PC03.
Do not infer save compatibility from strict replay. Old generated versions reject.

All original working-tree files were preserved. The entry snapshot is
`/tmp/blobber-pc02-entry.tar.gz`; no commit was made. PC01 remains accepted and closed.
PC02 is now accepted and closed. PC03 is next eligible for its own detailed plan
in a fresh task/context when requested; implementation still requires plan review.
This closeout changes documentation only. No builds, tests, captures, balance
experiments or commits were run; all working-tree changes remain preserved.
