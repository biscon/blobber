# SE04 — Integration audit and safe cleanup

Status: **Implemented; Ready for review (2026-11-06)**.
The user approved this reduced scope after accepting SE03: keep the current visual
treatment and check integration, lifecycle and safe cleanup only. No screenshots,
art regeneration, retuning, benchmark or full configuration matrix is needed.

## Design and boundaries

Retain existing ownership: `GameSession` owns ephemeral spell/support journals;
`EffectController` owns authoritative sustained conditions and ground/weapon state.
`GameView` supplies all three to the world renderer in ordinary gameplay, while
`CommandController` supplies portrait feedback. Restore clears journals and rebuilds
condition snapshots; campaign travel imports portable effects into the destination
session. The renderer's bounded players and prepared assets remain unchanged.

The audit found the old generic spell-glint branch in `world_effects.cpp` is bypassed
by ordinary `GameView` snapshots. Remove that alternate spell presentation, keeping
the shared glint texture and ordinary arrow/weapon behavior. The projectile loop
will handle only non-spell attacks; prepared spell/support players own spell art.
Remove obsolete tint and spell-only Scorching Ray spacing code. Existing callers that omit
spell journals must supply them to render spells; tests should exercise this real
contract rather than maintain an obsolete fallback. Do not change resource layouts,
capacities, allocation behavior, timing, rules, save schemas or publication versions.

## Verification strategy

Audit the 32-kind catalogue against SE01–SE03 command/playback tests and document
intentional reuse. Replace legacy fallback tests with checks that attack records
cannot draw duplicate generic spells, while arrow impacts and persistent weapons
remain supported. Keep prepared multi-target delivery coverage in `spell_effects`.
Add a small campaign lifecycle check covering fresh journals, paused application
feedback, Continue without replay and portable conditions across location travel.
Reuse existing shader/occlusion/GPU/resource checks from SE03 because art, packets,
shaders, resource acquisition and accepted presentation are not redesigned.

Run the focused `spell_effects`, `world_integration` and `campaign_save_smoke` suites,
one low-priority process at a time, and build the normal application with at most
two workers. No desktop automation or new capture gallery. Record actual outcomes
and any limitations below, then mark Ready for review.

## Catalogue audit

`rules::SpellKind` contains 32 spells. All have intentional treatment; no missing
family or new visual work was found. The rows below account for every kind once.
The existing SE01, SE02 and SE03 command tests execute all 32 using owned fixtures;
the world integration suite also verifies legal party builds for the catalogue.

| Spells | Existing delivery and lifecycle treatment |
| --- | --- |
| Fire Bolt, Ray of Frost, Burning Hands | SE01 fire/frost flight or cone, contact and remnants; frost reflects actual Slow state |
| Magic Missile, Scorching Ray | SE02 independent dart/ray deliveries and contacts; ray fire aftermath |
| Guiding Bolt | Radiant flight/contact and authoritative guiding coating |
| Sacred Flame | Descending radiant contact with saved/negated feedback |
| Shocking Grasp, Inflict Wounds | Electric contact or contracting necrotic contact, tied to resolved melee spell timing |
| Acid Splash, Poison Spray | Acid contact/flecks or poison cloud with save distinctions |
| Thunderwave, Shatter | One cast-centred release clipped to the actual area footprint |
| Cure Wounds | Accepted green healing portrait pulse and actual healing readout |
| Spare the Dying, Lesser Restoration | Distinct stabilization/cleansing portrait rows; no invented world body |
| Aid, False Life | Protective portrait application, actual HP/temporary-HP state and status; deliberate shared ward artwork |
| Bless, Bane | Boon/curse applications; quiet NPC sustained accents including shaman Bless, and party portrait/status feedback |
| Guidance, Resistance | Quiet party boon/ward application with status and consumption/end feedback; intentional shared rows |
| Shield of Faith, Mage Armor, Sanctuary | Distinct ward portrait treatments, actual effect lifecycle and status |
| Mirror Image | Ward portrait application and actual duplicate-bit state/loss; no invented party world silhouettes |
| Sleep, Color Spray, Hold Person | Footprint mist/prismatic fan or restraint; authoritative prone/held/blind state, recipient and end feedback |
| Grease, Web | Prepared ground art and placement accents; actual Web restraint/burning cells and expiry |
| Spiritual Weapon | Prepared weapon, summon/move/strike/end accents, idle bob and attack/return tied to real commands |

Shared support `Boon`/portrait rows are deliberate, not missing spell art. Offensive
style selection is guarded by the explicit thirteen-kind `elementalSpell` set;
the remaining offensive default is Shatter. Condition effects reconstruct from
authoritative masks and duplicate bits rather than replaying application events.
No catalogue, asset, style parameters or gameplay values were changed by this audit.

## Integration findings and cleanup

- `platform/game_view.hpp` supplies attack, spell, support, condition and ground
  snapshots unconditionally during normal play; review switches are not required.
  `assets/world_residency.cpp` acquires the prepared effect clips for normal views.
- Removed the redundant generic spell branch and its tint/Scorching Ray spacing
  helpers. `effect/glint` remains loaded for ordinary arrow impacts. Ground,
  Spiritual Weapon, hearth and non-spell projectile paths are retained. No source
  artwork or shared asset was deleted.
- Replaced obsolete generic spell tests with exhaustive suppression checks for all
  32 incoming/outgoing spell kinds, with and without journals, at flight/contact.
  The same fixture checks ordinary arrow flight/impact/fade and persistent weapons.
  Prepared multi-target behavior remains covered by `spell_effects`.
- Removed the old front-row formation workaround from the SE02 Shocking Grasp
  fixture, so its existing command/presentation check exercises the accepted rear-row
  behavior directly.
- Added one campaign lifecycle fixture: fresh journals; real ward application and
  portrait flash; paused clock/feedback; canonical Continue round trip without old
  bursts; normal expiry after resuming; city-to-inn-and-back doorway transactions
  retaining portable ward state with the destination world identity. Prior SE03
  checks already cover ground/weapon restore, conditions, duplicate loss, journal
  epochs, transient expiry, cache recovery and allocation bounds.

The audit found no additional visual or lifecycle bug requiring retuning. This is
a cleanup and regression-coverage slice, not a change in the accepted appearance.


## Verification and handoff — 2026-11-06

Built the normal Debug application plus `blobber_spell_effects_tests`,
`blobber_world_integration_tests` and `blobber_campaign_save_tests` with
`nice -n 15 ionice -c 3 cmake --build cmake-build-debug --target ... -j2`.
No compiler warnings. Focused integration command:

```sh
nice -n 15 ionice -c 3 ctest --test-dir cmake-build-debug --output-on-failure -j1 \
  -R '^(spell_effects|world_integration|campaign_save_smoke)$'
```

`campaign_save_smoke` passed in 22.22s, including the new pause/Continue/travel
fixture; `spell_effects` passed in 0.18s. The first world-integration run exposed an
old static weapon-height assertion that excluded the already accepted bob. Updated
its bounds to the existing 0.5 centre ±0.325 half-height ±0.025 bob; no runtime
parameter changed. Rebuilt only that test, then `-R '^world_integration$'` passed
in 0.15s. All three selected suites pass. The optional stress/benchmark branch was
not run. `git diff --check` and changed-C++ formatting checks pass.

The three suites cover prepared spell playback and allocation invariants,
non-spell arrows/impacts, ground/weapon state, hearth, door occlusion, save/Continue
and the added campaign lifecycle checks. Accepted art, shaders and asset payloads
are unchanged. Reuse [SE03 GPU checks and resource measurements](se03_design.md#measured-storage-and-resource-limits):
309,772 bytes of combined effect/light scratch per view, with the prior
active/staged payload table unchanged by this code-only cleanup. These are prior
measurements, not a fresh benchmark or GPU run.

No screenshots, new gallery, interactive desktop review, GPU rerun, Release or
sanitizer matrix was performed. No additional visual/lifecycle bug was found in
this bounded audit; it does not establish cross-device or production-scale
performance. The owned run `20261105T233935Z-2f22764bd0a54cc799a7fc6440b2ac4b`
was cleaned after recording results; no diagnostics retained. Existing accepted
SE01–SE03 evidence and working-tree changes were preserved.

SE04 and milestone E-C are **Ready for review**. User acceptance is still required
to close the roadmap; no further spell-effects slice is proposed.
