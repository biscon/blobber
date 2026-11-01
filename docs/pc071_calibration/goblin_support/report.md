# Arena playtesting report

Batch status: **complete** (all_planned_trials_recorded).
Planned 6; started 6; unstarted 0.

This is an exploratory policy comparison, not a target that every party should win.
Scenario-declared levels and spell choices; fixed starter gear; no rest, loot or advancement during trials.
Information: **diagnostic**. Real-time party action interval: 250 ms; initiative decisions have no added delay.
Diagnostic mode knows all enemy positions/HP. Visual mode uses prepared body picking. Preview is a zero-time legality oracle.
Seeds are shared across configurations; different actions need not consume the same rolls.

| Party | Roster | Policy | Started / planned | Valid W / L | Win rate (95% Wilson) | Other outcomes |
| --- | --- | --- | --- | --- | --- | --- |
| balanced | goblin_support | aggressive_v2 | 3 / 3 | 3 / 0 | 100.0% (43.9%–100.0%) | none |
| no_wizard | goblin_support | aggressive_v2 | 3 / 3 | 3 / 0 | 100.0% (43.9%–100.0%) | none |

Win-rate denominator is valid wins + combat losses only. Completion rates, wins/started and every failure count are also in summary.json.
Casualty, remaining/consumed-resource and duration statistics in summary.json include valid wins and losses; unfinished runs are separate in trials.csv/jsonl.

| Comparison (alternative − baseline) | Roster | Policy | Valid pairs | Difference | Uncertainty |
| --- | --- | --- | --- | --- | --- |
| no_wizard − balanced | goblin_support | aggressive_v2 | 3 | 0.0% | fewer than 20 pairs; preliminary |

Party packages (whole-party comparisons; race, gear and resources can differ):

- **balanced**, front-to-back: human fighter, hill_dwarf cleric, lightfoot_halfling rogue, high_elf wizard.
  - Slot 0 level 1: abilities [16, 14, 15, 9, 13, 11], HP 12, AC 19, mana 0; items {"camp_kit":3,"chain_mail":1,"healing_potion":2,"longsword":1,"shield":1}; spells none.
  - Slot 1 level 1: abilities [13, 10, 16, 8, 16, 12], HP 12, AC 16, mana 4; items {"healing_potion":2,"holy_symbol":1,"mace":1,"mana_potion":2,"scale_mail":1,"shield":1}; spells bane, bless, cure_wounds, guidance, guiding_bolt, inflict_wounds, sacred_flame, shield_of_faith, spare_the_dying.
  - Slot 2 level 1: abilities [8, 17, 14, 12, 13, 11], HP 10, AC 14, mana 0; items {"arrow":20,"healing_potion":2,"leather":1,"shortbow":1,"shortsword":1}; spells none.
  - Slot 3 level 1: abilities [8, 16, 13, 16, 12, 10], HP 7, AC 13, mana 4; items {"arcane_focus":1,"dagger":1,"healing_potion":2,"mana_potion":2,"spellbook":1}; spells acid_splash, burning_hands, fire_bolt, mage_armor, magic_missile, ray_of_frost, shocking_grasp, thunderwave.
- **no_wizard**, front-to-back: human fighter, hill_dwarf cleric, lightfoot_halfling rogue, lightfoot_halfling rogue.
  - Slot 0 level 1: abilities [16, 14, 15, 9, 13, 11], HP 12, AC 19, mana 0; items {"camp_kit":3,"chain_mail":1,"healing_potion":2,"longsword":1,"shield":1}; spells none.
  - Slot 1 level 1: abilities [13, 10, 16, 8, 16, 12], HP 12, AC 16, mana 4; items {"healing_potion":2,"holy_symbol":1,"mace":1,"mana_potion":2,"scale_mail":1,"shield":1}; spells bane, bless, cure_wounds, guidance, guiding_bolt, inflict_wounds, sacred_flame, shield_of_faith, spare_the_dying.
  - Slot 2 level 1: abilities [8, 17, 14, 12, 13, 11], HP 10, AC 14, mana 0; items {"arrow":20,"healing_potion":2,"leather":1,"shortbow":1,"shortsword":1}; spells none.
  - Slot 3 level 1: abilities [8, 17, 14, 12, 13, 11], HP 10, AC 14, mana 0; items {"arrow":20,"healing_potion":2,"leather":1,"shortbow":1,"shortsword":1}; spells none.

Resolved damage and healing (not net HP loss; damage may include temporary HP/overkill):

| Party | Roster | Member | Incoming | Largest hit | Max / 6 s | Max / round | Healing |
| --- | --- | --- | --- | --- | --- | --- | --- |
| balanced | goblin_support | 0 | 0/0/4 | 0/0/4 | 0/0/4 | 0/0/4 | 0/0/0 |
| balanced | goblin_support | 1 | 0/0/5 | 0/0/5 | 0/0/5 | 0/0/0 | 0/0/0 |
| balanced | goblin_support | 2 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 |
| balanced | goblin_support | 3 | 0/0/4 | 0/0/4 | 0/0/4 | 0/0/0 | 0/0/0 |
| no_wizard | goblin_support | 0 | 0/0/5 | 0/0/5 | 0/0/5 | 0/0/5 | 0/0/0 |
| no_wizard | goblin_support | 1 | 0/4/8 | 0/4/8 | 0/4/8 | 0/0/4 | 0/0/0 |
| no_wizard | goblin_support | 2 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 | 0/0/0 |
| no_wizard | goblin_support | 3 | 0/0/4 | 0/0/4 | 0/0/4 | 0/0/0 | 0/0/0 |

Damage cells are min/median/max across valid completions. Six-second windows are (t−6000ms,t]; contact round bursts reset on started/round events. Overflow is flagged in each trial.
Approach phase duration is sampled at response boundaries; first-contact event time and total simulation duration are retained separately. HP/downing transitions remain observed lower bounds.

Coverage is recorded per trial/group. Focus deliberately preserves offensive mana; other policies use only their documented supported subset.
Unsupported active abilities stop a trial. A bot failure or stall is never counted as a combat loss.
Full coverage, resource distributions, action counts and paired sample sizes are in summary.json.

Total trial wall time: 1.118 s (includes startup and failed trials).
Completed trials/min: 322.049; simulated seconds/wall second: 155.523.
These measurements include Python/IPC/recording and are not pure simulation benchmarks. report_seconds is separate in summary.json.
See provenance.json for executable/content/code identity. trial_index.json lists retained artifacts and exact replay/rerun commands.
No experiment is registered in the routine test suite.
