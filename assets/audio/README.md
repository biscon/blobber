# Underkeep audio bank

The active bank contains **75 cues / 210 WAV samples**: 205 synthesized variants
and five user-selected model exports. Csound produces synth layers; SoX processes
and exports them. Runtime needs
only the exported files and raylib. No authoring tools are required by players.

Listening revision (2026-11-02): 20 cues / 52 variants have been redesigned with
separate bell, glass chime, wood/felt tap, fabric, liquid-bubble and harmonic-bloom
structures. Start with **advancement** for the level-up DIIING, then the three UI
cues and inventory sounds. Musical pitches stay stable across variants. Potion
bubbles were preferred in listening review; existing creature vocals and
other unrevised physical sounds still retain the initial bank's limitations.
The audition page links directly to all revised cues.

The model experiment is closed. Only **dark B, radiant A, fire B, thunder B and
spiritual.loop A** were retained, under `replacements/`, with detailed provenance.
The model, Docker image, credentials, rejected candidates and superseded synth
files were deleted. Omitted/unreviewed choices keep the current synth sounds.
The encounter is now a short Csound bell/chime, triggered once at fight start.
Open [selected-audition.html](selected-audition.html) to hear these six cues without
the rejected alternatives. [Listening decisions](../../docs/audio_listening_choices.json)
record the user's choices. Future authoring uses Csound or individual replacement
recordings; no model is required to build or play the game.

Open [audition.html](audition.html) in a browser or [audition.m3u](audition.m3u) in a
player. [generated/review.txt](generated/review.txt) prioritizes creature voices and
door/chest creaks for listening review. Technical checks do not establish realism.
The in-game Settings submenu is available from both Main and Pause menus. Party
footsteps have their own volume, including off, multiplied by Effects and Master.

Runtime loudness is boosted by `mix_gain_db` in `bank.json`: Effects and Party
Footsteps +12 dB, UI +9 dB, Ambience +6 dB. These trims apply before the existing
volume sliders and do not alter sample files or saved settings. The final stereo
mix has a -1.5 dBFS sample-peak limiter to contain overlapping effects. Quiet
signals pass unchanged; busy overloads reduce gain with an 80 ms release. This
protects digital sample peaks, not perceived loudness or every reconstructed
inter-sample peak. Missing trims default to 0 dB; allowed values are -24..18 dB.
Lower Effects/UI/Ambience in Settings if the new mix is too strong. Browser audition
players play the WAV files directly and do not apply the game's mix controls.

## Replace one sample

1. Put your mono WAV in `assets/audio/replacements/`, for example `my_stone.wav`.
   Use stereo for ambience. Prefer 48 kHz PCM16 WAV with a little headroom.
2. Create `assets/audio/overrides.json` (or edit an existing one):

```json
{
  "cues": {
    "party.step": {
      "variant_overrides": {
        "01": {
          "path": "replacements/my_stone.wav",
          "gain": 0.8,
          "provenance": "Recorded by me, 2026-11-02"
        }
      }
    }
  }
}
```

The cue name and variant ID stay stable. No C++ changes are needed. Rebuild the
normal game target to copy changed/new assets, or launch with `--assets` pointing
to this source asset directory, then restart the game to preload replacements.
Keep license/source/author details in `provenance` for externally supplied audio.
A missing override falls back to the base bank's sample with a diagnostic.
A complete `variants` array can replace a cue's entire variation set. Cue overrides
also accept `gain`, `pitch_min`, `pitch_max`, `volume_min`, `range` (cells),
`cooldown` (seconds), `concurrency`, and `aliases` (1–8).
Categories are 1 Effects, 2 Ambience, 3 UI, 4 Party Footsteps.
`bank.json` is the editable base bank; it is never rewritten by regeneration.

## Rebuild generated audio

From the repository root, with Csound, SoX, Python, NumPy and SciPy installed:

```sh
nice -n 15 ionice -c 3 env OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python3 tools/audio/generate.py --cue party.step --variant 01
nice -n 15 ionice -c 3 env OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 python3 tools/audio/generate.py --validate
```

Omit `--cue` to rebuild the entire bank; repeat `--cue` to select several families.
`--dry-run` lists outputs without changing them. Generation only replaces files in
`generated/` and refreshes the audition index/playlist; it preserves `replacements/`,
`overrides.json`, and `bank.json`. Recipes marked `source: retained` validate and
preserve the selected exports instead of regenerating discarded synth alternatives.
The audition index previews the base bank, including retained selections;
launch the game to audition overrides in context.

Recipes: `tools/audio/recipes.json` and `tools/audio/voices.csd`. Csound seeds and
SoX repeatable dither make reruns deterministic on the same recorded toolchain.
`generated/provenance.json` records recipes, seeds, tool versions, hashes, peak/RMS,
DC and loop-seam measurements. Byte identity across different tool versions is not
promised. Exported peaks are at most -3 dBFS (the initial recipes target -6 dBFS).
`replacements/provenance.json` records the five retained model exports' original
prompts, seeds, source/model revisions, processing, hashes and review selection.

Revision-2 recipes accept a `layers` array: each entry schedules a Csound
`instrument`, `frequency`, `duration`, optional `start`, `gain` and `tone`.
Optional spreads vary strike brightness/frequency/timing within that recipe;
musical layers default to fixed frequencies. Layer endpoints must fit the cue's
declared duration. Python schedules these events; Csound synthesizes the audio.

Audio preferences use `$XDG_CONFIG_HOME/blobber/settings.json`, or
`$HOME/.config/blobber/settings.json`. `--settings FILE` overrides that location;
`--no-audio` skips device initialization. These preferences are independent of saves.

## Combat music

The separate [combat music playlist](music/README.md) contains the 21 supplied Nick Panek tracks.
It has its own volume/toggle, persistent shuffle and animated title announcement.
Music files and metadata are independent of synth regeneration and replacement cues.

## Dialogue reactions

[dialogue/](dialogue/README.md) owns the provisional mayor/guard reaction WAVs
and their synthesis source. YAML conversation beats reference them as
`audio: dialogue/boric-chuckle.wav`, relative to this shared audio directory.
They are prepared by the dialogue audio runtime and use the Effects volume.
