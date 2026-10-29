# Slice 12.2 verification and handoff

Status: **Accepted and closed**, 2026-10-29. The user explicitly accepted the slice:
“slice accepted, record it in the plan doc, so we can get started on the next slice”.
Slice 12.3 is next eligible for planning and remains Planned/unstarted. No menu, creator screen, leveling, rest
repreparation or session save/resume was added.

## Delivered

- Pure bounded creation drafts and previews: exact 27-point buy, racial increases,
  fixed supported features, data-driven curated gear and explicit spell grants.
- Library schema 1 with stable UUIDv4 identities, 128-record/1 MiB/depth-16 bounds,
  checked revision writes, writer lock, previous-good backup and atomic replacement.
  Empty stores stay empty; invalid/interrupted stores are preserved and block writes.
- Catalog schema 8, legal preset spell selections, quarterstaff content/art,
  independent explicit-party/session construction, and three party camp kits once
  per launch. Preset names, base stats, other gear and four original portraits remain.
- Per-character casting access shared by UI and authoritative commands, including
  martial High Elf cantrips and distinct Cleric WIS/racial INT statistics.
- Twenty stable portraits: four preserved masters and sixteen new imagegen masters
  with prompts, offline padded small/large atlases and bounded runtime upload.

See the [implemented design and APIs](character_builds_design.md) for ownership,
counts, options, exact preset selections, errors and compatibility policy.

## Checks and evidence

| Check | Result |
| --- | --- |
| Full Debug CTest | 34/34, 61.68 seconds |
| Full Release CTest | 34/34, 6.67 seconds |
| ASan + UBSan | 5/5: portraits, creation, library, spells, party_content |
| Allocation guard | Zero tracked engine-owned C++ frame allocations; Debug and Release |
| Hidden GPU lifecycle | Passed, including prepared portrait load/draw and resource teardown |
| Build/portrait captures | 225 at 1920×1080, 1280×720 and letterboxed 1024×768 |
| Spell migration captures | Existing 482-case image set completed with legal explicit selections |
| Production atlas memory | 40 MiB resident RGBA; bake peak RSS 63,684 KiB (~62 MiB) |
| Headless shipped-catalog diagnostic | All 16 race/class pairs, library round trip, independent parties, three kits |

Creation coverage includes all race/class combinations and supported gear choices,
point-cost transitions and budgets, invalid portraits/features/gear, overweight
derivation, learned/prepared/domain/racial constraints, direct forged-command
rejection, no-cost martial/cleric racial casts, and independent party state.
Persistence tests use isolated temporary files and cover duplicate names/UUIDs,
capacity, empty/first-run distinction, corrupt/truncated/oversized/unsupported
documents, missing content, stale writers/revisions, locks, entropy failure and
collision exhaustion, failed writes/flush/backup/replace/delete, interrupted
temporary files, and committed-but-durability-uncertain results.

Portrait tests generate their own twenty inputs and verify every region, padded
edge, atlas dimension and malformed/missing source rejection. The allocation test
exercises repeated previews, library lookup and mixed-source spell/sheet UI in
addition to existing frame paths. Filesystem transactions and scene construction
are explicit allocation-allowed phases. No claim is made about third-party/driver
allocation behavior. Sanitizers used `ASAN_OPTIONS=detect_leaks=0`; leak sanitizer
was not run. Existing aggregate-initializer warnings in `commands_tests.cpp` remain.
After the full runs, the final empty-build-ID guard and feature validation
regressions passed the affected creation/library/party-content suites in Debug,
Release and ASan/UBSan (3/3 each).

The memory check caught PNG encoder scratch exceeding the 96 MiB target. The final
baker serializes one atlas at a time to lossless QOI, whose largest encoder buffer
plus atlas is bounded at 72 MiB. Source PNG masters are unchanged; QOI upload
headers and encoded sizes are checked before decoding. The review PNG atlases
below show the identical crop pixels from the earlier PNG-format bake.

No OS mouse, keyboard, focus or desktop automation was used. Hidden in-process
captures and GPU readback passed. User acceptance is recorded above; no additional
interactive desktop verification is claimed by this closeout.

## Selected review images

- [All twenty small crops](screenshots/slice122/portraits-small.png),
  [all twenty tall crops](screenshots/slice122/portraits-large.png),
  [stable IDs and crop manifest](screenshots/slice122/portraits.tsv).
- [Original portraits in reordered formation](screenshots/slice122/builds-0-hud.png).
- [New elf portraits in the HUD](screenshots/slice122/builds-4-hud-1280.png).
- [Cleric WIS and racial INT sheet](screenshots/slice122/build-17-sheet.png).
- [Martial racial cantrip with Abilities tab](screenshots/slice122/build-16-spells-1280.png).
- [Quarterstaff inventory and three launch kits](screenshots/slice122/build-19-inventory-4x3.png).
- [Spell recipient chips](screenshots/slice122/builds-4-recipients-1280.png),
  [loot recipient portrait](screenshots/slice122/builds-4-loot-4x3.png).

All twenty masters and both crop sets were inspected. Selected native, reduced
and letterboxed UI captures were inspected for cropping, identity, labels and
overlap. New portraits retain varied adult faces/clothing; the original four keep
their existing pixel-art treatment. Portrait polish is still separately planned
in slice 15. The user accepted this slice, including its delivered portraits, on
2026-10-29.

## Reproduction

```sh
cmake --build cmake-build-debug -j4
ctest --test-dir cmake-build-debug --output-on-failure -j4
cmake --build cmake-build-release -j4
ctest --test-dir cmake-build-release --output-on-failure -j4
cmake-build-release/blobber_build_diagnostic assets/rules/catalog.json /tmp/blobber122-review/characters.json
cmake --build cmake-build-debug --target blobber_gpu_smoke -j4
cmake-build-debug/blobber_gpu_smoke
cmake-build-release/blobber --capture-builds /tmp/blobber122-build-captures
cmake-build-release/blobber --capture-spells /tmp/blobber122-spell-captures
```

The diagnostic only writes to its explicit library path. The normal game route
does not initialize a user library yet. Build diagnostics and captures are separate
from routine fixture-owned CTest. GPU/capture commands require a usable display but
create hidden windows and use only in-process scripted control.

Sanitizer build `/tmp/blobber122-asan` uses local pinned dependency sources,
`-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie` and linker
`-fsanitize=address,undefined -no-pie`. Final logs are under `/tmp/blobber122-*`;
the retained images and this record are the durable evidence.

## Next-agent handoff

Slice 12.2 is accepted and closed. Plan only 12.3 in a fresh context using this
design, the roadmap, approved menu/library visual references and relevant code.
Its prerequisite is complete; no outstanding 12.2 review blocker remains. Obtain
approval of the 12.3 plan before implementation. This documentation closeout does
not start 12.3 or change the recorded verification limitations.

- Use `CharacterLibrary::initialize` at the future menu's explicit loading phase;
  distinguish all errors and `committed`, especially durability uncertainty.
  There is no automatic backup restore. Preserve `.bak`, `.tmp`, `.bak.tmp` and
  `.lock` artifacts when recovery is required; recovery is an explicit user action.
- Default storage is absolute `$XDG_DATA_HOME/blobber/characters.json`, otherwise
  `$HOME/.local/share/blobber/characters.json`. Path overrides exist for diagnostics.
  The current filesystem backend is POSIX; other platforms need equivalent APIs.
- Cache fixed library/draft snapshots during menu frames. Disk CRUD and owning
  build/session construction happen only on explicit transitions, not per frame.
- Select four distinct saved UUIDs, pass a formation permutation to `startingParty`,
  then construct the explicit `GameSession`. Keep Map/Catalog alive for all borrowers.
  Destroy UI/session borrowers before replacing a run. Prepared resources can persist.
- Never derive portrait from class, slot or saved-record index. `PortraitId` resolves
  to stable metadata; runtime art IDs are 64–83. Unknown presentation IDs fall back;
  unknown build/library portraits reject. Source masters/provenance live in
  `assets/ui/portraits/`; existing four masters remain at `assets/ui/`.
- Persist stable content and gear-option IDs, not catalog indices. Keep schema 1
  blueprints separate from future run saves. Strict creation validation must stay
  on every library write and fresh party launch.
- Prepared/learned/domain/racial masks are separate. `Party::spellAccess` is the
  common UI/command authority; do not restore class-wide casting access. Capture
  scenarios use `spellCaptureParty` to construct legal requested selections.
- Picker capacity is now 38; other combat/effect capacities remain unchanged.
  Rest repreparation, skills, XP progression and session persistence remain deferred.
