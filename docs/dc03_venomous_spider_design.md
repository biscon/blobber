# DC03 · The teaching spider at the Seep arch

**Accepted and closed by the player on 2026-11-19.** [Roadmap](damp_caves_plan.md) · [Accepted camp and arch](dc02_limestone_kit_design.md#exit-checks-and-handoff)

## Encounter and player decisions

The depth-four encounter option starts at the normal threshold. Use the accepted camp reservation for one loose antitoxin and the teaching arch for one guarding venomous spider. Keep the cache warning as scenery. Graphical play and the headless walk construct the same placements, ordinary ground loot, actors, AI and combat session. The empty scenery walkthrough remains available. Campaign descent ends on floor three.

The spider is a one-subcell beast with AC 12, 11 HP, 30-foot speed and a +4 melee bite dealing 1d4+2 piercing. A kill awards 50 party XP once. The camp supplies the only dose. This isolated fight contributes 12.5 XP per survivor in a four-member clear. DC06 owns the route budget.

A landed bite resolves ordinary piercing damage and damage reactions at impact. If the recipient lives and is susceptible, roll the shared Constitution save against DC 11. A failed save applies Poisoned. Misses, interrupted bites, mirror interceptions, dead targets and already-poisoned targets consume no venom RNG. Source death after impact leaves poison active. Poison imposes one source of disadvantage on weapon and spell attack rolls. Advantage cancels it normally. Saving throws, automatic hits and periodic HP are unaffected.

## Ownership and clocks

Catalog data owns the attack, species, and antitoxin. Combat owns attack acceptance, authoritative impact, resource charging and turn boundaries. Effects owns four party-indexed counters. Each starts at 12,000 gameplay milliseconds. In Turns remove 6,000 (saturating at zero) at each completed victim initiative opportunity, including an incapacitated or cooldown skip. Extra actions and other participants never advance it. Outside initiative consume actual elapsed gameplay time. Carry the exact remainder across clock changes, pause and victory feedback. Death clears the counter. Publish the NPC source on the save and application events. The ongoing condition belongs to its recipient and has no source lifetime dependency.

Use existing effect queries for attack rolls, status display and headless observations. Expose the recipient's remaining turns in initiative and seconds in exploration. Log the save, application, remaining turn, recovery and expiry. Persist the bounded remainder with party effects at settled checkpoints, transfer it once between floors, reject corrupt durations and inactive-floor ownership, and retain existing unsafe-save gates. Version simulation and codec for the change. The existing read-only incompatible campaign policy applies to prior content identities.

## Cures

Any able holder can use a single dose on a living poisoned party member, including self or a downed ally. A dose costs one action in initiative or a six-second action cooldown in exploration. A level-three or higher rogue with Cunning Action available can spend that allowance on the item. Validate item handle, owner, inventory revision, turn, target and active poison before spending anything. A full bag leaves the dose on the ground. Lesser Restoration cycles blindness, paralysis and poison at its usual cost. The headless command accepts the named choice and the legacy paralysis flag. The item cures existing venom immediately. Preventive antitoxin, poison ability-check penalties, Lay on Hands cleansing and ancestry poison traits are deferred. Poison Spray keeps its existing damage rules.

## Art, publication and review

Use the shipped creature and arch references for a low, eight-legged, readable cave spider. Register directional idle, walk, hurt, prone and collapsing corpse poses to the ground, plus a four-frame frontal bite whose fang strike matches the registered impact. Use prepared selection masks, portrait and generated antitoxin icon. Keep source artwork and prompts with the assets. Inspect native hidden captures of the walk, arch, attack and status readout.

## Player review

Run `./review-caves --depth 4 --seed 5 --encounter`. This opens a fresh isolated Seep with the level-one preset party. W/S move forward and back, A/D strafe, Q/E turn, F opens a door and M opens the map.

1. From the threshold, follow the survey trail to the camp near the bedroll. The ground bag is at (5,13). Click the bag from an adjacent cell, choose Merrin's party card, click the single antitoxin, then close the loot panel. A full bag leaves the vial available to another member.
2. Turn east into the arch. From (11,14), look at the spider beneath the roof teeth. Aim at its body or a leg. Gaps between the legs expose the floor. Watch the approach and the fang extension as it bites.
3. To see the condition, use **End Turn** instead of attacking until a bite lands and someone fails the DC 11 CON save. Hover over the victim's status badge. In the recorded walk Alden failed in round four. Holding different positions or acting earlier can change the rolls. To watch natural expiry, let that member finish two turns.
4. To try the cure, wait for the vial holder's turn while the badge is active. Press **I**, select the holder and vial, press **U** and click the poisoned member's card. The vial disappears, the badge clears and the log names both party members. Finish the spider with ordinary weapon attacks. For a character who knows Lesser Restoration, arm it in the spellbook, press **Tab** until it says **Cure poison**, and select the victim. The preset in this walk uses the vial.
5. Compare front and side scale, bite timing, badge legibility, and the volume of the movement, strike and death sounds against the cave drips. [Six native review frames](screenshots/damp-caves-dc03/README.md).

## Delivery and exit checks

The encounter builder binds one ordinary ground stack and one west-facing guard to the accepted DC02 reservations. Combat uses the cave's runtime seed stream. The generator and scenery-only launcher still produce the DC02 layout. The review runner waits six gameplay seconds at the camp to check the guard stays in its routine state. It uses the loot, inventory, ally-target and combat controllers during the walk.

The sprite uses 28 registered 320×210 frames from two painted masters. Four views cover stance, steps, recoil, prone and corpse. The frontal bite uses four frames with durations of 300, 150, 150 and 300 ms. The impact begins on the third frame, at 450 ms. Appearance width is half a cell and height is 0.328125 cell. Publication uses the usual alpha masks, mipmaps and fixed UI atlases. The complete Debug world bake reported 601 textures, 942,419,468 mip bytes and 66,157,413 alpha bytes. These figures cover the entire catalog. [Creature sources and registration](../assets/npcs/venomous-spider/register.py) · [Art prompts](../assets/npcs/venomous-spider/masters/prompts.json) · [Audio recipes](../tools/audio/recipes.json).

| Check | Observed result |
| --- | --- |
| `./review-caves --depth 4 --seed 5 --encounter --check` and the same command with `--fallback` | Both cleared. Ordinary used its first proposal with 535 walkable cells. Fallback had 521. The party collected one dose before alert, failed its first venom save in round four, cured through inventory and won with weapon attacks. |
| Debug `blobber_review_caves capture 4 5 NEW_OUTPUT assets cmake-build-debug/assets/prepared --encounter` | The hidden application renderer completed the same fight. The spider was targetable at approach and impact. Both the frontal and side picking masks exposed gaps between legs. The images show the frontal strike at native scale and the hovered Poisoned badge with its penalty and cures. The renderer and presenter released their tracked resources, and captured frames passed the OpenGL error check. |
| Focused Debug CTest, one process | `spider` exercises miss, save, failure, mirror interception, canceled windup, repeat bite, advantage cancellation, natural expiry across two initiatives, a skipped stable victim, Action Surge, clock transitions, pause, full inventory, rejected and double use, a downed ally and both cures. A poisoned Fire Bolt rolls with disadvantage. Poison Spray matches a healthy caster’s seeded damage. The condition tick and repeat-bite path also passed the engine allocation check. `level_spells` and `expanded_spells` cover neighboring spell and turn behavior. `automation` covers headless commands and observations. `cave_layout` checks ordinary and fallback geometry. `campaign_save_smoke` verifies the exact fractional remainder through encode/decode, two doorway transfers, inactive-floor rejection and expiry. |
| Catalog, packaging and compatibility | `rules_content`, `runtime_inventory` and `progression_allocations` passed. `blobber_campaign_save_tests --ranger` checked the new envelope and rejection of prior schema/simulation values. The Debug game executable and world publication built successfully. |
| Four species sound cues | Built and validated the attack, step, hurt and death WAVs with `python3 tools/audio/generate.py`, selecting the four `venomous.spider.*` cues individually. Physical-speaker listening was outside the recorded checks. |

The rules catalog is version 21. Campaign schema 28 and simulation 30 persist a sparse four-member duration and recognize the new content identity. Prior-version campaign slots require their original build. The isolated review starts fresh on every launch. Campaign descent still ends on floor three. Headless observations expose the affected member, clock and remainder. The named Lesser Restoration field is `restore_condition` with `blindness`, `paralysis` or `poison`. Older clients can still send `restore_paralysis`. [Rules adaptations](srd_content_ledger.md#the-seeps-venomous-spider).

**Player acceptance, 2026-11-19.** The player approved the spider after getting poisoned and successfully using the antitoxin in play. The level-one party is the standalone demo preset. For campaign pacing, the party should reach the Seep approaching level three. Keep ordinary venomous spiders at the approved level-one strength, using the fixed catalog profile above.

**Dependency handoff.** DC04 can use `venomous.spider` for the mixed approach with the existing AI and party condition queries. DC06 should evaluate the arch encounter against the party's earned Underkeep XP on both routes. The isolated demo remains available for replay.
