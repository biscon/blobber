# Friendly NPC dialogue

Status: implemented and mechanically verified; awaiting runtime visual/interactive review, 2026-11-06. The user approved the
[mockup](mockups/dialogue-v1/README.md), gradual reveal/talk animation, a 700 ms
camera approach, and fixed world viewport with conversation panels overlaid.

## Ownership and interfaces

Immutable DialogueCatalog JSON is loaded during preparation, independently of
NPC artwork/publication. The application owns it for the entire gameplay run.
A --dialogue-dir override supports editing source JSON and restarting without
compilation; otherwise the normal runtime assets/dialogue directory is used.
Stable resident IDs identify characters, not artwork or ephemeral actor handles.
CampaignRun owns bounded DialogueHistory and checkpoints it. CommandController
owns transient DialoguePlayer and publishes a read-only HUD; GameSession tracks
the active resident to reject unsafe save/travel/movement. Teardown clears Talk.

Catalog entries supply id, name, greeting, introduction, topics and optional
camera framing. Topic responses supply pages, optional when_fact and set_fact;
a matching fact response wins over the required unconditional default. No script
language, quest system, trading or procedural rumour generation is introduced.
Intro is marked seen on entry, replayable through About yourself. Completing the
last response page, including revealing it instantly, records completion/fact.
Interrupting before then does not. History is stored by stable keys so prose edits
do not invalidate saves. Save schema/simulation are 12/16; no migration.

Limits: catalog 1 MiB, 64 characters, 16 topics/character, 64 facts/character,
8 authored pages/response, 1024 ASCII bytes/authored page. Immutable layout may
allocate during preparation; runtime uses bounded arrays/indices and no growth.
Wrap full text before reveal, paginate rather than clip, and reuse prefix buffers.
History has 64 character records with at most 64 completed topic/fact keys each;
exhaustion rejects before mutation. Friendly NPC identities must be unique.

## Flow and presentation

Frontal exposed residents within 2 cells can be spoken to with F or friendly
body picking, separate from offensive picking. Doorway F actions have precedence,
then talk, then other fixtures. Commands revalidate world, role, pose, visibility
and settled exploration. Queued movement/targeting is cleared. Conversation
freezes simulation while a focus-aware presentation clock advances only the
speaker's idle/talk, text and camera; other actor timelines stay frozen.
First entry automatically reveals the introduction; later entries reveal a short
greeting with topics immediately available. Reveal defaults to 45 characters/s.
Click response/Space completes reveal, next press advances; final page remains.
Topic click or 1..7 interrupts immediately; arrows/Enter select; scroll/PgUp/PgDn
handle 7 visible rows. Skip introduction skips all pages; Escape/Farewell leaves.
Focus/input edges cannot leak between pages, underlying controls or movement.

Panels use the existing native 1920x1080 pipeline and logical 640x360 layout:
response (4,224,306,134), topics (314,224,190,134). They cover the lower world
view and replace the dock/log; render targets and party sidebar never resize.
Panels fade over 150 ms; greeting begins then, during the 700 ms smoothstep
camera approach. Talk is active exactly while revealing; pauses between pages
and topic choice use Idle. Closing reverses from the current interpolated camera;
fresh movement can finish exit without a forced wait.

## Camera

FrameInput carries an optional effective camera and transition flag. CPU
visibility, GPU drawing, culling, clipping, sprite orientation, depth and sky use
that same camera and invalidate caches on changes. Default exploration remains
84-degree horizontal FOV, accepted .4 horizon and .4-cell pullback. A per-camera
horizon provides upper-view composition without introducing pitch or resizing.
Target is .6 cells in front of the resident. Registered body bounds and acquired
alpha bounds determine visible standing height and head framing, respecting cropped
and padded canvases. Default eye height is 92% of visible standing height above
the NPC floor, adapting to the character. Character JSON can override distance
or eye height for unusual artwork. The innkeeper uses the generic defaults;
the user rejected the earlier .75/.33 counter-based framing.
The prepared scene checks the camera segment for obstruction and clamps travel
before geometry with near-plane clearance; furniture occupancy alone is not a
camera blocker above the counter. Party/NPC positions, scales and facing stay.

## Verification

Owned small fixtures cover parser bounds/errors, independent same-art residents,
conditions/history/interruption, input edges, reveal and Talk/Idle, pause/focus,
frontal admission and stale/hostile rejection, save/travel, camera interpolation,
obstruction, default projection and allocation-free warmed updates. One Release
configuration, at most two build workers/one test process; affected CPU tests,
extended campaign persistence once, hidden CPU/GPU projection parity and selected
application-owned captures. No desktop input/focus automation. Retain up to six
selected images; log final commands/outcomes/limitations here and clean the owned
agent run. Interactive acceptance remains user-owned.

## Verification completed — 2026-11-06

All work used one Release configuration, two build workers and one test process,
with `nice -n 15 ionice -c 3`; captured/review jobs bounded OMP/OpenBLAS to one
thread. Approximately 21 GiB available RAM and 453 GiB disk space were checked.
The final production build passes without compiler warnings. The Release headless
consumer is also built to check the shared command/save interfaces.

- Built `blobber`, `blobber_review_inn`, dialogue and affected CPU test targets,
  plus `blobber_world_visibility_gpu_tests` with `cmake --build --preset release`.
- `ctest --preset release -R '^(dialogue|inn|residents|buttons|animation|commands|hud|npc_metadata|world_visibility|options|campaign_save_smoke)$'`
  passes all 11 suites in 2.42 s. After final parser validation, friendly-action
  routing and additional future-NPC/capacity coverage, only the affected
  `dialogue|buttons|commands` selection was rerun: all 3 pass in 0.05 s.
  A final parser-only regression checks that the default panel fade cannot exceed
  a shortened camera transition; its focused dialogue suite passes separately.
- `ctest --test-dir cmake-build-release -R '^campaign_save$' -j 1 --output-on-failure`
  passes the extended persistence integration check in 6.90 s. Intro/fact state
  survives disk encode/decode/Continue and travel; old schema rejection remains.
- Hidden `blobber_world_visibility_gpu_tests` passes on NVIDIA RTX 3060 / OpenGL
  3.3, including source/mid/end camera overrides and restoration: zero ID boundary
  pixel differences, exact visible membership, lifecycle and allocation checks.
- Hidden `blobber_review_inn cmake-build-release/assets NEW_OUTPUT dialogue`
  produces the [six selected runtime captures](screenshots/dialogue/README.md).
  Source (6.5,.5,5.9) approaches target (6.75,.33,4.5), horizon .392113;
  weight is .5 at 350 ms and 1 at 700 ms. Game time remains zero. Talk is clip 7
  during reveal and Idle clip 0 after revealing a topic. Return visits omit the
  introduction. Released resident texture/alpha counts are zero.
- Source and packaged dialogue JSON agree after the final framing adjustment;
  the `--dialogue-dir` option and missing-argument handling have focused coverage.
  `git diff --check` passes.

The first CPU iteration exposed a test sampling two timestamps in the same
animation frame; the fixture now samples across an actual frame boundary. Hidden
captures also identified generic modal dimming and transparent-canvas head
framing; both were corrected before selecting evidence. The final text footer
says Choose a topic when no additional page remains.

Routine checks use generated, owned fixtures, not game assets. The second friendly
fixture uses shared artwork with independent identity, another facing axis,
shorter body and different camera eye/distance. Allocation guards cover warmed
reveal, history/snapshot updates, interpolation/visibility and controller work;
they make no claim about graphics-driver or third-party C allocations.

No desktop input/focus automation was used. Interactive timing, camera comfort,
close-up art quality and runtime visual acceptance remain user-owned. Geometry
path checks are conservative over prepared scene triangles (including cutout
surfaces); no general camera navigation/pathfinding or pitched camera was added.
Trading/quests/audio speech/live reload remain outside this slice. No additional
build configuration, stress workload or benchmark was run.

The owned run's disposable logs, intermediate captures and failed/trial evidence
are removed after evidence selection with preview/apply cleanup. Exactly six PNGs
remain in the dialogue gallery; existing innkeeper evidence and source mockup/
artwork/prompts are preserved. No retained diagnostic exception is needed.

## Eye-level review correction — 2026-11-06

The user accepted the interface and fluid transition, but rejected the low camera
as a frog-on-the-counter view. They explicitly dropped counter-based framing and
reiterated that conversation must work for generic friendly NPCs.

Remove the innkeeper's .75/.33 framing override. Default
camera distance stays .6 cells; omitted eye_height now derives an eye line at
92% of the NPC's visible standing height above its floor, using registered body
and acquired alpha bounds. An explicit positive eye_height remains available for
unusual artwork. Compute height before constructing/checking the camera path;
never lower the eye to retain furniture. The existing head framing, shared level
projection, fixed viewport, 700 ms easing, UI and gameplay/history remain intact.
No schema bump or new campaign is needed. Verify the default on tall and short
owned fixtures, retain override coverage, and inspect a hidden inn capture.
Replace superseded evidence with the corrected view; keep a matched rejected
baseline within the six-image gallery. New framing awaits user review.


Verification of the eye-level correction: Release `blobber_dialogue_tests` and
`blobber_review_inn` build without warnings. `ctest --preset release -R '^dialogue$'
passes in 0.04 s, covering tall/short automatic eye lines, explicit override,
distance-only JSON retaining auto height, obstruction, interpolation, input,
history and warmed allocation checks. The production `blobber` build is refreshed.
The hidden inn runner verifies target (6.75,.543602,4.35), horizon .127214, .5/1
camera weight at 350/700 ms, Talk/Idle, frozen gameplay and zero released texture/
alpha retention. Five corrected captures and one matched rejected baseline replace
the previous six images. The source/package dialogue JSON agree.

Projection math, GPU camera interface, UI and save format do not change in this
correction. Existing GPU parity coverage is reused; no full test matrix or save
suite is rerun for this presentation-only change. The user explicitly approved the
interface/fluid transition; the new eye-level framing remains pending review.
Disposable output for this correction is cleaned after selecting those six images.
