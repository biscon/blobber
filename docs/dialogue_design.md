# Friendly NPC dialogue

Current revision: [multi-NPC event beats](#multi-npc-event-beats), implemented and mechanically verified. The original delivery and its review history are preserved below.

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
freezes simulation while a focus-aware presentation clock advances the speaker's
idle/talk, other residents' idle loops, text and camera. Hostile actor timelines
stay frozen. Resident idle sampling retains elapsed dialogue time after closing
to preserve phase continuity (see the Sanctuary resident design).
Ambient streams also continue at the configured volume during dialogue, including
entry/exit and pauses between pages. Other pause causes and focus loss still pause
ambience; gameplay effects remain frozen. See the audio design's dialogue correction.
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

## Multi-NPC event beats

Status: implemented and mechanically verified; awaiting user timing/listening review (2026-11-06). This supersedes the
single-speaker JSON authoring and Talk-state portions above; historical checks
remain evidence for the earlier delivery, not verification of this revision.

One YAML file per conversation host owns participants (local alias, stable resident
ID, name, text color and optional camera framing), greeting/introduction and topics.
Responses hold up to 32 authored beats with speaker/text and optional camera target,
duration, animation and audio path. Audio paths are relative to the shared assets/audio directory.
Scripts are immutable after preparation; no live reload or legacy JSON loader.
Retain stable host/topic/fact keys and existing save data. Total source capacity is
1 MiB, 64 hosts, eight participants/host, 16 topics and 64 facts/host, 1024 ASCII
bytes/beat and 64 laid-out pages/response. Reject capacity exhaustion at preparation.

DialoguePlayer owns progression/history and a monotonically increasing beat serial.
Prepared pages retain beat indices: wrapping never repeats camera/audio/reaction
cues. CommandController binds participant IDs to current-world ActorId handles,
revalidates selected responses and publishes presentation overrides. Primary host,
speaker and camera target are distinct. Missing actors/clips reject a response
before mutation; stale active handles close the conversation. Only host topics
and facts are affected. Speaker-only participants do not gain interaction prompts.

Talk and Laugh, whether default or explicit, run only during text reveal and then
return to natural Idle. Explicit Walk holds until replacement. Clips start at beat
entry; wrapped pages pause in Idle and resume without replaying event cues.
NpcAnimator consumes a view-only override,
not mutations of authoritative NPC state. All outgoing overrides clear on advance,
topic replacement, skip and exit. Resident idle phases remain continuous.

A view-owned DialogueCameraController captures exploration and primary framing;
entry retains 700 ms; outgoing and returning pans use 200 ms. Scripts can
request longer durations; playback clamps shorter values (including zero) to 200 ms. Retarget from the effective current camera,
use shortest-angle smoothstep interpolation, and return to exploration on exit.
Every move checks prepared geometry/near-plane clearance. Initial framing clamps;
blocked transitions between safe endpoints cut instead of passing through geometry.
CPU/GPU visibility, drawing, clipping and orientation share the effective camera.

Text, animation, sound and camera cue commit together on the presentation update;
initial speech waits for panel fade. First click reveals, next advances. Focus loss
freezes the presentation clock. Audio preparation loads referenced samples into
bounded runtime storage under the existing PCM ceiling. One dialogue voice uses
Effects volume, bypasses gameplay pause, pauses on focus loss/other modals, and
stops on beat replacement/exit/world change. Missing audio degrades to diagnosed
silence. Gameplay effects stay paused and ambience remains continuous.

No frame-time parsing, file access, allocations or resource creation. Immutable
vectors/layout and audio resource acquisition belong to preparation. Small bounded
indices/handles carry presentation; no general scripting VM or event bus is added.

Deliver migrated innkeeper/mayor scripts and a mayor/guards running gag using
existing Talk/Laugh artwork and provisional authored reaction audio. No new quest
mechanics, direct guard conversations, actor movement or full voice acting.

Verification: owned YAML/error/capacity fixtures; wrapping and exactly-once cues;
host fact continuity; speaker/color/animation reset; rapid retarget/exit/obstruction;
focus/audio teardown and missing-device paths; allocation guards. One Release
build at two workers, serial focused dialogue/resident/animation/command/UI/audio/
camera tests and save smoke once. Hidden application-owned camera/cutaway captures
and GPU parity; interactive visual/listening acceptance stays with the user.
Retain at most six selected images and clean the owned artifact run.

### Multi-NPC implementation and verification — 2026-11-06

`DialogueBeat` holds authored text, participant indices, optional clip/duration and
a prepared audio index. `DialogueResponse` retains immutable beats plus flattened
prepared pages with beat indices. `DialoguePlayer` exposes beat serial/elapsed time
and cue readiness; first visible glyph and cues share the first update after fade.
An early reveal click waits for that fade before displaying text or awarding facts.
Subsequent beat entry shows its first glyph immediately. Continuation pages keep the
same beat serial. The admission callback validates required current-world residents
and explicit clip availability before response selection. Short-lived actor lookups
return world-checked handles; there are no retained actor pointers.

`NpcDialogueOverride` carries one actor/clip/elapsed value. The ordinary actor
animation state is unchanged, so dropping the override restores natural Idle.
`DialogueCameraController` owns exploration/primary/current/transition endpoints
and uses conservative segment checks. Tests cover cutaways, independent camera
targets, minimum pan durations, blocked connections, retargeting and interrupted exit.
Presentation settings remain catalog-wide; optional declarations across YAML files
must agree. Prepared storage growth is confined to loading and layout.

AudioRuntime owns up to 64 dialogue source/alias pairs and one active dialogue
voice, separate from its 64 ordinary one-shot slots. Dialogue and base samples
share the 512-source/64 MiB decoded PCM limits. `prepareDialogue`/`clearDialogue`
are preparation/teardown boundaries; `presentDialogue` deduplicates beat serials.
The separate pause argument defaults to paused for existing menu/settings callers;
gameplay explicitly enables it only for dialogue without another modal. Focus loss
pauses all sound. The Effects slider controls dialogue volume. Missing files/device
or alias failure degrade to diagnosed silence. Implementation is separated into
`platform/dialogue_audio.cpp`; no dialogue audio is serialized.

Bundled content migrated to `assets/dialogue/{innkeeper,mayor}.yaml` with identical
preexisting resident/topic/fact IDs. The guards topic adds `heard_guards_lunch` for
its repeat response, with no save/schema/simulation bump. Only the primary host
becomes interactable. The managed-build retirement helper removes obsolete copied
`dialogue.json`, and an authoring guide documents defaults and limits. The mayor's
angry Talk and both guards' Laugh use existing accepted artwork. Three original
Csound/SoX placeholder reactions are retained with source and regeneration commands;
all are mono 48 kHz PCM16, non-silent, low-DC, peak -9 dBFS with endpoint fades.

YAML uses the compatible installed package. The reproducible fallback is pinned to
[upstream 0.8.0 commit f732014](https://github.com/jbeder/yaml-cpp/commit/f7320141120f720aecc4c32be25586e7da9eb978);
network fetching of the fallback was not exercised in this environment.

Verification used one Release configuration, at most two build workers, one test
process, low CPU/I/O priority and one-thread capture/audio environments. About
20 GiB RAM and 452 GiB disk space were available before build/asset work.

- Built the game, headless consumer, relevant tests and production capture runner.
  Compiler output contains no warnings.
- Initial focused `dialogue|residents|animation|audio_runtime`: four pass in 0.09 s.
- Integration `dialogue|commands|buttons|hud|audio|campaign_save_smoke`: six pass in
  1.67 s. Existing campaign schema/compatibility remains unchanged.
- Final dialogue/audio/inn checks exposed an old pre-fade greeting expectation and
  an inactive-controller teardown regression. The greeting assertion now tests
  synchronized fade/cue behavior. GDB localized the crash to controller destruction:
  cleanup must not inspect a borrowed session after a travel handoff unless this
  controller owns dialogue. The fix preserves the prior inactive-lifetime contract.
  Audio runtime passes; the final `dialogue|inn` selection passes in 0.36 s, including
  innkeeper history disk roundtrip/travel, explicit missing clips and Talk fallback.
- Owned fixtures cover YAML multiline text, references, malformed cues/colors,
  traversal rejection, capacities, independent speaker/camera, continued pages,
  history, focus, interruption, Idle reset and zero warmed engine C++ allocations.
  Audio tests cover exactly-once playback, modal/focus pause, missing/disabled
  device, missing files, alias failure, teardown and warmed allocation checks.
- Hidden `blobber_world_visibility_gpu_tests` passes on the available OpenGL
  desktop: lateral displacement, yaw, horizon and restored camera share CPU/GPU
  transforms with **zero boundary pixel differences**, including resource lifecycle.
- Hidden `blobber_review_sanctuary_city ASSETS RUN/capture dialogue-beats` passes.
  Primary camera is (9.85, .526579, 8.75), Boric (9.35, .567434, 8.25), and Balin
  (9.35, .47551, 9.25). The 80 ms cutaway midpoint is (.5 interpolation), the
  primary return is identical, and outgoing clips return to Idle. Host fact/repeat
  selection and released scene leases pass. [Six inspected images](screenshots/multi-npc-dialogue/README.md)
  retain these cases.

The sandbox initially could not open display :0; approved hidden checks ran with
display access. Debugging traced only the owned headless test process. No desktop
mouse/keyboard/focus automation, benchmark, stress or second build configuration
was used. Interactive feel and physical-device listening remain user review;
application-tick synchronization does not claim zero audio-device latency.

Disposable build/test/debug logs, synthesized intermediates and trial captures are
cleaned after promoting the six images and three selected audio assets. Source
recipes and prompts/artwork remain durable; no diagnostic exception is retained.

### Reveal lifetime, slower pans and audio placement correction — 2026-11-06

Status: implemented and mechanically verified; interactive feel remains pending review. User review rejects reactions
looping after text reveal and dialogue pans shorter than 200 ms. Explicit Talk
and Laugh now follow the same reveal lifetime as default Talk: return to natural
Idle on automatic or click-to-complete reveal. Wrapped display pages pause in Idle
and resume the same beat's clip during the next page's reveal without replaying
audio/camera cues. Explicit Walk/Idle cues retain their existing behavior.

Outgoing and returning pans default to 200 ms. All authored pan durations,
including zero or older short values, are clamped to at least 200 ms at playback;
the 700 ms entry/exit stays unchanged. Presentation transition configuration must
be at least 200 ms. Obstruction safety cuts remain cuts, not movement through walls.

Reaction audio and its source/guide move to `assets/audio/dialogue`. Script `audio`
paths resolve under the shared audio root, e.g. `dialogue/boric-chuckle.wav`. The
loader accepts an explicit audio root; normal gameplay always supplies the game's
assets/audio even with --dialogue-dir. Other callers default to a sibling audio
directory. Path confinement applies to the audio root. Retire only the known old
copied files under managed build assets/dialogue/audio; preserve unrelated files.

Verify focused dialogue tests for natural and instant completion, wrapped pages,
minimum/default/custom pan durations and audio-root resolution. Refresh the hidden
production review to show Laugh during reveal and Idle afterward, then replace
superseded gallery images. Reuse unchanged GPU projection/audio transport tests;
no full configuration matrix. Record results here and clean the owned run.

Verification of this correction: Release game, dialogue test and hidden city review
runner build successfully with two low-priority workers and no compiler warnings.
`ctest --preset release -R '^dialogue$'` passes in 0.05 s. Owned fixtures cover
natural/click completion of explicit Talk/Laugh, wrapped-page Idle/reveal,
allocation-free updates, 200 ms outgoing and return endpoints, clamped zero
durations, preserved 400 ms custom pans and shared audio-root resolution.

The updated hidden `dialogue-beats` runner passes: Boric Laugh while revealing,
Idle on natural completion at the same camera/beat serial, Balin Idle after
click-to-complete, mayor Talk-to-Idle, and both pan directions still moving at
100 ms and settled at 200 ms. Game time stays zero and scene leases release.
Six refreshed captures replace the superseded six in the existing gallery; user
approval is still pending. Unchanged GPU projection and audio transport coverage
is reused, with no broader test matrix or new sound generation.

All three WAVs are byte-identical after relocation and match their packaged
`assets/audio/dialogue` copies. YAML references resolve there, including with an
explicit script-directory override; old packaged sound copies are retired.
No desktop input/focus automation was used. Build/test/capture logs and temporary
images are cleaned after selecting evidence; no retained diagnostic exception.


## OnlyScrolls portrait briefing

Status: implemented and mechanically verified; artwork/timing acceptance pending,
2026-11-06. User selected centered
close-up, two dedicated sprite clips and a new campaign rather than save migration.

The Gwendolyn topic becomes the supplied five-beat Mayor/Boric/Balin/Boric/Mayor
scene. Pans are 200/300/200/200 ms; placeholder sound references stay relative to
assets/audio. Existing topic identity/history remains; no quest state is introduced.

DialogueBeat adds a typed optional overlay index and clear flag. Catalog-owned
canonical paths are deduplicated, bounded to 16, resolved against the shared UI
root (explicit in gameplay; sibling ui by default). YAML overlay_asset and
clear_overlay are mutually exclusive. Absolute/escaping paths and invalid types
reject. DialoguePlayer owns the persistent index and applies cues once, with
optional YAML delay and Next-dismissal policy (see the timing correction below).
Default cues apply at the first visible glyph and persist across pages/beats.
Response replacement, skip, leave, close and rebind always clear the image. HUD
publishes an index only.

UiArt owns up to 16 GpuLease textures prepared before gameplay, with 2048-pixel
per-axis and 32 MiB total decoded RGBA bounds. Failed preparation releases acquired
textures and reports the asset. Presenter forwards preparation/clear boundaries;
gameplay scope teardown clears the set. Rendering fits one transparent image into
a centered 1020x570 native-pixel box above dialogue panels, without intercepting
input. No per-frame parsing, decoding, resource acquisition or allocation.

Append PullDocument and ExplosiveRage enum values; preserve existing numbers.
Resident/front-only validation and directional Idle fallback include both. Boric's
four 150 ms frames play once then hold; Mayor's 120/100/120/160 ms rage loops. Both
are cosmetic overrides, end on reveal completion/skip or replacement/exit, and
use beat time so pagination cannot restart the document pull. Update appearance
publication revision and republish. NPC-definition fingerprints intentionally make
old campaigns incompatible; do not remove saves or weaken compatibility checks.

Use built-in imagegen and the sole approved character sheets. Parchment matches
Gwendolyn v3 (adult 25), face/hair/costume, bored turkey-leg pose, worn paper and
raven-letter stamp, with real transparency. Production asset belongs under
assets/ui/dialogue; source prompts and animation masters remain durable assets.

Verification: owned parser/path/capacity/state fixtures, page/reveal/focus and
clip-boundary tests, allocation guards; UI decode/resource-failure/teardown checks;
focused Release dialogue/animation/resident/UI/save checks; hidden production
captures of five beats and restored exploration, with frozen simulation, camera
timing and resource checks. At most two low-priority build workers and one heavy
job; no OS desktop input. Keep at most six review captures and clean the owned run.

### OnlyScrolls verification — 2026-11-06

Implemented the design above. PNG IHDR dimensions and encoded-size limits are
checked before decoding. Texture replacement is transactional: staged leases
roll back on failure and leave the previous prepared set intact. `UiArt` owns the
leases and `Presenter` forwards preparation/clear; gameplay establishes its scope
guard before either overlay or audio preparation. Existing scripts remain valid.
Both new clip values append to the enum (9/10), and publication appearance revision
is now 8. Existing numeric animation values and save schema/simulation 12/17 stay
unchanged; the intentional NPC-content fingerprint change requires a new campaign.

All three assets were generated using built-in imagegen and approved reference
sheets, with exact prompts retained beside masters. Decoded dimensions/alpha:
parchment 1536×1024 RGBA (0–254), Boric 1086×1448 (0–255), Mayor 1199×1312
(0–255). Registration retains all previous definitions and adds only the new
front poses. Publication succeeds with 525 textures; nearest-sampling extraction
validates canvas bounds. No procedural substitute artwork was used.

Commands and results (one job at a time; two low-priority build workers):

- `nice -n 15 ionice -c 3 cmake --build --preset release --target blobber blobber_dialogue_tests blobber_residents_tests blobber_gpu_smoke blobber_review_sanctuary_city blobber_campaign_save_tests -j2`
  (targets built in focused batches): pass, no compiler warnings.
- `ctest --preset release -R '^(dialogue|animation|residents|campaign_save_smoke)$'`:
  all four pass in 1.70 s. Coverage includes owned YAML/path/symlink/capacity
  fixtures, reveal/focus/page/response/exit transitions, both authored clips,
  directional Idle fallback/picking, allocation guards, and save compatibility.
  Multi-type visibility fixtures avoid creating unused new reactions for every
  appearance, retaining their bounded workload.
- `OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 cmake-build-release/blobber_gpu_smoke`:
  passes in a hidden OpenGL window. Owned PNGs verify alpha/aspect fit, invalid
  IDs/files/headers, dimension/count/decoded-byte limits, injected acquisition
  failure rollback, no draw-time uploads and idempotent lease release. The old
  smoke fixture lacked current effect atlases and assumed obsolete 8/512 target/
  texture capacities; corrected owned fixtures and shared capacity constants.
- `OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 nice -n 15 ionice -c 3 cmake-build-release/blobber_review_sanctuary_city cmake-build-release/assets RUN/capture dialogue-beats`:
  passes in a hidden window using only application-owned input. Five sequential
  beats, 200/300/200/200 ms pans, document final-frame hold, natural/click Idle
  resets, immediate rage/overlay clear, replay, unrelated-topic replacement, exit,
  frozen game time and scene/overlay lease release pass.
- Six production captures visually inspected and retained in the
  [OnlyScrolls gallery](screenshots/onlyscrolls/README.md). Transparent edges,
  complete portrait, readable text, unobstructed controls and rage are visible.
  The centered close-up intentionally occludes most of the guard.
- `git diff --check` passes. Production source/package PNGs and script match.

No OS input/focus automation, broad configuration matrix, benchmark or unrelated
full save suite. Unchanged audio transport and camera projection coverage is
reused; physical listening, interactive comedic timing and derived-art acceptance
remain with the user. User saves were not opened or removed. Selected source
artwork/prompts and six review images are durable; task logs and scratch output
are cleaned with the artifact helper, with no retained diagnostic exceptions.


### Overlay timing correction

Implemented and mechanically verified; interactive review pending. User rejects the close-up covering the camera pan
and document-pull animation, and persisting through the guards' remaining lines.
The correction must be authored in YAML, without conversation-specific C++ code.

Add optional overlay_delay_ms (integer 0..60000, default 0) and
overlay_dismiss_on_advance (boolean, default false); both require overlay_asset.
DialoguePlayer uses its focus-aware beat clock for the delay and a small boolean
to remember the active overlay's dismissal policy. The first advance input clears
the image (or cancels its pending show), while retaining normal text-reveal/page
advance behavior. Cancellation lasts for the authored beat, including subsequent
pages; new responses reset it. Existing uncued/persistent overlays keep defaults.
No renderer changes, per-frame allocation, animation-name checks or conversation
IDs are introduced.

The Boric document beat declares overlay_delay_ms: 600 (four 150 ms frames, longer
than its 200 ms pan) and overlay_dismiss_on_advance: true. Other guard beats do
not show the picture again. Timing is explicit content, not inferred from actor
identity. Verify generic delay boundaries/focus/cancellation/pagination/replay and
parser errors in dialogue tests; refresh six hidden production captures showing
the unobstructed pull, delayed close-up and subsequent guard conversation. Reuse
unchanged rendering/resource tests and clean the owned artifact run.

Verification of the timing correction: Release game, dialogue tests and hidden
review runner build without compiler warnings (`cmake --build --preset release
--target blobber blobber_dialogue_tests blobber_review_sanctuary_city -j2`, nice 15 /
ionice idle). `ctest --preset release -R '^dialogue$'` passes in 0.06 s. Owned
fixtures cover delay boundaries, paused focus clock, first-Next reveal/advance,
early cancellation, page continuation, subsequent beats, replay, natural text
completion, response replacement/exit, malformed YAML options and allocations.
Existing default immediate/persistent overlay coverage still passes.

The hidden `blobber_review_sanctuary_city ... dialogue-beats` run passes with
bounded OpenMP/OpenBLAS threads. Production checks see no overlay during the
200 ms pan or at 599 ms, then see it at 600 ms. First Next hides it without
changing beat serial, and both following guard beats stay unobstructed. Replay
respects the delay; early Next cancels pending show. Simulation stays frozen and
overlay/scene leases release. Six replacement images in the existing gallery
include the visible hold-out pose, delayed close-up, dismissal and unobstructed
Balin line; these changed views were visually inspected. The existing gallery
README records rejected prior timing and preserves pending artwork acceptance.

No new artwork, renderer/resource changes or save-format/content-fingerprint
changes in this correction. Unchanged GPU lifecycle, audio and broader suites
were not rerun. Source/package mayor YAML match; diff whitespace checks pass.
All task scratch and logs are cleaned after promoting six images; no diagnostic
exception retained.
