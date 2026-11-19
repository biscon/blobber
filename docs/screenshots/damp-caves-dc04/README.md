# The Drowned Galleries · Blind cave newt

**DC04 was accepted and closed by the player on 2026-11-19.** Launch `./review-caves --depth 5 --seed 5 --encounter` from the repository root to play these encounters. Each launch starts at the threshold with Alden (fighter), Brunna (cleric), Merrin (rogue) and Lethiel (wizard). [Player walkthrough](../../dc04_blind_cave_newt_design.md#player-review).

| Native game frame | What to inspect |
| --- | --- |
| [Idle front view](idle-front.png) | The closed jaw, mottled brow and splayed toes are visible at the same angle as the player’s first screenshot. A stationary camera inspected the solo creature before the simulation clock started. |
| [Named bite warning](raised-jaw.png) | Two native frames 480 ms apart show the throat rising and mouth opening while the same character can still choose Defend. |
| [Bite impact](impact.png) | The solo bite resolves after Defend. The combat log records the attack roll. |
| [North bank opening](north-first.png) | From (16,13) facing east, the rope route gives the party one legal melee target, the spider. |
| [South bank opening](south-first.png) | From (16,17) facing east, the unroped route gives the same party one legal melee target, the newt. |
| [Both species in contact](both-species.png) | Later in the south fight, the spider and newt both expose selectable body pixels in the same view. |

The five combat views come from animated threshold-to-encounter walks with the regular HUD and command controller. The hidden renderer exported all frames at 1920×1080. These frames show the artwork and initiative sequence before the heavy bite damage revision. [Current bite and Defend rules](../../dc04_blind_cave_newt_design.md#heavy-bite-balance-revision).
