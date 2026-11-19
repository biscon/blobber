# DC05 · The Brood Vault

**Accepted and closed by the player on 2026-11-19.** A full-strength level-two party can enter the standalone depth-six vault, break the hanging egg sacs with ranged attacks, or fight the brood. The west and east rope segments are survey dressing. The accepted floor-five fork still changes which creature a party can reach first. The rope itself grants no action, bonus or visibility effect. [DC04 dependency handoff](dc04_blind_cave_newt_design.md#limitations-and-dependency-handoff) · [DC02 vault kit](dc02_limestone_kit_design.md).

## Encounter design

The DC02 vault supplies a mother position, two separate nest sites and two legal hatch cells. The encounter replaces the two floor-standing clutch props with destructible silk hanging from the ceiling. A fighter can occupy the floor below a sac. The suspended sac has its own silhouette and exposed pixels. A flank, a pillar or the direction the party is facing can change target access. The fight runs from the party's actual position at contact. It does not reposition the party to line up a shot.

| Participant | Final profile | Use in play |
| --- | --- | --- |
| Alden, Brunna, Merrin and Lethiel | Standard fighter, cleric, rogue and wizard, each advanced to level 2 at 300 XP | Ordinary formation, weapons, ammunition, HP, class features and spell resources. Brunna can prepare Inflict Wounds and Guiding Bolt. Lethiel has Magic Missile and Fire Bolt. |
| Broodmother | AC 13, 124 HP, speed 30 ft, +5 to hit, 2d6+3 piercing | Pursues on sight or when a sac is attacked. Her bite has a registered windup and impact. Killing her stops hatching. |
| Each of two clutches | AC 10, 6 HP | A bow or Fire Bolt can break sealed or stirring silk. Melee reports “Above melee reach.” A covered sac takes Shatter damage once with no save. Creature-only spells cannot select it. |
| Vault young | AC 11, 7 HP, speed 40 ft, +5 to hit, 2d4+2 piercing | Uses Pack Tactics and the DC03 venom save, DC 12 against two victim turns of Poisoned. When other bodies block its melee approach it can flick barbed bristles for +5 to hit and 1d6+2 piercing. Only the bite carries venom. An intact sac replaces a slain young. |

The level-two party uses the ordinary advancement and preparation rules. Monsters use authored stat blocks. The first proposed 40-HP mother died in round two before a young could attack. Raising her HP alone also allowed clean openings. A doorway volley then exposed a second weakness. Young could spend their attack opportunities behind other bodies at the entrance. Each drops on the aisle side of its sac. Give it a weaker ranged bristle attack through the ordinary ranged attack, line-of-sight and projectile rules so holding the doorway has a cost. The tuning brings the first hatch forward and gives young an accurate pack bite that matters against the front rank.

## State and ordering

- During floor preparation, `placeBroodVault` binds five slots from the bounded actor roster. One is the mother. Each side gets one ceiling target and one dormant young. Incomplete or duplicate bindings and illegal or coincident hatch reservations fail preparation. A floor must have space for all five entries.
- The session's combat controller owns two clutch records with a sac handle, a reserved child slot, its original spawn placement, a warning round and a player-opportunity flag. The west sac starts stirring at the start of round one. The east starts in round two. Each remains open to attack until a later round boundary, after at least one conscious party member has received a normal action. A surviving sac with a slain child starts this cycle again. With both children alive, hatching pauses.
- The session drains due impacts before updating the round. A lethal hit on the mother or sac wins the tie with a hatch. An occupied hatch cell, including the party's own cell, defers the hatch to a later boundary. A new child faces the party and enters through ordinary group initiative and movement. It needs melee reach to bite or a clear shot to flick its bristles.
- Each successful birth increments a checked slot generation. The actor registry rejects stale handles, including incoming shots and selected targets. It clears the previous incarnation's pose, conditions, navigation and encounter membership before enrollment. Generation exhaustion leaves the slot unavailable. The encounter holds at most two living young and allocates no new actor storage in play.
- A mother death silences intact sacs and cancels outstanding warnings. Live young still fight. The objective tracks the mother and the current child identities and issues **200 party XP once** after their deaths with a surviving party. The vault awards no per-kill XP or loot. Cleared, intact sacs remain visible and inert. Broken sacs keep their torn silk.

### Presentation and interaction

The painted mother has four registered directional views and a distinct initiative portrait. The small young reuse the teaching spider artwork and spider sounds. Three silk poses show sealed, swollen and torn sacs. During a warning the sealed and swollen poses alternate every 340 ms. The combat log names the side, the picker reports Stirring, and hovering over the sac explains its reach and HP.

A sac is a destructible world body using the existing actor depth and pixel picker. It consumes a target slot and no ground occupancy or creature initiative. The controller checks action readiness, sight, range, attack rolls and ammunition or spell access through the normal command path. Both world clicks and picker rows resolve the same generation-checked identity. Area spells enumerate occupied subcells once. Art registration and texture preparation run offline.

## Player review

1. Run `./review-caves --depth 6 --seed 5 --encounter`. Each launch starts fresh at the cave threshold. Use **M** for the map, **W/S** to move forward or back, **A/D** to strafe, **Q/E** to turn, and **F** to open a door. Take the northern gallery east, then turn south into the final webbed room. The recorded aisle opening is **(47,12) facing south**. The mother starts at (47,15), with sacs two cells to either side. The forced fallback has its own positions.
2. Leave Alden and Brunna in front. On Merrin's turn use **3** for his shortbow. On Lethiel's turn choose **Fire Bolt** from the spell book (**4**). Aim at the western hanging sac as it stirs, then the eastern one. When facing south, west is on the player's right. Check the named log warning, the pulsing silk and the target row while a character can still act. A miss spends the attack and leaves the sac intact.
3. Relaunch with the same seed and ignore both sacs and young. Spend Action Surge, Brunna's offensive spells, Merrin's shortbow and Lethiel's Magic Missile on the mother. Watch when a young first bites, who goes down, and whether surviving young keep the fight open after the mother dies. For the pre-contact version, begin shooting while she is still approaching.
4. On a fresh run, break only the eastern sac. Kill the western young and leave its sac intact through another warning. After its replacement appears, break the remaining sac and finish the fight. The reward should appear once. Add `--fallback` to repeat on the fixed layout.

## Balance and exit checks

The review runner starts every strategy from the threshold with the same seed, level-two party and resources. It walks doors and cells and submits legal player commands. The suppression route assigns bow and Fire Bolt to the sacs. The contact burst spends Action Surge and offensive spells on the mother. Two further focus-fire runs open with the shortbow, Guiding Bolt and Magic Missile before contact. One holds the same aisle position. The other stays two cells farther back in the northern doorway. These focus-fire checks fail if the young cannot attack before the mother falls, or if the young fail both to down a party member and to deal at least 20 damage while she lives. This threshold is a regression criterion for the scripted openings. Player judgment governs whether the cost feels right.

| Level-two opening | Seed 5 | Seed 6 | Seed 7 |
| --- | --- | --- | --- |
| Shoot both sacs | Won round 5, no downs | Won round 6, one down | Won round 6, two downs |
| Focus the mother at contact | Defeat, 42 brood damage | Defeat, 50 brood damage | Won, 25 brood damage |
| Fire the three opening shots in the aisle, then focus the mother | Defeat, 37 brood damage | Won, 37 brood damage | Defeat, 34 brood damage |
| Fire from the doorway and hold position | Won, 11 brood damage | Won, 27 brood damage | Defeat, 44 brood damage |
| Kill and replace the western young, then break its sac | Won round 7 | Won round 8 | Won round 7 |

Brood damage in the table counts attacks resolved **while the mother was alive**. “Down” means a character fell unconscious. All nine focus-fire runs saw their first hatch and first young attack in round two. In each of the four wins, the brood downed a companion before the mother fell. Five runs ended with all four characters down. Both sacs broke before anything hatched in all three suppression runs. Each replacement run produced two generations from the western sac and paid the same 200 total XP. Forced fallback on seed five reproduced all five outcomes at its own aisle and doorway positions.

The decisive doorway counterexample was seed seven. Before young had a ranged attack, three opening shots let the party kill the mother in round three, win and take zero brood damage. In the same opening the final young fired six bristle shots, dealt 44 damage and downed two characters before the party lost. The mother's HP remained at 124 through this correction.

### Verification commands

From the repository root, these checks exercise the shipped party, cave layout and combat rules.

```sh
./review-caves --depth 6 --seed 5 --encounter --check
./review-caves --depth 6 --seed 6 --encounter --check
./review-caves --depth 6 --seed 7 --encounter --check
./review-caves --depth 6 --seed 5 --encounter --fallback --check
./review-caves --depth 5 --seed 5 --encounter --check
```

The depth-five dependency walk passed both bank openings. The configured Debug build produced the game, review executable and registered world assets. The publication contains 684 textures, 956,588,968 mip bytes and 68,820,433 alpha bytes. Focused CTest suites ran serially using the configured CLion toolchain. The relevant suites were `brood_vault`, `targeting`, `impact`, `actors`, `spider`, `newt`, `level_spells`, `rules_content`, `cave_layout`, `automation`, `runtime_inventory` and `campaign_save_smoke`.

The isolated vault test covers a lethal sac or mother hit immediately before the hatch boundary, a blocked cell, a dead child's stale identity, an interrupted mother bite, a Fire Bolt whose target breaks in flight, and one Shatter hit without a creature save. It verifies sword access to the mother, rejection on the ceiling target, exclusion from Web, the actor limit and the once-only reward after the last young dies. The allocation guard covers engine-owned updates across warnings, blocked retries and replacement.

The hidden Debug renderer repeated all five seed-five routes using visible target pixels. It round-tripped a pixel on the western sac through picking and the shortbow selector while the sac was stirring. The saved frames show the opening warning, Merrin’s eligible shot, and both living young. OpenGL reported no error and renderer resources returned to baseline. To repeat the capture, create a directory with `python3 tools/agent_artifacts.py create`, then run the following from the checkout with its path in `run_dir`. Preview and apply cleanup after inspecting the frames. [Native encounter views](screenshots/damp-caves-dc05/README.md).

```sh
nice -n 15 ionice -c 3 cmake-build-debug/blobber_review_caves capture 6 5 "$run_dir/vault" assets cmake-build-debug/assets/prepared --encounter
```

## Player acceptance

The player approved the encounter on 2026-11-19 after finding it fairly easy even with both sacs intact. Each sac can sustain one living hatchling, for a maximum of two. Only a hatchling’s bite can poison a party member.

If later play calls for a harder fight, the player’s preferred option is to let intact sacs keep hatching while earlier young are still alive, until the sacs break or the mother dies. The player prefers to keep poison off the mother because repeated poisoning over a long fight could exhaust potions and mana. Further tuning is deferred.

## Handoff and limitations

DC06 can place the five-slot roster with `generation::placeBroodVault` and the encounter form of the depth-six layout. Budget a single 200-XP objective and zero repeatable rewards. Allocate all other populations around the two legal hatch cells. DC07 must reconstruct the mother's life, each sac's integrity or dormancy, each child's generation, life and position, and the objective and issued reward. A settled checkpoint must exclude active combat, an unresolved warning or an in-flight hit. Both later slices remain planned. The playable review is standalone and restarts at the threshold.

These are deterministic openings on three seeds and a checked fallback. Critical rolls, initiative order, formation, equipment, healing and a different route can change the outcome. The focus-fire scripts hold position after the opening volley and spend cleric resources on offense. Bless and Cure Wounds were outside that comparison. They do not measure prolonged kiting or the balance of an equipped campaign party arriving from floor five. Player review covers warning readability in motion, whether the hatchlings merit a change of target, guardian pacing and the sound mix on speakers. The level-two review party has no Shatter spell slot. Its object interaction is exercised in a separate higher-level fixture.

New monster rules change the shipped catalog fingerprint. Existing campaign saves with the previous fingerprint follow the established read-only incompatibility check. The standalone review uses fresh sessions and does not write to campaign saves.
