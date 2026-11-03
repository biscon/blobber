# Combat music

Status: implemented and mechanically verified, 2026-11-02. Subjective listening and
interactive acceptance remain user review.

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
the existing stereo peak limiter. Offline FFmpeg analysis supplies attenuation-only
trims toward -18 LUFS and -3 dBTP; subjective balance remains listening review.

## Announcement

A six-second charcoal lower-left viewport panel slides in with a small overshoot,
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
`CombatMusic` owns prepared decoders and fixed title buffers and exposes a borrowed
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

The panel anchors to the lower-left viewport with a 360-art-unit width and 54/72-unit
height for one/two title lines. Titles are split during preparation. Large bold
native text, a teal edge and a gold rule provide readable visual hierarchy; a
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
