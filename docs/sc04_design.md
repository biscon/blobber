# SC04 — Sanctuary campaign hub replacement

Date: 2026-11-05. Status: **Accepted and closed — 2026-11-05**, including the entrance trim correction.

The user approved the conversation plan and requested implementation. SC03 art
is accepted; this slice makes it the normal campaign hub. No services, interiors,
outside travel, gate operation or PC08 work is included.

## Definition and ownership

Promote the preparation-only SanctuaryCity factory from review tooling to world
content. It owns the immutable 24×24 map and bounded doorway/reservation metadata.
Both campaign and review tooling consume it. A small campaign hub builder derives
fixtures, recovery and arrival records; campaign orchestration owns the resulting
Map, placements and snapshots through existing CampaignLocation ownership.

New Game starts at (11,8), north. Recall arrives at sanctuary (9,6), north;
upstairs travel from floor one arrives at (13,7), south. Continue restores the
saved pose. Hub entry/exit store recall/dungeon-return poses respectively. Existing
landing slots 0/1 represent those arrivals, with matching safety checks before
preparation and commit. Protect spawn, rest, landing and stairs using npcExcluded.
One north-facing downstairs fixture occupies (13,6). The closed city gate remains
static scenery without a fixture. Validate routes with the stair cell blocked.

Rest uses the existing RecoveryLevel sanctuary definition, including HUD prompts.
Recall charges one kit at commit and never rests implicitly. Existing free unlocked
floor travel remains entrance-local and requires facing north. Enable depth-zero
cell discovery; town has no generated-room metadata. Existing fixed map/discovery
storage covers its 576 cells without growth.

## Persistence and boundaries

Validate saved immutable hub terrain, world fingerprint, anchors, fixture/recovery
records and protected mask against the canonical factory. Preserve mutable pose,
discovery, loot and floor state. Keep schema 9, advance simulation 12 to 13, and
replace diagnostic hub-v1 with sanctuary-city-v1. Reject old identities unchanged;
no migration or automatic save replacement. Arbitrary codec fixtures belong outside
the canonical hub. The old courtyard survives only as owned test data.

Existing session/view staging stays transactional: preparation failure or cancel
retains source session, Party, image and picking state. Assets remain location-leased;
no frame-time creation, decoding, model resolution or container growth is added.
Use production build publication, existing chunk/material rendering and city water
clock. No new capacities or rendering policies. Metadata projects the largest
city/dungeon texture union at about 456 MiB under the 512 MiB ceiling; verify actual
active/staged storage, release and cancellation. Driver memory is outside accounting.

## Verification and delivery

Use one Debug configuration, sequential low-priority jobs, two build workers and
one test process. Focus city/campaign/recovery/map/save-smoke/CLI/residency checks
on changed callers; run extended campaign_save once for changed restore/travel.
Cover distinct arrivals, protected routes, wrong facing, recall single charging,
rest, discovery, active/inactive saves, invalid definitions, unsafe landings and
staging rollback. Check warmed frame allocations with owned small GPU fixtures.
Use hidden application-owned production captures for New Game, entrance/return,
recall/rest, map and Continue; inspect stairs against accepted architecture.
No OS input automation or benchmark matrix. Interactive review remains user-owned.

Record final implementation, measurements, evidence and limitations in
sc04_verification.md, reconcile roadmap/system overview, and stop Ready for review.


## Delivered implementation and evidence

`SanctuaryHub` is the preparation-only game/content adapter; `SanctuaryCity` remains
world-owned content. `GameSession::atSanctuary()` supplies the existing recovery and
HUD paths. Both serialized hub arrival fields and the two existing landing slots
are reused; no capacity or wire-shape expansion was needed. The production city
factory is shared by campaign and optional review tooling. Courtyard rendering
fixtures now live exclusively in test support.

Final campaign schema/simulation are 9/13. Production city/dungeon views reuse
publication 10, appearance 4, visibility v17, fingerprint revision 4 and generator/
templates 9/7. The ordinary build published all assets successfully. No art changes were needed; the subsequent entrance review correction below
adjusts prepared stair geometry. Generic environment codec coverage uses an owned
generated-location record; canonical city Continue coverage checks the real city.

Measured city/floor-one overlap: 455.72 MiB mip textures, 41.80 MiB alpha,
61.13 MiB tracked CPU and 470.73 MiB tracked world GPU. Application GPU peak is
590.82 MiB, above the advisory 448 MiB reference and below existing hard world
budgets. No quality reduction or budget increase. Cancelling and returning restore
the city-only residency; clearing releases all location textures/masks/models.

[Verification/handoff](sc04_verification.md) records focused CPU/extended save,
allocation, hidden GPU and production capture evidence. The optional full scripted
route reached its 30-second deadline in dungeon play; the changed city-entry route
passes as a separate short check. No broad repeat or benchmark was run.


## Entrance trim review correction (2026-11-05)

The authored doorway's raised arch projects 0.035 cells ahead of its building
footprint. The staircase terminal panel previously sat almost exactly on that
footprint boundary, allowing the arch to protrude through its black interior.
At preparation, stairs backed by a building-owned cell inset the opaque
terminal panel 0.06 cells into the stair volume and follow the sloping ceiling at
that depth. Ordinary terrain-backed stairs keep their existing terminal depth.
The same prepared triangles supply GPU color and CPU/GPU depth/picking. Fixture
identity, collision, travel, save schema, assets and bounded storage stay unchanged.
Visibility identity is v17 because prepared depth changes. Stair CPU checks and
hidden GPU parity/allocation checks pass. Production front/near/oblique captures
confirm the bright strips are hidden; see the [gallery](screenshots/sc04/README.md).
