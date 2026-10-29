# Slice 12.5: Arena

Status: implemented, **accepted and closed** (2026-10-29). The user accepted the
delivered arena slice. Slice 13 is next eligible for separate planning and remains
unstarted; creature-size mapping is still deferred.

## Ownership and flow

Main Menu → Arena Setup → shared gameplay → Rematch or Edit Setup. Setup retains
four independent level-one starting builds in formation order, ordered monster
quantities and a uint32 seed. The existing picker supports owned snapshot slots as
well as saved/default characters; its creator return contract is preserved. No
library mutation occurs when a scenario is loaded or played. Fresh sessions grant
the ordinary starting resources/supplies. Rematch reconstructs the immutable launch
snapshot; partial encounter victories continue normally until the roster is dead.
Pause retry/edit actions confirm abandoning an active attempt. Input edges remain
suppressed across transitions.

The application owns setup, scenario I/O, catalog and prepared resources. A launch
configuration explicitly supplies map, spawn, placements, recovery, party, seed and
return context to the existing gameplay loop. Borrowing sessions die before these
owners. NPC definition metadata loading is independent of authored placements and
retains definition ordering/fingerprints and offline preparation.

## Data and placement

ArenaScenario owns four build snapshots, stable monster IDs/quantities, a name,
seed and format/layout versions. Persistent data uses content IDs, not catalog
indices or library references. Shared character codec validation preserves the
existing library format. Scenario files are standalone JSON in the user-data
scenarios directory: 128 listed files, 1 MiB per file, checked atomic replacement.
Load failures retain the draft; writes never automatically repair corrupt files.
Sharing is file copying; the UI displays the path and can refresh the list.

Arena generation creates a 21×25 enclosed open room. Party starts at (10,21), north;
monster placement starts at (10,7), south, filling existing subcells four per cell,
then deterministic cardinal BFS cells. Maximum 256 actors, 32 monster types.
Validation checks occupancy, floor connectivity and all initial detection ranges.
Normal guard AI provides preparation time without immunity or forced combat.
Geometry is fixed; seed controls ordinary simulation randomness. Reproduction
requires matching game content and actions.

Creature-size mapping is deferred. Scenarios contain no positions or footprints;
the placement component owns the current one-subcell packing. Future multi-subcell
creatures require coordinated occupancy, movement, targeting and rendering work,
not merely a different arena packing rule. No speculative footprint framework here.

## UI and bounded work

Native existing menu theme, searchable catalog, combat stats, quantity controls,
party summary/picker, editable seed, save/save-as/load, exit and start. Four distinct
identities and 1–256 monsters required. Unavailable appearances cannot be selected.
Setup and list state are bounded; ordinary input/rendering performs no engine-owned
heap allocation. Explicit file I/O, generation, session preparation and diagnostics
may allocate. Prepared-image rendering and fixed presentation remain unchanged.

## Validation

Fixture-owned headless placement, limits, safe spawn, AI/openers, result lifecycle,
retry/reset, snapshot picker and persistence round-trip/failure checks. Full Debug
and Release suites, focused sanitizers/allocation checks; maximum roster stress
separate from routine tests. Hidden application-owned captures and GPU checks when
available; no OS input automation. Verification and handoff are recorded below;
the user subsequently accepted and closed the slice on 2026-10-29.

## Implemented interfaces and storage

`ArenaScenario` holds four fixed StartingSnapshot records (128-byte content IDs and
validated CharacterDraft), up to 32 ordered monster quantities, a 64-byte name and
uint32 seed. `arenaMap`, `arenaSpawn` and `arenaPlacements` prepare immutable world
data. `arenaCleared` checks authoritative death markers for the entire roster.

`ArenaSetup` owns the fixed UI state, available/filtered catalog indices and the
128-entry scenario listing. `MenuModel` accepts saved, preset and owned snapshot
choices, compares their stable identities and resolves an owning launch party.
`ArenaScenarioStore` owns only a catalog reference, directory and optional diagnostic
fault hooks. It performs bounded POSIX regular-file reads, per-directory flock,
pre-publication write/fsync/close, hard-link no-clobber creation or checked rename,
and directory fsync. The return value distinguishes a committed durability warning.
No automatic corrupt-file repair or silent capacity truncation occurs.

Files live beside the character library in `scenarios/` (normally
`$XDG_DATA_HOME/blobber/scenarios`, or `$HOME/.local/share/blobber/scenarios`).
Filenames are independently generated; display names need not be unique. The shared
starting-draft codec preserves library version 1 while resolving scenario content
by stable IDs. Snapshot identity is preserved but is not a library lookup.

`loadNpcDefinitions` reads definition metadata without authored placements. The
shared launcher builds the map/placements/recovery and prepared banks, then supplies
explicit spawn/seed/AI/destination policy through GameplayLaunch. `runGameplay`
returns a typed PlayOutcome and reconstructs from the original launch data for retry.
No additional GPU textures or changes to preparation budgets are needed.

See [verification, limitations and review recipe](slice125_verification.md).
