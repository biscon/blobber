# SE01 — Fire, frost and cone foundation

Status: **Accepted and implemented (2026-11-05)**.
The user explicitly accepted the delivered slice, particularly praising its local
lighting. Parent: [spell effects roadmap](spell_effects_plan.md).

## Contract and appearance

Deliver Fire Bolt, Ray of Frost and Burning Hands through ordinary spell commands,
with authored animation, selective additive glow, surface illumination and target
aftermath. Preserve rules, RNG, costs, impact deadlines, turn holds, discrete motion,
prepared assets and fixed 1920×1080 presentation. Other spells keep their renderer.

Fire Bolt curls around a compact core with an ember wake; its impact and attached
flames end 800 ms after contact. Frost is a narrow crystalline lance with a sparse
wake and shard impact; its initial coating lasts 640 ms, then a quiet coating follows
only authoritative Slow. Burning Hands emits one layered fan at instant resolution,
clipped to its three eligible front cells, including empty cells. The fan lasts
480 ms; damaged targets retain 800 ms remnants. Start authored playback at 12.5 FPS.
Trajectories use session age independently of authored frame cadence.

The user selected brief remnants that follow the visible death pose, clearing on
body disappearance or their own deadline; sustained Slow ends immediately. Misses
have no attached aftermath. Zero-damage contacts have only a compact contact burst;
Slow still follows actual authoritative state. Partial-save damage has a weaker
remnant. Existing audio, screenshake, hurt flash, markers and HUD take precedence.

## Ownership and interfaces

Combat presentation owns a SpellVisualSnapshot of 16 typed cast records, each with
at most 36 resolved target results (the existing area-result bound). Records carry
world/cosmetic serial, spell/caster, origin/facing, timestamps, stage, actual cells,
linked attack identity, target identity/position and resolved hit/damage/save/death.
Publish delayed spells at acceptance and update at impact/cancel. Publish one
Burning Hands record independently of its target loop. Retain completed cues for at least
1,000 ms before reusing their slots, independent of authoritative attack cleanup. Never consume combat RNG or
attack serials. Cosmetic overflow drops visuals, never rejects a command. These
records are not persisted; restore and session replacement clear them.

A compact world-checked Slow snapshot is derived from authoritative conditions.
FrameInput borrows snapshots for one update. View-owned SpellEffectsPlayer retains
16 cast and 64 transient target records, deduplicates stable identities, samples
absolute paused session age and cosmetic hash seeds, and never calls simulation.
Actors use world/slot IDs; these slots do not recycle. Attach to current pose/crop/
torso data. Cancellation, removal and world replacement clear matching visuals.
Offscreen age continues; large steps skip expired phases rather than spawning a
backlog. Continue reconstructs sustained Slow without replaying application bursts.

## Storage, assets and failure policy

Fixed limits: 16 cast records, 64 target aftermath records, 256 analytic secondary
particles, 8 lights, 1,024 combined effect packets, 256 sustained condition entries.
Four party casters bound live SE01 projectiles. Four full three-cell/four-occupant
cones consume 48 aftermath records, within 64. Reserve primary bodies/cones/impacts
before optional sparks, smoke and halos. The journal reclaims empty/expired slots, then oldest completed records, then
oldest cosmetic records. Playback expires records by absolute time and prefers
free slots, then oldest completed records; serial order breaks ties. Dropped identities
remain acknowledged. Light priority: projectile, cone/contact, remnant. Diagnostic
counters expose loss. No frame allocations or capacity growth.

Use ImageGen for original transparent painted pixel artwork. Keep source masters,
prompts and reproducible preparation inputs under assets/effects. Two shared
1024² atlases contain colour bodies/coatings and emission, with 128-pixel tiles,
eight-pixel gutters and at most eight frames per typed clip / 64 tiles per atlas.
Prepare premultiplied alpha and frame-safe mips, clamp sampled mip level to three.
Validate typed clip IDs, rectangles, timing, anchors, purpose and alpha conventions.
Publication format advances 10→11; source fingerprints identify changed artwork.
Interactive requirements acquire both atlases through existing active/staged leases.
Invalid required assets reject preparation without replacing the playable view.

Two full-mip atlases add about 10.67 MiB GPU / 2 MiB CPU alpha; active/staged views
share them. Added CPU storage is capped at 512 KiB per view, within the existing
8 MiB view limit. Each view has one 16 KiB light data texture and a matching CPU upload array. Combined
added resident GPU storage stays below 11 MiB, including both views. Existing world
CPU/GPU/count ceilings remain unchanged. Record measured totals below at handoff.

## Composition, illumination and cache boundaries

Packets add frame UVs, alpha/additive selection and cone-footprint clipping. Keep
transparent depth order; batch only adjacent compatible packets. Emission adds no
fog colour. Effects write neither picking IDs nor depth. Frost is sampled inside
the current creature colour pass, masked by current pose alpha and registered crop
coordinates; terrain/leaf clips and hurt-flash precedence remain. Protruding fire
uses attached textured quads and normal world/near-body/door depth rejection.

Eight local lights have at most 1.5-cell radius. Add surface-facing coloured
attenuation before fog while retaining AO/shade. Painted creatures receive a weaker
contribution. Zero lights preserve the accepted calculation exactly.

One 32×32 RGBA32F data texture carries lights, each light's 5×5 cell neighbourhood,
and all 128 existing door boxes. Each cell records solids, floor/ceiling and door
references. Bounded grid traversal (eight cells) tests the segment to the receiver;
corner handling is conservative and endpoint bias permits the near face without
transmission. The user selected animated leaf-following light: in door cells test
actual jamb/header/leaf boxes, not logical closed-cell flags. Invalid/out-of-bounds
queries are occluded. Building footprints remain conservative blockers; no global
shadow maps, detailed mesh shadows or creature-cast shadows.

Extend GPU resources with checked allocation-free updates of preallocated textures.
Use existing typed handles/RAII and track transfers. Upload only changed data.
Effect changes invalidate colour, not picking/geometry. Cache environment AO by
view identity, camera, geometry and door pose, independently of actors/effects;
staging updates shared scratch ownership. After transients finish, static sustained
frost permits idle caching. Do not add shaders/targets beyond existing pool limits.

## Verification and delivery

Use generated owned fixtures for snapshot/phase/condition/damage tests, malformed
publication inputs, deterministic exhaustion and allocation guards. Cover one cone
with zero/one/many recipients; same-time identities; rejection/cancellation;
miss/immune/partial save/lethal/removal; repeated snapshots; out-of-order outcomes;
pause/large steps/offscreen re-entry; attack-slot cleanup; restore without bursts.
Check unchanged RNG, damage, costs and action/turn deadlines.

Hidden GPU checks cover alpha/additive overlap, UV/mip edges, creature alpha/crop/
hurt/death, nearer bodies, walls/corners/animated doors, light removal, unchanged
picking, AO reuse, idle caching, both quality modes and staged rollback/release.
Extend the existing hidden review runner using real accepted commands and in-process
session time. Review short production motion sequences near/far, sparse/crowded,
dark/bright plus unchanged no-light dungeon/city/inn baselines. Interactive feel
and delivered acceptance remain user-owned; never inject OS input or change focus.

One build configuration and focused spell/impact/effect/world/asset checks plus
campaign-save smoke for restore integration. One intensive job, two build workers,
one test process, low CPU/I/O priority. No routine benchmark or full matrix.
Owned disposable run: output/agent-runs/20261105T182744Z-a6c0da133e594e36bf4c273886dead26.
Promote <=12 selected images and compact motion evidence; clean the run afterward.
Record exact checks, resource totals, deviations and limitations here. Stop at
Ready for review; SE02 remains Planned.

## Implementation and verification record

Implemented modules: `game/spell_visuals.*` owns the ephemeral cast journal;
`render/spell_effects.*` owns view-local playback; `render/spell_lights.*` prepares
bounded light/occluder data and its CPU reference; `platform/spell_lighting_shader.hpp`
shares the GLSL blocker logic between environment and creature passes. The existing
renderer owns GPU textures and colour composition. `assets/effect_clips.hpp` defines
typed prepared clips; publication format 11 validates them before interactive leases.
Restore increments a journal epoch even when world identity stays the same, clears
transient playback and reconstructs only authoritative Slow. Same-world cast serials
do not recycle. Renderer statistics expose visual-drop counters.

The final cone uses three broad, overlapping depth layers clipped to the actual
footprint, independent of recipient count. Its expansion is entirely cosmetic.
Frost coating is registered against the original sprite canvas inside pose alpha;
steady Slow uses strength 0.28 and the initial coating fades from 0.70. Generated
artwork, prompts and byte-identical offline atlas reproduction are retained in
[assets/effects/se01](../assets/effects/se01/README.md).

### Executed checks (Debug, 2026-11-05)

Commands below were run with `nice -n 15 ionice -c 3`; builds used at most two
workers and tests/captures ran sequentially. No benchmark or configuration matrix.

- `cmake --build --preset debug --target blobber_spell_effects_tests blobber_world_integration_gpu_tests blobber -j2`:
  application, focused runners and production publication built successfully.
- `ctest --test-dir cmake-build-debug --output-on-failure -j1 -R '^(spell_effects|expanded_spells|spells|effects|impact|campaign_save_smoke|world_integration|world_cpu|world_residency)$'`:
  the eight existing suites passed. The new SE01 suite first rejected an invalid
  armor-class test fixture; after correcting it to the supported range,
  `ctest --test-dir cmake-build-debug --output-on-failure -V -R '^spell_effects$'`
  passed. All nine selected suites pass on the final implementation; only the
  affected SE01 suite was repeated after presentation refinements.
- `./cmake-build-debug/blobber_world_integration_gpu_tests --se01-check`:
  hidden OpenGL checks passed at both quality settings. Includes CPU/GLSL light
  parity for walls, floor/ceiling and closed/half/open real door leaves; colour
  changes, exact no-effect restoration, unchanged picking, AO/visibility reuse,
  masked cropped-sprite frost, idle caching, zero engine C++ frame allocations,
  staged failure/cancel cleanup and complete resource release.
- `./cmake-build-debug/blobber_world_ao_gpu_tests` and
  `./cmake-build-debug/blobber_world_integration_gpu_tests`:
  existing hidden AO and world integration checks passed. These were not repeated
  for subsequent cone-layout-only tuning.
- `./cmake-build-debug/blobber_world_integration_gpu_tests cmake-build-debug/assets RUN/production --se01-review`:
  seven deterministic production-art scenarios passed, using ordinary accepted
  `GameSession::submit` spells, production rules and seed 5. All selected attacks
  hit; legal release/impact offsets were Fire Bolt range 3: 0/288 ms,
  Ray of Frost range 2: 0/234 ms, Burning Hands: 0/0 ms, near fire: 0/180 ms,
  far frost: 0/342 ms, daylight fire: 0/234 ms. Main spells captured every 60 ms
  through 1,320 ms; an additional frost capture at 1,700 ms shows quiet Slow.
  Cone tuning was recaptured using the optional `burning-hands` / `cone-sparse`
  case filter. Captures use hidden application targets, no OS input/focus changes.
- `python3 tools/prepare_spell_effects.py --output RUN/repacked` followed by
  byte comparison against both runtime atlases: identical output.

CPU coverage includes real accepted/rejected casts and actual resolved damage,
miss/lethal/removed targets, one cue for 1/12 cone recipients, empty-cell footprint,
synthetic zero-recipient playback, cancellation, zero-damage contact, exact expiry,
pause, large jumps, offscreen return, visible dying poses, Slow expiry, restore epoch,
bounded overload, producer/playback allocation guards and malformed publications.
Existing spell/impact suites cover unchanged combat costs, saves and deadlines.

### Measured bounds

| Storage / production scenario | Bytes / count |
| --- | ---: |
| Entire `EffectsPipeline` CPU object, including existing effects | 233,816 |
| Light CPU upload structure (16,384-byte payload plus count) | 16,388 |
| Session-owned cast journal | 29,480 |
| Two shared full-mip effect textures | 11,184,808 GPU bytes |
| Two shared atlas alpha masks | 2,097,152 CPU bytes |
| Per-view light GPU texture | 16,384 |
| Dungeon corridor resident textures / alpha | 110,374,696 / 15,987,586 |
| Dungeon corridor world / application GPU | 135,256,864 / 256,467,820 |
| Daylight city world / application GPU | 344,902,416 / 466,113,372 |
| City + staged dungeon resident textures / alpha / count | 336,517,576 / 17,298,306 / 99 |
| City + staged dungeon world / application GPU peak | 362,571,176 / 483,782,132 |

Existing ceilings were not raised. Two atlases plus two light textures consume
11,217,576 GPU bytes, below the added 11 MiB bound. Playback plus light scratch is
250,204 CPU bytes, below 512 KiB, even counting the pre-existing effect pipeline.
The installed publication contains 405 textures and 672,903,908 mip bytes across
all packs; this is disk content, not simultaneous residency. The staging check
retains the city while preparing the dungeon and then releases the staged view.

### Review, deviations and limits

The user accepted SE01 after reviewing the [selected motion and stills](screenshots/spell-effects/README.md),
with particular praise for local lighting. These are now accepted references for
subsequent slices. SE02 is next eligible and remains Planned. Acceptance does not
change the recorded verification limits or authorize SE02 implementation.

Existing rules reject an entirely empty Burning Hands command (`MissingTarget`).
That rule remains unchanged. Accepted cones fill eligible empty cells, and an owned
synthetic zero-recipient cue verifies that rendering does not depend on victims.
Local lights conservatively block at building footprints; detailed mesh shadows
and creature-cast shadows remain out of scope. GPU checks used the available local
OpenGL device, not a cross-device matrix. Exact no-light comparisons use owned test
scenes; production city/inn screenshot equivalence and interactive play were not
rerun. New effect-local allocation assertions cover engine C++ allocations, not
third-party or driver internals.

The disposable run is cleaned after selected evidence promotion. Generated source
masters and preparation inputs remain durable assets; build caches are retained.
Acceptance bookkeeping (2026-11-05): synchronized the roadmap, design and evidence
status; retained the distinct reference cases described in the gallery. Documentation
links and whitespace checked; no code changes or new runtime tests. The bookkeeping
run was cleaned without retained disposable output.

No unresolved failure diagnostics are retained. Local Markdown targets, retained
image sizes/animation duration and `git diff --check` were verified.
