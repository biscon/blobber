# PC07.2 generated-route evidence

2026-11-01. Eight explicitly approved serial diagnostic expeditions completed: **8 victories, 32 observed downings, zero permanent deaths, zero recorded merges**. All reached level two before the final guardian with three ordinary kit-funded field rests. Two selected retained traces strictly replayed and exported hidden application-owned captures. These are two seeds per composition/profile, not campaign win-rate estimates.

[Design](../pc072_population_design.md) · [Verification](../pc072_verification.md) · [Gallery](../screenshots/pc072/index.html) · [Trace index/hashes](index.json)

## Population and progression

| Recipe | Composition | XP | Role / placement | Evidence and risk |
| --- | --- | --- | --- | --- |
| goblin_pair | 2 goblins | 100 | F1/F3 ordinary, F2 reserve; front subslots | PC07.1 L1 incoming 0–7; door/pillar approaches produced real downings here. |
| rat_pack | 3 giant rats | 75 | F1 reserve; two front/one rear | Arena incoming 0–4 is not a bound on close-start Pack Tactics. |
| undead_pair | Skeleton + zombie | 100 | F2/F3 ordinary, F3 reserve; front subslots | Blunt/radiant options help; zombie Fortitude and depleted mana matter. |
| goblin_support | 2 goblins + archer + shaman | 200 | F2 ordinary; F1/F2 guardians; ranged/support rear | Arena L1 mana 8–10 for balanced; isolate ports and inspect cumulative costs. |
| bugbear_support | Bugbear + goblin + shaman | 300 | Final guardian only, intended L2 | Arena had downings and 14/15-damage cleric bursts. Generated route differs in geometry and resources. |

Lone bugbear is excluded: its PC07.1 ordinary noncritical 14-damage hit downed a level-one cleric. No monster statistics changed. Front/rear subslots rotate toward each generated approach; open-arena approach advantages do not transfer as guarantees.

The intended direct path earns 400 XP on F1, 500 on F2 and 300 before the final guardian: **1,200 total / 300 each** for four survivors. Final guardian adds 300. Reserves add 75/100/100, so reserve profiles entered the final fight at **368.75 XP each**, versus **300** direct. These are intended-route budgets, not compulsory battle gates; skipped ordinary fights can require backtracking. Full clears here remain below level three.

Fixed main caches per floor: 2 healing/4 mana potions/20 arrows. Reserve caches: 1/2/10. Floor 1–2 guardian rewards: 1 healing potion/1 kit. Final victory grants no new supply cache. Starting gear/supplies remain ordinary and monster drops are additional.

## Actual persistent-route costs

H/M are healing/mana potions consumed; damage includes temporary HP and overkill. Downings are sampled lower bounds. Simulation seconds are ticks × 10 ms, including preparation/approach/presentation but excluding the instant eight-hour rest skips; wall time is separate. All rows used three kits and all completed without a runner, policy, artifact or budget failure.

| Party / seed / route | Incoming | Downings | Gross mana | H / M | Arrows | Rounds | Simulation s | Wall s |
| --- | ---: | ---: | ---: | --- | ---: | ---: | ---: | ---: |
| balanced-5-direct | 62 | 2 | 62 | 2 / 12 | 13 | 14 | 335.69 | 16.67 |
| balanced-5-reserve | 107 | 5 | 66 | 5 / 17 | 14 | 19 | 466.93 | 22.22 |
| balanced-8-direct | 96 | 3 | 58 | 5 / 12 | 13 | 18 | 384.78 | 16.70 |
| balanced-8-reserve | 130 | 6 | 70 | 6 / 17 | 14 | 19 | 494.24 | 25.23 |
| no_wizard-5-direct | 92 | 3 | 34 | 5 / 10 | 22 | 14 | 322.03 | 15.19 |
| no_wizard-5-reserve | 95 | 4 | 42 | 7 / 15 | 28 | 20 | 465.62 | 20.87 |
| no_wizard-8-direct | 105 | 5 | 36 | 5 / 10 | 30 | 18 | 360.42 | 15.04 |
| no_wizard-8-reserve | 129 | 4 | 44 | 8 / 16 | 31 | 21 | 496.64 | 23.35 |

The reserve profiles used more mana potions and generally had more downings than their paired direct profiles. Optional XP/supplies are finite additional opportunities; they are not proven net recovery or a universal improvement for this aggressive policy. No tuning optimizer, extra trial or automatic rerun followed these observations. The direct path was viable for both tested compositions at both seeds. Frequent downings remain a human pacing/recovery review point.

Every result retains per-character damage, largest hit, six-second/round bursts, healing, HP/life, mana, feature use, exact inventory and spell/build state. Pre-recovery and post-recovery guardian snapshots preserve the actual depleted state, advancement and supply accounting. Burst queues did not overflow. Transfers are excluded from consumption; observed mana/HP gains still include ordinary rest and advancement and must not be described as combat healing.

## Reproduction and limitations

The exact run command used one case at a time, with result inspection between cases:

```sh
nice -n 15 ionice -c 3 python3 tools/campaign_route_review.py \
  --runner output/pc072/bin/blobber_headless --assets cmake-build-release/assets \
  --output output/pc072/study --case N
```

Cases 1–8 are balanced seeds 5/8 direct/reserve, then no_wizard seeds 5/8 direct/reserve. Do not rerun into that directory. A fresh reproduction is a new experiment requiring its own opt-in. The script enforces ordered cases, unchanged tool/binary/content identity, 120 seconds/60,000 ticks per route and thirty minutes/three GiB overall. Total trial wall time was 155.27 s; original trace bytes total 1,322,479,568. No ceiling was raised or reached.

All eight complete traces are retained compressed with uncompressed SHA-256 values in `index.json`. The pinned matching runner remains locally at `output/pc072/bin/blobber_headless`. Decompress a selected trace to a new disk-backed output file, then use that runner with `--assets cmake-build-release/assets --mode diagnostic --replay FILE`. Strict replay rejects another binary/content identity; no translation is promised. The recorded tool copy is an audit artifact; use the original tools-directory path/context for fresh runs.

Selected replays: balanced-8-reserve (six observed downings) and no_wizard-8-direct (five). Both matched. Their capture operations and per-floor incoming totals are in `review-selections.json`. Captures reconstruct current HUD/scene state, not historical hover/log feedback; no repeat-identical PNG claim. Two obsolete map-help strings were corrected after capture; retained study images are from the pinned binary before that text-only correction. The gallery uses native gameplay views.

Diagnostic knowledge is explicit. Tactics are aggressive v2 with a 250 ms real-time cadence; newly earned preparation slots stay empty, and Bless/Shield of Faith are not chosen. The script collects supplies into the first legal carrier and retains starting equipment; it does not optimize ammunition distribution, equipment, formation or rest timing. Ammunition carried by another member is not a personal bow refill. These limits matter when interpreting costs and choosing PC08 policies. No hub-recovery comparison, intentionally depleted initial party, broad seed sweep, class matrix or human exploration/fun measurement was performed.
