# R04 verification and review handoff

Status: **Implemented; Ready for review** (2026-10-31).
User approved the reviewed plan with “Implement the plan.” R03 remains Accepted.
[Design](renderer_integration_design.md) · [Roadmap](renderer_plan.md) ·
[Gallery](screenshots/r04/index.html). R04 and milestone R-B await user acceptance.

## Delivered behavior

`--renderer world` selects complete world gameplay through New Game, arena/rematch,
campaign stairs/recall/waypoints, save/Continue and native HUD. Legacy remains the
default until R06; `--renderer legacy` keeps controlled comparison available.
Supersampling defaults on; off selects native filtered. Only the selected runtime
world asset banks load. Existing legacy build dependencies remain until R06.

GameSession owns permanent fixture completion plus a transient 32-entry, 300 ms
opening barrier. Both backends/headless share it. Impacts/effect deadlines continue;
new movement, contact/facing, AI and automatic combat wait. One fresh graphical move
queues and is revalidated after contact; headless movement rejects without queuing.
Pause freezes opening and clears input; animation-off settles it. Save/travel check
both active opening and pending movement. Restores settle doors without changing the
save schema. A restored-clock regression ensures a later opening starts at zero.

Shared procedural slab/frame instances supply CPU/GPU depth. Closed leaves own fixture
IDs; frames/open leaves do not. Opening suspends actionable picking and previews.
Opaque depth clips ground pointers and HUD; effects never alter IDs/depth. World
commands use the same validation authority as legacy. Authored fixtures without
discovery now appear in visual observations (including hub stairs), reporting their
actual completion; generated discovery/remembered knowledge is unchanged.

`GameView` shares update/picking/ground/presentation orchestration across backends;
WorldRenderer retains one active and one staged view. First destination image and
picking precede campaign commit, and old view borrowers die before old sessions.
Preparation failures retain active simulation, Party, image, IDs and prior save.

The independent bounded EffectFrame builder ports current arrows, strikes, magical
projectiles/extras/pulses, spectral weapons, grease/web/burning and target previews.
It reuses ProjectilePlayer admission/overflow and R03 fitted attachments. Effect-only
animation does not rerasterize CPU visibility. Ground/shadows precede body color;
translucent effects follow with stable ordering and depth tests, no ID/depth writes.

Offline world format/appearance is **1/3**. World visibility is
**world-visibility-v2**; all new strict traces record
**shared-opening-300ms-v1** scheduling. Protocol 1 and campaign save schema remain.
Older publications/traces reject explicitly; rebuild/regenerate rather than fall
back. Local MCP `session_start(mode="visual", visibility="world")` retains backend
through reset/load. World observations need no graphics; replay images explicitly
own a hidden context. Captures do not advance session time.

## Initial delivery checks actually executed

| Check | Result |
| --- | --- |
| Full graphics CTest, Debug / Release | 65/65 each |
| Full graphics-disabled CTest, Debug / Release | 46/46 each |
| Final affected Debug rendering/fixtures/automation/campaign/options | 15/15 |
| Final protocol/automation after opening-decision guard, graphics Debug / Release | 6/6 each |
| Same final checks, headless Debug / Release | 5/5 each |
| CPU ASan/UBSan: impact, fixtures, campaign, world integration | 4/4 |
| Hidden R04 GPU checks, Debug / Release | Pass; zero door ID edge/interior differences |
| Existing R03 hidden GPU checks, Debug / Release | Pass; 21 bounded geometric-edge differences, no interior/member disagreement |
| Existing R02 hidden GPU checks, Debug / Release | Pass |
| Integrated CPU preparation injection | 16 owned failures among 32 positions; failure/cancel preserves prior image/IDs/resources |
| GPU preparation injection on actual staged campaign travel | Pass; retry and commit preserve destruction order |
| Warm opening/effect/quality frame C++ allocation and mesh/texture resource upload guards | Zero tracked allocations/uploads |
| Maximum visibility candidates | 2,656 retained; allocation guard passes |
| Initial maximum effect packets | 305 retained; zero tracked C++ allocations |
| Offline generated asset fixtures | Ninja + Multi-Config Debug/Release pass clean/no-op/changed effect and wall/touch/missing/add/remove/failure cases |
| Actual world CLI/client + strict replay + local MCP SDK stdio | Pass |
| Expedition baseline comparison | Same 1,070 requests; zero admission/inspected gameplay differences |
| Strict world replay images | Matched; 18 images at eight requested boundaries |
| Same-machine Debug/Release door and spell captures | 44 + 568 PNGs match byte-for-byte |
| Hidden arena/rematch, menu/save/Continue/error, native-filtered doors and projectile captures | Pass; selected evidence inspected |
| Final builds and whitespace | Pass; no package/toolchain changes or commits |

These are initial delivery results at appearance revision 2. The review correction
and its focused reruns are recorded below; full suites and whole-capture
Debug/Release parity were not rerun for appearance revision 3.

The full Debug suite preceded the final authored-fixture observation correction;
its affected 15 checks were rerun. Full Release and headless delivery suites include
that correction. The last automation-only decision guard was followed by the final
6/6 and 5/5 checks above and regenerated expedition/replay evidence. Existing warnings
in unchanged `commands_tests.cpp` aggregate initializers remain; changed targets build
without new warnings. Passing counts above are new R04 runs, not historical R03 counts.

Generated self-owned fixtures test queue/pause/deadline remainder, animation-off,
accepted impacts during opening, restored clocks/pending input, foreign snapshots,
stationary invalidation, slab bounds, shared CPU/GPU depth in four headings and
intermediate poses, closed-slab effect occlusion/open effect visibility, resource
lifetime/failure rollback, all current legal spell capture builds and allocation
invariants. Existing suites cover locks/keys, automatic guardian gates, command and
impact/death timing, ground spell eligibility, effect expiry, loot generations,
transactional campaign/save behavior and stale protocol identities.

## Performance and storage evidence

Reference: Linux, AMD Ryzen 9 7900 (24 logical CPUs), NVIDIA GeForce RTX 3060,
OpenGL 3.3 NVIDIA 615.78.08 / GLSL 3.30. Native window presentation is 1920×1080;
logical world is 500×282, supersampled color 1000×564 when enabled.

The opt-in **effect-only CPU** maximum workload uses 313 packets, 120 warmup updates,
600 samples and three trials. Release p95: **0.024716 / 0.024265 / 0.024306 ms**;
p99: **0.028173 / 0.027210 / 0.027010 ms**. All trials have zero tracked ordinary/aligned
C++ allocations. [Report](screenshots/r04/effects-benchmark-review.txt). These are packet
preparation timings, not GPU timings, total gameplay frames or a 60 Hz guarantee.
Full production presentation/performance comparison remains R06.

Per-view visibility storage is **4,841,152 bytes**; EffectsPipeline adds **83,736 bytes**,
combined **4,924,888 bytes** (4.70 MiB), below 8 MiB. World publication contains 122
textures, **194,129,080** mip payload bytes and **36,076,176** base-mask bytes. These
are managed payload/accounting values, not measured driver residency.

The generated-art GPU integration fixture records active **4,987,953 CPU / 5,724,224
GPU-estimate bytes**, active+staged **10,034,181 / 6,972,304**. This fixture verifies
ownership/accounting, not production-art resident memory. Existing 352 MiB subsystem
budgets and registry limits remain enforced; they exclude native UI/fonts, legacy
banks, exports and allocator/driver overhead. No whole-process memory claim is made.

## Spell baseline review correction

Implemented after screenshot review, at appearance revision **3**. Spiritual
Weapon's .65-unit quad is centered at .5, leaving .175 clearance above the floor
and below the ceiling. The previous .8 center intersected the ceiling. Spell glints
now use the legacy spell palette and 120 ms impact fade, impacts sit .002 units
forward of their target plane, and multi-dart/ray glyphs remain distinct. Effects
still obey ordinary depth occlusion and never write depth/IDs. Packet capacity is
313 with fixed storage, covering four party casters' extra Magic Missile glyphs.

Scorching Ray's former 650 ms specimen was taken after expiry. Its regenerated
specimen shows flight at 117 ms, impact at 234 ms, a half-faded tail at 294 ms and
expiry at 354 ms. Burning Hands already had immediate per-target impact flashes;
it has no flame-cone art. New samples cover its 0–120 ms pulse, alongside the
separate white body hit flash. Further spell graphics passes remain deferred.

Checks actually rerun for this correction:

- Release focused projectile, impact, spells, level spells, world CPU, visibility
  and integration CTest: **7/7**.
- CPU integration and maximum **313-packet** stress in Debug/Release: pass, zero
  tracked C++ frame allocations. CPU ASan/UBSan integration: pass.
- Hidden R04 GPU integration in Debug/Release: pass. New target-overlap readback
  verifies visible impact, approximately half contrast at 60 ms and exact expiry
  at 120 ms without visibility invalidation. Existing door occlusion, lifecycle,
  preparation failure, ID parity and allocation/upload guards also pass.
- Existing R03 GPU visibility integration, Release: pass.
- Generated asset incremental-build fixtures: Ninja Debug and Multi-Config
  Debug/Release pass. Headless Debug `world_visual_cli`: **1/1**.
- Release spell export: **586 PNGs** including the additional transient phases;
  selected hammer, flight, impact and Burning Hands frames visually inspected.
  [Timing manifest](screenshots/r04/effect-review-times.tsv) and sampled real-time
  previews are retained in the [gallery](screenshots/r04/index.html).

The gallery replaces the hammer and late Scorching Ray samples and adds Burning
Hands phases. Other retained campaign/door/UI captures and their strict trace are
initial-delivery evidence at appearance revision 2; rebuild/regenerate for the
current binary/appearance identity. No complete-suite or Debug/Release image-parity
rerun is claimed for the correction. Interactive desktop review remains unverified.

## Reproduce and limitations

```sh
cmake --build --preset debug -j 4
cmake --build --preset release -j 4
ctest --preset debug
ctest --preset release
ctest --preset headless-debug
ctest --preset headless-release
cmake --build --preset release --target blobber_world_integration_gpu_tests blobber_world_visibility_gpu_tests blobber_world_gpu_tests blobber_world_visibility_stress -j 4
./cmake-build-release/blobber_world_integration_gpu_tests
./cmake-build-release/blobber_world_visibility_stress
./cmake-build-release/blobber_world_integration_tests --stress
python3 tests/world_build_tests.py cmake-build-debug/blobber_bake_world cmake/WorldAssets.cmake
.venv/bin/python tests/campaign_save_mcp_tests.py cmake-build-headless-debug/blobber_headless cmake-build-headless-debug/blobber_world_visual_fixture
./cmake-build-debug/blobber --renderer world
```

Sanitizers use `/tmp/blobber-r04-asan` with graphics disabled and
`-fsanitize=address,undefined -fno-omit-frame-pointer`; run CTest matching
`^(world_integration|fixtures|impact|campaign)$`. Diagnostic logs are `/tmp/r04-*.log`;
retained evidence does not depend on those paths. The gallery README includes the
single-expedition review recipe and strict binary/content compatibility notes.

World selection supports normal campaign/arena flows, replay captures and focused
door/spell/projectile/menu/arena/save captures. Other historical focused diagnostic
capture recipes remain legacy and explicitly reject world selection. Their status
review reconstructions are not claimed as an actual campaign playthrough. The
retained expedition is real command execution, uses labelled diagnostic routing,
and contains no fabricated terminal victory. Level-two spell specimens use separate
legal diagnostic advancement; no new content or balance tuning was introduced.

No human interactive input, native-close/focus event playthrough or other GPU/vendor
coverage was performed. Hidden application-owned windows and in-process scripts were
used exclusively; no desktop mouse/keyboard/focus automation. Same-environment exact
captures are not a cross-device floating-point guarantee.

**Next action:** user review of R04. Legacy stays default. Only explicit acceptance
closes R04/R-B. R05, renderer retirement, new monsters, campaign balancing and PC07
remain unstarted. No commits are implied.
