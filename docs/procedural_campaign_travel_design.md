# PC04: persistent descent and fixed hub

Status: **Implemented; Accepted and closed**, 2026-10-30.
The user approved the reviewed PC04 plan with “Implement the plan” and accepted
the slice with “slice accepted, record it in the plan doc” (2026-10-30).
PC01–PC03 and milestone P-A remain accepted. PC05 is next for detailed planning
only; milestone P-B remains Planned.

## Responsibilities and ownership

CampaignRun owns one Party, the gameplay clock, run result and four stable location
records: an authored safe hub and three eagerly generated depths. The active
GameSession borrows the Party. Standalone sessions retain owned-party construction.
Maps/catalog outlive simulation and presentation borrowers. Inactive floors retain
bounded value snapshots, never independent parties or live navigation/controllers.
Prepared image banks are shared across the run. Only the active location, and a
short-lived staged destination, own navigation and presentation scratch.

Snapshots preserve fixtures/discovery, actor lifecycle/positions, AI intent and RNG,
loot/drop generations and publication ledgers, XP credit, objective/reward state,
local clocks/effects and readiness. Logical slots replace runtime world identities
on import. Unfinished navigation requests restart within ordinary bounded work;
restore does not step AI or draw RNG. Starting gear and loot are never regranted.

## Transaction and travel rules

Stairs are explicit fixture interactions with a destination label, without an
additional confirmation. Connections are hub ↔ 1 ↔ 2 ↔ 3, with downward travel
requiring the source guardian objective. Ascent is free with zero kits. Arrival uses
the protected entrance or objective landing, facing into the room. No heal, refill,
time skip, enemy relocation or automatic rest accompanies travel.

Prepare a source snapshot, destination session and replacement view before commit.
Revalidate source identity/revisions and safety, then nonthrowingly swap owners and
apply staged party clock changes. Preparation failures leave the source playable.
Construction/import must not reconcile effects against the shared Party. Source
and destination both use settled recovery safety, able leadership and hazard checks;
destination additionally uses PC03 landingSafety. Clear old targeting/overlays and
pending input, rebuild frame/picking/animation borrowers, and require input release.
Runtime actor/fixture/loot/effect identities change; party inventory identity stays.

## Clock and effect boundary

Campaign gameplay time follows the existing hybrid simulation clock, including
round advances and successful rests. Each inactive floor freezes presentation, AI,
local gameplay clocks and RNG. Loading, menus and travel advance no clock.
Party-only Bless, Bane, Shield of Faith, Mage Armor, Hold Person, Guidance,
Resistance, Sanctuary, Aid and Mirror Image travel with remaining expiry/retry,
caster and concentration state. Party Sleep/Blind/Slow/Knockdown conditions travel;
NPC conditions and Guiding marks stay local. Preserve caster-turn semantics,
Mirror duplicates, temporary HP, Defend and action/feature readiness.
Concentration referencing NPCs, mixed recipients or Web/areas blocks travel until
explicit dismissal. Spiritual Weapon is non-concentration: add an explicit free
safe-exploration dismissal action and require dismissal before travel. Party local
area restraint also blocks departure. Grease and other independent local areas,
NPC restraints, expiry/tick and burn deadlines freeze. Immediate damage spells have
no persistent transfer payload. Import uses remaining durations, with checked
rebasing, and never casts, rolls saves, expires, or emits consequences. Rest skips
and ordinary expiry continue through existing authorities exactly once. Combined
portable/local capacity exhaustion rejects travel, never drops an effect.

## Hub, lifecycle and adapters

Use a small authored enclosed masonry hub, no hostiles or replenishing loot, an
identified sanctuary rest cell and one dungeon entrance. New Game uses the existing
party picker once and starts here. Record legal first entries for PC05 without
exposing waypoint travel. Final guardian completion wins after pending consequences
settle; simultaneous defeat takes precedence. Terminal campaigns offer Main Menu,
not retry; arena rematches remain unchanged. No recall, saves, tuning or policies.

One campaign implementation serves UI and automation. Add a versioned campaign
scenario and status/transition/result observations to existing tools and replay;
known stairs use interact. Player output cannot reveal inactive/hidden geometry.
Diagnostic integrity includes inactive snapshots and RNG, using logical identities.
Existing bounded protocol budgets remain. Remove obsolete static New Game wiring,
retaining authored diagnostic and arena support actually in use.

## Storage, performance and validation

Four location records; current actor/effect/fixture/loot limits remain authoritative.
Snapshot and view preparation may allocate, ordinary processing may not grow.
Measure snapshot/transition storage and shared prepared-bank accounting. Explicit
capacity, identity and clock exhaustion reject before commit.

Tests own small fixtures and selected deterministic seeds: mutated revisits,
exactly-once gear/XP/loot/rewards, every effect category and clock, stale/poisoned IDs,
unsafe phases and moved enemies, zero-kit ascent, partial/final victory and defeat,
transaction failure injection, held input and borrower lifetimes. Test direct,
client/MCP and strict replay including inactive-state corruption. Run Debug/Release
headless/graphics suites, allocation checks, focused ASan/UBSan, and available hidden
application-owned GPU/capture checks. No OS input/focus automation. No balance
experiments, broad sweeps or full expedition experiments. Record exact verification,
limitations, captures and reproduction in pc04_verification.md before review.

## Implemented interfaces and evidence

`CampaignRun` holds four stable CampaignLocation records, one Party and one active
borrowed-party GameSession. `PreparedTravel` is a move-only opaque staging ticket;
callers can prepare a replacement view against its candidate session, then commit.
Commit validates source revision, session mutation epoch, actor revision and a
fixed Party/inventory/resource/formation stamp. UI fixture intent is consumed at
the same input boundary, never by reconstructing owners on a later frame.
Standalone session construction uses a private owned-Party convenience owner.

`FloorState` combines subsystem value checkpoints and logical references.
Import is preparation-only: validate references/capacities, rebuild occupancy,
restore values with fresh runtime epochs and bind permanent local event storage.
It never observes combat, recasts effects, rolls saves or reconciles Aid against
the travelling Party before commit. Portable instances/party condition rows are
removed from inactive records; active party readiness overlays the destination.
Active snapshot integrity is exported from the live owner, not its older checkpoint.
Diagnostic digests enumerate fields and RNG, excluding padding/addresses/epochs.
They allocate only for inspection/trace output and are not a disk-save format.

Final objective completion closes command admission immediately. Already accepted
impacts and presentation finish; new AI, movement and automatic turns do not start.
Unrelated living actors do not prevent campaign victory. Defeat takes precedence.
The final result must already have been presented before confirmation can return
to Main Menu. Native HUD labels and interaction hints reflect hub/depth/terminal
state, and input edges survive borrower replacement.

Four fixed snapshot payloads total 943,840 bytes, excluding immutable maps and
active/staged scratch. Prepared banks are loaded once (273,874,998 bytes in the
shipped capture) and borrowed by all views. Fault injection exercises every
allocation in the owned headless/graphics preparation workload. Post-travel
simulation, commands/HUD and prepared composition remain allocation guarded.
See [verification and review](pc04_verification.md) for exact commands, results,
retained trace/captures and unavailable interactive/live-host checks.
