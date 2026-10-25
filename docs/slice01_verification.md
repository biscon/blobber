# Slice 01 verification and review

Executed 2026-10-25 on Linux. Slice 01 was subsequently accepted by the user on
2026-10-25; the verification results and limitations below are unchanged.
See [rules design](rules_design.md) and the [delivery record](dnd5e_demo_design.md#slice-01-rules-foundation-and-content).

## Builds and headless checks

Commands run from the repository root:

```sh
cmake --preset debug
cmake --build --preset debug -j 4
ctest --preset debug
cmake --preset release
cmake --build --preset release -j 4
ctest --preset release
```

Final results: **8/8 passed in Debug (12.28 s), 8/8 in Release (1.87 s)**.
Both builds completed without remaining warnings in the new code. The first
runs exposed a diagnostic-order issue: an invalid attack ID was reported as an
unresolved reference before ID validation. IDs now validate before resolution.
Focused `ctest --preset debug -R '^rules'` passed after the fix, followed by both
complete suites above. No failing checks remain.

`rules` checks observable replay for seed 5489, standard dice and numeric limits,
ability rounding, proficiency, d20 selection/cancellation/consumption, attack/save
thresholds and natural rolls, malformed inputs, critical damage, and catalog
handle validity. `rules_content` owns generated JSON and isolated temporary files;
it checks resolution, validation diagnostics, byte/count limits, malformed and
duplicate JSON, and successful reuse after failures. It does not read game assets
in routine CTest. `frame_allocations` includes 1,000 rules iterations after
preparation and still reports zero engine-owned C++ allocations. Existing six
suites remain passing. No cross-platform or third-party allocation claim is made.

Explicit production-content checks, separate from test-owned correctness fixtures:

```sh
cmake-build-debug/blobber_rules_content_tests --validate assets/rules/catalog.json
cmake-build-debug/blobber_rules_content_tests --validate cmake-build-debug/assets/rules/catalog.json
cmake-build-release/blobber_rules_content_tests --validate cmake-build-release/assets/rules/catalog.json
cmp assets/rules/ATTRIBUTION.md cmake-build-debug/assets/rules/ATTRIBUTION.md
cmp assets/rules/ATTRIBUTION.md cmake-build-release/assets/rules/ATTRIBUTION.md
git diff --check
```

All passed; each catalog reported one monster and two attacks. Attribution is
copied byte-for-byte with the rules directory.

## Startup failure ordering

The following Python check was executed from the repository root. Missing rules
and an unsupported version fail with rules diagnostics before graphics startup.
A valid copied catalog advances to the deliberately missing map, establishing
that rules loading completes before world/image preparation. All three passed.

```python
import pathlib, tempfile, subprocess
root = pathlib.Path.cwd()
exe = root / 'cmake-build-debug/blobber'
source = root / 'assets/rules/catalog.json'
with tempfile.TemporaryDirectory(prefix='blobber-rules-startup-') as temp:
    assets = pathlib.Path(temp)
    (assets / 'rules').mkdir()
    def run(expected):
        result = subprocess.run([str(exe), '--assets', str(assets)],
                                capture_output=True, text=True, timeout=15)
        assert result.returncode == 1
        assert expected in result.stderr
        assert 'Initializing raylib' not in result.stdout
    run('Loading rules')
    catalog = assets / 'rules/catalog.json'
    catalog.write_text('{"schema_version":2,"attacks":[],"monsters":[]}')
    run('schema_version')
    catalog.write_bytes(source.read_bytes())
    run('Cannot read map')
```

## Desktop startup

The sandbox could not connect to display `:0`. A permitted unsandboxed
`xdpyinfo -display :0` succeeded, so the desktop check was executed with the same
access. The application invocation was:

```sh
cd /tmp
/home/bison/Projects/blobber/cmake-build-debug/blobber --size 1280 720 --capture /tmp/blobber-slice01-desktop-7tmrzmkj
```

A Python subprocess wrapper supplied a 120-second timeout, redirected output to
`startup.log` in that temporary directory, and checked exit status and captures.
Result: exit 0, 99 PNGs, OpenGL 3.3 on NVIDIA GeForce RTX 3060, and logged cleanup
of framebuffers/textures/window. The copied executable-relative rules/assets
loaded successfully with `/tmp` as the working directory. Visually inspected
`01-corridor.png` and `npc-front.png`: existing dungeon framing, HUD, and goblin
presentation are intact. Logical scene captures remain 1920×1080.

These are startup/presentation checks, not combat acceptance or a pixel-baseline
comparison. No interactive gameplay recheck, standalone GPU-resource smoke suite,
Windows/macOS run, or performance benchmark was performed for this CPU/content
slice. Temporary captures are disposable; the invocation above regenerates them.

## User review

1. Run `ctest --preset debug -R '^rules'` and
   `cmake-build-debug/blobber_rules_content_tests --validate assets/rules/catalog.json`.
   Expect both focused suites to pass and the catalog to report one monster/two attacks.
2. Inspect `assets/rules/catalog.json` alongside the source ledger. To try edits,
   copy it to a temporary file and run the same `--validate` command against that
   file. Changing goblin HP to another positive int32 should succeed; an unknown
   attack reference, zero HP, or unsupported version should produce a file/field
   diagnostic and exit 1. No rebuild is needed for JSON changes.
3. Optionally launch `cmake-build-debug/blobber --size 1280 720` from a desktop.
   Expect the existing dungeon; W/S/A/D/Q/E and Escape retain their behavior.
   Goblins do not fight yet. Review the foundation and content, not later combat.
4. Review completed: the user accepted slice 01 on 2026-10-25. Slice 02 remains
   Planned and is now eligible for its own planning session and plan review.
