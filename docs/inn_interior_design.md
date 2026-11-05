# Sanctuary inn

Status: implemented and mechanically verified; awaiting user review (2026-11-05).

## Content and behavior

The inn is a fifth, authored campaign location, separate from dungeon depth. F at
the named city inn approach, facing its facade, enters a cozy common room. F at
the interior exit returns outside facing the street. Doorway travel uses existing
safe exploration, fade, preparation and transactional commit boundaries. Solid
building facades remain solid. Smithy/provisioner interiors, NPCs, commerce, rooms
and storage are deferred. A decorative staircase reserves their future direction.

The room contains two seating groups, a bar with a clear staff aisle, shelves,
ale barrels, plants and a stone hearth. Prepared painted meshes follow the city
and Underkeep export pipeline, with retained Blender/GLB and image sources.
Scenery bases block movement without becoming full-height sight walls. Authored
validation protects routes and arrival/exit cells. The hearth has one bounded
24-particle visual emitter; deterministic flame/ember motion uses the paused view
clock, depth testing and no picking IDs or damage. Warm surroundings are painted,
not physically lit. Assets, geometry and effect storage prepare outside frames.

## Interfaces, ownership and persistence

Campaign location identities distinguish City, three dungeon floors and Inn.
Location-owned immutable doorway links carry approach/facing, destination and
arrival. Requests use world identity, campaign revision and a bounded doorway
slot; no long-lived borrowed pointer or runtime graph is needed. Inactive floor
snapshots preserve discovery, loot and party-effect transfer as before. Destination
session/view preparation must succeed before commit, including protected arrivals;
cancellation or failure preserves the source. Location leases release at commit.

RecoveryLevel gains a whole-map free-rest policy. The inn enables it; the city
loses its sanctuary recovery cell. Existing rest costs, durations, benefits and
safety remain, except long rests consume no kit anywhere in the inn. Recall stays
at the outdoor sanctuary arrival and never rests automatically. Travel UI inside
the inn does not offer paid recall or dungeon shortcuts. HUD, saves and automation
identify the inn independently from dungeon depth.

Campaign schema/simulation advance from 9/13 to 10/14. Old saves reject unchanged,
as approved; no migration. Both authored maps validate against canonical content.
Dungeon generation stays unchanged. The fifth location and its snapshots use fixed
bounded storage; existing scene/residency ceilings and staged rollback apply.

## Verification

Use owned small CPU fixtures for doorway facing/range, stale/foreign requests,
safety, round trips, rest policy, save/restore and rejected definitions. Check
allocation-free warmed interactions/effects, rendering depth/picking and lease
cleanup. Run focused suites in one configuration and extended campaign saves once
at integration. Inspect hidden application-owned captures of all room features,
entry/exit, rest UI and Continue. No desktop input/focus automation. One intensive
job at a time, low priority, two build workers and one test process; no routine
benchmarks. Record actual results and remaining visual review separately.

## Implemented details

The authored map is 12×12, including outer padding and modeled enclosing walls;
its common-room floor is 8×8. City entry uses the existing (16,5) east-facing
approach. Interior entry/exit share (5,9), north/south respectively. An optional
canonical doorway link per authored location is reconstructed during restore,
never trusted from disk. Slot zero is nonrecycled and checked against world/revision.
Existing two-slot dungeon landings remain; `arrivalSafety(Cell)` shares their
safety validation for doorway arrivals.

The hearth is one optional immutable `AmbientFire` anchor in WorldDefinition,
serialized and fingerprinted with a tagged extension that leaves absent-hearth
fingerprints unchanged. Twenty-four reserved effect packets animate at 20 Hz;
offscreen fire preserves cached frames, and paused clocks preserve visible frames.
The inn leases its prepared flame texture with the rest of its pack. Capacity
exhaustion rejects preparation; no frame storage grows.

HUD and map titles use authored location names at depth zero. Rest retains the
existing preview/command API (`atSanctuary` / `RestPreview::sanctuary`) while its
whole-map policy comes from RecoveryLevel. Free-rest prompts and the rest panel
identify the inn. The existing rest illustration/animation is reused.

[Verification and resource measurements](inn_interior_verification.md) and
[captures](screenshots/inn/README.md) record the delivered result and review limits.
