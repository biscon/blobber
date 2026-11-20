# DC07 · Campaign descent, persistence and victory

**Ready for review, 2026-11-19.** [Expansion tracker](damp_caves_plan.md) · [Population and reward handoff](dc06_population_rewards_design.md#runtime-and-dc07-handoff) · [Vault rules](dc05_broodmother_design.md#handoff-and-limitations)

## World and travel

One campaign owns the Party and ten immutable location definitions. Sanctuary and its three doorway interiors retain IDs 0, 4, 5 and 6. An ordered depth table maps floors 1–6 to IDs 1, 2, 3, 7, 8 and 9. Travel requests and waypoint discovery use depth. Location snapshots and the save summary use ID. The travel coordinator resolves the destination, stages its session and view, and commits both together. On a return visit it restores the local gate state before placing the party on the exit landing. Inactive locations retain bounded value snapshots and share the existing content catalog.

During campaign preparation, the generator binds the three Underkeep floors with their existing seed streams and the player's initial reward profile. Floor three keeps its commander and reward chest and gains stairs to The Seep beyond the gate. Each cave uses its DC06 populated layout, kill rewards and finite recovery supplies. The Vault also reserves the DC05 brood slots. Descending requires the local objective. Ascending and waypoint returns use the protected landings. The broodmother awards 200 party XP on death. The Vault gate opens and a surviving party wins once the last living young falls. Defeat takes precedence if nobody is conscious.

## Development starts

A normal new game starts in Sanctuary. In development mode the regular four-slot picker resolves the chosen presets and saved builds. A second screen offers floors 1–6 and displays level and per-survivor XP. Back preserves the roster. The headless campaign scenario takes the same optional `starting_depth`. The selected party begins rested at the chosen entrance, carrying its starting gear plus the guaranteed rewards from the ordinary route on skipped floors. Preparation credits ordinary kills and guardian completion once, moves required keys and guaranteed supplies to the party, and leaves optional enemies and caches for backtracking. Each reconstructed floor passes the settled-floor import checks before launch. Saving over another expedition still requires the usual replacement confirmation.

| Depth | Starting level | XP each |
| --- | --- | --- |
| 1 | 1 | 0 |
| 2 | 1 | 187.5 |
| 3 | 2 | 412.5 |
| 4 | 2 | 675 |
| 5 | 2 | 850 |
| 6 | 3 | 1150 |

The party retains fractional XP. Preparation advances each class through its levels with recommended spells and a Constitution-first level-four improvement. This runs once before play, on copies of the selected characters. Later advancement is manual.

## Checkpoints and limits

Campaign schema/simulation is 29/33 and generator/templates is 15/11 for the encounter pressure revision. Prior active saves require New Campaign. Inspect and failed Continue leave bytes untouched. Existing version-two terminal records retain run association. Regenerate the ten-location baseline once on load, validate shape and biome, and apply the existing sparse deltas. Preserve canonical cave layout, population, named award ranges, actor generations, objective members and their generations, finite loot, discovery and party XP. A settled Vault checkpoint records the health and state of each sac and the positions, lives and generations of the brood. Reject active combat, unresolved warnings and pending hits. Enforce canonical dimensions and the 1,600-cell ceiling, 32 placements and eight encounters per floor, and the existing 16 MiB save and active/staged rendering limits.

## Verification

The seed-five route uses owned definitions, a reordered party with a named saved build and the normal campaign travel, reward, inventory and store APIs. The test places the party at stair landings and resolves ordinary fights with direct damage. The CLI check launches two separate headless processes. The hidden renderer uses the shipped catalog, room blueprints and artwork.

| Exit check | Observation |
| --- | --- |
| Underkeep regression | Compared detailed seed-five reports before and after DC07. Floors one and two matched in geometry, dressing, supplies and population. Floor three changed only its exit fixture, from the endpoint at (8, 28) to downstairs at (8, 27) facing the same landing. Version, fingerprints and measured generation time were excluded. The commander still unlocks the wearable reward chest. |
| Travel and finite rewards | The living commander blocked descent. Clearing it admitted the party to The Seep. The route crossed floors 4, 5 and 6 in both directions, including a return through the cleared guardian gate. A recall spent one kit, each town doorway returned to Sanctuary and the entrance offered only known destinations. The Seep camp stayed looted after Continue and on return. |
| Continue and identity | A fresh process loaded the chosen roster at level two with 675 XP each and entries 1–4 unlocked. Inspection and rejected load preserved the bytes of a slot carrying the previous active version. Terminal records still bind the result to the owning run. |
| Final objective | The original DC07 run used the superseded survey bonuses. The November 20 kill-only ordinary route yields 850, 1,150 and 1,525 XP per survivor at the same checkpoints. Killing the mother with a living young kept the campaign active. A settled generation-two young survived a codec round trip, a stale generation was rejected, and killing the young completed victory. The brood encounter tests exercise the warning and hatch timing. |
| Rendering and rollback | The hidden renderer staged the transition from floor three to The Seep, cancelled it, retried and continued to the Vault. The source stayed playable on cancellation. During the route the highest sampled active-plus-staged world residency was 86,325,754 bytes CPU and 612,461,892 bytes GPU, within the respective 352 MiB and 1,024 MiB limits. [Native frames](screenshots/damp-caves-dc07/README.md) |

The seed-five Debug start on floor four took 145.6 ms to prepare all six floors, bind the selected party and reconstruct the Underkeep. Ten floor snapshots have a combined fixed capacity of 2,698,880 bytes. The settled Seep save occupied 89,014 bytes on disk. The fixture that also visited every town interior and dungeon floor encoded to 168,556 bytes, within the representative 256 KiB check and the store's 16 MiB admission limit. These are single observations.

The allocation suite injected failures at 146 stairs, 123 recall, 146 waypoint and 3,517 restore allocations. It checked the source owner after each failed preparation and recorded zero tracked C++ allocations during its steady-state workload.

The following focused Debug tests passed. The CPU suites use owned fixtures. For the hidden capture command, set `run_dir` to a newly created agent artifact directory. Use the CTest shipped alongside the configured CMake for this build tree.

```sh
ctest --test-dir cmake-build-debug -j1 --output-on-failure \
  -R '^(brood_vault|menu|automation_allocations|cave_layout|generation|generation_cli|guardians|damp_campaign|campaign|campaign_save|campaign_save_cli)$'
env OMP_NUM_THREADS=1 OPENBLAS_NUM_THREADS=1 LP_NUM_THREADS=2 nice -n 15 ionice -c 3 \
  cmake-build-debug/blobber_world_integration_gpu_tests assets "$run_dir/native" \
  --damp-campaign-review cmake-build-debug/assets/prepared
```

The Debug game and its prepared publication built successfully in a serial retry after two asset builds overlapped. The hidden renderer opened the regenerated publication, completed the route and exported three inspected frames. The gallery retains those frames. No further slice depends on this handoff. Player feedback and any corrections belong to DC07.

## Player review

The [encounter pressure revision](damp_caves_pressure_design.md#player-review) includes the two doorway checks, revised XP and continuous before/after expedition results.

1. From this checkout, launch the Debug game with the command below. Choose **Start new game**, fill the four party slots using saved characters or **Use default party**, then select **Choose floor**. Check the level and XP shown for **4. The Seep** and choose it. The separate review save stays available for Continue between sessions.

   ```sh
   ./cmake-build-debug/blobber --development --seed 5 --campaign-save output/dc07-player-review.json
   ```

2. At the Seep camp, pick up the guaranteed antitoxin. Save in quiet exploration, quit, and use **Continue**. Check your party order, inventory, location and XP. To try backtracking, recall with one camp kit, face the dungeon entrance in Sanctuary, and use the free return to The Seep. Entries deeper than your furthest visit should be absent.
3. Keep the same four survivors through both descents. For the ordinary-route comparison, take the camp and the webbed arch, clear the ordinary groups and guardians, and leave the two optional groups on each floor. The first spider takes each survivor to 687.5 XP. Level three becomes available at the Galleries scouts on the ordinary route. Advance manually when safe. The next ordinary room is the [five-spider nest trial](dc06_population_rewards_design.md#galleries-balance-trial). Compare routes at the divided banks and note where the party needed a rest. [Seed-five floor maps](screenshots/damp-caves-dc06/README.md)
4. Enter the Vault at 1,162.5 XP each and carry the party through the final guardian. Watch the clutches and choose when to interrupt them. With the ordinary groups cleared, victory banks **1,525 XP per survivor**. Optional fights, bypassed groups and casualties change that total. For a second comparison, use a separate review save and select **6. The Brood Vault**. Let a clutch hatch, focus the mother, and check that the party must still defeat her surviving young.

The route fixture checks state and earnings. The mother-first save case constructs a settled young, and the native victory capture shortens the fight. Human combat difficulty, sound, interaction feel and elapsed playtime await player review. One Debug seed and the sampled transitions establish the reported sizes and residency.
