# SE03 — Support, conditions and persistent effects

Status: **Accepted and implemented (2026-11-06)**.
The user explicitly accepted SE03 and its review corrections, closing E-B. SE04 is now [Ready for review](se04_design.md).
Parent: [spell effects roadmap](spell_effects_plan.md). SE01–SE02 remain Accepted.
The user approved the detailed plan and selected selective quiet portrait cues.

## Contract and visual recipes

Complete the remaining nineteen spells without changing rules, RNG, costs, targets,
authoritative deadlines, saves, discrete movement or fixed 1920×1080 presentation.
Cure Wounds retains green healing; Spare the Dying settles green without invented HP;
Lesser Restoration sweeps away the actual cured condition. Aid uses blue vitality,
False Life inward protection. Bless rises, Guidance converges, Resistance encloses;
ordinary party boons subsequently rely on status icons. NPC Bless retains a faint
mark. Bane descends violet. Shield of Faith, Mage Armor and Sanctuary use shield,
fitted sheen and veil respectively. Mirror Image uses existing portrait echoes
matching actual duplicate bits, never reservations. Sleep drifts across its real
footprint and leaves motes; Color Spray is one clipped prismatic fan; Hold Person
uses tightening bands. Retain prone/held poses and successful-save distinctions.
Grease spreads briefly then stays static; Web unfurls and burns as strands/flames
on actual cells. Spiritual Weapon retains its image with summon, endpoint movement,
strike and dismissal accents; no interpolation through walls.

Application: 640ms at 80ms/frame; negation/end: 240ms; weapon movement: 320ms.
Visible sleep/burning-Web loops sample every 160ms. Existing SE01–SE02 timing stays.

## Ownership and interfaces

A session-owned SupportVisualJournal retains 128 typed cosmetic records with world,
epoch, serial, occurrence time, source/target, effect/area identity and cell data.
CombatEvents directly observes resolved events, like the existing audio observer.
Explicit ground/movement/duplicate hooks fill missing presentation data. No event
bus, simulation executor or saved cosmetic state. Overflow replaces oldest records
and increments diagnostics, never gates gameplay. World/HUD consumers deduplicate
serials even when recipients are invisible. Restore clears transients and rebuilds
sustained state, never application bursts. Removal/death clears attachments.

The condition visual snapshot contains fixed NPC/party masks and duplicate bits;
construct from bounded existing state, not repeated per-actor effect scans. Ground
and weapon identity derive from existing owners; saved structs/schemas remain intact.
The sixteen-entry cast journal publishes one Sleep/Color Spray footprint independent
of recipients. Their HP budget/immunity rules remain authoritative.

World support recipes live beside existing spell playback. Reuse depth, footprint,
attachments and blocker-aware light paths. Two sustained accents per creature,
priority Hold/Restrained, Sleep/Blind, Bane, Guiding, Frost, Bless. Portraits retain
one ward and one disabling-condition cue beneath selection/status/readouts. Mirror
Image occupies the ward channel. New casts outrank sustained decoration. Static
cues preserve caching; visible animation invalidates colour only at its cadence.

Keep 64 aftermath records, 256 particles, 8 lights, 1024 packets, <=1MiB effects
storage/view and existing global ceilings. Essential ground/weapon art precedes
cast/application/impact bodies, then decoration/particles; overflow drops cosmetics
deterministically. Web fire deduplicates by cell and uses low-priority warm lights.
Grease never burns. Ending authoritative state removes its cue immediately, with
optional 240ms dissolution except death/removal/reset. Cosmetic timers never expire
conditions or prolong gameplay holds.

## Assets and preparation

Use built-in ImageGen, retaining masters/prompts and deterministic offline packing.
Two 1024-square body/emission pairs and one 1024-square portrait sheet: <=26MiB
added GPU texture storage, approximately 4MiB world alpha. Reuse accepted assets.
Typed clip validation and source fingerprints include new art; publication 12→13.
Normal startup packages required art. Missing assets reject preparation while
preserving the active view. Active/staged views share leases; measure actual totals.

## Verification and handoff

One Debug configuration, one intensive job at low CPU/I/O priority, <=2 build workers
and one test process. Owned CPU fixtures cover production commands, all nineteen
recipes, area geometry/HP budgets/saves, NPC Bless, effect lifetimes, partial mirror
loss, weapon movement/strike/miss, burning overlap, dedup/pause/large steps/restore,
formation identity, bounded exhaustion and zero engine-owned frame allocations.
Focused suites: spell_effects, spells, expanded_spells, level_spells, effects, impact,
commands, hud, shaman, world_cpu, world_integration, world_residency,
campaign_save_smoke. No full persistence/benchmark/configuration matrix.
Hidden GPU checks cover both quality modes, clipping/depth/doors, portraits, ground,
lights, picking, cache recovery and rollback. Production command-driven motion uses
recovery/wards, conditions, shaman, burning Web and weapon cases. No OS input/focus
control; interactive feel and unavailable checks remain explicitly unverified.
Own output through the artifact helper; promote <=6 selected SE03 evidence files,
record exact checks/totals/limits here and clean disposable output. Finish Ready for
review. Only user acceptance closes SE03/E-B; SE04 remains Planned.

## Implemented boundaries

`game/support_visuals.*` owns the 128-record journal. `CombatEvents` observes resolved
outcomes directly; ground placement, weapon movement, cleansing and duplicate loss
publish the missing facts at their authoritative owners. Whole-effect endings resolve
all recipients before deactivation. A new application supersedes the old application
on that recipient; ending a condition cancels its outstanding application immediately.
The journal stores typed IDs, not long-lived actor pointers. Its sole borrowed
controller reference stays within the owning session. There is no new save data.

`EffectController::visualConditions()` builds fixed masks and real duplicate bits
in bounded passes. `GameView` supplies that snapshot and journal to world/HUD consumers.
`render/spell_effects_support.cpp` samples records analytically; no second aftermath
pool is needed. Existing caps remain 16 casts, 64 remnants, 256 particles, 8 lights
and 1024 packets. Static marks reuse cached colour; sleep/fire animation samples the
authored 4,5,6,7,6,5 frame loop every 160ms. Invisible records still age by simulation
time. Co-located burning Web cells share one flame/light contribution. Legacy orange
Web tint and duplicate support glints yield to these authored recipes.

HUD feedback retains its two existing transient channels and adds typed support
profiles. Ordinary generic application feedback is replaced in its channel when the
journal is consumed, not layered twice. Healing keeps its accepted green treatment.
Sustained portrait wards and disabling cues are quiet; Mirror Image uses the actual
three-bit state, draws clipped portrait echoes and fades the precise consumed echo.
Existing status names/icons and readouts remain authoritative. Application rendering
is beneath selection, status and text. NPC accents are capped at two in the documented
priority order, including frost and guiding overlays.

Publication format is now 13. Two world body/emission pairs and one startup portrait
atlas come from [built-in ImageGen masters and exact prompts](../assets/effects/se03/README.md).
`tools/prepare_se03_effects.py` deterministically packs the masters; all five regenerated
runtime PNGs matched the checked-in files byte for byte. New atlas rows use typed clip
metadata and the existing prepared-image path. The application target copies the new
portrait atlas during normal asset preparation.

## Verification performed — 2026-11-05

One Debug configuration was built with `nice -n 15 ionice -c 3 cmake --build
cmake-build-debug --target ... -j2`, covering `blobber`, the GPU integration runner
and each test target below. No compiler warnings were reported. Only one intensive
job ran at a time.

```sh
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug --output-on-failure -j1 \
  -R '^(spell_effects|spells|expanded_spells|level_spells|effects|impact|commands|hud|shaman|campaign_save_smoke|world_integration|world_cpu|world_residency)$'
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests --se03-check
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets RUN/production --se03-review
```

All **13 selected CPU suites pass**. The first selected run passed 12/13 in 21.67s:
world integration still asserted exactly four old effect packets. Its fixture now
places Web in view and checks actual Web body/fire/light, weapon and projectile
invariants instead of obsolete packet count. Only that affected target was rebuilt
and rerun; it passed in 0.12s. `spell_effects` also passed directly with SE01–SE03
checks. New checks exercise all nineteen spells through ordinary accepted commands,
restored sustained state without application replay, Mirror Image reservation versus
actual consumption, per-recipient end events, bounded overwrite diagnostics,
condition priority, overlap deduplication, idle caching and zero engine-owned
command/HUD/render allocations. Existing selected gameplay suites cover saves,
area budgets and condition/ground/weapon lifetimes.

**Hidden GPU checks passed at both quality settings**, including existing depth,
door/blocker lighting parity, alpha/frost integration, unchanged picking IDs,
visibility/AO caching and resource rollback/destruction; SE03 adds sleep, burning
Web, idle-baseline recovery and warmed allocation checks. The sandbox could not
connect to display `:0`; the authorized hidden-window run succeeded outside that
restriction. No OS input, focus changes or desktop automation were used.

**Sixteen production cases passed**, using production rules/art, legal party
advancement/preparation and normal accepted commands. These cover stabilization,
cleansing, Aid, Mirror Image, Shield of Faith, daylight Sanctuary, Sleep, Color Spray,
Hold Person, Bane, Grease, Web, burning Web, weapon summon/strike, weapon movement and
actual shaman Bless. Warmed presentation checks report no engine-owned frame
allocations or GPU errors. Initial capture-only fixture issues were corrected:
real-time Approach does not wait for a turn; seed 3 keeps Web's target restrained
until Fire Bolt ignites the cell; weapon movement resumes exploration after Victory.
A bounded eight-seed windowless fixture probe selected that ignition case. The other
cases use seed 5. Previously successful cases were not recaptured unnecessarily.

The [six-file gallery](screenshots/spell-effects/se03/README.md) preserves three
annotated motion comparisons and three native-resolution stills. Application,
settled and movement phases were inspected. The representative checks establish
correctness, not production-scale performance. Interactive feel, cross-device GPU
appearance and Release/sanitizer/full persistence matrices were not tested in this
slice. No benchmark or new desktop-control investigation was run.

## Measured storage and resource limits

| Storage | Bytes |
| --- | ---: |
| Effects pipeline per view | 293,384 |
| Light scratch per view | 16,388 |
| Combined effects storage per view | 309,772 (below 1 MiB) |
| Existing cast journal per session | 32,936 |
| New support journal per session | 18,488 |
| Condition snapshot | 1,120 |
| New world atlas GPU mip payload | 22,369,616 |
| New world alpha planes | 4,194,304 |
| New portrait GPU payload | 4,194,304 |
| Total added GPU payload | 26,563,920 (25.33 MiB; below 26 MiB) |

The complete installed publication has 413 textures, 717,643,140 mip bytes and
71,842,917 alpha bytes; these are not all simultaneously resident. Representative
measured active/staged totals remain below configured ceilings:

| Case | Textures | Mip bytes | Alpha bytes | World GPU bytes | Application GPU bytes | Capture peak bytes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Dungeon/goblins | 97 | 152,585,740 | 23,903,590 | 177,467,236 | 305,362,864 | 321,951,664 |
| Daylight city | 90 | 362,252,292 | 24,680,165 | 387,113,460 | 515,009,088 | 531,597,888 |
| City plus staged dungeon | 107 | 378,728,620 | 25,214,310 | 404,782,220 | 532,677,848 | 532,677,848 |
| Shaman plus goblins | 153 | 208,804,392 | 34,457,108 | 233,685,888 | 361,581,516 | 378,170,316 |

These are engine resource-accounting payload estimates, not measured graphics-driver
or process memory. No claim is made that third-party internals never allocate.

## Handoff and artifact retention

SE03 and E-B are Accepted by the user (2026-11-06). SE01–SE02 acceptance
is unchanged and SE04 was not started. Gameplay, RNG, costs, deadlines, save schemas,
prepared-image presentation and discrete movement remain unchanged.

Six selected evidence files are retained in the gallery. Source masters, prompts,
packer and fixtures are durable inputs. The owned run
`20261105T215912Z-973947522e81471db97b8eb8cc90ee52` was cleaned after promotion,
including unused captures, failed-fixture diagnostics, logs and regeneration/probe
output. There are no retained diagnostic exceptions or unclosed runners.

## Review correction — 2026-11-05 (implemented; Ready for review)

User feedback requests lower NPC Bless placement, a gently hovering Spiritual Weapon,
and clarification of its later-turn control. Anchor both transient and settled Bless
to the registered feet midpoint, compensating for the prepared tile padding; other
conditions retain their torso anchors. Sample a ±0.025-cell, 2.4-second sinusoidal
weapon hover every 40ms using existing presentation time. This affects only packet
positions and colour invalidation while the weapon projects into view; pause, removal
and offscreen idle caching remain intact. Summon/strike/movement endpoint accents
share the hover position. No new textures, allocations, gameplay movement or save state.

The user confirmed the command works and additionally requested attack flight/return.
Use the existing per-caster attack visual slot: lunge during its 200ms windup, reach
the recorded enemy subcell at the authoritative impact, then return within 240ms.
A small tilt follows the lunge. Read the bounded snapshot directly; retain no new
attack history. Canceled attacks immediately resume hovering; missing/dead targets
use the recorded point, and an absent/replaced weapon never leaves a ghost. Weapon
movement still snaps to its legal destination cell before its local attack. The
new motion must not extend action holds or change the authoritative cell.

Verify the existing Spellbook → Abilities → Command Spiritual Weapon UI on the cleric's
next round, including its independent once-per-round allowance, optional attack and
zero additional mana/normal-action cost. Clarify that route in the displayed spell
description without changing catalog fingerprints. Focus checks on spell effects,
level spells and the two revised hidden production captures, not the prior full matrix.


Correction verification: Debug `blobber`, `blobber_spell_effects_tests`,
`blobber_level_spells_tests` and `blobber_world_integration_gpu_tests` built with
`nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target ... -j2`; no compiler
warnings. `ctest --test-dir cmake-build-debug --output-on-failure -j1 -R
'^(spell_effects|level_spells)$'` passed 2/2 in 0.25s. After the user's additional
lunge request, the affected `spell_effects` suite passed again in 0.22s. Tests prove
next-round Abilities selection/target/confirm before and after the cleric's normal
action, one attack with no mana or normal-action expenditure, and rejection of a
second command in that round. Rendering checks cover feet anchoring, bounded
vertical hover, pause/offscreen/removal caching, zero hover allocations, lunge
contact timing, disappeared targets, return to the unchanged authoritative cell
and cancellation.

Three authorized hidden production captures passed with the ordinary rules/art:

```sh
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets RUN/bless --se03-review npc-bless
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets RUN/strike --se03-review weapon
nice -n 15 ionice -c 3 ./cmake-build-debug/blobber_world_integration_gpu_tests \
  cmake-build-debug/assets RUN/idle --se03-review weapon-idle
```

Inspected the lowered gold base between the feet, the 200ms contact/240ms return
against a killed enemy, and the complete 2400ms idle cycle. Production checks include
warmed zero engine-owned frame allocations and GL error checks. Texture/resource
payloads match the corresponding dungeon/shaman rows above; the pipeline adds one
8-byte cache timestamp, with no new assets or pools. The prior both-quality GPU
matrix was not repeated for these packet-position/text changes. No OS input/focus
control was used; interactive feel remains for user review.

The persistent GIF was replaced with corrected Bless, attack/return and idle panels,
retaining the earlier Grease/Web/fire panels. The gallery remains six files. The
owned run `20261105T224822Z-4e648c7d08e040da955926fdeb305b08` was cleaned after promotion;
no diagnostics are retained. SE03 was subsequently accepted on 2026-11-06; SE04 remains Planned.

Current game controls: the summon costs one normal action and 3 mana, immediately
rolls a melee spell attack for 1d8 + Wisdom force damage, and lasts 60 gameplay
seconds without concentration. On later cleric turns use Spellbook → Abilities →
Command Spiritual Weapon. As of the [2026-11-06 control correction](spells_design.md#single-enemy-command-activation),
click an enemy to attack immediately, or select a floor cell and confirm to move. This allows
up to two clear cardinal floor steps and an optional attack in the destination cell,
once per round (once per six gameplay seconds outside turns), without more mana or
normal-action cost. It does not take autonomous turns. The user confirmed that
commanding it works; the description change improves discovery, not combat rules.
