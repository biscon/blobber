# Slice 13.1: Headless arena sessions

Status: implemented, **Accepted and closed**, 2026-10-30. User approved implementation
with “Implement the plan.” [Protocol/client guide](headless_protocol.md) and
[verification/handoff](slice131_verification.md) describe the delivered interface.
The user accepted the slice on 2026-10-30: “slice accepted, update the docs.”
The [automation roadmap](automation_plan.md) retains 13.2/13.3 as Planned; 13.2 is next
eligible for separate planning. Verification scope and limitations remain unchanged.

## Responsibilities and ownership

`blobber_headless` adapts versioned JSON lines to a reusable session host. The
host owns the arena scenario, map, placements and live GameSession in lifetime
order, borrowing the immutable catalog and NPC definition metadata. Replacement
is transactional: construct before swapping; old sessions and handles expire only
on success. One process hosts one session, with one request executing at a time.
No alternate rules, AI, arbitrary state mutation or development XP interface.

A graphics-independent NPC definition parser validates metadata once, retaining
image references without decoding. Existing image loading/baking consumes these
same records. Preserve ordering, timings and definition fingerprints. A graphics-
disabled CMake configuration excludes raylib discovery/download and all image
baking/copy targets while retaining CPU tests and the runner. Both builds include CPU world visual observations; normal graphics builds add
only GPU replay capture. Observations do not create a window. Capture explicitly
owns a hidden context and uses regular GameView/Presenter.

Diagnostic mode uses authoritative core validation/full observations. Visual mode
uses immutable world publication alpha masks, NpcAnimator and the CPU depth/ID
pipeline at observation/action boundaries, sharing UI target-visibility validation.
There is no selector or fallback. Hidden enemies/private RNG/pre-impact outcomes
remain filtered; inspect is separately diagnostic. Strict replay retains world
visibility/publication identities. The renderer_fixture public showcase is retired;
CPU/GPU test-owned height fixtures retain its regression coverage.

## Interface and time

CLI: `blobber_headless --assets DIR --mode diagnostic|visual`, optional `--record`
and `--replay`. Standard output is protocol JSON only; diagnostics use stderr.
Requests have protocol version and request ID; responses echo the ID. Event chunks
produced by advancement precede its final response. 64-bit identities/revisions
are strings. Content uses stable IDs; runtime references are session-scoped with
slot generations retained. Mutations require matching session and observation
revision. Failed parsing never mutates gameplay.

Operations: start/reset, observe/inspect, actions/preview, act, advance and close.
Start accepts the existing versioned arena scenario document; reset retains it
and may override seed. Starts stay level one. Act covers current movement, combat,
spells/features/items/End Turn, inventory/equipment, drop/pickup, rest/preparation,
earned advancement and partial-victory continuation through existing checked APIs.
Action descriptors/candidate targets are bounded; complete combinations use preview.
Movement during transitions rejects rather than queuing. No key emulation.

Advance uses versioned 10 ms steps with movement animation/timing enabled and no
wall-clock sleeps. Time between requests is frozen. Explicit ticks permit waiting;
decision mode stops on an awaiting party turn, settled real-time phase, or result.
Partial victory requires continue; full victory/defeat remain terminal until reset.
Comparisons with normal orchestration use the same timing schedule; arbitrary
frame-rate equivalence is not promised. Rest uses existing gameplay clock rules.

Limits: 2 MiB per request, JSON nesting 32, 1,000 ticks/request, 360,000 ticks/run,
100,000 mutating requests/run, 256 MiB trace file (including resets), event chunks at most 256.
Exhaustion reports reason and actual progress, never silently truncates events.
Session and typed observation storage are prepared/reused; protocol serialization
and trace I/O are diagnostics outside the allocation-free simulation boundary.

## Recording and replay

Stream initial scenario, seed, mode, build/content/asset identities, protocol and
timing versions, ordered requests, actual tick counts, normalized events/results
and boundary state. Consume borrowed event batches before any subsequent session
operation. Full trace events are diagnostic, distinct from filtered player output.
Normalize process-local owner/world identities, preserving slot generations and
serial relationships. Reconstruction from the beginning compares results/events
and normalized state and reports the first differing field. No memory snapshots
or save-game format. Strict compatibility; no cross-build/platform promise.

Trace envelopes reserve two extra nesting levels and 4 KiB above the request byte
bound, so a maximum-sized/depth rejected request remains replayable. This does not
increase accepted request limits. Files are flushed record by record, not retained
in memory; the completion footer indicates an orderly recording end, not victory.
Never overwrite traces. Record failures stop further simulation; missing footers
are incomplete, not successful replays. Stream rather than accumulating history.
Reads/previews do not advance simulation; inspection use remains in provenance.

## Implemented module boundaries

- `assets/npc_metadata.*`: bounded JSON metadata/definition loading, timings,
  attachments and canonical fingerprint. `npc_loader.*` adapts those records to image
  decoding for the existing offline preparation/rendering path.
- `automation/session.*`: typed fixed-shape actions/observations and arena ownership.
  The protocol enforces run/request budgets; trusted typed callers own their stepping
  budget. Borrowed catalog/definitions and visual context must outlive the session.
- `automation/world_visual.*`: graphics-independent world depth/ID adapter. It samples
  absolute simulation time and reuses cached targeting/loot membership when the frame
  has not changed. No extra window, image-export or presentation layer.
- `automation/decode.*`, `observe.*`, `actions.*`, `protocol.*`: bounded input schema,
  player/diagnostic encoding, descriptor discovery and transactional request handling.
  Ground zones are currently party-created and therefore player-known. Discovery
  exposes candidate targets and selection bounds; preview validates full combinations.
- `automation/trace.*`: exclusive bounded streaming recording and strict replay.
  Normalized boundaries include observable simulation/effect/AI state, not a private
  memory/RNG snapshot; replay re-executes the ordered requests from the initial seed.
- `tools/headless.cpp`: startup/CLI orchestration; `tools/headless_client.py`: synchronous
  stdlib client and script adapter. No policy, MCP server or worker pool.

## Validation and delivery

Fixture-owned tests cover protocols, stale references, atomic replacement,
limits/failure paths, complete wins/losses, approach/movement/partial victories,
combat/spells/management/advancement and timing. Replay tests include fresh process,
reset, changed inputs/content and truncated traces. Visual tests compare shared UI
validation/picking with walls, alpha gaps, overlapping living/dead actors, movement
and hidden/pre-impact observation filtering. Allocation guards cover prepared
simulation, validation, event staging and typed observation collection.

Debug/Release, focused sanitizers, allocation guards, a separate 256-actor
capacity run, clean graphics-disabled builds and both modes without DISPLAY were checked;
see the handoff for exact scopes and results.
Shipped assets and optional GPU integration remain separate from routine fixtures.
No desktop input automation. The scripted client/replay review recipe and roadmap/handoff are delivered.
The user subsequently accepted and closed 13.1. Slice 13.2 remains unstarted.
