#pragma once
#include "game/actors.hpp"
#include "rules/spells.hpp"
#include <string_view>
namespace blobber {
inline const char* monsterName(std::string_view id) noexcept {
    if (id == "goblin.captain")
        return "Goblin Captain";
    if (id == "bugbear.captain")
        return "Bugbear Captain";
    if (id == "bugbear.commander")
        return "Bugbear Commander";
    if (id == "goblin")
        return "Goblin";
    if (id == "goblin.archer")
        return "Goblin Archer";
    if (id == "goblin.guardian.placeholder")
        return "Goblin Guardian";
    if (id == "goblin_shaman")
        return "Goblin Shaman";
    if (id == "adult.web.spinner")
        return "Adult Web-Spinner";
    if (id == "broodmother")
        return "Broodmother";
    if (id == "vault.young")
        return "Hatchling";
    if (id == "blind.cave.newt")
        return "Blind Cave Newt";
    if (id == "venomous.spider")
        return "Venomous Spider";
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
inline const char* monsterName(const ActorWorld& world, const ActorState& actor) noexcept {
    if (actor.npc.sac())
        return actor.npc.vault == VaultPart::WestSac ? "Western egg sac" : "Eastern egg sac";
    return monsterName(world.catalog().monster(actor.monster)->id);
}
// Keep legacy catalog bytes/save fingerprints stable while removing authoring notes from UI.
inline const char* spellDescription(rules::SpellKind kind, const char* authored) noexcept {
    if (kind == rules::SpellKind::SpiritualWeapon)
        return "Summon and strike for 1d8 + Wisdom force. Lasts up to 60 seconds or until the fight ends. "
               "No concentration. Use Command Spiritual Weapon to move up to two cells and strike.";
    if (kind == rules::SpellKind::ShockingGrasp)
        return "Cast from either party row against an exposed enemy in the three cells directly ahead. "
               "Melee spell attack: 1d8 lightning, with advantage against metal armor.";
    if (kind == rules::SpellKind::AcidSplash)
        return "One or two enemies within one cell of each other, including across cell boundaries. "
               "Walls block the splash. Dexterity save negates 1d6 acid damage.";
    return kind == rules::SpellKind::LesserRestoration
               ? "End blindness, paralysis or poison on a living ally."
               : authored;
}
} // namespace blobber
