# Combat music

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
retain existing modal deferral and silence rules; menu music has no announcements.

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
of music. Sanctuary-to-sanctuary travel preserves its track, decoder position and
announcement clock through existing travel fades/loading pause. Other world/context
changes stop the old music; returning to sanctuary consumes its next song. Failed
travel keeps the source transport. Sanctuary overrides encounter activity and
suppresses the ambient hiss even with music disabled; dungeon ambience is unchanged.

Menu playback starts on initial menu presentation and restarts after returning from
campaign/arena launch (including failure). Connected menus never restart it. Launch
boundaries stop it before loading; a scope restores menu eligibility on return.
EOF leaves the menu silent until the next menu session. No menu announcements.
Menu Settings continues music, while gameplay Settings keeps its existing pause.
All music shares existing Master/Music settings, mute/focus suspension, stereo
compensation and limiter. Rename the settings label to Music. Sanctuary announcements
use the existing modal visibility/deferred-clock rules; dialogue and inventory do
not pause the song. No simulation, campaign schema or gameplay RNG changes.

Preparation owns vectors, metadata and warmed decoder contexts. Frame updates and
context transitions use fixed state, without filesystem access or engine C++
allocation. Explicit persistence boundaries flush both shuffled banks. Shutdown
stops and unloads all banks before closing the device.

Owned music/runtime fixtures cover single-play exhaustion,
re-entry, all sanctuary locations, world cleanup and travel continuity/rollback,
ambience exclusion, modal deferral, mute/focus, independent persistence and warmed
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
hold and sideways exit run for six seconds of visible, unpaused presentation time.

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
