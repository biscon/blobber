# Ranger through level four

**Implemented and mechanically verified. Interactive review is pending.**

## Rules and ownership

The catalog owns Ranger training, starting equipment and the three-spell roster.
The Ranger has a d10 Hit Die, Strength/Dexterity saves, light/medium armor and
shield training, and simple/martial weapons. Recommended scores are 8/15/14/8/15/10.
Creation offers shortsword or dagger and shortbow or longbow, leather armor,
20 arrows and a healing potion. At level two Archery adds +2 to ranged weapon
attack rolls. Level three grants Hunter and Colossus Slayer. Level four uses ASI.

Wisdom casting uses half-caster mana 0/4/6/6. Known spells are stored in the
existing learned mask and cast directly. At level two automatically learn Hunter’s
Mark and choose Cure Wounds or Goodberry. Learn the remaining spell at level
three. Racial cantrips still use Intelligence. Advancement rules own grants and
apply the same validation in campaign, arena, and headless execution.

One normal action and two mana cast each spell. Hunter’s Mark uses the hostile
spell target checks, alerts the enemy, and concentrates for an hour of gameplay.
It ends on target death, expiry, dismissal or concentration loss. Only the
caster’s weapon hits gain 1d6. Colossus Slayer grants 1d8 on a weapon hit against
an already injured enemy, bounded by the ordinary one-action allowance. Sample
eligibility and roll damage at attack acceptance. Both bonuses double on critical
hits and combine with the weapon damage before mitigation and one HP commit.

Goodberry distributes up to 10 HP to injured conscious allies, caster included,
one point at a time to the lowest projected HP percentage, with character index
as the tie-breaker. It requires an eligible recipient and creates no items.

## Integration

The effect controller owns marks in its existing 64-slot pool with local
`ActorId` values. Adding a mark requires an eligible Ranger and one living
hostile. Each Ranger has one concentration slot and multiple Rangers may mark
the same creature. A failed admission preserves action and mana.
Checkpoint validation checks caster, target and concentration, and travel
requires dismissal. Catalog schema is 18, campaign schema/simulation is 25/27,
library is 1, and arena is 3. Pre-Ranger campaigns require a compatible older
build. The loader retains earlier catalog schemas and existing catalog indices.
Hunter's Mark and Goodberry occupy spell indices 33 and 34 in the authored
catalog. Headless observations expose the Ranger's learned spells and
`passive_features`. The Goodberry descriptor identifies the party targeting
scope and uses the existing no-explicit-target command shape.

The character sheet, picker, target HUD and automation use the same validated
grants and status information. Badge layout reserves the hovered health bar so
it cannot obscure a mark or another spell indicator. The four new icons occupy
one additional row of the startup-prepared class ability atlas, adding 1 MiB of
RGBA8 texture storage. The existing resource registry owns the atlas and the
shipping UI selector includes the source PNGs.

The built-in imagegen tool created the source images. The exact prompts and
corresponding PNGs share their basenames.

- [Archery prompt](../assets/ui/spells/archery.prompt.json)
- [Hunter's Mark prompt](../assets/ui/spells/hunters_mark.prompt.json)
- [Colossus Slayer prompt](../assets/ui/spells/colossus_slayer.prompt.json)
- [Goodberry prompt](../assets/ui/spells/goodberry.prompt.json)

Wilderness tracking, favored enemies and terrain, more styles and subclasses,
spell replacement, and levels past four are deferred.

## Verification

Focused Debug checks cover the four races, both healing choices, exact grants,
ASI and racial casting, ammunition, mark admission and concentration, wounded
critical hits and canceled impacts, deterministic healing, library, arena
rematches, headless parity, and shipping selectors. The warmed game workload
checks engine-owned C++ allocations across Ranger advancement, the spell picker,
Goodberry, and the marked bow attack. Campaign coverage includes a safely staged
mark, active effect round trip, expiry, invalid recipient rejection, and travel
admission after dismissal. Live casting and target death are covered separately
through combat commands.

Builds retained the CMake and Ninja paths configured in the Debug cache and used
low CPU and I/O priority with at most two build workers. Test runs used one
process. The focused command set was:

```sh
/home/bison/clion/bin/cmake/linux/x64/bin/cmake --build cmake-build-debug --parallel 2 --target blobber blobber_headless blobber_world_integration_gpu_tests
/home/bison/clion/bin/cmake/linux/x64/bin/ctest --test-dir cmake-build-debug -j 1 --output-on-failure -R '^(creation|rules_content|party_content|progression|progression_allocations|recovery|spells|effects|targeting|impact|library|arena|automation|audio|hud|visual_review|runtime_inventory|dist_staging|campaign_save_smoke|campaign_save_cli)$'
cmake-build-debug/blobber_campaign_save_tests --ranger
cmake-build-debug/blobber_world_integration_gpu_tests assets RUN_DIRECTORY/review --ranger-review cmake-build-debug/assets/prepared
```

The 20 focused suites passed across the initial run and targeted reruns.
The Ranger campaign check passed separately. The allocation guard recorded
zero engine-owned C++ allocations in its Ranger workload. The visual regression
also corrected two stale Sleep fixture expectations to the existing 12-second
duration. Hidden OpenGL verified the production creator, advancement, character
sheet, casting picker, and marked-target tooltip and badge at native and reduced
presentation sizes. The captures use synthetic picking over the application menu
backdrop. Interactive combat and live dungeon presentation await user review.
[Six inspected captures](screenshots/ranger/README.md).

Disposable run output was cleaned after inspection (41 files, 27,604,931 bytes).
The six selected captures and the source artwork and prompts are retained.
