# Desktop distribution builds

The manual **Desktop distribution build** workflow produces Linux x86-64 and
Windows x86-64 packages from one selected source revision. It lives at
`.github/workflows/windows-review.yml` to preserve the existing workflow
registration. Until a separately instructed full merge, GitHub’s default-branch
workflow list may still show the old `Windows review build` display name; the
filename and selected-branch LD04 jobs identify this workflow. Commits, PRs, merges,
tags and Releases do not start it.

From an authenticated `gh` client with repository/workflow access:

```sh
gh workflow run windows-review.yml --ref distribution/ld04
gh run list --workflow windows-review.yml --branch distribution/ld04 --limit 5
gh run watch RUN_ID
gh run view RUN_ID --log-failed
```

Use the intended branch/ref and confirm the run's exact SHA. Choose the exact run
ID, not an ambiguous latest successful build. Dispatch one review run at a time;
the shared concurrency group prevents simultaneous builds but does not promise a
FIFO queue. Use a new manual dispatch for a retry, preserving old evidence.

## Downloads and review

A successful complete run's summary links these raw single-file player downloads:

- `blobber-VERSION-linux-x86_64.tar.gz`
- `blobber-VERSION-windows-x86_64.zip`

Each extracts into one game directory. The adjacent `.sha256` files are optional
integrity downloads. `linux-review-evidence`, `windows-review-evidence` and
`combined-verification` are separate developer evidence; they are not needed to
play. Player archives contain no review reports, screenshots, test executables or
extra archive wrapper. Revisions live in `BUILD.json`; preserve that metadata when
reporting a problem. Same-version runs have different immutable artifact IDs and
may have different bytes.

Download the raw player/checksum by exact artifact ID if using the CLI:

```sh
python3 tools/agent_artifacts.py create
# Use the printed owned run path for RUN_DIRECTORY below.
gh api --allow-escape-sequences repos/biscon/blobber/actions/artifacts/PLAYER_ID/zip > RUN_DIRECTORY/blobber-VERSION-windows-x86_64.zip
gh api --allow-escape-sequences repos/biscon/blobber/actions/artifacts/CHECKSUM_ID/zip > RUN_DIRECTORY/blobber-VERSION-windows-x86_64.zip.sha256
# Run in the download directory:
sha256sum --check blobber-VERSION-windows-x86_64.zip.sha256
```

The API suffix `/zip` also serves unwrapped raw artifacts. `gh run download`
automatically extracts ZIPs; use it only for archived evidence, with an exact
artifact name, not for preserving raw player ZIP/checksum bytes. All players and
review evidence expire after seven days. The small internal Linux format transport
expires after one day. Expiration is not public Release publication.

Linux: extract the complete tar, run `./blobber`; requires glibc 2.35+, generic
x86-64, X11/XWayland and OpenGL 3.3. Windows: extract the complete ZIP and launch
`blobber.exe`; targets Windows 11 x64 / OpenGL 3.3. Neither requires a compiler or
administrator install. Keep the package intact. Check New Campaign, town/inn/
dungeon resources, save, quit and Continue. Verify graphics and audio on the actual
machine; `--no-audio` permits use without an audio device. See the shipped README
for user-data paths. Keep saves/user data and older packages during upgrades.

The workflow verifies Ubuntu 22.04 and Debian 13 runtime userspaces using one Linux
package under Xvfb/Mesa, then the native Windows package under software OpenGL.
It checks read-only installation, default asset discovery, isolated writable user
paths, New Campaign/save/fresh-process Continue and both directions of tiny
campaign/library/scenario format interchange. The final job verifies downloaded
bytes and provenance. A green platform job alone is not combined success; require
the final `combined-verification` record. Failed/unavailable required graphics
checks leave the combined result incomplete. These checks do not establish
physical GPU compatibility or playback/listening; containers share a host kernel,
and the hosted Windows Server userspace is separate from Windows 11 play review.

## Storage, diagnosis and local commands

The Linux job builds in the accepted pinned baseline container. On a prepared-cache
miss it bakes once in that same build. An exact validated hit imports the portable
publication; Windows restores that same key and never silently rebakes. Cache
branch/version restrictions can cause the first build on a new branch to miss.
The shared owned namespace retains at most current/previous entries, 1 GiB each;
players are capped at 1 GiB and each evidence/transport bundle at 32 MiB. No build
trees, compiler cache, production-publication artifact or Docker-layer cache is
uploaded. Standard runners only; do not enable paid storage/runners to clear a
failure without a separate instruction.

Inspect the first failed phase's bounded log tail. Missing preparation, hash/source
mismatch, unavailable software GL, changed tool families or exhausted capacity are
failures to resolve, not reasons to accept an older package. Native Windows
`tools/windows_release.py --code-check --run-dir RUN_DIRECTORY` remains a developer
command without playable-package claims; the combined workflow always builds both
full packages. Local Linux build/verification instructions remain in
[linux/README.md](linux/README.md). Optional Linux import:

```sh
python3 tools/linux_release.py dist --run-dir RUN_DIRECTORY --build-dir build-linux-baseline-import --prepared-input PREPARED_BUNDLE
```

Use a fresh marked run and dedicated build identity. Preserve configured CMake and
Ninja for existing trees. After selecting intentional final review bundles, preview
and apply `tools/agent_artifacts.py clean RUN_DIRECTORY [--apply]`. Do not delete
unrelated caches, saves, active runs or accepted historical packages.

## Separately authorized public Release attachment

Building/downloading does not publish a GitHub Release or create/push a tag. Before
publication, obtain the explicit target tag/version, commit and draft/prerelease/
public intent. Match that commit/version to a passed combined record and both
embedded BUILD files, recheck the archive checksums and applicable notices, then
attach only the exact two verified players and their checksums. Do not rebuild,
rename or recompress at attachment time. Preserve the combined record internally
as provenance. Expired/missing artifacts require a new verified manual run.

Existing font/music/audio provenance gaps remain in the
[distribution handoff](../docs/linux_distribution_ld03.md); no project license,
end-user terms or publication clearance is introduced by packaging. Full details,
actual run identities and limitations are in the [LD04 handoff](../docs/linux_distribution_ld04.md).
