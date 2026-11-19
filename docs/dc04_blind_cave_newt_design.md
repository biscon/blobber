# DC04 · Blind cave newt in the Drowned Galleries

**Accepted and closed by the player on 2026-11-19.** The player approved the revised heavy bite and confirmed that it felt more dangerous. [Roadmap](damp_caves_plan.md) · [Spider handoff](dc03_venomous_spider_design.md#delivery-and-exit-checks)

## Player review revision

The first player walkthrough found a bright white fringe, inconsistent ochre lighting between clips, a frozen open jaw and two rear fighters with few useful actions. Two replacement paintings supply the directional poses, with matching moss-gray skin and naturally dark contours. Registration takes coverage from each master’s alpha channel, removes nearly transparent background noise and preserves antialiased toes and tail edges. One scale per view on each sheet keeps the shoulders and feet steady while the head moves. Pending bites alternate between a drawn-in throat and an open jaw every 480 ms on the existing animation clock. Only the first two attack frames can play during this loop. The committed lunge keeps its 420 ms impact and the defender retains a playable initiative opportunity. The launcher uses the regular fighter, cleric, rogue and wizard. The headless review also equips a separate four-fighter party for the original melee-only route check.

## Heavy bite balance revision

The approved single-target bite spends one newt action on the warning and another on the attack. Its +4 attack deals 3d6+3 piercing on an ordinary hit (6–21, mean 13.5). A critical doubles only the dice. When the named character uses Defend during this warning, the newt has disadvantage and a hit deals half the rolled damage, rounded down. Apply this reduction once before ordinary piercing resistance, temporary HP and injury handling. Keep the decision on the individual warning through round turnover and copy it into the committed attack. The log and HUD explain the response.

## Player decisions and placements

The standalone depth-five encounter uses the normal threshold, solo-newt reservation and both dry contact reservations. The mixed guards stand on the east shoulders, two cells east and one cell inward from their bank reservations. At the literal north and south reservation centers, the rib and four-cell separation isolated the north fight. The shoulder positions allow the normal three-cell ally alert to cross dry stone. The solo teaches the named heavy bite. At the west fork, the rope-marked north bank brings the spider into melee first and the south bank brings the newt into melee first. Both walks use identical placements, seed and party. Keep both mixed guards quiet during the lesson and while reading the fork. Use regular alert, pursuit, occupancy, initiative and picking throughout. Campaign descent still ends on floor three.

## Creature and perception

The blind cave newt is a one-subcell beast with AC 12, 16 HP, 15-foot initiative speed and a +4 heavy bite for 3d6+3 piercing. A kill grants 50 party XP once. These are standalone values for DC06 to evaluate. Use the accepted DC03 spider.

At the existing party step sound boundary, report the completed destination to AI exactly once. A conscious newt within three Manhattan cells hears it from any facing across a clear height-aware ray. The rib, closed doors and tight corners screen sound. Waiting, turning and bumps are silent. Volume and audio-device availability belong to presentation. Newts receive ordinary local warnings and investigate the remembered cell using the bounded navigation workspace. Their pursuit and investigation steps are spaced 1,200 ms apart. Use the species' actual perception for surprise and opening attacks. Intrinsic blindness is a sensing profile independent of the timed Blindness condition.

## Warn, respond, bite

Combat owns one transient warning per actor slot in a fixed array. A four-bit party mask lets the HUD find threatened characters and skips warning checks when none are pending. Hearing uses the existing navigation workspace. Neither sensing nor resolving a bite adds a frame-time allocation. In a legal melee position the newt spends one opportunity bracing against a conscious exposed character. Publish the source and named target in the view, HUD log and headless state. Cycle the two breathing poses while waiting. The target must receive a playable initiative opportunity after the warning. Record an accepted Defend against this particular bite so it counts across round turnover. On a later newt opportunity recheck source, target, ward, exposure and reach. With valid contact, roll the ordinary melee attack once and deliver damage at the registered impact. An invalid target, lost contact, obstructed reach, incapacitation or death cancels the warning. Ordinary damage during preparation preserves it. The committed lunge uses normal windup interruption. Clear warnings on leaving contact and session replacement. Legal settled saves cannot contain warning state.

## Presentation and content

The animator samples the two registered anticipation frames while a bite is pending. Each view shows the neck and jaw moving, with the toes planted. Defend uses the regular shield action. The HUD identifies the active participant and threatened ally, and the log names the victim when the newt braces. The headless event is `bite_warning`. A visible newt carries `heavy_bite_target` until it commits or abandons the attack. An abandoned warning emits `bite_abandoned`.

Two painted masters provide the four-direction stance, two walking steps, recoil, prone, corpse, warning and frontal lunge, plus the initiative portrait. The grounded 320×240 canvas spans half a cell. The four committed attack frames last 240, 180, 140 and 260 ms. The strike starts at 420 ms, subject to the existing hurt delay. Rules catalog 22 admits the newt trait. Campaign envelope schema 28 retains its settled-save format and simulation identity 31 identifies this rules revision. Prior identities follow the existing read-only rejection path. The heavier bite changes the shipped rules catalog fingerprint. Campaign saves written with the previous catalog therefore follow the same path. Standalone cave reviews start fresh. [Built-in imagegen prompts](../assets/npcs/blind-cave-newt/masters/prompt.txt) · [Frame registration](../assets/npcs/blind-cave-newt/register.py).

## Player review

Launch `./review-caves --depth 5 --seed 5 --encounter`. Each launch starts a fresh floor with Alden the fighter and Brunna the cleric in front, and Merrin the rogue and Lethiel the wizard in the rear. W/S move, A/D strafe, Q/E turn, F operates the survey door and M opens the map.

1. Follow the passage from the threshold and open the door. The first newt starts near the barrel at (5,15). Use **End Turn** until it braces. Watch the throat draw in and the jaw open on the same turn. The combat log and action dock name the threatened character.
2. When the named character can act, click the shield or press **2**. End the other characters’ turns to see the bite resolve. Check the attack roll in the log for disadvantage. If it hits, a second line gives the rolled and halved bite damage. On a fresh launch you can let the same warning pass with **End Turn** to see the ordinary bite. For the recorded bank openings, finish the solo fight with melee attacks and choose **Continue**. Try Merrin’s shortbow (3) and Lethiel’s Fire Bolt from the spell book (4) once both enemies are visible on the south bank.
3. Go east to the fork at (11,15). Face east toward the rib and take time to inspect the banks. Follow the rope on the left, turning east along the north bank. Use a front character’s melee action on the first reachable enemy. In the verified walk this was the spider from (16,13).
4. Relaunch and repeat the solo fight. At the same fork take the right, unroped bank and turn east. The first reachable enemy from (16,17) was the newt. After the first strike, use Defend or End Turn to give the spider time to approach. Inspect each silhouette and try selecting each body. The two guards initially occupy (17,14) and (17,16).
5. Compare the readiness of the named defender, the two possible openings and the sound of the heavy bite against the cave ambience. Add `--fallback` for the fixed alternate layout. AI movement and initiative respond to player timing, so later formations can differ.

## Mechanical verification

The ordinary walk and the fallback both started from the threshold with the same party and placements for the two bank choices. Each completed the solo warning, a valid Defend, the disadvantaged strike and a melee clear before the six-second wait at the fork. Each mixed fight included both creatures and ended in victory. The headless walks enumerated visible bodies and submitted the first attack through the command controller's melee validation. The native seed-five walks repeated this with the rendered body picker. Both paths used animated movement and doors.

| Floor / route | Position, facing | Legal first melee targets | First attack accepted |
| --- | --- | --- | --- |
| Seed 5, north rope | (16,13), east | 1 | Venomous spider |
| Seed 5, south | (16,17), east | 1 | Blind cave newt |
| Forced fallback, north rope | (16,12), east | 1 | Venomous spider |
| Forced fallback, south | (16,16), east | 1 | Blind cave newt |

Both party configurations used melee for the opening on each bank. In the regular party’s south fight, once both creatures were visible, Merrin submitted a shortbow attack and Lethiel submitted Fire Bolt from their original rear slots. The ordinary layout accepted its first proposal with 549 walkable cells. Forced fallback had 503. The two shoulder placements preserve the bank choice while permitting an ordinary three-cell companion warning and later enrollment. In the original four-fighter north run, the first strike killed the spider before the newt joined initiative. The south run showed both living enemies together. These are deterministic examples of target access and reinforcement timing.

| Focused check | Outcome |
| --- | --- |
| Configured Debug CTest, serial | `newt` covers all-around hearing, range, closed/open doors, blocked and queued steps, bounded slow pursuit, pause, a skipped cooldown opportunity, Defend across round turnover, an unanswered bite, Sanctuary, a downed victim, lost reach and source death during preparation and committed windup. Ordinary damage during preparation preserves the warning. Allocation guards cover engine-owned updates for hearing and biting. |
| Heavy bite revision | A fixed-seed combat test rolled 19 piercing with both choices. Ignoring the warning dealt 19 and the named character’s Defend reduced the hit to 9. The fixture checks the resulting HP and the standard critical roll. The serial Debug suites `newt`, `damage`, `impact` and `automation` passed. Both parties cleared both banks on the ordinary and forced-fallback layouts with the new rules. |
| Player revision | The Debug game and registered art rebuilt. Serial CTest `animation` and `newt` passed. Both parties completed both banks on the ordinary and fallback layouts. The native seed-five mixed party also used the rear shortbow and Fire Bolt. |
| Neighboring systems | `ai`, `spider`, `combat_positioning`, `impact`, `automation`, `cave_layout`, `campaign_save_smoke` and `runtime_inventory` passed. The automation test checks the playable warning event against the visible enemy's named target. `rules_content` initially used the newly assigned bit as its invalid-trait fixture. The corrected unknown-bit test passed. |
| Content and compatibility | The Debug game and registered world publication built. The complete world bake reports 637 textures, 950,685,844 mip bytes and 67,710,842 alpha bytes. The focused campaign-save `--ranger` case checks the envelope, including rejection of the preceding simulation identity. |
| Native capture | The hidden game renderer found pickable solo body pixels from all four directions, showed both living species together on the south bank, and completed both bank walks with the regular HUD, actor picker and command validation. Two captures 480 ms apart show distinct pre-impact poses while the same defender is still choosing an action. The renderer released its graphics resources on shutdown. [Six selected 1920×1080 views](screenshots/damp-caves-dc04/README.md). |
| Dependency handoff | The depth-four seed-five review still completed the safe camp pickup, failed venom save, antitoxin cure and weapon-only victory. |
| Audio | The offline Csound exporter generated and validated step, attack, hurt and death cues under `blind.cave.newt.*`. The bite uses the creature attack cue at commit. Party-step detection also ran without an audio device. |

Reproduction uses the configured Debug CMake and Ninja, at most two build workers and one CTest process. Run these commands from the repository root. For the capture command, set `capture_dir` to a fresh subdirectory of the owned run.

```sh
./review-caves --depth 4 --seed 5 --encounter --check
./review-caves --depth 5 --seed 5 --encounter --check
./review-caves --depth 5 --seed 5 --encounter --fallback --check
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest \
  --test-dir cmake-build-debug -j1 --output-on-failure \
  -R '^(animation|newt|ai|spider|combat_positioning|impact|automation|rules_content|cave_layout|campaign_save_smoke|runtime_inventory)$'
nice -n 15 ionice -c 3 /home/bison/clion/bin/cmake/linux/x64/bin/ctest \
  --test-dir cmake-build-debug -j1 --output-on-failure -R '^(newt|damage|impact|automation)$'
cmake-build-debug/blobber_campaign_save_tests --ranger
cmake-build-debug/blobber_review_caves capture 5 5 "$capture_dir" assets cmake-build-debug/assets/prepared --encounter
```

## Limitations and dependency handoff

Player review covers warning readability, the perceived weight of the bite, the ease of choosing a bank and the mix on physical speakers. Both banks were walked on seed five and forced fallback. Different solo actions can change the later initiative rolls. In a second ordinary-floor walk, using the shortbow and Fire Bolt during the solo fight let the spider join the newt before the first south-bank melee attack. Both enemies were then reachable. Other seeds and campaign-level encounter pacing were outside these checks. The standalone encounters start fresh on each launch. The ordinary campaign still ends at the Underkeep's third guardian.

For DC05, the reusable precedent is a player-visible warning followed by a verified initiative opportunity, with target access checked through the same commands available to a melee party. The newt's pending bite belongs to that individual attacker and its named party victim. DC05 owns its separate vault objective, clutch targeting and hatch state. The standalone review used a level-one party. DC06 should assess the bite against the level and equipment a party brings into the Galleries after clearing the Seep. DC06 can place the 50-XP newt and the accepted 50-XP spider, with 150 party XP across this isolated solo and pair.
