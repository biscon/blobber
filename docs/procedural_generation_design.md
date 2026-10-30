# PC01: generated-floor walking prototype

Status: **Implemented; Accepted and closed**, 2026-10-30. User authorized
implementation with “Implement the plan” and accepted with “slice accepted, record
it in the doc”. PC02 is next eligible for separate planning; later implementation
is not authorized.

## Responsibilities and ownership

A graphics-independent generation subsystem owns bounded topology/template records,
integer seed derivation, stamping, final-tile validation and diagnostic results.
The immutable published floor owns Map plus room/connection/role metadata. Application
launch and automation retain that owner until session, animator, commands and frames
are destroyed. GameSession remains the existing owner of the party and simulation;
no campaign Party transfer or persistent-floor system is introduced.

New Game uses the existing party picker and launches an empty depth-1 floor. Explicit
seed overrides reproduce it; otherwise one fresh seed is chosen before preparation.
Restart retains floor and starting party. Existing supplies and formation remain.
NPC definitions and prepared banks are shared facilities, but normal launch no
longer loads static placements/recovery. Authored diagnostic maps and bake inputs
remain until their actual consumers are migrated. Preparation failure preserves
menu selection/library. Arena remains supported.

## Layout and algorithm

Use a 4×4 lattice of 7×7 room footprints with pitch nine, in an enclosed 36×36 grid
(hard capacity 40×40). Six- and eight-room cycles carry an entrance, approach/objective
tail and one/two optional branches, within 8–12 rooms. Chamber, pillar, bent passage
and alcove interiors have cardinal ports; rotations transform cells, anchors and
facing together. All connections are explicit neighboring-site edges. Optional
branches remain upstream of the provisional objective. Final-tile validation checks
bounds, connectivity, port correspondence, accidental connections, anchors and roles.
A useful loop must retain connectivity when one connection is closed and increase
its endpoint shortest route by at least eight steps. Approach/objective anchors
have clear surrounding space. No keys/doors/actors/rewards are created; no endpoint
causes victory. Depths 1–3 distinguish generation identity only in PC01.

SplitMix64 supplies stable domain-separated seeds and integer draws. Multiply-high
bounded sampling has fixed cost; no unordered iteration or standard distributions.
Topology, interiors, future encounter/reward and runtime seeds are independent;
per-room draws use stable IDs. Fingerprints use canonical little-endian fields and
FNV-1a, never padding or timings. Versions and topology/cells are fingerprinted;
seed provenance is separate so fallback/diversity comparisons remain meaningful.

At most 16 candidates precede one authored fallback per depth. The same validator
checks fallback; failure never weakens invariants or publishes a partial floor.
Bounded diagnostics retain rejection code/location per attempt and fallback identity.
Malformed requests/catalogs reject explicitly. Generator caps: 1,600 cells, 16 rooms,
24 connections; retained floor data budget 16 KiB, scratch budget 128 KiB excluding
existing session/render resources. Preparation may allocate; ordinary frame work
reuses immutable metadata and cached diagnostic strings. Actual sizes are recorded
in the verification handoff.

## Interfaces and investigation

GenerationRequest, GeneratedFloor and GenerationDiagnostics are typed core records.
A diagnostic CLI emits ASCII/JSON single-floor reports and explicit streaming sweeps
(maximum 100,000; checked seed range; never part of routine tests). Forced fallback
is diagnostic/test-only. Output includes provenance, fingerprint, topology, rejection
reasons, fallback and timings. The full-map overlay is explicitly diagnostic and
shows provisional role markers; this is not player discovery.

Automation accepts an explicitly tagged generated_floor scenario using the existing
four starting-draft codec, seed, depth and versions. Untagged scenarios remain arenas.
The normal checked actions, 10 ms ticks, limits, transactional start/reset and stale
identities remain authoritative. Empty floors are nonterminal. Normalized replay
state contains generation identity; only diagnostic observations expose topology,
role coordinates and rejection details. Replay capture selects arena HUD context only
for arena sessions. Client/MCP reuse current lifecycle; no new policy or balance mode.

## Validation and acceptance

Dedicated fixtures cover deterministic output/domain separation/all rotations,
structural defects, meaningful loops, optional routes, bounds, forced exhaustion,
fallback failure, runtime movement, launch/reset lifetimes, stale tokens, replay and
visual metadata filtering. Small fixed seed sets establish variation, not balance.
Generated materials exercise room/strafe/occlusion rendering. Allocation guards begin
after preparation. Run focused Debug/Release headless/graphics and sanitizer checks;
record actual commands/results. Hidden application-owned captures may be exported;
never control OS input/focus. Interactive desktop review remains the user's.

No balance experiments or broad seed sweeps are authorized. Deliver two retained
layouts, movement replay, fallback and malformed-template evidence, memory accounting,
render checks and arena regressions. PC01 is accepted; PC02 still requires its own detailed plan review before implementation. No travel, discovery, doors, keys, saves or tuned population.

## Concrete implementation notes

The bounded families currently emit 10–12 rooms within the approved 8–12 admission
range. Templates are compiled records; final cells remain immutable. Optional
branches are leaves, entrance is a leaf, and approach/objective form a two-room tail.
Cycle closure checks compare actual tile distances and whole-floor connectivity.
Every room interior is restricted to its footprint and every external opening must
belong to a declared neighboring connection. No graph-only acceptance shortcut.

The automation `Scenario` is a value variant of arena and generated starting data.
A generated session borrows the Map inside its owned GeneratedFloor; an arena owns
its Map separately. Destruction order destroys visibility/simulation/placements
before the borrowed map. StartingParty construction is shared with arena instead
of fabricating an empty arena roster. No live Party ownership changes were needed.

`DiagnosticView` caches fixed strings and role markers at preparation time. Normal
frames only borrow it. A forced fallback is available through the inspection CLI
and diagnostic scenario `force_fallback`; visual mode rejects that input. Replay
capture emits a diagnostic map companion while preserving observer immutability and
the aggregate capture byte limit. It does not reconstruct historical UI interactions.

The inspection CLI's multi-seed aggregation retains at most 100,000 uint64 layout
fingerprints (800,000 bytes of payload), then sorts/deduplicates them. Per-floor
reports stream; no unbounded history is retained. Timing and stdout JSON allocation
are explicit diagnostic work, outside the generator/frame storage guarantee.

[Verification and next-agent handoff](pc01_verification.md) records executed checks,
retained captures, storage measurements and limitations. User acceptance was recorded
on 2026-10-30; the verification scope and limitations are unchanged.

## PC02 extension (2026-10-30)

The accepted PC01 record above describes its original 1/1 generator. PC02 adds
immutable fixture definitions and tile-state lock/key validation, and advances
generator/template versions to 2/2. See [fixture design](procedural_fixtures_design.md)
for the current runtime geometry, discovery and memory contracts. PC01 acceptance
and its historical verification remain unchanged.
