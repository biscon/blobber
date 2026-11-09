# PC06: campaign save and resume

The [early equipment expansion](early_equipment_design.md) advances campaign
schema/simulation to **21/24**, generator/templates to **12/9**. The compact
envelope now carries the immutable initial-party reward profile so regeneration
preserves future rewards despite equipment changes and deaths. Baseline signatures
also bind reward identities. Inspection stays metadata-only and saving shares
already-prepared definitions. Final guardian completion is now valid active saved
progress; it does not publish a terminal result. Defeat still does. Old campaign
formats reject untouched without migration.

## Compact persistence — 2026-11-09

**Implemented and mechanically verified; interactive timing/user acceptance pending.**
Schema **20**, simulation **23**, generator/templates **11/8**. Older campaign
formats reject without migration or automatic deletion; explicit overwrite is
still required. Library and arena formats are unchanged.

### Baselines, ownership and disk data

`prepareCampaignLocations` supplies New Game and Load with the same deterministic
authored/generated definitions. `CampaignDefinitions` owns immutable preparation
values for the seven locations and actual-population actor baselines, using the
same `initialActorState` helper as ActorWorld. Each run shares its canonical and
actual definitions with `CampaignSaveData`; captures copy mutable values only.
They never copy map definitions or retain another set of dense floor baselines.
The definition caches are bounded by seven locations, 1,600 cells per location
and the actual population; they trade a small amount of preparation memory for
zero generation/reconstruction and no map copies at save time. Runtime and travel
pools remain dense. Captures, JSON storage and one reusable FloorState comparison
workspace allocate only at explicit persistence/preparation boundaries.

The envelope stores compatibility, campaign seed, baseline fingerprints and a
small menu summary. Canonical terrain, layouts, placements, recovery/fixture
and objective definitions inherit from the regenerated baseline. Unvisited
locations have no state records. Existing supported generated-world surface,
architecture and scenery overrides are changes against that baseline. Vectors
carry an explicit final length plus changed indices, supporting replacement,
append and removal without a full-map fallback. Generation diagnostics stay off
disk. Immutable definition admission uses typed comparisons instead of JSON dumps.

The explicit field list supplies typed defaults and canonical baselines. Arrays
are sorted `[slot, change]` records, vectors contain `size`/`entries`, bitsets
contain changed bit indices and optional values distinguish clears from absence.
Omitted fields inherit their baseline/default. Counted actor/AI, target, waypoint,
container, objective and dialogue arrays traverse only their meaningful prefix;
other pools emit only meaningful non-default slots. Empty inventory mappings
inherit UINT16_MAX. Inventory/loot generations, retired slots and container-owner
cells remain explicit even for empty slots. Expired effect/condition/area/restraint
and empty buyback payloads do not survive merely because a pool slot once held
something. Navigation retains pending/ready records, its meaningful queue prefix,
visited membership, first-step data and exact scheduling; unused workspace is
canonicalized during checkpoint export. Canonical floor diagnostic hashes now
ignore dormant payloads; historical raw diagnostic hashes are superseded.

Unknown fields, duplicate/unordered/out-of-range indices, logical-count overflow,
invalid vector sizes, content references, enum/numeric limits, baseline mismatch,
oversized input and excessive nesting reject transactionally. Current bounds
remain 16 MiB and nesting 32. Uint64 state remains lossless decimal text. RNG,
clocks, effect durations and subsequent simulation/navigation behavior are retained.

### Store and presentation

Save performs eligibility, owned capture, encoding and atomic publication; it
neither generates locations nor constructs CampaignRun/GameSession candidates.
Inspect reads compatible metadata only, with no simulation owners or generation.
It does not promise that an otherwise readable slot passes deeper logical checks:
Continue performs full admission and retains the current session/save on failure.
Load decodes once, regenerates baselines once, applies changes, then constructs
one replacement campaign. Each visited session is admitted once and the checked
active session is reused. No fresh starting party, supplies, rewards or live RNG
are used to validate a save.

Normal saves use the existing single worker with exclusive simulation access while
the window thread pumps cached Saving frames and audio. Terminal transitions first
present the result, immediately deny in-process Continue, then submit a small owned
ID/status request to the worker. Cached result frames remain visible during file
work; close is deferred without cancelling publication. CLI/MCP/replay use the same
codec/store and may run synchronously. Revision checks, locks, atomic rename,
file/directory fsync, overwrite confirmation, durability uncertainty, terminal
association and retry behavior remain intact.

### Verification — 2026-11-09

Configured CLion CMake/Ninja, Debug only, at most two low-priority build workers
and one test process. Focused integration selection was campaign persistence and
travel, CLI/MCP/strict replay, frame allocations and graphical startup. All passed:

- `ctest --test-dir cmake-build-debug -j 1 --output-on-failure -R '^(campaign_save|campaign|campaign_save_cli|automation_allocations|gameplay_startup)$'`:
  allocation guard 16.20 s, campaign 3.25 s, CLI/replay 1.28 s and startup 0.13 s.
  Initial persistence testing exposed a discarded empty chest-owner cell; corrected
  its encoding and canonical digest. The affected full `campaign_save` rerun passed
  in **7.49 s**, including chest depletion, effects, inactive floors, corruption,
  I/O faults, high/retired slots, partial navigation transport and actual victory/
  defeat worker/close tests. A final disabled-store guard was covered by
  `campaign_save_smoke` (2.12 s) and a Debug application rebuild. Passing unrelated
  suites were not repeated.
- New structural checks cover sparse pool families, nested counts, default/expired
  payload omission, geometry overrides and duplicate/capacity failures. Preparation
  probes verify that Save/Inspect never generate floors or construct campaign/session
  owners. The explicit seven-location measurement checks a 256 KiB/90%-reduction
  budget; timing is reported, never asserted by routine tests.
- Hidden `blobber_world_integration_gpu_tests cmake-build-debug/assets NEW_OUTPUT --campaign-save-review`
  passed: repeated Saving/result presentation during deliberately delayed worker
  work, non-cancellable close deferral, stable engine C++/GPU resource counts and
  no OpenGL errors. Both native 1920×1080 captures were visually inspected;
  [two selected images](screenshots/compact-saves/README.md) remain. No desktop input
  automation or interactive play review was performed.

One bounded Debug observation used the same owned low-occupancy seed-5 campaign
with all seven locations visited (two completed dungeon floors). No compression:

| Measurement | Schema 19 | Schema 20 |
| --- | ---: | ---: |
| Save bytes | 5,831,821 | 113,060 |
| Capture | 1.10 ms | 0.13 ms |
| Encode | 614.20 ms | 11.05 ms |
| Inspect plus ordinary overwrite | 4,595.95 ms | 42.90 ms |

The new atomic write/flush stage measured 21.36 ms and the complete save call
24.62 ms. These are single observations, affected by scheduling/filesystem flushes,
not latency guarantees. Size fell **98.06%**; the strict continuation/fault tests,
not the measurement, establish correctness. Debug game rebuilt. Disposable
captures, old codec copies, measurement saves and logs were cleaned after selecting
review evidence; no user saves were modified.

## Historical format and rollout notes

The sections below describe earlier deliveries and their superseded formats.

Historical paperdoll revision (2026-11-08): campaign schema/simulation **18/22**.
The [approved equipment extension](inventory_design.md#expanded-paperdoll-mockup--2026-11-08)
expands inventory storage to 61 locations per character and 244 stacks per party.
Older campaigns reject unchanged without migration; library/arena formats remain.

Historical movement revision (2026-11-08): campaign schema/simulation **16/20**.
[SRD combat movement](combat_positioning.md#srd-combat-movement--2026-11-08)
changes the catalog fingerprint and simulation rules, retaining strict rejection
of older campaigns without rewriting them. Tactical paths and active movement
budgets are transient because encounters cannot be saved. Arena scenarios retain
their existing format. Extended campaign persistence passes for this revision.

Status: **Accepted and closed**, 2026-10-30.
User approved the reviewed PC06-only plan with “Implement the plan”, then accepted
the delivered save/load slice on 2026-10-30. Review corrections are included.
Accepted PC04–PC06 complete milestone P-B; PC07–PC09 remain Planned. Verification
and limitations are unchanged; see the [acceptance handoff](procedural_campaign_plan.md#pc06-accepted-closeout-and-next-agent-handoff-2026-10-30-current).

## Ownership and checkpoint

CampaignRun exports owned preparation values, with checked Party/Inventory restore
that never constructs a starting party or grants supplies. One Party remains
authoritative. The checkpoint retains all four actual location definitions,
placements/recovery/rewards, immutable provenance, inactive snapshots and a live
active snapshot plus player pose/animation. Catalog references serialize by content
name; runtime owner/world epochs never serialize. Build identity/portrait/spells,
levels 1–3, inventory slots/generations/retirement, resources, formation, quest keys,
XP/death/loot/objective ledgers and discovery are independent of the library.

Import validates definitions, values and cross-owner references before constructing
replacement owners. All locations are checked, including unvisited definitions.
The active session retains exact clocks/RNG and navigation continuation (pending
requests/results, bounded queue and scheduling progress); rebuilt occupancy,
geometry/navigation/frame storage and fresh runtime epochs reject old requests.
Menus/loading/offline time advance no gameplay clocks. Import does not cast,
expire, reconcile rewards, heal or draw RNG. Inactive portable party effects remain
absent; local durations remain frozen. Existing travel restrictions still apply.

Save eligibility is a shared allocation-free query: active run, stationary with no
queued movement, settled safe Exploration without impacts/presentation or a party
cell area hazard. Local concentration and summons can save in place. Save/load and
view preparation explicitly allow allocation; steady frames and previews do not.

## Format and store

Strict JSON schema 1, simulation compatibility 1, declared generator/templates and
catalog/NPC fingerprints. Unsupported schemas/content fail without migrations.
Maximum 16 MiB, nesting 32, existing bounded capacities, exact numeric range checks,
no duplicate/unknown fields or invalid logical references. Uint64 counters use
lossless decimal strings. RNG text has bounded, exact state parsing.

One campaign.json alongside user data; explicit path override for fixtures/tools.
Persistent run ID and observed slot revision separate different runs. Same-directory
exclusive temp creation, complete write, fsync/close, atomic rename and directory
fsync publish under a writer lock with a final revision check. Pre-publication
failures preserve the old slot. Post-rename durability uncertainty is explicit.
Loads never repair/delete a bad file and stage simulation/view before publication.

Terminal status immediately denies in-process Continue for that run. The app/tool
attempts a matching-run terminal marker once at an explicit I/O boundary, never
invalidating a different run. Failure offers Retry and warns that restart can still
see the previous disk save; exiting with this warning requires acknowledgement.
No general autosave, copy prevention or slot migration framework.

## Presentation and adapters

Main Menu Continue bypasses party selection/library and shows validated slot
summary/errors. Pause offers Save and Save and Main Menu, confirmed leave without
saving and clear eligibility/error feedback. Save and Main Menu stays on failure
or uncertain durability. A new run leaves the previous slot until its first
confirmed successful save. Replacing different/unreadable slots is explicit and
revision-checked. Owner replacement clears transient input/targeting and suppresses
held input until release. Prepared image banks remain shared.

Headless save_status/preview_save/save/load reuse the same authority, with an
explicit configured slot path; absent path disables writes. Mutating requests bind
current session/revision; live load requires explicit replacement. New protocol/MCP
epochs invalidate stale requests. Python/MCP use isolated slots and bounded artifact
inputs/outputs. Trace persistence records capture external reads/results and run
IDs in 256 KiB chunks; replay uses memory rather than user files. Existing line and
256 MiB trace limits remain. MCP reserves trace + maximum slot + temp storage.
No expedition policy, balance run, seed sweep or later-slice content.

## Validation and delivery

Owned fixtures round-trip hub/all depths, unvisited definitions, non-entry pose,
advancement, inventory, keys/doors/guardians/loot/XP/discovery, recall/unlocks and all
effect/condition/readiness clocks. Compare uninterrupted continuation including RNG
and navigation. Malformed/oversized/incompatible/invalid ownership/clock/reference
inputs fail transactionally. Inject allocation/write/flush/close/rename/directory
flush/load/view/terminal failures; test retries, stale slots and cross-run isolation.
Cover UI click/keyboard/cancel/held input, removed library entries, fresh process,
CLI/client/local MCP, strict replay and corruption rejection.

Run Debug/Release headless/graphics suites, allocation guards after load, focused
ASan/UBSan lifetime checks and available hidden application-owned captures/GPU
checks. No OS input/focus automation. Record exact results/limits, save/staging
memory and review recipe in pc06_verification.md. Stop Ready for review; only the
user accepts PC06/P-B. No balance experiments are authorized.


## Implemented interfaces and limits

CampaignCheckpoint owns Party::Saved, four CampaignLocationState values, active
pose/animation/navigation and campaign clock/revision/run ID. FloorState continues
to serve travel; checkpoint validation is additional admission for untrusted saves.
The new Party constructor restores live builds and derives caches without the
starting constructor. Inventory rebuilds logical slot ownership with a fresh owner.
Navigation::Saved retains exact bounded work scheduling; workspace validation
checks queue/cursors, visited membership and logical request/result cells.

Schema 1 enumerates fields explicitly in the persistence codec, with content names
for catalog IDs, portraits, appearances and spell selections. Remaining runtime
world epochs and derived caches stay off disk. Clock values occupy at most the
lower half of uint64, reserving arithmetic headroom; remaining counters/generations
retain checked 64-bit encoding. Rest/expiry resume on saved clocks without advancing
offline time. Definition admission also verifies binary tile values, runtime seed
provenance, shared fixtures, initial population/supplies/guardian membership and
the fixed authored hub. Visited-depth prerequisites and party key ownership must
agree across the four records. Active snapshots supersede old travel checkpoints.

CampaignStore encodes and constructs a checked candidate before replacing the slot.
CampaignDisk supplies bounded regular-file reads and locked atomic publication;
fault hooks cover read/write/flush/close/rename/directory flush. Publication and
durability are separate results. A single denied slot association plus an unresolved
terminal-read flag implements bounded in-process denial, rather than accumulating
a history of all ended runs. Explicit successful new-run publication clears the
old association. Failed reads fail closed until retry/new save; another unsaved
run ending cannot forget a pending denial or invalidate someone else's slot.
Manual file copying and forced process termination remain outside the guarantee.

CampaignSaveFlow owns application I/O transitions; CommandController retains only
fixed SaveHud intents/dialog state. Native close also requests confirmation for an
active campaign or unresolved terminal marker. The GLFW integration resets its
close flag during ordinary event polling; continuing to draw the app's confirmation
does not inject OS input or select any window. All owner replacement retains the
existing input-release suppression. Continue constructs no starting party and does
not require any starting-library entry.

TraceCampaignFiles captures raw file inputs/results in 256 KiB byte chunks encoded
as hexadecimal ASCII (at most 512 KiB per chunk record). This also records corrupt
UTF-8 without breaking the JSON transport; exception messages are bounded ASCII.
Replay consumes recorded I/O without touching the configured slot. Both original
256 MiB trace and protocol line limits remain enforced. MCP sessions reserve
290 MiB: trace budget, slot and temporary replacement plus metadata allowance.
Full saves/traces contain diagnostic hidden state; player policies must not inspect
their contents as discovered knowledge. See [protocol](headless_protocol.md#pc06-campaign-save-and-resume)
and [MCP usage](mcp_usage.md#pc06-campaign-save-and-resume).

Measured verification and review evidence are recorded in
[the PC06 handoff](verification/archive/pc06_verification.md). No balance experiments or later slices
are included.


## PC06 presentation review correction (2026-10-30)

User requests hiding unavailable Continue entirely, compact confirmation dialogs,
and plain save-write failure wording. Main Menu shows Continue and its summary only
for a resumable slot; otherwise it uses the original four-button layout. The paused
menu keeps its existing size. Two-button confirmations and the three-button write
failure dialog are centered with shorter heights; rendering and click routing use
one shared state-dependent layout. The error title is “COULDN'T WRITE SAVE” with
“Retry” and a direct explanation that writing the file failed. This dialog remains
limited to real I/O failure after a run ends (the gallery injects that failure).
Persistence behavior is unchanged. Check keyboard/click parity, allocations and
hidden native/reduced captures; no balance experiments or OS input automation.


## Unsaved-run terminal association fix (2026-10-31)

Ending an unsaved expedition while a pre-refresh incompatible slot existed could
raise the terminal write-failure dialog: terminal handling restored the slot before
checking its run ID. `savedCampaignRunId` now uses the same bounded, depth-limited,
duplicate-key-checked parser to extract association independently of schema/content
admission. Unrelated old saves remain byte-identical with no warning; matching slots
receive the durable terminal marker. Genuine read/write failures retain denial and
retry. There is no migration. Owned regressions and verification are recorded in
[dungeon flow polish](dungeon_flow_polish.md).

## Graphical Continue at an old NPC spawn (2026-11-02)

Implemented and verified; interactive user review remains. Continue correctly
restored the run, but `runGameplay` unconditionally called `gameplayPlacements`,
rebuilding immutable initial NPC placements against the saved player position.
After killing or moving an NPC, that position can legitimately be its original
spawn. The reported save had the party at depth 1, cell (32,25), the original
spawn of three now-dead rats. This caused a graphical-startup rejection after
ordinary store inspection and restore had succeeded. Prop asset changes were
unrelated to the rejection.

The preparation helper now returns no standalone placements when CampaignRun
supplies its authoritative live session. `runGameplay` passes that same campaign
to the helper and borrows its GameSession for the view. Standalone/arena sessions
retain their validated initial placement copy and idle-clip normalization. No
NPC validation is relaxed, no saved records are filtered, and no state is rebuilt
or regenerated. Save schema/simulation/content identities remain unchanged.

`gameplay_startup_tests` owns generated campaign fixtures and exercises an encoded
save/restore at an old occupied spawn, reproduces the old rejection through the
standalone branch, verifies corrected startup preserves canonical state, and
keeps standalone overlap rejection and copy normalization covered. It links the
graphical application preparation code but runs without a window. The new case
initially waited on an automatically opening guardian gate after diagnostic
deaths; disabling animation before fixture setup made that controlled boundary
settled without weakening save safety.

Main-menu errors now appear in an opaque inset panel at art coordinates
(20,328,600,25), using 27px native bold heading text in a light peach colour.
The panel clears the navigation hint and bottom frame. Existing error storage,
button/input routing and non-main screens retain their behavior.

Release and the user's Debug application built with two low-priority workers.
Focused `campaign_save_smoke` passed in 1.24 s; `gameplay_startup` passed in 0.11 s.
The latter alone was rerun after the fixture correction. No broad matrix,
benchmark or balance route was run.

The explicit hidden diagnostic mode `blobber_world_integration_gpu_tests ASSETS
NEW_OUTPUT --campaign-startup-review COPIED_SAVE` loads through CampaignStore,
executes the same startup helper, renders the restored GameView, and captures an
injected main-menu error for readability review. It checks unchanged floor-state
digest and copied save bytes, world resource release and GL errors. It passed
using the actual reported save copied to `~/.cache/blobber-campaign-startup/`.
The original save SHA256 remained
`37e736762683aa00f9aa7bca688055cb06092a2638132d51040f6da596319eec`.
[Inspected native captures](screenshots/campaign-startup/README.md) show the
continued game and readable error panel. No OS input/focus automation or original
save writes were used. Logs are `~/.cache/campaign-startup-{build,regression-build,
debug-build,gpu}.log`. `git diff --check` passed.


## SC04 authored hub validation (2026-11-05)

The production hub is the canonical Sanctuary city. Schema remains 9 and simulation
advances to 13. Hub `entry` is recall at sanctuary; `exit` is upstairs return at the
dungeon landing. New Game uses a separate authored square spawn, while Continue
uses the saved pose. Existing landing slots 0/1 validate recall/return respectively.
World fingerprint, terrain, arrival records, fixtures, recovery and protected mask
must match the canonical builder. Mutable discovery, loot and floor state retain
existing codecs. Generic world codec fixtures no longer mutate the canonical hub.
Unsupported saves are left untouched; no migration. See [SC04 design](sc04_design.md).
