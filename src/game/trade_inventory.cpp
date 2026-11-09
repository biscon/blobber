#include "game/party.hpp"

namespace blobber {
InventoryResult Party::exchange(const InventoryExchange& r, bool preview) noexcept {
    if (!character(r.character) || r.revision != inventory_.revision() || r.revision == UINT64_MAX)
        return {InventoryError::Stale};
    if (!r.quantity || r.quantity > UINT16_MAX || r.gold < -1000000000ll || r.gold > 1000000000ll)
        return {InventoryError::InvalidQuantity};
    auto purse = gold_;
    if (r.gold < 0) {
        const auto payment = std::uint64_t(-r.gold);
        if (payment > purse)
            return {InventoryError::Funds};
        purse -= payment;
    } else {
        const auto proceeds = std::uint64_t(r.gold);
        if (purse > UINT64_MAX - proceeds)
            return {InventoryError::GoldOverflow};
        purse += proceeds;
    }
    auto candidate = inventory_;
    if (r.sold) {
        const auto* s = candidate.stack(r.sold);
        const auto where = candidate.locate(r.sold);
        if (!s || where.character.index != r.character.index || where.cell >= bagCells ||
            catalog_->item(s->item)->kind == rules::ItemKind::Currency ||
            !candidate.consume(r.sold, r.quantity))
            return {InventoryError::InvalidItem};
    }
    if (const auto* bought = catalog_->item(r.bought)) {
        if (bought->kind == rules::ItemKind::Currency)
            return {InventoryError::InvalidItem};
        const auto result = candidate.acquire(
            {r.character, r.bought, std::uint16_t(r.quantity), candidate.revision()}, *catalog_);
        if (!result)
            return result;
    } else if (!r.sold)
        return {InventoryError::InvalidItem};
    const auto derived =
        rules::deriveCharacter(*catalog_, character(r.character)->build, candidate.loadout(r.character));
    if (!derived)
        return {derived.error == rules::CharacterError::Overweight ? InventoryError::Overweight
                                                                   : InventoryError::Equipment};
    if (!preview) {
        inventory_ = candidate;
        gold_ = purse;
        auto& c = state_.characters[r.character.index];
        auto stats = derived.value;
        stats.maxHitPoints += c.aidBonus;
        commitStats(r.character, stats);
    }
    return {InventoryError::None, true};
}
} // namespace blobber
