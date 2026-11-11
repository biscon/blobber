# LD03 — Versioned archive and relocated-package verification

Status: **Implemented, mechanically verified, accepted and closed by the user, 2026-11-11.**
The user reports “done, all seems to work, slice accepted.” The recorded mechanical
evidence and limits remain unchanged; no additional detailed GPU/audio results were
supplied. The user subsequently inserted WD01 (native hosted Windows preparation)
before LD04; the current distribution roadmap owns that next-slice scope.
The user approved the conversation plan and selected project-version-plus-revision
naming and a shipped opt-in smoke mode. LD01/LD02 remain accepted; LD04 is planned.
No public publication, tag push or project licensing change is authorized.

## Design and ownership

The existing Linux driver owns baseline image selection, the single-writer lock,
source/environment capture and the ordered configure/test/stage/audit/archive pipeline.
Use the configured CMake/Ninja pair and preserve the accepted baseline recipe and
`cmake/LinuxBaseline.cmake` identity. A separate distribution CMake module adds
`dist`; Python stdlib archive tools consume the same curated stage and its audit.
Incremental preparation reuses unchanged publications. Reject native/nonbaseline
builds, stale stage/audit identities and dirty source unless explicitly allowed.

Names are `blobber-VERSION-gREVISION[-dirty]-linux-x86_64`, using CMake's project
version and twelve Git revision characters. Final owned bundles live in the baseline
build's `dist/NAME/`. Each contains NAME.tar.gz, its SHA-256 checksum and build
metadata; runtime evidence is separate and bound to that archive hash. Candidate
bundles are promoted only after full validation; failure preserves an older bundle
without claiming that the attempted run succeeded. No debug symbols or review tools
enter the player payload. Dirty source status is recorded, never labelled pristine.

One matching top-level directory contains the unchanged stage, README.txt, BUILD.json
and MANIFEST.json. The manifest lists every other regular file's path, size, hash
and normalized mode (0755 executable, 0644 data). It excludes itself explicitly.
Reject links, special files, unsafe/duplicate paths and unexpected content. Stream
hashing/tar/gzip with fixed-size buffers, sorted entries, zero numeric ownership,
empty owner names and commit-time timestamps. Compression is single-threaded level 6.
The build metadata records revision/dirty status, baseline/tool/policy identities,
content fingerprints and audit linkage, excluding host absolute paths.

## Application smoke interface

`--distribution-smoke create|continue --smoke-output DIRECTORY` selects a bounded
application-owned controller, inactive during normal play. It uses a hidden window,
fixed seed 5, the default party, existing menu launch/Continue, command and travel
transactions, rendering and CampaignSaveFlow. Only synthetic samples/commands are
used; no desktop input, focus changes or host display connection. Small explicit
hooks connect menu/session/gameplay owners; main remains orchestration only.

Create presents menu/welcome/Sanctuary, dismisses welcome, renders dialogue/UI,
visits the inn and returns, enters floor one, saves through Save and Main Menu, then
exits. A fresh process discovers Continue using default paths, restores and renders
the same run/seed/location/pose/party/visited state. Settings persistence also uses
its real owner. Default asset/settings/save paths are mandatory; overrides fail.
Audio is disabled for device-free checks but bank, three music catalogs and dialogue
references are explicitly resolved and validated; device playback/listening is separate.
Each runtime is bounded to ten minutes and 4096 scripted actions. Captures are
explicit diagnostic boundaries, not normal frame work. Normal gameplay, fixed
presentation, content identities, persistence formats and steady-state allocations
remain unchanged.

## Isolation and verification

Separate pinned Ubuntu 22.04 and Debian 13 runtime-only recipes supply Xvfb/Mesa and
runtime libraries, without altering the LD02 build image. Debian uses the signed
20261110 snapshot. Ubuntu's snapshot service returned 502/503 during this run:
resolve once against signed jammy indexes, retain their InRelease hashes, and lock
all 93 downloaded packages by exact URL/SHA-256 in `runtime-ubuntu22.debs.tsv`.
Reproduction disables apt sources and installs only these verified local inputs;
it performs no rolling resolution. Base image/package URLs failing later fail
closed rather than silently updating the runtime. The lock JSON records provenance. Record actual digests,
packages, GL renderer/version and glibc. Validate/extract the archive once into the
owned run. Mount it read-only; mount only independent user-data/output and minimal
verification code. No checkout/build, host display/audio sockets or devices. Run
from an unrelated directory with isolated HOME/XDG/cache/temp paths. Check resources
and writable data paths, restored summaries and unchanged package hashes. Serialize
create/Continue and both runtimes. Software GL is explicitly not hardware evidence.

Tiny owned tests cover naming/dirty policy, deterministic bytes, inventory/hash/mode
validation, unsafe members, stale audit, failed promotion and verification linkage.
Smoke-controller tests cover progression bounds/failure/restore mismatch. Focused
existing suites: runtime_inventory, dist_staging, linux_distribution_tools, options,
packaged_options, menu, gameplay_startup, dialogue, audio, music, campaign_save_smoke,
and affected command/allocation checks. No production fixtures in routine tests;
one production archive and extraction at integration; no benchmark/full matrix.

Check memory/disk first; one intensive job, low CPU/I/O priority, at most two compile
workers, serialized preparation/tests, bounded Mesa/library threads. Disposable run:
`output/agent-runs/20261111T212856Z-8a4af59ba7e9452da84f334c526569a9/`.
Keep the final archive bundle intentionally; select at most six useful screenshots,
retain user acceptance status, close runners and preview/apply owned-run cleanup.

## OpenGL compatibility correction discovered during verification

Mesa rejected the first world frame with `GL_INVALID_OPERATION` in
`glBlitFramebuffer`: raylib's unsized depth allocations chose different actual
formats for sampled textures and renderbuffers. The correction
uses explicit GL_DEPTH_COMPONENT24 storage for both GpuResources target kinds.
This is guaranteed by the existing OpenGL 3.3 contract and makes depth transfers
legal across drivers. Allocate only during target preparation; keep ownership,
cleanup, fixed targets, prepared images and the conservative four-byte depth memory
accounting unchanged. A generated 32x32 hidden-GL regression clears/copies/reads
known depth values in both directions, checks errors, and releases both targets.
It is selected with `blobber_gpu_smoke --depth-copy` so unrelated native-UI fixtures
do not gate this focused check.
Do not suppress the failing GL error or weaken verification.

## Implementation handoff

Delivered baseline-only `dist`, deterministic streaming archive construction,
checked extraction/manifests/checksums, two runtime images and `verify-package`,
the opt-in shipped controller and source/build/content identity capture. Native
CLion trees, the accepted LD02 build image/toolchain policy, assets, content
fingerprints and save formats remain unchanged. Entry points are
`tools/linux_release.py`, `tools/distribution/`, `cmake/LinuxDistribution.cmake`,
and `src/app/distribution_smoke.*`; the optional Options pointer borrows a stack
controller owned by application::run until all menu/gameplay resources are destroyed.
GPU target preparation now chooses matching sized depth storage. No steady-state
container, resource-owner or upload path was added to ordinary gameplay.

### Final deliverable and identity

Bundle: `build-linux-baseline/dist/blobber-0.1.0-g835be710b30e-dirty-linux-x86_64/`.
Retain its archive, checksum, `build.json` and `verification.json` intentionally.
The archive contains one matching top-level directory, the 391-file staged payload
and three package metadata/instruction files. No review executables or source
artwork enter the download. No separate debug-symbol artifact was required.

| Item | Final result |
| --- | --- |
| Archive | `blobber-0.1.0-g835be710b30e-dirty-linux-x86_64.tar.gz` |
| Archive bytes | **566,499,003** (540.26 MiB) |
| Unpacked regular-file bytes | **1,337,391,900**, including manifest |
| Payload stage | 391 files / 1,337,315,804 bytes |
| Archive file count | **394** |
| Archive SHA-256 | `ea9893ebafff0c20b0e8a778b2d2762f111f07dae57692113bfc319cdd7c71a7` |
| Source revision | `835be710b30e` plus explicitly allowed uncommitted LD03 changes |
| Captured source fingerprint | `26e9940b85d4cd1202e6edfede7df404ddabb7919a50e772043cc0cbf3862dcf` |
| Baseline ABI audit | Passed; one shipped ELF, eleven host providers; maximum GLIBC_2.35 |

The dirty marker is deliberate. `baseline-environment.json` retains the complete
captured source-file hash ledger; BUILD.json retains its digest and dirty paths.
This is not a pristine/tagged public release or a byte-identical native rebuild
claim. After the final package, only documentation, generated-equipment GPU fixtures and
packaging-tool ownership/path guards changed; its executable/runtime content did
not change afterward. Two added tiny path-guard tests and the final real-bundle
preflight pass; the baseline rerun selected only distribution_archive.
Package hashes were checked after both runtime runs. LD01 rules, NPC, blueprint,
world and world-payload identities remain identical to accepted LD02.

### Reproduction and executed focused checks

From the checkout root, create a run and substitute its printed directory. These
are implemented commands, not proposed interfaces:

```sh
python3 tools/agent_artifacts.py create
nice -n 15 ionice -c 3 python3 tools/linux_release.py dist --run-dir output/agent-runs/RUN_ID --allow-dirty
python3 tools/agent_artifacts.py create
nice -n 15 ionice -c 3 python3 tools/linux_release.py verify-package --run-dir output/agent-runs/FRESH_RUN_ID --package-dir build-linux-baseline/dist/blobber-0.1.0-g835be710b30e-dirty-linux-x86_64
```

Omit `--allow-dirty` for clean source; the default rejects dirty packages. The driver
holds one writer lock, records source/environment, uses the pinned image's configured
CMake 3.31.12 and Ninja, builds selected fixtures at two workers, runs one CTest
process, then serializes game preparation/staging/audit/archive. Source hashes are
checked before and after archive construction. No incompatible baseline recipe
identity, native reconfiguration or production rebake was needed.

The final `dist` invocation used owned run
`20261111T212856Z-8a4af59ba7e9452da84f334c526569a9`; final `verify-package` used
`20261111T215836Z-01938a882c8040ee9d084f2ad7f028fc`.
All **18 selected CTest suites passed in 2.05 s** in the baseline:

```text
runtime_inventory|dist_staging|options|packaged_options|npc_metadata|dialogue|
audio|music|blueprints|library|campaign_save_smoke|linux_distribution_tools|
distribution_archive|distribution_smoke|menu|gameplay_startup|commands|buttons
```

Selection covers new package/controller behavior and its path, menu, resource and
save callers; buttons includes its engine C++ allocation guard. The nested checker
suite has seventeen cases; the archive suite now has ten fixture tests, including
permissions/layout/hashes, deterministic bytes, unsafe members, stale audits,
failed promotion, complete-runtime/hash binding and rejection before unowned lock/
symlink-directed result changes. The final integration run used eight archive cases;
the two follow-up guard cases pass in the focused baseline rerun. `python3
tests/distribution_archive_tests.py` also passes directly with tiny temporary
fixtures. No full persistence, Debug/Release/sanitizer matrix, benchmark or
production-scale performance investigation ran.

Built `blobber_gpu_smoke` with the same `/opt/cmake/bin/cmake --build /build --target
blobber_gpu_smoke --parallel 2` inside the baseline image. Ran its `--depth-copy`
selection and then its complete native-UI/resource-lifecycle check in the Ubuntu
runtime image under application-owned Xvfb. Both pass; no Mesa GL error remains.
The first complete check exposed its pre-existing missing `items/padded.png` fixture;
the test now generates all 31 equipment-icon inputs from code IDs, independent of
game artwork. The full check then passes, including pixel/overlay rollback, stale
handles, target/texture exhaustion and cleanup. Its exact runtime invocation was:

```sh
nice -n 15 ionice -c 3 docker run --rm --init --network=none --read-only \
  --cpus=2 --memory=6g --pids-limit=256 --cap-drop=ALL --security-opt=no-new-privileges \
  --tmpfs /tmp:rw,nosuid,nodev,size=128m,mode=1777 --user "$(id -u):$(id -g)" \
  --mount type=bind,src="$PWD/build-linux-baseline/blobber_gpu_smoke",dst=/gpu-smoke,readonly \
  --mount type=bind,src=OWNED_RUN/gpu-regression,dst=/results \
  --env HOME=/results/home --env TMPDIR=/results/tmp --env DISPLAY=:93 \
  --env LIBGL_ALWAYS_SOFTWARE=1 --env GALLIUM_DRIVER=llvmpipe --env LP_NUM_THREADS=2 \
  --env MESA_DEBUG=context blobber-runtime-ubuntu22:ld03 sh -c \
  'Xvfb :93 -screen 0 1920x1080x24 -nolisten tcp -ac > /results/xvfb.log 2>&1 & task_xvfb_pid=$!; trap "kill $task_xvfb_pid; wait $task_xvfb_pid" EXIT; sleep 1; nice -n 15 ionice -c 3 /gpu-smoke'
```

`OWNED_RUN` was the absolute primary run above; create its `gpu-regression/tmp` and
`gpu-regression/home` first. Add `--depth-copy` to the final executable invocation
to select only the new regression. The `:ld03` aliases refer to the image IDs below;
the normal verifier uses content-derived recipe tags and records actual image IDs.

### Actual relocation/runtime evidence

The final archive was validated and extracted once for the final two-runtime pass.
Each runtime uses the same extracted package read-only, a separate fresh writable
HOME/XDG/temp root, no network/devices/host display, and `/work` as unrelated CWD.
The checkout, `/src`, `/build` and compiler are absent. Mount flags are checked;
per-file hashes remain unchanged afterward. Each runtime launches two processes:
create/save/menu/exit, then Continue/restore/render/exit. Scripts consume 48 and
three bounded actions respectively. Fresh user-data roots contain settings,
character library, a valid scenario and the saved campaign outside the install.

| Runtime | Actual result |
| --- | --- |
| Ubuntu 22.04 | **Passed**, glibc 2.35; 194 packages; 19.195 s total; OpenGL 4.5 Mesa 23.2.1, llvmpipe LLVM 15.0.7 |
| Debian 13 | **Passed**, glibc 2.41; 169 packages; 18.824 s total; OpenGL 4.5 Mesa 25.0.7, llvmpipe LLVM 19.1.7 |

Ubuntu runtime image:
`sha256:b74d90d7a7ef35167f2d94e42285f5cd5d08100ff96107e565d1f5b06ffbee0f`.
Debian runtime image:
`sha256:dcd8d83e6616126768a625cc976aacb209d15d461288214d1f5265e24b510db0`.
Their full inventories, recipes, GL identities and successful phase records are in
the bundle's compact `verification.json`, bound to the archive SHA-256. The build
image remains accepted LD02 image `sha256:b78912195a15a804e6397e2e49e17e0466a996bcaab652093932d88f17790ee9`.

Six [selected captures](screenshots/linux-distribution/ld03/README.md) from the final
Ubuntu archive run were visually inspected: menu, welcome, Sanctuary, dialogue,
inn and dungeon. No missing art/font/layout issue was observed in these views.
They are mechanical evidence; the user subsequently accepted LD03 as a whole. Debian's repeated scenes were
not retained as duplicate evidence.

### Corrections, remaining limits and user review

Ubuntu's snapshot endpoint returned 502/503, so runtime package inputs use the
signed-index/hash-pinned fallback described above. An initial local-deb apt install
used `--no-download`, which prevented apt staging its local inputs; disabling all
repositories and installing the supplied verified local files fixes it without
rolling resolution. Failed image containers were removed with `--force-rm`.
The smoke fixture initially tried saving an empty default arena; it now supplies a
valid goblin roster and has a regression check. Mayor dialogue requires one actual
step closer from the new-campaign spawn, now included. Mesa exposed the real depth
format mismatch fixed above. Intermediate diagnostic runs reused disposable extracts
with a rebuilt executable; they were never promoted as successful archive evidence.
The final public `verify-package` command tests the exact newly produced archive.

No physical GPU, native desktop/window-manager behavior, real audio device/playback,
human listening, musl/ARM/native-Wayland support or hosted GitHub execution was
verified. OpenGL 3.3 remains the minimum contract; actual software contexts were 4.5.
Containers share the host kernel. Font/music/audio notice gaps remain unresolved;
no public publication/license clearance, tag, push or GitHub Release is claimed.

For user review, verify the adjacent `.sha256`, extract the whole archive into a
new directory, then invoke the extracted `blobber` by absolute path from another
working directory with no `--assets` override. Use a fresh HOME/XDG root if you want
to avoid touching your existing library/save. Check menu/party/welcome, town,
mayor dialogue, inn/dungeon travel, actual audio, Save and Main Menu, then exit and
restart Continue. Keep the installation read-only if convenient. Review fixed
1920×1080 text/graphics on your real desktop. The user has now accepted LD03; this recipe remains available for reproduction.
LD04 remains planned and must not start automatically.

### Cleanup and retained outputs

Closed all owned container runners, previewed each run with agent_artifacts.py,
then applied cleanup to all four owned runs. Removed **2,181 disposable files /
5,467,736,089 bytes**: logs, downloaded package-resolution inputs, intermediate
captures, isolated user data/caches and all temporary extracted trees. The two
additional failed-check runs were
`20261111T214612Z-07e37cafdbb54355b15350a398f39580` and
`20261111T214853Z-b2d3b052464a42609305d959178832d0`.

Retained only the selected final distribution bundle, accepted baseline build/image
caches, the two runtime image caches and six selected captures. No unresolved-failure
diagnostic exception, temporary image container or scratch extract remains. No user
saves, unrelated runs, source assets, build caches or Git history were removed.
The documentation/link and whitespace checks pass. The user subsequently accepted
LD03 explicitly; that acceptance, rather than cleanup, closes the slice. No LD04
implementation or Windows port has started.
