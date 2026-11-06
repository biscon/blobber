#pragma once
#include "rules/spells.hpp"
#include <string_view>
namespace blobber {
inline const char* monsterName(std::string_view id) noexcept {
    if (id == "goblin")
        return "Goblin";
    if (id == "goblin.archer")
        return "Goblin Archer";
    if (id == "goblin.guardian.placeholder")
        return "Goblin Guardian";
    if (id == "goblin_shaman")
        return "Goblin Shaman";
    if (id == "giant.rat")
        return "Giant Rat";
    if (id == "skeleton")
        return "Skeleton";
    if (id == "bugbear")
        return "Bugbear";
    if (id == "zombie")
        return "Zombie";
    return "Creature";
}
// Keep legacy catalog bytes/save fingerprints stable while removing authoring notes from UI.
inline const char* spellDescription(rules::SpellKind kind, const char* authored) noexcept {
    if (kind == rules::SpellKind::ShockingGrasp)
        return "Cast from either party row against an exposed enemy in the three cells directly ahead. "
               "Melee spell attack: 1d8 lightning, with advantage against metal armor.";
    if (kind == rules::SpellKind::AcidSplash)
        return "One or two enemies within one cell of each other, including across cell boundaries. "
               "Walls block the splash. Dexterity save negates 1d6 acid damage.";
    return kind == rules::SpellKind::LesserRestoration
               ? "End one selected blindness or paralysis on a living ally."
               : authored;
}
} // namespace blobber
