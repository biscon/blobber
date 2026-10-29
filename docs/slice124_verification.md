# Slice 12.4 verification and handoff

Status: **Accepted and closed**, 2026-10-29.
The user approved implementation and authorized autonomous completion while AFK.
The user subsequently said “Slice accepted, record it in the doc”. Slice 13 has not started.

## Delivered

New character is available from both the library and party picker. The wizard
provides name/cursor editing, race and five matching portraits, class features,
27-point buy with recommendations/reset/live derived stats, reserved Skills,
conditional cantrip/racial/spellbook/preparation stages, curated equipment and
Review with Change links. Ordinary martial builds skip spells; High Elf martial
builds receive the separate Intelligence-based racial choice. Wizard preparation
uses the selected spellbook; Life Domain grants are locked outside the allowance.

Backtracking preserves valid input. Race/class changes prune incompatible choices
and explain defaults; lower preparation allowances retain overfull picks for the
user to remove. Search, card inspection, checkboxes, scrolling and recommendations
share the same fixed selection model used by camp preparation. Skills remain
explicitly unavailable. No progression, new gameplay content or run-save was added.

Save uses the existing checked library transaction once. Failure retains the draft;
a committed durability warning leaves creation without retrying. A stale store is
reloaded and requires another explicit Save. Successful saves clear filters, reveal
and highlight the saved UUID, and restore the caller/active slot. Picker companions
and formation are preserved; saving never assigns or launches. Dirty Cancel/Escape
opens an exclusive Keep/Discard confirmation. Library View shares read-only review.

Long Rest optionally opens the same selection panel for living Clerics/Wizards.
Done stages all edited lists; Cancel discards that editing pass. Closing camp
abandons pending choices, and Short Rest isolates them. The user approved changed
preparation alone as a benefit at full health/mana: normal safety, eight hours and
one field camp kit still apply. Inventory/preparation revisions and world/rest-token
checks precede mutation. Supply consumption, recovery and prepared masks commit
once before cosmetic playback; skipping playback cannot replay or undo the rest.
Library blueprints, learned spells, cantrips and other grants are unchanged.

## Verification

- Full rebuilt Debug and Release CTest: **37/37 each**. Final CPU logs:
  `/tmp/blobber124-review-debug-tests.log` and
  `/tmp/blobber124-review-release-tests.log`.
- Focused ASan/UBSan with `ASAN_OPTIONS=detect_leaks=1`: **4/4** (`creator`,
  `creator_art`, `menu`, `recovery`). No sanitizer report. Fresh sanitizer build:
  `/tmp/blobber124-sanitize`; log `/tmp/blobber124-review-sanitize-tests.log`.
- The engine allocation guard passes with **zero tracked C++ allocations** while
  repeatedly editing/filtering/layouting drafts, opening preparation, querying
  eligibility and committing preparation. Existing render/movement/inventory/rest
  work remains covered. This does not assert that drivers or C allocators never allocate.
- New generated-fixture creator checks cover all sixteen race/class combinations,
  every matching portrait, conditional navigation/live spell access, input bounds,
  preservation/pruning, selection limits, minimum preparation, overfull allowances,
  recommendations, no-result search, review wheel routing and dirty cancellation.
  Temporary-library checks cover failure/stale/full/committed-warning/repeated Save,
  reload and picker restoration. Rest checks cover multi-caster commit, no-op,
  normal cost, cancellation/staging, Short Rest isolation, invalid/stale requests,
  dying/stable/dead recipients and supply exhaustion. Existing recovery tests also
  cover safety, movement, deadlines and recovery failures.
- Advancement-shaped selection fixtures lock the six baseline learned spells and
  accept exactly two new choices without modifying baseline. This establishes the
  panel reuse boundary; it does not implement advancement.
- Generated creator-art fixtures verify dimensions, tile identity, edge extrusion,
  deterministic re-bakes and invalid/missing-master rejection before replacement.
  The current-assets bake uses **16 MiB GPU residency** and measured **37,116 KiB
  peak process RSS**, within the 64 MiB workspace design. See
  `/tmp/blobber124-art-check.log`. Existing masters were reused; no art was generated.
- Hidden raylib/OpenGL GPU lifecycle and pixel checks pass on the RTX 3060, including
  the large detail atlas and exhaustion/reuse with all eight texture slots occupied.
  Fixtures generate their own atlas; routine tests do not read production art.
- `--capture-creator` uses deterministic UUID entropy, a unique temporary library
  removed on exit, and application-owned in-process actions. It creates/saves/reloads
  all sixteen combinations, exercises a real injected pre-commit write failure and
  retry, launches four created builds through the production menu-launch path,
  reconstructs that party, and commits a preparation-only rest before playback.
  **187 images** cover native 1920×1080, reduced 1280×720 and letterboxed
  1024×768 states, with native dungeon launch/pause/reconstruction captures.
  Selected captures were inspected for clipping, selection state, portrait identity,
  long-name bounds, errors and letterboxing. Launch/reconstruction images and an
  independent production-atlas re-bake match byte-for-byte.
  Capture output and its assertions are in `/tmp/blobber124-review`.

The final text-clipping adjustment was rebuilt in Debug/Release and rechecked with
GPU/captures after the full CPU suites. Earlier clean recompilation reports the
five existing `PartyCommand::aimedCell` initializer warnings in commands tests;
this slice introduces no new build warning. No mouse/keyboard injection, window
selection or other desktop automation was used. **Interactive desktop use remains
unverified** and is the user's review task. Cross-GPU visual equivalence was not tested.

## Review images

27 selected reduced/native/letterboxed captures are retained in
[screenshots/slice124](screenshots/slice124/index.html). The generated complete set
remains under `/tmp/blobber124-review`; rerun the recipe below to recreate it.

## Reproduction and user review

```sh
cmake --build cmake-build-debug -j4
ctest --test-dir cmake-build-debug --output-on-failure -j4
cmake --build cmake-build-release -j4
ctest --test-dir cmake-build-release --output-on-failure -j4
cmake --build cmake-build-debug --target blobber_gpu_smoke -j4
cmake-build-debug/blobber_gpu_smoke
cmake-build-debug/blobber --capture-creator /tmp/blobber124-review
```

For interactive review, run `cmake-build-debug/blobber`. Open Character Creator →
New character; edit the recommended choices and save. Repeat from Start new game
with existing companions selected: the new record should be highlighted without
assignment. Drag four records into formation and enter the dungeon. At a safe camp,
choose Long Rest → Prepare spells, edit Cleric/Wizard lists and use Done. Cancel
camp to abandon them, or Start Rest to commit at the displayed cost. Confirm that
ordinary recovery with current preparation still works. Review dirty cancellation,
name/search cursor editing and native/reduced-window readability.

## Next-agent contracts

Read [the implemented design](character_creator_design.md), this handoff and the
current roadmap checkpoint. `CreatorModel` owns the bounded level-one draft and
validation; `CreatorFlow` owns explicit persistence/return state. `SpellSelection`
accepts identity, baseline, available/locked masks and minimum/maximum budgets;
its selected mask is a draft result. All long-lived spell references are catalog
IDs/masks. Identity strings borrow the draft or Party for the panel's lifetime.
`FlowControls` shares fixed geometry between input and rendering; text edits use
fixed 65-byte buffers. Ordinary editing/layout performs no disk access or allocation.

`CampHud` owns working and staged preparation for four party members. Only the
caller commits. `GameSession::rest` retains authoritative safety/token/time checks;
Party validates both revisions and every changed mask before consuming supply or
using RNG. The live preparation revision increments only on a changed commit and
has explicit exhaustion behavior. Slice 13 must invalidate outstanding rest requests
when changing eligibility and must use its own atomic advancement commit, retaining
live identity, inventory and spellbook. Do not reuse creation's starting-gear grant.

Catalog schema 8, the existing versioned library schema and twenty portrait IDs
are unchanged. Live rest never writes blueprints. Slice 14 must persist actual run
state, including preparation, rather than rereading mutable starting blueprints.
The added offline `creator.qoi` atlas uses the eighth GPU texture slot; another
resident atlas requires a deliberate capacity/lifetime decision.

## Accepted closeout and next-agent handoff

The user explicitly accepted slice 12.4 on 2026-10-29: “Slice accepted, record it in
the doc”. This closes the delivered creator, checked library saving, shared
selection/review and Long Rest preparation. The user also chose to retain the
current ability rules for now: 27-point buy, base scores 8–15, and fixed racial
bonuses (for example, High Elf +2 Dexterity/+1 Intelligence). The discussion of
BG3 did not authorize assignable bonuses or a higher creation cap.

Verification results and their recorded limitations above remain unchanged;
acceptance is not a claim of additional automated or exhaustive interactive tests.
This closeout changes documentation only, so no implementation tests were rerun.

Slice 13 is next eligible for separate planning and remains Planned/unstarted.
Read this handoff, the system overview, the slice-13 roadmap and relevant code
before proposing its plan. Preserve the accepted creator/recovery contracts and
obtain plan approval before implementation. This closeout does not start slice 13.
