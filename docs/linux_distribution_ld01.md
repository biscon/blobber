# LD01 — Runtime inventory and clean staging

Approved scope: the LD01 plan in this conversation, implementation authorized by
the user on 2026-11-11. Status: **Implemented, mechanically verified, accepted and
closed by the user on 2026-11-11.**
Only LD01 is authorized; LD02–LD04 remain planned. This is a content staging
contract, not evidence of a portable build or a playable extracted package.

## Design and ownership

`packaging/runtime-content.json` owns the shipping inventory. Explicit source and
prepared entries plus concrete NPC, dialogue, shop, UI and audio selectors resolve
to a sorted list of source/destination/reason records. A native, CPU-only C++ helper
uses the existing JSON/YAML dependencies and loaders; it is not shipped. Both
normal Release raw staging and `dist-stage` consume that inventory. Debug retains
source raw assets and executable-local prepared assets.

Keep source paths unchanged: NPC JSON includes artwork paths in its fingerprint.
Copy definitions/publications without rewriting them. NPC source sheets/frames,
world source textures, portrait masters, prompts, models, review exports and
authoring executables are not selected merely because metadata names them.
`prepared/world/sources.json` is required for blueprint checking; `portraits.tsv`
has no runtime consumer and is excluded. Shared UI icon names retain their order.

The helper resolves actual opens: NPC level-to-definition JSON, top-level dialogue
and shop YAML, dialogue overlays/audio, bank base/override variants, and the three
music playlists. Missing selected files, malformed content and audio-loader
warnings reject staging. Existing runtime loaders still retain their own behavior.
Prepared content consists of three QOI atlases and world textures/metadata/sources.
Existing applicable notices are copied; missing distribution documentation is
recorded rather than replaced with invented terms.

Raw synchronization owns only the declared raw subdirectories under the configured
build asset root, including migration of the previous broad CMake copies. It
preserves `prepared/` and unrelated build files. Unchanged source size/timestamps
avoid copies. The distribution helper owns `<build>/dist-stage/` through a marker;
it rejects unowned nonempty destinations, unsafe paths, symlinks, special files and
overlap with inputs. Fresh candidates are validated before replacing the old stage.
A failed call returns failure and cannot designate the old stage as a new result.

## Build and interfaces

`blobber_stage_runtime` is a host build tool with raw/dist/verify operations, explicit
manifest/source/prepared/build/configuration roots, and an executable input for
dist. It uses bounded streaming copies; allocations and filesystem work occur only
in build/preparation phases. Runtime frame processing and save formats do not change.

CLI: `blobber_stage_runtime raw|dist|verify --manifest FILE --source ASSETS
--prepared DIR --build DIR --config CONFIG [--executable FILE] [--multi-config]`.
`--skip-debug` is the internal multi-configuration normal-build no-op; explicit raw
staging remains available in Debug. The build root must contain `CMakeCache.txt`.
Manifest version 1 has `files`, `selectors` and `validate` arrays: file entries use
`root` (`assets`, `prepared`, `manifest`), relative `path`, and optional `destination`.
Defaults preserve `assets/` and `assets/prepared/`; notices use explicit destinations.
Unknown fields/tags and conflicting destinations fail. Identical mappings deduplicate.
UI names share `src/ui/art_names.hpp`; equipment keeps its existing shared table.

`blobber_assets` explicitly synchronizes raw content for tools; non-Debug game
builds run the same synchronization. `dist-stage` is supported for Linux x86-64,
single-configuration Release builds with graphics enabled. Unsupported setups get
an actionable rejection target without building the game or baking assets.
Existing publication targets own generation and dependencies. Production verification
uses one low-priority job so bake phases cannot run concurrently.

Payload: `<build>/dist-stage/Release/{blobber,assets/,notices/}`. A sorted report
beside the payload records selected/excluded paths, reasons, sizes, totals,
configuration and loader content identities. No archive or final release promotion
is introduced. LD02 must invoke `dist-stage` successfully in its controlled build;
an existing directory alone is not evidence of a successful current stage.

The marked distribution root uses `.candidate` and `.previous` paths for replacement.
Validation failure removes the candidate and leaves the old report/stage unchanged;
promotion errors attempt rollback. Interrupted `.previous` output rejects further
staging/verification for inspection instead of guessing which result succeeded.
The report has a distinct `stage_id`; `verify` checks report completion, exact file
membership/sizes, execute permission and loader identities against the staged roots.
It does not substitute for successful invocation of the current `dist-stage` target.
Use one writer per build tree; this is ordinary build-output ownership, not a
filesystem security sandbox or a source snapshot for concurrent authoring edits.

Raw ownership covers audio, blueprints, dialogue, effects, fonts, NPCs, rules,
shops, textures and UI directories. It preflights inputs/output trees, retires stale
leaves/empty directories, then copies changed files. File/directory replacements
are supported. `.blobber-raw-inventory.json` is written last. Raw synchronization
does not promise transactional recovery after I/O failure; the failed build must be
fixed/rerun. Tools requiring authoring originals must use the checkout, rather than
expecting those originals in the curated runtime staging root.

## Verification strategy

Small generated fixtures exercise manifest selection, real reference resolution,
configuration rejection, absent/malformed content, duplicate/unsafe paths, ownership,
stale replacement, timestamp preservation and failure without promotion. Include
unchanged NPC definitions loading without bake-only artwork. Tests own their inputs
and do not bake production content. Run the new suites and existing options,
packaged_options, npc_metadata, dialogue, audio, music and blueprints checks.

Then run one production Release staging pass using the configured CLion CMake/Ninja,
at low CPU/I/O priority with bounded library threads. Verify staged loader identities,
blueprint/publication agreement, exclusions and inventory sizes. Do not run desktop
automation, an unrelated matrix or benchmarks. Artifact logs live in an owned agent
run, are summarized here, and are cleaned at handoff.

## Implementation handoff

Delivered 2026-11-11. Entry points: `packaging/runtime-content.json`,
`tools/stage_runtime.cpp`, the cohesive `tools/distribution/` implementation, shared
UI names, and CMake's staging/test targets. Source assets were not moved or edited.
No changes to save compatibility, publication/catalog identities or frame processing.

The successful local payload is
`cmake-build-release/dist-stage/Release/`; its complete required/excluded inventory
is `cmake-build-release/dist-stage/Release.inventory.json`. These are intentional
ignored build outputs retained for LD02/review, not a portable release or archive.

| Content | Files | Bytes |
| --- | ---: | ---: |
| Runtime raw assets | 373 | 353,018,675 |
| Prepared QOI atlases | 3 | 19,631,363 |
| Prepared world | 3 | 954,378,584 |
| Game executable | 1 | 7,519,744 |
| Existing notices/attribution | 8 | 28,356 |
| **Stage total** | **388** | **1,334,576,722** |
| Excluded source/prepared files | 986 | 863,166,770 |

Stage identity `1794425472820654886`; content identities: rules
`16064805046093731371`, NPC definitions `16167988745088981044`, blueprint
`11177173040059835938`, world `16850201644598948673`, world payload checksum
`162051066310005404`. These are existing loader fingerprints, not cryptographic
release checksums. All 14 NPC JSON files and the shared UI filename order match
their original content. All 388 staged files were additionally compared with their
actual inputs using bounded byte reads and permission checks; every comparison passed.

### Commands and results

Used `/home/bison/clion/bin/cmake/linux/x64/bin/cmake` and its configured
`/home/bison/clion/bin/ninja/linux/x64/ninja`, read from both caches. Commands below
were run from the repository root. Lengthy commands used `nice -n 15 ionice -c 3`;
production preparation used `OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2`.
`TMPDIR` pointed into the owned run. Initial memory/disk checks showed 22 GiB
available RAM and 449 GiB disk space. Only one build/test job ran at a time.

```sh
/home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-release --target \
  blobber_distribution_tests blobber_stage_runtime blobber_options_tests \
  blobber_packaged_options_tests blobber_npc_metadata_tests blobber_dialogue_tests \
  blobber_audio_tests blobber_music_tests blobber_blueprint_tests -j 2
/home/bison/clion/bin/cmake/linux/x64/bin/ctest --test-dir cmake-build-release \
  -R '^(runtime_inventory|dist_staging|options|packaged_options|npc_metadata|dialogue|audio|music|blueprints)$' \
  -j 1 --output-on-failure
/home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-release --target dist-stage -j 1
```

All nine suites passed in 0.16 s. One production staging pass succeeded, including
one world publication and portrait/creator preparation as required by the current
Release build dependencies. No production fixture was used as a unit-test input.
After refining raw file/directory replacement, rebuilt only `blobber_distribution_tests`
and `blobber_stage_runtime` (`-j 2`) and reran `runtime_inventory|dist_staging`:
both passed in 0.07 s. This safety correction does not change shipping selection;
the final helper verified the existing production stage rather than rebaking it.

```sh
/home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-debug \
  --target blobber_options_tests blobber_packaged_options_tests -j 2
/home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-debug --target dist-stage -j 1
/home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-release --target blobber_assets -j 1
cmake-build-release/blobber_stage_runtime verify \
  --manifest "$PWD/packaging/runtime-content.json" --source "$PWD/assets" \
  --prepared "$PWD/cmake-build-release/assets/prepared" \
  --build "$PWD/cmake-build-release" --config Release
```

Debug and packaged option executables, plus Release packaged options, passed when
launched by absolute path with the owned fixture directory as working directory,
without `--assets`. This exercises the Debug source-path compile branch separately
from Release. The Debug `dist-stage` command intentionally exited nonzero with its
configuration message, without compiling the game or baking assets. It is an
expected negative check, not an unresolved failure. Explicit Release `blobber_assets`
reported 373 files and preserved every raw/prepared file's size and nanosecond
timestamp (excluding the synchronization completion marker). Final `verify` passed.

Fixtures cover real dialogue/shop additions/removals, missing references,
base/override audio selection, malformed YAML/disabled cues, duplicate/unsafe paths,
selected symlinks/FIFOs, output links, unowned destinations, overlapping inputs,
raw migration/idempotence/type transitions, candidate failure preserving the old
stage, stale replacement, exact membership and missing files. A tiny generated
world and blueprint fixture cover provenance/publication mismatches without baking.
`git diff --check`, local documentation links and manifest input paths were checked.

### Residual dependencies and review

#### User review and duplication audit — 2026-11-11

The user reports that the stage works and its asset folders look clean, initially
making acceptance conditional on checking duplication. A focused read-only audit
found small existing duplicates. After reviewing the findings, the user explicitly
approved LD01 and clarified that the concern was shipping both packed world
textures and their unpacked originals. The stage excludes those originals. The
small internal material/sound duplicates are accepted; no size optimization was
requested. No content, bake format, stage or fingerprint was changed by this review.

The 953,847,534-byte world payload is accounted for by:

| Representation | Bytes |
| --- | ---: |
| Uncompressed base RGBA pixels | 663,049,856 |
| Additional mip levels | 220,583,760 |
| Alpha masks | 64,967,422 |
| Mesh vertex data | 5,246,496 |

Five distinct 4096×4096 city/interior atlases total 447,392,420 bytes including
mips. The serialized texture/mask/mesh ranges cover the entire payload contiguously,
with no gaps or unaccounted tail. Raw world textures, pack source folders and NPC
animation artwork are absent from the stage. The binary holds prepared pixels,
not compressed PNGs; this largely explains its size. The eventual compressed
download size belongs to LD03 and has not been measured.

The check grouped all staged regular files by size and SHA-256 (streaming 1 MiB
chunks), then grouped equal-dimension world texture records by their pixel/mask
checksums and compared candidate payload ranges byte-for-byte:

- Three groups of identical WAVs: each `weapon.arrow_impact_0N.wav`,
  `weapon.pierce_0N.wav` and `weapon.slash_0N.wav` for N=1,2,3. All are bank-referenced;
  redundant bytes total **126,984**.
- Three groups of identical prepared material payloads: city/emporium/smithy wall;
  city ceiling/inn wall; emporium/smithy floor. The extra serialized copies total
  **1,398,096 bytes**. Different pack roles currently retain separate texture IDs.
- Combined exact redundant storage found: **1,525,080 bytes**, approximately
  **0.114%** of the stage. No duplicate whole copy of a large atlas or world payload
  was found. This does not claim a perceptual/subimage duplication search.

Some artwork also intentionally has multiple representations: small/large portrait
atlases and creator tiles alongside normal HUD icons. Those have current runtime
consumers and are not leftover source assets. Removing that overlap or changing the
prepared world format requires an asset-pipeline design preserving visual behavior
and addressing publication identities; it is not silent manifest pruning.

Audit command: `nice -n 15 ionice -c 3 python3 -` with inline standard-library
size/SHA-256 grouping and metadata-range comparison, reading only
`cmake-build-release/dist-stage/Release/`. No rebuild, bake, compression benchmark,
desktop action or gameplay test was run. The disposable JSON result belongs to run
`20261111T200506Z-a885d4315fe84adeaa742ecc4808805b`; its findings are retained here,
and the run is previewed/cleaned at handoff. No extra diagnostics are retained.

#### Host dependencies and next-slice contract

Actual `readelf -d` on this staged executable lists `libOpenGL.so.0`, `libGLX.so.0`,
`libGLU.so.1`, `libm.so.6`, `libSM.so.6`, `libICE.so.6`, `libX11.so.6`,
`libXext.so.6`, `libyaml-cpp.so.0.9`, `libgcc_s.so.1`, `libstdc++.so.6` and
`libc.so.6`. Its maximum required GLIBC version observed with
`readelf --version-info` is **GLIBC_2.43**. This native development build is not
the proposed Ubuntu 22.04/glibc 2.35 baseline. No shared libraries were bundled.
Dynamic graphics/window/audio backend loads and transitive dependency closure
remain unaudited; LD02 must supply its actual baseline binary and policy.

[Retained notice provenance and gaps](../packaging/notices/README.md): IBM Plex and
Inconsolata notices are absent from the current asset directory; music and audio
replacement distribution documentation needs review. Retained library notices
must be reconciled with LD02's actual pinned dependencies and embedded codecs.
No project license or end-user terms were added, and no publication clearance is claimed.

Review recipe: inspect the payload and its sidecar inventory, reproduce `dist-stage`
with the configured toolchain, and run the `verify` command above. Keep any manual
game launch/listening review user-owned. No game window, OS input automation,
physical GPU/listening test, extracted-archive startup/save smoke, baseline runtime,
container or hosted workflow was run. Those checks do not pass by implication.

LD02 consumes this manifest/layout through a successful `dist-stage` invocation in
its separate controlled build; do not copy this host executable as its release
candidate. Preserve notices and exact content identities. Archive versioning,
hashes, user run instructions and relocation testing remain LD03; CI remains LD04.
The user accepted and closed LD01 on 2026-11-11. Acceptance covers the LD01 content
staging contract and the documented duplication findings; subsequent distribution
slices remain separate work. There is no automatic advancement; LD02 planning has
not started.

Disposable logs/fixtures belonged to agent run
`20261111T191813Z-863c6790640c4ffb9e8f910df85ea128`; cleanup is previewed and applied
with `tools/agent_artifacts.py`. No screenshots or unresolved diagnostic exceptions
are retained. The successful stage/report and existing build caches remain intentional.
