# College of Lore Bard through level four

## Class and choices

The Bard uses Charisma, a d8 Hit Die, Dexterity and Charisma save proficiency,
light armor, simple weapons, longswords, rapiers, shortswords and hand crossbows.
A starting character chooses a rapier or shortsword and receives leather armor,
a shortbow, arrows and a potion.

| Level | Mana | Cantrips / known spells | New feature |
| --- | --- | --- | --- |
| 1 | 4 | 2 / 4 | Bardic Inspiration and Vicious Mockery |
| 2 | 6 | 2 / 5 | Song of Rest |
| 3 | 14 | 2 / 6 | College of Lore, Cutting Words and second-level spells |
| 4 | 17 | 3 / 7 | Ability score improvement and another cantrip |

Vicious Mockery is the locked class cantrip. The catalog also offers Guidance,
Resistance and Spare the Dying. First-level choices are Bane, Color Spray, Cure
Wounds, Sleep and Thunderwave. Second-level choices are Hold Person, Lesser
Restoration and Shatter. The creator, saved library blueprints, arena preparation
and campaign advancement use the existing spell masks and validate the same
choices. Bards cast their known spells directly. Racial cantrips retain their
own casting ability. The class adapts SRD 5.1 to the available dungeon actions.
[Content ledger](srd_content_ledger.md).

## Spending Inspiration

The party stores each Bard's spent uses and each recipient's source and expiry.
A Bard has at least one use, otherwise their Charisma modifier, refreshed on a
long rest. Granting a die uses the Bard's bonus action and requires another
conscious companion with an empty die slot. A held d6 lasts one gameplay hour.
It follows the character through formation changes, unconsciousness and floor
travel. The portrait badge identifies its giver and remaining time on hover.

After modifiers, a missed attack or failed rolled save can trigger a decision
when a d6 could change the outcome. This includes concentration and death saves.
Natural automatic outcomes retain their rules. The HUD shows the total, target,
minimum useful face, and Roll d6 and Keep die buttons. Keeping preserves the
die and RNG state. Spending consumes the die even if the result still fails.
Each ray can trigger its own decision while the caster holds the die.

At level three, a visible enemy hit within bow range can trigger Cutting Words.
It costs the chosen Bard one unassigned use and their reaction. Natural twenties
and attacks redirected to a mirror image skip the offer. A reaction refreshes
at the Bard's next initiative turn or after six gameplay seconds in real time.
Only one Bard can contest each attack. Song of Rest adds one d6 per living
short-rest recipient spending Hit Dice, even with several eligible Bards.
Vicious Mockery deals 1d4 psychic damage on a failed Wisdom save and imposes
disadvantage on the next attack until the end of the enemy's next turn, or six
seconds in real time.

## Simulation and persistence

GameSession owns one RollResolver per floor session containing a Bard. It reserves
a 1 MiB simulation stack at load. Combat runs on the caller's thread, using
POSIX contexts or a Windows fiber when an operation can ask a question. A suspended
operation retains its local rolls, target cursor and outstanding consequences.
This lets a concentration save nested within an area spell resume after damage
without repeating earlier recipients, rolling again or copying the world.
Parties with no Bard allocate no decision stack. Only one question can be
outstanding. The session keeps operation arguments alive across answers.

Both clients submit the world identity, decision serial and spend/keep choice.
The simulation checks the serial and eligible Bard before resuming. A stale
response has no effect. While waiting, movement, combat clocks, save and travel
are blocked. Settled campaign checkpoints store held dice, charges, reaction
availability and Mockery. Floor travel rebases held-die deadlines to preserve
remaining duration. Pending decisions have no serialized form. Catalog schema
19 and campaign schema 26 / simulation 28 apply. Older campaign saves require a
compatible earlier build. Library and arena retain their existing formats.

[Headless protocol](headless_protocol.md) · [Review captures](screenshots/bard/README.md)

## Verification

The Linux Debug build used the CMake and Ninja from the existing build cache,
two low-priority build workers and one test process. The following checks passed.

```sh
ctest --test-dir cmake-build-debug -j 1 --output-on-failure -R '^(progression|progression_allocations|creation|hud|buttons|automation|recovery|level_spells|impact|library|arena|audio|runtime_inventory)$'
cmake-build-debug/blobber_campaign_save_tests --bard
cmake-build-debug/blobber_world_integration_gpu_tests assets RUN_DIRECTORY/native --bard-review cmake-build-debug/assets/prepared
```

The 13 focused suites exercised class creation through level four, resource
ownership, spell and weapon attacks, death saves and concentration inside
Shatter. Separate rays of one cast can offer consecutive decisions. A two-Bard fixture
verified that only the chosen Lore Bard spends a use on a given attack. Headless
checks replay the same seeded spend and verify a stopped clock, read-only
preview and duplicate-answer rejection. The allocation guard measured zero
engine-owned C++ allocations while granting and spending a die. The campaign
check round-tripped charges and a recipient, travelled with the held die and
rejected a malformed source. Progression and recovery passed again after the
expiry-order adjustment. The hidden OpenGL review exercised the live picker,
portrait tooltip, both roll dialogs and the camp preview. The Windows fiber
path and interactive dungeon play remain unverified.
