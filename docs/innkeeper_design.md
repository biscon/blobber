# Sanctuary innkeeper

Status: implemented and mechanically verified; awaiting runtime visual review (2026-11-06).
The user approved
[reference v1](../assets/npcs/innkeeper/reference/turnaround-v1.png) and authorized
implementation. Dialogue, trading and the rest of the economy are deferred.

## Responsibilities and ownership

The implementation reuses immutable NPC definitions, stable actor handles, occupancy,
the animation player and prepared-image rendering. It adds an explicit `NpcRole` with
`Hostile` (the backward-compatible default for authored content) and `Resident`.
Residents are stationary, peaceful, persistent actors without monster rules,
combat HP, attacks, loot or rewards. Their invalid MonsterId and zero HP are
intentional, not fabricated monster content. Role belongs to placement data and
is copied into floor-owned runtime actors and snapshots. No factions, shops,
dialogue state or generic service framework are introduced.
The actor owner copies at most 32 bounded appearance IDs during preparation, so
the resident identity can be read without borrowing strings from temporary placement
objects. Contact checks scan the existing contiguous active actor range to exclude
residents; no second occupancy grid, faction lookup or frame allocation is added.

Source appearance metadata also declares the role (default hostile). It validates
placed roles and permits resident-only idle/talk artwork without the mandatory
monster prone pose during publication. Residents inherit the declared role when
an authored JSON placement omits it; mismatched roles reject at load time.

Actor construction requires valid monster rules only for hostile instances. Resident
placements require idle (or the explicit talk preview), guard/no patrol, and no
monster override. Resident movement, attacks, damage and removal reject without
mutation. They still block party movement through the existing occupancy mask.
Combat discovery, admission, initiative, hostile spells/areas, AI, effects,
target lists, enemy hover UI, loot and experience all exclude residents. Rendering
retains their normal depth-tested bodies, contact shadows and view selection.

## Content and animation

The inn owns exactly one resident `innkeeper`, at cell (6,3), southeast subcell,
facing south into the common room. Its feet are in the existing staff aisle north
of the bar at row 4. Canonical authored placements are reconstructed/validated
at campaign loading; no procedural dungeon population includes this appearance.
Small generated test catalogs explicitly own their resident appearance/type data.

`Talk` is appended to `NpcAnimation`, retaining existing numeric clip values. Talk
requires front art only; side/rear views use their directional idle timeline,
sharing the existing attack fallback contract. A small checked resident-talk
presentation setter lets future dialogue start/stop the prepared loop; normal
gameplay leaves the innkeeper idling. No talk command, interaction prompt or UI
is exposed in this slice. Optional missing talk artwork safely renders idle,
including when the animator is reconstructed while Talk is selected.

Four two-pose idle masters and one four-pose front talk master were generated from the
approved reference with the built-in imagegen skill. Exact prompts are preserved and
sheets are registered offline using the existing extractor. Idle is subtle breathing
and blinking with planted boots; talk has small mouth/head changes with the
same planted feet. All poses keep identity and wardrobe. Use shared human scale,
full-resolution registered canvases and the normal crop/mip publication. No
procedural body distortion substitutes for authored animation.

## Persistence and resource bounds

Persist role in placements/runtime snapshots and validate it against original
placements; reject forged resident combat state or changed canonical innkeeper
position/facing. Advance campaign schema/simulation and appearance publication
revision for the added field/clip. Old saves reject explicitly rather than being
silently reinterpreted; new assets also change the NPC content fingerprint.
The implemented campaign schema/simulation are **11/15**, appearance revision is
**6**, and world publication format remains 13. Automation observations add a
separate `residents` array with identity, pose, idle/talk and `targetable: false`;
residents never enter `enemies` or offensive target lists.
Use existing fixed actor, clip/frame and renderer capacities; no new dynamic
frame containers or allocator trips. Preparation still fails on existing bounded
budgets. The runtime atlas/publication is regenerated during integration.

## Verification strategy

Use small owned fixtures for resident construction without monster definitions,
stationary occupancy, rejected offensive mutations, AI/contact/target/effect/spell
exclusion, talk playback and directional idle fallback. Verify pause/timing and
warmed frame processing without allocations. Extend inn roundtrip checks for
one canonical resident, persistence across travel/Continue, free rest nearby,
and rejected role/placement/actor corruption. Preserve ordinary monster behavior.

Run focused Release CPU suites for actors, AI/combat, animation, metadata,
targeting/commands, inn and save smoke; use a single process and at most two build
workers at low priority. Inspect hidden application-owned front/side/rear captures,
bar occlusion and a talk preview, plus a focused OpenGL resource/presentation
check where available. No desktop input/focus automation or benchmarks.
Disposable output belongs to the task's marked agent run. Retain at most six
selected captures with a README and remove the run after checks complete.

## Verification results

2026-11-06. Release builds of `blobber`, `blobber_headless`, `blobber_review_inn`
and the selected tests pass. The source sheet extractor registered all 12 poses
without clipping; the world publication contains 425 textures. Idle timing is
1800/140 ms; Talk is 180/160/180/220 ms. Generated low-alpha edge RGB is discarded
by the existing alpha cutoff of 15; no recoloring or procedural animation is used.

The focused integration command passes all 12 selected suites in 2.40 seconds:

```sh
nice -n 15 ionice -c 3 ctest --preset release -R '^(residents|inn|npc_metadata|actors|ai|combat|animation|targeting|campaign_save_smoke|world_visibility|effects|expanded_spells)$'
```

Coverage includes resident/hostile coexistence, no monster binding for residents,
movement/damage/AI/effect/spell/target exclusions, canonical inn placement,
travel/Continue, free rest around the innkeeper, resident tamper rejection,
metadata role inheritance, talk boundaries and directional idle fallback, and
zero engine C++ allocations during warmed simulation/visibility. The optional
missing-Talk reconstruction correction is checked separately with the affected
resident, metadata and animation suites (all three pass in 0.03 seconds); passing
unrelated checks and the unchanged production captures are reused.

Hidden production capture command (use a new directory in an owned agent run):

```sh
nice -n 15 ionice -c 3 env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 cmake-build-release/blobber_review_inn cmake-build-release/assets RUN_DIRECTORY/captures innkeeper
nice -n 15 ionice -c 3 env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 cmake-build-release/blobber_world_visibility_gpu_tests
```

The sandbox could not open display :0; the authorized hidden run outside that
restriction succeeded using NVIDIA/OpenGL. Front, both side and two talking
captures were inspected for human scale, planted feet, identity, transparent edges
and counter occlusion. The GPU visibility/parity/lifecycle check passes with zero
boundary pixel differences. All world texture/alpha leases return to zero after
the view is released. Observed city/inn staged overlap is 407.30 MiB textures,
18.94 MiB alpha, 436.38 MiB tracked world GPU and 40.48 MiB tracked world CPU,
within existing hard ceilings; peak application GPU is 558.35 MiB (the historical
448 MiB reference is advisory). These are engine-owned resource measurements,
not a claim about driver-internal allocations.

[Six selected captures](screenshots/innkeeper/README.md) retain pending user
acceptance. No desktop input/focus automation, benchmark, stress workload or
additional build configuration was run. A direct rear approach is blocked by the
existing shelving; rear art and all view-selection fallbacks were inspected/tested
with owned source/CPU fixtures. Timing/feel and final visual approval remain
user-owned. Source artwork/prompts and registration inputs remain durable; the
owned disposable run is cleaned after selecting review evidence.

## Runtime review correction — 2026-11-06

Implemented and mechanically verified; awaiting user re-review.

The user rejected the runtime size impression and reversed side directions. The
side masters were assigned to the wrong anatomical slots: from east of a south-facing
resident, looking west, his nose must point screen left. Correct the innkeeper's
source left/right sheet labels and their registration/provenance together, keeping
the definition JSON and renderer's existing sector selection unchanged. This is
an asset binding correction, not a renderer-wide convention change. Preserve exact
generation requests; their original anatomical wording was mistaken.

Keep source image bytes unchanged and preserve the NPC definition fingerprint, so
this cosmetic correction does not invalidate the campaign the user just started.
The authored position is already cell (6,3), subcell (1,1): world feet (6.75,3.75),
the southern half-cell closest to the counter. Make that placement explicit in
the focused inn regression check; do not center the resident on the whole cell.

Measured standing body height is .75×480/608 = .5921 cells, about 1.9 m when the
fixed .5-cell eye represents 1.6 m. The counter slab top is .3465 cells (1.11 m),
and the shelf top is .82775 cells (2.65 m). The front view's counter is closer to
the camera than the resident; the close staff-aisle view is a better body-height
check. The user chose a modest reduction of the counter and rear shelves, explicitly
retaining the tables, chairs, fireplace and innkeeper scale. Apply a uniform .85
factor to only the bar and shelves in the offline exporter, around their existing
floor/wall pivots. Counter slab height becomes .294525 cells (about .94 m), and
shelving height .703588 cells (about 2.25 m). Keep map anchors, collision reservations,
the nearest resident subcell, camera, other geometry and existing save compatibility.
Regenerate the packed Blender/GLB, mesh parts and metric atlas together; compare
unaffected model positions/normals against the pre-export snapshot.
Inspect matched front and close left/right views after the correction; use only
the affected inn CPU check and one hidden production capture pass. Keep resource
limits and six-image evidence retention unchanged.

Verification: Release game/review runner rebuild and the focused `inn` suite pass
(0.35 seconds), including the explicit nearest-subcell assertion. Source direction
requests and corrected masters agree: `idle-left` faces screen left and `idle-right`
screen right. The innkeeper definition JSON is byte-identical to the preceding
delivery; no schema, simulation or NPC content fingerprint change is needed.

The bounded background Blender export passes, with all nine models and no stale
mesh parts. Position/normal hashes for every model except the two resized models
match the previous export exactly; bar/shelf height ratios are .85 within export
rounding. Blender's sandbox-denied thumbnail/extension-cache writes do not affect
the verified packed source, GLB, atlas or runtime exports. Counter and shelf object
width/depth also shrink by 15%; shelves retain their wall pivot and both keep
their authored placement/collision reservations.

Hidden NVIDIA/OpenGL captures at the same front pose show the furniture change.
Close side views now show the resident looking screen left when viewed westward
from (7,3), and screen right when viewed eastward from (5,3), consistent with his
south facing. Reports confirm cell (6,3), subcell (1,1), feet (6.75,3.75), and zero
remaining texture/alpha leases after release. Normal travel and offensive-picking
exclusion checks pass. No renderer code changed, so the preceding GPU parity and
unaffected gameplay checks are reused rather than rerunning a matrix.

The [six-file gallery](screenshots/innkeeper/README.md) now retains one matched
before image and five corrected images, replacing superseded evidence. Initial
runtime visuals remain rejected; the approved character concept is unchanged and
the corrected runtime presentation awaits user acceptance. Owned disposable exports,
comparison snapshots, logs and unused captures are cleaned after selection.
