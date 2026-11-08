# Encounter density and guardian balance

Status: implemented, mechanically verified and positively reviewed in a full user playthrough, 2026-11-08.
The authorized balance sample stopped on an initial diagnostic setup failure before combat.
The user approved the plan and explicitly accepts requiring a new campaign.
Historical PC07 calibration remains evidence for its original build only.

The user subsequently reported a 40–50 minute full clear: floor one somewhat tough,
floor two comfortable, and the final guardian tense toward the end. The party
finished just below level three, matching the intended XP headroom. This is one
human playthrough, not a measured win/downing-rate sample. Preserve this tuning
while correcting the [reported archer targeting/navigation](combat_positioning.md#archer-visibility-and-firing-positions--2026-11-08-follow-up)
and [interrupted-round behavior](combat_entry_followup.md#resume-interrupted-rounds--2026-11-08-correction).

## Experience and fixed first-pass tuning

Keep the existing three 36×36 floors, movement, prepared presentation, XP sharing,
300/900 advancement thresholds, supplies and ordinary attack damage. Target 6/7/8
encounters: 4/4/5 ordinary, 1/2/2 optional and one guardian per floor.
Ordinary sequences are pair/patrol/pair/patrol; undead/patrol/undead/patrol;
undead/support/undead/pair/undead. Patrol means two melee goblins plus an archer;
other recipes retain existing membership. Optional groups are rats, two goblin
pairs, and two undead pairs respectively. The first optional group owns the sole
existing reserve cache; additional optional fights have ordinary drops only.

Guardian groups: goblin captain/goblin/two archers/shaman; bugbear captain/two
goblins/archer/shaman; bugbear commander/three goblins/archer/shaman. Leads have
20/34/42 HP and otherwise retain base statistics, loot, attacks and 50/200/200 XP.
Separate catalog definitions reuse shared appearance aliases and normal UI names.
Target challenging guardians: 3–5 meaningful rounds, occasional downings, uncommon
permanent deaths. No automatic damage, supply or HP optimization after sampling.

Party XP by floor (ordinary/optional/guardian): 500/75/250, 500/200/400,
600/200/450. Four survivors receive 675 each direct or 793.75 full-clear; clearing
the intended ordinary route makes level two available before guardian two.

## Ownership, bounded planning and placement

FloorLayout owns eight contiguous encounter records, up to six members per recipe
and the existing 32 actor placements. Generation selects sites, plans routes and
places formations during preparation. Runtime retains ordinary actor, AI, combat,
objective and campaign ownership; no new per-frame allocation or resource layer.
The population resolver remains the authority for launch and save admission.

Select ordinary sites from junctions, including guarded key rooms. First optional
site is a leaf; the second may be a leaf or an upstream junction whose exclusion
preserves key, ordinary and guardian access. Enumerate bounded eligible site sets,
find a shortest legal walk visiting their ordinary sites and the key before the
guardian, and rank by first encounter distance, longest empty interval, walk length
and stable room IDs. Measure actual tile distances and report alternate progression
routes too. Targets of 12 first steps / 15 between opportunities are soft, reported
pacing goals rather than silent admission guarantees. Revisited rooms have no new fight.
Existing generation attempts and both checked fallbacks must satisfy hard invariants.
Preparation workspaces are fixed and phase-local, within existing scratch accounting.

Approach-relative formations use two adjacent rows of legal room cells with at most
two initial actors per cell, melee forward and separate ranged/support positions.
Every pair is within three cardinal steps, retaining existing group-alert reach. Avoid fixtures,
door approaches, landings and collisions. Keep coherent alert reach, reachable melee
approaches and usable ranged/support sightlines. Closed room doors separate groups;
actual AI tests must exercise this. Exhaustion rejects a candidate instead of publishing
partial population. Spell immunity, Sleep rescue and ordinary AI remain unchanged.

Fingerprints, reports, enum bounds, catalog identity and campaign schema/simulation
advance with the new definitions. Old saves reject without migration or modification.

## Verification and authorized sample

Owned CPU fixtures cover counts/XP/HP, all depths/rotations, small fixed seed set,
both fallbacks, invalid/capacity placements, guarded keys, bypassable optional sites,
real perception/movement/casting, Sleep/area interaction, death/objective/XP exactly
once, travel and save continuation. Run focused Release callers/allocation suites,
smoke saves during development, extended persistence once, then build the Debug game.
Route diagnostics must handle repeated rooms and all optional groups.

Authorized serial sample: 12 generated-room guardian trials (three floors × balanced/
no-wizard × seeds 5/8, full resources at L1/L2/L2), four persistent direct routes
(both parties × seeds 5/8), then two full-clear routes (both parties, seed 5).
Inspect results between stages; keep defeats, distinguish tooling/policy failures,
and stop on tooling/coverage/storage failure without sample expansion or auto-tuning.
Guardian limit 60 wall seconds; route limit 180; total 30 minutes / 3 GiB.
Measure rounds, damage/bursts, downings/deaths, supplies/rests, advancement, merges,
first/empty/retraced steps. Diagnostic knowledge and bot limitations remain explicit.

Use configured CMake/Ninja, one intensive low-priority job, at most two build workers
and one test process. No desktop input/focus automation. Artifacts live in the owned
agent run; promote only selected evidence, document limitations here and clean owned
output after runners close. User play and acceptance remain pending.

## Implementation and measured results

Implementation in progress. The bounded planner uses a 1,024-state indexed heap
(room plus up to six required-site/key bits), exact passage/room tile distances,
and exhaustive small site sets over at most eight junctions. Work is preparation-only.
Pacing reports measure forward steps to first entry into each new populated room,
including final blocking decoration geometry; they do not measure player search time.
The alternate report follows a shortest key/guardian route and exposes skipped fights.

The maintained route tool now understands blocking decorations, explicit chest
approaches/opening, guarded keys, repeated rooms and multiple optional groups.
Routine route fixtures disable full-state trace recording: repeating generation
metadata at every preview exceeded the existing 256 MiB trace cap. The balance
study instead retains bounded compact request/event/resource journals, initial/final
states, checkpoints and input hashes. These are explicitly **not strict replay
traces**; the existing normal runner trace/replay format is unchanged. Diagnostic
fresh-guardian setup is confined to a separate opt-in executable, not normal protocol.
Current-stock party inputs live under `experiments/encounter_density/parties/`;
historical calibration inputs are unchanged.

### Delivered verification

Generator/templates are **11/8**; campaign schema/simulation are **19/23**. New
campaigns are required. Catalog validation loads all eleven monster definitions
and ordinary current-stock parties. No user saves or character libraries were edited.

Focused Release checks pass across generation, population, fixtures, guardians,
dressing, campaign, arena, automation, allocation guards, world integration and
transport/persistence. In particular, the final eighteen-suite selection passed
sixteen immediately; updated owned CLI fixture catalogs/resident appearances and
new guardian membership expectations corrected the remaining two. Their final
`generation_cli` and `campaign_save_cli` checks pass (1.26 / 1.31 s). The separate
`fixtures_automation` check passes (4.74 s), including fresh-process strict replay
and corruption rejection. No admission checks were weakened.

The transport fixture uses harmless one-HP enemies, including guardian variants;
it replans after queued contact movement/automatic facing and supports checked
bow/cantrip actions for targets across multiple cells. Its old melee-only helper
and full-strength guardian fixtures could stall and exceed the unchanged 256 MiB
trace cap; the focused replay fixture now finishes without raising that cap.

The owned direct-route fixture passes. The additional explicit full-clear fixture
also passes: all three floors, both optional fights on depths two/three, three
recovery opportunities, and exactly **793 XP + 9/12** per character at completion.
Optional encounters may form a two-fight branch; the diagnostic route chooses a
currently reachable optional group first rather than assuming ID order is reachable.
These harmless fixtures establish mechanics and XP, not production difficulty.
For the reviewed depth-three seed-5 floor, CLI accounting reports **11,264 bytes**
of generation metadata and **113,360 bytes** scratch, within the existing 16/128 KiB
budgets. World-definition/occupancy data and prepared artwork are separate owners.

Commands (using `/home/bison/clion/bin/cmake/linux/x64/bin/cmake` from each existing
cache; `nice -n 15 ionice -c 3`, at most two build workers and one test process):

- Build affected Release test targets, `blobber_headless`, `blobber_generate_floor`
  and the opt-in `blobber_guardian_trial`; build Debug `blobber` with `--build
  --preset debug --target blobber -j2`. The Debug game and its prepared assets build;
  `blobber --help` also succeeds without opening a window.
- `ctest --test-dir cmake-build-release -R '^(generation|population|fixtures|guardians|dressing|campaign|generation_automation|automation|automation_allocations|progression_allocations|arena|world_integration|campaign_save|campaign_route_tools|campaign_route_fixture|campaign_city_route|generation_cli|campaign_save_cli)$' -j1 --output-on-failure`
- Corrected caller checks separately: `-R '^(fixtures_automation|generation_cli|campaign_save_cli)$'`;
  only failing checks were repeated after their relevant fixes.
- Extended `campaign_save` passes (about eight seconds), covering new definitions,
  corrupt data, progression and restoration. It was rerun after formations changed;
  no unrelated configuration/sanitizer/GPU matrix was run.
- `python3 tests/campaign_route_fixture.py cmake-build-release/blobber_campaign_route_fixture --full-clear`
- `cmake-build-release/blobber_rules_content_tests --validate assets/rules/catalog.json`
- `git diff --check` passes.

### Static pacing observations

These are six final-layout reports (seeds **5, 8**, in that order), not combat
trials or human search-time measurements. Both frozen fallback seeds **0, 1** also
validate at all depths with exact 6/7/8 encounter counts.

| Depth | First room opportunity, steps | Longest empty interval | Shortest key/guardian alternative: longest interval | Intended tour steps |
| --- | --- | --- | --- | --- |
| 1 | 16 / 19 | 17 / 19 | 16 / 19 | 67 / 63 |
| 2 | 10 / 16 | 16 / 36 | 16 / 34 | 69 / 106 |
| 3 | 15 / 10 | 19 / 22 | 19 / 34 | 80 / 97 |

The soft **12/15** targets are **not consistently met**. Protected arrival geometry,
existing passages and travel through previously cleared rooms remain significant.
The delivered encounter counts are higher; these observations do not establish that
all searching/backtracking is solved. No map expansion, landing-safety relaxation,
extra fights beyond the approved counts or automatic tuning followed these reports.

### Balance sample stopped; setup corrected

The approved first case was started with:

```sh
nice -n 15 ionice -c 3 python3 tools/encounter_balance_review.py \
  --runner cmake-build-release/blobber_headless \
  --guardian-runner cmake-build-release/blobber_guardian_trial --assets assets \
  --output OWNED_RUN/balance --case 1
```

It stopped after **0.011 s**, before responding to initial setup, with
`Unexpected arena advancement choices above the target level.` The private runner
incorrectly filled level-two wizard choices for a level-one trial. It now supplies
those choices only at level two. A setup-only preflight subsequently passed for
both parties at all three depths: correct L1/L2/L2 levels, full HP/mana and exact
0/300/300 XP. Those six preflights ran no fights.

Following the approved **stop on tooling failure** rule, the combat sample was
**not restarted or expanded**. **Zero of eighteen production combat/route trials
completed.** Guardian round counts, downing/death rates and cumulative production
supply costs therefore remain unmeasured. There is no new win-rate or 3–5-round
success claim. A fresh explicitly authorized sample can use the corrected tool and
a new owned run directory; the failed study directory cannot silently continue.

No desktop input/focus automation or interactive play occurred. No new visual/GPU
acceptance is claimed. Ordinary rendering, existing artwork and attack damage are
unchanged. Selected results and limitations are recorded here; disposable logs,
journals and generated reports are cleaned after runners close. No retained run
exception is needed for the corrected setup failure.
