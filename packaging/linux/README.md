# Controlled Linux baseline (LD02)

The environment targets Ubuntu 22.04 amd64 / glibc 2.35, OpenGL 3.3 and X11/XWayland.
It deliberately does not inherit the development host's toolchain or libraries.
See [design and executed handoff](../../docs/linux_distribution_ld02.md).

From the checkout root, with Docker available:

```sh
python3 tools/agent_artifacts.py create
python3 tools/linux_release.py build --run-dir output/agent-runs/RUN_ID
```

The driver builds/reuses the recipe-hashed image, verifies `packages.tsv`, configures
an independent `build-linux-baseline/` tree, runs focused tests, and builds
`dist-audit`. Configuration and compilation have networking disabled. The source
mount is read-only; container output belongs to the caller, not root. Only the
image preparation downloads packages and verified sources. No GPU/display/audio
socket or Docker socket is passed into the build container.

Intentional outputs are `build-linux-baseline/dist-stage/Release/`, its LD01
inventory and `Release.audit.json`; retain the build cache for reuse. A passing
audit is bound to its stage ID, inventory/ELF/policy hashes and environment identity.
Always invoke the driver rather than trusting an old output directory.
For checker/policy-only corrections, `python3 tools/linux_release.py audit
--run-dir output/agent-runs/RUN_ID` reruns notice/checker tests and stages/audits
with incremental assets, without rerunning the unrelated passing suites. Configuration
identity changes require a fresh `--build-dir build-linux-baseline-SUFFIX`; no cache
or unrelated file is deleted automatically. Native CLion trees remain independent.

The pinned base digest, fixed Ubuntu snapshot and verified CMake/dependency source
hashes live in `lock.json`. The complete package/version/architecture resolution is
in `packages.tsv`; unexpected resolution fails the build. `bootstrap-ca.pem` is a
public Mozilla CA bundle retrieved from curl.se, hash-pinned by the lock, used only
to bootstrap HTTPS before the snapshot's ca-certificates package is installed.
Its source/provenance and Mozilla license reference are in its existing header.
It is a build input, not player content. Rebuilding an image is not a promise of
byte-identical compiler output or image layer hashes.

Runtime system libraries are enumerated in `runtime-policy.json`: glibc, X11 and
OpenGL/driver libraries are host-provided. GLFW extensions and miniaudio audio
backends are audited separately as runtime loads. Sound requires a functioning
host backend/server/device; optional library resolution is not a playback test.
The game defaults to GLX. EGL/OSMesa paths remain unused optional GLFW code.
No private libc, driver, raylib, yaml-cpp or compiler shared runtime is bundled.

For dependency notice reconciliation inside the image:

```sh
python3 /src/packaging/linux/notices.py --check /src/packaging/notices
```

The normal driver performs this check before compiling. No project license or
publication clearance is implied. Archive/relocation checks are LD03; CI is LD04.
After recording verification, preview then apply cleanup of the owned agent run
with `python3 tools/agent_artifacts.py clean RUN_DIRECTORY [--apply]`.
