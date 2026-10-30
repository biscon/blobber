#include "game/party.hpp"
#include <algorithm>
namespace blobber {
const char* advancementMessage(AdvancementError e) noexcept {
    switch (e) {
    case AdvancementError::None:
        return "Ready to level up.";
    case AdvancementError::Stale:
        return "Your character changed. Reopen Level Up.";
    case AdvancementError::Unavailable:
        return "Only conscious supported characters below level 3 can advance.";
    case AdvancementError::Experience:
        return "Earn enough XP for the next level first.";
    case AdvancementError::Unsafe:
        return "Level up during settled, safe exploration away from hazards.";
    case AdvancementError::Moving:
        return "Wait until movement finishes.";
    case AdvancementError::Choices:
        return "Complete the newly earned spell choices.";
    case AdvancementError::Capacity:
        return "Progression revision capacity exhausted.";
    }
    return "Advancement unavailable.";
}
rules::ProgressionClass Party::progressionClass(rules::CharacterId id) const noexcept {
    const auto* c = character(id);
    const auto* cls = c ? catalog_->characterClass(c->build.characterClass) : nullptr;
    return cls ? cls->progression : rules::ProgressionClass::None;
}
AdvancementRequest Party::advancementRequest(rules::CharacterId id) const noexcept {
    AdvancementRequest r;
    r.character = id;
    r.revision = progressionRevision_;
    r.inventoryRevision = inventory_.revision();
    r.preparationRevision = preparationRevision_;
    if (const auto* c = character(id))
        r.level = c->build.level;
    return r;
}
AdvancementError Party::advancementEligibility(const AdvancementRequest& r) const noexcept {
    const auto* c = character(r.character);
    if (!c || c->life != rules::Life::Conscious || c->build.level >= 3 ||
        progressionClass(r.character) == rules::ProgressionClass::None)
        return AdvancementError::Unavailable;
    if (r.level != c->build.level || r.revision != progressionRevision_ ||
        r.inventoryRevision != inventory_.revision() || r.preparationRevision != preparationRevision_)
        return AdvancementError::Stale;
    if (progressionRevision_ == UINT64_MAX || preparationRevision_ == UINT64_MAX)
        return AdvancementError::Capacity;
    if (c->experience < rules::nextLevelExperience(c->build.level))
        return AdvancementError::Experience;
    return AdvancementError::None;
}
AdvancementResult Party::advance(const AdvancementRequest& r, bool preview) noexcept {
    AdvancementResult result;
    result.error = advancementEligibility(r);
    if (!result)
        return result;
    auto& c = state_.characters[r.character.index];
    result.preview = rules::previewAdvancement(*catalog_, c.build.view(), c.build.spells,
                                               inventory_.loadout(r.character), r.choices);
    if (!result.preview.valid) {
        result.error = AdvancementError::Choices;
        return result;
    }
    result.preview.stats.maxHitPoints += c.aidBonus;
    if (preview)
        return result;
    const auto oldMax = c.stats.maxHitPoints;
    c.stats = result.preview.stats;
    c.hitPoints += c.stats.maxHitPoints - oldMax;
    ++c.build.level;
    c.build.spells = result.preview.spells;
    const auto mana =
        rules::maximumMana(catalog_->characterClass(c.build.characterClass)->casting, c.build.level);
    c.mana += mana - c.maxMana;
    c.maxMana = mana;
    ++c.availableHitDice;
    if (c.build.level == 2) {
        c.actionSurge = progressionClass(r.character) == rules::ProgressionClass::Fighter;
        c.channelDivinity =
            progressionClass(r.character) == rules::ProgressionClass::Cleric && c.build.lifeDomain;
    }
    ++progressionRevision_;
    ++preparationRevision_;
    return result;
}
void Party::awardExperience(std::uint32_t reward) noexcept {
    unsigned living = 0;
    for (const auto& c : state_.characters)
        living += c.life != rules::Life::Dead;
    if (!living)
        return;
    const auto share = std::uint64_t(reward) * rules::experienceScale / living;
    for (auto& c : state_.characters) {
        if (c.life == rules::Life::Dead)
            continue;
        const auto units = std::min<std::uint64_t>(rules::experienceCap * rules::experienceScale,
                                                   std::uint64_t(c.experience) * rules::experienceScale +
                                                       c.experienceFraction + share);
        c.experience = std::uint32_t(units / rules::experienceScale);
        c.experienceFraction = std::uint8_t(units % rules::experienceScale);
    }
}
bool Party::grantNextLevelExperience() noexcept {
    bool changed = false;
    for (auto& c : state_.characters) {
        const auto next = rules::nextLevelExperience(c.build.level);
        if (c.life != rules::Life::Dead && next && c.experience < next) {
            c.experience = next;
            c.experienceFraction = 0;
            changed = true;
        }
    }
    return changed;
}
std::array<int, 4> Party::preserveLifePreview(rules::CharacterId id) const noexcept {
    std::array<int, 4> out{};
    const auto* caster = character(id);
    if (!caster || !caster->channelDivinity)
        return out;
    for (int n = 0; n < 5 * caster->build.level; ++n) {
        int best = -1;
        for (int i = 0; i < 4; ++i) {
            const auto& c = state_.characters[i];
            if (c.life == rules::Life::Dead || c.hitPoints + out[i] >= c.stats.maxHitPoints / 2)
                continue;
            if (best < 0 || (c.hitPoints + out[i]) * state_.characters[best].stats.maxHitPoints <
                                (state_.characters[best].hitPoints + out[best]) * c.stats.maxHitPoints)
                best = i;
        }
        if (best < 0)
            break;
        ++out[best];
    }
    return out;
}
} // namespace blobber
