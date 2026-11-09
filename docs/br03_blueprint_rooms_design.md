# BR03: generated blueprint rooms

Status: **Implemented and mechanically verified; user visual/play acceptance pending**, 2026-11-10. BR01 and BR02
remain accepted. This slice requires separate final visual/play acceptance.

## Design and ownership

Load the immutable authored catalog at application/tool preparation boundaries and
pass it explicitly to generation and canonical campaign reconstruction. Core world
and generation code remains independent of JSON and graphics. Runtime consumes
ordinary owned Map/scenery/fixtures, not live blueprint interpretation. Four bounded
instance records retain stable blueprint IDs, purpose, room ID, rotation and active
socket mask; origins remain in Room. A canonical catalog fingerprint joins floor,
save and replay identities. Preparation reuses bounded masks for physical blocking,
ground furnishings, lanes and encounter sites; they are not runtime resource owners.

Generation places hall, kitchen, barracks and armory in four existing Junction
slots before ordinary rooms, protected stairs and progression regions. Purpose and
progression role stay independent. The pair has a direct one-to-three-cell passage.
Armory attachment prefers distance <=2 from barracks, then shorter graph/passage
distance; other rooms are preferred for later attachments and loops. This is a
preference, not a mandatory side-room constraint. Preserve 36x36 floors, 10–12 rooms,
the current population/reward quantities, useful loops and all key/guardian rules.
Authored rotations/sockets are authoritative; no stretching or mirroring.

Sequence: geometry/sockets, authored reservations, fixtures/population, rewards and
dressing, ceilings, final furnished validation, publication. Share socket-dependent
kit omission and scenery/fire emission with isolated review assembly. Six beds
remain blocking; other quarters props remain nonblocking. Ground furnishings reserve
population/reward sites without changing party collision. Lanes exclude permanent
additions, but authored encounter regions may overlap lanes. Use authored reward
cell/access pairs only when existing budgets assign a reward to that room. Preserve
three containers and guardian locking. Blueprint rooms exclude generic interior
dressing; doorway surrounds remain. **Keep cages/racks and associated dressing in
ordinary rooms**, per the user's planning decision. Apply authored ceilings and
existing four-tick passage/door ceilings.

Retain 32 proposals/room, 96 proposals/loop, 16 floor attempts and the bounded route
search. Every rejection records a reason. Both explicit fallback layouts contain
the complete roster and validate at all depths; invalid content or fallback fails
without publishing or silently dropping a required room. Preparation/view failures
leave current session, save and GPU owners intact.

## Interfaces, compatibility and budgets

Generation, campaign creation/restoration, persistence and automation receive the
catalog explicitly. Tests own small catalogs and generated artwork. Save definitions
retain instance records; restore checks them against current content and published
geometry/scenery/ceilings. Diagnostics expose instances, relationship distances and
failures. Check catalog source provenance against the prepared art publication.

Generator/templates become 14/10; campaign schema becomes 22 (previously 21), simulation
stays 24. Add blueprint identity to save/replay content checks. Old saves reject
without modification. Blueprint JSON and prepared-art wire formats stay unchanged.
Runtime RNG and reward rules remain; topology/interior/dressing streams use explicit
new identities. Correct stale current-version summaries at handoff.

The accepted roster has up to 36 scenery placements and six ambient fires. Raise
those capacities from 32/2 to 36/6, and fixed effect packets from 1536 to 1680 to
include six fires plus the existing optional hearth. Keep renderer byte ceilings,
16 KiB generation metadata and 128 KiB generation scratch; report actual accounting.
Preparation may allocate, ordinary engine-owned frames may not. No new artwork,
simulation mechanics, generic prefab framework or new campaign depth is included.

## Verification and handoff

Use the configured CLion CMake/Ninja, Debug only, <=2 build workers and one test
process at low CPU/I/O priority. Focused owned fixtures cover transformed sockets,
combined furnishing collision/circulation, encounter admission, rewards, bad content,
placement/fallback exhaustion, capacities and rollback. Existing eight fixed seeds
at three depths and both forced fallbacks cover roster, deterministic topology,
progression and unchanged budgets. Run campaign save smoke during development,
full persistence once at integration, strict replay and engine-owned allocations.
Combined effect capacity needs a single-update correctness test, not a benchmark.

Use small generated assets for rendering/lifetime tests, then selected hidden
production captures of full-floor layouts and room/door/ceiling transitions. Never
control desktop input or focus. Interactive acceptance remains with the user.
Owned output belongs in an agent run; retain <=6 selected BR03 images, record
commands/results/limitations here and clean scratch at handoff. No broad sweep,
balance run, benchmark, sanitizer matrix or second build configuration is planned.

## Execution record

Implementation started from a clean worktree. Available memory was ~20 GiB and
workspace disk ~450 GiB free. No user saves were read or rewritten.

### Implemented boundaries and accounting

- `generation/blueprint_rooms` owns preparation masks and published-world validation;
  `stone_layout` owns bounded placement/connection proposals; `fallback` contains
  frozen geometry for both profiles, independent of the random placement path.
- `world/blueprints` shares socket filtering and furnishing emission between isolated
  rooms and campaign generation. Canonical fingerprinting hashes explicit catalog
  fields, never padding or addresses. IDs are validated against current content.
- Each instance caches authored ceiling ticks alongside its ID, purpose, rotation,
  sockets and room index. Restore checks that cached value against the catalog.
  Origins/dimensions remain owned by `Room`. Runtime keeps no catalog pointer.
- `CampaignStore` and automation borrow the immutable catalog from their longer-lived
  application/tool owner. Codecs use it for identity and canonical reconstruction.
  Source provenance must match `prepared/world/sources.json` before graphical launch.
- Preparation ordering ensures scenery blocks movement before population/rewards are
  admitted. Final collision-aware validation checks keys, protected landings, guardian
  progression, expected furnishings, actor sites and authored ceilings. Generic props
  remain allowed only in ordinary interiors; existing doorway meshes are unchanged.
- Armory proposals rank parent graph distance, then feasible passage length within
  that parent, with seeded ordering for ties. Loop routing retains the best 96 socket
  pair proposals, penalizes armory endpoints and keeps the original routing bounds.
- Debug retained generation metadata: **11,792 bytes** (limit 16 KiB); bounded scratch
  accounting: **122,016 bytes** (limit 128 KiB), including authored masks. Static
  assertions cover the enlarged preparation storage. Maximum authored scenery is 36;
  unused wall-backed placements can be omitted by active sockets. Six ambient fires
  plus an optional hearth fit 1680 effect packets. Renderer byte budgets are unchanged.

### Verification commands and outcomes

Configured tools: `/home/bison/clion/bin/cmake/linux/x64/bin/cmake` and bundled Ninja
from the existing Debug cache; builds use at most two workers. Commands below were
run at `nice -n 15 ionice -c 3`, one test process and one intensive job at a time.

The Debug game, headless runner, generator, affected test targets and affected
production review tools build. The production world publication contains 556 textures,
883,633,616 mip bytes and 64,967,422 alpha-mask bytes, within existing byte ceilings.

Focused CTest groups (run with `--test-dir cmake-build-debug -j 1 --output-on-failure`):

- `blueprints`, `blueprint_render`, `generation`, `dressing`, `fixtures`, `population`,
  `campaign`, `generation_automation`, `generation_cli`, `campaign_save_cli`,
  `automation`, `headless_cli`, `automation_allocations`, `world_integration`,
  `world_environment`, `game_time`, `gameplay_startup`, `inn`, `smithy`, `emporium`:
  passed. Allocation checks cover engine-owned prepared frame work and injected
  preparation failures; they do not claim allocation freedom in graphics drivers.
- `campaign_save_smoke`: passed after adapting the owned-scenery assertion to append
  to the four furnished rooms. Active and inactive dungeon metadata tampering rejects
  even when the floor fingerprint is recomputed. Schema 21 is explicitly rejected.
- Full `campaign_save` (labelled slow/integration): passed once in **17.58 s**, including
  multi-floor continuation, sparse definitions, effects, travel, I/O failures and
  inactive state. Subsequent validation tightening/test changes received the smoke check.
- `world_integration`: one-update combined-effects correctness passed with six fires,
  an optional hearth and the existing actor/area effects; no capacity benchmark ran.
- The eight fixed seeds at all three depths pass deterministic roster, topology,
  pacing, progression and architecture checks; both forced fallbacks pass at every depth
  (compact/chamber seeds: 0/21 at depth 1, 0/13 at depth 2, 8/5 at depth 3).
  Small production samples also published the full roster without fallback.

The hidden OpenGL command was:

```sh
cmake-build-debug/blobber_review_blueprint_campaign assets \
  cmake-build-debug/assets/prepared NEW_AGENT_RUN_DIRECTORY/review
```

It passed **four floors, twenty native views**, all kitchen/hall traversals, GL checks,
failed staging rollback and GPU ownership cleanup. Sandboxed display access was
unavailable; the authorized hidden check used display access without OS input or focus
control. An initial harness assumed the door was at the kitchen end of every passage;
opening the encountered door at each checked step corrected that harness assumption.
Selected images were inspected for furnished rooms, ceiling/passage transitions and
all-roster floor topology. [Six retained review images](screenshots/blueprint-rooms-br03/README.md).

Final rechecks passed: `guardians` (**0.42 s**), `world_visual_cli` (**0.88 s**),
`fixtures_automation` (**37.75 s**). The guardian fixture now selects the entrance's
actual graph edge instead of a nearby unrelated door. The visual CLI fixture uses
current visibility identity v18 and verifies prepared-source mismatch rejection.

The first full fixture replay was interrupted after identifying 535 redundant one-tick
settling snapshots in its 72.9 MB trace. Ten-tick batches preserve bounded settling
and checked route replanning. This still exercises a complete keyed-floor route,
guardian fight, fresh-process replay and deliberate key/reward corruption; it is now
explicitly labelled `slow;integration` (60 s timeout), outside ordinary presets. No
unrelated temporary directories were removed; the interrupted fixture was transferred
to the owned run for cleanup.

### Limitations and handoff

User visual/play acceptance remains pending. Static art captures intentionally remove
actors; CPU encounter, guardian, movement, save and replay checks retain them. No broad
balance trial, capacity stress, benchmark, sanitizer matrix or Release configuration
was run. Existing campaigns require a new run under generator/templates 14/10 and
schema/simulation 22/24; incompatible files reject without migration or modification.

Only the six selected images are durable review output. Disposable logs, JSON, failed
captures and the interrupted replay fixture are owned by this run and cleaned at handoff.
