#include "game/perception.hpp"
#include "game/session.hpp"
namespace blobber {
PickupResult GameSession::dropItem(ItemHandle handle, std::uint64_t revision, bool preview) noexcept {
    if (!runActive())
        return {InventoryError::Unavailable, "Campaign has ended."};
    if (!preview)
        if (mutation_ != UINT64_MAX)
            ++mutation_;
    if (!preview)
        events_.clear();
    if (movement().transition.active())
        return {InventoryError::Moving, "Wait until movement finishes."};
    if (!combat_.safeRecovery(movement().player))
        return {InventoryError::Unsafe, "Dropping requires safe exploration."};
    const auto* stack = party_.inventory().stack(handle);
    if (revision != party_.inventory().revision())
        return {InventoryError::Stale, inventoryMessage(InventoryError::Stale)};
    if (!stack)
        return {InventoryError::InvalidItem, inventoryMessage(InventoryError::InvalidItem)};
    const auto cell = movement().player.cell;
    const auto& defs = floor_.definition();
    if (defs.landingCount && defs.npcExcluded[std::size_t(cell.y * map_.width() + cell.x)])
        return {InventoryError::Unavailable, "Keep the protected stairs room clear."};
    const auto ticket = loot_.availableDrop();
    if (!ticket)
        return {InventoryError::Full, "Floor loot is full. Pick up a dropped stack first."};
    const auto item = stack->item;
    const auto quantity = stack->quantity;
    const auto result = party_.releaseItem(handle, revision, preview);
    if (!result)
        return {result.error, inventoryMessage(result.error)};
    if (!preview) {
        loot_.publishDrop(ticket, movement().player.cell, item, quantity);
        combat_.reconcileEffects(timeMs_, events_);
    }
    return {InventoryError::None, "Dropped in a loot bag at your feet."};
}
PickupResult GameSession::lootAccess(LootId id) const noexcept {
    if (!runActive())
        return {InventoryError::Unavailable, "Campaign has ended."};
    const auto* s = loot_.find(id);
    if (!s || loot_.snapshot().revision == UINT64_MAX)
        return {InventoryError::Stale, "That loot bag is no longer available."};
    if (movement().transition.active())
        return {InventoryError::Moving, "Wait until movement finishes."};
    if (!combat_.safeRecovery(movement().player))
        return {InventoryError::Unsafe, "Loot requires safe exploration."};
    if (!(s->cell == movement().player.cell) &&
        !inMeleeFront(map_, movement().player.cell, movement().player.heading, s->cell))
        return {InventoryError::Unavailable, "Move closer and face the loot bag."};
    return {};
}
PickupResult GameSession::pickup(const PickupRequest& r, bool preview) noexcept {
    if (!preview)
        if (mutation_ != UINT64_MAX)
            ++mutation_;
    if (!preview) {
        events_.clear();
        loot_.reconcile(actors_);
    }
    const auto access = lootAccess(r.loot);
    if (!access)
        return access;
    if (!party_.character(r.recipient) || !effects_.canAct(r.recipient))
        return {InventoryError::Unavailable, "Select a conscious, able companion to collect loot."};
    const auto* s = loot_.find(r.loot);
    const auto result = party_.acquire({r.recipient, s->item, s->quantity, r.inventoryRevision}, preview);
    if (!result)
        return {result.error, inventoryMessage(result.error)};
    if (!preview)
        loot_.collect(r.loot);
    return {};
}
RestPreview GameSession::previewRest(const RestRequest& r) const noexcept {
    if (!runActive()) {
        RestPreview out;
        out.error = RestError::Unsafe;
        return out;
    }
    RestPreview p;
    p.elapsedMs = r.kind == RestKind::Short ? 3600000 : 28800000;
    for (std::size_t i = 0; i < recovery_.sanctuaryCount; ++i)
        p.sanctuary |= recovery_.sanctuaries[i] == movement().player.cell;
    for (std::uint16_t c = 0; c < 4; ++c)
        for (std::uint16_t i = 0; i < bagCells; ++i) {
            const auto h = party_.inventory().at({{c}, i});
            const auto* s = party_.inventory().stack(h);
            if (!s || party_.catalog().item(s->item)->kind != rules::ItemKind::Supply)
                continue;
            p.kits += s->quantity;
            if (!p.supply) {
                p.supply = h;
                p.supplier = {c};
            }
        }
    if (p.sanctuary || r.kind == RestKind::Short)
        p.supply = {};
    bool leader = false;
    for (std::uint16_t i = 0; i < 4; ++i)
        leader |= effects_.canAct(rules::CharacterId{i});
    if (r.world != actors_.snapshot().world || r.token != restToken_)
        p.error = RestError::Stale;
    else if (movement().transition.active())
        p.error = RestError::Moving;
    else if (effects_.area(movement().player.cell) || !combat_.settledRecovery(movement().player, timeMs_))
        p.error = RestError::Unsafe;
    else if (!leader)
        p.error = RestError::NoLeader;
    else if (combat_.snapshot().gameplayMs > UINT64_MAX - p.elapsedMs || restToken_ == UINT64_MAX)
        p.error = RestError::ClockFull;
    else
        p.error = party_.recoveryBenefits(r);
    if (p.error == RestError::None && r.kind == RestKind::Long && !p.sanctuary &&
        (!p.supply || party_.inventory().revision() == UINT64_MAX))
        p.error = RestError::NoSupplies;
    return p;
}
RestResult GameSession::rest(const RestRequest& r) noexcept {
    if (mutation_ != UINT64_MAX)
        ++mutation_;
    events_.clear();
    const auto p = previewRest(r);
    if (!p)
        return {p.error};
    const auto out = party_.recover(r, p.supply, rng_);
    if (!out)
        return out;
    ++restToken_;
    movement_.clearPending();
    combat_.restTime(p.elapsedMs, timeMs_, events_);
    loot_.reconcile(actors_);
    reconcileRewards();
    return out;
}
} // namespace blobber
