# PC07.1 measured encounter costs

2026-11-01. Diagnostic open-arena sample: three fixed seeds per party/recipe; all 36 completed as victories. Three observed downings, zero permanent deaths, zero incomplete burst metrics. Every figure below is observed evidence, not a win-rate or expedition guarantee.

Balanced = Fighter/Cleric/Rogue/Wizard; no_wizard = Fighter/Cleric/Rogue/Rogue. Starting builds, spell choices and initial/final per-character resources were recorded in trial journals; compact summaries and provenance remain. All start with ordinary full resources and starting gear.

| Recipe / level / total XP | Party | Incoming damage, total | Gross mana spent | H / M potions used | Completed rounds | Simulation seconds |
| --- | --- | --- | --- | --- | --- | --- |
| goblin_pair / L1 / 100 | balanced | 0 | 6 | 0 / 0 | 0–1 | 16.55–23.35 |
| goblin_pair / L1 / 100 | no_wizard | 0–7 | 2 | 0 / 0 | 1 | 18.86–24.3 |
| rat_pack / L1 / 75 | balanced | 0 | 6–8 | 0 / 0–1 | 1 | 22.85–29.23 |
| rat_pack / L1 / 75 | no_wizard | 0–4 | 2–4 | 0 / 0–1 | 1–3 | 19.96–34.66 |
| undead_pair / L1 / 100 | balanced | 0 | 6–10 | 0 / 0–1 | 1–2 | 24.55–33.16 |
| undead_pair / L1 / 100 | no_wizard | 0 | 2–4 | 0 / 0 | 1 | 18.1–23.08 |
| bugbear_solo / L1 / 200 | balanced | 0 | 6–8 | 0 / 0 | 0–1 | 16.55–27.93 |
| bugbear_solo / L1 / 200 | no_wizard | 0–14 | 2–4 | 0–1 / 0–1 | 0–3 | 10.3–32.98 |
| goblin_support / L1 / 200 | balanced | 0–8 | 8–10 | 0 / 0–2 | 1–2 | 27.94–33.32 |
| goblin_support / L1 / 200 | no_wizard | 4–9 | 4 | 0 / 0–1 | 2 | 24.81–32.01 |
| bugbear_support / L2 / 300 | balanced | 0–25 | 8–14 | 0–1 / 0–1 | 1–3 | 29.49–52.04 |
| bugbear_support / L2 / 300 | no_wizard | 0–29 | 2–8 | 0–2 / 0–1 | 1–4 | 22.26–66.34 |

Ranges cover all three completed trials; no failures were excluded. Damage is the resolved incoming total (including temporary HP and overkill), not net HP depletion. Mana is gross spending, so Arcane Recovery/potions can make it exceed the initial pool. Starting Mage Armor and its ordinary Arcane Recovery cost are included. Simulation duration includes preparation/approach/presentation and is not human exploration time.

Per-character damage, maximum hit, six-second/round burst, exact healing events, sampled HP/downings, remaining resources and ability use were recorded in the original trial journals and reports; journals are now pruned. The six-second window is `(t−6000ms,t]`; sampled HP/downing counters are lower bounds.

## Candidate recommendations for PC07.2

| Recipe | Candidate role | Placement needs and risks |
| --- | --- | --- |
| goblin_pair | Ordinary group; optional XP reserve candidate | Two front subslots; readable approach. No-wizard incoming 0–7. Check how doors/corners remove the arena ranged opening. |
| rat_pack | Easier optional reserve candidate | Keep melee pack coherent and accessible before the guardian. Observed 0–4 incoming, but this sample often killed rats during approach; it does not bound close-start Pack Tactics. Only 75 XP total. |
| undead_pair | Ordinary/optional alternative | Same accessible melee approach, useful blunt/radiant options. All sampled incoming was zero, yet balanced parties spent 6–10 mana and sometimes a mana potion. Zombie Fortitude and approach advantage need real-room review. |
| bugbear_solo | Conditional early guardian candidate | Readable warning and a recovery opportunity first. One normal, noncritical 14-damage hit downed the L1 cleric; retain the counterexample to five cheap wins. Do not promote it to a routine L1 enemy from these results. |
| goblin_support | Later ordinary/guardian candidate | Two front goblins, archer and shaman behind; keep reinforcement paths isolated. Sample total incoming 0–9, balanced mana 8–10 and up to two mana potions. |
| bugbear_support | Conditional L2 guardian candidate | Bugbear/goblin front and shaman rear. Two downings, 0–29 incoming total, up to two healing potions, and 14/15-damage six-second bursts on the cleric. Action Surge was used in all six; Cunning Action and Preserve Life were exercised. |

No guaranteed supplies/rewards were granted by the experiment; ordinary monster loot was not collected. PC07.2 must propose finite supplies from cumulative costs, including arrows and spent short/long-rest charges. Mage Armor may persist across fights, whereas the arena starts Arcane Recovery and class charges fresh every trial.

XP divides among non-dead members: these recipes give 25, 18.75, 25, 50, 50 and 75 XP each to four living members. The target is 300 XP each before the final guardian: **1,200 total compulsory-route XP**. No automatic floor levels, milestone XP, respawns or XP multipliers were introduced. No concrete floor allocation is selected here.

## Limits and reproduction

The policy is deliberately focused: paid damage/rescue, existing selected spells and contact-turn level-2 class features. Newly earned preparation slots stay empty. Bless and Shield of Faith are not selected by aggressive tactics; unsupported active spells fail preflight. The open arena and exact diagnostic enemy information advantage are explicit. Seed count three is too small for population confidence; no full expedition, depleted-entry or different approach geometry was sampled.

The original study selected first/fastest/slowest victories and downing traces;
six selected replays matched at that checkpoint. Housekeeping on 2026-11-05 removed
raw trial/decision journals, trace bodies and the local pinned runner under
`output/pc071/`. Exact historical replay is no longer available. `index.json` and
`replay-manifest.json` retain historical hashes and recorded replay outcomes, not
available trace files. Per-recipe manifests, scenarios, summaries and provenance
remain, including the manifest consumed by `tools/campaign_route_review.py`.
[Calibration reports](../verification/archive/README.md#calibration-summaries) preserve
measured conclusions and limitations. Manifests/scenarios under
`experiments/arena/pc071/` support fresh reproduction with a current runner and a
new disposable output directory, only when a new experiment is explicitly requested.

All six batches completed; recorded batch wall time 6.824 s, summed trial wall time 5.851 s. No runtime/output limit was raised or reached. Output footprint at study/capture checkpoint: 96,821,410 bytes, below 3 GiB. One serial low-priority runner; no concurrent build/test/experiment.
