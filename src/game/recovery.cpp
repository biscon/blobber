#include "game/party.hpp"
#include "rules/spell_selection.hpp"
#include <algorithm>
namespace blobber {
const char* restMessage(RestError e) noexcept {
    switch (e) {
    case RestError::None:
        return "Ready to rest.";
    case RestError::Stale:
        return "Rest request expired; open camp again.";
    case RestError::Unsafe:
        return "Rest requires safe exploration and settled attacks.";
    case RestError::Moving:
        return "Wait until movement finishes.";
    case RestError::Dying:
        return "Stabilize dying companions before resting.";
    case RestError::NoLeader:
        return "A conscious, able companion must lead camp.";
    case RestError::InvalidDice:
        return "Choose available Hit Dice for injured living companions.";
    case RestError::NoBenefit:
        return "No recovery needed. Choose Hit Dice or another rest.";
    case RestError::NoSupplies:
        return "A field long rest requires one camp kit.";
    case RestError::InvalidPreparation:
        return "Choose legal prepared spells for living casters.";
    case RestError::PreparationFull:
        return "Preparation revision capacity exhausted.";
    case RestError::ClockFull:
        return "Rest time capacity exhausted.";
    }
    return "Rest unavailable.";
}
CampSupplies Party::campSupplies() const noexcept {
    CampSupplies out;
    for (std::uint16_t c = 0; c < rules::partySize; ++c)
        for (std::uint16_t i = 0; i < bagCells; ++i) {
            const auto h = inventory_.at({{c}, i});
            const auto* stack = inventory_.stack(h);
            if (!stack || catalog_->item(stack->item)->kind != rules::ItemKind::Supply)
                continue;
            out.kits += stack->quantity;
            if (!out.supply) {
                out.supply = h;
                out.supplier = {c};
            }
        }
    return out;
}
bool Party::stageCampSupply(ItemHandle h, std::uint64_t revision, Inventory& candidate,
                            rules::CharacterId& supplier, rules::CharacterStats& stats) const noexcept {
    const auto* stack = inventory_.stack(h);
    const auto location = inventory_.locate(h);
    if (revision != inventory_.revision() || !stack || location.cell >= bagCells ||
        catalog_->item(stack->item)->kind != rules::ItemKind::Supply)
        return false;
    candidate = inventory_;
    if (!candidate.consume(h, 1))
        return false;
    supplier = location.character;
    const auto derived = rules::deriveCharacter(*catalog_, state_.characters[supplier.index].build,
                                                candidate.loadout(supplier));
    if (!derived)
        return false;
    stats = derived.value;
    stats.maxHitPoints += state_.characters[supplier.index].aidBonus;
    return true;
}
InventoryResult Party::acquire(const AcquisitionRequest& request, bool preview) noexcept {
    auto candidate = inventory_;
    const auto result = candidate.acquire(request, *catalog_);
    if (!result)
        return result;
    const auto id = request.character;
    const auto derived =
        rules::deriveCharacter(*catalog_, state_.characters[id.index].build, candidate.loadout(id));
    if (!derived)
        return {derived.error == rules::CharacterError::Overweight ? InventoryError::Overweight
                                                                   : InventoryError::Equipment};
    if (!preview) {
        inventory_ = candidate;
        state_.characters[id.index].stats = derived.value;
        state_.characters[id.index].stats.maxHitPoints += state_.characters[id.index].aidBonus;
    }
    return result;
}
PreparationRequest Party::preparationRequest() const noexcept {
    PreparationRequest p;
    p.enabled = true;
    p.inventoryRevision = inventory_.revision();
    p.revision = preparationRevision_;
    for (unsigned i = 0; i < 4; ++i)
        p.prepared[i] = state_.characters[i].build.spells.prepared;
    return p;
}
bool Party::alternativePreparation(RestRequest& r) const noexcept {
    r.preparation = preparationRequest();
    for (unsigned i = 0; i < 4; ++i) {
        const auto& ch = state_.characters[i];
        if (ch.life == rules::Life::Dead || ch.life == rules::Life::Dying)
            continue;
        auto options = rules::preparationOptions(*catalog_, ch.build.view(), ch.build.spells);
        for (std::uint16_t j = 0; j < catalog_->spellCount(); ++j) {
            const auto candidate = options & rules::spellBit({j});
            if (candidate != ch.build.spells.prepared &&
                rules::validPreparation(*catalog_, ch.build.view(), ch.build.spells, candidate)) {
                r.preparation.prepared[i] = candidate;
                return true;
            }
        }
    }
    r.preparation = {};
    return false;
}
RestError Party::recoveryBenefits(const RestRequest& r) const noexcept {
    if (r.kind != RestKind::Short && r.kind != RestKind::Long)
        return RestError::InvalidDice;
    bool benefit = false;
    if (r.preparation.enabled) {
        if (r.kind != RestKind::Long)
            return RestError::InvalidPreparation;
        if (r.preparation.revision != preparationRevision_ ||
            r.preparation.inventoryRevision != inventory_.revision())
            return RestError::Stale;
        for (unsigned i = 0; i < 4; ++i) {
            const auto& c = state_.characters[i];
            if (r.preparation.prepared[i] == c.build.spells.prepared)
                continue;
            if (c.life == rules::Life::Dead || c.life == rules::Life::Dying ||
                !rules::validPreparation(*catalog_, c.build.view(), c.build.spells,
                                         r.preparation.prepared[i]))
                return RestError::InvalidPreparation;
            benefit = true;
        }
        if (benefit && preparationRevision_ == UINT64_MAX)
            return RestError::PreparationFull;
    }
    for (std::uint16_t i = 0; i < 4; ++i) {
        const auto& c = state_.characters[i];
        if (c.life == rules::Life::Dying)
            return RestError::Dying;
        if (r.dice[i] && (r.kind != RestKind::Short || c.life == rules::Life::Dead ||
                          r.dice[i] > c.availableHitDice || c.hitPoints == c.stats.maxHitPoints))
            return RestError::InvalidDice;
        if (c.life == rules::Life::Dead)
            continue;
        benefit |= c.build.level >= 2 &&
                   ((progressionClass({i}) == rules::ProgressionClass::Fighter && !c.actionSurge) ||
                    (progressionClass({i}) == rules::ProgressionClass::Cleric && c.build.lifeDomain &&
                     !c.channelDivinity));
        benefit |= r.dice[i] || (hasFeature({i}, rules::ClassFeature::SecondWind) && !c.secondWind);
        if (r.kind == RestKind::Long)
            benefit |= c.hitPoints < c.stats.maxHitPoints || c.mana < c.maxMana ||
                       c.availableHitDice < c.build.level ||
                       (hasFeature({i}, rules::ClassFeature::ArcaneRecovery) && !c.arcaneRecovery);
    }
    return benefit ? RestError::None : RestError::NoBenefit;
}
RestResult Party::recover(const RestRequest& r, ItemHandle supply, rules::DiceRng& rng) noexcept {
    RestResult out;
    out.error = recoveryBenefits(r);
    if (out.error != RestError::None)
        return out;
    if (supply) {
        const auto* stack = inventory_.stack(supply);
        if (r.kind != RestKind::Long || !stack ||
            catalog_->item(stack->item)->kind != rules::ItemKind::Supply || !consumeItem(supply))
            return {RestError::NoSupplies};
        out.usedSupply = true;
    }
    if (r.preparation.enabled) {
        for (unsigned i = 0; i < 4; ++i) {
            auto& prepared = state_.characters[i].build.spells.prepared;
            out.preparationChanged |= prepared != r.preparation.prepared[i];
            prepared = r.preparation.prepared[i];
        }
        if (out.preparationChanged)
            ++preparationRevision_;
    }
    // Every failure precedes RNG and recovery mutation. All class dice are validated at loading.
    for (std::uint16_t i = 0; i < 4; ++i) {
        auto& c = state_.characters[i];
        if (c.life == rules::Life::Dead)
            continue;
        const int beforeHp = c.hitPoints, beforeMana = c.mana;
        if (r.kind == RestKind::Short) {
            const auto sides = catalog_->characterClass(c.build.characterClass)->hitDie;
            for (unsigned n = 0; n < r.dice[i]; ++n) {
                const auto amount =
                    std::max<std::int64_t>(0, rng.roll({1, sides, c.stats.modifiers[2]}).value.total);
                heal({i}, amount);
                --c.availableHitDice;
                ++out.diceSpent[i];
            }
        } else {
            heal({i}, c.stats.maxHitPoints);
            c.mana = c.maxMana;
            c.availableHitDice = std::min(c.build.level, c.availableHitDice + std::max(1, c.build.level / 2));
            if (hasFeature({i}, rules::ClassFeature::ArcaneRecovery))
                c.arcaneRecovery = true;
        }
        if (hasFeature({i}, rules::ClassFeature::SecondWind))
            c.secondWind = true;
        if (c.build.level >= 2) {
            if (progressionClass({i}) == rules::ProgressionClass::Fighter)
                c.actionSurge = true;
            if (progressionClass({i}) == rules::ProgressionClass::Cleric && c.build.lifeDomain)
                c.channelDivinity = true;
        }
        out.healed[i] = c.hitPoints - beforeHp;
        out.mana[i] = c.mana - beforeMana;
    }
    return out;
}
} // namespace blobber
