# Path of the Berserker through level four

The Barbarian is the eighth playable class. It has a d12 Hit Die, Strength and Constitution save proficiency, and training in simple and martial weapons, light and medium armor, and shields. The creator offers a greatsword or a battleaxe and shield. Both kits carry a shortbow, 20 arrows and a healing potion in the bag. The bow can be equipped using the normal hand-compatibility rules.

| Level | Rages per long rest | Features gained |
| --- | --- | --- |
| 1 | 2 | Rage and Unarmored Defense |
| 2 | 2 | Reckless Attack and Danger Sense |
| 3 | 3 | Path of the Berserker and Frenzy Strike |
| 4 | 3 | Ability Score Improvement |

## Combat decisions

**Rage** uses a bonus action and lasts 60 gameplay seconds. A round advances that clock by six seconds, and time spent picking an action does not use up Rage. Activation ends the Barbarian’s concentration and blocks spellcasting, including racial spells. Rage ends early on unconsciousness, sleep, heavy armor or deliberate dismissal. Dismissal costs a bonus action. A short rest leaves spent uses intact.

During Rage, Strength-based melee hits deal two extra damage, including on a critical. Strength saves and the Web escape check gain advantage. Incoming slashing, bludgeoning and piercing damage is halved at impact, rounding down per component. The damage resolver then applies temporary HP and HP loss once for the whole hit. Danger Sense grants advantage on Dexterity saves against perceptible dangers while conscious and able to see. These advantages use ordinary disadvantage cancellation. With an empty body slot, Unarmored Defense adds Constitution to Dexterity for AC. The game takes the best applicable base, including ordinary unarmored AC and Mage Armor, then applies shield and equipment bonuses once.

**Reckless Attack** is a choice on the first weapon attack of the turn. It grants advantage on eligible Strength melee attacks that turn, including a later Frenzy Strike. Enemies get advantage against that Barbarian until the Barbarian’s next turn. Ordinary melee in the action bar remains an ordinary swing. The Abilities picker also offers the ordinary and Reckless options together.

**Frenzy Strike** gives a Berserker one extra melee swing per Rage. Starting on a later turn, the player can spend their bonus action before or after their ordinary attack, pick a different legal target, or keep the swing for another round. Reckless Frenzy combines the two decisions when Frenzy is the first attack. A miss spends the swing. Ending Rage discards it. This adaptation has no exhaustion penalty.

## State and presentation

Each party member has fixed-size state for spent Rage uses, the Rage and Reckless deadlines, whether a weapon attack has been made this turn, and the unspent Frenzy swing. Combat owns bonus-action readiness. All melee variants go through normal target and formation checks, weapon rolls, criticals, attack presentation and the Bard decision path. Condition and damage rules read the recipient's live state. The portrait badge reports Rage uses or time remaining. Its tooltip explains Frenzy readiness and Reckless exposure.

The catalog uses schema 20. Character-library and arena blueprints use existing class and gear identifiers. Campaign schema 27 and simulation 29 preserve Barbarian state, and floor travel rebases deadlines against the destination gameplay clock. Older campaign saves require their matching build. Headless observations and commands use the same class and combat rules. [Headless protocol](headless_protocol.md#barbarian-actions)

## Verification

The Debug game and headless client built with the checkout's configured CMake and Ninja. The following checks passed on Linux.

```sh
ctest --test-dir cmake-build-debug -j1 --output-on-failure -R '^(damage|impact|creation|recovery|hud|runtime_inventory|arena|automation|library|progression|progression_allocations)$'
cmake-build-debug/blobber_campaign_save_tests --barbarian
```

The checks exercised both weapons and shield AC, Rage duration and restoration, mixed damage before temporary HP, visible and blinded saves, loss of concentration and racial casting, and bonus-action debt carried into initiative. Combat tests spent Frenzy before and after the ordinary attack, held it across rounds and resumed a Bard interrupt. Campaign checks restored active Rage across save/load and floor travel, and rejected inconsistent character state. The allocation check observed zero engine-owned C++ allocations in the measured combat workload.

The hidden native review uses the shipping catalog, both starting kits and the player command controller. It activates Rage, waits for a later turn, selects Reckless Attack, inspects the portrait badge and spends Frenzy against a second enemy. Run `blobber_world_integration_gpu_tests assets <new-output-directory> --barbarian-review cmake-build-debug/assets/prepared` in the graphics build. Interactive playtesting and balance acceptance are pending. [Native captures](screenshots/barbarian/README.md)
