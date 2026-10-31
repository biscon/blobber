# B01 — Skeleton delivery and review

2026-10-31. **Accepted and closed (2026-10-31).**
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
are 480 px tall; full-resolution source art is preserved. Body width is .39375
cells after the review adjustment; clip canvases may extend to the existing one-cell publication bound.
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

B01 acceptance is recorded below and in the roadmap. B02 gets a separate detailed
bugbear plan, beginning with the measured texture-budget constraint. Campaign
integration still waits for the roster’s arena review and a separate request.

## User review adjustment — size and initiative (2026-10-31)

User found the skeletons too large and the centered initiative strip obstructed
health bars. Skeleton appearance height and every body/clip width now scale by
.875: .74375-cell height and .39375-cell body width. Ground anchors, source images,
registered pixel canvases and animation timing remain unchanged. The initiative
strip retains its top edge and aligns to the viewport’s right edge with an
18-native-pixel inset for every queue length.

Rebuilt the Release game, headless runner and affected HUD/metadata checks; **2/2
focused tests passed**. A fresh normal paired-skeleton arena run replayed strictly
with `matched` using the new executable/content identities. Inspected the updated
[contact view](screenshots/b01/scale-review/captures/operation-31.png),
[hurt view](screenshots/b01/scale-review/captures/operation-35.png) and
[corpse view](screenshots/b01/scale-review/captures/operation-41.png) through hidden
application-owned GPU capture. The strip is right-aligned and upright skeletons
have additional headroom with unchanged ground contact. Replay HUD captures omit
mouse-hover health bars; interactive hover review remains with the user. No OS
input or full regression/configuration matrix was used for this visual tweak.
Prepared texture/alpha byte totals are unchanged. Original B01 captures retain the
initial larger size and centered strip as historical evidence; the gallery links
the latest capture separately. This adjustment was included in the subsequent B01 acceptance below.

## User review adjustment — health-bar centering (2026-10-31)

Health bars now use the existing projected body-axis `badgeX` for horizontal
centering. The previous visible-silhouette center included the sword and shifted
the skeleton bar 6 logical pixels (18 native pixels) left of its head. Vertical
placement, viewport clamping and depth clipping are retained.

Rebuilt Release game/headless and affected targets; **HUD and world_visibility
passed (2/2, 0.07 seconds)**. An application-owned, hidden GPU review used the
production arena map, roster, party, movement updates, picking and HUD hover API.
No OS input was injected. Inspected the actual hover bar centered over the skull:
[left skeleton](screenshots/b01/health-review/skeleton-0.png) and
[right skeleton](screenshots/b01/health-review/skeleton-1.png). Body-axis centers
were 208/292 logical pixels versus silhouette centers 202/286. No asset rebake,
new storage or full test matrix was needed for this layout correction.

## Acceptance and continuation — 2026-10-31

User: “cool slice accepted, record it in the plan doc, so we can get started on the
next mob”. **B01 Accepted and closed**, including the final size, initiative-strip
and health-bar corrections. Executed checks, retained evidence and limitations
above remain unchanged; acceptance adds no new test or balance claims.

Next: B02 bugbear detailed planning against the current code and this handoff,
especially the approximately 4.8 MiB remaining mip budget. B02 is Planned and
unimplemented; its design needs review before implementation. Campaign integration
and PC07 remain paused. The [roadmap](bestiary_plan.md) is the active checkpoint.
