# Local sound experiments

Stable Audio 3 **Small-SFX**, isolated in Docker. No host Python/CUDA libraries are
installed or changed. The game never loads this model. The wrapper assumes Linux,
Docker and the NVIDIA CDI device `nvidia.com/gpu=all` (verified on this machine).

## First setup on another machine

Run from the Blobber checkout:

```sh
tools/audio/local/run.sh build
tools/audio/local/run.sh login
tools/audio/local/run.sh download
tools/audio/local/run.sh status
```

First accept access at <https://huggingface.co/stabilityai/stable-audio-3-small-sfx>.
This is a free gated download, not a subscription. Use a read-only Hugging Face
token with access to this gated repository. Paste it only at the local hidden
login prompt, never in chat, command arguments, a Dockerfile or Git. Login does
not add Git credentials. The token is stored in the private ignored local cache.
Commercial use is subject to the Stability Community License and registration;
see <https://stability.ai/license>. Bundled Gemma components have additional terms
displayed on the model access page.

The source commit, archive SHA-256, Python image digest, uv version, upstream
dependency lock and model revision are pinned. Rebuilds need network access;
generation runs with **network disabled**, from the pinned local snapshot including
its bundled text encoder. Small-SFX needs neither Flash Attention nor the heavier
Medium model. An upstream notice about disabling optional Flash Attention is normal.

## Generate and audition

```sh
tools/audio/local/run.sh generate --prompt "One dry wooden door creaking open, close isolated foley, quiet background, no music" --duration 4 --seed 42 --name door-test-01
tools/audio/local/run.sh audition
```

`audition` renders the four sequential experiments in `audition.json` (door,
potion, collapsing bones and a nonverbal hurt reaction). The last is intentionally
a difficult test: this model is not designed for voice generation. Each output
name is protected against overwriting; use new names/seeds for subsequent trials.

Open `.cache/audio-model/output/index.html` for listening. Each raw float WAV has
adjacent JSON with its prompt, seed, exact model/source revisions, timing, device,
peak, hash and pending listening status. Generation uses eight inference steps,
one clip at a time, two CPU threads and a 12 GiB container RAM limit. There is no
background server, browser launch or desktop/audio-device access. GPU memory is
measured, not hard-partitioned; keep other GPU-intensive work stopped during a run.

## Select a result for the game

Experiments are not automatically installed. Audition first, trim a single usable
action and process with SoX/FFmpeg; export mono 48 kHz PCM16 with headroom and short
edge fades (stereo for ambience). Put selected exports in `assets/audio/replacements/`
and use the existing [override guide](../../../assets/audio/README.md). Preserve
the adjacent provenance JSON alongside any selected WAV. Mark it AI-generated,
not a microphone recording. A fixed seed records the recipe but does not promise
byte-identical results across different GPU/software environments.

## Storage and Git

- `.cache/audio-model/`: ignored weights, downloaded upstream source, tokens,
  logs and unselected WAVs; never included in game packaging.
- Docker's own storage: image layers and Python dependencies, outside Git.
- `tools/audio/local/`: tracked, small rebuild recipes/scripts and audition prompts.
- Selected game WAVs, recipes and provenance: deliberately tracked game content.

The Docker build context is restricted to its Dockerfile and driver; it cannot
copy the checkout, tokens, weights or generated bank. Nothing modifies another
project or an existing Docker image. Removing this task's image/cache is optional;
do not prune unrelated Docker resources.
