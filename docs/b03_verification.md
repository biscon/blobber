# B03 — Giant rat delivery and review

2026-10-31. **Accepted and closed (2026-10-31).** Detailed plan approved with “Implement the plan.”
The user requested a less cute, more vicious rat and approved turnaround-v3 with
“approved, thats a mean and nasty rat :)”. The user accepted the completed slice with “slice accepted, record it in the plan doc”.

[Review gallery](screenshots/b03/index.html) · [design](b03_giant_rat_design.md) ·
[art provenance](../assets/npcs/giant-rat/README.md).

## Delivered behavior

Normal Arena Setup exposes `giant.rat`: abilities 7/15/11/2/10/4, AC 12, fixed
HP 7, +4 bite for 1d4+2 piercing (critical 2d4+2), 25 XP, no loot/currency/potions.
Small beast, nonhumanoid, sleep-susceptible, unarmored, normal damage profile.
No diseased variant, poison, Keen Smell, new recovery, footprint or campaign recipe.
Ordinary approach, front protection and six-second action economy apply.

Pack Tactics scans at most 256 world-checked actors at accepted NPC attacks, after
target/ward selection and before the d20. Another living conscious allied monster
in legal melee contact with the shared party cell qualifies, including same-cell,
protected rear and mixed-species supporters. Conservative reach/LOS, corner,
elevation and door rules apply without individual party-character adjacency.
Sleep/Hold Person prevent support; prone, blind, slow or restrained alone do not.
Existing advantage cancellation/nonstacking applies; support adds no RNG. Accepted
rolls survive supporter removal during windup; subsequent attacks check again.
Resolved NPC weapon events/logs explain eligibility and final roll mode. Pending,
released and canceled attacks retain private roll context.

The full 48-pose set has four-direction idle, walk, hurt, death/corpse and prone,
plus front bite. Bite is 300/150/150/300 ms with impact at 450 ms; other timings
retain the roadmap baseline. Built-in imagegen produced three references, 21
selected sheets, nine corrections and the native portrait. Prompts, references,
corrections and measured registration are retained. Rear-view/facing corrections
and wider death gutters avoid invalid extraction. Alpha-composited QA confirms
background RGB outside the silhouette has transparent alpha; no backdrop is drawn.

Registration uses one scale per sheet, upper-torso pivots, explicit ground rows,
536 px logical canvases, 512 px ground anchors and 656–1256 px clip widths.
Death retains the first upright frame’s scale; prone matches anatomical length,
without normalizing collapse heights. Logical canvas height .30 cells and body
width .40 cells retain a small low silhouette. The portrait uses art ID 84, beyond
the current reserved party-portrait range 64–83. Existing extraction/cropping,
mips/alpha, shadows, picking, atlas and native presentation owners are reused.

## Storage and compatibility

Production publication includes five appearances and **271 textures**:

| Payload | Bytes | MiB |
| --- | ---: | ---: |
| RGBA mip textures | 244,123,736 | 232.81 |
| CPU alpha masks | 43,330,663 | 41.32 |

Current 512 MiB mip / 64 MiB alpha / 352 MiB CPU / 608 MiB tracked GPU ceilings,
scene/scratch and all count capacities remain unchanged. These are engine payload
accounting totals, not driver VRAM residency or whole-process RSS measurements.

Rules schema **11** recognizes `pack_tactics` and `nimble_escape`, at most two
distinct traits; old schemas retain their vocabulary. Unknown/duplicate schema-11
traits and unsupported programmatic bits reject. Campaign schema/simulation are
**3/5**, arena/layout formats remain **1/1**, world format/appearance remain **3/4**,
visibility remains world-visibility-v6. Older simulation/content rejects without
migration or rewriting files. Protocol remains v1 with additive `pack_tactics` /
`attack_roll_mode` on resolved NPC weapon attacks. Strict traces retain binary,
content, world and visibility identities. Campaign population/PC07 stay paused.

## Executed checks

One Release configuration, <=2 build workers, one test process, one intensive job,
low CPU/I/O priority and bounded internal threads. No benchmark, balance sweep,
capacity stress, sanitizer or configuration matrix. Memory/disk were checked first.

- Application, headless runner, affected targets and production bake built. All 48
  registered poses passed the existing extractor’s visible-footprint checks.
  Production catalog validation passed: six definitions / five attacks (including
  the existing placeholder guardian definition), four races/classes and 19 items.
- **13 focused CPU CTests passed in 1.57 s:** giant_rat, bugbear, animation, effects,
  impact, combat, hud, commands, rules_content, party_content, arena, automation,
  campaign_save_smoke. Subsequent localized giant_rat / commands checks passed;
  the added npc_metadata check passed independently. No unchanged matrix repeated.
- Rat fixtures cover solo/same-cell/protected/mixed/neighbor/multiple/behind-party
  support; dead/removed/distant/blocked-diagonal/elevation/closed-door cases;
  Sleep and held humanoids versus conscious prone/blind/slow/real-Web-restrained
  allies; foreign identity rejection; ordinary/advantaged/disadvantaged/canceled
  rolls, fixed-seed miss/hit/critical damage, 450 ms impact, private saved results,
  supporter death/removal/control during windup, own-attacker cancellation, exactly
  once XP sharing, no loot and engine-owned acceptance/impact/log allocation guards.
  Real Defend cancels Pack Tactics. Beast Hold Person rejection and Sleep eligibility
  are checked. Real floor-state export/import preserves pack scheduling and RNG
  under fresh actor-world identities. Existing impact tests cover hurt interruption.
- Loader tests cover schema 11 combinations/duplicates/unknown bits and legacy
  vocabulary. Automation covers additive fields and hidden acceptance/cancellation
  metadata. Campaign smoke covers normal roundtrip/continuation and explicit
  simulation-4 rejection with unchanged file bytes.
- Hidden **world_integration_gpu_tests** passed resource lifecycle, allocation,
  CPU/GPU picking, effects/door integration and preparation/travel rollback. Selected
  production captures exercise the actual rat textures/native portrait through
  raylib/OpenGL on the available desktop, using hidden owned windows only.
- **Five production traces replayed strictly with matched**, each with selected
  hidden application-owned captures. A separate selected mixed-trace replay adds
  the first nonlethal rat hurt/flash capture. Gallery links and rematch full-resource
  invariants were checked. No installed user library, scenario folder or save changed.

Early fixture compile/initiative/geometry setup errors were corrected before these
passing results. Initial review scripting read the wrong visual-observation key for
Sleep; that uncontrolled recording was moved to disposable scratch, then the legal
Sleep review was recorded and verified. No passing balance sweep is claimed.

## Arena evidence

All retained scenarios use **seed 8**, the ordinary level-one party and normal
visual-mode commands. Scenario JSON, exact event/checkpoint JSON and strict traces
are beside the gallery. These are behavior recipes, not difficulty promises.

- **rat-one:** one ordinary, unsupported bite deals 6; victory in round one.
- **rat-pack:** three rats produce four advantaged bites; victory in round two.
  Alden finishes at 1/12 HP. Three rats are dangerous despite low individual HP.
- **rat-goblin:** two rats plus a melee goblin demonstrate mixed-species support
  and visible size/overlap. Victory in round three; Brunna reaches 0 HP.
- **rat-thinning:** after two kills, ordinary End Turn actions allow the last rat
  another attack. It publishes `pack_tactics=false`, `attack_roll_mode=normal`.
- **rat-sleep:** legally prepare already-learned Sleep instead of Magic Missile,
  aiming beyond the rats to exclude the party. Two supporters fall asleep; the
  awake rat’s next bite is normal without Pack Tactics. Victory in round two.
- All five normal rematches reset time to zero and restore ordinary HP/mana.

Selected captures inspect approach/contact, rank overlap, scale beside goblin,
quick bite, lethal reaction/corpse, Sleep/prone/status badges, native initiative
portrait and rematch. All directions/poses were inspected in the registered alpha
contact sheet. This is not an exhaustive every-direction/diagonal/distance/status
GPU matrix. Human interactive motion, hover/selection feel and fun remain
unverified. Replay captures reconstruct current HUD, not historical floating
feedback/logs/hover. Full slow campaign persistence was not run: no saved structure
changed; relevant smoke and floor-continuation checks passed.

For review, open normal Arena Setup and try one Giant Rat, three Giant Rats, then
two Giant Rats plus one melee Goblin. Retained scenario files can be copied into
your scenario folder. Review low silhouette/selection, attack anticipation, ground
contact, control and pack thinning. B03 acceptance is recorded below; B04 implementation and campaign integration remain unstarted.


## Acceptance — 2026-10-31

User: “slice accepted, record it in the plan doc”. **B03 Accepted and closed**.
This includes the delivered standard rat, Pack Tactics, quick bite, full production
art, native portrait, arena integration and affected automation/save identities.
The executed checks and limitations above remain unchanged. B04 — Zombie and
Undead Fortitude is next for separate detailed planning; no implementation or
campaign integration is started.
