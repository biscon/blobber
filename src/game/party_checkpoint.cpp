#include "game/party.hpp"
#include <stdexcept>
namespace blobber {
Inventory::Saved Inventory::checkpoint() const noexcept {
    Saved s;
    s.stacks = stacks_;
    s.revision = revision_;
    for (unsigned c = 0; c < 4; ++c)
        for (unsigned i = 0; i < 52; ++i)
            s.locations[c][i] = locations_[c][i] ? locations_[c][i].slot : UINT16_MAX;
    return s;
}
void Inventory::restore(const Saved& s, const rules::Catalog& catalog) {
    std::array<bool, inventoryCapacity> used{};
    if (!s.revision)
        throw std::invalid_argument("Invalid inventory revision");
    for (const auto& row : s.locations)
        for (auto slot : row) {
            if (slot == UINT16_MAX)
                continue;
            if (slot >= inventoryCapacity || used[slot] || !s.stacks[slot].quantity)
                throw std::invalid_argument("Duplicate or invalid inventory ownership");
            used[slot] = true;
        }
    for (unsigned i = 0; i < inventoryCapacity; ++i) {
        const auto& stack = s.stacks[i];
        const auto* item = catalog.item(stack.item);
        if (!stack.generation ||
            (stack.quantity && (!used[i] || !item || stack.quantity > rules::stackLimit(*item))))
            throw std::invalid_argument("Invalid inventory stack");
    }
    stacks_ = s.stacks;
    revision_ = s.revision;
    for (unsigned c = 0; c < 4; ++c)
        for (unsigned i = 0; i < 52; ++i) {
            auto slot = s.locations[c][i];
            locations_[c][i] =
                slot == UINT16_MAX ? ItemHandle{} : ItemHandle{identity_, stacks_[slot].generation, slot};
        }
}
Party::Saved Party::checkpoint() const {
    return {state_, inventory_.checkpoint(), questKeys_, preparationRevision_, progressionRevision_};
}
Party::Party(const rules::Catalog& catalog, const Saved& s)
    : questKeys_(s.questKeys), catalog_(&catalog), state_(s.state),
      preparationRevision_(s.preparationRevision), progressionRevision_(s.progressionRevision) {
    if (!rules::validFormation(state_.formation) || (questKeys_ & ~std::uint8_t((1u << 2) | (1u << 3))))
        throw std::invalid_argument("Invalid saved party formation or keys");
    inventory_.restore(s.inventory, catalog);
    for (std::uint16_t i = 0; i < 4; ++i) {
        auto& c = state_.characters[i];
        const auto* cls = catalog.characterClass(c.build.characterClass);
        const auto* race = catalog.race(c.build.race);
        if (!rules::validContentId(c.build.id) || !rules::validDisplayText(c.build.name, 64) || !cls ||
            !race || c.build.level < 1 || c.build.level > 3 ||
            rules::validateGrants(catalog, c.build.view(), c.build.spells,
                                  bool(catalog.creation(c.build.characterClass))) != rules::BuildError::None)
            throw std::invalid_argument("Invalid saved character build");
        for (unsigned j = 0; j < i; ++j)
            if (state_.characters[j].build.id == c.build.id)
                throw std::invalid_argument("Duplicate saved character identity");
        if (catalog.creation(c.build.characterClass)) {
            const auto* portrait = rules::portrait(c.build.portrait);
            if (!portrait || portrait->race != race->id)
                throw std::invalid_argument("Invalid saved portrait");
        }
        const auto stats = rules::deriveCharacter(catalog, c.build, inventory_.loadout({i}));
        if (!stats || c.aidBonus < 0 || c.aidBonus > 5)
            throw std::invalid_argument("Invalid saved character equipment or Aid");
        c.stats = stats.value;
        c.stats.maxHitPoints += c.aidBonus;
        c.maxMana = rules::maximumMana(cls->casting, c.build.level);
        if (!rules::validHealth(c, c.stats.maxHitPoints) || c.mana < 0 || c.mana > c.maxMana ||
            c.temporaryUntilMs > UINT64_MAX / 2 || c.availableHitDice < 0 ||
            c.availableHitDice > c.build.level || c.experience > rules::experienceCap ||
            c.experienceFraction >= rules::experienceScale ||
            (c.build.level > 1 && c.experience < rules::nextLevelExperience(c.build.level - 1)) ||
            (c.secondWind && !rules::hasFeature(cls->features, rules::ClassFeature::SecondWind)) ||
            (c.arcaneRecovery && !rules::hasFeature(cls->features, rules::ClassFeature::ArcaneRecovery)) ||
            (c.actionSurge && (c.build.level < 2 || cls->progression != rules::ProgressionClass::Fighter)) ||
            (c.channelDivinity && (c.build.level < 2 || !c.build.lifeDomain)))
            throw std::invalid_argument("Invalid saved character resources");
    }
}
} // namespace blobber
