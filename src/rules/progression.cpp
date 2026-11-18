#include "rules/progression.hpp"
#include <algorithm>
namespace blobber::rules {
std::uint32_t nextLevelExperience(int level) noexcept {
    return level == 1 ? 300 : level == 2 ? 900 : level == 3 ? 2700 : 0;
}
const char* subclassName(ProgressionClass cls, int level, bool life) noexcept {
    if (cls == ProgressionClass::Cleric && life)
        return "Life Domain";
    if (cls == ProgressionClass::Wizard && level >= 2)
        return "Evocation";
    if (cls == ProgressionClass::Fighter && level >= 3)
        return "Champion";
    if (cls == ProgressionClass::Rogue && level >= 3)
        return "Thief";
    if (cls == ProgressionClass::Paladin && level >= 3)
        return "Oath of Devotion";
    if (cls == ProgressionClass::Ranger && level >= 3)
        return "Hunter";
    if (cls == ProgressionClass::Bard && level >= 3)
        return "College of Lore";
    if (cls == ProgressionClass::Barbarian && level >= 3)
        return "Path of the Berserker";
    return "None yet";
}
SpellSelectionContext advancementChoices(const Catalog& cat, const CharacterIdentityView& source,
                                         PortraitId portrait, const SpellGrants& baseline,
                                         AdvancementChoices choices, bool learned) noexcept {
    return advancementChoices(cat, source, portrait, baseline, choices,
                              learned ? SpellChoice::Learned : SpellChoice::Prepared);
}
SpellSelectionContext advancementChoices(const Catalog& cat, const CharacterIdentityView& source,
                                         PortraitId portrait, const SpellGrants& baseline,
                                         AdvancementChoices choices, SpellChoice kind) noexcept {
    auto next = source;
    ++next.level;
    if (next.level == 4)
        next.levelFourAsi = choices.abilityImprovement;
    SpellSelectionContext out;
    out.identity = next;
    out.portrait = portrait;
    out.editableFirst = true;
    out.showLocked = false;
    auto grants = baseline;
    grants.learned |= choices.learned;
    grants.domain = domainSpells(cat, next);
    if (kind == SpellChoice::Cantrips) {
        out.locked = baseline.cantrips | baseline.racial;
        const auto* cls = cat.characterClass(source.characterClass);
        if (next.level == 4 && cls && cls->casting != Casting::None && cls->spellProgression == SpellProgression::Full) {
            out.available = classSpells(cat, source.characterClass, 0) & ~out.locked;
            out.minimum = out.maximum = 1;
        }
    } else if (kind == SpellChoice::Learned) {
        out.locked = baseline.learned;
        out.lockedLabel = "Known";
        const auto* cls = cat.characterClass(source.characterClass);
        if (cls &&
            (cls->progression == ProgressionClass::Wizard || cls->progression == ProgressionClass::Bard)) {
            out.available = eligibleLeveledSpells(cat, next) & ~out.locked;
            out.minimum = out.maximum = cls->progression == ProgressionClass::Bard ? 1 : 2;
        } else if (cls && cls->progression == ProgressionClass::Ranger && next.level == 2) {
            out.granted = rangerMarkGrant(cat, next);
            out.locked |= out.granted;
            out.available = eligibleLeveledSpells(cat, next) & ~out.locked;
            out.minimum = out.maximum = 1;
        }
    } else {
        out.locked = baseline.prepared | grants.domain;
        out.lockedLabel = "Prepared";
        out.granted = grants.domain;
        out.available = preparationOptions(cat, next, grants) & ~out.locked;
        out.maximum = unsigned(std::max(0, preparationLimit(cat, next) - preparationLimit(cat, source)));
        out.maximum = std::min(out.maximum, spellCount(out.available));
        if (!preparationLimit(cat, source) && preparationLimit(cat, next))
            out.minimum = 1;
    }
    return out;
}
AdvancementPreview previewAdvancement(const Catalog& cat, const CharacterIdentityView& source,
                                      const SpellGrants& baseline, const CharacterLoadout& gear,
                                      AdvancementChoices choices) noexcept {
    AdvancementPreview out;
    const auto* cls = cat.characterClass(source.characterClass);
    if (!cls || cls->progression == ProgressionClass::None || source.level < 1 || source.level >= 4)
        return out;
    auto next = source;
    ++next.level;
    if (next.level == 4)
        next.levelFourAsi = choices.abilityImprovement;
    out.abilityImprovement = next.levelFourAsi;
    // Reject premature allocations as well as malformed existing identities.
    if (!effectiveAbilities(cat, source) || (next.level < 4 && choices.abilityImprovement.points()) ||
        !effectiveAbilities(cat, next))
        return out;
    const auto stats = deriveCharacter(cat, next, gear);
    out.derivation = stats.error;
    out.stats = stats.value;
    if (!stats)
        return out;
    auto learned = advancementChoices(cat, source, {}, baseline, choices, true);
    auto prepared = advancementChoices(cat, source, {}, baseline, choices, false);
    SpellSelection l, p, can;
    can.context = advancementChoices(cat, source, {}, baseline, choices, SpellChoice::Cantrips);
    can.selected = choices.cantrips;
    l.context = learned;
    l.selected = choices.learned;
    p.context = prepared;
    p.selected = choices.prepared;
    if (!l.valid() || !p.valid() || !can.valid())
        return out;
    out.spells = baseline;
    out.spells.cantrips |= choices.cantrips;
    out.spells.learned |= choices.learned;
    if (cls->progression == ProgressionClass::Ranger) {
        if (next.level == 2)
            out.spells.learned |= learned.granted;
        else if (next.level == 3)
            out.spells.learned = eligibleLeveledSpells(cat, next);
    }
    out.spells.prepared |= choices.prepared;
    out.spells.domain = domainSpells(cat, next);
    if (validateGrants(cat, next, out.spells) != BuildError::None)
        return out;
    out.valid = true;
    return out;
}
} // namespace blobber::rules
