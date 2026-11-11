# LD02 — Controlled Linux baseline and dependency closure

Status: **Implemented, mechanically verified, accepted and closed by the user,
2026-11-11.** The user reports it is working fine and explicitly approves LD02.
LD01 remains accepted. This slice produces a baseline stage and ABI/dependency
evidence, not an archive, playable relocation claim, hosted run or publication.

## Design and ownership

Build against Ubuntu 22.04 amd64 / glibc 2.35 in a digest-pinned Docker image,
independently of the EndeavourOS development host. Pin archive package resolution
with an Ubuntu snapshot and record the entire package inventory. Use GCC 11,
Kitware CMake 3.31.12 (hash verified), the snapshot's Ninja and binutils. Fetch the
existing hash-pinned raylib/JSON sources and commit-pinned yaml-cpp into the image;
configuration/build run without networking and cannot discover host dependencies.

`packaging/linux/` owns the recipe, lock and runtime policy. A Python stdlib driver
owns image preparation, a marked `build-linux-baseline/` tree, isolated test homes,
and the ordered configure/test/stage/audit pipeline. The checkout is read-only at
`/src`, the separate build is writable at `/build`, and logs/temp files belong to
an explicit agent run. Run as the caller's UID/GID. Reject a changed image/recipe/
configuration identity rather than silently recycling an incompatible cache.
Existing CLion trees and toolchains are never used or changed.

The opt-in `BLOBBER_LINUX_BASELINE` CMake mode requires Linux x86-64, GCC 11,
graphics and single-config Release. It forces pinned source discovery, static
raylib/yaml-cpp, static libstdc++/libgcc and generic x86-64 compiler flags. glibc,
its loader and desktop libraries remain dynamic. Select OpenGL 3.3, X11, no native
Wayland. Native configuration defaults remain unchanged. There is no frame-path,
asset identity, save-format or gameplay change.

LD01 retains ownership of runtime selection and transactional staging. The driver
builds only selected test targets, runs their focused tests, then calls `dist-audit`
which invalidates the old audit before invoking a fresh successful `dist-stage`. All production assets are baked
in this controlled build; no host publication is imported. One worker serializes
production preparation. Source mount paths are stable; prefix mapping removes
build paths from compiled diagnostics. Build metadata records revision/dirty state,
image/lock identity, tool versions and the actual configuration.

## Audit interface and failure behavior

The stdlib `tools/distribution/elf_audit.py` CLI consumes `--build`, `--stage`,
`--policy` and `--output`. It uses the image's readelf under LC_ALL=C, scans files
by ELF magic, and follows DT_NEEDED through baseline library providers. Inspect
all shipped ELFs and transitive bundled ELFs, architecture/interpreter, required
symbol versions, search paths and CPU notes. Numerically compare GLIBC tuples to
2.35; reject private/unknown requirements in shipped ELFs, missing/unexpected
providers, shared
compiler runtimes/raylib/yaml-cpp in shipped objects, and any shipped RPATH/RUNPATH. Host libraries retain their packaged search paths;
the host glibc/loader pair may use its own GLIBC_PRIVATE interface. Check
actual compile and link commands for baseline flags, static linkage and host leaks.
Validate versioned imports against the provider's exported symbol/version pairs,
including required dlopen roots (X11 and GLX) and their transitive closure.
Host library closure is recorded separately and does not become bundled content.

A checked-in policy enumerates desktop library families and required/optional
GLFW/miniaudio runtime loads from their pinned sources. Resolve required loads in
the baseline image and report optional absence without claiming runtime playback.
Do not infer dlopen closure from DT_NEEDED or use the developer's library paths.

Invalidate the prior successful audit before beginning a new driver run or audit.
Failures exit nonzero; atomic success output binds the current stage ID, ELF hashes,
build metadata and policy hashes. Existing stage files cannot alone establish
success. The build uses one writer with a lock. Recipe/image mismatch requires a
new dedicated build directory; never delete unrelated output automatically.

## Notices and boundaries

Reconcile retained notices with pinned source code, embedded components/codecs and
static compiler runtime notices, through the shared runtime manifest. Preserve
existing font/music/audio documentation gaps and proprietary-project licensing
status. No project license, end-user terms or publication clearance is introduced.
LD03 owns archive/version naming, extracted gameplay/resource/save verification;
LD04 owns hosted automation. Container ABI evidence is not physical GPU/desktop
compatibility, human visual acceptance or listening evidence.

## Verification and resource budget

Use generated tiny ELF fixtures and small parser/policy tests: valid closure,
wrong architecture/interpreter, malformed input, numeric version ordering,
too-new/private requirements, unexpected/missing dependencies, transitive bundled
failure, search paths, CPU flags, imported symbol versions and stale-result removal.
Run runtime_inventory, dist_staging, options, packaged_options, npc_metadata,
dialogue, audio, music, blueprints, library and campaign_save_smoke in the baseline.
One production Release build and stage/audit; no slow persistence/matrix/benchmark.

Check memory/disk before production work. One intensive job, low CPU/I/O priority,
one production build worker (at most two for focused compilation), one test process,
OMP/OpenBLAS threads=1 and Mesa workers=2. Docker CPU/memory limits keep the desktop
responsive. No desktop input, window control or GPU smoke is authorized here.

Disposable run: `output/agent-runs/20261111T203403Z-c93ac86d82204c0f9145caf7c8ddce00/`.
Executed commands, outcomes and completed cleanup are recorded below.

## Implementation handoff

Delivered the recipe/lock/package inventory, isolated build driver, opt-in CMake
policy, ELF checker, runtime-load policy, generated fixtures, reconciled notices and
build instructions. The user accepted and closed LD02 on 2026-11-11 after reporting
that it is working fine. The existing mechanical evidence and limitations remain
unchanged; no additional detailed desktop/graphics/audio coverage was supplied.
LD03 is next for separate planning when requested; no automatic advancement.
No native CLion tree was reconfigured, built or used as an input.

### Reproduction and retained outputs

From the checkout root, with Docker access:

```sh
python3 tools/agent_artifacts.py create
python3 tools/linux_release.py build --run-dir output/agent-runs/RUN_ID
```

The implementation ran that command with run ID
`20261111T203403Z-c93ac86d82204c0f9145caf7c8ddce00`. After checker/policy corrections,
ran only the focused checker plus incremental staging/audit:

```sh
python3 tools/linux_release.py audit --run-dir output/agent-runs/RUN_ID
```

The full command configures with image CMake 3.31.12 and `/usr/bin/ninja`, builds the
selected fixture targets at `-j 2`, runs the documented twelve CTest selections at
`-j 1`, then invokes `cmake --build /build --target dist-audit -j 1`. The latter
invalidates the old audit before invoking the configured CMake's serialized
`dist-stage`, LD01 `verify`, and the ELF checker. This ordering also handles a
prerequisite compilation/staging failure without preserving an old success claim.
`audit` checks environment/notices and the seventeen checker cases, then the same
stage/audit target; it does not rerun the other passing suites or rebake assets
whose inputs are unchanged.

Intentional retained outputs (ignored, not disposable evidence):

- `build-linux-baseline/`: independent marked cache, compiler commands and build/
  environment metadata, approximately 2.6 GiB including runtime content.
- `build-linux-baseline/dist-stage/Release/`: **391 files / 1,337,241,412 bytes**.
- Adjacent `Release.inventory.json` and `Release.audit.json` (about 7 KiB audit),
  the manifest/content inventory and machine-readable ABI/dependency evidence.
- Recipe-hashed Docker image `blobber-baseline:2a741990652b8d049e17`, also initially
  built as `blobber-baseline:ld02`; image ID
  `sha256:b78912195a15a804e6397e2e49e17e0466a996bcaab652093932d88f17790ee9`.

These are LD02 build/review inputs for LD03, not an archive or publicly published
release. The native LD01 stage remains independent.

### Actual environment and results

| Item | Verified identity/result |
| --- | --- |
| Baseline | Ubuntu 22.04 amd64, glibc 2.35; archive snapshot `20261110T000000Z` |
| Base image | `ubuntu:22.04@sha256:8a2321a92b438238f93b4346fff61bc4259c99fdd4d9b52ee86f153fee0c344f` |
| Compiler | GCC/G++ 11.4.0, package `11.4.0-1ubuntu1~22.04.3` |
| CMake / Ninja | 3.31.12 / 1.10.1 |
| Binutils / Python | 2.38 / 3.10.12 |
| Package lock | 251 package/version/architecture entries; SHA-256 `a7d18868e90d8e0f58fc7213c1bc9f84a768aed804533e00df5c960ddaa9a8f7` |
| Source | `ed293ee6c9450d00c9f7e9f94cabb2c4d8bc510f` with LD02 working changes; dirty state recorded, no pristine release claim |
| Final stage ID | `1794431528765842008` |
| Executable | 9,985,800 bytes; SHA-256 `c476de0b523f1848404ef4437a7792d23166d6f6a61c0faec6e546e8d7dbd36d` |
| Maximum shipped GLIBC requirement | **GLIBC_2.35** |
| Shipped ELF count / host closure | 1 / 11, including required runtime-loaded GLX |

All **12 focused CTest suites passed in 1.79 s** inside the controlled baseline:
blueprints, packaged_options, options, dialogue, library, npc_metadata,
campaign_save_smoke, music, audio, runtime_inventory, dist_staging and
linux_distribution_tools. The checker suite initially contained thirteen cases;
after adding host glibc-private/search-path distinctions, stream-boundary leak
checking and required-dlopen closure coverage, all **17 checker cases passed in
0.128 s** in the baseline. These include actual tiny generated ELF consumers and
providers; no game assets or GPU are fixture inputs. No unrelated matrix ran.

One baseline game compilation and production world/portrait/creator preparation
completed. Later audit corrections reused those outputs; no repeated production
bakes or recompilation were needed. The final successful audit checks generic CPU
flags across all compile commands, actual static-runtime link flags, every shipped
ELF, numeric ABI limits, imported versioned symbols, direct/transitive providers,
required runtime loads, forbidden shipped search paths and build-path leakage.
`git diff --check` and relevant local documentation links were checked.

LD01 content identities match exactly: rules `16064805046093731371`, NPC definitions
`16167988745088981044`, blueprint `11177173040059835938`, world
`16850201644598948673`, payload checksum `162051066310005404`. The three extra
files are dependency notices; the executable grows with static compiler runtimes.
No runtime asset/fingerprint rewrite occurred.

### Actual dependency/runtime policy

Direct DT_NEEDED: `libm.so.6`, `libX11.so.6`, `libc.so.6` and
`ld-linux-x86-64.so.2`; interpreter `/lib64/ld-linux-x86-64.so.2`.
The remaining checked closure is `libxcb.so.1`, `libXau.so.6`, `libXdmcp.so.6`,
`libbsd.so.0`, `libmd.so.0`, plus runtime-loaded `libGLX.so.0` and its
`libGLdispatch.so.0`. Every imported versioned symbol resolves in the baseline.
The game requires no shared raylib, yaml-cpp, libstdc++ or libgcc. Nothing from the
host graphics stack or glibc is bundled. The policy allows known baseline desktop
link variants rather than treating the old EndeavourOS DT_NEEDED list as required.

Required X11/GLX libraries resolve. Optional GL/EGL and listed X11 extension
libraries resolve; OSMesa, JACK and sndio are absent, as recorded. PulseAudio and
ALSA resolve; PulseAudio's own packaged library search path is recorded and is not
shipped in the game. OpenGL vendor-driver loading remains host-owned. The game uses
GLX/OpenGL 3.3; unused EGL/OSMesa alternatives do not establish alternate support.
Audio libraries resolving does not establish a working server/device or listening.

Retained notices are checked against the actual image sources before compilation.
Compiler-runtime notices include the full packaged copyright (with Runtime Library
Exception 3.1) and referenced GPLv3 text. Existing IBM Plex/Inconsolata and
music/audio provenance gaps remain; no project licensing change/publication
clearance is claimed. See [notice provenance](../packaging/notices/README.md).

### Corrections, limits and cleanup

Initial image preparation exposed the minimal base's missing CA certificates and
missing xz extractor. Added a public hash-pinned CA bootstrap input and explicit
xz-utils; signed snapshot metadata and TLS verification remain enabled. The first
production audit correctly failed on a missing allowlist entry for the standard
glibc loader. A subsequent audit exposed overly strict treatment of a host
PulseAudio search path. Restrictions now apply to shipped ELFs, with separate
host-library evidence and regression coverage. Failures left no successful audit
sidecar; the final recorded result passes. No unresolved diagnostic exception.

The source checkout is read-only to container processes, not an immutable source
snapshot against concurrent authoring; use one writer and do not edit inputs while
building. Package/tool/source pins permit environment reproduction, not byte-identical
native rebuilds. Containers share the host kernel. Agent verification included no baseline desktop window,
physical GPU, newer-distribution runtime, playback/listening, extracted archive,
relocated startup/save test, GitHub run, tag/push or publication was performed.
The user subsequently reported working behavior and accepted LD02; extracted
archive verification and hosted automation remain LD03/LD04 work. No portability beyond the declared
ABI/dependency policy is asserted.

Disposable logs, downloaded audit inputs and generated notices/fixtures were kept
in the owned run. After selecting this durable handoff, previewed and applied cleanup with
`tools/agent_artifacts.py`: 17 disposable files / 1,522,870 bytes removed. No
screenshots or diagnostic exceptions remain. Removed only the two known failed
image-build containers (`df7e08c83183`, `33bb02d2fb35`), preserving all unrelated
containers, images and caches. Build cache/image, stage and compact sidecars remain
intentional.

