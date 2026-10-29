# Starting-character portrait masters

Sixteen new production masters generated with imagegen for slice 12.2. Each PNG
has an adjacent `.prompt.json` with its exact prompt and source-output provenance.
The four original masters in the parent directory were preserved unchanged.

| Race | Fighter | Mage | Cleric | Rogue | Adventurer |
| --- | --- | --- | --- | --- | --- |
| Human | [alden](../alden.png) | [human.mage](human-mage.png) | [human.cleric](human-cleric.png) | [human.rogue](human-rogue.png) | [human.adventurer](human-adventurer.png) |
| Hill Dwarf | [dwarf.fighter](dwarf-fighter.png) | [dwarf.mage](dwarf-mage.png) | [brunna](../brunna.png) | [dwarf.rogue](dwarf-rogue.png) | [dwarf.adventurer](dwarf-adventurer.png) |
| Lightfoot Halfling | [halfling.fighter](halfling-fighter.png) | [halfling.mage](halfling-mage.png) | [halfling.cleric](halfling-cleric.png) | [merrin](../merrin.png) | [halfling.adventurer](halfling-adventurer.png) |
| High Elf | [elf.fighter](elf-fighter.png) | [lethiel](../lethiel.png) | [elf.cleric](elf-cleric.png) | [elf.rogue](elf-rogue.png) | [elf.adventurer](elf-adventurer.png) |

Labels are stable portrait IDs, not class restrictions. A character can select any
of the five portraits for their race. `rules/creation.cpp` owns the immutable
manifest. `blobber_bake_portraits` emits padded 216×252 and 432×648 center crops
into 2048×1024 and 2048×4096 QOI atlases plus `portraits.tsv` under the build's
`assets/prepared/` directory. Runtime art IDs 64–83 map to these portraits, never
to formation slots. Master images are not decoded in a game frame.

See [design](../../../docs/character_builds_design.md) and
[review/verification](../../../docs/slice122_verification.md).
