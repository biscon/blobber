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
    out.nextLevelExperience = rules::nextLevelExperience(c->build.level);
    out.casting = cls.casting;
    out.spellAttack = party_.castingModifier(i.character) + out.proficiency;
    out.spellDifficulty = 8 + out.spellAttack;
    out.spellAttack += c->stats.spellAttackBonus;
    out.racialCasting = c->build.spells.racial != 0;
    out.racialAttack = c->stats.modifiers[3] + out.proficiency;
    out.racialDifficulty = 8 + out.racialAttack;
    out.racialAttack += c->stats.spellAttackBonus;
    out.bolts = party_.inventory().count(i.character, rules::Ammunition::Bolt, party_.catalog());
    out.arrows = party_.inventory().count(i.character, rules::Ammunition::Arrow, party_.catalog());
    constexpr const char* armor[]{"Light armor", "Medium armor", "Heavy armor", "Shields"};
    for (unsigned n = 0; n < 4; ++n)
        if (cls.armor & (1u << n))
            append(out.armor, armor[n]);
    if (!out.armor[0])
        append(out.armor, "None");
    auto weapons = cls.weapons | race.weapons;
    if (weapons == rules::allWeaponTraining) {
        append(out.weapons, "All weapons");
        weapons = 0;
    } else if ((weapons & rules::simpleWeaponTraining) == rules::simpleWeaponTraining) {
        append(out.weapons, "Simple weapons");
        weapons &= ~rules::simpleWeaponTraining;
    }
    constexpr const char* names[]{"Daggers",       "Maces",       "Longswords",    "Shortswords",
                                  "Shortbows",     "Longbows",    "Battleaxes",    "Handaxes",
                                  "Light hammers", "Warhammers",  "Rapiers",       "Hand crossbows",
                                  "Darts",         "Slings",      "Quarterstaffs", "Light crossbows",
                                  "Scimitars",     "Greatswords", "Mauls"};
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
    if (c->build.defense || (cls.progression == rules::ProgressionClass::Paladin && c->build.level >= 2))
        feature("Defense", "While wearing armor; included in AC.", "+1 AC");
    if (party_.hasFeature(i.character, rules::ClassFeature::SecondWind))
        feature("Second Wind", "Heal 1d10 + level; refreshes on rest.",
                c->secondWind ? "1 / 1 use" : "0 / 1 use");
    if (party_.hasFeature(i.character, rules::ClassFeature::SneakAttack))
        feature("Sneak Attack", "Conditional extra damage; once per round.",
                c->build.level >= 3 ? "+2d6" : "+1d6");
    if (party_.hasFeature(i.character, rules::ClassFeature::LifeDomain))
        feature("Life Domain", "Bonus to healing spells of level 1 or higher.", "+2 + spell level");
    if (party_.hasFeature(i.character, rules::ClassFeature::ArcaneRecovery))
        feature("Arcane Recovery", "Safe mana recovery; refreshes on long rest.",
                c->arcaneRecovery ? "1 / 1 use" : "0 / 1 use");
    if (c->build.level >= 2) {
        if (cls.progression == rules::ProgressionClass::Fighter) {
            feature("Action Surge", "One extra action; refreshes on rest.",
                    c->actionSurge ? "1 / 1 use" : "0 / 1 use");
            if (c->build.level >= 3)
                feature("Champion", "Weapon critical range.", "19-20");
        }
        if (cls.progression == rules::ProgressionClass::Cleric && c->build.lifeDomain)
            feature("Preserve Life", "Lowest HP first; half-max HP cap.",
                    c->channelDivinity ? "1 / 1 use" : "0 / 1 use");
        if (cls.progression == rules::ProgressionClass::Rogue)
            feature(c->build.level >= 3 ? "Thief / Fast Hands" : "Cunning Action",
                    c->build.level >= 3 ? "Free Defend or potion once per turn."
                                        : "Free Defend once per turn.",
                    "Optional");
        if (cls.progression == rules::ProgressionClass::Wizard)
            feature("Evocation / Sculpt", "Shatter protects companions, not caster.", "Automatic");
    }
    if (cls.progression == rules::ProgressionClass::Paladin) {
        char pool[32]; std::snprintf(pool, sizeof(pool), "%d / %d healing points", c->layOnHands, 5 * c->build.level);
        feature("Lay on Hands", "One point heals 1 HP; long rest refills the pool.", pool);
        if (c->build.level >= 2) feature("Divine Smite", "Two mana on a melee hit; critical doubles dice.", "+2d8 radiant");
        if (c->build.level >= 3) feature("Devotion / Sacred Weapon", "+CHA to weapon attacks for 60 seconds.", c->channelDivinity ? "1 / 1 use" : "0 / 1 use");
    }
    if (cls.progression == rules::ProgressionClass::Barbarian) {
        char uses[48];
        std::snprintf(uses, sizeof(uses), "%u / %d uses", party_.rageUses(i.character),
                      c->build.level >= 3 ? 3 : 2);
        feature("Rage", "One minute. Long rest restores uses.", uses);
        feature("Unarmored Defense", "Unarmored AC uses Dexterity and Constitution.", "Shield allowed");
        if (c->build.level >= 2) {
            feature("Reckless Attack", "Gain advantage on Strength melee attacks and expose yourself.",
                    "First attack");
            feature("Danger Sense", "Advantage on Dexterity saves against visible dangers.", "Automatic");
        }
        if (c->build.level >= 3)
            feature("Frenzy Strike", "One bonus-action melee swing per Rage after its first turn.",
                    "Berserker");
    }
    if (cls.progression == rules::ProgressionClass::Bard) {
        char uses[32];
        std::snprintf(uses, sizeof(uses), "%d / %d uses", party_.inspirationUses(i.character),
                      std::max(1, c->stats.modifiers[5]));
        feature("Bardic Inspiration", "Give a companion a d6. Long rest restores uses.", uses);
        if (c->build.level >= 2)
            feature("Song of Rest", "Adds healing when companions spend Hit Dice.", "+1d6 per rest");
        if (c->build.level >= 3)
            feature("Cutting Words", "Spend one Bard use and a reaction on an incoming attack.",
                    "-1d6 to hit");
    }
    if (cls.progression == rules::ProgressionClass::Ranger && c->build.level >= 2) {
        feature("Archery", "Included in ranged weapon attack rolls.", "+2 ranged attack");
        if (c->build.level >= 3)
            feature("Colossus Slayer", "Weapon hits against wounded enemies.", "+1d8 once per turn");
    }
    if (cls.progression == rules::ProgressionClass::Bard) {
        char count[32];
        std::snprintf(count, sizeof(count), "%u known spells", rules::spellCount(c->build.spells.learned));
        feature("Known Spells", "Choose a known spell in the spell picker. Cast with Charisma.", count);
    }
    if (cls.progression == rules::ProgressionClass::Ranger && c->build.level >= 2) {
        auto& known = out.features[out.featureCount++];
        known.name = "Known Spells";
        known.detail = "Cast directly with Wisdom.";
        for (std::uint16_t n = 0; n < party_.catalog().spellCount(); ++n)
            if (c->build.spells.learned & rules::spellBit({n}))
                append(known.value, party_.catalog().spell({n})->name.c_str());
    }
    if (c->build.level == 4) {
        char summary[64]{};
        constexpr const char* scores[]{"STR", "DEX", "CON", "INT", "WIS", "CHA"};
        for (unsigned n = 0; n < rules::abilityCount; ++n)
            if (c->build.levelFourAsi.increases[n]) {
                char gain[16];
                std::snprintf(gain, sizeof(gain), "%s +%u", scores[n],
                              unsigned(c->build.levelFourAsi.increases[n]));
                const auto used = std::strlen(summary);
                std::snprintf(summary + used, sizeof(summary) - used, "%s%s", used ? ", " : "", gain);
            }
        feature("Level 4 ASI", "Permanent increases; included in ability scores.", summary);
    }
    {
        for (unsigned n = 0; n < unsigned(EffectKind::Count); ++n)
            if (session_.effects().has(i.character, static_cast<EffectKind>(n)))
                append(out.effects, effectName(static_cast<EffectKind>(n)));
        if (const auto concentration = session_.effects().concentrationStatus(i.character))
            append(out.effects, concentration->effect == EffectKind::HuntersMark
                                    ? "Concentrating on Hunter's Mark" : "Concentrating");
    }
    if (!out.effects[0])
        append(out.effects, "None");
}
} // namespace blobber
