# PC03: guardians and structurally safe progression

Status: **Accepted and closed**, 2026-10-30. Implementation approved by the user:
“Implement the plan.” The user then played through the accessible floor, defeated
the guardian, collected the loot and explicitly accepted the slice. Milestone P-A
is Accepted. PC01/PC02 remain accepted; PC04 is not started.

## Approved experience and boundaries

An intentionally easy functional slice: one ordinary melee goblin, one optional
melee goblin, and a designated guardian pair (14-HP placeholder melee lead and
ordinary archer). All depths use this proof set. The lead retains normal attacks,
other stats, drops and 50 XP; no scaling or balance experiments. Recipes are small
replaceable tables, not a future placement framework. Existing prepared appearances
are reused. The approach door retains depth-1 unlocked / depth-2 Bronze / depth-3
Silver rules. A separate physical guardian gate protects the reward/objective.
Both stairs rooms have normal initially closed unlocked doors and protected bent
arrival alcoves. Doors remain permanently open after use, as chosen by the user.

Travel, hub, campaign victory, persistence, broader monsters and tuning remain later
slices. Normal New Game remains depth 1; other depths are independent scenarios.
PC03 and P-A are now accepted by the user; PC04 requires its own reviewed plan.

## Responsibilities, ownership and data

Generation retains bounded logical placements, stable recipe/member identities,
fixture associations and landing metadata. A shared preparation adapter resolves
catalog/appearance IDs and constructs NpcWorld, RecoveryLevel and a bounded floor
objective definition for graphical and headless sessions. Preparation failure cannot
replace the live session. Generation/templates advance to 3/3; old generated inputs
reject. Scenario/protocol remain 1, prepared format remains 4, renderer revision 5.
Layout, encounter, reward and runtime seeds remain independent.

GameSession owns objective state and checked actor bindings; immutable definitions
are copied at preparation and all borrowers remain within existing session lifetimes.
Only permanent hasDeath markers for every designated member complete the objective.
Living removal, awareness changes, unrelated victory and reinforcement membership
cannot count. Death then removal still counts. Gate opening, reserved reward
activation and completion bookkeeping commit together after revision preflight.
Existing XP/death loot remain exactly once; no bonus guardian XP. Reserve one healing
potion and one camp kit beyond the gate; full bags leave ground rewards intact.
Optional branch supplies are one healing potion and one mana potion.

FloorGeometry owns fixed landing reservations and exposes NPC passability separately
from physical solidity. Actor construction/movement and all navigation/AI destinations
respect it; player movement, LOS and attacks retain real geometry. Reject drops on
protected landing cells. Bent rooms and exclusion masks must prevent LOS/contact
from every permitted NPC cell into arrival pads with every door open. Expose a
read-only checked landing query for PC04; do not implement travel or immunity.

## Generation and bounded work

Keep 32 generated actors, 32 fixtures, 1,600 cells, 16 attempts plus validated
fallback, 16 KiB retained definition and 128 KiB scratch. Fixed guard routines and
compatible anchors keep optional encounters upstream and separate via actual LOS
and radius-three alert behavior. Validate final tiles with bounded
(cell, key-acquired, guardian-defeated) search; legally openable ordinary doors are
abstract traversable edges, and guardian defeat is an abstract transition at a valid
reachable attack approach. Prove keys before locks, no guardian bypass, optional
independence, return routes, valid placements, attack access and landing protection.
Normal and fallback layouts satisfy the same checks. Include all new definitions in
fingerprints and diagnostics. No allocations in ordinary simulation, interaction,
HUD or rendering; preparation/diagnostics may allocate.

## Interfaces and presentation

Add GuardianGate fixture kind, bounded objective definition/snapshot, landing safety
query and shared generated-floor preparation. Existing authored/arena callers use
empty defaults. Reuse prepared sealed-door/threshold imagery, checked interact
commands, existing navigation/frame invalidation and discovery. Warn at the approach
door before opening: guardians beyond; combat prevents movement. Label visible
guardians and placeholder lead, report completion/reward availability. Stairs explain
travel remains unavailable. Depth 3 secures a floor objective, not campaign victory.

Extend ordinary observations/actions/client/MCP contracts and strict replay with
objective state. Player knowledge does not reveal hidden membership/placements;
diagnostics expose full definitions/ledgers without discovering them. No objective
completion command or duplicated game rules. A shared explicit placeholder appearance
alias preserves arena selection without a new content schema.

## Verification strategy

Owned small fixtures cover all depths/rotations, determinism/seed domains, malformed
keys/gates/placements/fallbacks, optional bypass routes, bounded storage, partial and
simultaneous guardian deaths, live removal and stale IDs, repeat requests, full bags
and drop storage, reserved rewards, exactly-once XP. Exercise actual perception,
alerts, pursuit, wandering, ranged threats and combat repositioning with doors open;
test landings after enemies move, hazards and unresolved actions. Verify warnings,
fog isolation, gate image/picking invalidation, direct/headless/MCP agreement, replay
and deliberate state corruption. Retain existing arena tests and meaningful PC02
regressions while updating obsolete empty-floor expectations.

Run Debug/Release headless and graphics suites, focused ASan/UBSan, allocation guards,
measured memory bounds and available hidden application-owned GPU/captures. No OS
input/focus automation, balance matrices or broad seed sweeps. Record exact commands,
failures/corrections, captures and unavailable checks in pc03_verification.md. Preserve
all entry working-tree changes. No commit is implied.

## Implemented boundaries and measured costs

The shared adapter is [population.cpp](../src/generation/population.cpp); objective
ownership and the checked arrival query are in
[session_objective.cpp](../src/game/session_objective.cpp). The gate uses existing
prepared door/threshold pixels. The placeholder aliases the existing goblin
appearance in generated floors and the arena. No new render bank or content schema.

Landing reservations cover walkable interior cells of both bent stairs rooms;
only the central arrival cell is an arrival destination. NPC exclusion is a fixed
1,600-bit mask consulted in constant time by actor movement, BFS and AI. Physical
walls still govern sight, attacks and player movement. Generator validation checks
both arrival destinations against every permitted hostile cell with doors
abstractly open. The runtime query additionally checks current occupancy, movement,
party ability, hazards, pending combat work and threats; PC04 must use it in a
transaction that prepares the destination before mutating a campaign.

The existing 64 authored-ground reservation region also holds the two inactive
objective rewards; preparation rejects overflow. Player drops retain their separate
256-slot pool. Objective members are immutable, nonrecycled actor slots bound to
this session's world identity. Gate and reward revisions are preflighted before
completion; capacity exhaustion leaves both unpublished and reports failure.

Measured retained definition plus map payload is 5,264 bytes at 36×36 (5,568 at the
1,600-cell bound), and accounted generation scratch is 34,576 bytes. Engine-owned
tracked runtime allocations remain zero in the exercised paths. Prepared launch
accounting remains 273,874,998 bytes; all imagery is reused.

[Verification and next-agent handoff](pc03_verification.md) records exact checks,
resolved failures, the diagnostic capture gallery, strict owned-fixture combat trace
and review limits. PC03 is Accepted and closed; milestone P-A is Accepted. PC04 has
not started and is next eligible for detailed planning only.
