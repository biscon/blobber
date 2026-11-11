# Linux distribution: proposed delivery roadmap

Created: 2026-11-11.
Status: **LD01 and LD02 accepted and closed by the user, 2026-11-11.
LD03–LD04 remain planned.**

Deliver a downloadable Linux x86-64 game archive containing the executable and
only its required runtime content. The user will run a separate planning step and
implementation step for each slice in fresh contexts, following the existing
[campaign roadmap](procedural_campaign_plan.md) workflow. Do not implement or
advance through slices automatically. Runtime behavior, rendering and gameplay
acceptance retain their existing status.

## Current checkpoint

| Field | Current value |
| --- | --- |
| Active slice | None — LD02 accepted and closed |
| Next eligible slice | LD03 — separate detailed planning; not started |
| Implemented distribution support | Curated staging plus pinned Ubuntu 22.04 baseline build and passing ELF/dependency audit; no archive/CI |
| Awaiting user review | None for LD01/LD02; [accepted LD02 handoff](linux_distribution_ld02.md) retains evidence and limitations |
| Known environment | Public repository on github.com, confirmed by the user |
| Next action | Separate LD03 planning when requested; do not advance automatically |

Update this checkpoint, the tracker and the active slice's handoff together.
Implementation completion and user acceptance are separate states. Only the user
can mark a slice Accepted; review corrections remain within that slice.

## Intent and boundaries

The first supported download is a versioned Linux x86-64 .tar.gz: extract it and
run the game without a compiler, the source checkout or administrator installation.
Declare a concrete compatibility baseline and verify it rather than claiming
support for every Linux distribution. Initial direction is glibc-based desktop
Linux, OpenGL 3.3 and X11/XWayland. Native Wayland, Linux ARM, Alpine/musl, NixOS,
Windows, macOS, browser builds and store integration are outside these four slices.
AppImage may be a later wrapper around the same staged package; it is not required
to complete this roadmap.

Preserve fixed 1920×1080 presentation, discrete movement, prepared assets, current
gameplay, and campaign/save compatibility. Packaging must not alter catalog or
publication identities merely to reduce the download. Keep authoring assets,
prompts and editable models in the repository; excluding them from a package does
not authorize deleting or changing them.

The repository is currently public, but no project license change is requested.
The user wants to preserve the option to make future development closed source.
Do not add an open-source license or invent end-user terms as packaging work.
Retain existing applicable third-party license/attribution notices in the package;
record any unresolved notice or distribution requirement for user decision.

## Findings to recheck before implementation

These are pre-LD01 observations retained as roadmap context. LD01's current
implementation, inventory, observed dependencies and limitations are in its
[handoff](linux_distribution_ld01.md#implementation-handoff); the broad copy list
and missing blueprint below have been corrected. Later slices must inspect their
own actual build rather than treating these historical observations as tests.

- CMakeLists.txt stages broad recursive asset directories for Release and prepares
  world textures, portraits and creator artwork. There are no install, dist or
  CPack targets, and no .github workflow directory at this checkpoint.
- Broad NPC/UI/audio staging includes authoring material. Conversely, runtime
  loads blueprints/underkeep.json, but the current copy list omits blueprints.
- src/app/options.cpp defaults to assets beside the executable. Debug switches
  raw assets to the source checkout while retaining prepared assets in the build
  tree. Distribution must use Release paths and verify relocation without an
  explicit --assets argument that could hide a packaging error.
- JSON/YAML/TSV files are not all disposable metadata. NPC definitions and animation
  data, rules, shops, dialogue, playlists, portrait registration and prepared-world
  metadata can be runtime inputs. Blueprint startup also reads the blueprint hash
  from prepared/world/sources.json. Audit consumers before trimming anything.
- The inspected local Release executable requires GLIBC_2.43 and a host
  libyaml-cpp.so.0.9. It is a development artifact, not a portable release candidate.
  Reinspect actual binaries; do not hard-code these historical observations into
  success tests.
- GLFW loads graphics/window-system libraries at runtime; audio can load host
  backends too. ELF DT_NEEDED inspection alone does not enumerate all dependencies.
- Prepared assets were approximately 945 MiB in the inspected Debug tree, before
  other runtime content. That is an on-disk observation, not an archive-size goal.
  Check disk/memory and use bounded compression before generating large packages.

Relevant entry points: CMakeLists.txt, CMakePresets.json, cmake/WorldAssets.cmake,
src/app/options.cpp, src/app/menu.cpp, src/app/session_launch.cpp,
src/assets/blueprint_loader.cpp, src/assets/npc_metadata.cpp,
src/assets/world_assets.cpp, src/ui/art.cpp, src/platform/audio.cpp and
src/persistence/. Read actual asset references and loaders during LD01; this list
is a starting point, not an authoritative runtime manifest.

## Responsibilities and compatibility design

One packaging manifest owns the runtime file inventory. Prefer explicit entries
and constrained groups, expanded through real runtime references where useful,
over copying whole source directories and maintaining a growing exclusion list.
The same inventory should serve Release staging and distribution where practical;
do not introduce parallel lists that silently diverge. Generated publication
outputs remain owned by the existing bake pipeline.

CMake orchestrates preparation and the proposed dist-stage/dist targets. A small
cohesive packaging helper may resolve/validate inventory, stage files, inspect ELF
requirements and write archives/manifests. Use existing install/CPack facilities
if they simplify this; no generic packaging framework is required. Packaging work
runs outside frame processing and introduces no steady-state work or allocations.

Stage only into an explicitly managed output directory, start from an empty
inventory and reject missing inputs or unsafe paths. Remove stale files only
within packaging-owned output. Failed preparation/validation must not present an
old or partial package as a successful new result. Preserve build caches, source
assets, saves and unrelated output. Define final artifact promotion in LD03.

Use an isolated release build environment rather than modifying the existing
CLion Debug/Release trees. Proposed baseline: Ubuntu 22.04 / glibc 2.35, with an
explicitly pinned compatible CMake/compiler/dependency environment selected in
LD02. This is a proposed minimum, not verified support. Keep a generic x86-64 CPU
target; do not inherit -march=native or host-only instruction requirements.

Build pinned raylib and yaml-cpp statically. Select static or packaged compiler
runtimes deliberately and verify the result. Keep glibc and the graphics driver
stack dynamic: fully static glibc does not make dynamically loaded host drivers
self-contained. Do not bundle a host GPU driver or use a private libc/loader as
an unplanned workaround. Identify which window/audio libraries must be supplied
by the host and which are bundled. Audit every shipped ELF, including bundled
libraries, for architecture, dependencies, symbol versions and build-path leakage.

Dist-stage from LD01 establishes content correctness only. Portable-distribution
claims require LD02's baseline build and LD03's extracted-package verification.
Deterministic packaging of identical inputs is desirable; byte-identical native
rebuilds across time are not an acceptance requirement of this roadmap. Record
source revision, toolchain and content identity so results can be reproduced.

## Delivery slices

| Slice | Deliverable | Dependency | Status |
| --- | --- | --- | --- |
| LD01 | Runtime inventory and clean dist-stage target | Existing game and asset pipeline | Accepted and closed — 2026-11-11 |
| LD02 | Controlled Linux baseline build and dependency/ABI checks | Accepted LD01 content contract | Accepted and closed — 2026-11-11 |
| LD03 | Versioned archive, checksums and relocated-package verification | Accepted LD02 release build | Planned |
| LD04 | GitHub-hosted automation and compatibility evidence | Reviewed LD03 package/verification commands | Planned |

### LD01 — Runtime content and clean staging

Approved [detailed design and handoff](linux_distribution_ld01.md), 2026-11-11.
Implementation and focused verification are complete; **accepted and closed by the
user on 2026-11-11**.
The user reports working content and clean folders. A follow-up audit found about
1.5 MB (0.114%) of existing repeated material/sound data. The user accepted these
findings and clarified that the concern was shipping both packed world textures
and their original files; the stage does not do that. Details remain in the handoff.
The local stage contains
388 files / 1,334,576,722 bytes. Its native executable is
not portable; no archive, desktop smoke or hosted run is claimed. See the handoff
for commands, inventory location, notice gaps, cleanup and LD02's contract.

Audit runtime file access and build a single shipping inventory. Include all
required prepared outputs, runtime definitions/scripts/fonts/audio/UI images and
existing notices. Exclude original sheets used only by baking, reference art,
prompts, revision folders, editable models, tests and bake/review executables.
Retain metadata that consumers actually require. Avoid rewriting definition JSON
or stripping fingerprint inputs unless the detailed plan demonstrates unchanged
semantics and identity. Add the missing blueprint content to relevant staging.

Add dist-stage with Release/configuration validation and clear ownership of its
destination. Preserve the current convenient Debug source-asset workflow. Define
the staging layout that later packaging consumes and how generated inputs are
built before staging. Fail clearly for missing required content rather than
silently relying on a stale build-tree asset directory.

**Focused verification:** small generated fixtures for manifest selection,
reference resolution, missing files, stale-file replacement, unsafe paths and
configuration rejection; relevant existing options/asset-loader checks. Production
inventory validation complements these fixtures but must not become their input.
Do not bake production assets repeatedly just to test selection rules.

**Exit evidence:** successful clean staging, required/excluded file inventory and
sizes, focused results, documented residual dependencies and staging contract for
LD02. No portable ABI claim yet. Record exact commands in this slice's design and
handoff, not a new permanent report for every attempt.

### LD02 — Controlled baseline and dependency closure

Approved [detailed design/handoff](linux_distribution_ld02.md); implementation
authorized 2026-11-11. **Implemented, mechanically verified, accepted and closed
by the user, 2026-11-11.** The user reports it is working fine. One actual Ubuntu
22.04 Release build, 391-file stage and ELF/dependency
audit pass. Maximum GLIBC requirement 2.35; static raylib/yaml-cpp/compiler runtimes.
Twelve focused suites and seventeen checker cases pass. Exact evidence, corrections,
notices and limits remain in the accepted handoff. LD03 planning has not started.

Choose and pin a Linux build environment compatible with the proposed baseline.
A digest-pinned container recipe can run locally and inside a GitHub-hosted VM;
record compiler/CMake versions and deliberate package/dependency resolution.
Ubuntu's default CMake may be older than the project's 3.24 requirement: choose a
compatible tool explicitly. Reuse the repository's pinned dependency fallbacks,
and ensure system discovery cannot accidentally select the host's yaml-cpp or
raylib. Use a separate build tree and keep its configured CMake/Ninja together.

Build the game and LD01 staging with deliberate linkage. Add machine-checkable
ELF architecture, dependency and ABI-baseline checks, including transitive bundled
ELFs and numeric symbol-version comparison. Account separately for dlopen-loaded
window, graphics and audio dependencies. Check generic CPU compilation flags.
Do not lower the claimed minimum merely because the executable starts on the
developer's newer distribution.

**Focused verification:** owned fixtures exercise checker failures, including a
too-new symbol/dependency and unexpected shared library. Perform one real baseline
Release build and dependency audit, then run selected content/options/persistence
checks appropriate to changes. Keep existing native build trees intact.

**Exit evidence:** reproducible build entry point, actual baseline-built executable
and stage, dependency/runtime policy, ABI audit and compiler/environment identities.
Record unavailable container/network facilities as limitations; a recipe that has
not run is not evidence of a successful baseline build. Hand off exact local and
future CI entry points to LD03/LD04.

### LD03 — Downloadable archive and relocation verification

Add dist, consuming the same manifest/staging and baseline build. Produce a
versioned Linux x86-64 .tar.gz with one top-level game directory, preserved execute
permissions, required dependency notices, concise run instructions, an inventory
with file sizes/hashes and build/content identity, and an archive SHA-256 checksum.
Define version/revision naming and dirty-tree behavior. Clearly identify local
nonbaseline packages; do not label them portable release candidates. Keep any
separate debug symbols outside the player download.

Verify the extracted archive from an unrelated working directory, using default
asset discovery. Isolate home/XDG/settings/library/scenario/save locations under
the owned run. Make the checkout and original build assets unavailable to the
test process through an isolated environment or equivalent evidence; do not rename
the user's checkout to achieve this. --help alone cannot establish playable
packaging. Confirm that installation content can remain read-only while user data
is saved outside it.

Use the smallest in-process application-owned smoke route needed to exercise
menu/startup, a fixed-seed new campaign, town/dungeon/interior resource loading,
save and Continue. Include dialogue/UI and audio asset resolution; report actual
listening separately. Use existing helpers where they reach the same shipped
content, and avoid adding a large automation subsystem. Do not ship the review
tools merely because they are useful for verification.

**Focused verification:** small archive/layout/hash/permission fixtures, followed
by one production archive extraction and selected startup/save/resource checks.
Exercise the baseline runtime and one newer runtime where available. Use hidden
application-owned OpenGL capture or a virtual display/software GL when supported;
label software GL as such. Never inject OS input, change focus or control desktops.
Container/CLI checks do not establish physical GPU compatibility or human review.

**Exit evidence:** usable archive/checksum, unpacked/runtime sizes, relocated smoke
results, explicit distribution/graphics/audio coverage and a user review recipe.
Keep the selected final package as an intentional deliverable in the documented
build distribution location; clean disposable intermediate copies and extracts.

### LD04 — Hosted automation and compatibility evidence

Use standard GitHub-hosted Linux runners for the confirmed public repository;
no self-hosted machine, separately rented VM or cloud account is required.
Run LD02's controlled environment and LD03's packaging/verification commands
unchanged in CI. An explicit runner OS label still receives image updates; it is
not a substitute for the pinned build environment. Start with manual dispatch and
deliberate version-tag triggers rather than baking/uploading full assets on every
unrelated change. Check how all required assets reach a clean checkout, including
Git LFS/submodules only if actually used.

Use one substantial build job at a time, at most two build workers and one test
process, bounded library threads, timeouts and concurrency controls. Serialize
compatibility checks and reuse the built package instead of rebuilding for each
distribution. Validate at least the declared baseline and a newer distinct Linux
distribution userspace; a second container shares its host kernel and does not
prove another desktop/GPU stack. Document the exact coverage and limits.

Upload the archive, checksum and compact verification metadata as workflow
artifacts with explicit short retention; avoid duplicate uncompressed stages and
large logs. Define bounded caches without assuming production bakes are free.
Retain third-party notices and distinguish GitHub artifact downloads from a
public GitHub Release download. Document the release attachment procedure; any
automated public publication requires an explicitly approved publication scope.
Writing this roadmap or workflow does not authorize pushing tags or publishing.

Standard hosted runner compute is free for public repositories, but artifact/cache
storage has separate policies and allowances; do not assume unlimited free storage
for this asset-heavy project. No paid/larger runner or self-hosted setup is needed
by default. Review current GitHub policy during this slice rather than embedding
fixed pricing/quota numbers in build scripts.

**Focused verification:** workflow syntax/static checks and the same local smoke
commands; when authorized/access is available, observe one successful hosted build
and inspect its downloaded artifact. Local success cannot be reported as an actual
GitHub Actions run. Do not silently enable paid services to clear a blocker.

**Exit evidence:** checked-in workflow and user-facing release instructions,
baseline/newer-userspace results, actual hosted artifact/run link when executed,
and explicit remaining hardware or interactive checks. A workflow awaiting its
first hosted run must be recorded as such before user acceptance.

## Fresh-context execution and handoff

For each slice, read AGENTS.md, system_design.md, this checkpoint/tracker, the
previous slice's handoff and the relevant current code. Preserve working-tree
changes. Work on one selected slice; do not launch another agent or advance to
the next slice implicitly.

During the planning step, mark the slice In progress — planning and write its
practical design under docs/, linked from system_design.md and this roadmap.
Specify ownership/interfaces, paths, dependency choices, failure behavior,
verification cost and open decisions in proportion to that slice. Do not create
empty future design files or imply that proposed commands already exist. Stop
for the user's implementation instruction. Existing explicit authorization in
that slice's conversation suffices; do not request it twice.

During implementation, complete the authorized slice, run focused checks and
update its design/handoff, this tracker and the system overview. Stop at Ready
for review. User acceptance and limitations must remain visible for the next
fresh context. If scope needs a separately reviewed split, record it explicitly
without silently dropping exit requirements or renumbering completed history.

**Planning prompt:**

> Read AGENTS.md, docs/system_design.md and docs/linux_distribution_plan.md.
> Inspect the current checkpoint, previous slice's handoff and relevant code.
> Plan the next eligible LD slice only, with a linked detailed design, for review.
> Preserve working-tree changes. Do not implement, publish or advance slices.

**Implementation prompt:**

> Implement the approved LD slice from docs/linux_distribution_plan.md and its
> detailed design. Preserve working-tree changes, follow the configured toolchain,
> resource and desktop restrictions, run focused checks, update tracking/designs
> and handoff, clean owned disposable output, and stop at Ready for review.

Each slice handoff records: approved scope and date; delivered behavior and file
entry points; exact verification commands/results; failed/skipped/unavailable
checks; package/run locations and retained evidence; review recipe; deviations;
next-slice contracts/pitfalls; and user acceptance or explicitly pending review.

Follow [verification_workflow.md](verification_workflow.md): low-priority lengthy
commands, check free disk/memory first, one intensive job, small owned fixtures,
no routine benchmark/stress matrix. Use tools/agent_artifacts.py for disposable
output even after failed checks. Promote only selected useful screenshots, at
most six per feature or twelve pending review, with a README preserving acceptance
status. Keep verification in the slice design; historical reports follow the
existing archive policy. Final release artifacts are intentional outputs with a
documented location, not an excuse to retain every scratch archive or extracted
asset tree.

## References and roadmap verification

Policy references checked 2026-11-11; recheck when implementing hosted automation:

- [GitHub-hosted runners](https://docs.github.com/en/actions/concepts/runners/github-hosted-runners):
  GitHub supplies managed Linux, Windows and macOS execution environments.
- [GitHub Actions billing](https://docs.github.com/en/billing/concepts/product-billing/github-actions):
  standard public-repository hosted compute is free; storage is accounted separately.
- [AppImage compatibility guidance](https://docs.appimage.org/reference/best-practices.html):
  the payload's build baseline matters independently of the packaging format.

Original roadmap-only verification, before LD01 implementation: this roadmap was
checked against the existing build/path/loading code and roadmap
format. No distribution code, build, package, hosted workflow or desktop test was
run as part of writing it. Documentation verification: git diff --check passes;
a focused Python check confirms all local roadmap links resolve, each LD01–LD04
heading occurs once and system_design.md indexes this roadmap. The owned disposable
run was previewed and cleaned with tools/agent_artifacts.py; no output was retained.
These checks do not establish distribution support.
Current LD01 implementation verification is recorded in its linked design/handoff.
