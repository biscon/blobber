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
    if (actionBusy())
        return {InventoryError::Moving,
                opening() ? "Wait until the door finishes moving." : "Wait until movement finishes."};
    if (!combat_.safeRecovery(movement().player))
        return {InventoryError::Unsafe, "Wait until it is safe to drop items."};
    const auto* stack = party_.inventory().stack(handle);
    if (revision != party_.inventory().revision())
        return {InventoryError::Stale, inventoryMessage(InventoryError::Stale)};
    if (!stack)
        return {InventoryError::InvalidItem, inventoryMessage(InventoryError::InvalidItem)};
    const auto cell = movement().player.cell;
    const auto& defs = floor_.definition();
    if (defs.landingCount && defs.npcExcluded[std::size_t(cell.y * map_.width() + cell.x)])
        return {InventoryError::Unavailable, "Keep the entrance clear."};
    const auto ticket = loot_.availableDrop();
    if (!ticket)
        return {InventoryError::Full, "Too many items on the ground. Pick some up first."};
    const auto item = stack->item;
    const auto quantity = stack->quantity;
    const auto result = party_.releaseItem(handle, revision, preview);
    if (!result)
        return {result.error, inventoryMessage(result.error)};
    if (!preview) {
        audio_.emit({"inventory.drop", {}, 0, 0, false, false, true});
        loot_.publishDrop(ticket, movement().player.cell, item, quantity);
        combat_.reconcileEffects(timeMs_, events_);
    }
    return {InventoryError::None, "Dropped in a loot bag at your feet."};
}
ContainerId GameSession::nearbyContainer() const noexcept {
    const auto cell = movement().player.cell + forward(movement().player.heading);
    const auto& s = loot_.snapshot();
    for (unsigned i = 0; i < s.containerCount; ++i)
        if (s.containers[i].cell == movement().player.cell)
            return {s.world, std::uint8_t(i)};
    for (unsigned i = 0; i < s.containerCount; ++i)
        if (s.containers[i].cell == cell)
            return {s.world, std::uint8_t(i)};
    return {};
}
PickupResult GameSession::containerAccess(ContainerId id) const noexcept {
    if (!runActive())
        return {InventoryError::Unavailable, "Campaign has ended."};
    const auto* c = loot_.findContainer(id);
    if (!c || loot_.snapshot().revision == UINT64_MAX)
        return {InventoryError::Stale, "That chest is unavailable."};
    if (actionBusy())
        return {InventoryError::Moving, "Wait until actions finish."};
    if (!combat_.settledRecovery(movement().player, timeMs_) || effects_.area(movement().player.cell))
        return {InventoryError::Unsafe, "Wait until it is safe to open the chest."};
    bool able = false;
    for (std::uint16_t i = 0; i < 4; ++i)
        able |= effects_.canAct(rules::CharacterId{i});
    if (!able)
        return {InventoryError::Unavailable, "An able companion is required."};
    if (c->sealed)
        return {InventoryError::Unavailable, "The guardians' seal holds this chest shut."};
    const auto from = movement().player.cell;
    if (from == c->cell)
        return {};
    if (!inFrontBand(from, movement().player.heading, c->cell) ||
        !surfacePassage(map_.surface(from), map_.surface(c->cell)) || !lineOfSight(map_, from, c->cell))
        return {InventoryError::Unavailable, "Move closer and face the chest."};
    // A diagonal interaction must not reach through occupied or closed corners.
    if (from.x != c->cell.x && from.y != c->cell.y &&
        (map_.movementBlocked({from.x, c->cell.y}) || map_.movementBlocked({c->cell.x, from.y})))
        return {InventoryError::Unavailable, "The chest is obstructed."};
    return {};
}
PickupResult GameSession::openContainer(ContainerId id) noexcept {
    const auto access = containerAccess(id);
    if (!access)
        return access;
    auto& c = loot_.state_.containers[id.slot];
    if (!c.opened) {
        audio_.emit({"chest.open", {}, float(c.cell.x) + .5f, float(c.cell.y) + .5f, true, false, true});
        c.opened = true;
        ++loot_.state_.revision;
        if (mutation_ != UINT64_MAX)
            ++mutation_;
    }
    return {InventoryError::None, "Chest opened."};
}
PickupResult GameSession::lootAccess(LootId id) const noexcept {
    if (!runActive())
        return {InventoryError::Unavailable, "Campaign has ended."};
    const auto* s = loot_.find(id);
    if (!s || loot_.snapshot().revision == UINT64_MAX)
        return {InventoryError::Stale, "That loot bag is no longer available."};
    if (s->container != noContainer) {
        const auto access = containerAccess({id.world, s->container});
        if (!access)
            return access;
        if (!loot_.state_.containers[s->container].opened)
            return {InventoryError::Unavailable, "Open the chest first."};
        return {};
    }
    if (actionBusy())
        return {InventoryError::Moving,
                opening() ? "Wait until the door finishes moving." : "Wait until movement finishes."};
    if (!combat_.safeRecovery(movement().player))
        return {InventoryError::Unsafe, "Wait until it is safe to collect loot."};
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
    if (!preview) {
        audio_.emit({"loot.pickup", {}, 0, 0, false, false, true});
        loot_.collect(r.loot);
    }
    return {};
}
bool GameSession::atSanctuary() const noexcept {
    if (recovery_.freeRestEverywhere)
        return true;
    for (std::size_t i = 0; i < recovery_.sanctuaryCount; ++i)
        if (recovery_.sanctuaries[i] == movement().player.cell)
            return true;
    return false;
}
RestPreview GameSession::previewRest(const RestRequest& r) const noexcept {
    if (conversation_ || !runActive()) {
        RestPreview out;
        out.error = RestError::Unsafe;
        return out;
    }
    RestPreview p;
    p.elapsedMs = r.kind == RestKind::Short ? 3600000 : 28800000;
    p.sanctuary = atSanctuary();
    const auto supplies = party_.campSupplies();
    p.kits = supplies.kits;
    p.supply = supplies.supply;
    p.supplier = supplies.supplier;
    if (p.sanctuary || r.kind == RestKind::Short)
        p.supply = {};
    bool leader = false;
    for (std::uint16_t i = 0; i < 4; ++i)
        leader |= effects_.canAct(rules::CharacterId{i});
    if (r.world != actors_.snapshot().world || r.token != restToken_)
        p.error = RestError::Stale;
    else if (actionBusy())
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
    audio_.emit({"recovery", {}, 0, 0, false, false, true});
    ++restToken_;
    movement_.clearPending();
    combat_.restTime(p.elapsedMs, timeMs_, events_);
    loot_.reconcile(actors_);
    reconcileRewards();
    return out;
}
} // namespace blobber
