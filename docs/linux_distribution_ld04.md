# LD04 — Combined hosted builds and compatibility evidence

Created: 2026-11-12.
Status: **In progress — implementation, authorized by the user, 2026-11-12.**
The user instructed “implement the plan.” The design below is the approved scope;
actual implementation and verification are recorded in the handoff when complete.

Dependencies: accepted [LD03](linux_distribution_ld03.md) and
[WD01](linux_distribution_wd01.md). The [roadmap](linux_distribution_plan.md)
owns slice status. Preserve their accepted artifacts and evidence; this design
describes future changes, not additional successful builds or user acceptance.

## Outcome and scope

One manual dispatch builds the same immutable source revision into Linux x86-64
and Windows x86-64 player downloads, verifies both, and records a combined result.
Linux retains Ubuntu 22.04 / glibc 2.35, generic x86-64, OpenGL 3.3 and X11/XWayland;
Windows retains Windows 11 x64 / OpenGL 3.3, native hosted MSVC and GUI startup.
Software OpenGL evidence, host userspace evidence and human/device review remain
separate. Preserve gameplay, saves, prepared-image rendering and presentation.

Keep `workflow_dispatch` as the sole event. No commit, PR, merge, tag, Release or
schedule starts a build. Public publication, tag creation/push, signing, installers,
AppImage, new architectures, paid runners and license changes are outside LD04.
Document attaching verified assets to a separately authorized GitHub Release;
the build workflow itself has no publication action or `contents: write` permission.

## Inspected starting point

Planning inspected branch `distribution/wd01` at `3946e09`, with a clean tree.
The current code provides:

| Entry point | Existing responsibility / LD04 gap |
| --- | --- |
| `.github/workflows/windows-review.yml` | Registered manual workflow; serial Linux preparation then Windows, common concurrency group, pinned actions, raw ZIP/checksum and separate evidence. It does not build a Linux player package. |
| `tools/linux_release.py` | Pinned baseline image, ownership/lock, source/environment capture, focused tests, `dist` and `verify-package`. No prepared-import driver argument or hosted result contract yet. |
| `tools/distribution/archive.py` | Checked streaming tar, manifest, stale-audit checks and transactional promotion. Linux archive/root currently includes `-gREVISION`; LD04 must implement the approved plain player name. |
| `tools/distribution/verify_package.py` | Verifies one extracted Linux archive serially in Ubuntu 22.04 and Debian 13 runtime-only images, then atomically writes hash-bound results. Reuse this behavior. |
| `tools/distribution/hosted_prepare.py` | Separate WD01 producer/build tree. Avoid calling this alongside a native Linux distribution bake in the new full pipeline. |
| `tools/distribution/prepared.py`, `manage_cache.py` | Checked import/export, conservative content digest, cross-OS cache and owned storage pruning. The digest covers assets, source, CMake and compatibility fixture inputs; it is deliberately broader than artwork alone. |
| `tools/windows_release.py`, `windows_archive.py`, `windows_smoke.py` | Native selected build/tests, typed imported-data validation, PE/ZIP checks, isolated extracted smoke, Windows format fixtures and depth-copy check. The driver can return normally with an unavailable graphics status; combined success must explicitly reject that status. |
| `blobber_distribution_compatibility` | Tiny generated campaign/library/scenario export/import fixtures, already used in WD01; no production assets required. Automate the reverse import currently checked locally during handoff. |

Tracked assets have no detected LFS pointers; `.gitattributes`, `.gitmodules` and
gitlink entries are absent. A normal checkout currently supplies the artwork.
Recheck this before implementation; fail on missing inputs rather than adding
unnecessary LFS/submodule setup. Dependency downloads retain the accepted hashes
and snapshot recipes. An OS runner label alone does not pin its changing image.

## Workflow, ownership and interfaces

Keep `.github/workflows/windows-review.yml` as the single registered workflow;
change its display name to `Desktop distribution build` and run name to include
the source SHA. Reusing the filename permits dispatching the LD04 branch through
the existing default-branch registration. A new workflow file/bootstrap PR is
unnecessary. Historical WD01 runs remain historical Windows-only evidence.

The full path has explicit serial dependencies:

```text
linux (ubuntu-22.04) -> windows (windows-2022) -> verify (ubuntu-22.04)
```

Use workflow-wide `blobber-distribution-manual`, `cancel-in-progress: false`, shared
with older WD01 refs. Dependencies serialize work inside a run; concurrency prevents
overlap with another dispatch. Do not promise a FIFO queue: default concurrency
can replace an already pending run. Dispatch one intended review run at a time.
Remove the workflow's `code_check` input from the combined build UI; retain the
native driver's existing `--code-check` development command. A combined dispatch
always means both complete packages, not a configurable platform/test subset.

All jobs disable newline conversion before checkout, check out `github.sha`,
disable persisted credentials and assert HEAD/source cleanliness. Record source
SHA separately from workflow ref/SHA, run ID/attempt and actual runner image.
Use pinned action commits, including download actions added for small transfers;
check their supported runtime and raw-file behavior when implementing. Pass paths
and values through structured arguments/environment variables, not interpolated
untrusted shell code. Top-level permissions remain `contents: read`; add
`actions: read` only where downloading/querying run artifacts requires it.
Only the Linux cache-management job needs `actions: write`, with its token scoped
to the exact cache-management step. No personal tokens enter files or logs.

Keep platform commands cohesive. Add a small Linux orchestration helper under
`tools/distribution/` for hosted preparation selection, result paths and compact
evidence; add a separate compact result/transport validator if needed. Reuse
platform drivers, shared hash/promotion helpers and current CMake targets. Do not
replace them with a general CI framework or put a second packaging implementation
in YAML. Packaging allocates outside gameplay and adds no frame-path work.

### 1. Linux package and shared preparation

1. Create marked agent runs, check disk/memory, resolve the prepared key and prune
   only the exact owned cache namespace. Restore at the existing relative path
   `output/wd01-prepared`, with cross-OS archives and no restore-key fallback.
2. On an exact valid hit, run the baseline distribution with that bundle imported.
   Add an optional **proposed** `--prepared-input PATH` to the Linux `dist` driver;
   validate on the host, mount it read-only into the pinned container, configure
   `BLOBBER_PREPARED_INPUT`, run the typed validator and preserve producer provenance.
   Extend build ownership/environment identity to distinguish native/import mode
   and the import digest. A mismatch requires a fresh owned build directory, not
   destructive reconfiguration. Verify the imported game graph schedules no
   production baker. Invalid data is a failure, not an implicit fallback bake.
3. On a miss, use the same baseline distribution build's native preparation once.
   Build `blobber_validate_prepared` and `blobber_distribution_compatibility` in
   that tree alongside the focused targets. Validate/export its prepared outputs
   through `prepared.py`; record actual image/tools/content identities and save
   the checked transfer. Do not run the old standalone WD01 producer as well.
4. Retain the existing `dist` stage/ELF audit/archive sequence. Invoke
   `verify-package` with a fresh marked run and the exact emitted bundle directory;
   use Ubuntu 22.04 then Debian 13 against this one extracted archive. No userspace
   rebuild, host display socket or OS input. Require both create/save and
   fresh-process Continue phases to pass. Keep read-only install/default discovery,
   source/build exclusion, isolated HOME/XDG/temp and post-run integrity checks.
5. Export current-source Linux compatibility fixtures and the already-built Linux
   compatibility verifier as a small internal transport, with a file/hash ledger.
   Regenerate these fixtures each run, even on a prepared cache hit. Cached
   producer fixtures are not current-run evidence. Upload the verified Linux
   player/checksum separately from compact evidence and internal transport.

Keep the current Linux command semantics when no import is selected. Use dedicated
`build-linux-baseline-hosted-*` directories compatible with the driver's ownership
rules, not CLion trees. Emit machine-readable paths for the selected bundle and
reports; do not glob every historical `dist/blobber-*` directory or parse a vague
latest-file timestamp. Cache hits must preserve the original producer revision;
both packages record their actual consumer revision and matching publication hash.

### 2. Native Windows package

After Linux succeeds, restore its exact prepared key and verify its digest/file
ledger again. A missing/evicted transfer fails clearly; Windows never rebakes it.
Download current-run Linux format fixtures by artifact ID. Add an optional
**proposed** `--compatibility-input PATH` to `windows_release.py`, defaulting to
the existing bundle fixtures for backward compatibility. The combined workflow
always passes the current-run directory without modifying the immutable cache.

Reuse MSVC 14.44 / SDK 10.0.22621.0, static runtime policy, selected native tests,
typed production-data validation, PE GUI/import checks, import graph check and
streaming ZIP. Keep the accepted Unicode/read-only NTFS/source-denial smoke and
external hash-pinned Mesa provider. Require `verification.json.status == passed`
and successful hidden depth-copy/readback/cleanup before advertising this job's
player artifact as verified. Preserve the existing audio-device skip as a named
limitation; a context-unavailable result cannot satisfy full LD04 verification.

Upload the exact ZIP/checksum and separate evidence containing Windows-written
compatibility fixtures with hashes. Keep graphical provider/test DLLs, CLI tools,
reports and captures outside the player payload. Record actual MSVC/CMake/Ninja/
Python/SDK/runner identities; tool-family drift fails explicitly, without silently
changing the accepted minimum or downloading a different toolchain.

### 3. Download verification and combined result

The final job checks out the same source and consumes exact artifact IDs from the
two producer jobs, bound to run ID/attempt and SHA. Download both raw players and
checksums and verify their transport hashes, complete tar/ZIP ledgers, single roots,
embedded source/build/content identities and absence of review/test payloads.
Compare common runtime content identities across packages while allowing platform
executables/notices to differ. The extracted smoke reports must name these exact
player hashes. Downloading an artifact does not justify rebuilding either game.

Run the transferred, hash-checked baseline Linux compatibility verifier against
the Windows-written tiny campaign/library/scenario fixtures and retain the reverse
import/re-encode result. It is an internal CI executable built at this same SHA,
not a player dependency or a cached executable. Restore its execute bit only after
ledger validation; archived Actions transfers do not preserve file modes. Execute
on the compatible Ubuntu 22.04 runner without production assets or recompilation.

Write the combined record atomically only after every required check passes.
Use a bounded versioned JSON object containing source/workflow/run identities,
both player artifact IDs/names/bytes/SHA-256, embedded metadata hashes, prepared
producer/consumer/input identity, per-platform verification references/status,
forward/reverse format results and explicit graphics/audio/notice limits. Include
artifact links in the job summary and publish the compact record as separate
review evidence. A missing, failed, skipped, unavailable, stale or mixed-source
required result cannot produce combined `passed`. A failed run may retain a
successfully verified platform artifact, but its summary must state that the
combined build is incomplete. No job treats an earlier run's package as this run's
result. Keep bounded failure tails/reports and always clean owned scratch after
runners finish; do not upload raw build trees or large logs.

## Package naming, transfer and storage bounds

Implement `blobber-VERSION-linux-x86_64.tar.gz` and preserve
`blobber-VERSION-windows-x86_64.zip`, each with a matching single game directory.
Exact revision/fingerprint stays in `BUILD.json` and external metadata. Preserve
explicit local `-dirty` naming and dirty-source rejection by default. Update Linux
name validation, bundle lookup, checksum lines, fixtures and player instructions
together. A new same-version run has distinct immutable artifact IDs and provenance;
never relabel or overwrite the accepted historical LD03 archive. Marked build-owned
bundle replacement keeps the existing candidate/promotion/rollback rules.

Use raw single-file `archive: false` uploads for each player and checksum, preserving
their bytes and plain filenames. Linux tar retains executable permissions inside
the tar; Windows remains one ZIP download with no extra wrapper. Current v7 raw
uploads derive the artifact name from the file, so require exactly one selected
file per upload. Fail on absent files. Keep review metadata/captures separate.

| Storage class | Proposed bound and lifetime |
| --- | --- |
| Prepared cache | Keep existing `wd01-prepared-v1-DIGEST` schema/path; at most two actual owned entries total, current and previous, each at most 1 GiB compressed, owned total at most 2 GiB. No compiler/build/Docker-layer cache. |
| Player artifacts/checksums | Seven-day retention; one player per platform per attempt, each at most 1 GiB, with its small adjacent checksum. Fail before upload if exceeded. |
| Review evidence | Seven-day retention; at most 32 MiB per platform, six selected captures per platform, no repeated player archive or uncompressed stage. |
| Internal Linux transport | One-day retention; at most 32 MiB for the already-built verifier, tiny generated fixtures and ledger. No production publication copy. |
| Final verification scratch | Disk-backed owned run, streaming download/hash/extraction with existing archive limits; delete extracts/downloads after checks and keep compact results. |

Retain the existing prepared file/count/unpacked limits and conservative compression
preflight. Check actual cache API size after save as well as the estimate; fail and
remove only the just-created owned oversized entry if the actual limit is exceeded.
Use explicit restore/save ordering and verify a new entry is available before
Windows begins. Cache saving is not a correctness guarantee.

GitHub caches are scoped by branch and cache version as well as key. An LD04 sibling
branch cannot assume access to the accepted WD01 branch cache: its first run may
legitimately perform one bake. Keep this behavior visible. Make pruning aware of
the current/default ref and cross-OS version so an inaccessible same-key entry does
not consume the reserved current slot. Preserve unrelated keys and active writers;
the common workflow concurrency protects owned writers. Test duplicate keys across
refs, oversized entries and cache disappearance. Do not broaden restore keys,
rename the namespace for cosmetic reasons or silently raise storage limits.

## Resource and verification budget

Use standard public-repository runners only. Default timeouts: Linux 150 minutes
(image, optional bake, package and two serial runtimes), Windows 60 minutes, final
verification 20 minutes. Keep at most two compile workers and one test process;
production bake/archive work stays serial. Bound OMP/OpenBLAS to one thread and
Mesa to two. Retain container CPU/memory/PID limits, Linux `nice -n 15`/`ionice -c 3`
and below-normal Windows priority. Check stage-specific free memory/disk before
image building, extraction and copying; fail rather than silently consuming the
runner disk. No automatic retry loop or new cache/paid runner to hide a limit.

Implementation checks, in order:

1. Small Python fixtures for plain Linux naming/provenance/dirty rejection and
   safe replacement, native/import command selection and mount ownership,
   current-fixture selection, cache/ref bounds, transport hashes and aggregate
   stale/mixed/failed/unavailable rejection. Reuse existing archive/PE/import tests;
   no production artwork needed. Use `linux_distribution_tools`,
   `distribution_archive`, `windows_distribution_tools` and a focused new hosted
   helper suite where behavior warrants it.
2. Workflow syntax and expression validation using a pinned available validator;
   review dispatch-only events, dependency chain, permissions, pinned actions,
   exact paths, uploads, timeouts and cleanup. Syntax success is not a hosted run.
3. One integration dispatch through the authenticated local `gh`. Linux runs the
   driver's existing 18 focused selections plus the small newly affected tooling/
   import/format checks, then one archive and the two isolated userspaces. Windows
   reuses its selected native packaging/persistence tests and extracted/depth
   verification. Remove the inherited Windows full `campaign_save` selection from
   the routine driver path: LD04 does not change persistence semantics and already
   runs `campaign_save_smoke`, portability/format and process-I/O checks. Keep the
   extended test available explicitly; do not run a full Debug/Release/GPU matrix.
4. Observe the exact run/jobs, inspect the final downloaded players/checksums and
   combined result, and record cache hit/miss and actual sizes. Reuse final-job
   transport checks rather than adding another full extraction/smoke. A cold run
   does not prove a hosted warm path: test hit selection with small fixtures and
   record the missing warm-hosted evidence. Run a second full dispatch only if a
   failure or an unresolved cache/integration question justifies it.

No local production bake/build is needed merely to repeat the hosted checkpoint.
If local focused compilation is necessary, read and retain `CMAKE_COMMAND` and
`CMAKE_MAKE_PROGRAM` from that tree and use configured CMake for builds/dry runs.
Never control desktops or inject OS input. Hosted Mesa, container userspaces and
the existing Windows play report do not establish new physical GPU or listening
coverage. Report skips/unavailable checks explicitly and keep user review separate.

## Implementation sequence and GitHub operations

1. Recheck working tree, current code, repo/default branch/workflow registration,
   account/Actions access and storage policy with `gh`; preserve new user edits.
   Start `distribution/ld04` from the accepted WD01 implementation plus this plan.
2. Implement naming/result/import/transport/cache changes and small fixtures first;
   then consolidate the existing workflow and update player/operator instructions.
   Keep shared prepared/manifest contracts and separate ELF/PE ownership.
3. Review exact local diff and checks, commit/push the implementation branch in
   the subsequently authorized LD04 scope. Verify its remote SHA, then dispatch:

   ```sh
   gh workflow run windows-review.yml --ref distribution/ld04
   ```

   This is a future operation, not a command run during planning. Check the exact
   run SHA immediately; registration already exists on `main`. No bootstrap merge
   or full Windows/LD04 merge is required to review the branch. Any eventual full
   implementation merge remains separately instructed; never force-push or bypass
   repository rules.
4. Track the precise workflow/ref/SHA/run ID/attempt. For raw player/checksum
   downloads use exact artifact API IDs and binary-safe output into owned paths;
   do not use `gh run download` to auto-extract a raw player ZIP or decode a raw
   text checksum. Use the existing WD01 raw download method and verify hashes.
   Archived evidence may use `gh run download` with an exact artifact name.
5. Retain selected final local bundles under their marked platform build/review
   directories and add a marked run-specific combined review bundle under
   `build-distribution-review/`. Record run/attempt/SHA and exact artifact IDs to
   distinguish same-version downloads. Retain only players/checksums/compact
   metadata and selected evidence; clean temporary copies through agent artifacts.
6. Update this handoff, roadmap and system overview to **Ready for review** after
   verification. Stop there. Only the user accepts/closes LD04.

Operator documentation must explain dispatch, locate exact player versus evidence
downloads, extraction/run/save/Continue review, compatibility limits, expiration
and failure diagnosis. Include a separate publication recipe: after an explicit
instruction naming the tag/version, target commit and draft/prerelease/public
intent, verify that both archived players and the combined record match that
commit/version, recheck checksums/notices, and attach only the two players and
their checksums. Do not rebuild or rename them during attachment. Expired/missing
artifacts need a new verified manual run, not an invented successful result.
The known font/music/audio notice gaps remain visible; LD04 supplies no legal
clearance or new end-user/project license.

## Exit evidence and planning handoff

Completion requires the checked-in combined manual workflow/operator instructions,
one actual successful same-source hosted run, both raw downloadable players and
checksums, Linux baseline/Debian results, native Windows extracted/depth results,
forward/reverse format results, verified downloaded bytes and compact combined
provenance. Record tool/image/source/cache identities, exact test selections and
outcomes, failed attempts/corrections, retained locations, and hardware/audio/
notice limitations here. If access or graphics is unavailable, record the missing
requirement and leave implementation incomplete rather than claiming acceptance.

Planning selected the choices above without unresolved product decisions. Recheck
mutable service access/tool availability at implementation time. Planning read
code/docs and public official service documentation only; it performed no build,
bake, package, workflow edit/dispatch, Git commit/push/merge, settings mutation,
tag or publication. Documentation validation and scratch cleanup are recorded below.

Policy references checked 2026-11-12:

- [Manual dispatch and default-branch registration](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow).
- [Cache matching, cross-OS support and branch restrictions](https://docs.github.com/en/actions/reference/workflows-and-actions/dependency-caching).
- [Official upload-artifact v7 inputs and raw-file behavior](https://github.com/actions/upload-artifact/blob/v7/README.md).
- [Workflow concurrency and pending-run behavior](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency).
- [Actions billing and separate storage accounting](https://docs.github.com/en/billing/concepts/product-billing/github-actions): standard public-repository hosted compute remains free; storage is separately accounted. Recheck account allowances before implementation; no unlimited-storage or future pricing guarantee.

Planning verification: `git diff --check` passes. A focused Python document check
resolved all local links in this design, the roadmap and system overview; confirmed
one heading per distribution slice, WD01 before LD04, consistent planning state and
both design-index links. No executable/workflow behavior changed, so no gameplay
tests, configuration changes or production work were run. Owned scratch run
`output/agent-runs/20261112T124027Z-53a58d54c7ec4cba9d59b448c1d7c52e/`
was previewed and cleaned with `tools/agent_artifacts.py clean RUN_DIRECTORY` and
`--apply`; no disposable output or diagnostic exception remains.

Implementation adaptation, 2026-11-12: the accepted baseline image has no Git.
Prepared imports use an optional host-captured source-file ledger, passed by CMake
to `prepared.py` inside the read-only source mount. Input files are still hashed
and compared; host source identity is checked again after the build. This avoids
changing the accepted compiler image/package inventory merely to enumerate files.

## Implementation verification in progress — 2026-11-12

Implemented the registered three-job manual workflow, optional Linux import/source
ledger, plain Linux package name, current-run format transport, bounded cache
management, exact artifact downloads and combined result gate. Added the
[operator guide](../packaging/README.md). No gameplay or persistence format changed.

Local focused commands: `python3 tests/distribution_archive_tests.py` (10 pass),
`python3 tests/windows_distribution_tests.py` (9 pass),
`python3 tests/linux_distribution_tests.py` (17 pass), and
`python3 tests/hosted_distribution_tests.py` (10 pass, about 0.02 s). The latter
checks real tiny tar/ZIP validation/corruption, unavailable/stale/incomplete smoke,
mixed run/attempt/content/producer, safe evidence extraction, Git-free checked
import and prior-output preservation, cache ref/version bounds and wrong-run
artifact rejection. Windows tests omit Linux chmod verification because NTFS does
not implement that POSIX contract; Linux exercises it. A duplicate ZIP-entry
warning is deliberate malformed-fixture input. Python 3.10 syntax checks pass.

Official `actionlint` v1.7.12, archive SHA-256
`8aca8db96f1b94770f1b0d72b6dddcb1ebb8123cb3712530b08cc387b349a3d8`,
passes `.github/workflows/windows-review.yml` (`-shellcheck=`; shellcheck unavailable).
`git diff --check` passes. These are local tooling checks, not hosted success.
No local production compilation, bake or desktop input was run. The subsequent
hosted run and exact downloaded results remain to be recorded.

Hosted-path correction during integration: moved the disk/memory guard into the
shared Linux `build_release` API, since the hosted helper calls that API directly
rather than the CLI wrapper. A focused fixture proves insufficient disk or memory
fails before lock/build creation. Added per-platform and final-runner image/OS/arch
identity to compact evidence. The focused hosted suite now has 11 passing cases
(one Linux-only case is skipped on Windows). These changes do not affect prepared
inputs. The failed cold run produced no publication. The next dispatch must create one
with the corrected transport and entry point. Verify the full Linux import graph
and typed payload against that publication within the same job, without another
game compilation or bake; the native Windows consumer then restores that exact key.
A further full warm-producer run is unnecessary unless a failure justifies it;
record that path as unexecuted rather than claiming hosted warm-build evidence.

Integration failure and transport adaptation: cold run
[43223470781](https://github.com/biscon/blobber/actions/runs/43223470781), source
`f2fdd90231d2023bd0a7e7c0b4184dab5a2ec87c`, failed before compilation or baking.
The accepted compiler-image recipe spent 46 minutes on Ubuntu snapshot package
fetches and failed with HTTP 503 for graphics/LLVM dependencies. Windows was
correctly skipped and the combined job rejected incomplete producers; hosted
scratch cleanup completed. No prepared publication or player was produced.

Use LD03's existing Ubuntu runtime transport pattern for the compiler image:
curate SHA-256-pinned `.deb` URLs from signed Ubuntu indexes, selecting only the
exact package versions already in `packaging/linux/packages.tsv`. The final recipe
uses those locked inputs without rolling resolution and verifies the same complete
installed inventory. Record signed-index hashes and the package-ledger hash in a
small checked-in transport lock. Compiler/CMake/dependency source pins, glibc 2.35,
static-link policy and generic CPU flags stay unchanged. Recipe identity includes
the new ledger/transport lock, and preparation digest includes the producer recipe
inputs so changed producer environments cannot reuse a stale key. Existing native
build/image caches remain preserved; hosted builds use a fresh dedicated identity.
This corrects transport availability rather than changing the compatibility floor.

The compiler transport image was built locally at low priority with a two-CPU/
4 GiB cap, without compiling the game or baking assets. Its installed inventory
compares byte-for-byte with the unchanged accepted 251-package `packages.tsv`
(SHA-256 `a7d18868e90d8e0f58fc7213c1bc9f84a768aed804533e00df5c960ddaa9a8f7`).
It passes notice verification and retains GCC 11.4.0, CMake 3.31.12 and glibc 2.35.
The first local installer attempt exposed APT’s `--no-download` handling of local
archives; removed that flag, matching LD03’s runtime recipe. Apt sources/indexes
are absent during installation, so dependency resolution uses only the locked
local debs. The corrected image passes. The signed transport lock pins 150 debs /
124,719,810 bytes; no package version or source-tool pin was changed.

The cold producer now configures a fresh import-only CMake tree inside its marked
run, using the already-built compiler image and a host-captured source ledger. It
executes only `blobber_import_prepared`, validates that real imported publication
with the existing native typed validator, and dry-runs the game graph to reject
production bakers. No second game or bake is built. Its roughly 1.3 GiB temporary
copy is removed before runtime extraction. A warm producer uses its actual game
import result instead. The compact import proof is bound to the publication hash
and required by final download verification. Local hosted fixtures now total 13
passing cases, with Linux-specific cases explicitly skipped on Windows.

Corrected run [43233063432](https://github.com/biscon/blobber/actions/runs/43233063432),
source `d82aeb5843d968172a3027140205891204ac53b1`, built the compiler image in
94 seconds, passed its inventory/notices/environment check and compiled focused
C++ targets in 3m48s. It then exposed a hosted-API integration bug: container HOME/
TMPDIR creation had remained only in the CLI wrapper. Eight C++ fixture suites
failed immediately because `/run-output/tmp` did not exist; 11/19 passed. No
production bake occurred. Moved HOME/TMPDIR setup into the shared build entry point
and added a fixture asserting those directories exist before container invocation.
The focused hosted suite now passes all 14 local cases. Windows was correctly
skipped; combined verification rejected incomplete producers and scratch cleanup
completed. The next serial dispatch verifies this wrapper correction.

Run [43234513066](https://github.com/biscon/blobber/actions/runs/43234513066),
source `841ac2bbe70d4103850c026d1f1181786e9bfb3b`, passed all 19 Linux suites,
production preparation/typed validation, stage/ELF audit/archive, real Git-free
CMake import/data validation, and Ubuntu 22.04 plus Debian 13 extracted smoke.
It failed afterward collecting review evidence: `hosted_evidence.py` imported
`verify_package` outside its package, causing a relative-import error. Correct the
import and add a tiny actual platform-evidence creation fixture.

Separate the Linux helper into build/preparation and runtime/evidence steps within
the same serial job. Atomically record a context-bound build handoff after typed
publication/import validation. Save and check the bounded prepared cache then,
before userspace verification/report collection. A later failure still blocks
player upload/Windows/combined success, but does not discard a valid costly
publication. This also makes build versus runtime progress visible through CLI
job status, without extra builds, bakes or a generic scheduler.
