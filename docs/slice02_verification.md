# Slice 02 verification and review

Executed 2026-10-25 on Linux. Slice 02 was subsequently accepted by the user on
2026-10-25; the verification results and limitations below are unchanged.
See the [character design](characters_design.md) and
[delivery record](dnd5e_demo_design.md#slice-02-characters-and-preset-party).

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

Final results: **10/10 passed in Debug (12.61 s), 10/10 in Release (1.85 s)**.
Both builds completed without warnings in the new code. Focused rules and party
checks also passed during implementation. No failing checks remain.

`party` checks the four fixed mechanical examples, all sixteen supported race/class
pairings with legal gear, HP/resources, six saves, initiative, armor changes,
shield/Defense interaction, negative/capped Dexterity, finesse (including ranged),
versatile grip, trained/untrained weapons, carried previews, weight/quantity limits,
formation identity, invalid references, and in-memory validation. `party_content`
loads its own JSON fixtures and checks malformed definitions/builds, unresolved
references, strict fields/types, capacities, ownership/hand conflicts, legacy
schema support, and successful loading after failures. Neither depends on shipped
content. Existing parser tests retain duplicate-key/depth/byte-limit coverage.

`frame_allocations` performs 1,000 party-query/derivation/formation iterations after
preparation, including all four members, invalid builds, and rejected swaps, in
addition to its existing rules/render/input work. It reports zero engine-owned
C++ allocations. This does not cover driver internals or C allocator interception.

During implementation review, ranged finesse was corrected to choose the best
Strength/Dexterity modifier rather than always using Dexterity; a dedicated fixture
covers that interaction. No shipped item or scope change was needed.

## Production content checks

These explicit checks are separate from test-owned correctness fixtures:

```sh
cmake-build-debug/blobber_rules_content_tests --validate assets/rules/catalog.json
cmake-build-debug/blobber_rules_content_tests --validate cmake-build-debug/assets/rules/catalog.json
cmake-build-release/blobber_rules_content_tests --validate cmake-build-release/assets/rules/catalog.json
cmp assets/rules/catalog.json cmake-build-debug/assets/rules/catalog.json
cmp assets/rules/catalog.json cmake-build-release/assets/rules/catalog.json
cmp assets/rules/ATTRIBUTION.md cmake-build-debug/assets/rules/ATTRIBUTION.md
cmp assets/rules/ATTRIBUTION.md cmake-build-release/assets/rules/ATTRIBUTION.md
git diff --check
```

All passed. The catalog reports one monster, two monster attacks, four races,
four classes, and thirteen items. It prints these initialized party results:

| Position / character | HP | AC | Available Hit Dice | Weight / capacity |
| --- | --- | --- | --- | --- |
| Front left / Alden | 12/12 | 19 | 1d10 | 64/240 lb |
| Front right / Brunna | 12/12 | 16 | 1d8 | 56/195 lb |
| Back left / Merrin | 10/10 | 14 | 1d8 | 15/120 lb |
| Back right / Lethiel | 7/7 | 13 | 1d6 | 5/120 lb |

Everyone starts at level 1 and XP 0. No mana progression is implemented.

## Startup failure ordering

The following Python check was executed from the repository root. All six checks
passed: missing catalog, valid legacy catalog without party, schema 2 with null
preset, unsupported version, unresolved race, and valid production content advancing
to the deliberately missing map. None initialized raylib.

```python
import json, pathlib, subprocess, tempfile
root = pathlib.Path.cwd()
source = json.loads((root / 'assets/rules/catalog.json').read_text())
exe = root / 'cmake-build-debug/blobber'
with tempfile.TemporaryDirectory(prefix='blobber-party-startup-') as temp:
    assets = pathlib.Path(temp)
    (assets / 'rules').mkdir()
    path = assets / 'rules/catalog.json'
    def run(expected):
        r = subprocess.run([str(exe), '--assets', str(assets)],
                           capture_output=True, text=True, timeout=15)
        assert r.returncode == 1 and expected in r.stderr
        assert 'Initializing raylib' not in r.stdout
    run('Loading rules')
    path.write_text(json.dumps({'schema_version': 1, 'attacks': [], 'monsters': []}))
    run('catalog has no preset_party')
    path.write_text(json.dumps(dict(source, preset_party=None)))
    run('catalog has no preset_party')
    path.write_text(json.dumps(dict(source, schema_version=9)))
    run('schema_version')
    data = json.loads(json.dumps(source))
    data['preset_party']['characters'][0]['race'] = 'missing'
    path.write_text(json.dumps(data))
    run('unknown reference: missing')
    path.write_text(json.dumps(source))
    run('Cannot read map')
```

## Desktop startup and presentation

The sandbox could not connect to display `:0`; approved display access was used
for the desktop application check. A Python subprocess wrapper ran this invocation
from `/tmp`, with a 120-second timeout and stdout/stderr redirected to `startup.log`
in the capture directory:

```sh
/home/bison/Projects/blobber/cmake-build-debug/blobber --size 1280 720 --capture /tmp/blobber-slice02-desktop-2ow2x_fs
```

Result: exit 0, **99 PNGs**, OpenGL 3.3 on NVIDIA GeForce RTX 3060, and clean
GPU/window shutdown. Executable-relative copied rules/assets loaded while the
working directory was `/tmp`. Visually inspected `01-corridor.png` and
`npc-front.png`: existing dungeon framing, HUD, and goblin presentation are intact.
Logical captures remain 1920×1080. Temporary captures are disposable and can be
regenerated with the command above using a fresh output directory.

This was a startup/capture smoke check. No interactive gameplay recheck, pixel
comparison against an earlier baseline, standalone GPU lifecycle suite, stress
benchmark, or Windows/macOS execution was performed. Rendering and controls were
not changed. Party panels and combat deliberately do not appear in this slice.

## User review

1. Run `ctest --preset debug -R '^(rules|rules_content|party|party_content)$'`.
   Expect all four focused suites to pass.
2. Run `cmake-build-debug/blobber_rules_content_tests --validate assets/rules/catalog.json`.
   Review the formation, abilities, resources, equipment, attacks, and table above.
3. Copy the catalog to a temporary file and edit that copy. Set Alden's
   `equipment.off_hand` to null: AC becomes 17, his longsword preview becomes 1d10+3,
   and his carried shield still counts toward weight. Set `equipment.body` to
   null as well: AC becomes 12, with Defense inactive. Validate after each edit.
   An unknown race, duplicate formation entry, or invalid equipment index must
   fail with a diagnostic. Edits to the temporary file do not alter the game.
4. Optionally launch `cmake-build-debug/blobber --size 1280 720` from a desktop.
   Expect the existing dungeon and controls, with party validation at startup.
   There is no character UI, gear editing, casting, or combat yet.
5. Review completed: the user accepted slice 02 on 2026-10-25. Slice 03 remains
   Planned and is now eligible for its own planning session and plan review.
