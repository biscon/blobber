# PC06: campaign save and resume

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
[the PC06 handoff](pc06_verification.md). No balance experiments or later slices
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
