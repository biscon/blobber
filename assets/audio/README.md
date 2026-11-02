# Underkeep audio bank

This is an original, provisional synthesized bank: **75 cues / 218 WAV variants**.
Csound produces the sound layers; SoX processes and exports them. Runtime needs
only the exported files and raylib. No authoring tools are required by players.

Open [audition.html](audition.html) in a browser or [audition.m3u](audition.m3u) in a
player. [generated/review.txt](generated/review.txt) prioritizes creature voices and
door/chest creaks for listening review. Technical checks do not establish realism.
The in-game Settings submenu is available from both Main and Pause menus. Party
footsteps have their own volume, including off, multiplied by Effects and Master.

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
A missing replacement falls back to its generated base variant with a diagnostic.
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
`overrides.json`, and `bank.json`. The audition index previews generated base sounds;
launch the game to audition overrides in context.

Recipes: `tools/audio/recipes.json` and `tools/audio/voices.csd`. Csound seeds and
SoX repeatable dither make reruns deterministic on the same recorded toolchain.
`generated/provenance.json` records recipes, seeds, tool versions, hashes, peak/RMS,
DC and loop-seam measurements. Byte identity across different tool versions is not
promised. Exported peaks are at most -3 dBFS (the initial recipes target -6 dBFS).

Audio preferences use `$XDG_CONFIG_HOME/blobber/settings.json`, or
`$HOME/.config/blobber/settings.json`. `--settings FILE` overrides that location;
`--no-audio` skips device initialization. These preferences are independent of saves.
