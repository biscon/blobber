# DC06 · Population, rewards and floor pacing

**Accepted and closed by the player on 2026-11-19.** The [roadmap](damp_caves_plan.md) authorizes standalone floors. [DC05](dc05_broodmother_design.md#handoff-and-limitations) supplies five reserved actor slots and a single 200-party-XP objective. Campaign travel, save-slot admission and final victory belong to DC07.

## Encounters and rewards

### Galleries balance trial

**Earlier trial, 2026-11-20.** The [next balance revision](damp_caves_pressure_design.md) supersedes the route totals and Vault reward timing below. The player's level-three and level-four parties cleared all three floors comfortably with one long rest. The 1,350 XP popups came from encounter-completion awards introduced to meet the original pacing target. All three floors now pay ordinary kill XP. The Vault still pays 200 party XP once for defeating the mother and her surviving brood, with zero individual XP for sacs and renewable young.

Only the fourth ordinary Galleries encounter changes composition. The three venomous spiders are joined by two Adult Web-Spinners, each worth 200 XP with AC 14, 26 HP, a venomous bite and a ranged silk attack. At a distance an adult with a free strand favors silk. Each adult can bind one conscious companion. The victim can spend an action on **Escape Web** (DC 12 Strength), or an ally can drop the spinner. The generator owns the fixed roster and back placement. Combat tracks at most four strands by victim and source actor, exposes restraint through the existing conditions display, and releases strands at encounter end. Adults reuse the spider artwork and have a distinct name. The trial uses the existing potion, camp and boss rules.

| Four surviving companions, ordinary route | Seep entry | Galleries entry | After Galleries scouts | Vault entry | After the brood |
| --- | ---: | ---: | ---: | ---: | ---: |
| XP each | 675 | 850 | 925 | 1,150 | 1,437.5 |
| Level available | 2 | 2 | 3 | 3 | 3 |

Clearing every optional group from the 793.75-XP full-clear entry gives 1,768.75 XP per survivor at victory. Level four requires 2,700. With four survivors the revised caves support level three, including on the full clear.

**Matched route trial.** Run `cmake-build-debug/blobber_review_caves attrition 5 5 assets` from the checkout root. The default fighter, cleric, rogue and wizard start at The Seep, collect the ordinary supplies, fight all five ordinary Seep encounters and carry the party down to the Galleries. They use Hit Dice, drink from inventory between fights and consume a camp kit if still badly hurt or low on mana. Both runs used one kit between the divided banks and scouts. The party advanced after the scouts and reached the nest with 86/99 HP, 22/28 combined mana, three camp kits, six healing potions and zero mana potions. For the control, the runner removes just the two adults before contact, without awarding XP or advancing time.

| Seed-five nest result | Three small spiders | Three small spiders + two adults |
| --- | ---: | ---: |
| Victory round | 2 | 5 |
| Enemy attacks / damage dealt | 3 / 0 | 14 / 50 |
| Silk bindings / party downs | 0 / 0 | 2 / 1 |
| Combined mana after the fight | 18 | 6 |
| Party HP after the fight | 86 | 41 |

One deterministic run with a spell-heavy policy measures the cost for this party. It does not estimate a win rate. The script prioritizes visible targets in roster order, uses between-fight recovery and never chooses Escape Web. A player who targets adults early, saves spells or takes a different route may reach the nest in a very different state. This comparison ends after the nest.

**Player review.** Launch `./cmake-build-debug/blobber --development --seed 5 --campaign-save output/galleries-trial.json`, choose **Start new game → your party → Choose floor → 4. The Seep**, then descend normally. Use supplies and rest when you choose. On floor five, pass the lone newt, the divided banks and the goblin scouts. The nest is next, around (31,16). Watch who gets bound, whether killing an adult frees that companion, and whether you spend a kit before the next fight. A direct Galleries start gives a rested party. The new monster changes the catalog fingerprint, so start a fresh expedition for this version.

**Verification.** The seed-five populated checks for depths 4–6 cover ordinary and optional earnings, guardian gates, shortest routes and Vault completion. The paired attrition command exercises the new roster, real silk hits, carried supplies and recovery. All five focused Debug suites passed with `ctest --test-dir cmake-build-debug -j1 --output-on-failure -R '^(spider|cave_layout|damp_campaign|campaign_save_smoke|campaign_save_cli)$'`. The Debug game built. Interactive review is pending.

### Original DC06 review figures

The ordinary route visits the named encounters in order, skipping both optional rooms. Numbers below show group size, ordinary kill XP and the once-only survey encounter bonus in party XP. Individual spiders and newts retain their accepted 50 XP. Goblin groups use the shipped melee, archer, captain and shaman. Shooting a sac or replacing a young pays zero.

| Floor | Ordinary groups in order (size, kill + bonus) | Guardian | Optional pocket and lookout |
| --- | --- | --- | --- |
| Seep | Arch spider (1, 50+0), scavengers (3, 150+225), spider nest (3, 150+450), captain's patrol (3, 150+1225) | Webbed guard (4 spiders, 200+2000) | Web shortcut (2 spiders, 100+275), goblin lookout (3, 150+0) |
| Drowned Galleries | Solo newt (1, 50+0), divided banks (2, 100+950), goblin scouts (3, 150+1350), spiders (3, 150+1350), two newts with spider (3, 150+1350) | Two newts and two spiders (4, 200+1800) | Three cave beasts (150), three goblins (150) |
| Brood Vault | Spiders (3, 150+1250), scavengers (3, 150+1650), three spiders and newt (4, 200+1800), captain with two archers and shaman (4, 200+2200), three spiders and two newts (5, 250+2550) | DC05 mother, sacs and young (5 slots, 200 objective) | Three spiders (150), newt and two spiders (150) |

The Seep, Galleries and Vault have 7, 8 and 8 encounters respectively. They use 19, 22 and 30 of 32 placements. The largest group uses five of six member slots. For comparison, seed-five Underkeep floors use 18, 19 and 22 placements, including guardians. The Vault reserves four of its slots for sacs and young. Ordinary party XP is 4,600/7,600/10,600. Optional party XP is 525/300/300. Each ordinary approach supplies a healing potion, a mana potion and ten arrows. The two optional locations on each floor hold an extra antitoxin and a healing potion between them. Each of the first two guardians drops one camp kit. The Seep camp has an additional guaranteed antitoxin before contact. Random goblin drops are extra.

| Four-survivor milestone | 675 entry | 793.75 entry |
| --- | ---: | ---: |
| First spider | 687.5 | 806.25 |
| Second ordinary encounter | 781.25 | 900 |
| Third ordinary encounter | 931.25 | 1050 |
| Seep guardian | 1825 | 1943.75 |
| Fourth Galleries group | 2850 | 2968.75 |
| Galleries guardian | 3725 | 3843.75 |
| Vault guardian, ordinary | 6375 | 6493.75 |
| Vault guardian, all cave optionals | 6656.25 | 6775 |

Level three becomes available on Seep encounter three or two respectively. Level four becomes available on Galleries encounter four on the ordinary routes. The party advances manually outside danger. The party stores earned XP beyond 2,700 with twelfths preserved, including above 6,500. Level four is the final playable level.

## Ownership and placement

During preparation, the cave generator selects stable room sites from the connected final geometry. Prefer the intended route through the exploration network and use local doors to isolate roaming, noise and long lines of sight. Keep two avoidable side pockets, both pool banks, all five vault positions and protected arrivals. Validate fallbacks under the same placement rules. The population resolver binds catalog IDs and constructs the same NPC world, finite recovery placements and floor objective definition for the GUI and headless sessions. The floor owns its immutable encounter ranges. Award the bonus on the transition from incomplete credited deaths to complete membership. Snapshot death credits are sufficient for DC07 to reconstruct consumed group payouts. The player sees the clear and bonus in the combat log. No frame-time population construction.

Keep the first arch after the camp and the lone newt before the divided banks. Lock the exit on the far side of the guardian encounter. The vault issues its 200 XP only after mother and living young die. A sealed gate opens once and cleared floors can be backtracked. Preserve Underkeep generation, old standalone proof encounters and the three-floor campaign ending.

## Verification and review

The Debug build used the configured CLion CMake and Ninja, at most two build workers and serial tests. The following checks passed on 2026-11-19.

| Check | What it verified |
| --- | --- |
| `./review-caves --depth D --seed 5 --populated --check` for 4, 5 and 6, repeated with `--fallback` | Ordinary and full-clear entry parties, quiet camp and isolated contacts, optional detours, finite supplies and XP, guardian gate, return to the entrance, shortest-path earnings and advancement. The level-four sheet showed 6,775 banked XP on the full clear. |
| `ctest --test-dir cmake-build-debug -j1 --output-on-failure -R '^(brood_vault\|newt\|progression_allocations\|progression\|automation\|cave_layout\|guardians\|campaign_save)$'` | All eight suites passed. The focused allocation check also passed with the added encounter completion and log refresh. |
| `cmake-build-debug/blobber_campaign_save_tests --progression` | The level-four party round-tripped 6,725.25 XP through the existing checkpoint codec. |
| `./review-caves --depth D --seed 5 --encounter --check` for 4, 5 and 6 | The original isolated arch, newt and Brood Vault fixtures still passed at their original levels. |
| `ctest --test-dir cmake-build-debug -R '^population$' -j1` | Underkeep population binding passed after sharing the actor resolver with the caves. |
| `cmake-build-debug/blobber_generate_floor --seed 5 --depth D --format json` for 1, 2 and 3 | Before/after generation fingerprints, geometry, fixtures and rosters matched. The only report changes were generation time and 32 additional bytes in the retained-memory and world-definition diagnostics. |
| `cmake-build-debug/blobber_review_caves capture D 5 "$run_dir/review-D" assets cmake-build-debug/assets/prepared --populated` for 4, 5 and 6 | Three hidden native 1920×1080 guardian views and matching encounter plans inspected. Replace both occurrences of D with the depth and use a fresh output directory. The six selected files are in the review gallery. Disposable captures, export directories and logs were cleaned. |

### Player review

From the checkout root, launch each floor separately. Each launch starts at its entrance with a fresh, fully rested Alden, Brunna, Merrin and Lethiel. **M** opens the discovery map. Use **W/S** to walk, **A/D** to strafe, **Q/E** to turn and **F** to open a door. Use the ordinary ground-loot and inventory controls. When the level-up reminder appears, choose each eligible companion in the character sheet and advance them at a quiet spot. [Encounter maps and guardian views](screenshots/damp-caves-dc06/README.md)

| Launch | Seed-five route and decisions |
| --- | --- |
| `./review-caves --depth 4 --seed 5 --populated` | Open the entrance door and collect the camp antitoxin before the webbed arch. The single spider is at (14,14). Continue east to the goblins around (22,14) and the nest at (30,15). Turn north toward the captain at (37,4). The webbed shortcut north of the arch and the southeastern lookout each hold an optional fight and supply. The four guardian spiders form two flanks behind the approach door. |
| `./review-caves --depth 5 --seed 5 --populated` | Collect the entrance supplies. Meet the solo newt at (5,15). Try **Defend** on the character named in its warning. From (11,15) face east and choose the roped north bank or the south bank. Replay the other bank with the same opening. Continue through the scouts, spider nest and flooded crossing. In the last room two newts cover the flanks while a spider starts forward. |
| `./review-caves --depth 6 --seed 5 --populated` | Start at level three. East leads to the outer spiders. North from the first chamber leads to the optional survey cache. The ordinary route crosses the galleries to the goblin sentries, then a five-beast guard at (46,5). After that fight, use the quiet approach to prepare for the mother. From (47,12) facing south, try shooting the sacs with Merrin and Lethiel. On a fresh launch focus the mother and watch the surviving young and the gate. |

Add `--full-clear-entry` to bring in all four survivors' XP from every earlier optional encounter. The ordinary entry presets are 675 / 850 / 1,150 XP at levels 2 / 2 / 3. Full-clear entry is 793.75 / 1,031.25 / 1,406.25 XP at levels 2 / 3 / 3. Add `--fallback` for fixed geometry or `--check` for the headless review. The full-clear-entry flag models previous XP. The equipment and supplies are controlled examples. Alden wears a cloak of protection and Merrin wears boots of balance. They use the preset weapons. Floors five and six start with one additional kit from the previous guardian. Review preparation uses the ordinary advancement rules, recommended spells with Inflict Wounds and Magic Missile preferred when available, and +2 Constitution at level four. In-floor advancement remains a player choice. Cleared exits admit the party to the objective landing. Return by walking back to the up stair.

### Measured pacing and encounters

The initial room-graph placement put two ordinary groups on longer paths. On seed five the shortest walk missed the Seep nest and a Vault pack, costing 2,600 party XP. The final generator takes a cardinal shortest path over the dressed floor with the two optional rooms excluded. It places network encounters at the first gallery, the gallery closest to the midpoint in walking distance, and the outlet. The Seep middle group moved to room 10 and the Vault middle group to room 11 on seed five. The generator retains the three exploration circuits. A party choosing other passages can bypass fights and earn less. The two optional encounters remain behind local doors.

| Seed-five measurement | Seep | Galleries | Vault |
| --- | ---: | ---: | ---: |
| Ordinary steps to first contact, including supplies | 25 | 14 | 21 |
| Longest subsequent ordinary approach between contacts | 20 | 17 | 17 |
| Shortest walk to guardian contact, normal / forced fallback | 78 / 78 | 72 / 72 | 75 / 77 |
| Ordinary XP on that walk, normal and fallback | 4,600 | 7,600 | 10,600 |

Counts measure completed cardinal moves. Doors, turning, combat and the return to the entrance consume additional time. The full-clear fixture visits both side rooms late to prove the party can leave them alone through the ordinary fights. On seed five that order can retrace over 50 steps between encounters. The webbed shortcut can instead be explored immediately after the arch. The optional two-spider clear also takes a 793.75-XP entrant to exactly 900. Reaching both side rooms early can shift later level-up milestones.

| Equipped-party opening | Seed-five result |
| --- | --- |
| Seep guardian, level three | Victory in round two, three spell casts, zero incoming damage. The forced fallback also took two rounds. This is the lightest of the three guardians. |
| Galleries teaching newt, level three | Alden had AC 20 and 28 HP. Repeating the warned bite until a hit, Defend left him at 21 HP. On the unanswered opening the first landed bite left him at 15 HP. The accepted +4, 3d6+3 attack can remove nearly half his HP on an ordinary hit. Each experiment started afresh. |
| Galleries banks | With identical solo-fight choices on the two walks, the first legal melee target at (16,13) was the spider. At (16,17) it was the newt. The fallback stances were one row north. The south opening exercised both the rogue's shortbow and the wizard's Fire Bolt. Taking longer in the solo fight changed the initiative sequence and sometimes allowed both targets by the first melee opportunity. |
| Galleries guardian, level four | Victory in round three with six spell casts and 18 incoming attack damage, no downs. The fallback took two rounds, four casts and no incoming attack damage. |
| Vault guardian, level four | Both openings started at the same northern aisle position. Suppression broke both sacs, won in round five and took no brood damage. Mother focus won in round four. Two young hatched and began attacking in round two, dealing 15 damage before the mother died. Neither run downed a character. Both paid 200 total party XP, including the fallback. |

The actor, AI, fixture, loot, objective and XP paths run normally in the route checks. Ordinary route fixtures resolve deaths directly after observed contact, then collect supplies through inventory transactions. The Seep and Galleries ordinary-route guardians use legal attacks from the party at its earned level. The Vault tactic comparisons use the same populated floor geometry, reserved guardian roster, gate and a fresh level-four party. Earlier fixture fights do not deplete the party's resources as a player's fights would. These openings establish target access and example costs. Human judgments still needed include whether the Seep guardian is too gentle, whether two heavy bites are readable together, whether to spend a kit before the Vault and whether the long optional detours pay enough. Revisit placement first if enemies merge or exploration regularly becomes a long empty walk.

## Player acceptance

The player approved DC06 on 2026-11-19. The demo launches each floor separately, and the player reported starting at level two on each launch. They deferred judging progression across all three floors until they can descend with the same party in the integrated campaign. The XP milestones above are measured fixture results.

### Runtime and DC07 handoff

The launcher binds the generated roster through ordinary monster appearance and rules resolution. All uncollected stacks and reserved camp-kit drops live in the floor loot owner. The five vault slots retain DC05 identities, two live young at most and zero repeatable rewards. The gate occupies the exit landing. Rock shoulders prevent a side approach to the stair or endpoint. Defeating the guardians opens the gate and makes the reserved kit available in their room. The vault opens its gate on the same completion that pays its 200 XP.

Planning reuses two fixed 1,600-entry 16-bit arrays for route reconstruction. The eight encounter records and 32 placement slots stay in the floor layout. The objective owns up to eight named bonuses with contiguous member ranges. Only newly credited deaths can complete a range. Death and completion share the existing survivor split. The log and headless encounter-clear event expose the bonus. Live reward reconciliation uses bounded stack storage and the existing event journal.

DC07 must regenerate the identical initial roster and named ranges, then restore death credits together with party XP, loot, door state and guardian completion. It also needs the DC05 mother, sacs, child generations and settled-checkpoint rules. DC06 uses the existing party XP fields and their twelfths, with wide arithmetic and saturation at the representable integer maximum. Current three-floor checkpoints keep their existing admission rules. Caves do not enter New Campaign, Continue, recall or location-ID mapping in this slice.
