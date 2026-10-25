# Slice 03 verification and review

Executed 2026-10-25 on Linux. Slice 03 was subsequently accepted by the user on
2026-10-25; the verification results and limitations below are unchanged.
See [actor design](actors_design.md) and the
[delivery record](dnd5e_demo_design.md#slice-03-mutable-monster-world).

## Builds and headless checks

Commands run from the repository root:

```sh
cmake --preset debug
cmake --build --preset debug -j 4
ctest --preset debug
cmake --preset release
cmake --build --preset release -j 4
ctest --preset release
cmake --build --preset debug --target blobber_gpu_smoke -j 4
git diff --check
```

Both builds succeeded without compiler warnings. Final full suites: **12/12
passed in Debug (14.99 s), 12/12 in Release (2.24 s)**. The two additions are
`actors` (graphics-independent) and `actor_render` (CPU composition, no window).
The existing configured builds also passed 10/10 before implementation.

Actor checks cover initialized catalog HP/identity, unknown bindings, all 256
slots and 32 types, exhaustion, foreign/destroyed/removed handles, invalid and
occupied destinations, unchanged state on failure, subslot changes, facing,
zero/negative/oversized damage, backwards hit timestamps, exactly-once death and
removal, reused spatial slots, stable record indices, and queued movement against
changing occupancy. Fixtures own their maps/catalogs and never load game content.

Presentation checks cover stationary invalidation/reuse, stale/foreign animation
snapshots (including cached frames with equal revisions), surviving and lethal flash boundaries, frozen lethal pose, repeated
hits, skipped/hidden playback, alpha/brightness behavior, wall occlusion, removal,
world replacement, and both endpoints of all six movement transitions. Existing
NPC geometry, depth, transparency, shadows and animation tests remain passing.

The allocation guard exercises 100 move/turn/rejection/render iterations, plus
surviving damage, lethal flashes, expiry and removal, alongside its prior frame,
input, party and rules work. **Zero engine-owned C++ allocations** were observed
once preparation ended. The guard does not intercept C allocation APIs or claim
anything about driver internals.

An initial capacity fixture omitted required perimeter walls; it was corrected
before the passing results above. The desktop diagnostic initially selected a
right strafe into a wall; its pose was corrected and the complete capture run
repeated. The final diagnostic targets the foreground left goblin to make its
flash easy to inspect. Both application builds were refreshed after these
capture-only adjustments; core/test sources were unchanged after the full suites.

## Startup and desktop integration

An isolated temporary asset directory linked the production NPC directory/map
and contained a copy of the rules catalog with its monster ID changed. Running
`cmake-build-debug/blobber --assets TEMP_DIRECTORY` returned 1 with
`NPC appearance has no monster definition: goblin` and no raylib initialization.
The precise check was:

```python
import json, pathlib, subprocess, tempfile
root = pathlib.Path.cwd()
with tempfile.TemporaryDirectory(prefix='blobber-slice03-binding-') as temp:
    assets = pathlib.Path(temp)
    (assets / 'npcs').symlink_to(root / 'assets/npcs', target_is_directory=True)
    (assets / 'maze.txt').symlink_to(root / 'assets/maze.txt')
    (assets / 'rules').mkdir()
    data = json.loads((root / 'assets/rules/catalog.json').read_text())
    data['monsters'][0]['id'] = 'unknown-for-appearance'
    (assets / 'rules/catalog.json').write_text(json.dumps(data))
    r = subprocess.run([str(root / 'cmake-build-debug/blobber'), '--assets', str(assets)],
                       capture_output=True, text=True, timeout=15)
    assert r.returncode == 1 and 'NPC appearance has no monster definition: goblin' in r.stderr
    assert 'Initializing raylib' not in r.stdout
```

The sandbox could not connect to display `:0`; approved desktop access was used
for the following checks. Before implementation, the original Release executable
captured its baseline; the final executable ran from `/tmp`, verifying copied,
executable-relative assets. Commands (logs redirected to the capture directories):

```sh
# Before code changes, from the repository:
timeout 60 cmake-build-release/blobber --capture /tmp/blobber-slice03-before
# Final application, working directory /tmp:
timeout 60 /home/bison/Projects/blobber/cmake-build-release/blobber --capture /tmp/blobber-slice03-after
# From the repository:
timeout 30 cmake-build-debug/blobber_gpu_smoke
```

Final application exit: **0**, with **115 PNGs** and clean window/GPU shutdown.
All **99 original PNGs were pixel-identical**, including the window screenshot;
16 new actor diagnostics were added. Logical captures remain 1920×1080; the
window screenshot is 1280×720. Comparison used decoded pixels, not PNG file bytes:

```python
from pathlib import Path
from PIL import Image
before, after = Path('/tmp/blobber-slice03-before'), Path('/tmp/blobber-slice03-after')
for p in before.glob('*.png'):
    with Image.open(p) as a, Image.open(after / p.name) as b:
        assert a.size == b.size and a.tobytes() == b.tobytes(), p.name
assert len(list(before.glob('*.png'))) == 99
assert len(list(after.glob('*.png'))) == 115
```

Visually inspected baseline, move, turn, surviving hit, lethal flash at 99 ms,
death expiry at 100 ms, and right-strafe removal captures. White silhouettes keep
transparent holes and disappear on schedule; foreground removal reveals the
smaller goblin behind it. The GPU lifecycle executable passed on OpenGL 3.3,
NVIDIA GeForce RTX 3060. Temporary captures/logs are disposable and regenerable.

## User review

1. Run `ctest --preset debug -R '^(actors|actor_render|frame_allocations|npc|npc_animation)$'`.
2. From a desktop, run `cmake-build-debug/blobber --capture /tmp/blobber-slice03-review`.
   The normal game still has no actor manipulation controls, AI or combat UI.
3. Compare `actor-baseline.png`, `actor-moved.png`, and `actor-turned.png`.
   The foreground left goblin moves one cell away or turns west in isolated cases.
4. Compare `actor-hit-700.png`, `actor-hit-799.png`, and `actor-hit-800.png`:
   a hit at 700 ms flashes white through 799 ms, returning to normal at 800 ms.
   Compare the corresponding `actor-death-*` images: lethal feedback holds its
   pose through 799 ms and hides the body/shadow at 800 ms. Other actors remain.
5. Inspect `actor-removed.png` and `actor-transition-removed-0.png` through `-5.png`.
   All actors and shadows are removed, including from already-cached transition
   endpoints. Headless tests establish immediate unblocking and stale-handle behavior.

Capture poses target the shipped demo level; actor scenarios are skipped for an
empty manifest or a different first placement cell. Routine tests use independent
fixtures. No manual interactive movement recheck, Windows/macOS execution, scale
benchmark, combat playtest, or new animation artwork was performed. These are
not claimed as passing. The user accepted slice 03 on 2026-10-25. Slice 04
remains Planned and is now eligible for its own planning session and plan review.
