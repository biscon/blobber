# Runtime mix loudness

Status: implemented and verified, 2026-11-02. User reports effects much quieter than browser audio.
Current samples peak near -6 dBFS, but cue trims, category/master sliders and raylib's
center pan law compound: .5 × .45 × .85 × .6875 ≈ .13 (-18 dBFS), even at full master.

## Design

Keep exported assets, per-cue balance, user slider values, attenuation, gameplay and
saves unchanged. Bank-level `mix_gain_db` defines effects +12 dB, footsteps +12 dB,
UI +9 dB and ambience +6 dB. Missing fields default to 0 dB for existing fixtures/
custom banks; values outside -24..18 dB or malformed inputs reject with a diagnostic.
Convert to linear gain once during
preparation. Footsteps still multiply both their own and Effects sliders, but get
only one boost. All existing mute/focus/travel behavior remains.

Raylib 6.2 source confirms buffer volumes permit values above one and mixing adds
floating-point signals before calling mixed processors. Attach one final stereo
peak limiter via its standard mixed-processor API; no dependency patch. Include
these APIs in the system-package compatibility probe. No individual sample clipping
or automatic normalization is introduced. Future music can use a separately
balanced bus before the same final protection; music delivery is outside this change.

The CPU-only limiter uses stereo-linked instantaneous peak attack and an exponential
80 ms release, a -1.5 dBFS sample ceiling and no delay, allocation, locks, filesystem
access or logging. Signals below the ceiling pass unchanged when gain has recovered.
Both channels receive the same gain to preserve stereo position. Invalid non-finite
samples become silence. This is sample-peak protection, not oversampled true-peak or
perceptual loudness normalization; sustained overload can audibly reduce dynamics.

Platform owns one device-wide static callback state (raylib has no callback userdata).
Reset/configure it only while detached, using the prepared resident sound's actual
device sample rate; if no resident sound is available, use a transient silent sound
probe during preparation. Callback state is touched only by the audio thread while
attached. Stop all voices/streams, detach under raylib's audio lock, then release
resources and close the device. No callback points at an application object.

## Verification

Owned CPU fixtures check unchanged quiet signals, positive/negative overloads,
stereo ratio preservation, bounded release/recovery, non-finite inputs, arbitrary
callback partitioning and reset. Existing runtime mocks add callback lifecycle,
gain/mute behavior and warmed allocation coverage including processing. Bank tests
exercise defaults, explicit dB conversion and malformed values. Perform one bounded
real-device check using synthetic fixtures and capture callbacks: verify the limiter
after summing simultaneous voices, quiet-path amplification, pause/resume and teardown.
Mute physical device output during the check; measure before device master volume.
Use two low-priority build workers and serial focused audio tests, plus a graphics-
disabled build/check for the affected CPU targets. No stress or unrelated test matrix.

## Executed evidence

- Debug graphical game, audio CPU/runtime tests and device checker build with two
  low-priority workers. Focused audio/audio_runtime suites pass (2/2, 0.02 s),
  including callback lifecycle/reset, saved slider/mute behavior and the warmed
  allocation guard covering both runtime mixing and limiter processing.
- The bounded real-device check loads the complete production bank without
  omissions, then uses owned quiet/hot tone fixtures with physical device output
  muted. Capture occurs after the limiter and before the device master-volume
  stage. Quiet peak rises from 0.00457764 to 0.0182239 (3.981× / +12 dB).
  Eight simultaneous loud voices engage the limiter and peak at 0.841395
  (-1.5 dBFS). Left/right isolation, paused zero output, resume, streaming loop
  wrap and teardown pass. No test tone was played through speakers.
- CPU limiter fixtures verify identity for quiet stereo, both peak polarities,
  channel balance, recovery without overshoot, non-finite sanitization, reset and
  identical output across callback partitions. Bank fixtures verify legacy unity
  defaults, dB conversion and malformed/excessive-value diagnostics.
- Graphics-disabled Debug audio target builds and its CPU suite passes (1/1,
  0.01 s), preserving headless use. `git diff --check` passes. No broader matrix
  or performance stress test was run.
- No WAV files, user settings, combat/save state or music playback were changed.
  The mix now has substantially more level; subjective balance remains user review.
