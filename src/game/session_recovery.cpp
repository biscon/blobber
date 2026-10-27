#include "game/perception.hpp"
#include "game/session.hpp"
namespace blobber {
PickupResult GameSession::pickup(const PickupRequest& r, bool preview) noexcept {
    if (!preview) {
        events_.clear();
        loot_.reconcile(actors_);
    }
    const auto* s = loot_.find(r.loot);
    if (!s || loot_.snapshot().revision == UINT64_MAX)
        return {InventoryError::Stale, "That floor item is no longer available."};
    if (movement().transition.active())
        return {InventoryError::Moving, "Wait until movement finishes."};
    if (!combat_.safeRecovery(movement().player))
        return {InventoryError::Unsafe, "Loot requires safe exploration."};
    if (!party_.character(r.recipient) || !effects_.canAct(r.recipient))
        return {InventoryError::Unavailable, "Select a conscious, able companion to collect loot."};
    if (!(s->cell == movement().player.cell) &&
        !inMeleeFront(map_, movement().player.cell, movement().player.heading, s->cell))
        return {InventoryError::Unavailable, "Move closer and face the item to collect it."};
    const auto result = party_.acquire({r.recipient, s->item, s->quantity, r.inventoryRevision}, preview);
    if (!result)
        return {result.error, inventoryMessage(result.error)};
    if (!preview)
        loot_.collect(r.loot); // Validated stable reservation; no failure after inventory commit.
    return {};
}
RestPreview GameSession::previewRest(const RestRequest& r) const noexcept {
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
    else if (!combat_.settledRecovery(movement().player, timeMs_))
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
    return out;
}
} // namespace blobber
