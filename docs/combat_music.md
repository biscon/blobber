# Combat music

## Discard hidden announcements — 2026-11-08

Status: **implemented and mechanically verified**; listening review unverified.
`MusicTransport` previously advanced its announcement clock only while visible,
so an overlay deferred or resumed a song announcement after the song already started.
That deferral is replaced with permanent expiry: `bannerVisible=false` expires the
current announcement before any suspension/early return; starting a track while
hidden initializes its announcement as expired. Reopening gameplay never re-arms
that track's announcement. A subsequent track that starts visibly announces normally.
Explicit playback pause/mute/focus behavior remains independent; a hidden screen
also discards the announcement when its music is paused. The six-second visible
animation and renderer are unchanged.

The player continues servicing only its selected prepared decoder. Dialogue does
not restart playback, reset the playlist or consume additional selections on exit.
The existing fixed-array shuffle consumes every song once per cycle and swaps the
first selection on a wrap when it matches the last played song; a one-song bank
necessarily repeats. No new allocation, ownership, storage or persistence format.

Verification selection: focused Debug `music` and `audio_runtime` suites and game
build. Owned multi-song runtime fixtures check selected mock stream identity and
start counts at EOF during dialogue, repeated hidden starts, visible exit, wrap,
and persisted continuation. Check hidden starts/interruption/paused suppression
and normal later announcements in the CPU transport, retaining frame allocation
coverage. No desktop automation or physical listening; the reported real-world
repeat was not reproduced independently of the deferred banner.

Executed verification: the new CPU regression first failed against the old
transport at the hidden-announcement assertion. With the correction, configured
CLion CMake `--build --preset debug --target blobber blobber_music_tests
blobber_audio_runtime_tests -j 2` passes at low CPU/I/O priority. The game build
also regenerated its existing prepared-asset dependencies; no asset inputs changed.
After making the new runtime fixture use a fixed persisted RNG seed, rebuilt that
checker and ran configured CTest `--preset debug -R '^(music|audio_runtime)$'
--output-on-failure`: both suites pass in 0.02 seconds. Twelve selections cover
four complete three-song cycles; reopening dialogue neither starts another stream
nor revives the banner, and a persisted full-cycle boundary resumes with a different
song. Existing single-song fallback remains covered. EOF transitions retain zero
engine-owned C++ allocations. A read-only check of the user's ten-song sanctuary
manifest/state found unique IDs/paths, consistent consumed/last state and a different
next entry; playlist/settings/save files were not modified. Shuffle logic itself
needed no change. Formatting and `git diff --check` pass. No device playback,
desktop interaction or graphical captures were performed. Owned temporary output
was cleaned; no retained diagnostics or new permanent evidence.

## Compact announcement — 2026-11-07

Status: compact typography accepted by the user; requested top-left integration
implemented and mechanically verified. Replaces the old renderer and preview API.
Use a fixed 540 × 90 native pixel panel for one title line and 540 × 117 for two,
24 pixels inside the viewport's top and left edges (native origin 36, 78).
Keep the accepted internal padding, colors, shadow, six-second animation and
half-distance horizontal slide. Panel width does not expand: prepared titles split
into up to two lines and each rendered line uses measured ellipsis clipping within
its 504-pixel text area, including while letters scramble. Artist text is clipped
by the same policy; no text may cross the enclosing panel.

UiFonts owns separate IBM Plex Sans bold 24 px and regular 18 px atlases for title
and caption/artist respectively, loaded at preparation and included in RAII cleanup
and GPU memory accounting. Draw only at loaded sizes, rounding final native text
origins after animation/layout. No glyph or render-target scaling, new per-frame
allocation, prepared-world invalidation or audio transport change. Announcements
use the modal discard rules above and existing silence rules; menu music has no announcements.

The capture tool now exercises the production renderer directly, including settled
one/two-line titles, entry/reveal/exit and deliberately overlong text. Verify hidden
OpenGL captures at native 1920 × 1080, assert changes stay inside the viewport and
long text stays within the panel, build the game, and run the focused music check.
Replace superseded evidence with at most six selected captures. No desktop input
or focus automation.

Executed verification: rebuilt Debug `blobber`, `blobber_capture_music_banner` and
`blobber_music_tests` with at most two low-priority workers and no compiler warnings.
`nice -n 15 ionice -c 3 ctest --preset debug -R '^music$'` passes in 0.01 s.
The hidden production-renderer capture succeeds outside the X11-restricted sandbox,
without OS input or focus control. Inspected settled and overlong two-line captures;
all six animation/title cases remain within the viewport, with settled origin
(36, 78), expected panel/shadow bounds and no bright title pixels outside the panel.
Fonts retain the accepted size-specific atlases and integer draw origins. No audio
runtime suite or additional build configuration was repeated for this UI-only edit.
Six selected captures replace superseded evidence in the existing gallery;
disposable run output was cleaned. `git diff --check` and formatting checks pass.
Interactive game/listening review was not performed.

## Sanctuary and menu extension — 2026-11-07

Status: implemented and mechanically verified; listening/interactive review pending.

Generalize the prepared player for three independently owned banks. Combat keeps
its existing manifest and `music-state.json`; sanctuary and mainmenu use manifests
inside their respective directories. Sanctuary uses the same bounded shuffle and
transport, persisting to `sanctuary-music-state.json` beside settings. Menu uses a
single-play policy with an explicit completed latch and no persisted progress.
Each bank retains the 64-track ceiling; only the selected bank services a decoder.
Missing/invalid banks remain silent. Original music files are not rewritten.

Gameplay supplies the committed canonical location at initial attachment and after
travel. AudioRuntime selects sanctuary for City/Inn/Smithy/Emporium locations,
combat for other gameplay, and menu only within explicit menu-session boundaries.
World cleanup stops old voices, dialogue and effect/ambient streams independently
of music. Sanctuary-to-sanctuary travel preserves its track and decoder position;
hidden announcements are discarded as described above. The continuous doorway correction below also
keeps audible playback and gain stable through fades/loading. Other world/context
changes stop the old music; returning to sanctuary consumes its next song. Failed
travel keeps the source transport. Sanctuary overrides encounter activity and
suppresses the ambient hiss even with music disabled. Dungeon combat/music
exclusion is described below.

Menu playback starts on initial menu presentation and restarts after returning from
campaign/arena launch (including failure). Connected menus never restart it. Launch
boundaries stop it before loading; a scope restores menu eligibility on return.
EOF leaves the menu silent until the next menu session. No menu announcements.
Menu Settings continues music, while gameplay Settings keeps its existing pause.
All music shares existing Master/Music settings, mute/focus suspension, stereo
compensation and limiter. Rename the settings label to Music. Sanctuary announcements
use the modal discard rules above; dialogue and inventory do
not pause the song. No simulation, campaign schema or gameplay RNG changes.

Preparation owns vectors, metadata and warmed decoder contexts. Frame updates and
context transitions use fixed state, without filesystem access or engine C++
allocation. Explicit persistence boundaries flush both shuffled banks. Shutdown
stops and unloads all banks before closing the device.

Owned music/runtime fixtures cover single-play exhaustion,
re-entry, all sanctuary locations, world cleanup and travel continuity/rollback,
ambience exclusion, modal suppression, mute/focus, independent persistence and warmed
allocation invariants. Decoder servicing respects incoming pause/mute as well as
already suspended state, preserving EOF detection when unmuting the menu song.
No OS input automation; subjective listening remains user review.

Executed verification (Debug, at most two low-priority build workers, serial tests):

- Built `blobber`, `blobber_music_tests`, `blobber_audio_runtime_tests`,
  `blobber_audio_tests` and `blobber_menu_tests`, without compiler warnings.
- `nice -n 15 ionice -c 3 ctest --preset debug -R '^(music|audio_runtime|audio|menu)$'`
  passed all four focused suites in 0.03 s. After tightening decoder suspension and
  adding the EOF-boundary/transition allocation regressions, rebuilt the game and
  runtime checker; the affected `audio_runtime` suite passed again in 0.01 s.
- Runtime checks cover all four town locations, world replacement with old-voice
  cleanup, source retention on rollback, town/dungeon music and ambience changes,
  independent sanctuary persistence, one active song, deferred banners, focus/mute,
  menu Settings continuity, exhaustion, re-entry and allocation-free transitions.
- Both new manifests cover exactly the supplied ten sanctuary MP3s and one menu
  MP3. Sequential FFmpeg checks decode the first 0.1 s of each without audio output;
  these bounded checks do not establish whole-file integrity or listening quality.
  The offline measurement tool's `--bank` CLI and `git diff --check` pass.
- No physical-device listening, desktop interaction or new graphical capture was
  performed. Banner artwork/animation is reused unchanged; the Settings label is
  now Music. User settings, original song bytes and campaign formats are preserved.
  Disposable run logs and the failed timing-assumption fixture were cleaned after
  recording results; no new permanent screenshots or diagnostic exceptions remain.

Status: implemented and mechanically verified, 2026-11-02. Subjective listening and
interactive acceptance remain user review.


## Combat/sanctuary balance investigation — 2026-11-08

Both banks use Master × Music × world travel gain through the same `MusicPlayer`;
at investigation time, source, Debug and Release manifests all had 0 dB trims.
The sanctuary trial below supersedes those town values. No lower combat bus
gain was found. A read-only, sequential FFmpeg `loudnorm` measurement of the two
last persisted playlist selections found Epic Hair Metal Synth Rock at **-13.28
LUFS / -3.38 dBTP**, versus Relaxing Piano Coffeehouse at **-15.80 LUFS / -0.33
dBTP**. This pair's combat source is about 2.5 dB louder in integrated loudness;
these selections do not establish levels for the whole sanctuary bank.

The likely difference during play is masking by combat effects and possible
shared-limiter gain reduction: the effects bus has a +12 dB boost, and the final
-1.5 dBFS limiter lowers the entire mix when combined peaks exceed its ceiling.
Music has no separate protection from that gain reduction. This mechanism is
confirmed in code; its live contribution to the user's observation is not
measured. No game process was running for live verification. No physical listening,
audio/settings/playlist edits, or playback automation was performed. Temporary
measurement output was cleaned after recording these findings.

## Sanctuary balance trim — 2026-11-08

Status: **implemented and mechanically verified; listening trial pending**. The user's goal
is a smaller perceived music-volume change between town and combat while keeping
effects prominent. Apply **-4 dB** to all ten sanctuary manifest entries using the
existing attenuation-only `gain_db` field (linear gain about 0.631). The user liked
the initial -3 dB trial and requested one additional decibel of reduction. This covers
the city and its shared interior playlist. Combat retains its current trim;
the menu balance extension below now matches sanctuary. Boosted effects and final
peak limiting retain their current behavior.

This is authored balance data, with no new mixer abstraction, resource ownership,
frame processing or allocations. The existing prepared player loads trims before
playback; relaunch to hear the new balance. IDs, playlist order/state, original
MP3 bytes, Music/Master settings and saves are preserved. The level is a modest
adjustment based on the user's listening feedback, not a claim of measured
perceptual equality between all tracks or encounters.

Validation selection: inspect the manifest diff for gain-only edits, confirm all
ten safe paths and current trim values, build the Debug game to publish the updated
manifest, verify the copied asset matches, and run the existing focused `music`
test for loading/manual-gain semantics. No device playback or broad test matrix.

Initial -3 dB verification passed: Debug `blobber` and `blobber_music_tests` build (two workers),
the focused `music` CTest (0.01 seconds), and direct manifest/publication checks.
All ten changes are exactly `gain_db: 0` to `gain_db: -3`; every other entry field
is preserved, every source path exists, the copied Debug manifest is byte-identical,
and combat/menu manifests match their prior bytes. No new tests, mixer changes,
audio re-encoding or physical playback were needed. The user's listening review
will determine whether this modest reduction is sufficient. Disposable output
was cleaned after recording the result.

The follow-up -4 dB publication also passed: Debug game build and gain-only
checks confirm all ten -3 to -4 edits, preserved entry data/source paths, and a
byte-identical copied manifest. The already-passing music parser/transport suite
was not repeated for this parameter-only adjustment. Disposable output was cleaned.

## Main-menu balance trim — 2026-11-08

Status: **implemented and mechanically verified; listening review pending**. Match the user's improved sanctuary
balance by changing the single menu song's existing `gain_db` from 0 to **-4 dB**.
This reuses the prepared player and leaves its one-shot policy, Music/Master
controls, source MP3, metadata identity and all other banks intact. No frame-path,
resource or mixer changes are needed. Relaunch to load the authored trim.

Validation selection: Debug game asset publication, gain-only manifest comparison,
existing source path and source/build byte equality. The existing manual-trim
tests already pass; no broad suite or device playback is needed for this data edit.

Verification passed: low-priority Debug `blobber` build (two-worker cap), one
gain-only change with all other entry fields preserved, an existing MP3 path and
a byte-identical copied manifest. No repeat test matrix or physical playback.
Disposable build output was cleaned after recording the result.

## Dungeon ambience and combat music — 2026-11-08

Status: **implemented and mechanically verified; listening review pending**. Dungeon ambience must pause
while combat music is audible, including its ending fade, then resume from its
held position. This uses actual prepared playback state rather than encounter
activity or announcement visibility: muted/zero-volume/unavailable music permits
ambience, while focus loss and ordinary pause rules still suspend it. Town's
existing ambient exclusion and positional Web/Spiritual Weapon loops are unchanged.

`MusicPlayer::audible()` exposes a bounded, allocation-free read of its selected
track, suspension state and positive fade level. `AudioRuntime::update` advances
the music transport before reconciling looping streams, so a start, track advance,
mute or fade completion affects ambience in the same engine update. It passes
an ephemeral suppression flag to `streamUpdate`; no persisted field, resource or
per-frame container is added. Existing ambient streams use Pause/Resume without
rewinding. A dungeon entered during existing combat defers starting its ambient
stream until music is inaudible, avoiding a start-then-pause burst. Effects loops
and boosted one-shots retain their independent world pause/admission rules.

Validate one Debug game build and focused `audio_runtime`, `music` and `audio`
CPU suites. Extend owned mock fixtures for entry, ongoing playback, track rollover,
fade completion, mute/unmute, zero Music volume, restored active combat, missing
playlist fallback, one-shot coexistence and allocation-free transitions. Preserve
existing focus/modal/town/travel lifecycle coverage. No physical audio playback,
desktop automation or re-encoding is needed for this coordination change.

Verification passed: Debug game and the three focused suites (`audio_runtime`,
`music`, `audio`), serially in 0.03 seconds. Owned mock coverage confirms ambient
position preservation, same-update exclusion, mute/zero-volume fallback, rollover,
fade completion, restored-combat deferred startup, missing-playlist fallback,
one-shot coexistence and zero engine-owned C++ allocations during repeated
switching. Existing focus, modal, town/travel and shutdown assertions pass.
No device output or physical listening was performed.

## Continuous sanctuary doorway music — 2026-11-08

Status: **implemented and mechanically verified; listening review pending**. Preserve normal sanctuary
music level and service its decoder during city/interior doorway fade-out,
background loading, publication and fade-in. Previously travel both
multiplied music by `worldGain` down to zero and suspended its transport during
loading. Continuing an existing playlist cursor alone does not prevent silence.

An application-owned `TravelMusicScope` brackets only validated doorway trips
from sanctuary to another sanctuary location. It asks `AudioRuntime` to preserve
sanctuary playback until scope exit, including failed-travel rollback and close/
exception paths. Source/destination location checks happen before worker activity.
World effects/ambience retain the travel fade and pause. Dungeon transitions keep
their existing music fade/suspension behavior. Detach/shutdown clear the scope flag.

`AudioRuntime::pumpLoadingMusic(focused)` services only the prepared sanctuary
player while this scoped policy is active. It takes no GameSession, world, Party,
renderer or store reference, performs no I/O for persistence, and hides/holds the
announcement. Master/Music controls, track trims and focus suspension still apply;
EOF advances the existing shuffle normally. Stream refilling, volume and transport
operations stay on the window thread; raylib's audio callback remains raylib-owned.
The worker never calls raylib. Ordinary updates use normal music gain throughout
the scope, independently of `worldGain`, and ignore only the travel pause for this
bank. The original explicit pause behavior resumes when the scope exits.

`LoadingScreen::point` calls the pump when its presenter has audio, at the same
throttled checkpoints used for cached presentation. It never services world audio
or inspects the source/destination session, which may be borrowed by preparation.
No extra decoder, thread, buffer, unbounded storage or frame allocation is added.
Long uninterruptible work on the window thread can still exceed available stream
buffering; the change services the existing responsive threaded/checkpoint path,
not a guarantee against arbitrary driver/filesystem stalls.

Validate the focused audio suites and Debug game build. Owned mock tests cover
full-volume travel/black loading, thread ownership through `PreparationWorker`
checkpoint pumping, deferred banner, EOF, focus/mute, non-town exclusion, cleanup
and allocation-free pumping. An opt-in muted generated-MP3 test checks the real
backend without physical speaker output or OS input automation.

Verification passed: Debug `blobber` and runtime checker builds; the affected
`audio_runtime` suite passed again in 0.01 seconds after the loading changes.
The earlier three-suite result above remains the checkpoint for ambience/music
coordination; unchanged core suites were not repeated. Mock assertions cover
normal music gain even at `worldGain == 0`, all sanctuary locations/committed
shop replacement, scope exit, non-town exclusion, deferred announcements,
loading EOF, focus/mute/zero-volume controls and allocation-free pumping.
`PreparationWorker` executes an owned background task while the waiting main
thread services music through preparation checkpoints; every mock stream update
asserts window-thread ownership.

The optional `blobber_music_device_check` gained `--loading-only` to select this
new integration without repeating its unrelated fade/limiter scenarios. The
wrapper creates short/long MP3s and a separate sanctuary playlist from generated
sine fixtures. Device master is set to zero before test playback. After 600 ms
of threaded loading/checkpoint pumping, later captured application mixer samples
remain audible at the normal song level: **0.0840149 normal peak versus 0.0839844
loading peak**, with unchanged track revision. Ordinary pause is silent after
the travel scope exits. These are application-owned samples before the muted
device master, not system/desktop audio capture.

```sh
nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target blobber_music_device_check --parallel 2
TMPDIR=AGENT_RUN nice -n 15 ionice -c 3 python3 tests/music_device_check.py cmake-build-debug/blobber_music_device_check --loading-only
```

The sandbox attempt returned unavailable (77); the same muted check passed with
audio-device access outside the sandbox. No OS input/focus control or physical
listening was performed. Actual shop-travel listening remains user review.
Source artwork, audio bytes, preferences and campaign data are untouched; test
fixtures, logs and intermediate outputs were cleaned after verification.

## Music level correction (2026-11-03, implemented)

Listening feedback finds music too quiet even at Master/Music 100%. The initial
-18 LUFS attenuation-only policy removes up to 5.44 dB; raylib 6.2's centered stereo
pan law multiplies both channels by 0.6875 (another -3.25 dB). This was excessive
for these already mastered tracks alongside the boosted SFX.

Set the supplied track trims to 0 dB, retaining editable manual attenuation and
measurement metadata. Change the offline measurement tool to preserve manual trims
and default new tracks to 0 dB rather than applying normalization. At the platform
boundary multiply music volume by 1/0.6875 to cancel the pinned raylib center law;
leave the stereo pan centered. Master, Music, travel/fade and manual trims continue
to multiply normally. The existing final stereo limiter remains the sample-peak
safety ceiling; occasional peak reduction is expected for hot originals and effects.
No MP3 rewrite, user settings or playlist state change. Validate actual quiet MP3
output against decoded source level before checking mixed peaks, plus focused runtime
and music tests. No full-bank reanalysis or unrelated test matrix is needed.

Verification: Debug game and affected checkers build without warnings; focused
music/runtime suites pass (2/2). The muted real-device MP3 test compares quiet stereo
output against its decoded source and passes, then passes pause/mute/advance/fade
and the existing mixed-peak ceiling check. The owned authoring probe confirms manual
trims survive remeasurement and newly discovered tracks default to 0 dB. All 21
supplied entries now have zero trim. `git diff --check` passes. No user settings,
playlist progress or audio file bytes were changed. Subjective balance remains
listening review; hot peaks can still be reduced by the shared limiter.


## Ownership and data

A graphics-independent bounded playlist/transport owns a shuffle of at most 64
stable track IDs, its private RNG, cursor, active track and fade/announcement
clocks. A platform music component owned by AudioRuntime owns prepared non-looping
raylib Music streams. All decoder contexts and metadata load during preparation;
only the active stream is updated. No full-song PCM or gameplay RNG is used.
The audio journal holds non-droppable encounter lifecycle state independently of
its sound-effect queue. Repeated Started within one episode is idempotent. A
restored active turn-based encounter is adopted once on session attachment.

`assets/audio/music/playlist.json` lists editable IDs, relative paths, titles,
artist and attenuation trims for all 21 supplied MP3s. Original files stay intact.
The playlist has a separate `music-state.json` next to settings. It records the
shuffled IDs, next cursor, last track and RNG, not song position or campaign state.
Writes are atomic and occur at explicit menu/settings/travel/shutdown boundaries,
not ordinary frames. Abnormal termination may lose the latest progress. Surviving
IDs retain their order/used status after edits; new tracks join the remaining queue.

## Behavior

Starting a track consumes it. Encounter entry starts the next track at zero;
track completion advances only while combat remains active. Finished is handled
before end-of-track: fade the current song for min(5 seconds, remaining playback)
and never advance during the fade. A new encounter interrupts a fade with the next
track. Playlist wrap reshuffles and avoids immediate repeats when possible.
Detach stops playback but retains the rotation. Missing tracks are excluded with
preparation diagnostics; no usable tracks/device means silent operation.

Music defaults on at 50%, with its own draggable slider and toggle. Existing
settings remain compatible. Master/music mute freezes the current track during
the same encounter; combat ending while muted discards it. Muted encounters consume
nothing. Explicit pause/settings and lost focus freeze music. Tactical panels do
not. Music uses Master × Music × track trim × travel/fade, with no SFX boost, then
the existing stereo peak limiter. The initial attenuation-only -18 LUFS/-3 dBTP
policy was superseded by the 2026-11-03 level correction above: supplied tracks use
0 dB trims and the platform compensates raylib center attenuation. Offline FFmpeg
analysis now records measurements and preserves manual trims; subjective balance
remains listening review.

## Announcement

A six-second charcoal top-left viewport panel slides in with a small overshoot,
uses teal/gold accents and deterministic scrambling letters that settle by 1.2s,
holds through 5.5s then retracts. It announces every started track, never resumes.
Tactical overlays defer the current unfinished announcement; only the latest is
retained. Muted/no-device playback hides it. Fixed buffers and prepared metadata
avoid frame allocations; animation does not invalidate the prepared world image.

## Verification strategy

Owned CPU fixtures exercise shuffle, persistence reconciliation, encounter timing,
end races, shortened/interrupted fades, pause/mute and allocation invariants.
Runtime mocks cover prepared streams, lifecycle and failure. Bounded muted-device
checks cover actual MP3 decoding, transition/fade and final peak protection. Hidden
application-owned captures inspect animation stages and settings; no OS automation.
Use at most two low-priority build workers, one intensive job and focused serial
tests. Record final results and limitations here.


## Implemented interfaces and edge handling

`audio::Playlist` uses a fixed 64-index array, a cursor and private xorshift RNG.
`audio::MusicTransport::update(MusicInput)` consumes encounter identity/active state,
audibility, pause, decoder EOF/remaining time and elapsed time. It publishes the
current track, playback revision, fade level, suspension and announcement age.
`MusicPlayer` owns prepared decoders and fixed title buffers and exposes a borrowed
`MusicBanner`. AudioRuntime owns this component alongside the existing SFX/ambience
owners; its update takes separate music-pause/banner-visibility flags. Presenter
draws the banner in its native HUD pass without rebuilding the world image.

Audio Journal lifecycle remains independent of cue admission/enabled state. Returning
to Exploration after losing all threats clears the active episode even when no
victory cue is emitted. Import clears transient audio history; attaching to an
already active initiative snapshot adopts it once. No serialized simulation fields
or campaign formats change.

Settings retain the existing five-value audio array and add `music_volume` and
`music_enabled`; old settings receive enabled/50% defaults without changing their
existing values. The music state file has version, shuffled stable IDs, cursor,
last ID and RNG state. Unavailable IDs are removed while preserving consumed versus
unplayed status. Newly added IDs join the unplayed tail. Invalid state retains a
fresh prepared shuffle. A missing playlist is permitted for small owned fixtures
and audio installations without combat music.

All decoder contexts are loaded and silently warmed during preparation (raylib
lazily grows PCM scratch on first update). The pinned fallback explicitly enables
MP3 decoding. Paused streams are not serviced. Shutdown stops tracks, detaches the
shared limiter, unloads music/ambience streams and aliases/sources, then closes the
device. Only the selected track is decoded during gameplay; raylib/decoder internals
and their file reads are outside the engine-owned C++ allocation guarantee.

The panel anchors 8 art units inside the top-left viewport with a 180-art-unit
width and 30/39-unit height for one/two title lines. Titles split during preparation
and clip with ellipses to the measured text width. Native 24 px bold title and
18 px regular caption atlases, a teal edge and a gold rule provide visual hierarchy; a
12-Hz deterministic character scramble settles by 1.2 seconds. Its slide/overshoot,
hold and sideways exit run for six seconds of unpaused presentation time while
visible. Hiding the panel discards its remaining animation as described above.

## Executed verification

- Debug graphical game and affected test/capture tools build successfully with two
  low-priority workers and no compiler warnings. Final music/audio/runtime suites
  pass (3/3); the subsequent active-session adoption regression also passes in the
  runtime suite. Tests cover wrap without repeats, the 2.5-track scenario, fade/end
  races, shortened/interrupted fades, toggles, explicit/tactical pause, settings
  compatibility, state roundtrip/reconciliation/corruption, bounded variation and
  allocation-free transitions/banner generation. Mock runtime verifies decoder
  preparation, stop-before-unload, muted visibility and adoption of actual owned
  initiative state once.
- Graphics-disabled Debug music/audio/hybrid/combat targets build and all four
  focused suites pass (0.24 seconds). No unrelated/full build matrix was run.
- `tests/music_device_check.py` creates owned short/long MP3s and a loud WAV in an
  isolated temporary directory. The real-device checker passes with physical
  master output muted: actual MP3 EOF advancement, menu/focus/mute silence,
  tactical continuity, resume without banner restart, five-second and shortened
  fades, and postcombat silence. Eight loud SFX plus music peak at 0.841395
  (-1.5 dBFS) through the final limiter. All fixtures are deleted afterward.
- The existing bounded audio-device checker loads the complete production bank,
  including all 21 MP3s, without diagnostics; its effects/pan/pause/loop/limiter
  checks pass. Physical speaker output remains muted during measurements.
- Sequential FFmpeg analysis validates all 21 files. Manifest trims range from
  0 to -5.44 dB; resulting measured values meet the -18 LUFS/-3 dBTP targets
  (quiet originals are never boosted). No MP3 bytes were rewritten.
- Hidden native HUD captures of entry/reveal/settled/exit, a long two-line title,
  and the settings panel were inspected. The banner uses an existing application
  capture as a static backdrop; these are rendering checks, not a new gameplay
  playthrough. The initial settings capture used unprepared source assets and
  failed; rerunning against the prepared build assets succeeded. No OS input,
  desktop focus changes or interactive automation was used.
- One initial CPU test compared irrelevant unused array entries; it was corrected
  to compare the active playlist prefix. A graphical combat test was initially
  unbuilt; combat verification was then built and executed in the headless
  configuration as recorded above. `git diff --check` passes.

See [captures](screenshots/combat-music/README.md) and the
[playlist editing guide](../assets/audio/music/README.md). Final perceived loudness,
track suitability and animated feel still require listening/interactive review.
