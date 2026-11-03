# Organic audio candidate pass

Status: closed after user listening review, 2026-11-02. The model approach is
rejected for ongoing authoring. Keep only dark B, radiant A, fire B, thunder B and
spiritual.loop A. Unreviewed cues mean keep the current synth. Encounter requires
a short bell/chime instead of the drum/heartbeat attempt.

## Selection and retirement design

Copy the five selected processed WAVs byte-for-byte into tracked replacements,
with source/model/prompt/seed/hash/processing provenance and the user's exported
choices. Map each selected cue directly to its one retained variant in the base
bank; preserve cue identity, mix settings and loop behavior. Delete the thirteen
superseded synth WAV variants and stale build copies. Mark retained recipes as
read-only imported samples: Csound regeneration validates/preserves them and
does not recreate discarded alternatives. User overrides remain supported.

Replace encounter with a brief single struck chime, distinct from the longer
level-up bell. Remove the unused drum instrument. Publish a small selected-sounds
page for the five retained samples and encounter, with no rejected comparisons.

After verifying retained hashes and bank references, remove this task's Docker
image/layers, model cache (including local credentials and every rejected/raw
experiment), installer/inference/review tooling and its now-obsolete tests. Keep
the core Csound authoring workflow and test retained-sample preservation using
owned fixtures. Do not prune unrelated Docker resources or modify Downloads.
Final validation checks every active sample, orphan-free source/build audio,
focused audio suites and a working Debug build. Historical evidence follows.
User listening feedback accepts the revised synth
palette, specifically retaining potion, healing and control. The local model is
promising but its first hurt attempt sounded like a sigh. Generate more useful
candidate performances before selecting replacements.

## Historical design and scope

Keep gameplay, cue names, event timing and approved synth assets. `encounter` is
the cue emitted by `CombatEvent::Started`; redesign its Csound recipe as a short,
restrained low drum/rising tension accent, distinct from UI and victory chimes.
Publish its two new variants into the current synthesized bank. Preserve the old
encounter sample in ignored audition storage for comparison.

Use pinned Stable Audio Small-SFX offline in the existing Docker container. Render
two independently prompted/seeded candidates for every weapon cue, six creature
families (step/attack/hurt/death), party steps/reactions, doors/gate/chest/lock,
spell families except healing/control, mirror shatter/Web ignition, and all three
loops. Keep potion and the accepted synth cues. Creature prompts describe separate
physical actions and performances, not transpositions. Bugbear alternative B uses
an exaggerated Arnold Schwarzenegger-style action-movie grunt as requested;
this is a playful generated performance experiment, not a voice recording or clone.
Skeletons use bones/joints rather than voiced grunts. Rats scuttle/squeak, zombies
shuffle/groan, goblins snarl/yelp, and shamans rasp with cloth/charms.

Add a validated batch manifest to the existing authoring CLI: sequential inference,
one loaded model, checkpointed per-file output, and resume only when prompt/seed/
model/hash metadata match. Keep all unselected raw and processed audio beneath
ignored `.cache/audio-model/organic-pass/`; track prompts, recipes and small tooling.
No automatic AI replacement of the active bank, no new dependencies or account.

Host SoX processes exports: reject invalid/silent data, remove DC/low rumble,
resample, preserve a pre-onset margin, trim excess silence without time-compression,
short edge fades and peak headroom. Retain raw generations to inspect trimming
mistakes or multi-action outputs. Mono one-shots/persistent emitters, stereo room
ambience, 48 kHz PCM16. Loop candidates use wrap crossfades, no fade to silence;
also export two consecutive copies for exact join audition. Mechanical loop checks
do not certify perceptual seamlessness. Record hashes and processing measurements.

Build a dedicated offline HTML comparison page with category filters, cue search,
collapsible creature groups, current/A/B players, raw previews and loop-join clips.
Only one player runs at a time; audio is never autoplayed/preloaded. Local listening
choices/notes can be exported as JSON for later selection. No server, desktop
automation, model downloads or experiment WAVs enter Git/game packaging.

## Verification

Use owned small audio fixtures to verify trimming preserves attacks, loop wrapping,
export format/headroom, source preservation and escaped valid audition links.
Check manifest coverage against current cue inventory, complete offline generation,
finite/non-silent outputs and processing reports. Existing authoring fixtures cover
deterministic selective Csound regeneration and user replacement preservation.
Build/copy the updated encounter assets and run the focused audio suites; no broad
simulation or GPU matrix. One intensive job at a time, two CPU workers, bounded
container RAM, low priority; no speculative large stress run. Listening quality,
action timing and any questionable vocal results remain explicit review items.

## Delivered evidence

- **56 cues / 112 AI candidates**, all generated offline using the pinned Small-SFX
  model. Includes eight weapon cues, 24 creature cues, four party cues, five fixture
  cues, twelve spell/effect cues and three loop cues. Each has two different prompts
  and seeds. Party A/B explore masculine/feminine reactions without adding runtime
  character voice mapping. Bugbear B attack/hurt/death explicitly prompt the requested
  exaggerated AAAARGH / OOOOUCH / WAAAAARGH action-movie performances.
- All 112 raw files have checked prompt/seed/hash provenance and finite output.
  Their processed previews pass 48 kHz PCM16, headroom, DC, non-silence and one-shot
  endpoint checks. Six loop candidates have join discontinuities from 0.000061 to
  0.002686 normalized amplitude; each join-preview WAV is exactly two concatenated
  copies. This proves signal continuity bounds, not perceptual seamlessness.
- Many candidates (100) trigger conservative long-tail or separated-burst review
  flags. These are deliberately still auditions, not automatically cut single-hit
  runtime replacements. Raw clips are available next to each processed preview.
  The processed candidates total approximately 333 seconds; unselected outputs,
  browser checks and reports occupy roughly 210 MiB in the ignored cache.
- The former separate page was `.cache/audio-model/organic-pass/index.html`, generated
  by the now-removed model review tools and HTML template. It had
  57 cue comparisons including encounter, filtered groups, collapsible creatures,
  current/A/B playback, loop-join previews and exportable notes. A hidden Chromium
  check verifies cue search, category filtering, stopping other players on play,
  and local note persistence; screenshot visually inspected. No OS input, focus
  change or speaker playback occurred. Owned HTML fixtures verify escaping and links.
- Owned audio fixtures pass attack-padding/trim, stereo wrap, exact loop repeats,
  source-byte preservation, silence rejection and stale/corrupt batch-resume checks.
  A second real batch invocation reports zero candidates pending without loading
  the model, proving successful reuse of the matching completed set.
- The encounter is now a 0.72-second Csound drum pair with a brushed tension accent,
  not a metallic FM sweep. Two variants are in the bank and Debug asset copy. Its
  authoritative trigger remains `CombatEvent::Started`. Only encounter audio and
  cue mix parameters change in the running game; AI candidates stay unselected.
- Existing owned Csound authoring tests pass. Debug `blobber` builds with two
  low-priority workers; focused `audio`/`audio_runtime` suites pass (2/2, 0.03 s).
  No gameplay, save, audio-event ownership, device or runtime code changes.

The user rejected most of the model's output, especially creature sounds, and
requested retirement. The final selections supersede the pending review above.

## Completed selection and cleanup

- Imported the five selected processed files byte-for-byte into
  `assets/audio/replacements/` and mapped each cue to its one selected variant.
  Prompt, seed, exact model/source revisions, processing details and hashes are
  preserved in `replacements/provenance.json`. The original user export is recorded
  in [audio_listening_choices.json](audio_listening_choices.json); Downloads is unchanged.
  Unreviewed and omitted entries preserve the current synth sounds.
- Encounter now uses one short 0.48-second struck chime, with no heartbeat/drum
  rhythm. The unused drum instrument was removed. The concise retained-sounds
  audition page is [selected-audition.html](../assets/audio/selected-audition.html).
- Deleted all 13 superseded synth WAVs and their stale Debug copies. Deleted the
  entire 3.6 GiB task model cache, local HF credentials, upstream source clone,
  raw/unselected/processed experiments and rejected comparison pages. Removed ten
  task-specific Docker image/layer records without pruning unrelated images.
  Removed the installer/inference/review tools and their obsolete model tests.
- Source and Debug asset directories each contain exactly 210 referenced WAVs:
  205 synthesized samples and five retained model exports, across 75 cues. No
  orphan WAVs remain in either directory. Selected hashes, audition links and
  absence of task-specific model images/cache/tools were verified.
- The Csound authoring workflow explicitly validates/preserves retained samples;
  it never regenerates their discarded synth alternatives. Owned fixtures prove
  byte preservation and missing-retained-file failure. All 210 active audio files
  pass mechanical validation. Debug build and focused audio/audio_runtime suites
  pass (2/2, 0.02 s); `git diff --check` passes. No device/gameplay/save changes.

The retained samples reflect the user's choices. The new short encounter chime is
implemented per their note, with subjective acceptance left to their listening.
