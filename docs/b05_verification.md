# B05 — Goblin shaman delivery and review

2026-11-01. **Accepted and closed.** The user approved implementation with
“Implement the plan” and accepted turnaround v1 with “reference approved”.
The user accepted the completed slice with “slice accepted, record it in the plan
doc”. Existing checks and limitations remain unchanged by acceptance.

[Review gallery](screenshots/b05/index.html) · [design](b05_goblin_shaman_design.md) ·
[exact artwork prompts/provenance](../assets/npcs/goblin-shaman/README.md).

## Delivered behavior

Normal Arena Setup exposes `goblin_shaman`: Small one-subcell humanoid, abilities
10/10/10/10/14/11, AC 10, fixed HP 9, DC 12, 50 XP, +2 staff/club for 1d4
bludgeoning (critical 2d4). No loot, metal armor, damage modifiers, sleep immunity,
Nimble Escape, healing or Sanctuary casting. The source ledger records the Acolyte
baseline and adaptations. B05 did not change campaign population/guardians.

Each shaman starts with six mana. Bless costs two, Sacred Flame is free, and
encounter transitions do not refill resources. Bless requires another conscious,
unblessed encounter ally; choose other actors by stable slot and include self last,
up to three recipients. Otherwise rotate Sacred Flame through conscious party
members in formation order, skipping downed/dead characters without inspecting HP.
Sanctuary uses ordinary checked ward saves and candidate rotation. When casting is
unavailable, existing approach movement or weak melee applies. Casting capability
participates in initiative/preview and holds usable support positions behind allies.

NPC casts reuse the existing actor-owned pending action and ranged animation slot.
Production casting has 600/400 ms gather/release timing and no arrow flight.
Acceptance retains private rolls and charges once; damage and target save publish
at release. Hurt suspends/resumes windup; pause freezes it; incapacitation/death or
a downed/dead locked Sacred Flame target cancels without retargeting or refund.
Bless reserves existing effect capacity before costs/RNG and starts its 60-second
concentration at release. Reservations are separate from Mirror Image bits.

Tagged effect sources distinguish party identities from world-checked ActorIds.
NPC sources can own only Bless on NPC recipients. Positive adjusted damage notifies
concentration once through real impacts and burning Web; zero/immune damage rolls
nothing, and incapacitated/dead/removed sources clean up without a damage save.
Ordinary prone alone retains concentration. Source identity appears in normal cast,
save/damage/lifecycle events, logs, hover and observations. No privileged NPC command
or new generic ability framework was added.

All 56 production poses /25 sheets, five retained corrections, native portrait 86
and normal appearance binding are delivered through built-in imagegen. The accepted
reference fixes the tribal robe/charms/right-hand staff identity. Melee, idle, walk,
cast, hurt, death/corpse and prone use the approved timings. Measured body pivots,
gutters and crown corrections retain one scale per sheet; fallen frames are never
normalized to upright height. 800×720 logical canvases/ground anchor 696 accommodate
the staff while preserving goblin body scale. Existing extraction, full-resolution
crop, mips/alpha, shadows, attachments, picking and UI atlas ownership are reused.

## Storage and compatibility

Seven-appearance publication contains **375 textures**:

| Payload | Bytes | MiB | Existing ceiling |
| --- | ---: | ---: | ---: |
| RGBA mip textures | 329,839,864 | 314.56 | 512 MiB |
| CPU alpha masks | 59,422,180 | 56.67 | 64 MiB |

Tracked world CPU/GPU ceilings remain 352/608 MiB. Actor/appearance/catalog/effect/
timeline/event capacities stay fixed. These are engine payload budgets, not process
RSS or driver residency. The conservative event allowance is 6,728 <8,192, including
NPC save notices/concentration checks and the existing lifecycle/area allowances.

Rules schema **13** adds `none` / `acolyte_support` casting profiles; older schemas
retain their vocabulary and default to no casting. Campaign schema/simulation are
**4/7** (previously 3/6): tagged sources and bounded mana/cursors are serialized,
validated, included in canonical digests and rebound to fresh world identities.
NPC resources/effects remain local and frozen while inactive. Travel portability
requires party ownership; local NPC Bless neither travels nor requires party
concentration dismissal. Old saves reject without migration or rewriting files.

Arena/layout remain 1/1, world/appearance 3/4 and protocol v1. Effect events retain
numeric `caster` for party sources, null for NPC, plus tagged `caster_source`.
Visible NPCs can expose concentration; diagnostics include exact mana/target cursor.
Strict replay retains binary/content/publication identity checks.

## Executed checks

Used Release builds, at most two build workers, serial tests and low CPU/I/O
priority for lengthy commands. The graphics build supplied asset preparation,
application/headless capture and GPU integration; CPU suites were not duplicated
as a Debug/Release or graphics/headless matrix. No benchmark, swarm/capacity stress,
sanitizer sweep, desktop input/focus control or user save/library mutation.

- **24 focused CPU CTest suites passed across the affected development checks:**
  shaman, zombie, effects, impact, hybrid, ai, combat, hud, commands, rules_content,
  party_content, arena, automation, automation_allocations, campaign_save_smoke,
  animation, progression_allocations, level_spells, visual_review, expanded_spells,
  recovery, inventory, spells and npc_metadata. The nine existing spell/status
  caller suites completed together in 0.11 s. Final shaman/automation checks pass;
  a final blinded-caster melee fallback check also passes.
- The full **campaign_save** integration suite passed once at the persistence
  checkpoint (3.99 s), including the new live NPC source departure, inactive-floor
  hub rest, disk roundtrip, return, mana/cursor retention and source-world rebinding.
  Save smoke and incompatible-file byte-preservation checks also pass.
- Self-owned small fixtures cover ally-first support/solo cantrip/target rotation,
  mana exhaustion, no duplicate reservations, full-capacity rejection before mana/
  RNG, Sanctuary, source identity separation, invalid/foreign sources, unrelated
  NPC spell rejection, death/removal/sleep/prone cleanup, pause, exact release,
  downed locked targets, real-arrow hurt interruption/resume and timestep parity.
- Real arrow and burning-Web hits each issue exactly one NPC concentration check.
  C++ allocation guards cover NPC decisions/acceptance, reserved release, damage
  notification/Web and capacity rejection. Existing runner/allocation coverage also
  passes. These intercept ordinary/aligned C++ allocation, not graphics drivers or
  third-party C-library internals.
- Loader/automation/UI checks cover schema profiles/defaults/rejection, additive
  caster tags, legacy party caster fields, private acceptance data, source-specific
  casting logs, visible concentration hover and the new portrait discriminator.
- Application, graphics headless runner and tools built. All 56 poses pass the
  ordinary full-resolution extractor's clipping checks. One completed production
  world bake validated the complete seven-appearance payload against fixed limits.
- Hidden **world_integration_gpu_tests** passed actual raylib/OpenGL resource,
  allocation/upload, effects/doors, CPU/GPU picking and staged rollback checks;
  reported edge differences =0. Production captures separately exercise actual
  shaman artwork, casting cue, native portrait, support ranks and corpse.
- **Four production arena traces replayed strictly with matched**, exporting 16
  selected hidden captures. Three further strict reconstructions retain two
  post-flash collapse boundaries and one settled corpse: **seven matched manifests,
  19 captures total**. Normal rematches reset time and full party resources; exact
  diagnostic checks confirm six mana on each fresh shaman.

The initial new boundary fixture used float deltas and was corrected to the
session's double timing API. Hurt tests were changed to real authoritative arrow
impacts rather than diagnostic direct HP writes. The existing allocation suite
initially requested stairs without facing them; its setup now uses the retained
checked campaign travel helper. Typed source API call sites were updated before
the passing caller builds. Artwork corrections and measured registration fixes
were completed before the production bake and retained recordings.

## Arena evidence and limits

All recipes use **seed 8**, the ordinary four-character **level-one** party and
normal starting gear/spell choices retained in each scenario JSON. The normal
headless visual interface supplies checked movement/actions, with exact diagnostics
only for the post-rematch resource check. Scripted waiting exposes a casting beat;
these are behavior recipes, not balance labels or unattended balance experiments.

| Recipe (ordered roster) | Observed behavior/result | Final party HP: Alden /Brunna /Merrin /Lethiel |
| --- | --- | --- |
| shaman-one: shaman ×1 | Two Sacred Flames; victory during ranged approach | 8/12, 12/12, 10/10, 7/7 |
| shaman-support: shaman ×1, goblin ×2 | Bless on three; two successful concentration saves; victory round 2 | 4/12, 4/12, 10/10, 7/7 |
| shaman-concentration: shaman ×1, goblin ×1 | First concentration total 13 succeeds; later total 3 fails DC10, ending Bless; victory round 1 | 12/12, 4/12, 10/10, 7/7 |
| shaman-two: shaman ×2, goblin ×1 | First caster Bless, second Sacred Flame then its own Bless after source loss; victory round 2 | 2/12, 5/12, 10/10, 7/7 |

The roster puts shamans in the rear subslots, and actual approach keeps them behind
the melee goblins. Ranged attacks legally reached them; ordinary melee still
respected front protection. The dual-caster recipe is dangerous for the front rank;
no hidden HP protection, healing or mana grants were used. Timings include approach
and presentation, so these runs do not provide isolated battle-length comparisons.

Inspected all directions/poses in the registered contact sheet and selected native
far/near/rank overlap, gathering/release, portrait, hurt, collapse and final corpse
captures. This is not an exhaustive every-direction/diagonal/status GPU matrix.
Replay captures reconstruct the current HUD, not historical hover/logs/floating
feedback; CPU checks verify those feedback paths. Interactive motion, target-selection
feel and fun remain user review items. Campaign balance is not established, and no
arena level-preparation UI or campaign population integration is claimed.

Review ordinary Arena Setup by searching for shaman, then try the retained recipes
and concentrate ranged pressure on the caster. B05 is **Accepted and closed**.
The bestiary roadmap is closed; B06 is superseded without implementation and its
useful calibration scope transfers to PC07 planning. No campaign balance is claimed.
