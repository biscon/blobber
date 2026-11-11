# SE02 — Offensive spell identities and portrait feedback

Status: **Accepted and implemented (2026-11-05)**. User acceptance includes the
poison/healing colour corrections and option A painted combat feedback.
Parent: [spell effects roadmap](spell_effects_plan.md). SE01 remains Accepted.

World feedback backdrop follow-up: the user selected A from the
[two mockups](screenshots/combat-feedback-concepts/README.md) and requested
implementation. The [painted feedback design](combat_feedback_design.md) records
the authored damage/outcome backdrops, instant appearance, local numeral tremor
and fade-out. This supersedes the earlier decision to retain world/weapon badges.

Review correction implemented (2026-11-05): Poison Spray now has a strong green
tint and a denser cloud that holds its opacity through the first half of its
lifetime, then fades. Healing shimmer uses a distinctly emerald-green tint.
Existing prepared sprites and per-draw controls provide both changes. The
240 ms negated / 640 ms hit lifetimes, other treatments, resource ownership and
gameplay are unchanged. No new artwork or system abstraction was needed.

## Magic Missile speed — 2026-11-11

Status: **Implemented; visual feel awaits user review**. The user selected 25% slower
projectile movement. The existing rounded, distance-based flight duration is multiplied
by 4/3, rounding to the nearest millisecond (180–450 ms becomes 240–600 ms).
The established curves, dart assignments, sprites, trails and glow are preserved.

`projectileFlightMs(Cell, Cell, rules::SpellKind)` lives alongside `arrowFlightMs`
in the combat timeline. It returns the adjusted duration for Magic Missile and
the existing duration for every other kind, including ordinary arrows. It is used
for primary attacks, additional recipients and release-time rescheduling.
The combat controller continues to own arrival deadlines; the visual journal and
renderer consume those same deadlines. Damage, local alerts, impact effects and
action holds therefore follow the later arrival. No new storage, frame allocation,
settings, artwork or save fields are required.

Verification uses the existing Debug tree's configured CLion CMake/CTest
(`/home/bison/clion/bin/cmake/linux/x64/bin/`), two build workers and one test
process, with `nice -n 15 ionice -c 3` and bounded library threads.
`ctest --preset debug -R '^(spells|impact)$'` passed (0.10 s): short/capped/rounded
durations, exact arrival, pause, multiple distances, large versus split updates,
removed recipients, and unchanged other projectiles. Missile journal deadlines
are also asserted so rendering consumes the adjusted arrival for every recipient.
After strengthening those journal checks, `ctest --preset debug -R '^(spells|combat)$'`
also passed (0.18 s), including existing combat action-hold/handoff coverage.
`cmake --build --preset debug --target blobber blobber_spells_tests blobber_combat_tests`
completed successfully, including its automatic prepared-asset regeneration.

The initial build exposed stale footprint tests; the follow-up below repairs them.
No renderer or artwork changed, and GPU captures/interactive appearance were not
checked. No desktop automation was used.
The owned artifact run was cleaned; no new review images or diagnostics are retained.

### CPU fixture maintenance — 2026-11-11

Status: **Implemented and CPU verified**. Spell-effects tests now use the implemented
[subcell spell areas](spell_area_design.md): authored cue footprints are populated,
packet `subcells` checks compare both origin and mask and require clipped packets,
and area fixtures distinguish covered and uncovered squares. The multi-recipient
fan fixture fills the front band to prevent tactical movement during turn setup
from changing its six covered recipients. Shatter verifies twelve covered squares
and excluded corners; Thunderwave uses a recipient inside its eight-square template.
Missile/ray journal checks distinguish the slower missile arrival from ray timing.
Production code and existing allocation, lifetime, restore and publication checks
are unchanged by this repair.

Using the configured Debug tools and resource limits above,
`cmake --build --preset debug --target blobber_spell_effects_tests` succeeds and
`ctest --preset debug -R '^spell_effects$'` passes (0.26 s). Only this affected CPU
suite was run; no GPU/desktop checks were needed for test-fixture changes.
The owned artifact run was cleaned with no retained diagnostics or new images.

## Approved contract

The [Magic Missile timing follow-up](#magic-missile-speed--2026-11-11) supersedes
the original unchanged-impact-time requirement for that spell only.

Extend SE01 to Magic Missile, Scorching Ray, Guiding Bolt, Sacred Flame,
Shocking Grasp, Inflict Wounds, Acid Splash, Poison Spray, Thunderwave and Shatter.
Keep rules, RNG, costs, targeting, impact times and action holds unchanged.
Keep prepared-image rendering, fixed 1920×1080 presentation and discrete movement.

Force darts curve separately and preserve assigned counts; rays are narrow ragged
fire streaks with individual outcomes. Radiant bolt/flame have compact gold-white
impacts. Electricity branches at contact; necrotic contact contracts inward.
Acid throws heavy droplets, poison drifts as a short cloud. Thunderwave spreads
through the three eligible front cells; Shatter fractures its nine-cell area.
Instant spells animate from resolution; contact spells retain their 200 ms windup.
World frames use 80 ms spacing. SE02 hit remnants last 640 ms; negated contacts
fade within 240 ms. Thunderwave/Shatter releases last 640 ms. The existing SE01
timings are retained. No cosmetic timer extends gameplay deadlines.

The user explicitly included portrait healing/buff feedback in SE02: green rising
healing shimmer, blue protective/buff shimmer and element-coloured damage pulses.
Remove named party badges; keep small numbers and SAVED/MISS/IMMUNE text. Names
remain in logs/tooltips. Harmful conditions use subdued violet. Temporary HP uses
blue protection. World weapon feedback and sustained support work remain outside
this change. No party world models or new conditions.

## Ownership and interfaces

The session's ephemeral journal retains 16 cast identities, each with up to three
independent deliveries and 36 NPC area results. Deliveries link existing attack
IDs and carry dart count, deadlines and resolution, with corresponding results in
the cast's contiguous target array. Register extras as
well as primary attacks; deduplicate each outcome. Capture actual area geometry
once, before pushing actors, independently of target loops. Party outcomes route
to portrait feedback through existing command event consumption.

Expand footprint storage to nine cells with explicit caster-origin inclusion.
Shatter never includes the corridor back to its caster. Extend condition snapshots
with authoritative guiding marks; quiet alpha-masked gold coating ends on actual
consumption/expiry/death and coexists with restrained frost. No sustained light.
The view owns playback and resources, uses session time and world/restore epochs,
and reconstructs sustained state without replaying transient bursts. Removed
targets cannot gain new remnants; brief existing remnants follow visible death
poses. Misses, immunity and zero-damage saves receive short cosmetic contacts,
never damaging aftermath. Poison retains a dense initial cloud even on a save,
with a shorter lifetime and lower opacity than its damaging hit.

Portrait playback belongs to CommandController/HudSnapshot: two fixed channels
per character (supportive/hostile), latest event replaces its channel, one latest
numeric readout. UI draws prepared animation inside portrait bounds beneath status
and selection feedback. Session time freezes animation on pause; replacement and
restore clear it. Supportive shimmers last 640 ms, damage/harmful pulses 400 ms,
and negated pulses 240 ms. Draw code never mutates simulation or allocates.

## Assets, storage and failure policy

Preserve SE01 atlases. Add two 1024² body/emission pairs (sixteen eight-frame rows)
and one startup-prepared 1024×512 portrait sheet. Keep ImageGen masters/prompts and
deterministic preparation inputs under assets/effects/se02. Typed clips explicitly
map to sheets; validate UVs, timing and references. World publication 11→12;
no save schema change. Missing assets reject preparation, preserving the active view.

Retain 16 casts, 64 aftermath records, 256 secondary particles, eight lights and
1,024 world packets. Primary bodies/areas/impacts precede optional particles;
overflow deterministically drops cosmetics and increments diagnostics. Total
effects CPU storage <=1 MiB/view within the existing 8 MiB limit. Added world
atlas mip/alpha estimates: 21.34/4 MiB; added total GPU residency <=24 MiB including
portrait art. Active/staged views share textures; existing global ceilings remain.
Use existing light blockers, 1.5-cell radius, depth/picking and RAII boundaries.

## Verification and delivery

Owned CPU fixtures cover command outcomes, grouped deliveries, same-target mixed
hits, unequal deadlines, contact timing, area cover/empty cells, Shatter party and
Evocation behavior, blocked/successful pushes, immunity/death/cancel, deduplication,
pause/large steps, restore, guiding lifetime, bounded exhaustion and allocations.
Portrait tests cover identity after formation changes, classifications, overlap,
latest readout, no named badges and no refresh/restore replay.

Hidden GPU integration covers footprint clipping, alpha/additive overlap, doors,
frost/guiding coexistence, unchanged picking, idle caching, portrait layering,
both quality settings and staged cleanup. Production motion uses ordinary commands
with representative multi-target, area, dark/bright and portrait cases alongside
SE01 references. No desktop input/focus automation; interactive feel is user-owned.

One Debug configuration; focused spell/effect/impact/command/HUD/shaman/world checks
and save smoke. One intensive job, two build workers, one test process, low CPU/I/O
priority. No automatic benchmark/matrix. Record commands/outcomes, measured totals,
limits and selected evidence here. Clean owned runs after promotion. Finish Ready
for review; only explicit user acceptance closes SE02 or enables SE03 planning.

## Implementation record

The implementation follows this contract. `SpellVisualJournal` groups primary and
extra attacks by the primary attack ID, and `SpellEffectsPlayer` tracks resolved
recipients with a fixed bit mask. `spell_effects_offensive.cpp` owns the additional
visual recipes; both spell families submit primary packets before optional trails.
`EffectPacket` carries nine footprint cells and an explicit origin bit. The
billboard shader composites guiding/frost inside creature alpha without changing
picking or quiet-scene caching. Incoming Sacred Flame uses portrait feedback.

`CommandController` consumes resolved combat events into identity-indexed portrait
channels. `portrait_feedback.hpp` samples their frames/tints without graphics, and
`HudPainter` draws the startup-owned atlas below portrait borders/status icons.
Ordinary weapon and world-enemy badges initially retained their existing treatment;
the approved [painted feedback follow-up](combat_feedback_design.md) now replaces
their backdrops and adds fade/local numeral motion. Healing no longer activates the red recently-hit outline. No save fields or combat RNG/cost/
targeting rules changed.

[Source artwork, prompts and regeneration](../assets/effects/se02/README.md) are
durable. Built-in ImageGen produced the masters and cleanup edits; the offline
packer derives registered emission from authored bright pixels. Review replaced
the initial filled Thunderwave dome with a broken pressure rim and reduced its
opacity. Only that source row is selected from the final pressure master. The
five runtime PNGs reproduce byte-for-byte from the retained sources. SE01 artwork
is unchanged.

### Checks performed

One Debug configuration, at most two build workers and one test process. Commands
run from the repository root; `RUN` denotes the owned directory below.

```sh
nice -n 15 ionice -c 3 cmake --build --preset debug \
  --target blobber blobber_spell_effects_tests blobber_world_integration_gpu_tests -j2
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug --output-on-failure -j1 \
  -R '^(spell_effects|spells|expanded_spells|level_spells|effects|impact|commands|hud|shaman|campaign_save_smoke|world_integration|world_cpu|world_residency)$'
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_spell_effects_tests
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests --se02-check
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets RUN/production --se02-review
nice -n 15 ionice -c 3 python3 tools/prepare_se02_effects.py --output RUN/repacked
```

- All 13 selected CPU suites passed (21.46 s at the integration checkpoint).
  Existing suites cover costs, rolls, immunity, guiding consumption/expiry,
  Shatter party/Evocation rules, pushes and save smoke. Added checks cover all ten
  command publications, independent/out-of-order deliveries, same-target mixed
  outcomes, three assigned darts, contact deadlines, nine-cell publication,
  portrait classifications/overlap/formation identity/pause/restore/expiry, and
  incomplete or invalid clip publications. Final focused spell checks passed
  after the additional regressions and playback tuning.
- Engine-owned CPU command/resolution and frame loops reported zero allocations.
  This includes independent ray flight/contact/trails, overlap/exhaustion and
  steady quiet coatings. This does not assert allocation behavior of drivers.
- Hidden OpenGL checks passed at both quality settings: effect/color submission,
  light/door blockers, exact baseline restoration, unchanged picking, guiding
  plus frost, idle caching, footprint effects and resource cleanup. The sandbox
  initially could not connect to the display; the authorized hidden-window run
  succeeded. No desktop input or focus automation was used.
- Fifteen production cases were captured through real commands with seed 5,
  production rules/art and legal spell-party presets. Contact deadlines remained
  200 ms; single ranged deliveries 288 ms, the longest three-target deliveries
  297 ms; instant spells resolved at 0 ms. Eight affected cases were recaptured
  after fixing immediate-event consumption in the review harness and refining
  Thunderwave. The final gallery uses those corrected sequences. The harness
  asserts actual healing, buff recipients and incoming damage before capture.
- The generated sheets reproduced byte-for-byte. Selected frames and phase
  filmstrips were inspected; [four motion previews and two full-size stills](screenshots/spell-effects/se02/README.md)
  are retained for user review, alongside accepted SE01 references.

### Measured storage

These original SE02 measurements precede the painted-feedback follow-up, which
adds one 393,216-byte UI texture: combined added GPU storage is 24,859,984 bytes
(23.71 MiB), still below 24 MiB. Current dungeon application GPU is 281,327,132
bytes; world residency is unchanged.

| Storage / residency | Bytes |
| --- | ---: |
| Per-view effects pipeline | 289,872 |
| Per-view light scratch | 16,388 |
| Session journal | 32,936 |
| Added world atlas GPU mip storage | 22,369,616 |
| Added world atlas CPU alpha masks | 4,194,304 |
| Added portrait GPU texture | 2,097,152 |
| Total added GPU texture storage | 24,466,768 (23.33 MiB) |
| Dungeon world / application GPU | 157,625,808 / 280,933,916 |
| Daylight city world / application GPU | 367,272,032 / 490,580,140 |
| City + staged dungeon world / application GPU | 384,940,792 / 508,248,900 |
| Enemy shaman scene world / application GPU | 178,973,960 / 302,282,068 |

Effects plus light scratch total 306,260 bytes/view, below 1 MiB. Added GPU texture
storage stays below 24 MiB; no capacity or global memory ceiling was raised.
The full installed publication contains 409 textures, 695,273,524 GPU mip bytes
and 67,648,613 CPU alpha bytes; it is not all resident at once. The staged peak
above uses 103 shared resident textures, 358,887,192 mip bytes and 21,492,610 alpha
bytes. These are resource-accounting estimates, not driver process-memory metrics.

### Handoff and limitations

SE02 is **Accepted** (2026-11-05), including the ten offensive identities,
pressure-ring readability, three independent ray/dart deliveries, quiet guiding
mark and portrait healing/buff/damage treatments. Sacred Flame and Poison Spray
captures show successful saves; Acid Splash shows one save and one hit. Enemy
feedback now uses the approved painted splashes; hit flashes remain. Interactive feel,
cross-device appearance and production-scale performance remain unverified; small
correctness fixtures are not benchmarks. No full Release/headless/sanitizer matrix
was run. Support spell-specific and sustained presentation remains SE03, which
is now eligible for planning and requires a separately approved plan before implementation.

Owned run: `output/agent-runs/20261105T200101Z-714f6ef3910b4ff69ee74243366b9fcd`.
After promoting the six selected files, cleanup was previewed and applied:
629 disposable files / 592,173,341 bytes removed. No transient captures/logs or
failed-iteration diagnostics retained. Source artwork, accepted SE01 evidence and
build caches remain durable.


### Colour/density review correction verification

Rebuilt `blobber`, `blobber_spell_effects_tests` and
`blobber_world_integration_gpu_tests` with the same low-priority Debug build command
above. The focused `blobber_spell_effects_tests` passed, and hidden `--se02-check`
passed at both quality settings. No broader suite rerun or new palette-value tests:
this is visual tuning within the tested presentation path.

Ran the production review command above separately with final selectors
`poison-spray` and `portrait-healing`, each in a new output directory. Inspected
native captures at 120 ms (poison) and 240 ms (healing): the cloud is strongly green
and denser even on the successful-save case; healing is distinct from the existing
radiant damage pulse while the face, +5 readout and selection border remain legible.
Updated only those two panels in the motion gallery, and replaced the two selected
native stills with these corrections. Other panels retain their prior captures.
Runtime atlas/source files and measured storage totals are unchanged.

Correction run: `output/agent-runs/20261105T205246Z-841cb6112bf84adebac46d64dc08eb05`.
Cleanup previewed and applied after evidence promotion: 63 disposable files /
66,790,333 bytes removed, no retained diagnostics. SE02 was Ready for review at this correction handoff. The user subsequently
accepted the completed slice, including the painted-feedback follow-up (2026-11-05).

Acceptance recorded (2026-11-05): roadmap checkpoint, tracker, decision log,
handoff and linked status notes agree. Documentation-only verification: local
Markdown links and `git diff --check` pass; no runtime checks rerun. The empty
owned acceptance run was previewed and cleaned; no temporary artifacts retained.
