# Slice 12.1 verification and handoff

Status: **Ready for review**, 2026-10-29, including the approved
[visual review corrections](spell_visual_review_design.md). Implementation and
verification are complete. This is not user acceptance. Slice 12 remains the
latest accepted slice; do not begin 12.2 until 12.1 is accepted.

## Visual review corrections delivered

- The picker has populated Cantrips, Roman-numeral level and Abilities tabs,
  explicit spell levels in the detail pane, keyboard/click navigation and
  remembered selection/scroll per character and tab. Cast identities and costs
  are unchanged; currently only Cantrips and level I contain spells.
- Enemy effects use miniature spell icons. Portraits hold six icons with teal
  beneficial/red harmful borders; more than six become five icons plus +N.
  Hover shows the spell name and actual remaining duration, or the governing
  turn/consumption rule. Overflow lists every remaining effect. Hover also works
  while the spellbook pauses gameplay. Harmful party effects use this same path.
- Grease is a generated transparent oily pool, projected and lit offline into
  prepared floor banks. It blends beneath actors/loot, respects walls, survives
  strafing and remains separate from target selection. Overlap does not multiply
  opacity.
- Living sleeping or knocked-down melee goblins and archers use a static prone
  pose in all four directions. Death takes priority; waking/standing restores
  normal poses. Picking uses the real prepared silhouette. Gameplay baking
  requires this one-frame, four-direction clip for every future appearance.
- Imagegen produced the Grease master, eight prone masters and a Hold Person
  status icon. PNGs and exact `.prompt.json` requests are under `assets/effects/`,
  `assets/npcs/{goblin,goblin-archer}/animation/sheets/` and `assets/ui/spells/`.
  Registered derivatives remain build outputs. Source masters were inspected;
  transparent alpha, registration and final rendered views were checked.

## Current verification, 2026-10-29

Final Debug **31/31** (60.89s) and Release **31/31** (6.03s) pass. The allocation
suite reports **zero tracked C++ frame allocations**, now including tab switching,
status summaries/hover, prepared Grease and prone pose changes. Six focused
ASan/UBSan suites pass: visual review, spells, projectile, NPC, image preparation
and prepared assets (159.30s). Leak detection remains disabled for the existing
dependency environment. These tests do not claim graphics drivers or third-party
internals never allocate.

Generated fixtures exercise filtering/identity, tab memory, disappearing abilities,
status grouping and actual clocks, hostile effects and overflow, modal hover
without click-through, four-direction prone picking, independent Sleep/Knockdown,
wake/stand/death priority, single-column extraction, prepared round trips, Grease
alpha/anchoring/wall occlusion/overlap/strafe/expiry and source validation.

The production capacity bake passes the unchanged 512 MiB resident/loading
limits: scene 140,655,424 bytes; NPCs 69,010,094; total 209,665,518. Tracked loading
high-water is 150,419,024. Two complete appearance sets use 278,669,284 resident
bytes and 200,068,708 NPC raster loading bytes. Binary format/renderer revision 2
invalidate old bundles; a normal CMake build rebakes them.

The hidden GPU smoke passes texture/font/atlas/presentation lifecycle checks.
Application-owned captures produce **470 PNGs** plus `spells.tsv`, including 30
new tab/status/four-direction pose views at 1920×1080, 1280×720 and letterboxed
1024×768. Representative Grease, Sleep, all prone directions, tabs, hostile status
and overflow captures were visually inspected. The +N count and overflow tooltip
were corrected and recaptured. No OS input or window switching was used;
interactive desktop gameplay remains unverified.

Capture directory: `/tmp/blobber121-visual-captures/`. Useful review files:
`spell-grease-650.png`, `spell-sleep-650.png`, `review-prone-{0,1,2,3}.png`,
`review-tabs-{cantrips,level-one,abilities}.png`,
`review-status-{benefit,hostile,overflow}.png`; review files also have `-1280` and
`-1024` variants. Review tabs with Left/Right, navigate entries with Up/Down, hover
portrait icons, and verify waking/standing returns a living actor to normal art.

```sh
cmake --build --preset debug -j6
ctest --preset debug -j6
cmake --build --preset release -j6
ctest --preset release -j6
cmake --build --preset debug --target blobber_gpu_smoke -j6
cmake-build-debug/blobber_gpu_smoke
cmake-build-release/blobber_bake_assets assets /tmp/blobber121-visual-capacity --capacity-check
cmake-build-release/blobber --capture-spells /tmp/blobber121-visual-captures
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/blobber121-visual-asan \
  -R '^(visual_review|spells|projectile|npc|image_preparation|prepared_assets)$' --output-on-failure -j2
```

Sanitizer configuration uses `-fsanitize=address,undefined -fno-omit-frame-pointer
-fno-pie` and linker flags `-fsanitize=address,undefined -no-pie`. Logs are
`/tmp/blobber121-visual-{tests,release-tests,asan-tests,capacity,gpu,captures}.log`.
`git diff --check` and all 210 local file links in changed documentation pass.
The original spell implementation/verification is preserved below as history;
this section is the current presentation and verification checkpoint.

## Original spell delivery, 2026-10-28

All fourteen earlier spells remain available, alongside Ray of Frost, Shocking
Grasp, Poison Spray, Sleep, False Life, Grease, Color Spray, Guidance, Resistance
and Sanctuary. The [expanded-spells design](expanded_spells_design.md) records the
approved targeting, timing, condition and movement adaptations. The
[source ledger](srd_content_ledger.md) records SRD pages and project differences.

- Sleep/Grease use floor selection and separate confirmation, with keyboard
  left/right cycling. Sleep covers nine cells; Color Spray retains the three
  front cells. Eligible party members are explicitly marked **At risk**.
- Sleep, blindness, slow and knockdown use bounded per-creature storage. Sleep
  preserves life/occupancy, wakes on positive damage or automatic Aid, and cannot
  cause defeat by itself. High Elf magical-sleep immunity is active.
- Grease persists on the floor, participates in movement and save checks, and
  prevents rest beneath the party. Slowed player/NPC travel is visibly doubled;
  slowed NPC turn movement uses preparation followed by a revalidated step.
  Knockdown consumes a turn standing and preserves concentration.
- False Life maintains a separate temporary-HP pool and lifetime. Damage consumes
  it first; concentration uses incoming damage before absorption. The HUD displays
  the buffer separately from actual HP.
- Guidance and Resistance consume on their next eligible roll. Sanctuary tries
  each legal formation target at most once, costs one enemy action, and breaks
  on accepted offense. Released attacks retain their accepted outcomes.
- Schema 7 supplies explicit sleep-immunity and metal-armor metadata. Older
  versions default those capabilities off. Class-wide spell access remains in
  place for this slice.
- Ten painted spell icons were generated with the built-in imagegen tool. PNG
  masters and exact prompt/provenance JSON are under `assets/ui/spells/`.
- The unchanged official SRD 5.1 PDF, searchable extraction, attribution, source
  URL and checksum are now in [docs/references](references/README.md). Read these
  before downloading the rules again.

## Original verification, 2026-10-28

All tests use test-owned rules/maps/art rather than editable game assets. Full
Debug and Release suites passed **30/30**; focused ASan/UBSan passed **4/4**
(expanded spells, original spells, effects, schema/party content). Leak detection
is disabled for the existing dependency environment. These are correctness runs,
not production performance benchmarks.

Focused coverage includes all ten accepted casts and costs, a missed Ray, empty
and immune-only rejection, low and full 36-NPC HP budgets with stable ties,
friendly sleep and automatic Aid, zero/absorbed-damage waking, life/occupancy
preservation, blind acquisition/remembered positions, turn-bound expiry,
concentration through knockdown and temporary HP, one-shot consumption/automatic
save failure, natural death-save 1/20, Sanctuary's finite formation retries,
overlapping Grease, repeated knockdown, area exhaustion/expiry, two-action slow
movement, immutable committed duration, unsafe rest, ground UI confirmation and
schema 1–7 compatibility. Original spell/weapon/loot/recovery regressions pass.

The allocation guard reports **zero tracked C++ allocations**, including first
and repeated expanded casting, condition application/recovery, ground rendering,
strafe composition, target selection and UI work. The 8,192-event backing store
allocates once when its owner is constructed; it never grows during simulation.
This does not claim driver or third-party internals never allocate.

Separate maximum-capacity stress fills all 256 NPC condition records and all 16
areas, then drains a 60-second update: **2,752 / 8,192 events**, with all conditions
expiring. Prepared floor tests cover all nine strafe endpoints/phases, wall
occlusion, cached-frame reuse, expiry invalidation and shared ground-picking
legality.

The hidden GPU smoke test passed resource lifetime, native atlas and presentation
checks, including all ten new icons. Application-owned scripted captures completed
without desktop input: **440 PNGs** and `spells.tsv`, covering the 24 spells,
existing feature/area cases, and all new spells at 1920×1080, 1280×720 and
letterboxed 1024×768. Outputs: `/tmp/blobber121-captures-final/`.
Representative ground, condition, temporary-HP, targeting and picker captures were
visually inspected. Interactive desktop review remains for the user.

Commands and logs:

```sh
cmake --build --preset debug -j6
ctest --preset debug -j6
cmake --build --preset release -j6
ctest --preset release -j6
cmake-build-release/blobber_expanded_spells_tests --stress
cmake-build-release/blobber_frame_allocations_tests
cmake --build --preset debug --target blobber_gpu_smoke -j6
cmake-build-debug/blobber_gpu_smoke
cmake-build-release/blobber --capture-spells /tmp/blobber121-captures-final
```

Sanitizers use `/tmp/blobber121-asan`, configured with
`-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie`, linker flags
`-fsanitize=address,undefined -no-pie`, and the existing pinned raylib source at
`cmake-build-debug/_deps/raylib-src`. Run the four focused tests with
`ASAN_OPTIONS=detect_leaks=0`. Verification logs use `/tmp/blobber121-*`. Final full Debug: 30/30 in 58.71s;
full Release: 30/30 in 5.96s; focused sanitizers: 4/4 in 0.88s.
The final allocation extension also passes independently in both builds.
`git diff --check` and 218 local documentation file links passed; the cached PDF
checksum matches its recorded source checksum.

## Review and next-agent handoff

Review Sleep's floor targeting and ally-risk display, Grease's persistent floor
and automatic standing, slow travel, the spell icons and duration tooltips, False Life's
HP display, and cleric buff/ward feedback. Shocking Grasp requires an exposed
caster; the capture fixture explicitly places the wizard in the front row.
The runtime keeps the approved fixed presentation and discrete movement.
No new stand button, bonus actions, reactions, skill system, creator, spell-grant
selection, persistence or advancement was introduced.

Useful integration boundaries for 12.2, after acceptance:

- Stable spell IDs: `ray_of_frost`, `shocking_grasp`, `poison_spray`, `sleep`,
  `false_life`, `grease`, `color_spray`, `guidance`, `resistance`, `sanctuary`.
  `rules::spellRules` exposes class casting kind, level, descriptions and
  ground/self/area targeting. The catalog has 24 spells: 13 wizard and 11 cleric.
- `PartyCommand::aimedCell` is checked again by `CombatController::validate`.
  `CombatController::area` and `spellCell` are shared authoritative preview
  queries. `CommandController::magicGround` owns selection, never execution.
- `EffectController` owns condition and ground lifetimes, ability/save one-shots,
  sight/action checks and ward saves. Party owns temporary health. Neither
  creator nor saved blueprints should copy live condition/area/effect state.
- `EffectController::statuses` is the read-only status presentation boundary;
  summary capacity is 14, portrait slots six, tooltip storage 29×96 bytes.
  `MagicHud` retains stable complete entries plus filtered indices and 11 tabs.
  Future NPC appearances must have one Prone frame per direction; session-owned
  animation must receive the authoritative EffectController.
- Capacities remain 32 catalog spells, 16 class spells and 22 picker entries;
  64 ordinary effects with three recipients; 260 × four condition records;
  16 ground patches with world/serial identity; nine cells, 36 NPCs and 40 combined
  recipient slots. Cosmetic pulse exhaustion cannot discard gameplay effects.
- Prepared floor projection occupies 3,384,000 bytes plus row-depth data, allocated
  during scene preparation. The eight strafe banks now also contain four Grease RGBA frames per view.
  Prepared bank format/renderer revision 2 and the 2,048-square icon atlas
  require no per-frame image generation or resizing.

Only the user can accept this slice. Update the checkpoint and acceptance record
when that happens; do not infer acceptance from this handoff or passing tests.
