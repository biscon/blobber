#include "game/party.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace blobber {
Party::Party(const rules::Catalog& catalog) : catalog_(&catalog) {
    const auto* preset = catalog.preset();
    if (!preset)
        throw std::runtime_error("Starting party: catalog has no preset_party (schema 2 required)");
    inventory_.initialize(catalog, *preset);
    state_.formation = preset->formation;
    for (std::size_t i = 0; i < rules::partySize; ++i) {
        auto& c = state_.characters[i];
        c.build = preset->characters[i];
        const auto stats =
            rules::deriveCharacter(catalog, c.build, inventory_.loadout({static_cast<std::uint16_t>(i)}));
        if (!stats)
            throw std::runtime_error(rules::characterErrorMessage(stats.error));
        c.stats = stats.value;
        c.hitPoints = c.stats.maxHitPoints;
        c.availableHitDice = c.build.level;
        const auto* cls = catalog.characterClass(c.build.characterClass);
        c.mana = c.maxMana = rules::maximumMana(cls->casting, c.build.level);
        c.secondWind = rules::hasFeature(cls->features, rules::ClassFeature::SecondWind);
        c.arcaneRecovery = rules::hasFeature(cls->features, rules::ClassFeature::ArcaneRecovery);
    }
}
InventoryResult Party::stageInventoryChange(const InventoryRequest& request, Inventory& candidate,
                                            std::array<rules::CharacterStats, 4>& stats) const noexcept {
    const auto result = candidate.apply(request, *catalog_);
    if (!result || !result.changed)
        return result;
    for (std::uint16_t i = 0; i < 4; ++i) {
        const auto derived =
            rules::deriveCharacter(*catalog_, state_.characters[i].build, candidate.loadout({i}));
        if (!derived)
            return {derived.error == rules::CharacterError::Overweight ? InventoryError::Overweight
                                                                       : InventoryError::Equipment};
        stats[i] = derived.value;
    }
    return result;
}
InventoryResult Party::previewInventoryChange(const InventoryRequest& request) const noexcept {
    auto candidate = inventory_;
    std::array<rules::CharacterStats, 4> stats;
    return stageInventoryChange(request, candidate, stats);
}
InventoryResult Party::inventoryChange(const InventoryRequest& request) noexcept {
    auto candidate = inventory_;
    std::array<rules::CharacterStats, 4> stats;
    const auto result = stageInventoryChange(request, candidate, stats);
    if (!result || !result.changed)
        return result;
    inventory_ = candidate;
    for (std::uint16_t i = 0; i < 4; ++i)
        state_.characters[i].stats = stats[i];
    return result;
}
InventoryResult Party::releaseItem(ItemHandle handle, std::uint64_t revision, bool preview) noexcept {
    if (revision != inventory_.revision() || revision == UINT64_MAX)
        return {InventoryError::Stale};
    const auto* stack = inventory_.stack(handle);
    const auto location = inventory_.locate(handle);
    if (!stack || location.cell >= 52)
        return {InventoryError::InvalidItem};
    auto candidate = inventory_;
    if (!candidate.consume(handle, stack->quantity))
        return {InventoryError::InvalidItem};
    const auto derived = rules::deriveCharacter(*catalog_, state_.characters[location.character.index].build,
                                                candidate.loadout(location.character));
    if (!derived)
        return {InventoryError::Equipment};
    if (!preview) {
        inventory_ = candidate;
        state_.characters[location.character.index].stats = derived.value;
    }
    return {InventoryError::None, true};
}
bool Party::consumeItem(ItemHandle h, unsigned q) noexcept {
    const auto location = inventory_.locate(h);
    if (location.cell >= bagCells || !inventory_.consume(h, q))
        return false;
    auto& c = state_.characters[location.character.index];
    c.stats = rules::deriveCharacter(*catalog_, c.build, inventory_.loadout(location.character)).value;
    return true;
}
bool Party::restoreMana(rules::CharacterId id, int amount) noexcept {
    if (!character(id) || amount < 0)
        return false;
    auto& c = state_.characters[id.index];
    c.mana += std::min(amount, c.maxMana - c.mana);
    return true;
}
const rules::ItemDefinition* Party::equipped(rules::CharacterId c, EquipmentSlot slot) const noexcept {
    const auto* s = inventory_.stack(inventory_.at({c, static_cast<std::uint16_t>(slot)}));
    return s ? catalog_->item(s->item) : nullptr;
}
const PartyCharacter* Party::character(rules::CharacterId id) const noexcept {
    return id.index < rules::partySize ? &state_.characters[id.index] : nullptr;
}
const PartyCharacter* Party::occupant(FormationSlot slot) const noexcept {
    const auto index = static_cast<unsigned>(slot);
    return index < rules::partySize ? character(state_.formation[index]) : nullptr;
}
bool Party::swapFormation(FormationSlot a, FormationSlot b) noexcept {
    const auto x = static_cast<unsigned>(a), y = static_cast<unsigned>(b);
    if (formationLocked_ || x >= rules::partySize || y >= rules::partySize)
        return false;
    std::swap(state_.formation[x], state_.formation[y]);
    return true;
}
rules::HealthResult Party::commit(rules::CharacterId id, rules::HealthResult result) noexcept {
    if (result && result.changed)
        static_cast<rules::HealthState&>(state_.characters[id.index]) = result.state;
    return result;
}
rules::HealthResult Party::damage(rules::CharacterId id, std::int64_t amount, bool critical) noexcept {
    const auto* c = character(id);
    return c ? commit(id, rules::takeDamage(*c, c->stats.maxHitPoints, amount, critical))
             : rules::HealthResult{{}, rules::HealthError::InvalidCharacter};
}
rules::HealthResult Party::heal(rules::CharacterId id, std::int64_t amount) noexcept {
    const auto* c = character(id);
    return c ? commit(id, rules::receiveHealing(*c, c->stats.maxHitPoints, amount))
             : rules::HealthResult{{}, rules::HealthError::InvalidCharacter};
}
rules::HealthResult Party::stabilize(rules::CharacterId id) noexcept {
    const auto* c = character(id);
    return c ? commit(id, rules::stabilize(*c, c->stats.maxHitPoints))
             : rules::HealthResult{{}, rules::HealthError::InvalidCharacter};
}
rules::HealthResult Party::deathSave(rules::CharacterId id, int face, int modifier) noexcept {
    const auto* c = character(id);
    return c ? commit(id, rules::deathSave(*c, c->stats.maxHitPoints, face, modifier))
             : rules::HealthResult{{}, rules::HealthError::InvalidCharacter};
}
bool Party::hasFeature(rules::CharacterId id, rules::ClassFeature feature) const noexcept {
    const auto* c = character(id);
    return c && rules::hasFeature(catalog_->characterClass(c->build.characterClass)->features, feature) &&
           (feature != rules::ClassFeature::LifeDomain || c->build.lifeDomain);
}
int Party::castingModifier(rules::CharacterId id) const noexcept {
    const auto* c = character(id);
    if (!c)
        return 0;
    const auto casting = catalog_->characterClass(c->build.characterClass)->casting;
    return casting == rules::Casting::None ? 0
                                           : c->stats.modifiers[casting == rules::Casting::Wisdom ? 4 : 3];
}
bool Party::spendMana(rules::CharacterId id, int amount) noexcept {
    const auto* c = character(id);
    if (!c || amount < 0 || amount > c->mana)
        return false;
    state_.characters[id.index].mana -= amount;
    return true;
}
bool Party::useFeature(rules::CharacterId id, rules::Feature feature) noexcept {
    if (!character(id))
        return false;
    auto& c = state_.characters[id.index];
    if (feature == rules::Feature::SecondWind && c.secondWind) {
        c.secondWind = false;
        return true;
    }
    if (feature == rules::Feature::ArcaneRecovery && c.arcaneRecovery && c.mana < c.maxMana) {
        c.arcaneRecovery = false;
        c.mana = std::min(c.maxMana, c.mana + 2 * ((c.build.level + 1) / 2));
        return true;
    }
    return false;
}
} // namespace blobber
