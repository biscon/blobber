#include "game/party.hpp"
#include <algorithm>

namespace blobber {
InventoryResult Party::exchange(const InventoryExchange& r, bool preview) noexcept {
    if (!character(r.character) || r.revision != inventory_.revision() || r.revision == UINT64_MAX)
        return {InventoryError::Stale};
    if (!r.quantity || r.quantity > UINT16_MAX || r.gold < -1000000000ll || r.gold > 1000000000ll)
        return {InventoryError::InvalidQuantity};
    const auto gold = catalog_->findItem("gold");
    const auto* currency = catalog_->item(gold);
    if (!currency || currency->kind != rules::ItemKind::Currency)
        return {InventoryError::Unavailable};
    auto candidate = inventory_;
    if (r.sold) {
        const auto* s = candidate.stack(r.sold);
        const auto where = candidate.locate(r.sold);
        if (!s || where.character.index != r.character.index || where.cell >= bagCells ||
            s->item.index == gold.index || !candidate.consume(r.sold, r.quantity))
            return {InventoryError::InvalidItem};
    }
    if (r.gold < 0) {
        auto remaining = std::uint64_t(-r.gold);
        for (std::uint16_t cell = 0; cell < bagCells && remaining; ++cell) {
            const auto h = candidate.at({r.character, cell});
            const auto* s = candidate.stack(h);
            if (!s || s->item.index != gold.index)
                continue;
            const auto taken = unsigned(std::min<std::uint64_t>(remaining, s->quantity));
            if (!candidate.consume(h, taken))
                return {InventoryError::Stale};
            remaining -= taken;
        }
        if (remaining)
            return {InventoryError::Funds};
    }
    auto acquire = [&](rules::ItemId item, std::uint64_t count) {
        while (count) {
            const auto q = std::uint16_t(std::min<std::uint64_t>(count, UINT16_MAX));
            const auto result = candidate.acquire({r.character, item, q, candidate.revision()}, *catalog_);
            if (!result)
                return result;
            count -= q;
        }
        return InventoryResult{InventoryError::None, true};
    };
    if (r.gold > 0) {
        const auto result = acquire(gold, std::uint64_t(r.gold));
        if (!result)
            return result;
    }
    if (catalog_->item(r.bought)) {
        const auto result = acquire(r.bought, r.quantity);
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
        auto& c = state_.characters[r.character.index];
        c.stats = derived.value;
        c.stats.maxHitPoints += c.aidBonus;
    }
    return {InventoryError::None, true};
}
} // namespace blobber
