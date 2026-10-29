# Slice 12.5 verification and handoff

Status: **Accepted and closed**, 2026-10-29. Implementation was authorized
by “Implement the plan.” The user subsequently said “slice accepted, record it in
the doc”. Slice 13 has not started.

## Delivered

Main Menu → Arena provides a named setup with the default party, reusable saved
character picker/creator and formation dragging, monster search/quantities, seed
editing, Save/Save As/Load and Start. Setup survives returning from a fight or
visiting Main Menu. Four distinct level-one identities and 1–256 monsters are
required; unavailable NPC appearances cannot be selected. No new image masters or
GPU atlas slots are used.

Runs use the normal GameSession, AI, spell/combat, inventory, loot, recovery and
prepared rendering. The generated 21×25 room has four-per-cell deterministic
packing and a distant party spawn. Guards remain outside detection until approached
or alerted through normal attacks. Rematch restores starting characters, formation,
roster, seed and resources. Edit Setup returns to the preserved draft; active-attempt
retry/edit actions require confirmation. Partial encounter victories retain Continue;
only clearing the whole roster produces the final arena result.

Scenario JSON files contain independent validated character snapshots and stable
monster IDs/quantities, name, seed and versioned layout/format. They contain no
library references, runtime state or fixed placements. Load/Save screens show the
user-data scenarios folder for manual file sharing and support Refresh. Files use
bounded decoding, checked replacement, no-clobber new-file publication, explicit
pre-commit failures and committed durability warnings. The library schema is unchanged.

## Verification

- Full Debug and Release CTest: **38/38 each**. Final logs:
  `/tmp/blobber125-final-debug-tests.log` and
  `/tmp/blobber125-final-release-tests.log`.
- Focused ASan/UBSan with leak detection: **5/5** (`arena`, `library`, `menu`,
  `npc`, `creator`); `/tmp/blobber125-final-sanitize-tests.log`. Dedicated build:
  `/tmp/blobber125-sanitize`.
- Allocation guard: **zero tracked engine-owned C++ allocations**, including arena
  filtering/validation, quantity editing, snapshot-picker manipulation and live
  arena prep updates, alongside existing frame/render/UI checks. This does not
  assert that drivers or third-party C allocation never allocate.
- Fixture-owned arena tests cover 1/4/5 placements, mixed types, stable ordering,
  unique subcells, safe idle preparation, ordinary approach/ranged/spell openers,
  preparation spells, whole-roster completion, fresh session resources, confirmed
  retry routing, snapshot formation/identity swaps, seed/count bounds and filtering.
- Persistence tests cover portable round trips between independent temporary
  stores, strict schema/content/build validation, duplicates, stale reads/writes,
  pre-commit write/flush/publication failures, committed directory-flush warnings,
  corrupt-file listing, filename traversal and 128-file capacity. NPC metadata tests
  prove arena definition loading ignores invalid authored placements while retaining
  the same prepared-bank identity; the dungeon loader still rejects those placements.
- Separate Release maximum-roster arena stress passes (256 actors). Existing AI
  stress also passes at 25×25 and 1024×1024 with 256 actors, approximately 2 ms and
  6.0 s respectively on this machine. Logs: `/tmp/blobber125-arena-stress.log` and
  `/tmp/blobber125-ai-stress.log`. These are selected capacity/workload checks, not
  a promise about every future monster's frame cost.
- Hidden raylib/OpenGL GPU lifecycle checks pass on the NVIDIA RTX 3060, including
  the existing eight-texture-slot lifecycle checks; `/tmp/blobber125-gpu.log`.
- **54 application-owned captures** cover 18 states at native 1920×1080, reduced
  1280×720 and letterboxed 1024×768. [Gallery](screenshots/slice125/index.html).
  Selected native/reduced/letterboxed setup and result images were inspected.
  Initial spawn and fresh rematch images match byte-for-byte (SHA-256
  `9373ef760238da030e6622a5b0f8446228450ff356cb2978c27aa00571f285a7`).
  Captures exercise production setup actions, scenario save/load, prepared arena
  launch, approach, victory, defeat and reconstruction using a temporary library
  and scenario directory, removed on exit. No OS input or window-focus control.

The first defeat capture attempted to kill the party before an encounter existed;
it did not reach the normal combat result phase. The fixture now enters combat
before applying its deterministic defeat. No alternate arena defeat rule was added.
Existing test-only missing-field-initializer warnings in commands_tests remain.
Deterministic captures and headless checks do not establish encounter balance or
exhaustive interaction coverage. User acceptance does not imply additional automated
or exhaustive interactive testing.

## Review recipe

1. Run `cmake-build-debug/blobber`, select **Arena**, add four goblins and one archer.
2. Choose party/formation, swap slots, select a saved character, and return with
   **Use party**. Creating a character returns to this picker without auto-assigning it.
3. Name the scenario, choose a seed, Save, then Load it. Copy its JSON file from the
   displayed folder to another installation's scenario folder and Refresh to share.
4. Start. Prepare spells while distant, then advance toward the north cluster or
   try an attack opener. A fifth monster occupies an adjacent cell.
5. Pause → Rematch → confirm. The original party/resources/spawn return. Pause →
   Edit Setup → confirm preserves the setup. Victory/defeat offer Rematch/Edit Setup.
6. Exit the setup to Main Menu and verify normal Start new game still works.

Headless: `ctest --test-dir cmake-build-debug --output-on-failure`.
Capacity: `cmake-build-release/blobber_arena_tests --stress`.
Capture: `cmake-build-debug/blobber --capture-arena /tmp/arena-review`.

## Next-agent handoff

Read [arena design](arena_design.md) and the current roadmap checkpoint before
advancing. The application owns ArenaSetup and scenario I/O; immutable launch data
outlives the borrowing session and prepared renderer. Snapshot choices extend the
existing picker without changing library ownership or the creator caller contract.
NPC definitions retain the baker's order/fingerprint; authored placements are
separate. Scenario format/layout are version 1; changing content or gameplay can
change seeded outcomes, and imported scenarios must validate against local content.

Level 2–3 advancement, arbitrary equipment editing, procedural floors/bosses and
creature footprints are deferred. In particular, scenarios have no serialized
subcell assignments: future size mapping must coordinate placement, occupancy,
movement, targeting and rendering. Do not implement it as an arena-only workaround.

## Accepted closeout and next-agent handoff

The user explicitly accepted slice 12.5 on 2026-10-29: “slice accepted, record it in
the doc”. This closes arena setup, default/saved party selection, portable scenario
files, generated-room battles and Rematch/Edit Setup. Level-one-only setup and the
deferred creature-size work remain as agreed.

Recorded verification results and limitations are unchanged. This acceptance update
changes documentation only; no implementation tests were rerun.

Slice 13 — Levels 2 and 3 — is next eligible for separate planning and remains
Planned/unstarted. Read this closeout, the system overview, the slice-13 roadmap
and relevant code before proposing its plan. Preserve the accepted arena, creator,
scenario and recovery contracts. Obtain approval of that plan before implementation;
this closeout does not start slice 13.
