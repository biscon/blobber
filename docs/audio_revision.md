# Audio authoring revision

Status: implemented and mechanically verified, 2026-11-02; revised synth palette
accepted in user listening review. The separate [organic candidate pass](audio_model_pass.md)
adds further model auditions and a new encounter cue for listening review.
The model trial is now closed: five exports retained, model/setup/unused audio
removed. See the linked retirement record for the current bank and authoring state.
The first bank passed mechanical
checks but failed listening review: excessive shared modal/FM/formant structures
made unrelated actions sound alike. File count is not perceptual variety.

## Design

Keep the existing runtime, semantic cue names, event timing and replacement
contract. Improve offline authoring, not gameplay, saves or simulation RNG.

Stable Audio 3 Small-SFX is installed in a dedicated Docker image with pinned upstream
source, model revision and Python dependencies. Store weights, downloaded sources,
caches and unselected experiments under ignored `.cache/audio-model/`; Docker
images remain in Docker's storage. Track only build/launch recipes, prompt/seed
manifests, provenance, documentation and deliberately selected playable exports.
Never bake credentials into images or track tokens. Authentication uses a local
hidden terminal prompt and a private ignored cache. The user accepted Hugging
Face's gated terms and authenticated locally; weights downloaded successfully.

Use the existing NVIDIA CDI device for a bounded CLI inference check. Containers
run as the invoking user, with two CPU workers, bounded RAM and one generation at
a time; no privileged container, desktop control or public web service. Provide
commands for download, status and prompted/seeded generation. Output normal WAVs
with adjacent model/prompt/seed provenance; offline reruns use cached weights.
Start with a small audition set, not automatic replacement of the organic bank.

Csound remains the synthesis engine. Introduce separate structures: dry wooden UI
ticks, soft harmonic confirmation/cancel notes, a muted two-pulse rejection,
glass/coin pickup sparkles, fabric-and-buckle inventory feedback and a struck
inharmonic bell with independently decaying partials for advancement. The bell
has a clear initial strike and a roughly 2.5-second singing decay: a DIIING,
not a pitch-shifted spell sweep. Keep musical cues at stable pitch; variations
change strike/brightness/timing rather than transposing the whole motif. Revise
selected tonal spell/flow cues with distinct envelopes and layered note events.
Retain SoX export, controlled headroom, selective rebuild and replacement safety.

## Verification

Check Docker CUDA access and dependency imports, gated download status and a
short real inference when access permits. Verify weights/caches/credentials cannot
enter Git or the Docker build context. Validate generated WAVs, deterministic
selective Csound regeneration and replacement preservation using owned fixtures.
Add an owned layered-recipe fixture and advancement event regression where missing.
Run focused audio tests; rebuild/copy game assets. Publish an audition index that
prioritizes changed sounds and clearly separates technical checks from listening
acceptance. No full build/test matrix or batch GPU workload.

## Delivered changes and evidence

- Docker image `blobber-stable-audio:small-sfx-20261102` is installed (5.76 GB
  uncompressed). Model snapshot is about 3.3 GiB, outside tracked assets.
  Source revision `5f5e6ac21ae9ff78a6e8847eb7a1dcbe969ffb84`, model revision
  `05ca3857881c94a2b84026b1111c537d78f5f9f0`, Python 3.11 and the upstream frozen
  dependency lock are pinned. PyTorch 2.7.1+cu126 runs on the RTX 3060. The first
  build exposed an upstream Python-3.10 pin whose interpreter landed underneath
  the cache mount; explicitly selecting the image's Python 3.11 fixed it.
- Four real offline GPU generations succeeded: 4-second door, 3-second potion,
  3-second bone collapse and 2-second hurt reaction. Warm generation times were
  approximately 0.21–0.77 seconds (excluding model startup); peak PyTorch allocated
  GPU memory was 2,284,434,432 bytes. These are observations, not a benchmark.
  All four WAVs are finite, non-silent stereo 44.1 kHz with expected durations and
  unclipped raw peaks. Prompts, seeds, model IDs and hashes accompany the ignored
  outputs. No network was available in the generation containers.
- Historical local auditions: `.cache/audio-model/output/index.html`; these
  experiments and their model installation tools have since been removed.
  These are unselected raw float experiments, not installed game replacements.
- Twenty synth cues / 52 variants were selectively rebuilt with Csound and SoX:
  UI confirm/cancel/reject; inventory move/drop/equip/unequip; key/loot pickup;
  provisional potion bubbles; radiant/healing/ice/control/arcane; victory/defeat;
  travel/recovery; and advancement. The level-up bell lasts 2.8 seconds, has fixed
  pitch, a raised but bounded gain and important-cue priority. Existing advancement
  action timing is unchanged. A bounded single voice avoids stacking its tails.
- All 218 exported bank files pass format, non-silence, headroom, DC and endpoint/
  loop checks. Owned authoring fixtures pass deterministic/selective rebuild and
  replacement preservation, including a two-note fixture testing the quiet gap and
  stable frequencies across variants. Runtime bank remains mono 48 kHz PCM16 for
  effects; unchanged ambience remains stereo.
- Debug game and progression/audio test targets build with two low-priority workers.
  Focused `audio`, `audio_runtime` and `progression` suites pass (3/3, 0.03 seconds).
  Progression explicitly verifies silent preview/rejection, one committed bell and
  no replay on a repeated request. Updated assets are copied into the Debug build.
  Shell/Python syntax checks and `git diff --check` pass. Git ignore checks confirm
  tokens, model weights and experimental WAVs are excluded from normal adds; the
  Docker build context permits only its Dockerfile and driver.

The user subsequently accepted the new synth palette, including the stylized potion,
healing and control cues. Initial model outputs were considered promising, with the
hurt performance needing improvement. Existing creature
and physical combat cues retain their known initial-bank limitations. No desktop
input, playback automation, full test matrix or host Python/toolchain changes occurred.
