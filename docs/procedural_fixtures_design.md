# PC02: doors, keys and discovered navigation

Status: **Accepted and closed**, 2026-10-30.
User approval: “Implement the plan.” User acceptance (2026-10-30): “okay in that case
I accept the slice, ive tested as far as I could and it is working, update the doc”.
PC01 remains accepted and closed; PC03 is not started. The user tested the available
gameplay and accepts the lack of traversal in this slice. Basic door visuals remain
a known quality limitation; this acceptance does not authorize a renderer redesign.

## Ownership and geometry

Generated Map and fixture definitions remain immutable. GameSession owns FloorGeometry
before all its borrowers. A small GeometryView borrows the base Map and optional
fixture owner; runtime consumers take this value view, while existing empty-fixture
maps have the same query behavior. There is no virtual dispatch or copied collision map.
FloorGeometry owns 32 fixed fixture slots and a preparation-allocated contiguous
cell-to-slot index. Slots never recycle; floor lifetime identities reject stale handles.
Closed doors are solid full cells, opened doors never close. Geometry revision changes
only on opening; fixture revision also changes on key collection. Revisions cannot wrap.
All movement, actors, BFS, perception, attacks/areas, rendering and picking share this
truth. Navigation cancels stale searches/results and cleans under its existing budget;
frames/picking include geometry and fixture identity/revisions, including stationary changes.

Party-wide acquired quest keys are fixed bits independent of inventory/weight and
remain acquired after opening their one designated lock. PC04 will move this record
into campaign ownership. Fixture mutation is only through checked session commands:
current-cell or directly-ahead keys, directly-ahead doors, stationary settled safe
exploration, no party-cell area hazard, and at least one able companion. No RNG,
turn/item cost or closing; successful operations clear queued movement. Preview and
rejection are atomic. Repeated completed requests do not repeat effects. No NPC opening.

## Generated definitions and structural validation

Exactly one door occupies the corridor entering Approach. Depth 1 is unlocked; depths
2–3 each have one matching key in a cycle room upstream of the lock. Choose the cycle
room maximizing minimum graph distance from entrance/approach attachments, excluding
those attachments, ties by stable room ID. Preserve the optional branches. Entrance
markers are upstairs; depths 1–2 have downstairs and depth 3 an inert endpoint marker.
No travel, guardian, victory, population or balance changes occur here.
Fixture cells/orientations rotate with layouts and enter the fingerprint; generator/
template versions are **2/2** (scenario schema remains 1). Validate final tiles with bounded (cell,key,door-open) reachability
using shared passability/interaction rules, plus explicit no-key bypass checks. Keys,
locks, approach and return must satisfy the same guarantees in fallbacks. Reject
malformed IDs/associations, overlaps and unusable interaction positions. Retain 16
attempts, 16 KiB retained-definition and 128 KiB scratch limits.

## Discovery, UI and prepared imagery

Generated sessions own bounded discovered-cell bits and remembered fixture state.
Reveal current cell and six-cell facing sight using supercover occlusion, allowing a
blocking target cell to be seen but never revealing beyond it. Update on initialization,
settled moves/turns and successful interaction; observations/rendering never reveal.
M displays remembered terrain/fixtures, unknown space, party/facing and acquired keys.
A development-only full-map toggle is explicitly diagnostic and never writes discovery.
Reset clears both. Maps retain existing pause/modal behavior.

F and visible-fixture clicks route through the same checked command; modal and existing
spell/item targeting take precedence. Fixed HUD prompts/logs identify matching key/lock.
Closed doors use prepared wall projection columns with two offline colour planes
(unlocked and sealed), sharing column geometry. Open thresholds and key/stair/endpoint
markers extend the existing loot bank to seven sprite kinds (including ordinary loot).
Authored SVG masters and raster PNG inputs live in `assets/textures/` and `assets/fixtures/`. Shared depth and
picking honor nearer walls, actors and ground objects, with no runtime raster preparation.
The initial sample was visually reviewed before final capture verification. Prepared
format/renderer revisions are **4/4**; the 512 MiB limits, fixed 1920×1080 and shared
banks remain. The marker loader has a 4 MiB budget; each door plane has the existing
wall-plane pixel cap, with all planes included in scene accounting. Preparation/loading may allocate;
ordinary discovery, interactions, queries, simulation, UI and composition may not.

## Automation and verification

Add interact preview/action through existing typed host, JSON client and MCP transport.
Player observations expose only discovered cells, remembered fixtures, acquired keys
and legitimate interactions; unknown guessed IDs have indistinguishable errors.
Diagnostic inspection cannot contaminate player state. Strict normalized replay includes
fixtures/discovery and generation versions; arena behavior stays supported.

Owned fixtures cover closed/open geometry across every consumer, incremental BFS
invalidation, stationary render/pick refresh, all orientations/strafe/occlusion, transaction
failure/preview/repeat cases, full inventory, stale/reset IDs, safe phase/hazard/ability,
key-behind-lock/bypass/fallback defects, facing/corner discovery and diagnostic isolation.
Run Debug/Release headless and graphics suites, focused lifetime sanitizers, allocation
checks, prepared-bank integrity and small deterministic capture/replay checks. Measure
bounded storage and preparation memory. Hidden application-owned GPU/capture checks
are permitted; OS input/focus control is prohibited. Interactive review is the user’s.
No balance experiments, broad seed sweeps, travel, save, hub or PC03 work.

## Delivery

Preserve all entry working-tree changes. Record exact executed commands, outcomes,
limitations and retained review captures in a durable handoff. Update checkpoint,
tracker and overview together. Implementation stopped at Ready for review; the user
has now accepted PC02. See the handoff for the recorded closeout and next planning step.

## Implemented boundaries and storage

`src/world/fixtures.*` defines the immutable definitions, read-only geometry borrow,
fixed state and identity. `src/game/session_fixtures.cpp` owns command validation
and pose-driven discovery. `src/generation/fixtures.cpp` places and validates the
gates. Navigation and AI discard stale geometry results; FramePipeline includes
fixture identity/revisions so stationary mutation regenerates colour and picking.
Queued movement reveals each completed endpoint before beginning the next transition.

The discovery scan visits at most 169 candidate cells per settled pose/fixture change,
with bounded six-cell supercover rays. It stores 1,600 discovery bits, three fixed
32-entry state arrays and a byte-per-cell lookup allocated at floor construction.
On this build FloorGeometry is 888 bytes plus 1,296 index bytes for a generated floor.
The generator retains 4,240 bytes including Map payload (4,544 at the hard cell cap),
and accounts for 32,528 scratch bytes, including the four-state fixture search.
The latter is 6,400 queue/visited states with no per-node allocation.
Capacity and malformed definition failures are explicit; no slot recycling or
implicit runtime container growth is supported. Floor identity and mutation revision
exhaustion fail; discovery revision saturates without wrapping.

[Verification/handoff](pc02_verification.md) records executed checks, measured prepared
bytes, review artifacts and the interactive-review limitation. Future campaign state
transfer, guardian gates and saved floor state remain separate reviewed slices.
