# R04 — Doors, effects and campaign integration

Status: **Implemented; Ready for review** (2026-10-31).
User approved the reviewed R04 plan with “Implement the plan.”
Parent: [renderer roadmap](renderer_plan.md). R03 is Accepted and closed.

## Ownership and timing

GameSession owns a bounded 32-fixture opening snapshot and one queued graphical
movement action. Completing a door/gate permanently updates the existing fixture
and starts a 300 ms session-clock opening barrier. Pause freezes the clock;
animation-off settles openings. Restores contain settled permanent fixtures only;
save/travel reject during an opening. Completion/rewards remain authoritative.

Both rendering backends and headless sessions share this scheduling rule. Existing
accepted impacts/effect deadlines continue; new movement, contact/facing, AI and
combat actions wait. Graphical input retains one fresh movement; headless commands
remain rejected without queuing. Pending input clears on pause/contact. Crossing a
barrier gives movement/AI only the unblocked remainder. Admission, previews, HUD
and automation must agree. Final campaign presentation waits for the opening.

## Geometry, effects and interfaces

FrameInput borrows door snapshot, attack visuals, SpellArea/GroundSnapshot and time.
WorldFrame owns transformed door instances alongside existing billboard/depth/IDs.
Door geometry is shared CPU/GPU: facing-oriented frame, left hinge near entry edge,
0.06-unit posts/thickness, 0.08 lintel, leaf filling the aperture and swinging inward
90 degrees within its cell. Static base-map meshes never rebuild on opening.
Frames/slabs write opaque depth; only closed slabs own fixture IDs. Picking and
previews are suspended during opening. Legacy holds its closed proxy until settled.

A separate bounded EffectFrame builder reuses ProjectilePlayer visual admission:
260 shots, 16 ground patches with two layers, nine preview cells, four spectral
weapons. Effects reuse actor fitting/attachments and preserve source timings,
misses, flight arcs and tails. Ground/shadows precede body color; translucent effects
follow with stable depth ordering, premultiplied blending and no depth/ID writes.
Effect color invalidation does not force visibility work. Assets bake offline into
format 1 / appearance revision 3; runtime never decodes source art.

Keep 8 MiB scratch per view, registry capacities and 352 MiB subsystem ceilings.
Count capacity, share immutable assets, preallocate packets/sorts, reject preparation
exhaustion, retain deterministic newest-projectile cosmetic overflow. No frame
resource uploads or engine-owned allocator traffic.

## Integration and transactions

A tagged presentation adapter shares gameplay scheduling, picking, ground queries
and native Presenter composition. --renderer legacy|world defaults legacy until
R06; supersampling defaults on and off selects native filtered. Backend choice is
per launch; only the selected asset banks load. One world owner shares resources
between active/staged scenes. Destination preparation/first draw precede campaign
commit; publish without allocation; destroy old view before old borrowed session.
Preparation failures retain session, Party, image, IDs and save.

New Game, arena/rematch, stairs/recall/waypoints, save/Continue and captures use the
selected renderer. Replay captures own a hidden context and never advance time.
CLI/client/local MCP expose world visibility; defaults stay legacy. Protocol/save
schemas stay unchanged; strict traces record world-visibility-v2, appearance 3 and
shared door scheduling identity. Opening diagnostics participate in strict replay.
No graphics context is needed for world observations; capture requires graphics.

## Validation and handoff

Generated owned fixtures cover barrier boundaries/pauses/animation-off/queued input,
AI/contact/accepted impacts, locks/keys/gates, settled save/restore, all door headings,
CPU/GPU depth, camera clearance, effect ordering/coverage/overflow and preview rules.
Exercise campaign/arena transactions and injected CPU/GPU preparation failures,
CLI/client/local MCP/replay/capture identities, warm allocation/upload guards and
separate capacity/performance workloads. Run graphics/headless Debug/Release suites,
focused sanitizers and incremental asset checks. Inspect hidden application captures
and deterministic motion; never automate desktop input. Record actual checks and
limitations in r04_verification.md and update this document to the final result.

R04 ends Ready for review; only user acceptance closes it and R-B. No R05, new
monsters, balance work, PC07, legacy retirement or commits are authorized here.

## Implemented interfaces and evidence

`DoorSnapshot` contains the fixture world, revision, current session clock and 32
opening deadlines. GameSession starts it at permanent door/gate completion,
`actionBusy()` shares admission, and a separate pending graphical move follows
existing pause/contact clearing. Rehydration resets deadlines at the restored clock.
`GeometryView::presentation` is a render-only borrow retaining legacy closed proxies;
logic continues to use the authoritative base/effective geometry view.

`world_render::doorBoxes` emits at most 128 affine boxes from fixed fixture metadata.
CPU triangles and GPU unit-cube models use those same bases. FrameInput carries
read-only doors/attacks/area/time; WorldFrame retains door instances and visibility.
EffectsPipeline separately retains up to 313 packets and ProjectilePlayer storage;
its color revision does not invalidate visibility. Bodies are indexed once by actor
slot for attachment lookup. Per-view visibility plus effects storage is 4,924,888
bytes, with the original registry/subsystem budgets enforced before publication.

`GameView` owns a legacy FramePipeline or borrows the shared active/staged WorldRenderer.
It exposes common update, picking, ground-query and native presentation operations.
Its active-owner identity prevents an old view destructor from clearing a newly
committed scene; staged cancellation releases only the candidate. Presenter forwards
world images through the same native UI/map composition. Shared resources die before
the registry/context; view borrowers die before owned or ticket-retained sessions.

World appearance revision 3 includes ordinary/locked door materials, grease/web/
spectral art and code-authored reusable arrow/strike/glint/preview textures. No save
format changed. Protocol remains 1, visibility advances to world-visibility-v2 and
strict trace headers include shared-opening-300ms-v1. Local MCP forwards an explicit
world backend and retains it through reset/load. Replay captures own their context
and sample actual session state without advancing time.

Integration exposed two diagnostic gaps: authored fixtures with discovery disabled
were omitted from visual observations, now corrected without changing generated
knowledge; and old spell captures attempted level-two spells on starting builds.
Capture preparation now advances a legal Party caster before those independent
specimens and gives Lesser Restoration a real affected recipient. Historical focused
capture recipes beyond doors/spells/projectiles/menu/arena/save stay legacy and
reject world selection explicitly; world replay covers arbitrary recorded boundaries.

[Verification/handoff](r04_verification.md) and [gallery](screenshots/r04/index.html)
record actual tests, resource/failure guards, strict expedition parity, captures,
versions and limitations. R04/R-B await user acceptance; R05 is not started.

## R04 effect review correction — 2026-10-31 (implemented)

User review identified clipped Spiritual Weapon art and apparently absent Scorching
Ray/Burning Hands effects. The persistent weapon's previous .8-unit center with
.65-unit height intersects the flat one-unit ceiling. Center it at .5, preserving
its size and leaving .175-unit floor/ceiling clearance. Keep world depth testing.

Restore the existing legacy spell tint palette and 120 ms impact fade in GPU
packets. A neutral white prepared glint takes that tint (appearance revision 3;
format/save/visibility contracts unchanged). Bias impact quads .002 units toward
the camera so the target billboard does not reject its own coplanar impact. Nearer
walls/bodies continue to occlude them; effects still write no depth or IDs.

Preserve multi-dart glyphs and separate simultaneous Scorching Ray lanes. A four-
character party can add at most eight extra glyphs over the 260 retained visual
records: reserve 313 effect packets including existing ground/previews/weapons.
Reuse fixed storage, sorting and quad draws. No new particle framework or flame-cone
art; Burning Hands retains its existing brief fiery impact flashes. Capture phases
must use actual release/impact timestamps and show its first 120 ms instead of a
650 ms aftermath. Include contact/motion evidence, CPU bounds/timing/palette/burst
regressions, GPU effect visibility and occlusion, and allocation/resource guards.


The correction is implemented. CPU timing/palette/multi-glyph/clearance regressions,
313-packet allocation stress, generated-art GPU impact visibility/fade/expiry and
existing wall occlusion pass. Release captures now include actual 117/234/294/354 ms
Scorching Ray phases and 0–120 ms Burning Hands pulses. Updated images and sampled
real-time previews are in the gallery. No new spell artwork or cone system was added.
See the [correction verification](r04_verification.md#spell-baseline-review-correction).
