# Slice 10 casting UI correction: verification and handoff

Latest follow-up: [Class Features verification](spellcasting_ui_design.md#class-features-verification),
accepted and closed on 2026-10-27, records non-caster labels/icon, passive browsing,
resource clarity and the picker layout fix. The earlier
[native HUD and impact-alert verification](native_hud_verification.md) supersedes
the low-resolution rendering and acceptance-time alert behavior below.

Status: **Accepted and closed**, 2026-10-27. The user explicitly accepted slice 10
after its native HUD and impact-alert follow-up. Slice 11 remains Planned/unstarted. This implements the approved mockup-based correction and supersedes
the UI portions of the [original handoff](slice10_verification.md).

Design: [casting UI ownership and rendering](spellcasting_ui_design.md).

## Delivered

- Compact, input-capturing two-column picker with seven scrollable rows, specific
  unavailable reasons, hover/keyboard details and per-character scroll memory.
- Independent pending casts and Normal/SpellPicker/Targeting/Resolving states.
  Back returns to the picker, Cancel spends nothing, and confirmation commits once.
  Focus loss preserves valid selections and requires fresh input on return.
- Shared casting dock with fixed Cast/Back/Cancel positions and a visible,
  four-line wrapped combat log. Simple valid target clicks cast immediately.
- Bless portrait toggles, gold caster border, teal selected frames/checkmarks,
  explicit “Will receive” previews and removable recipient chips. Limits never
  replace recipients or auto-cast.
- Empty-to-three manual missile allocation, left-click add, right/Shift-click
  remove, fixed-size dart/count badges and pips. Invalid recipients are pruned
  individually without discarding remaining choices.
- Burning Hands/Thunderwave floor and enemy previews use the same bounded area
  query as validation and execution, including wall rules. Hidden bodies are not
  revealed; affected counts still include body-occluded occupants.
- Seventeen individual painted spell/ability icons, generated with built-in
  `image_gen.imagegen`. Sources are in [assets/ui/spells](../assets/ui/spells/);
  every exact prompt and original output path is in the
  [prompt manifest](../assets/ui/spells/prompts.json). Existing portraits, character
  data, costs, spell outcomes and formation remain intact.

## Executed checks

| Check | Result |
| --- | --- |
| Debug build and full CTest | 26/26, 108.85 s |
| Release build and full CTest | 26/26, 11.99 s |
| Focused ASan/UBSan | 6/6, 10.93 s; leak detection disabled |
| First/repeated-use allocation guard | Passed in both full suites |
| Hidden raylib/OpenGL lifecycle | Passed, NVIDIA RTX 3060 |
| Scripted hidden spell capture | 228 PNGs, including 39 at 1280×720; one TSV |
| Debug/Release seeded trace | Byte-identical to retained original trace |
| Source-free prepared startup | Passed with no NPC source PNGs or textures directory |
| Whitespace validation | `git diff --check` passed |

Build logs contained no compiler warnings or errors. Reproduction commands:

```sh
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
cmake --build cmake-build-release
ctest --test-dir cmake-build-release --output-on-failure
ASAN_OPTIONS=detect_leaks=0 ctest --test-dir /tmp/blobber10-sanitizers --output-on-failure -R '^(spells|effects|impact|hud|commands|actor_render)$'
cmake-build-release/blobber_spells_tests --stress
cmake-build-debug/blobber_spells_tests --trace
cmake-build-release/blobber_spells_tests --trace
cmake-build-debug/blobber_gpu_smoke
cmake-build-debug/blobber --capture-spells /tmp/blobber10-ui-captures
cmake-build-release/blobber --assets /tmp/blobber10-runtime-only --check-prepared
```

The sanitizer build uses AddressSanitizer/UndefinedBehaviorSanitizer and
`-fno-omit-frame-pointer`. Its focused suite covers spells, effects, impact, HUD,
commands and actor rendering. Regression coverage includes click ownership,
selection limits, cancellation/resource/RNG invariants, per-character picker
position, focus/held inputs, single and duplicate submissions, live invalidation,
precise eligibility reasons, area-query/commit agreement across headings, wall
occlusion, picking invariance, preview cache removal and logical log rewrapping.
Translucent sprite edges correctly reveal tinted floor; opaque interiors do not.

Separate stress: 256 NPCs plus twelve spell recipients, 528 drained events,
100 bursts, mean 161.705 µs/max 209.522 µs. One hundred area/effect-capacity cycles
took 45.3065 ms. These are local observations, not performance guarantees.
The unchanged seeded trace spends two mana at time zero and applies three darts
for fifteen damage at 342 ms; see [retained trace](slice10_trace.txt).

Source-free startup loaded 182,314,670 prepared bytes under the 512 MiB budget.
The new fixed floor mask/depth rows add 142,128 CPU bytes; atlas expansion adds
262,144 GPU bytes outside the prepared-bank accounting. All new steady-state
storage is bounded; PNG loading/resizing and diagnostic export happen outside it.

## Retained visual review

Sixteen native/reduced screenshots plus the complete capture TSV are retained in
[screenshots/slice10-ui](screenshots/slice10-ui/). The full disposable set is in
`/tmp/blobber10-ui-captures`. Selected native and reduced captures were inspected,
not every PNG. Final inspected cases include the wizard and cleric pickers,
unavailable mana, self-selected Bless caster, three-dart allocation, sparse orange
and blue area previews, Second Wind and concentration dismissal.

| Case | 1920×1080 | 1280×720 |
| --- | --- | --- |
| Spellbook | [Native](screenshots/slice10-ui/spell-magic_missile-book.png) | [Reduced](screenshots/slice10-ui/spell-magic_missile-book-1280.png) |
| Bless | [Native](screenshots/slice10-ui/spell-bless-target.png) | [Reduced](screenshots/slice10-ui/spell-bless-target-1280.png) |
| Magic Missile | [Native](screenshots/slice10-ui/spell-magic_missile-target.png) | [Reduced](screenshots/slice10-ui/spell-magic_missile-target-1280.png) |
| Burning Hands | [Native](screenshots/slice10-ui/spell-burning_hands-sparse-target.png) | [Reduced](screenshots/slice10-ui/spell-burning_hands-sparse-target-1280.png) |
| Thunderwave | [Native](screenshots/slice10-ui/spell-thunderwave-sparse-target.png) | [Reduced](screenshots/slice10-ui/spell-thunderwave-sparse-target-1280.png) |
| Insufficient mana | [Native](screenshots/slice10-ui/spell-magic_missile-unavailable.png) | [Reduced](screenshots/slice10-ui/spell-magic_missile-unavailable-1280.png) |
| Focus suspension | [Native](screenshots/slice10-ui/spell-magic_missile-unfocused.png) | [Reduced](screenshots/slice10-ui/spell-magic_missile-unfocused-1280.png) |
| Concentration | [Native](screenshots/slice10-ui/spell-bless-concentration.png) | [Reduced](screenshots/slice10-ui/spell-bless-concentration-1280.png) |

## Manual review recipe and limitations

```sh
cmake-build-debug/blobber --spell-demo --seed 5
```

The wizard picker starts open. Choose Magic Missile, allocate three darts, try an
extra click, remove one with right-click or Shift-click, and confirm. Try Back/B
and verify the picker position; Escape cancels without costs. Use the cleric's
Bless to select themselves plus allies, remove chips, and revise a full selection.
Choose Burning Hands and inspect its fixed area before confirming. Valid simple
single-target spells say “Click to cast”. Left/Right navigates targets, Up
adds/toggles, Down removes, and Enter confirms. Switch characters in exploration;
in turns use the active character. Verify focus return and normal turn progression.

At this earlier handoff, interactive desktop play, real high-DPI input and human
acceptance were unverified. The user subsequently accepted the completed slice on
2026-10-27; agent-driven interactive and real high-DPI checks remain unverified. LeakSanitizer remains unverified because leak detection was disabled
for the environment's tracing limitation. Source-free startup passed, but a full
source-free screenshot comparison was not run. Hidden application-owned windows
and in-process scripted input were used; no OS mouse, keyboard or focus automation.

Slice 10 is now accepted and closed. Preserve the original handoff's event-batch,
clock, capacity and ownership contracts. Slice 11 remains Planned/unstarted.

## Review polish: badge stability and spacing

Implemented and accepted with slice 10 on 2026-10-27. Spellbook icons now have a two-art-pixel
inset within their rows. Header text uses font ink bounds for vertical centering;
the gear shares the header center. Portraits are inset farther from the active
card border, leaving a visible gap. Action and log headings have more top padding;
footer/last-line spacing clears the bottom border while preserving four log lines.

Badges use projected actor positions and canonical idle geometry, separately from
animated visible-body bounds. A regression fixture changes silhouette width and
height between idle frames, checks stable badge/collision positions, then checks
actual movement and full occlusion. Picking and outline visibility still follow
the visible body; no smoothing delay or per-frame allocation was introduced.

Executed after these changes:

- Debug build; focused `spells|hud|commands|picking|actor_render|frame_allocations`:
  **6/6**, 21.69 s.
- Release build; the same suite plus `render|npc`: **8/8**, 3.79 s.
- Hidden application-owned `--capture-spells /tmp/blobber-ui-polish`: **231 PNGs**.
- Hidden `--capture-hud /tmp/blobber-hud-polish`: **73 PNGs**.
- Inspected native/reduced picker, selection and ordinary HUD captures. For the
  fixed simulation missile scenario, the badge region (native x=300..999,
  y=80..319) is pixel-identical at animation offsets 0/200/400/600 ms, while the
  creature region changes at each offset.
- `git diff --check` passed. No desktop input/focus automation was used.

The Debug build was warning-free. Release emitted GCC 16 array-bounds diagnostics
from the unchanged `std::sort` in `EffectController::apply` (effects.cpp:200);
`validate()` rejects counts outside the fixed three-target capacity before that
call. This review did not modify effect sorting. Full suites and sanitizers above
are historical results from the preceding implementation, not rerun for this polish.
The user subsequently completed review and accepted slice 10 on 2026-10-27.

Selected captures are retained in [slice10-ui-polish](screenshots/slice10-ui-polish/):
[picker](screenshots/slice10-ui-polish/spell-magic_missile-book.png),
[reduced picker](screenshots/slice10-ui-polish/spell-magic_missile-book-1280.png),
[missile selection](screenshots/slice10-ui-polish/spell-magic_missile-target.png),
[idle 200](screenshots/slice10-ui-polish/spell-magic_missile-idle-200.png),
[idle 400](screenshots/slice10-ui-polish/spell-magic_missile-idle-400.png),
[idle 600](screenshots/slice10-ui-polish/spell-magic_missile-idle-600.png),
[Bless borders](screenshots/slice10-ui-polish/spell-bless-target-1280.png), and
[ordinary HUD](screenshots/slice10-ui-polish/hud-impact-hit-handoff.png).
