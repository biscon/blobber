# B04 — Zombie delivery and review

2026-11-01. **Accepted and closed.** Detailed plan approved with “Implement the plan.”
The user selected the decayed villager direction and accepted turnaround v1 with
“reference accepted”. The user accepted the completed slice with
“slice accepted, record it in the plan doc”.

[Review gallery](screenshots/b04/index.html) · [design](b04_zombie_design.md) ·
[art provenance and exact prompts](../assets/npcs/zombie/README.md).

## Delivered behavior

Normal Arena Setup exposes `zombie`: abilities 13/6/16/3/6/5, AC 8, fixed HP 22,
+3 slam for 1d6+1 bludgeoning (critical 2d6+1), 50 XP, no loot/currency/potions.
Medium undead, one subcell, poison-immune, sleep-immune, nonhumanoid and nonmetal
clothing. No radiant or bludgeoning vulnerability. Existing movement, approach,
formation protection and six-second economy remain. The source ledger records
omitted tabletop Wisdom save proficiency, speed, senses, languages and poisoned
condition handling. Campaign population/guardians and PC07 remain paused.

One shared NPC damage helper applies mitigation and Fortitude for combat impacts
and burning Web. Positive lethal damage makes an effect-modified Constitution
save (+3 before effects) against DC 5 + the full post-mitigation amount, including
overkill. Radiant and critical hits bypass; ordinary natural-20 save rules apply.
The 64-bit DC saturates safely. Saves occur at impact, preserving accepted rolls
and existing recipient order; Magic Missile stays aggregated per target and
Scorching Ray stays separate. Misses, zero/immune/nonlethal/invalid/gone impacts
and bypasses consume no Fortitude RNG.

Success atomically retains one HP, life, occupancy and identity before any death
marker, death animation, loot, XP, objective or arena-clear notification. Positive
hits still record hurt/interruption even when already at one HP. Failure/bypass
commits ordinary death. Resolved events retain full adjusted damage independently
of actual HP removed. Fixed logs explain the save/failure/bypass; the survival
splash says “1 HP”. New metadata is private until the resolved attack event.

The complete 48-pose / 21-sheet production set has four-direction idle, walk,
hurt, death/corpse and prone, plus front slam. Slam uses 700/250/250/350 ms with
impact at 950 ms; remaining timings match the roadmap. Built-in imagegen produced
the accepted reference, full set, five retained corrections and native portrait.
The existing extractor uses measured gutters/body pivots, 800×608 canvases and
ground anchor 584, one scale per sheet. Fallen frames keep the upright scale;
prone uses anatomical length. Full-resolution crop offsets, mips/alpha, shadows,
picking and the existing UI atlas are reused. Zombie portrait ID 85 follows rat
84, beyond reserved party IDs 64–83. No renderer or runtime resource owner added.

## Storage and compatibility

Production publication contains **six appearances and 319 textures**:

| Payload | Bytes | MiB |
| --- | ---: | ---: |
| RGBA mip textures | 273,621,212 | 260.95 |
| CPU alpha masks | 48,868,662 | 46.60 |

Unchanged ceilings: 512 MiB mip / 64 MiB alpha / 352 MiB tracked CPU / 608 MiB
tracked GPU; all actor/appearance/effect/event count capacities remain unchanged.
These are engine payload totals, not process RSS or driver VRAM residency.

Rules schema **12** adds `undead_fortitude` to the typed byte mask, at most three
distinct traits. Earlier schemas retain their vocabulary/limits; unknown/duplicate
traits and unsupported programmatic bits reject. Campaign schema/simulation are
**3/6**, with explicit old-simulation/content rejection without rewriting files or
migration. No saved structures/trait charges were added. Arena/layout remain 1/1,
world/appearance 3/4, visibility world-visibility-v6. Protocol v1 adds resolved
`undead_fortitude` outcome and attempted-save fields; strict identities still apply.

## Executed checks

One Release configuration, <=2 build workers, one test process, one intensive job
at a time, low CPU/I/O priority and bounded internal threads. Memory/disk checked
before costly work. No benchmark, capacity stress, balance sweep, sanitizer or
configuration matrix. No OS input/focus/desktop automation or user save/library
changes.

- Application, headless runner, affected targets and one production world bake
  built successfully. All 48 registered poses pass the ordinary extractor's
  visible-footprint checks. Production catalog validation passes: seven monsters
  (including the existing guardian placeholder), six attacks, four races/classes
  and 19 items.
- **17 focused CPU CTests passed in 1.67 s:** zombie, giant_rat, bugbear, damage,
  animation, effects, impact, combat, hud, commands, actors, rules_content,
  party_content, arena, automation, npc_metadata and campaign_save_smoke.
- New self-owned fixtures cover success/failure, exact lethal/overkill DCs,
  repeated survival at one HP with zero HP removed, hurt instead of death,
  stable identity/occupancy/revision, ordinary save rules (including natural 20
  failure), Bane and exact RNG consumption, huge amounts, immunity/nonlethal/zero,
  invalid/foreign/removed/dead identities and stale timestamps.
- Real weapon/spell paths cover delayed arrows, melee, misses/criticals, pause,
  exact deadlines, Poison Spray, Sacred Flame, Burning Hands, Magic Missile,
  Scorching Ray, Shatter, Ray of Frost, Guiding Bolt, Thunderwave, Spiritual Weapon,
  Inflict Wounds, Shocking Grasp and burning Web. Three simultaneous rays preserve
  ordered results, final HP/death and RNG across fresh-world restore and 10/100 ms
  tick subdivision. Existing impact/effect tests cover cancellation/interruption.
- Dedicated reward fixtures verify no premature arena clear, gate opening, XP or
  corpse/objective drops; actual death issues each once. One-HP and dead floor
  snapshots retain authoritative markers and credited deaths under fresh world
  identities. Save smoke checks continuation and simulation-5 rejection with
  unchanged file bytes. Saving/travel admission remains unchanged.
- Allocation guards cover the new helper/save/atomic commit, real delayed impact
  processing and HUD survival feedback. They intercept engine-owned ordinary and
  aligned C++ allocation; no claim covers graphics-driver or C-library internals.
- Loader and automation fixtures verify schema vocabulary/counts, invalid bits,
  independent 64-bit save metadata, bypasses without invented saves and no
  acceptance/release/cancellation disclosure. HUD tests verify species portrait,
  survival log and “1 HP” splash. Existing arena checks retain save/edit/rematch.
- Hidden **world_integration_gpu_tests** passed actual raylib/OpenGL lifecycle,
  CPU/GPU picking, frame allocation/upload guards, effects/doors and staged rollback
  on the available NVIDIA desktop. Production captures separately exercise the
  real zombie artwork and native portrait.
- **Four production traces replayed strictly with matched** and 16 selected hidden
  captures. Three supplemental strict reconstructions retain five later hurt/corpse
  boundaries after the initial white hit flash. All seven capture manifests say
  matched; rematches restore zero time and ordinary full HP/mana.

Initial fixture seed coverage, curated racial-cantrip overlap and melee formation
setup were corrected before the passing results. The first capture invocation
omitted the recording's campaign-persistence flag and correctly rejected the
header; matching that flag fixed invocation compatibility. No passing work was
repeated to obtain another count.

## Arena evidence and limits

All retained scenarios use **seed 8** and the ordinary **level-one** party. Exact
scenario, event/checkpoint JSON, strict traces, capture manifests and gallery links
are beside this handoff's gallery. These are behavior recipes, not balance labels.

- **zombie-one:** one zombie; ordinary weapon attacks and wizard Fire Bolt.
  A lethal 3-damage blow fails DC 8 (total 7); victory in round two, no party HP lost.
- **zombie-radiant:** one zombie; cleric uses Sacred Flame, with ordinary Defend
  actions reserving low-HP finishing attempts for the cleric. Lethal radiant 6
  bypasses Fortitude. Victory in round four; Brunna 9/12 HP. Deliberate waiting
  means this is not a direct battle-length comparison.
- **zombie-two:** two zombies; two successful saves (5 damage/DC 10, 6 damage/DC 11)
  retain one HP. Subsequent blows fail their saves and kill normally. Victory in
  round three; Brunna 10/12 HP, other members full.
- **zombie-goblin:** one zombie plus melee goblin, for mixed-species scale and
  readability. Zombie survives 9 damage/DC 14 then fails against 14 damage/DC 19.
  Victory in round three; Alden 5/12, Brunna 0/12, Merrin 10/10, Lethiel 0/7.
  **Dangerous encounter**, with two companions down; no hidden party protection.
- Every ordinary rematch restores time zero and full starting HP/mana.

Inspected native contact/near/far approach, goblin comparison, portrait, slam,
hurt/surviving standing body and collapsing corpse. All directions, alpha holes,
walking/idle variants and prone poses were inspected in the registered contact
sheet. This is not an exhaustive every-direction/diagonal/rank/status GPU matrix.
Replay captures reconstruct current HUD, not historical hover/logs/floating
feedback; the CPU suite verifies survival text, while interactive presentation,
hover/selection feel, motion and fun remain for the user to review. Full slow
campaign persistence was not run: no saved structure changed; smoke and focused
floor/RNG continuation checks passed. Campaign balance is not established.

Review normal Arena Setup with one Zombie, two Zombies, then one Zombie plus one
Goblin. Try Sacred Flame or Guiding Bolt as finishers. Completed-slice acceptance
is recorded below; B05 and campaign/PC07 work remain unstarted.

## Acceptance — 2026-11-01

User: “slice accepted, record it in the plan doc”. **B04 Accepted and closed**.
This includes zombie mechanics, atomic Undead Fortitude, full production art,
native portrait, normal arena integration and affected automation/save identities.
The executed checks and limitations above remain unchanged. B05 — Goblin shaman
and enemy spellcasting is next for separate detailed planning; implementation
and campaign integration/PC07 remain unstarted.
