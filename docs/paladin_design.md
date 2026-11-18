# Paladin through level four

Status: **implemented and mechanically verified; awaiting user review** (2026-11-16).
The class is playable through level four. Existing level-four acceptance is unchanged.

## Responsibilities and data

The immutable rules catalog owns the class, creation gear, explicit spell roster,
casting ability and progression profile. Pure rules functions derive stats and
validate grants. Party owns healing points and Channel Divinity; combat owns
accepted attack results. Effects owns timed Sacred Weapon and Protection state.
UI and automation submit the same checked commands, never mutate resources.

Use a shared uint64 SpellMask and capacity 64 throughout spell selection, grants,
serialization and tooling. Preserve current spell indices and stable string IDs.
Separate casting ability from progression and spell eligibility. Paladin is a
Charisma half-caster; cleric and wizard behavior remains unchanged.

## Level gains

1. d10 Hit Die; all armor, shields, simple/martial weapons; Wisdom/Charisma saves;
   Lay on Hands pool 5 x level.
2. Automatically grant Defense, spellcasting and Divine Smite. Base mana by level
   is 0/4/6/6. Level-one spells and successful Smites cost two mana.
3. Automatically grant Devotion, Sacred Weapon and always-prepared Sanctuary and
   Protection from Evil and Good, outside the elective preparation allowance.
4. Reuse approved ASI rules without class cantrip grants.

Elective spells: Bless, Cure Wounds, Shield of Faith. Preparation from level two:
max(1, CHA modifier + floor(level/2)), bounded by available spells. No spellbook or
class cantrips; High Elf racial cantrip retains Intelligence casting.

## Actions and effects

Lay on Hands selects one living party ally, including self/dying, and explicit
healing points bounded by pool and missing HP. A pool-scaled slider in the lower
casting dock defaults to a full useful heal, then requires confirmation. See
[healing power slider](#healing-power-slider--2026-11-16). Costs an action; long
rest restores pool, short rest does not.
Level advancement adds five remaining points, preserving expenditure.

Smite is an explicit melee attack with ordinary melee validation. Require level
two and two mana before accepting it. Roll at acceptance as existing attacks do;
a hit spends two mana, a miss none. Accepted cancellation does not refund mana.
Add 2d8 radiant, or 3d8 against undead/fiends; critical doubles all attack dice.
Use at most two typed damage components, mitigate separately, apply one combined
health transition and damage-triggered effects once. Applied radiant prevents
Undead Fortitude. Do not approximate a mixed hit using one damage type.

Sacred Weapon costs one action and Channel Divinity at level three. Main-hand
melee item gains max(1, CHA modifier) attack bonus for 60 gameplay seconds, without
concentration or extra damage. Bind to generation-checked item identity. End on
expiry, incapacitation, main-hand removal/replacement or free dismissal. Recast
replaces. Charge restores on short/long rest. Preserve valid binding across save
and travel, remapping saved item identity and rebasing deadlines. Visual glow
only; no new visibility/physical-damage mitigation system.

Protection costs two mana and an action; one living ally, concentration for 600
gameplay seconds. Attacks by aberrations, celestials, elementals, fey, fiends and
undead have disadvantage, using existing cancellation rules. Add explicit monster
creature types independent of combat traits, preserving supported legacy reads.
Charm/fear/possession protections remain deferred and disclosed.

## Integration and compatibility

Shared creation, advancement, arena replay, rest, character sheet, HUD and
headless descriptors must support the class. Reuse chain mail/shield, longsword
or warhammer, holy symbol and supplies, existing portraits and default party.
Generate four matching static icons through imagegen and prepare normally.
All runtime storage is fixed/bounded; no frame allocations. Reject invalid/full
commands before RNG/resource mutation.

Campaign schema/simulation becomes 24/26 (strict incompatibility, no migration).
Library schema 1 and arena schema 3 retain their shapes and old supported reads.
Widen masks end-to-end including campaign field visitors and automation outputs.
Persist healing pool, charge and active effects with authoritative validation.

## Delivery and verification

Implement foundations, class rules/progression/resources, combat/effects, UI/art,
then persistence/automation integration. Verify existing classes after foundation
changes. Test all races and levels, ASI and racial casting, invalid grants,
resource recovery/deficits, healing boundaries, Smite miss/hit/critical/cancellation,
mitigation and Undead Fortitude, effect expiry/binding/concentration, save/travel,
arena replay, headless parity and allocation guards. Test spell indices 31/32/63
and invalid 64. Use owned small fixtures and focused Debug checks with configured
CMake/Ninja, <=2 workers, one test process. Inspect hidden application-owned UI
captures at native/reduced presentation, keep <=6 selected review images and clean
owned disposable run output. Record commands/results and limitations here.

Deferred: Divine Sense, disease/poison curing, Divine Health mechanics, Turn the
Unholy, extra fighting styles/oaths/spells, skills, feats, multiclassing, level 5.


## Implemented interfaces and ownership

`ClassDefinition` now separates `Casting`, `SpellList`, and `SpellProgression`.
Authored catalog schema 17 supplies explicit lists/profiles and creature types;
supported older catalogs retain their legacy defaults. Existing spell IDs retain
positions; Protection occupies index 32. Half-caster mana and spell-level queries
are class-aware. Shared spell access supplies the actual ability modifier. Oath
access has its own `SpellSource::Oath` label while using existing always-prepared
storage. Defense and Devotion are derived from paladin level, not new mutable
character choices.

`PartyCharacter::layOnHands` owns remaining healing points (0..20), with the
maximum derived from level. `Party::useLayOnHands` validates both participants and
the amount before healing/spending. `PartyCommand::healingAmount` and the wire
field `healing_amount` carry the explicit amount. The shared ability picker offers
Lay on Hands, Sacred Weapon, free dismissal and Divine Smite. `PartyAction::Smite`
also supports direct headless submission. Descriptors expose limits and legal
healing amounts per ally; observations expose the remaining pool.

`DamagePacket` holds at most two contiguous typed components. NPC mitigation
returns both component results and a saturating combined result, then performs one
Fortitude decision/HP commit. Accepted Smite rolls are retained in the existing
pending timeline and redacted from acceptance/release notices. Resolved feedback
and headless events expose separate weapon/radiant results. Paladins cannot spend
their Channel Divinity through the cleric's Preserve Life action.

Sacred Weapon uses the existing 64-slot effect owner. Its binding stores inventory
slot plus generation, not a long-lived pointer or serialized runtime owner token;
Party restoration recreates runtime inventory identity while preserving those
stable values. Checked save restoration rejects stale bindings. Inventory changes
reconcile the effect immediately; save/travel retains and rebases valid effects.
Both new effects participate in existing status/expiry/concentration presentation.
Sacred Weapon's generated glowing sword appears as a portrait badge; it does not
change world illumination or add nonmagical weapon resistance rules.

The four new icons use one startup-prepared 1024x256 RGBA atlas (1 MiB GPU texture).
Loading/resizing is sequential during preparation. UI fonts and native pixel
coordinates retain the existing renderer contract. The shipping UI selector uses
the same `paladinArtNames` list, so raw staging/distributions include all four PNGs.
Debug intentionally uses source artwork plus prepared atlases.

### Artwork and final prompts

Generated through the built-in imagegen tool, visually inspected and saved as
project assets. Each prompt below is the exact final generation prompt; the PNG
has the same basename alongside it:

- [Lay on Hands prompt](../assets/ui/spells/lay_on_hands.prompt.json)
- [Divine Smite prompt](../assets/ui/spells/divine_smite.prompt.json)
- [Sacred Weapon prompt](../assets/ui/spells/sacred_weapon.prompt.json)
- [Protection prompt](../assets/ui/spells/protection_from_evil_and_good.prompt.json)

## Verification — 2026-11-16

Configured Debug CMake/Ninja were retained. Builds used at most two workers and
low CPU/I/O priority. `blobber`, `blobber_headless`, and the focused test/capture
executables build successfully. No Release, sanitizer, benchmark or full slow
campaign-save matrix was run.

The meaningful integration checkpoint passed **22/22** selected suites:

```sh
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug -j 1 --output-on-failure \
  -R '^(rules|rules_content|party_content|creation|library|recovery|inventory|commands|effects|impact|spells|level_spells|damage|zombie|progression|progression_allocations|arena|automation|runtime_inventory|dist_staging|campaign_save_smoke|campaign_save_cli)$'
```

Affected spell/damage callers were relinked after the integration edits; their
seven-suite rules/creation/library/spells/level-spells/damage/zombie check passed.
Library coverage includes saving/reloading a High Elf paladin without changing
library schema 1. The dedicated allocation workload additionally covers paladin
advancement, selected healing, ability browsing, Sacred Weapon, Protection, and a
Smite timeline: zero engine-owned C++ allocations. This is not a claim about
third-party graphics/driver allocation behavior.

```sh
nice -n 15 ionice -c 3 cmake-build-debug/blobber_progression_allocations_tests
nice -n 15 ionice -c 3 cmake-build-debug/blobber_campaign_save_tests --paladin
nice -n 15 ionice -c 3 cmake-build-debug/blobber_campaign_save_tests --progression
```

Both standalone persistence checks pass. Paladin coverage includes level-four
resources, high-bit oath grants, spent healing/mana/Channel Divinity, active effects,
inn travel and return, exact logical round trips, and rejection of corrupt healing
pools and stale weapon generations. The prior progression path still passes its
floor/recall/revisit checks and rejects the previous campaign schema/simulation.

The new rules tests cover all races/levels, racial INT casting, CHA preparation,
Defense timing, required level-two preparation, ASIs, dying/dead/self healing,
rest distinctions, normal/undead/fiend Smite hits/misses/criticals, separate damage
mitigation and applied-radiant Fortitude bypass, Protection filtering/advantage
cancellation/expiry, weapon removal and concentration coexistence. Headless tests
cover bounded amounts, preview/commit/duplicate rejection, descriptors and typed
feedback. Arena preparation/round trips/rematches include paladins through four.
The existing Poison Spray damage fixture was corrected to put its single target
inside the actual view cone; production visibility rules were unchanged.

Hidden production UI capture passed using source assets and the configured
prepared atlases. The capture opens an application-owned hidden window, routes
in-process commands, verifies production creation and level-four commit, and
exports six selected images. No OS input, window focus or desktop automation was
used. The sandbox initially could not connect to display :0; the authorized hidden
runner succeeded with GPU resources released on exit.

```sh
env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2 nice -n 15 ionice -c 3 \
  cmake-build-debug/blobber_world_integration_gpu_tests assets RUN_DIRECTORY/review \
  --paladin-review cmake-build-debug/assets/prepared
```

[Review gallery](screenshots/paladin/README.md): creation, ASI, character sheet,
abilities, and selected-amount healing at 1920x1080 and 1280x720. All selected images
were visually inspected. The menu backdrop is deliberate; this checks production
UI composition rather than dungeon world rendering. Interactive user play/review
remains unverified and acceptance is pending. Deferred SRD features listed above
remain outside this delivery.

Disposable build logs, intermediary captures and edit scripts are task-owned and
removed after promoting the six review images (57 files, 30,324,994 bytes; cleanup
previewed and applied). Source artwork/prompts, test inputs,
build caches, existing level-four evidence and user saves are retained.


## Implementation review — 2026-11-16

Review completed with **three confirmed issues corrected**:

- **P1 — reward-profile undefined shift:** `generation::rewardProfile` retained
  `1u << i` while iterating the now-33-entry spell catalog. This evaluates an
  invalid 32-bit shift at index 32, even when racial grants are empty. Use the
  shared bounded 64-bit bit helper. A self-owned reordered catalog places Fire
  Bolt at index 32; its racial attack-reward regression failed before the fix.
- **P2 — saved Protection recipient shape:** checkpoint validation accepted extra
  Protection recipients that live casting rejects. Enforce one party recipient.
  A campaign corruption regression adding a second ally failed before the fix;
  legitimate paladin saves/travel and invalid-shape rejection now pass.
- **P3 — missing Defense ability entry:** the bonus worked and appeared on the
  character sheet, but the abilities picker gated automatic Defense on being a
  noncaster. Derive the picker entry from paladin level two as well as the existing
  fighter choice. A regression now requires this automatic entry.

The review also inspected Smite validation, critical/typed damage and timeline
handling, healing/rest resources, casting/ASI grants, and effect/item bindings.
No additional confirmed defect was found in those paths. Deferred SRD features
remain deliberate scope decisions. This was a focused code/integration review,
not a claim of exhaustive correctness or new interactive user acceptance.

Existing interfaces/storage were retained; these fixes require no new artwork,
save shape, version bump or progression changes. The configured Debug build of
`blobber` and the test/capture executables passes without compiler warnings.

```sh
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug -j 1 --output-on-failure \
  -R '^(progression|progression_allocations|automation)$'
nice -n 15 ionice -c 3 cmake-build-debug/blobber_campaign_save_tests --paladin
```

All three selected suites pass (6.28 seconds combined), including the new reward
and picker regressions and the engine-owned allocation guard. The dedicated
paladin campaign save/travel/corruption check passes. The previous 22-suite
checkpoint remains historical evidence; the complete matrix, Release and full
sanitizer configuration were not rerun for this review.

The existing hidden `--paladin-review` capture also passes after the fixes, with
GPU cleanup on exit and no OS input/focus automation. Visually inspected and
replaced only the abilities image in the six-image gallery; its new Defense row
fits without clipping. The other five review images remain unchanged. User
acceptance and interactive desktop play remain pending. Disposable review logs
and duplicate captures are cleaned after promotion.

## Healing power slider — 2026-11-16

Design approved by the user before implementation. Replace the five amount
buttons with one bounded integer slider. Its full width represents the remaining
Lay on Hands pool; selecting an injured ally defaults to min(pool, missing HP).
Keep the slider inside the lower casting dock, beneath the healing summary and
beside Use; leave the 3D viewport unobstructed. Shade the portion beyond a full
heal and clamp input so it cannot waste points.
Dragging down preserves points; show selected healing and the pool remaining
after use. No valid recipient means a disabled bar and “Choose an injured ally”.
Use still explicitly confirms; selection/dragging never spend resources.

CommandController owns one transient drag flag and validates every amount update.
Physical press/hold/release is consumed before ordinary HUD button routing;
release outside clamps to the track, focus loss/stale input cancels capture, and
no slider event may fall through into world targeting. Up/Down adjusts one point
(Shift: five); Left/Right retains existing ally navigation. A new recipient resets
to its useful maximum; refreshing the same selection preserves a dialed amount.
Use fixed HUD values and existing native rectangle/text primitives (this is an
interactive control, not new icon artwork). Save/headless action shapes stay the
same. Verify defaults, clamping, dragging, cancellation, no-target behavior,
resource conservation, and native/reduced hidden UI captures.

Implemented in `MagicHud`, `CommandController::routeHealingSlider`, HUD layout,
button admission and the spell dock painter. The draft alone changes while
dragging; the existing combat validation and explicit confirmation still own
resource spending. No persistent fields or new assets are required. All slider
storage is fixed-size; amount updates and repeated dragging make no engine-owned
C++ allocations in the existing allocation guard.

Verification (configured Debug toolchain, two build workers, low CPU/I/O priority):

```sh
/home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-debug --target blobber_progression_tests blobber_progression_allocations_tests blobber_buttons_tests blobber_world_integration_gpu_tests blobber -j 2
/home/bison/clion/bin/cmake/linux/x64/bin/ctest --test-dir cmake-build-debug -R '^(progression|progression_allocations|buttons)$' -j 1 --output-on-failure
cmake-build-debug/blobber_world_integration_gpu_tests assets RUN/slider-review --paladin-review cmake-build-debug/assets/prepared
```

Build passed without compiler warnings; all three focused CPU checks passed
(0.13 seconds total). Coverage includes missing-HP and pool-limited defaults,
same-recipient retention, disabled input, press/drag/release clamping, focus-loss
cancellation, held-pointer suppression, keyboard increments and exact confirmed
spending. The allocation check includes a bounded repeated-drag workload. Hidden
OpenGL capture passed with successful resource/window cleanup. Visually inspected
the native and 1280x720 healing captures: the slider, summary and buttons fit in
the lower dock and leave the viewport clear. Replaced only the two healing images
in the six-image [review gallery](screenshots/paladin/README.md). Interactive
desktop play and user acceptance remain pending. Disposable run output is cleaned
after promotion; no diagnostic exceptions are retained.

### Slider drag and healing-pool clarity follow-up — 2026-11-16

User play exposed a missed integration path: gameplay calls `routeHudButtons`
before `routeHudInput`, converting physical presses to logical activations before
the slider sees them. Earlier tests bypassed that prepass, so they did not establish
that real dragging worked. Move continuous slider admission into `routeHudButtons`
before ordinary button conversion. Consume handled frames as neutral logical input
so the later routing pass cannot clear or replay a captured drag. Preserve direct
logical clicks and keyboard adjustment for in-process clients.

Share handle geometry between painting and hit testing; widen it to 12 logical
pixels (36 native pixels), with a 14-pixel height and a bounded dock hit area.
Keep a transient horizontal grab offset so pressing either edge of the handle
does not jump the selected amount. Track clicks still jump, then drag. Capture
continues outside the dock and ends on release; focus loss, stale input, Back or
Cancel end it without spending. Storage stays fixed-size and allocation-free.

Show `Healing pool current / maximum` explicitly in the lower dock, with selected
healing, points spent and points kept. Maximum is five times paladin level; the
slider still scales to the remaining pool and clamps to the recipient's missing
HP. Label the existing Lay on Hands character-sheet feature as healing points
and explain one point per HP and long-rest recovery. Per the user's clarification,
do not put healing points in the portrait mana row. Level-one paladins have no
spell mana; the existing mana row/sheet presentation starts at level two. No
resource rules, save fields, or artwork change.

Verify the actual physical-prepass-plus-routing sequence, handle-edge grabbing,
held movement, release outside, cancellation, non-spending, pool maximum and sheet
values at levels one and four. Reuse the focused CPU/allocation checks and hidden
native/reduced captures, replacing superseded gallery evidence.

Implemented and mechanically verified. The new physical-prepass regression failed
before the fix with `Physical gameplay prepass retains slider capture` and now
passes. Tests also sample `UiInputEdges`, refresh between held frames, grab both
handle edges without jumping, and drop over Use without casting. Stale admission
and focus loss cancel capture; held input cannot resume it. Resource checks cover
levels one through four with partially spent pools, separately checking mana,
healing capacity, remaining points and target-specific useful limits. The sheet
now explicitly says `5 / 5 healing points` (or its live current/maximum values).

Configured Debug verification, at low CPU/I/O priority and at most two workers:

```sh
/home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-debug --target blobber_progression_tests blobber_progression_allocations_tests blobber_buttons_tests blobber_inventory_tests blobber_world_integration_gpu_tests -j 2
/home/bison/clion/bin/cmake/linux/x64/bin/ctest --test-dir cmake-build-debug -R '^(progression|progression_allocations|buttons|inventory)$' -j 1 --output-on-failure
env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2 nice -n 15 ionice -c 3 cmake-build-debug/blobber_world_integration_gpu_tests assets RUN/review --paladin-review cmake-build-debug/assets/prepared
/home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-debug --target blobber -j 2
```

The game and selected test/capture targets build successfully. All four selected
CPU suites pass (0.16 seconds total). The allocation workload
now includes gameplay's prepass and refresh during repeated dragging: zero
engine-owned C++ allocations. Hidden OpenGL verification passes, including a
production level-one press/hold/release from five points to three, with no pool
spending and successful resource/window cleanup. Native and reduced dock captures
and both level-one/four sheets were visually inspected; labels and handles fit
without clipping. Replaced the level-four sheet and two healing captures and
retained two distinct level-one cases in the [review gallery](screenshots/paladin/README.md)
(eight images while acceptance is pending). No desktop input or focus automation
was used. Interactive review of the corrected build remains pending. Disposable
run output is cleaned after closing the build and promoting evidence; no
diagnostic exceptions are retained.

### Continuous slider feedback — 2026-11-16

User play confirmed that quantizing handle geometry to whole healing points gives
poor feedback when only two points remain. Keep a transient fractional healing
amount during a physical drag, independently of the integer command amount.
Clamp the continuous amount to 1..useful maximum and round only the draft's
spend/healing values. Paint the fill and handle at the continuous position, aligned
to native pixels; on release or cancellation return to the integer detent. Keep
grab offsets, overheal limits, explicit Use and the existing raw-input ownership.
Keyboard adjustment ends pointer capture and immediately displays its integer
value. No saved or submitted field changes and no frame allocations.

Draw the track, fill, cap marker and track border first, then a fully opaque handle
with a solid face and grip. The old track border was drawn after the handle,
causing the visible horizontal line through it despite its opaque fill.

Verify sub-step movement and rounding around the midpoint with a two-point pool,
release snapping, refresh stability and non-spending through the actual gameplay
input prepass. Reuse focused progression/button/allocation checks and hidden
native/reduced captures; include a held drag between detents and replace
superseded healing evidence.

Implemented with one additional float in `MagicHud` and a shared float position
mapping; no per-frame allocation or persistent state was added. The painter rounds
only final handle placement to native pixels and draws its opaque face after the
track border. Tests exercise the actual gameplay prepass and refresh on each held
sample with a two-point pool: movement from 1.1 to 1.2 changes the visible position
without changing the one-point draft; crossing 1.49 to 1.51 changes the draft to
two without a handle jump. Release snaps to the selected detent. Keyboard input
ends a fractional drag; no points or HP change before Use.

Configured Debug verification, low priority, at most two workers:

```sh
/home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-debug --target blobber_progression_tests blobber_progression_allocations_tests blobber_buttons_tests blobber_world_integration_gpu_tests -j 2
/home/bison/clion/bin/cmake/linux/x64/bin/ctest --test-dir cmake-build-debug -R '^(progression|progression_allocations|buttons)$' -j 1 --output-on-failure
env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2 nice -n 15 ionice -c 3 cmake-build-debug/blobber_world_integration_gpu_tests assets RUN/review --paladin-review cmake-build-debug/assets/prepared
/home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-debug --target blobber -j 2
```

The selected targets and final game executable build successfully without compiler
warnings. The initial game build ended with status 143 during asset preparation;
the configured-toolchain retry completed successfully. All three focused CPU
suites pass (0.14 seconds total), including the existing
engine-owned allocation guard with repeated dragging. Hidden OpenGL capture passes
with a two-point production pool held at a fractional position and then released
to one point. GPU resources/window are released successfully. Resting and held
handles were visually inspected at 1920x1080 and 1280x720: no track line crosses
the handle, and the held position differs from the integer preview. Replaced only
the three healing images in the eight-image pending [review gallery](screenshots/paladin/README.md).
Interactive review remains pending; no OS input or focus automation was used.
Disposable build logs and intermediary captures are cleaned after closing runners
and promoting evidence; no diagnostic exceptions are retained.
