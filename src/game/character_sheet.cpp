#include "game/commands.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace blobber {
namespace {
template <std::size_t N> void append(std::array<char, N>& text, const char* value) noexcept {
    const auto used = std::strlen(text.data());
    if (used < N - 1)
        std::snprintf(text.data() + used, N - used, "%s%s", used ? ", " : "", value);
}
} // namespace
void CommandController::refreshCharacterSheet() noexcept {
    auto& i = state_.inventory;
    auto& out = i.sheet;
    out = {};
    const auto* c = party_.character(i.character);
    if (!c)
        return;
    const auto& cls = *party_.catalog().characterClass(c->build.characterClass);
    const auto& race = *party_.catalog().race(c->build.race);
    out.modifiers = c->stats.modifiers;
    out.saves = c->stats.saves;
    out.proficientSaves = cls.saves;
    out.initiative = c->stats.initiative;
    out.proficiency = c->stats.proficiency;
    out.hitDice = c->availableHitDice;
    out.hitDie = cls.hitDie;
    out.experience = c->experience;
    // Presentation only for the currently supported level. Advancement owns no
    // behavior here; its future threshold query can replace this value.
    out.nextLevelExperience = c->build.level == 1 ? 300 : 0;
    out.spellAttack = party_.castingModifier(i.character) + out.proficiency;
    out.spellDifficulty = 8 + out.spellAttack;
    out.arrows = party_.inventory().count(i.character, rules::Ammunition::Arrow, party_.catalog());
    constexpr const char* armor[]{"Light armor", "Medium armor", "Heavy armor", "Shields"};
    for (unsigned n = 0; n < 4; ++n)
        if (cls.armor & (1u << n))
            append(out.armor, armor[n]);
    if (!out.armor[0])
        append(out.armor, "None");
    auto weapons = cls.weapons | race.weapons;
    if (weapons == rules::allWeaponTraining) {
        append(out.weapons, "All supported weapons");
        weapons = 0;
    } else if ((weapons & rules::simpleWeaponTraining) == rules::simpleWeaponTraining) {
        append(out.weapons, "Simple weapons");
        weapons &= ~rules::simpleWeaponTraining;
    }
    constexpr const char* names[]{"Daggers",       "Maces",      "Longswords",    "Shortswords",
                                  "Shortbows",     "Longbows",   "Battleaxes",    "Handaxes",
                                  "Light hammers", "Warhammers", "Rapiers",       "Hand crossbows",
                                  "Darts",         "Slings",     "Quarterstaffs", "Light crossbows"};
    static_assert(std::size(names) == rules::weaponTypeCount);
    for (unsigned n = 0; n < rules::weaponTypeCount; ++n)
        if (weapons & (1u << n))
            append(out.weapons, names[n]);
    if (!out.weapons[0])
        append(out.weapons, "None");
    auto feature = [&](const char* name, const char* detail, const char* value) {
        auto& row = out.features[out.featureCount++];
        row.name = name;
        row.detail = detail;
        std::snprintf(row.value.data(), row.value.size(), "%s", value);
    };
    if (c->build.defense)
        feature("Defense", "While wearing armor; included in AC.", "+1 AC");
    if (party_.hasFeature(i.character, rules::ClassFeature::SecondWind))
        feature("Second Wind", "Heal 1d10 + level; one action.", c->secondWind ? "1 / 1 use" : "0 / 1 use");
    if (party_.hasFeature(i.character, rules::ClassFeature::SneakAttack))
        feature("Sneak Attack", "Conditional extra damage; once per round.", "+1d6");
    if (party_.hasFeature(i.character, rules::ClassFeature::LifeDomain))
        feature("Life Domain", "Bonus to healing spells of level 1 or higher.", "+2 + spell level");
    if (party_.hasFeature(i.character, rules::ClassFeature::ArcaneRecovery))
        feature("Arcane Recovery", "Recover 2 mana in safe exploration.",
                c->arcaneRecovery ? "1 / 1 use" : "0 / 1 use");
    if (session_) {
        constexpr const char* effects[]{"Bless", "Bane", "Shield of Faith", "Mage Armor", "Hold Person"};
        for (unsigned n = 0; n < std::size(effects); ++n)
            if (session_->effects().has(i.character, static_cast<EffectKind>(n)))
                append(out.effects, effects[n]);
        if (session_->effects().concentration(i.character).serial)
            append(out.effects, "Concentrating");
    }
    if (!out.effects[0])
        append(out.effects, "None");
}
} // namespace blobber
