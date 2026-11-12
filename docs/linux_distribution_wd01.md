# WD01 — Windows readiness and manual review build

Status: **In progress — implementation, 2026-11-12. Design reviewed; the user
instructed continuation. Mechanical verification and user acceptance pending.**

This document records the reviewed WD01 design and its implementation progress.
Requirements below remain the target contract; the handoff records which checks
actually ran. Accepted LD01–LD03 behavior and acceptance remain in the
[roadmap](linux_distribution_plan.md) and [LD03 handoff](linux_distribution_ld03.md).
The user approved this design and authorized WD01 implementation after the planning
handoff. Complete WD01 and stop at Ready for review; do not start LD04 or publish.

## Decisions and existing constraints

The user selected Windows 11 x64 / OpenGL 3.3, native MSVC on a standard hosted
Windows runner, Linux preparation followed by Windows compilation through a shared
validated cache, and agent-managed merging of the narrow workflow bootstrap PR
during the later authorized implementation. Development and `gh` management remain
on Linux; the user needs only the downloaded game for dual-boot play review.
No Windows compiler, CMake, Git or agent installation on the user's machine is
required. Cross-compilation, Wine, installers, signing and paid services are out.

Both WD01 and the eventual LD04 workflow are **manual-only**: GitHub's Run workflow
button or one `gh workflow run` command. No push, PR, merge, tag or Release event
automatically starts builds. Building a review artifact does not publish a public
GitHub Release; publication/tag operations require an explicit user instruction.
This supersedes the roadmap's earlier suggestion of deliberate version-tag triggers.

Preserve C++17, prepared images, fixed 1920×1080 presentation, discrete movement,
current game behavior and all content/save compatibility identities. Do not change
the project license. Existing unresolved font/music/audio notice gaps stay visible.
Packaging/preparation/I/O may allocate; ordinary engine-owned frames gain no new
allocations, handles, callbacks or filesystem work.

The user authorized committing the previously uncommitted work. It was preserved
unchanged in prerequisite commit `08217e6` before these planning edits. Do not amend
that snapshot to introduce WD01 code or rewrite LD03 package provenance: the
accepted LD03 archive remains the recorded dirty build of its historical revision.

### Findings before implementation

- `CampaignDisk` uses POSIX descriptors, blocking `flock`, bounded regular-file
  reads, a checked temporary write, two observed-revision checks, rename and
  directory `fsync`. Publication and durability are separate result fields.
- `CharacterLibrary` uses a nonblocking writer lock, exact observed bytes, exclusive
  `.tmp`/`.bak.tmp` files, backup-before-replacement and `/dev/urandom`. Existing
  recovery artifacts prohibit accidental initialization; lock files alone do not.
- `ArenaScenarioStore` uses a blocking folder lock, bounded regular-file reads,
  stale selection checks and a hard-link publication for create-without-overwrite.
- Library/campaign defaults derive from HOME/XDG; settings use a separate HOME/XDG
  helper. Settings and playlist writers rely on POSIX replacement by filesystem
  rename. Filename `.string()` conversions, YAML filename APIs, raylib/audio
  filename APIs and application-directory discovery need explicit Unicode treatment.
- CMake always attaches world, portrait and creator preparation to the game.
  Copying Linux publications into a Windows build alone does not remove bake-tool
  dependencies. Staging currently hardcodes Linux x86-64 and the name `blobber`;
  `archive.py` imports `fcntl` and implements Linux-only naming/audit/locking.
- World publication 13 stores texture/mask bytes and explicitly little-endian
  IEEE-754 mesh words; metadata and other runtime formats are JSON/YAML/TSV/QOI.
  This is a plausible portability basis, not evidence that the Windows loaders work.
- Owned tests include POSIX temporary-directory/locking/wait APIs, GCC bit-scan
  builtins and allocation helpers. Port affected selected fixtures rather than
  disabling the required coverage or attempting an unrelated entire-suite rewrite.

## Platform boundaries and persistence behavior

Add a small graphics-independent OS library with one header and separate POSIX/
Windows implementations selected by CMake. It has no raylib, gameplay, JSON or YAML
dependency. Persistence and audio I/O borrow its functions at existing loading/save
boundaries. Do not introduce a virtual filesystem or injectable service hierarchy.

The implemented interface provides move-only file/lock ownership, bounded regular-file
reads with explicit missing-file results, exclusive temporary creation, checked
write-all/flush/close, replace-or-create publication, publication confirmation and
secure entropy. File owners close on every exit; temporary owners delete only files
they actually created. Explicit checked close reports errors before publication;
destructors perform best-effort cleanup. Errors preserve native codes for diagnostics
and are mapped by each store to its existing result/error contract.

Keep the stores responsible for schemas, byte limits, expected revisions, lock
choice, backup order, recovery decisions, in-memory commit and user messages:

| Store | Preserved behavior |
| --- | --- |
| Campaign | 16 MiB maximum; blocking slot lock; recheck exact revision under lock and immediately before replacement; distinguish not published, published/uncertain and durable |
| Library | 1 MiB maximum; nonblocking lock returns Busy; compare exact bytes including ignored data; preserve `.bak`; refuse unowned `.tmp`/`.bak.tmp`; retain committed state after late failure |
| Scenario | 1 MiB maximum; blocking folder lock; retain listing capacity and stale-selection checks; new filenames never overwrite an intervening file |
| Settings/music | Preserve formats and warning behavior; support repeated replacement on Windows using checked temporary publication, without adding campaign-style backup/recovery policy |

### Native operations and failure states

Keep POSIX behavior through the same `open`/`fstat`/`flock`/`write`/`fsync`/
`rename`/no-clobber-link operations, including interrupted-I/O handling and
non-following regular-file checks where currently required.

Windows uses non-inheritable `CreateFileW` handles, `GetFileType` and handle metadata
to reject directories/devices/reparse points for checked store files. Lock files
must be regular files too. Use `LockFileEx` on a fixed byte range; its immediate-fail
mode maps to Library Busy and its blocking mode preserves campaign/scenario policy.
Do not treat unrelated access-denied errors as lock contention. Use `CREATE_NEW`
for exclusive temporary files and checked bounded `ReadFile`/`WriteFile` loops.

Flush temporary contents with `FlushFileBuffers` and check close before publication.
Use same-directory/same-volume `MoveFileExW` with `MOVEFILE_WRITE_THROUGH`; add
`MOVEFILE_REPLACE_EXISTING` only for an explicit replacement. Do not enable
copy-and-delete across volumes or delete the live destination first. Scenario
Save As uses a move without replacement. Keep library backups as separately
written, flushed files containing the exact previous bytes (or initial valid
document), published before the live library.

Set the published/committed flag immediately when publication succeeds. The final
confirmation reopens and flushes the published file with checked native operations;
failure returns the existing published-but-uncertain outcome and never pretends the
old file is still active. Preserve current fault stages, mapping DirectoryFlush to
the final native confirmation on Windows and to directory `fsync` on POSIX. Keep
the library's pre-replacement backup barrier separate from final confirmation.

On Windows, `durable` means the checked file flush and native write-through move/
confirmation operations succeeded. Do not claim a portable directory-`fsync` or
power-loss guarantee from this, or report unsupported directory flushing as a
successful call. Test on the hosted local NTFS volume; other filesystem durability
is not established by that result. `ReplaceFileW`'s WRITE_THROUGH flag is unsupported
and is not the selected primitive. See Microsoft's [move contract](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw),
[file flush contract](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-flushfilebuffers)
and [ReplaceFileW limitations](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-replacefilew).

Use `BCryptGenRandom` with the system-preferred RNG for Windows identity entropy;
retain the POSIX source. Preserve library UUID-v4 bits, ID text and the eight-attempt
collision bound. Do not use clocks, process IDs or deterministic PRNGs as an entropy
fallback. Retain existing seams for deterministic failure/collision tests. The
existing campaign identity format stays unchanged; no save migration is introduced.

### Native defaults and Unicode paths

On Windows, use the absolute wide `%LOCALAPPDATA%` environment path, falling back
to `SHGetKnownFolderPath(FOLDERID_LocalAppData)` when missing/invalid. Free its returned
memory correctly. The game-owned child is `blobber/`; campaign, character library,
scenarios, settings and playlist state retain their existing filenames beneath it.
An explicit path override retains precedence. Fail/report missing writable defaults
through the relevant existing startup/warning path; never fall back into the install.
Preserve Linux HOME/XDG resolution and existing override semantics.

Use `std::filesystem::path` internally, wide Windows API calls, and explicit
`u8path`/`u8string` at UTF-8 string interfaces. Append `.lock`/`.tmp`/`.bak` to paths
without a lossy `.string()` roundtrip. Persist scenario filenames as UTF-8 in the
existing bounded arrays and check the encoded length before copying. Do not change
display-text rules or expand persistence capacities as a path fix.

Give relevant Windows executables an embedded UTF-8 process manifest and UTF-8
compiler source settings; compilation `/utf-8` alone is insufficient. Convert wide
arguments once at startup and keep main focused on orchestration. Use a native wide
executable-path helper instead of relying on raylib's bounded application-directory
string. Native paths remain paths for C++ stream I/O; strings handed to YAML/raylib/
audio APIs must be UTF-8 under the manifest. Where a YAML filename API would still
lose encoding, parse a bounded native-path stream instead. Test actual decode of
generated image/font/audio fixtures, including the MP3 streaming filename path;
metadata resolution alone cannot establish Unicode playback loading.
See [Microsoft's UTF-8 process manifest](https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page).

## Build, prepared publication and package

### RNG state portability correction discovered during implementation

The existing save/restore contract requires 624 MT19937 state words plus an index.
MSVC 17.14's standard-library stream writes 624 normalized history words without
the index, so its `std::mt19937` stream cannot implement that existing contract.
Use a small owned MT19937 value with 624 uint32 words and a bounded index in the two
existing RNG owners. Keep the standard MT recurrence, seed and output sequence and
the exact existing Linux text representation; no save/content version changes.
It uses fixed storage, no allocation or indirection, and replaces rather than
duplicates the library engine. Verify outputs against `std::mt19937` across twist
boundaries and compare serialized state directly to the accepted Linux implementation;
then run cross-platform interchange and affected dice/AI/persistence checks.
The concrete dependency difference is visible in Microsoft's
[17.14 STL source](https://github.com/microsoft/STL/blob/vs-2022-17.14/stl/inc/random).

### Native Windows build policy

Use `windows-2022`, VS 2022/v143, the MSVC 14.44 compiler family and Windows SDK
10.0.22621.0 in a dedicated single-config Ninja Release tree,
`build-windows-release/`. Initialize the x64 native environment explicitly; validate
the selected family/SDK and fail clearly if missing. Use CMake 3.31.x, Ninja 1.13.x
and Python 3.12.x, recording exact versions/paths, `cl /Bv`, linker/SDK identities,
runner image version and OS build. Do not silently substitute VS 2026 or another
SDK. Patch updates within the validated families are deliberate; a runner label
does not pin an immutable compiler image. Recheck availability before implementation
against the [runner inventory](https://github.com/actions/runner-images/blob/main/images/windows/Windows2022-Readme.md).

Select `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded` before dependency targets are
created, with CMP0091 active. Build raylib and yaml-cpp static with matching runtime
options; audit actual flags rather than trusting cache values. Use C++17, `/utf-8`,
ordinary precise floating point and generic x64 compilation without AVX/native CPU
requirements. Keep Linux presets and the accepted LD02 image/policy independent.

Disable accidental system dependency discovery. Reuse hash-verified raylib 6.2 and
nlohmann/json 3.12.0 inputs and yaml-cpp commit
`f7320141120f720aecc4c32be25586e7da9eb978`; use the existing source-archive hashes
in `packaging/linux/lock.json` as the shared pin source rather than floating package
resolution. Keep compiler-runtime flags consistent across game, tests and tools.
Windows OS/graphics libraries remain host-provided. No Linux library is copied.

Windows release policy/orchestration and PE-audit helpers sit alongside
the current distribution tooling. Their responsibilities are native environment
validation, a single build writer, source capture, ordered selected tests/staging/
audit/package and evidence promotion. Read CMAKE_COMMAND and CMAKE_MAKE_PROGRAM
from an existing build cache and retain both. Do not reconfigure CLion trees.
Build named required targets; do not invoke an unrelated all-tests/default build.

Audit every shipped PE for AMD64, direct/delay imports and declared runtime policy.
Reject debug/dynamic MSVC runtimes, unexpected third-party DLLs, misplaced helper
executables and undeclared imports. Maintain an explicit policy for permitted
Windows system DLLs/API sets; a name resolving from a developer PATH is not proof
of a host-provided OS dependency. Record relevant runtime-loaded modules separately
for GLFW graphics/audio. Require no VC redistributable installer for the chosen
static-runtime package. Inspect the actual PE and linkage before making that claim.

### Prepared-asset import and transfer

CMake input: `BLOBBER_PREPARED_INPUT`, default empty. Empty retains native
world/portrait/creator preparation. Nonempty imports a validated prepared publication
into marked build-owned storage; it must not mutate the input bundle or silently
fall back to baking if validation fails. Provide a cohesive preparation-only target
for the three outputs so the Linux producer need not build the graphical game,
stage a Linux package or run an entire test suite.

The prepared manifest records producer revision/source fingerprint, tool/dependency
identity, publication/appearance versions, content identities and a sorted per-file
size/SHA-256 ledger. Its input digest covers actual source-art inventories (including
deletions), transitive baker source/header dependencies, bake build rules, dependency
pins and format constants. Derive the list from the preparation dependency graph;
avoid another manually drifting artwork list. Use content hashes, not timestamps.
Native CPU-produced files must contain no machine-specific paths as content inputs.

Allow reuse when the selected consumer revision has exactly the same validated
preparation-input digest. Record both producer and consumer provenance; do not
rewrite the producer revision to imply a new bake. Different inputs, unknown
formats, missing files, links, hash/identity mismatch or mixed outputs fail closed.
The runtime loaders must actually open and fully validate the imported publication,
including payload checksums, mesh decoding, NPC/blueprint identity and QOI atlases.
Keep native miniature preparation tests on Windows; never run Linux executables there.

Replace the game's three production bake dependencies with validation/import
dependencies when importing. Keep the familiar target boundaries so callers need
not special-case the source of prepared files. No hidden dependency may still build
or run a production baker in import mode. Validate before staging and use a marked
candidate plus promotion/rollback rather than presenting partial imported output.
Prove the graph with the configured CMake build dry run after an import.

Hosted flow is strictly serial: `prepare` on `ubuntu-22.04`, then native Windows.
The Linux producer uses the accepted pinned dependency/preparation environment;
on an exact cache miss it performs one preparation, verifies it and saves the
publication plus small owned Linux compatibility fixtures. The initial hosted bake
is expected because accepted local build caches are not available on a fresh runner.
Do not run the full Linux distribution pipeline as a WD01 prerequisite.

Use a fixed relative cache directory and `enableCrossOsArchive: true` on save/restore.
The key is `wd01-prepared-v1-DIGEST` for the verified preparation-input digest;
no broad restore keys or partial-key acceptance. Require the Windows restore to
match the exact producer key, verify the ledger and consumer input identity, then
import. Cache eviction/miss during transfer fails the run clearly; do not unexpectedly
bake on Windows or use stale content. Cache only portable prepared data/fixtures,
not build trees, native executables or raw authoring assets.

Retain at most the current and previous WD01 cache entries; cap each compressed
prepared entry at 1 GiB and the owned set at 2 GiB. Check actual size/headroom and
fail before saving if the bound cannot be met; do not increase it silently. Prune
only exact workflow-owned WD01 keys through the authorized Linux `gh` manager after
runners finish, without sweeping other repository caches. GitHub cache retention
is separate from artifact retention; cache availability is not a correctness promise.
See [cross-OS caching and key behavior](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching).

The manager now runs on the Linux preparation job before cache restoration, after
previous runs have finished through workflow concurrency. That job alone requests
`actions: write` for cache deletion; its token is exposed only to the manager step.
It matches the exact WD01 digest-key namespace, reserves space for a cache miss and
preserves unrelated caches. Windows retains read-only repository permissions.

### Curated Windows ZIP

Extend the current curated runtime inventory/stage contract for native Windows x64
Release and `blobber.exe`; do not introduce a second content list. Add platform
selection only for actual platform differences, including notices, retaining all
applicable attribution. GCC-runtime notices need not describe an MSVC build.
Audit remaining notices without inventing a project license or claiming clearance.

Keep ELF and PE policy/checkers distinct. Isolate Linux-only Python imports/locking
from shared archive hash/inventory/promotion helpers. A small Windows ZIP path can
reuse those helpers without turning the Linux driver into a generic build framework.

Name: `blobber-VERSION-gREVISION[-dirty]-windows-x86_64.zip`, with CMake project
version and twelve Git revision characters. Require clean source by default;
explicitly allowed local dirty builds retain the marker and source ledger. Hosted
checkout builds are clean and bound to the dispatched immutable SHA. Do not relabel
accepted old artifacts when commits advance.

One matching top-level directory contains `blobber.exe`, curated runtime files,
applicable notices, `README.txt`, `BUILD.json` and `MANIFEST.json`. The manifest lists
every other regular file's relative path/size/SHA-256 and explicitly excludes itself.
BUILD records source/dirty state, actual tool/policy identities, producer/consumer
prepared provenance, content identities and PE-audit linkage without host paths.
Adjacent final outputs include the ZIP, `.sha256`, build and verification metadata.
Keep them in the Windows build's marked `dist/NAME/` bundle and download the selected
final bundle intentionally into `build-windows-review/dist/NAME/` on Linux.

Use sorted streaming ZIP writes with fixed buffers, single-threaded deflate level 6
and normalized commit-derived timestamps within ZIP's date range. Preserve LD03's
owned candidate/promotion/rollback behavior and stale-audit rejection. Reject links,
reparse points, special files, traversal, duplicate/case-colliding paths, drive/ADS
syntax and Windows-reserved names. Validate ZIP CRCs, manifest/hash completeness
and extraction before declaring success. Test malformed members with tiny fixtures.

Tests, bakers, diagnostic/review tools, debug symbols and test graphics DLLs stay
outside the payload. Never ship original world artwork beside its packed publication.
No installer, signing or public Release target is added in this slice.

## Manual workflow and bootstrap

Workflow filename: `.github/workflows/windows-review.yml`. Its sole trigger
is `workflow_dispatch`. Dispatch selects the source branch/ref using the standard
Run workflow selector or this command:

```sh
gh workflow run windows-review.yml --ref distribution/wd01
```

Each job checks out the immutable `github.sha` chosen by dispatch, validates HEAD
and records it. Disable Git checkout newline conversion before checkout so hashed
JSON/YAML/source inputs retain repository bytes. Pin checkout/cache/upload actions
to immutable action commits; use least-privilege GitHub-provided tokens and disable
persisted checkout credentials. Do not embed OAuth tokens in YAML, inputs or logs.
Give runs a source-SHA name and record workflow revision separately from content.

The workflow uses only the serial preparation and Windows jobs, not a release
matrix. Windows builds selected targets, runs tests, stages/audits/packages and
checks the extracted ZIP. Set workflow-wide concurrency with no overlapping
substantial jobs and no automatic cancellation of a healthy in-flight run. Use two
compile workers maximum, one test process, bounded library/Mesa threads, a 90-minute
prepare timeout and a 60-minute Windows timeout. Check memory/disk first; use Linux
`nice -n 15`/`ionice -c 3` and below-normal Windows process priority for lengthy work.

Upload only the final ZIP/checksum and compact build/test/compatibility/import/smoke
evidence with seven-day artifact retention. Store the already-compressed ZIP without
another expensive compression pass. No duplicate uncompressed player stage,
production publication artifact, build tree or large raw log archive is uploaded.
Keep failure evidence compact and marked failed; never label an older package as
the result of a failed attempt. Infrastructure-unavailable smoke is recorded separately.

### Authorized Git/GitHub sequence

1. Recheck account/access, remote/default branch and current working tree without
   reading credential values. `gh auth status` already confirmed `workflow` scope;
   do not ask the user to refresh it again unless an actual new access failure arises.
2. Create `distribution/wd01` from the local prerequisite/planning commits, preserving
   any newer user edits. Keep WD01 source/workflow changes in subsequent commits;
   review exact staged paths rather than absorbing unrelated new edits automatically.
3. Create a separate clean `distribution/wd01-bootstrap` worktree from remote `main`.
   Its PR adds only the dispatch workflow file, identical to the one on the full
   WD01 branch. It may reference scripts present on the selected WD01 source branch;
   it does not require those unreviewed code changes on `main` to register dispatch.
4. Check the narrow PR diff, open it with `gh`, and merge it with `gh` as the user
   authorized. Do not merge the full Windows implementation to register the trigger,
   bypass repository rules or force-push. Keep SSH if it works; an authentication
   fallback must use the `gh` credential helper without printing/storing a token.
5. Push the full WD01 branch and dispatch it by `--ref distribution/wd01`. Capture
   the source SHA before dispatch. The workflow file must exist both on default
   `main` for registration and on the dispatched branch; `--ref` alone does not
   register a new workflow. No event-triggered build runs when the bootstrap merges.
6. Resolve the exact run by workflow, branch/source SHA, actor and dispatch time;
   use `gh run watch`, `gh run view --log-failed` and `gh run download RUN_ID`
   with its exact artifact name. Never download an ambiguous latest successful run.
   Check downloaded checksum, manifest, source/prepared identity and compact evidence.
7. Import the Windows compatibility fixtures into the same revision's Linux verifier,
   promote only the selected review bundle/evidence, close runners and clean owned
   temporary downloads/worktrees. Do not delete another active branch/worktree/cache.

The default branch must contain a dispatch workflow for registration; see
[manual dispatch prerequisites](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow).
The original planning turn performed no remote mutation. The authorized bootstrap
merge and implementation dispatches are recorded below. That authorization does
not extend to public publication, paid services or unrelated settings.

## Verification and handoff contract

### Focused correctness and format checks

Use small generated owned fixtures; do not depend on production art, mutable saves
or full maps for routine regression tests. Add a shared portable temporary-directory
fixture and process/lock test helper for affected tests. Replace GCC-only bit scans
and incorrect MSVC aligned-allocation pairs in selected callers without weakening
existing assertions. Gate genuinely Linux-only harnesses by platform; do not skip
the corresponding required persistence/path/allocation behavior on Windows.

- Native file fixtures cover missing/oversized/nonregular files, short writes,
  lock contention in a separate process, stale writes, no-clobber creation,
  backup bytes, injected write/flush/close/publication failures, late uncertainty,
  owned-temp cleanup and released handles/locks. Exercise actual native operations
  as well as fault seams; test reparse/symlink rejection where available and report
  unavailable privileged fixture creation honestly.
- Path/content fixtures cover defaults and overrides, missing/relative environment
  values, spaces and non-ASCII installation/user roots, bounded scenario filenames,
  native executable discovery and repeated settings/playlist replacement. Resolve
  and decode generated audio/image/font inputs through the real affected loaders.
- Generate campaign/library/scenario interchange fixtures with fixed IDs/seeds and
  small owned content on Linux; import and re-encode on Windows. Export Windows
  fixtures as compact evidence, then import them with the same revision's Linux
  verifier after download. Assert semantic snapshots, unchanged formats/content
  identities and rejection of invalid/incompatible documents, not accidental
  cross-compiler floating-point byte identity. Do not use a user save as a fixture.
- Prepared-input tests cover tiny native bake/export/import, full loader validation,
  missing/mixed/corrupt/stale input, producer/consumer provenance, changed/deleted
  source files and failed promotion. Test import mode without scheduling production
  bakes. ZIP/PE fixtures cover unsafe members, name collisions, hashes, stale audit,
  wrong architecture, undeclared dependencies and interrupted output promotion.

Run relevant existing `library`, `arena`, `campaign_save_smoke`, options/
packaged-options, runtime-inventory/staging, dialogue/audio/music, menu/startup and
distribution-smoke suites for the changed callers, plus affected allocation guards.
Run extended campaign persistence once at the meaningful integration checkpoint
because the disk adapter changed. During iteration run the smaller smoke selection;
do not repeat a full Debug/Release/sanitizer/graphics matrix or benchmark.

### Extracted ZIP and graphics smoke

Validate/extract the exact produced ZIP once into an owned location containing spaces
and non-ASCII characters. Use default asset discovery with no `--assets` override,
an unrelated CWD, isolated absolute LOCALAPPDATA/temp roots and a read-only install
enforced by temporary ACLs. Adapt LD03's marked user-data checks/reporting to native
paths; preserve its opt-in flags, hidden window, action/deadline bounds and actual
menu/New Campaign/dialogue/inn/dungeon/save/fresh-process Continue sequence.

Provision verification-only x64 Mesa llvmpipe from a version/hash-locked upstream
archive. Selected verification input is `mesa-dist-win` 26.2.6 MinGW x64 release, SHA-256
`da0f71d9181b672c1918f3c342f11118c687f6059a46b16502845160733cc388`.
The originally planned MSVC provider archive omits WGL in this release because of
an upstream regression; the correction is recorded in the handoff below. This is
an external verification-only OpenGL C-ABI provider, independent of the game
compiler/static-runtime policy.
Verify the downloaded bytes; never use a floating latest URL. Extract only the
needed x64 runtime closure into the owned runner directory, not the player package
or system directories. Select `GALLIUM_DRIVER=llvmpipe` and cap `LP_NUM_THREADS=2`.
Use a separate launcher with `SetDllDirectoryW` before creating the child, which
Windows documents as affecting child DLL lookup; record actual loaded GL/DLL paths
and GL vendor/renderer/version. A PATH edit alone cannot override normal system DLL
search order. See [provider](https://github.com/pal1000/mesa-dist-win/releases/tag/26.2.6)
and [launcher behavior](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setdlldirectoryw).

Copy the minimal verifier/launcher inputs into the owned run before isolation.
Temporarily deny read access to source assets and original build asset roots;
verify access denial using the same child identity. Save/restore original ACLs in
`finally`, including on timeout/failure; do not rename the user's checkout. Leave
Windows/runtime-provider directories readable and isolated user data writable.
Check all package file hashes and inventory again after smoke; the test provider
must not add files to the extracted player installation.

Preserve audio bank/music/dialogue resource checks with device audio disabled;
listening and real audio devices remain human review. Run the hidden depth-copy/
GPU lifecycle check for the affected Windows integration where supported. Do not
inject OS input, select windows, change focus or automate the user's desktop.

An unavailable suitable GL context is **unverified**, with the failed probe and
reason retained compactly. With a working context, content/render/save/controller
failure fails the run. Compilation or `--help` is never playable-package evidence.
Software OpenGL on hosted Windows Server is not Windows 11 physical-GPU evidence.

### Review, resource ownership and completion

Report actual workflow/run/job links and source SHA; tool/image/dependency identities;
focused results and unavailable checks; ZIP/checksum/inventory/import sizes and
identities; prepared cache hit/miss/provenance; and extracted-smoke results/limits.
Include a short Windows 11 recipe: verify SHA-256, extract the entire ZIP, launch
from another CWD without asset overrides, review graphics/text/audio, start New
Campaign, travel, Save and Main Menu, exit and restart Continue. Identify the native
LocalAppData save/settings location so the user can protect existing data.

Keep one substantial job at a time, at most two build workers and one test process.
Check memory/disk before costly work; no production rebake or benchmark solely to
increase a passing count. Use `tools/agent_artifacts.py create` for disposable
outputs. Its current cleanup uses a POSIX symlink-safe `rmtree` check; adapt Windows
owned-run cleanup deliberately, preserving tracked/ownership/reparse-point guards
and closing writers first, rather than silently bypassing a failing cleanup guard.

Retain only the selected final bundle and at most six useful screenshots with a
README preserving pending acceptance. Record commands/results/limits here rather
than creating another permanent per-run report. Close runners, restore ACLs, preview
and apply cleanup of owned extracts/logs/downloads even after failures. Preserve
build caches, existing evidence, unrelated active runs and user saves.

Implementation stops at **Ready for review**. Only explicit user acceptance
closes WD01 and makes LD04 eligible. No combined Linux/Windows release automation,
public publication or automatic advancement is part of this documentation handoff.

## Planning verification and current handoff

The planning work read AGENTS.md, system_design.md, the roadmap, accepted LD03
handoff and relevant persistence/path/content/build/packaging/smoke/test code. The
read-only GitHub check confirmed `biscon/blobber`, public `main`, admin access,
Actions enabled, zero registered workflows and OAuth `repo` plus `workflow` scopes.
No credential values were read; no authentication refresh or remote mutation ran.

Before planning edits, `git diff --check` passed and the 36 existing changed/
untracked files were byte-hashed into the owned run, then committed as `08217e6`.
Documentation verification checks local links, heading/checkpoint consistency,
manual-only trigger policy and unchanged prerequisite source bytes. No gameplay
build/test, bake, Windows execution, archive or hosted workflow is claimed by this
planning step. That planning deliverable was the proposed design for review; the implementation
handoff below records subsequent work.

Executed planning checks, 2026-11-12: `git diff --check` passed; a focused Python
check resolved **283 local Markdown links** across the three planning/overview
documents, confirmed exactly LD01/LD02/LD03/WD01/LD04 in slice order, checked active
WD01/review status, current scope notes and the manual-only trigger policy, and
confirmed there is no `.github/workflows` directory. The prerequisite byte ledger
matches all 36 files in `08217e6`; 34 remain byte-identical in the working tree and
only the two intended existing planning/overview documents changed. The third
changed document is this newly authored design. Staged whitespace and final Git
scope are checked before the documentation commit.

Owned planning run
`output/agent-runs/20261111T225731Z-512e707955934866896ddfbd1f03f4a5/` was previewed
and cleaned with `tools/agent_artifacts.py clean RUN_DIRECTORY` and `--apply`:
three disposable files / 4,303 bytes removed. No runner, capture, retained diagnostic
exception or temporary extract remains. Existing LD03 captures/build caches and
user data were preserved. No public/remote operation occurred in the planning step;
the user then reviewed the design and authorized implementation.

## Implementation progress — 2026-11-12

The user reviewed the design and instructed continuation. Work is on
`distribution/wd01`; prerequisite commits `08217e6` and `e48505d` remain unchanged.
Core port/package/workflow work is in `9abc9fa`, provider evidence in `fcb81b5`, and
the portable RNG/verification correction in `24df866` (later handoff commits may follow).

Native Linux focused checks passed before hosting: library/arena/campaign-save
smoke (3 suites, 1.03 s), 16 selected content/options/menu/package/controller suites
(1.23 s), and extended campaign persistence (2.96 s). The RNG correction then passed
10 selected rules/AI/allocation/persistence/I/O suites in 7.54 s. Its golden-state
check compares the owned engine's outputs and text to the previous Linux library
representation across twist boundaries; the new code also imports the campaign/
library/scenario fixtures exported by the pre-correction build. No production bake
or benchmark was run locally for those checks. Actionlint 1.7.12 passes.

The narrow [bootstrap PR #4](https://github.com/biscon/blobber/pull/4) added only the
manual workflow and was merged as authorized, registering it on `main`. The full
Windows code remains on the review branch. No push/PR/merge/tag/Release event builds
are enabled, and no public Release/tag was published.

First [hosted run](https://github.com/biscon/blobber/actions/runs/43119803791),
source `fcb81b50dbf318662f02d7077fc0f1eaeb9b03fc`, was deliberately canceled during
Linux preparation after the portability audit exposed the incompatible native
MSVC RNG stream contract. It did not execute Windows or produce an accepted package.
Its ephemeral runner was closed; the recorded cleanup ran during cancellation before
the platform's final orphan termination. Cancellation cleanup now skips filesystem deletion while writers may still be
active: GitHub terminates the canceled ephemeral runner and disposes its storage.
Ordinary completed/failed commands are awaited, then the marked run is previewed
and cleaned by the helper. This avoids deleting logs while an orphan still writes.

Second [hosted run](https://github.com/biscon/blobber/actions/runs/43122551224),
source `24df8662887e49fdf022abe6bac65621e9949e43`, compiled the native preparation
tools, baked the publication and passed full typed validation. Export then failed
because the intentionally minimal accepted compiler image contains no Git. The
fix in `9931f7e` keeps that image unchanged and performs source provenance/export
with host Git/Python; native baking/validation remain in the pinned container.
Windows was skipped and no package was produced by this failed run.

Third [hosted run](https://github.com/biscon/blobber/actions/runs/43124549100),
source `9931f7ea17d143c67dc61eccfa9641865f2d5a46`, passed preparation and cached the
validated transfer as `wd01-prepared-v1-8f86f441597591f5fb79c74b77f2490399a9eb1edbd922440b798b33ea05dc7d`
(218,297,446 compressed cache bytes; deflate estimate 228,595,513). Its cold compiler
image build took about 22 minutes; native baking itself took 53 seconds. Windows
selected the required tool families and compiled 147/343 build steps before the
missing Windows declarations in the BCrypt header stopped compilation. The native
header ordering has been corrected. No Windows tests/package were produced by this
failed attempt; the next corrected hosted run remains pending.


### Additional verification corrections

Source capture now includes tracked and untracked nonignored files and records
tracked deletions. The final source check therefore catches byte changes to an
untracked input even when Git status text is unchanged. The prepared envelope
uses the same file listing, filtered to its preparation inputs; a clean checkout
retains its previous digest. Six focused Python packaging/cache/import tests pass,
including added/deleted/modified input rejection and invalid-import preservation.

Portable temporary fixtures now use spaces and non-ASCII characters. The owned
`path_decode` test exports a tiny WAV and decodes WAV/MP3 through real raylib filename
APIs. `path_stream_decode` opens those filenames through `LoadMusicStream` without
playing sound; CTest marks it skipped (77) when no audio device is available.
The dedicated 522-byte MP3 fixture was generated once with:

```sh
ffmpeg -f lavfi -i sine=frequency=440:sample_rate=22050:duration=0.06 \
  -ac 1 -c:a libmp3lame -b:a 32k -map_metadata -1 -write_xing 0 \
  -id3v2_version 0 -threads 1 tests/fixtures/path-tone.mp3
```

The configured CLion CMake/Ninja pair rebuilt the affected graphics/application
libraries and selected fixtures with two workers at low CPU/I/O priority. A prior
dry run confirmed no production bakers were scheduled. Twelve selected suites
passed in 0.97 s: `path_decode|path_stream_decode|creator_art|portraits|audio|music|audio_runtime|dialogue|library|arena|platform_io|platform_io_process`
(anchored CTest expression, one test process). Both audio decode checks ran locally;
streaming was not skipped and no sound was played. Native Windows execution of
these corrections remains pending. No Linux production bake or desktop control ran.
The same path audit corrected the arena folder display to UTF-8 and kept native
paths until audio API boundaries. Audio/music/dialogue paths reject Windows drive/
root-relative forms; a missing audio override cannot restore an unsafe base path.
The drivers now check at least 3 GiB available RAM plus their scratch-space bound
before costly work and record the Windows resource preflight in build metadata.

### Windows 11 play review

1. Back up existing `%LOCALAPPDATA%\blobber` before launching this review build.
2. Compare `Get-FileHash -Algorithm SHA256 .\PACKAGE.zip` with the adjacent
   `.zip.sha256` text, then extract the entire ZIP. Keep `blobber.exe`, `assets/`
   and `notices/` together. No build tools or VC redistributable installer are
   required by the static-runtime package; a working OpenGL 3.3 driver is required.
3. Put the extracted directory in a path with spaces/non-ASCII characters. From a
   different working directory, launch the executable by its full path without
   asset overrides. Review menu text, scaling, graphics, sound/music and dialogue.
4. Start New Campaign, travel through town/inn/dungeon, Save and Main Menu, exit,
   then restart and Continue. Check that the restored location/party are correct.
   User state belongs beneath `%LOCALAPPDATA%\blobber`, outside the install.
5. Report physical-GPU/audio issues and acceptance separately. Hosted Windows
   Server/software OpenGL evidence does not replace this Windows 11 review.


Corrected verification attempt: [manual run 43129185992](https://github.com/biscon/blobber/actions/runs/43129185992),
source `624ff9d7900c85cb226bfc7a9d424793123652bf`. Preparation passed with at least
15,290,556,416 available RAM bytes and 89,553,797,120 free scratch bytes. The exact
prepared cache key is `wd01-prepared-v1-45c112d366752c2be9f04f31437f3965e8ae5beab7bd325e01243a346a8d428d`
(218,297,352 compressed bytes). Windows compiled and linked the native OS library,
then stopped at 182/345 build steps: MSVC rejects an aggregate reset containing
the owned RNG when its only constructor is explicit with a default argument.
Separate a nonexplicit default constructor and an explicit seeded constructor,
matching the standard engine interface; retain the exact seed/state/output behavior.
The configured headless CMake compiled the affected campaign codec object and RNG
test, and the golden-state/sequence suite passed again. The Windows suites/package
remain unexecuted; another corrected manual attempt is required.
The focused local command was:

```sh
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest \
  --test-dir cmake-build-release \
  -R '^(path_decode|path_stream_decode|creator_art|portraits|audio|music|audio_runtime|dialogue|library|arena|platform_io|platform_io_process)$' \
  --parallel 1 --output-on-failure --no-tests=error
python3 tests/windows_distribution_tests.py
```

The configured CMake and Ninja paths were read from `cmake-build-release/CMakeCache.txt`;
configuration and builds used that CLion pair. No existing build cache was switched
to another toolchain. Local font path coverage remains in the packaged smoke rather
than a newly copied TTF fixture; physical text rendering/audio listening remain user
review, independently of these filename/codec tests.


The smoke classifier now uses the application's fatal window-construction message
rather than any earlier WGL warning. A content/shader failure with a working context
must fail instead of becoming an unavailable-context result. Seven local Python
packaging/cache/import/classification tests pass (0.04 s); this helper correction
changes no player payload or prepared input.


Fifth [manual run](https://github.com/biscon/blobber/actions/runs/43131715951),
source `9c61adb5b60d36ebea050094a23c28683de483da`, is in progress with the separate
RNG constructors and strict smoke failure classification. Native Windows results,
package/interchange review and cleanup remain pending.

A focused Python correction also rejects the superscript COM/LPT device names
listed by [Microsoft's Windows naming rules](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file).
The existing path-rejection test covers these names (seven tooling tests pass in
0.04 s). It changes no player files or preparation identity; the downloaded archive
will be checked with this stricter validator too.


### Manual code-check mode — implementation adjustment

Repeated native compiler stops exposed an avoidable verification cost: each
conservative source-key change restarts a 9–22 minute cold compiler-image build
before Windows can diagnose another C++ error. Add an optional `code_check`
workflow-dispatch boolean (default false) and matching `--code-check` driver flag.
The serial Linux job still captures the selected source/key but skips prepared
cache mutation and baking in this mode. Windows compiles/links the application,
bakers and selected tests, runs the owned CPU/miniature fixtures and an entry-point
`--help` check. It publishes no review ZIP and claims no prepared/gameplay smoke.

Use a small `blobber_code_check` executable target sharing the real `main.cpp`,
application libraries, native entry point, UTF-8 manifest and static-runtime policy;
it has no runtime asset dependencies. The full `blobber` target keeps its validated
import dependencies. The code-check driver selects the former and a dummy import
path whose target is never run. Both modes retain two workers/one test and the same
native tool/dependency/source checks. Keep independent compilation going after
errors (`Ninja -k 0`) and print bounded compiler diagnostics so one failure does not
hide other port errors. The normal manual build remains the complete preparation,
validation/staging/ZIP/smoke path, required before Ready for review.

Fifth hosted attempt compiled through the earlier RNG failure, then stopped at the
portable fixture header's missing explicit `<string>` include. Fix that dependency;
use code-check mode to collect remaining native issues before the next full bake.
This adjustment changes verification orchestration, not game behavior or acceptance.


Code-check run [43136582454](https://github.com/biscon/blobber/actions/runs/43136582454),
source `b0ca294bcd49a7e477a878708e8b54d79579570f`, proved the preparation/cache
steps were skipped and all application/test/tool targets compiled and linked.
Twenty-four of 27 selected suites passed, audio-device streaming was skipped, and
two suites failed in 15.97 s. The old-save fixture retained an `ifstream` across
replacement; Windows correctly refuses a move when an existing reader has not
shared delete access. Close that fixture reader before mutation. Inventory paths
mixed a short `RUNNER~1` temporary root with the dialogue loader's canonical long
path; lexical subtraction then escaped the root. After rejecting each original
ancestor's links/reparse points, normalize checked roots with `weakly_canonical`
before comparisons/subtraction and use explicit UTF-8 path conversions. Preserve
escape/link rejection and the original campaign replacement rules.


Configured headless CMake/Ninja rebuilt the affected targets with two low-priority
workers. `ctest --test-dir cmake-build-headless-release -R '^(campaign_save|runtime_inventory|dist_staging)$' --parallel 1 --output-on-failure --no-tests=error`
passed all three suites in 2.87 s (full campaign 2.82 s). No production bake,
graphics window or unrelated matrix ran. Native code-check retry remains required.


Code-check retry [43138720185](https://github.com/biscon/blobber/actions/runs/43138720185)
resolved the just-pushed branch to the previous `b0ca294` revision. Its SHA mismatch
was caught and the obsolete attempt canceled during Windows checkout; the native
driver and upload were skipped. The ephemeral runner/storage were disposed by
GitHub. Subsequent dispatches verify the remote branch SHA before dispatch and the
actual run SHA immediately afterward, rather than assuming push visibility.

Correct retry [43139052241](https://github.com/biscon/blobber/actions/runs/43139052241),
source `302b5d0e3ddf86682a40bf0580cfe1473bba1bf5`, passed: 26 suites passed and
`path_stream_decode` was skipped because no audio device was available (27 selected,
15.02 s). Native compilation/linking, AMD64/static-runtime PE audit and the real
entry-point `--help` check passed. The code-check executable imports only bcrypt,
gdi32, kernel32, ole32, shell32, user32 and winmm; this is not yet an audit of a
shipped ZIP. No review artifact or prepared/graphics smoke is claimed by this mode.

Observed native environment: CMake 3.31.6, Ninja 1.13.2, Python 3.12.10, MSVC tools
14.44.35207 / compiler 19.44.35229, SDK 10.0.22621.0; runner image 20261101.353.1,
Windows Server version 10.0.20348.5773. The driver binds CTest to the configured
CMake installation, passes its validated Python to CMake and records exact tool
paths, memory and scratch availability. Normal full builds use the same policy.

The clean owned bootstrap worktree was removed after verifying its exact commit and
clean status; branch/history remain. Remote main contained only the authorized
workflow bootstrap. It was merged into the review branch as `36cc467`, resolving
only the workflow add/add conflict with the reviewed branch version. `git diff
--exit-code 302b5d0 HEAD` proved the complete file tree unchanged; no implementation
was merged to main by this reconciliation.

Full manual run [43140724828](https://github.com/biscon/blobber/actions/runs/43140724828)
was dispatched and immediately verified at source
`36cc467c3009255414b19152c0ff0352810a1209`. Its complete preparation/import,
player ZIP, extracted smoke, reverse interchange and final cleanup remain pending.


Full run 43140724828 produced the Windows ZIP and passed native suites, complete
prepared validation, Linux-to-Windows interchange, staging and the exact game PE
audit. Its extracted smoke was **unverified at provider setup**, before game launch:
the hash-correct MSVC Mesa archive has no WGL/OpenGL driver. Inspection of that
archive confirmed the missing DLL; [upstream 26.2.6 notes](https://github.com/pal1000/mesa-dist-win/releases/tag/26.2.6)
document the MSVC regression and recommend MinGW x64 for the affected functionality.
Select that same version's hash-locked MinGW x64 provider, inspect its actual layout
and rerun extracted verification. No MinGW compiler/runtime enters the player ZIP;
the game remains native MSVC. Record the provider archive hash, driver/module path
and llvmpipe identity. A working-context render/content failure still fails the run.

The initial downloaded package is not the selected review evidence yet:
`blobber-0.1.0-g36cc467c3009-windows-x86_64.zip`, 564,914,993 bytes, SHA-256
`85d49b86736c5e9d82e3e638197f973ee88eb03b5d7cc5d509594c6858eb83a8`;
1,331,447,171 unpacked bytes / 392 files. Its code-check and complete game imports
are the seven allowed Windows DLLs. The download CLI's internal temporary archive
was automatically removed after transfer; subsequent transfers set TMPDIR to the
owned disk run explicitly. The superseded package/remote artifact will be removed
when final evidence is selected.


Local verification of the downloaded 36cc467 ZIP passed its SHA-256/checksum file,
complete manifest/CRC and source/prepared metadata comparison. Extracting only the
player executable into the owned run and performing a fresh PE audit reproduced
the recorded executable hash/imports. The configured headless CMake rebuilt
`blobber_distribution_compatibility`; importing the Windows-reencoded campaign,
library and scenario fixtures passed and wrote the Linux roundtrip into the owned
run. Thus both format-interchange directions passed before graphics verification.

The replacement Mesa archive is 55,815,794 bytes and its locally computed hash
matches the official release digest above. `7z l` and selected extraction/PE import
inspection confirmed the x64 WGL closure: opengl32.dll, libgallium_wgl.dll and
libdlclose-skip.dll. Extract only these three files with one decompression thread;
record their hashes in the smoke result. The provider consumes Windows system/UCRT
APIs and is kept entirely outside the player install. Seven focused tooling tests
and Python compilation pass. The prepared key remains
`wd01-prepared-v1-57a198d470b11b52fb61d7f05781e8257ce4e3bbc75c7fc5c19551c176307850`;
this verifier-only change can reuse the validated 36cc467 publication.

Only this task's superseded unverified artifact (13910881488, run 43140724828) is
eligible for remote pruning before replacement upload. Its verified downloaded ZIP
remains in the owned run until the replacement is selected. Preserve all other
artifacts/caches/history and the accepted Linux deliverable/evidence.


Corrected-provider run [43146739319](https://github.com/biscon/blobber/actions/runs/43146739319),
source `4506e5ff8f6db337e573f9c41f7b3b3896eaf8f1`, restored/validated the exact
prepared cache, passed native suites and full prepared/interchange/staging checks,
and reached the now-present WGL closure. ACL setup then failed before any game
launch or permission mutation: Windows PowerShell could not autoload
Microsoft.PowerShell.Security. No artifact was uploaded; the superseded remote
artifact had already been pruned after validating its local backup.

Use Windows PowerShell's .NET Framework Directory.GetAccessControl/SetAccessControl
APIs directly, avoiding cmdlet-module autoload. Save/restore only the DACL through
AccessControlSections.Access so owner/group/audit metadata remain untouched. Restore
from the saved DirectorySecurity descriptor without reading a deliberately denied
path. Add a snapshot/restore preflight on an owned Unicode directory before costly
compilation; the actual smoke still enforces/tests its read-only install and denied
source/build asset trees. This is a verifier-only correction; reuse the same
prepared key and keep graphics pending until the real extracted smoke runs.


The verifier binds the built-in Windows PowerShell executable under SystemRoot and
uses the .NET Framework [DACL setter](https://learn.microsoft.com/en-us/dotnet/api/system.io.directory.setaccesscontrol?view=netframework-4.8.1)
and [section-specific SDDL setter](https://learn.microsoft.com/en-us/dotnet/api/system.security.accesscontrol.objectsecurity.setsecuritydescriptorsddlform?view=netframework-4.8.1).
The preflight snapshots/restores/rereads the descriptor and requires equality.
Python compilation, seven focused tooling checks and `git diff --check` pass;
the next native full run must exercise these Windows APIs and real smoke.
