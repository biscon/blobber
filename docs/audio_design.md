# First audio delivery

Status: implemented and mechanically verified (2026-11-02). Subjective listening and interactive acceptance remain user review.

Listening feedback found the initial bank too repetitive and the voices unsuitable.
The [authoring revision](audio_revision.md) replaces selected synth cues and adds
an isolated local AI audition workflow. Mechanical checks below are historical
delivery evidence, not acceptance of the initial sounds.

The application owns a prepared audio bank and raylib device. Simulation publishes
transient, bounded semantic observations, never raylib resources. Audio consumes
neither combat nor AI randomness, and has no save/hash representation. A compact
1024-record journal retains events across command batches; sequence IDs prevent
replay. Lower-priority events yield on exhaustion; overflow never affects gameplay.
Movement completion precedes queued movement replacement. Enemy sound follows
visual arrival, not destination reservation. Restoration establishes a baseline.

CPU audio policy owns shuffle bags, signed pan, distance attenuation, obstruction,
voice selection and generation handles. Platform audio owns resident samples,
preloaded aliases and eight ordinary Music streams. Use 64 one-shot slots (eight
reserved for critical cues), 256 cues, 512 samples, 16 variants/cue, four aliases
per sample by default (eight maximum), and a 64 MiB decoded PCM ceiling. All load,
JSON, allocation and resource creation occurs during preparation. No ECS or general
asset manager is imported. Adapt sibling engine shuffle/attenuation/alias patterns.
Raylib 6.2 has signed pan (-1 left, +1 right); do not port its old [0,1] conversion.
Enable raudio in the pinned fallback; neither looping nor low-pass patches is needed.

One representative party step per completed translation, silent turns. Enemy ranges
are 3 cells for rat/goblin/shaman, 4 skeleton/zombie, 5 bugbear; world combat/doors 6.
Direct grid obstruction blocks walls/closed doors, independently of discovery and
facing. Pausing freezes world sound, focused menu feedback remains available. Focus
loss suspends all. Stop aliases before unloading sources, unload streams before
closing the device. Missing device/assets degrade to diagnosed silence.

The bank covers fixtures, inventory, all 32 spells, eight monster definitions,
weapons, party reactions, encounter/terminal cues and restrained Underkeep ambience.
Shared spell/species families are explicit in the manifest. Burning Web and spiritual
weapons use selected nearest persistent loops, reconciled against live effects.
No full music, diffraction, muffling, or simulation/mechanics changes.
The later [music integration](combat_music.md) adds prepared playlists; dungeon
ambience now pauses during audible combat music/fades and resumes afterward.
Town doorway loading pumps only sanctuary music on the window thread, preserving
its level without reading the session borrowed by the loading worker.

Csound supplies substantive synthesis; SoX processes/exports. Python orchestrates
recipes, seeds and validation. Generated files and replacement files are separate;
overrides address stable cue/variant IDs. Selective regeneration only writes generated
files, retains recipes/provenance, and provides HTML/M3U audition outputs. Export
48 kHz PCM16, mono effects/stereo ambience, peak <= -3 dBFS, restrained tails.
All cues receive a synthesis attempt; vocal/creak realism requires listening review.

Main and pause menus share Settings: Master/Effects/Ambience/UI/Party Footsteps,
default 70/80/35/50/45 percent; zero disables a bus. Settings persist separately from
saves on leaving the submenu; --no-audio avoids device initialization.

Verification: owned CPU fixtures for timing/dedup, shuffle, spatialization/obstruction,
voice pressure/stale handles, pause/lifecycle, settings, missing assets/device and
allocation guards. Generated-file validation and bounded real-device integration
are separate from subjective listening. Build with <=2 workers, tests serial,
lengthy work nice 15/ionice idle. Do not automate desktop input.

## Delivery evidence

Implemented delivery and executed validation are recorded below.

## Implemented interfaces and ownership

`audio::Journal` is embedded in GameSession but disabled until a graphical audio
observer attaches. This avoids maintaining an unconsumed journal in headless runs.
CombatEvents forwards observations at append, not at HUD record/render; fixture,
loot, inventory, advancement and effect commits publish additional cues. Preview
calls remain silent. Consuming clears the journal, so repeated updates cannot replay
it; monotonically increasing sequence numbers survive clears. Sequence exhaustion
rejects new observations. Import clears pending observations and movement markers.
The journal preserves insertion order, evicting the oldest lowest-priority event on
overflow; no allocation or gameplay consequence is involved.

`audio::Variation`, `spatial`, `unobstructed` and `chooseVoice` are CPU-only policy.
A bounded grid traversal uses actual FloorGeometry, including closed doors, while
ignoring discovery and facing. ActorWorld's interpolated position supplies source
positions; world_render::camera supplies the listener basis at the platform boundary.
Generation-zero slots are retired; terminal voices cannot be stolen. Per-cue limits
reject excess instances; alias exhaustion replaces an eligible same-sample voice.

`AudioRuntime` owns device, bank, sounds, aliases, streams and playback slots.
The menu application owns it and Presenter borrows a pointer solely to pass it to
orchestration and shared Settings. Preparation resolves JSON and sample paths,
preallocates shuffle/voice state and checks decoded resident PCM against 64 MiB.
Device/resource work stays on the application thread. Routine update scans bounded
voices/streams and consumes observations; no filesystem operations or C++ allocations.
The guarantee excludes raylib, decoder, driver and system internals.

Travel uses the existing 250 ms fade: world gain follows the visual fade; playback
pauses during loading, old voices stop on successful world replacement, and failure
resumes the source session. Eight streams are preloaded: two ambience slots (one
active; one reserved), four selected burning-Web locations and two spiritual weapons.
Co-located burning areas share one emitter. Stable selected identities keep their
streams while nearest ordering changes. Live effect reconciliation stops expired,
dismissed or replaced effects even if journal entries were dropped.

`loadBank` resolves an editable base and optional per-cue/per-variant overrides.
Missing replacements fall back to generated base variants with a warning. Malformed
cues are disabled; absent device/bank does not prevent gameplay. Settings use a
separate atomic JSON write when leaving the shared main/pause Settings submenu;
no settings fields enter campaign files. `--no-audio` skips device initialization.
The existing source fallback has raudio enabled; an installed 6.2 package must pass
an audio API compile/link probe before its imported target is promoted for use.
Neither engine raylib patch is imported.

### Dialogue beat reactions

Implemented and mechanically verified (2026-11-06). [Multi-NPC dialogue](dialogue_design.md#multi-npc-event-beats)
adds preloaded reaction files under the shared audio root, one active dialogue voice, exactly-once
beat dispatch and its own pause domain. This voice uses Effects gain and continues
while gameplay is frozen; focus loss and other modals pause it. Beat replacement,
exit and world replacement stop it. Up to 64 source/alias pairs share the existing
512-source/64 MiB PCM ceilings; the one dialogue voice is separate from the 64
ordinary slots. `prepareDialogue`/`clearDialogue` are explicit resource boundaries.
Mock runtime tests cover synchronization/dedup, failure paths, pause, ownership and
allocation invariants. Three original synthetic placeholders and their Csound
source are under `assets/audio/dialogue/`; listening approval remains pending.

### Dialogue ambience

Implemented and mechanically verified (2026-11-06): `AudioRuntime::update` accepts an optional
ambience-pause override, defaulting to its existing world pause. Only the two
ambient stream slots use this override; effect voices/streams still use world
pause, and focus loss always suspends playback. Existing gains, stream identity
and playback position remain intact; there is no restart, fade or ducking.

Gameplay supplies an override on dialogue entry, open/exit transitions and the
closing frame. `HudSnapshot::modal(false)` excludes only dialogue when computing
other pause causes. Camp, map view and other modals still pause ambience. All
other callers, including Settings and travel, keep their existing default.
The extra bool/optional state is bounded, allocation-free and not persisted.

Verification uses the owned audio runtime fixture to check playback position,
start count, volume, effects isolation, focus/menu pause and warmed allocation
behavior, plus focused audio/dialogue tests and a game build. Listening review
remains with the user; no desktop automation or save compatibility change.

Verification: rebuilt `blobber`, `blobber_audio_runtime_tests`, `blobber_audio_tests`
and `blobber_dialogue_tests` in Release with two workers at low CPU/I/O priority.
`nice -n 15 ionice -c 3 ctest --preset release -R '^(audio_runtime|audio|dialogue)$'`
passed all three suites in 0.05 seconds. The runtime regression checks continuous
ambient decoder position, unchanged volume/start count, paused gameplay voices,
focus-loss/resume, explicit and default menu pause, and allocation-free warmed
dialogue updates. Dialogue integration checks the separate modal policy and
precedence of other overlays. No physical-device listening test was run for this
transport-only correction. Task-owned build/test logs were cleaned after recording
these results; no additional review captures were needed.

## Delivered cue inventory

There are 75 semantic cues and 218 distinct generated WAV files, including three
loops. The [bank and replacement guide](../assets/audio/README.md) and
[audition index](../assets/audio/audition.html) are the editable handoff.

- Party: six stone steps, hurt, newly downed, actual death and recovery.
- Fixtures/items: timber/iron door open/close, lock attempt, key pickup, guardian gate,
  chest opening, pickup, inventory move/drop/equip/unequip, potion use and advancement.
- Combat: ordinary/heavy swing, blunt/slash/pierce, miss, bow release and arrow impact;
  goblin/shaman/rat/skeleton/bugbear/zombie step, attack, hurt and death families.
  Goblin archer and placeholder guardian reuse goblin reactions and weapon-specific release.
- Flow: restrained UI confirm/cancel/reject, encounter, victory, defeat and travel.
- Effects: shared cast/release/impact families, effect application/end, mirror shatter,
  Web ignition and sustained burning/spectral presence.
- Underkeep: 60-second stereo room air with sparse drips and a distant creak.

Spell mapping (all current SpellKind values): radiant = Sacred Flame, Bless,
Shield of Faith, Guiding Bolt, Guidance, Resistance, Sanctuary; healing = Cure
Wounds, Spare the Dying, Lesser Restoration, Aid; dark = Bane, Inflict Wounds,
False Life; fire = Fire Bolt, Burning Hands, Scorching Ray; ice = Ray of Frost;
lightning = Shocking Grasp; acid = Acid Splash; poison = Poison Spray;
thunder = Thunderwave, Shatter; control = Sleep, Hold Person; material = Grease,
Web; arcane = Mage Armor, Magic Missile, Color Spray, Mirror Image, Spiritual Weapon.
Casts, releases and impacts intentionally share family variants in this delivery;
short per-family cooldowns coalesce simultaneous multi-target feedback.

The reusable sibling algorithms informed shuffle-bag boundary avoidance, bounded
playback slots, alias ownership and distance taper. Blobber owns its independent
implementation and test fixtures; the sibling project was not modified.

## Authoring and replacement contract

Csound instruments use modal resonance, noise-band friction, stick-slip creaks,
jittered glottal/formant vocalization, FM/noise spells and quiet room air. SoX handles
filtering, final gain, fades and PCM export with repeatable dither. Python only
orchestrates, measures and processes loop crossfades/stereo decorrelation. Recipes,
seeds, tool versions, hashes and measurements are checked in. Authoring uses atomic
same-filesystem publication and never writes replacements/overrides/base definitions.
A focused owned-fixture test verifies deterministic regeneration and replacement
preservation. All samples are original generated attempts, not licensed recordings.
Creature voices and creaks are explicitly provisional; no realism claim follows from
peak/DC/format/loop validation. The optional Csound OSC plugin reports missing liblo
on this machine; these recipes do not use that plugin and synthesis succeeds.

## Verification and limitations

The shared Settings screen was rendered in an application-owned hidden window and
inspected at 1920×1080: [capture](screenshots/audio/settings.png). No desktop mouse,
keyboard, focus or window automation was used. Interactive navigation and subjective
listening remain user acceptance.

CPU policy/event tests cover variation wraps, range/pan/walls/doors, critical reserve,
journal capacity and batch reset, zero damage/death, queued and instantaneous party
steps, silent turns, pause, enemy reservation/arrival/death cancellation, settings
fallback/roundtrip and canonical-state/RNG equivalence with audio enabled/disabled.
The mock raylib runtime test checks absent device, disabled startup, partial alias
failure, stale/cross-runtime handles, interface/world pause distinction, focus loss,
one-time consumption, alias-before-source teardown and warmed C++ allocation guards.

The bounded real-device check uses its own PCM tone fixture, waits 300 ms for backend
startup, checks measured left/right channel energy, paused silence, resumed playback,
stream wrap and cleanup. It also prepares the production bank to detect runtime
loading omissions. This confirms signal routing, not speaker quality or realism.
System-package audio compatibility is probed by CMake; the actual builds here use the
pinned source fallback. Full Debug/Release/GPU/stress matrices are intentionally not run.


### Executed checks (2026-11-02)

- Debug graphical `blobber` builds successfully with two low-priority workers.
- Graphics-disabled Debug `blobber_headless` and `blobber_audio_tests` build;
  the policy/event test passes and the headless CLI starts without audio/window dependencies.
- Focused CTest selection: **13/13 pass in 19.26 s** — audio, audio_runtime,
  core, hybrid, impact, effects, inventory, fixtures, campaign_save_smoke, buttons,
  options, menu, progression_allocations. The existing campaign save smoke fixture
  accounts for 18.79 s; audio policy/runtime tests each complete in <=0.01 s.
  No additional full save, stress, sanitizer or Release matrix was run.
- After extending the live-session allocation guard, its focused runtime test was
  rebuilt and rerun successfully; no unrelated suites were repeated for that test-only change.
- Real device check with `blobber_audio_device_check assets`: production bank load,
  actual signed stereo routing, aliases, pause/resume, stream wrap and teardown pass.
  Measured energy: left cue L=0.0285901/R=0; right cue L=0/R=0.0285965;
  paused L=R=0; resume L=R=0.0349211. Initial measurement was corrected to wait
  300 ms for backend startup rather than treating startup latency as incorrect pan.
- Authoring validation: **218/218 exports**, 170 seconds total audio including loops,
  highest peak approximately -5.9994 dBFS. DC, non-silence, PCM format and endpoints
  pass. Loop seam deltas (normalized amplitude): ambience 0.00900, fire 0.00134,
  spectral 0.02075; these mechanical bounds are not listening acceptance.
- Owned authoring fixture proves identical regenerated bytes on the recorded
  toolchain, selective rebuild, and byte-preservation of replacements/overrides.
- Settings hidden capture inspected; `git diff --check` passes.

Useful focused commands:

```sh
cmake --build cmake-build-debug --target blobber blobber_audio_tests blobber_audio_runtime_tests -j2
ctest --test-dir cmake-build-debug -j1 --output-on-failure -R '^(audio|audio_runtime)$'
cmake --build cmake-build-debug --target blobber_audio_device_check -j2
cmake-build-debug/blobber_audio_device_check assets
python3 tests/audio_authoring_tests.py
python3 tools/audio/generate.py --validate
```

Use `nice -n 15 ionice -c 3` for lengthy commands and set
`OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1` for authoring. Do not run intensive jobs
concurrently. Device check exit 77 means unavailable hardware, not a pass.

## Travel cue removal — 2026-11-05

Status: implemented and mechanically verified. Removed the application's sole
`play("travel")` call: the completion ding/air swell no longer plays after stairs,
doorway, recall or waypoint transitions. World gain restoration, music persistence
and fades remain intact. Bank entries/source samples remain authoring assets; no
replacement cue or setting was added.

Verification: the Debug game builds successfully, source search finds no remaining
runtime playback caller for this cue, and focused campaign/inn/startup checks pass.
No sound assets or mixer behavior changed. No listening session was performed;
interactive listening acceptance remains user-owned.
