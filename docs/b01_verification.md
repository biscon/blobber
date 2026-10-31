# B01 — Skeleton delivery and review

2026-10-31. **Ready for review; user acceptance pending.**
The roadmap and detailed implementation plan were approved. The user approved
`turnaround-v1.png` with “yes concept art approved”. B02 is Planned. Campaign
integration and PC07 remain paused.

## Delivered behavior

Skeleton is selectable in normal **Main Menu → Arena Setup**, alongside the
existing goblins. Its melee shortsword is +4, 1d6+2 piercing; AC 13, HP 13, 50 XP,
one standard shortsword loot. Poison deals zero damage; bludgeoning doubles the
final rolled total. Sleep excludes it, Hold Person rejects it, and rusted armor
participates in the existing Shocking Grasp rule. It still occupies one subcell.

Typed damage applies once at impact across weapons, immediate/delayed/area and
multi-hit spells, Spiritual Weapon, Shatter and burning Web. The new profiles are
immutable catalog data. Pending rolls remain hidden. Zero damage causes no hurt,
windup interruption, awakening or concentration check. Independent hit riders,
push saves and environmental ignition retain their behavior. Weapon hover ranges,
poison targeting warnings, neutral IMMUNE feedback and vulnerability logs explain
the result. Initiative portraits and logs identify skeletons correctly.

Rules schema is 10; schemas 1–9 retain normal profiles. Save schema stays 3 and
simulation becomes 4. Pre-B01 saves reject without migration; strict traces retain
binary/content validation. No campaign recipe, placement, guardian or XP pacing
was changed. See [design](b01_skeleton_design.md) and [source/adaptation ledger](srd_content_ledger.md#b01--skeleton-and-typed-damage-2026-10-31).

## Art and renderer evidence

[Animated gallery and arena captures](screenshots/b01/index.html),
[all-pose contact sheet](screenshots/b01/pose-contact-sheet.jpg), and
[art provenance](../assets/npcs/skeleton/README.md).

The approved skeleton uses the goblins’ textured, shaded, muted dungeon style.
Built-in imagegen produced 21 sheets (48 poses), eight corrective edits and a
native initiative portrait. Transparent full-resolution masters, superseded
masters, exact prompts, references and measured registration are retained under
`assets/npcs/skeleton/`. No unused ranged clip was added. The four-direction
idle/walk/hurt/death/prone set and front melee follow existing goblin timings;
impact is at 950 ms. Death ends as a bone pile; prone remains an intact body.

The production extractor generated all 48 poses successfully with nearest
sampling at resolution 1. Registration preserves ground rows and uniform scale
within each sheet, including collapse frames. Reviewed the contact sheet and
selected native-sized poses, plus 1920×1080 production-asset captures of paired
skeletons, mace reaction, death/loot, Poison Spray and a mixed goblin-archer group.
Grounding, margins, sword handedness, separate prone/death silhouettes and skull
initiative portraits were inspected. Source corrective edits address repeated
stride poses and back-view handedness. Side strides remain restrained two-frame
movement and await human motion review.

The prepared publication contains **175 textures, 263,362,808 mip bytes and
46,895,760 alpha bytes**, within the unchanged 256 MiB / 64 MiB budgets. Canvases
are 480 px tall; full-resolution source art is preserved. Body width stays .45
cells; clip canvases may extend to the existing one-cell publication bound.
No world format, occupancy or GPU registry capacity changed. Only about **4.8 MiB
of mip budget remains**: B02 planning must address total publication size before
adding more art. This is a measured content limit, not a reason to raise budgets
without evaluating the preparation/layout options.

## Executed checks

One Release configuration, at most two build workers and one test process, low
CPU/I/O priority. No benchmark, balance sweep or repeated configuration matrix.

- Application, headless runner, affected test targets and production asset bake
  built successfully.
- **15/15 focused CTests passed**: damage, rules_content, party_content, combat,
  impact, commands, hud, spells, expanded_spells, level_spells, effects, arena,
  automation, campaign_save_smoke and npc_metadata (1.56 seconds).
- New typed damage tests cover resolver bounds/overflow, catalog profiles,
  legal spell costs, immunity/vulnerability on immediate/delayed/area and
  multi-impact paths, damage-only save skipping, retained push saves/Guiding Bolt/
  Ray of Frost riders, burning Web, undead spell eligibility and metal armor.
  The allocation guard covers an actual typed melee impact. Command tests check
  explicit IMMUNE feedback, actual species logs and skull portrait selection.
- A subsequent damage-only check passed after adding paired normal/vulnerable/
  immune weapon sessions with identical fixed seeds, covering hit, miss and
  critical totals, unchanged rolls and no immune hurt (0.01 seconds).
- After tightening the save regression to the actual pre-B01 simulation version 3,
  campaign_save_smoke passed again (0.54 seconds), explicitly verifying rejection.
- Rules tests reject duplicate/unknown/overlapping damage names and missing data;
  schema 9 defaults remain normal. Metadata tests admit padded clip canvases and
  reject widths beyond the existing publication bound, retaining narrow bodies.
- Hidden `blobber_world_integration_gpu_tests` passed after the final production
  build: lifecycle, owned frame allocation, picking parity, preparation failures
  and travel rollback; reported edge differences 0. These use owned fixtures,
  separately from the production arena captures.
- Three normal production-asset arena traces replayed strictly with `matched`.
  [Exact operations and events](screenshots/b01/review-final.json) record the
  cleric’s mace **raw 6 → 12 bludgeoning**, the wizard’s Poison Spray **raw 11 → 0
  poison / immune**, a normal slashing killing blow and the archer’s piercing
  approach shot. All actions used normal previews, submissions and fixed ticks.

The ordinary impact/combat/effects/arena suites retain coverage of interruptions,
critical handling, blindness, knockdown, corpse behavior and arena save/rematch.
Those invariants are shared across species; no alternate skeleton combat system
or test dependency on production art was introduced.

## Review recipe and limitations

Run the rebuilt game, open **Arena Setup**, select the normal party and add
**Skeleton**. Start with one, then two, then two with one goblin archer. Use
Brunna’s mace to see vulnerability; compare Alden’s longsword. Inspect movement,
attack anticipation, hurt, bone-pile death, loot and Rematch. The mixed group lets
you compare the art directly with existing goblins.

Retained seed-5 scenarios:
[one](screenshots/b01/skeleton-one.json),
[pair](screenshots/b01/skeleton-pair.json),
[mixed](screenshots/b01/skeleton-archer.json),
[Poison Spray party](screenshots/b01/skeleton-poison.json).
To load them in the menu, copy the chosen files to your existing Blobber
`scenarios/` folder (normally `$XDG_DATA_HOME/blobber/scenarios` or
`$HOME/.local/share/blobber/scenarios`). No installed character library or user
scenario files were modified. The poison scenario legally replaces the wizard’s
Acid Splash selection with Poison Spray; other party choices remain unchanged.

The gallery’s replay captures reconstruct current scene/HUD; they omit historical
combat logs, floating feedback and past mouse interactions. The exact immunity
result is in trace events, and command/HUD tests verify its feedback state. These
captures do not visually verify the floating IMMUNE label. The normal game records
those events through its existing persistent command controller.

No OS input, focus changes or desktop automation were used. Human interactive
motion/readability/fun review remains unverified. The contact sheet covers every
clip/direction; production arena captures are selected front-facing combat views,
not an exhaustive near/far/diagonal/overlap or every-status capture matrix. No
claim of balance or fun-factor acceptance is made. Debug, sanitizer, broader slow
save/CLI tests and stress/benchmark runs were not run for this slice.

After user acceptance, close B01 in the roadmap. B02 gets a separate detailed
bugbear plan, beginning with the measured texture-budget constraint. Campaign
integration still waits for the roster’s arena review and a separate request.
