# Slice 12.1 verification and handoff

Status: **Ready for review**, 2026-10-28. Implementation was explicitly requested
following approval of the detailed plan. This is not user acceptance. Slice 12
remains the latest accepted slice; do not begin 12.2 until 12.1 is accepted.

## Delivered

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

## Verification

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
and automatic standing, slow travel, the new icons and status labels, False Life's
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
- Capacities remain 32 catalog spells, 16 class spells and 22 picker entries;
  64 ordinary effects with three recipients; 260 × four condition records;
  16 ground patches with world/serial identity; nine cells, 36 NPCs and 40 combined
  recipient slots. Cosmetic pulse exhaustion cannot discard gameplay effects.
- Prepared floor projection occupies 3,384,000 bytes plus row-depth data, allocated
  during scene preparation. The eight strafe banks and 2,048-square icon atlas
  require no per-frame image generation or resizing.

Only the user can accept this slice. Update the checkpoint and acceptance record
when that happens; do not infer acceptance from this handoff or passing tests.
