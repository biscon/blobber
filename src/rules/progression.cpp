#include "rules/progression.hpp"
namespace blobber::rules {
std::uint32_t nextLevelExperience(int level) noexcept {
    return level == 1 ? 300 : level == 2 ? 900 : 0;
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
    return "None yet";
}
SpellSelectionContext advancementChoices(const Catalog& cat, const CharacterIdentityView& source,
                                         PortraitId portrait, const SpellGrants& baseline,
                                         AdvancementChoices choices, bool learned) noexcept {
    auto next = source;
    ++next.level;
    SpellSelectionContext out;
    out.identity = next;
    out.portrait = portrait;
    out.editableFirst = true;
    out.showLocked = false;
    auto grants = baseline;
    grants.learned |= choices.learned;
    grants.domain = domainSpells(cat, next);
    if (learned) {
        out.locked = baseline.learned;
        out.lockedLabel = "Known";
        const auto* cls = cat.characterClass(source.characterClass);
        if (cls && cls->progression == ProgressionClass::Wizard) {
            out.available = eligibleLeveledSpells(cat, next) & ~out.locked;
            out.minimum = out.maximum = 2;
        }
    } else {
        out.locked = baseline.prepared | grants.domain;
        out.lockedLabel = "Prepared";
        out.granted = grants.domain;
        out.available = preparationOptions(cat, next, grants) & ~out.locked;
        out.maximum = preparationLimit(cat, next) > preparationLimit(cat, source) ? 1 : 0;
    }
    return out;
}
AdvancementPreview previewAdvancement(const Catalog& cat, const CharacterIdentityView& source,
                                      const SpellGrants& baseline, const CharacterLoadout& gear,
                                      AdvancementChoices choices) noexcept {
    AdvancementPreview out;
    const auto* cls = cat.characterClass(source.characterClass);
    if (!cls || cls->progression == ProgressionClass::None || source.level < 1 || source.level >= 3)
        return out;
    auto learned = advancementChoices(cat, source, {}, baseline, choices, true);
    auto prepared = advancementChoices(cat, source, {}, baseline, choices, false);
    SpellSelection l, p;
    l.context = learned;
    l.selected = choices.learned;
    p.context = prepared;
    p.selected = choices.prepared;
    if (!l.valid() || !p.valid())
        return out;
    auto next = source;
    ++next.level;
    out.spells = baseline;
    out.spells.learned |= choices.learned;
    out.spells.prepared |= choices.prepared;
    out.spells.domain = domainSpells(cat, next);
    if (validateGrants(cat, next, out.spells) != BuildError::None)
        return out;
    const auto stats = deriveCharacter(cat, next, gear);
    out.derivation = stats.error;
    out.stats = stats.value;
    out.valid = bool(stats);
    return out;
}
} // namespace blobber::rules
