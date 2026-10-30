#pragma once
#include "rules/spell_selection.hpp"
namespace blobber::rules {
inline constexpr std::uint32_t experienceCap = 900, experienceScale = 12;
std::uint32_t nextLevelExperience(int level) noexcept;
const char* subclassName(ProgressionClass, int level, bool lifeDomain) noexcept;
struct AdvancementChoices {
    std::uint32_t learned = 0, prepared = 0; // New choices only; never baseline masks.
};
struct AdvancementPreview {
    CharacterStats stats{};
    SpellGrants spells{};
    CharacterError derivation = CharacterError::None;
    bool valid = false;
};
AdvancementPreview previewAdvancement(const Catalog&, const CharacterIdentityView&, const SpellGrants&,
                                      const CharacterLoadout&, AdvancementChoices) noexcept;
SpellSelectionContext advancementChoices(const Catalog&, const CharacterIdentityView&, PortraitId,
                                         const SpellGrants&, AdvancementChoices, bool learned) noexcept;
} // namespace blobber::rules
