#include "game/inventory.hpp"
#include <algorithm>
#include <atomic>
#include <limits>
#include <stdexcept>
namespace blobber {
namespace {
std::uint64_t nextInventoryIdentity() {
    static std::atomic<std::uint64_t> next{1};
    auto value = next.load(std::memory_order_relaxed);
    for (;;) {
        if (value == UINT64_MAX)
            throw std::runtime_error("Inventory identity exhausted");
        if (next.compare_exchange_weak(value, value + 1, std::memory_order_relaxed))
            return value;
    }
}
} // namespace
Inventory::Inventory() : identity_(nextInventoryIdentity()) {}
const char* inventoryMessage(InventoryError e) noexcept {
    switch (e) {
    case InventoryError::None:
        return "Done.";
    case InventoryError::Stale:
        return "Inventory changed; select the item again.";
    case InventoryError::InvalidItem:
        return "Item is no longer available.";
    case InventoryError::InvalidDestination:
        return "Choose a valid destination.";
    case InventoryError::InvalidQuantity:
        return "That quantity cannot be moved or split.";
    case InventoryError::Full:
        return "Not enough bag or stack space.";
    case InventoryError::Overweight:
        return "That exceeds Strength x 15 lb.";
    case InventoryError::Equipment:
        return "Incompatible equipment or missing armor training.";
    case InventoryError::Unsafe:
        return "Equipment and transfers require safe exploration.";
    case InventoryError::Moving:
        return "Wait until movement finishes.";
    case InventoryError::Unavailable:
        return "Inventory changes are unavailable.";
    }
    return "Invalid inventory request.";
}
const ItemStack* Inventory::stack(ItemHandle h) const noexcept {
    if (h.owner != identity_ || h.slot >= stacks_.size())
        return nullptr;
    const auto& s = stacks_[h.slot];
    return s.quantity && s.generation == h.generation ? &s : nullptr;
}
ItemHandle Inventory::at(ItemLocation l) const noexcept {
    return l.character.index < 4 && l.cell < 52 ? locations_[l.character.index][l.cell] : ItemHandle{};
}
ItemLocation Inventory::locate(ItemHandle h) const noexcept {
    if (stack(h))
        for (std::uint16_t c = 0; c < 4; ++c)
            for (std::uint16_t i = 0; i < 52; ++i)
                if (locations_[c][i] == h)
                    return {{c}, i};
    return {};
}
ItemHandle Inventory::create(rules::ItemId item, unsigned q) noexcept {
    for (std::uint16_t i = 0; i < stacks_.size(); ++i) {
        auto& s = stacks_[i];
        if (!s.quantity && s.generation != UINT64_MAX) {
            s.item = item;
            s.quantity = static_cast<std::uint16_t>(q);
            return {identity_, s.generation, i};
        }
    }
    return {};
}
void Inventory::erase(ItemHandle h) noexcept {
    if (!stack(h))
        return;
    const auto l = locate(h);
    if (l.cell < 52)
        locations_[l.character.index][l.cell] = {};
    auto& s = stacks_[h.slot];
    s.quantity = 0;
    if (s.generation != UINT64_MAX)
        ++s.generation;
}
bool Inventory::consume(ItemHandle h, unsigned q) noexcept {
    const auto* s = stack(h);
    if (!s || !q || q > s->quantity || revision_ == UINT64_MAX)
        return false;
    if (q == s->quantity)
        erase(h);
    else
        stacks_[h.slot].quantity -= q;
    ++revision_;
    return true;
}
rules::CharacterLoadout Inventory::loadout(rules::CharacterId c) const noexcept {
    rules::CharacterLoadout l;
    if (c.index >= 4)
        return l;
    for (std::uint16_t i = 0; i < 52; ++i)
        if (const auto* s = stack(at({c, i})))
            l.items[i] = {s->item, s->quantity};
    l.equipment = {l.items[48].quantity ? rules::OwnedItemId{48} : rules::OwnedItemId{},
                   l.items[49].quantity ? rules::OwnedItemId{49} : rules::OwnedItemId{},
                   l.items[50].quantity ? rules::OwnedItemId{50} : rules::OwnedItemId{},
                   l.items[51].quantity ? rules::OwnedItemId{51} : rules::OwnedItemId{}};
    return l;
}
ItemHandle Inventory::ammunition(rules::CharacterId c, rules::Ammunition kind,
                                 const rules::Catalog& cat) const noexcept {
    if (kind == rules::Ammunition::None)
        return {};
    for (std::uint16_t i = 0; i < bagCells; ++i) {
        const auto h = at({c, i});
        const auto* s = stack(h);
        if (s) {
            const auto* d = cat.item(s->item);
            if (d && d->kind == rules::ItemKind::Ammunition && rules::ammunitionFor(*d) == kind)
                return h;
        }
    }
    return {};
}
unsigned Inventory::count(rules::CharacterId c, rules::Ammunition kind,
                          const rules::Catalog& cat) const noexcept {
    unsigned n = 0;
    for (std::uint16_t i = 0; i < bagCells; ++i)
        if (const auto* s = stack(at({c, i}))) {
            const auto* d = cat.item(s->item);
            if (d && d->kind == rules::ItemKind::Ammunition && rules::ammunitionFor(*d) == kind)
                n += s->quantity;
        }
    return n;
}
void Inventory::initialize(const rules::Catalog& cat, const rules::PresetParty& preset) {
    for (std::uint16_t c = 0; c < 4; ++c) {
        const auto& b = preset.characters[c];
        const std::array<rules::OwnedItemId, 4> eq{b.equipment.body, b.equipment.mainHand,
                                                   b.equipment.offHand, b.equipment.ranged};
        for (std::size_t i = 0; i < b.itemCount; ++i) {
            const auto& item = b.items[i];
            unsigned q = item.quantity;
            const auto* d = cat.item(item.item);
            if (!d || !q)
                throw std::runtime_error("Invalid starting inventory");
            bool equipped = false;
            for (std::uint16_t e = 0; e < 4; ++e)
                if (eq[e].index == i) {
                    if (q != 1 || equipped)
                        throw std::runtime_error("Invalid equipped ownership");
                    locations_[c][48 + e] = create(item.item, 1);
                    equipped = true;
                }
            if (equipped)
                continue;
            for (std::uint16_t k = 0; k < bagCells && q; ++k) {
                const auto* existing = stack(locations_[c][k]);
                if (existing && existing->item.index == item.item.index) {
                    const auto add = std::min(q, rules::stackLimit(*d) - existing->quantity);
                    stacks_[locations_[c][k].slot].quantity += add;
                    q -= add;
                }
            }
            for (std::uint16_t k = 0; k < bagCells && q; ++k)
                if (!locations_[c][k]) {
                    const auto n = std::min(q, rules::stackLimit(*d));
                    const auto h = create(item.item, n);
                    if (!h)
                        throw std::runtime_error("Starting inventory capacity exceeded");
                    locations_[c][k] = h;
                    q -= n;
                }
            if (q)
                throw std::runtime_error("Starting bag capacity exceeded");
        }
    }
}
InventoryResult Inventory::apply(const InventoryRequest& r, const rules::Catalog& cat) noexcept {
    if (r.revision != revision_ || revision_ == UINT64_MAX)
        return {InventoryError::Stale};
    if (!stack(r.source))
        return {InventoryError::InvalidItem};
    const auto result = move(r, cat);
    if (result && result.changed)
        ++revision_;
    return result;
}
InventoryResult Inventory::move(const InventoryRequest& r, const rules::Catalog& cat) noexcept {
    const auto from = locate(r.source);
    const auto to = r.destination;
    if (to.character.index >= 4 || from.cell >= 52)
        return {InventoryError::InvalidDestination};
    const auto original = *stack(r.source);
    const auto* definition = cat.item(original.item);
    if (!definition)
        return {InventoryError::InvalidItem};
    const unsigned q = r.quantity ? r.quantity : original.quantity;
    if (!q || q > original.quantity || (r.operation == InventoryOp::Split && q == original.quantity))
        return {InventoryError::InvalidQuantity};
    if (r.operation == InventoryOp::Transfer) {
        if (to.character.index == from.character.index)
            return {InventoryError::InvalidDestination};
        unsigned remaining = q;
        // Whole-stack transfer preserves the source handle in the first empty destination.
        for (std::uint16_t i = 0; i < bagCells && remaining; ++i) {
            auto h = at({to.character, i});
            const auto* dest = stack(h);
            if (dest && dest->item.index == original.item.index) {
                const auto n = std::min(remaining, rules::stackLimit(*definition) - dest->quantity);
                stacks_[h.slot].quantity += n;
                remaining -= n;
            }
        }
        if (remaining) {
            std::uint16_t empty = 0;
            while (empty < bagCells && at({to.character, empty}))
                ++empty;
            if (empty == bagCells)
                return {InventoryError::Full};
            ItemHandle h;
            if (q == original.quantity) {
                h = r.source;
                locations_[from.character.index][from.cell] = {};
                stacks_[h.slot].quantity = remaining;
            } else {
                h = create(original.item, remaining);
                if (!h)
                    return {InventoryError::Full};
            }
            locations_[to.character.index][empty] = h;
        }
        if (q == original.quantity) {
            if (!remaining)
                erase(r.source);
        } else
            stacks_[r.source.slot].quantity -= q;
        return {InventoryError::None, true};
    }
    if (r.operation != InventoryOp::Move && r.operation != InventoryOp::Split)
        return {InventoryError::Unavailable};
    if (to.cell >= 52 || to.character.index != from.character.index)
        return {InventoryError::InvalidDestination};
    if (from.cell == to.cell)
        return {};
    const auto other = at(to);
    const auto* dest = stack(other);
    if ((from.cell >= bagCells || to.cell >= bagCells) && q != original.quantity)
        return {InventoryError::InvalidQuantity};
    if (dest && to.cell < bagCells && from.cell < bagCells && dest->item.index == original.item.index &&
        rules::stackLimit(*definition) > 1) {
        const auto room = rules::stackLimit(*definition) - dest->quantity;
        const auto n = r.quantity ? q : std::min(q, room);
        if (!n || n > room)
            return {InventoryError::Full};
        stacks_[other.slot].quantity += n;
        if (n == original.quantity)
            erase(r.source);
        else
            stacks_[r.source.slot].quantity -= n;
    } else if (dest) {
        if (q != original.quantity || r.operation == InventoryOp::Split)
            return {InventoryError::InvalidDestination};
        std::swap(locations_[from.character.index][from.cell], locations_[to.character.index][to.cell]);
    } else if (q == original.quantity) {
        locations_[to.character.index][to.cell] = r.source;
        locations_[from.character.index][from.cell] = {};
    } else {
        if (to.cell >= bagCells)
            return {InventoryError::Equipment};
        auto h = create(original.item, q);
        if (!h)
            return {InventoryError::Full};
        locations_[to.character.index][to.cell] = h;
        stacks_[r.source.slot].quantity -= q;
    }
    return {InventoryError::None, true};
}
InventoryResult Inventory::acquire(const AcquisitionRequest& r, const rules::Catalog& cat) noexcept {
    if (r.revision != revision_ || revision_ == UINT64_MAX)
        return {InventoryError::Stale};
    if (r.character.index >= 4)
        return {InventoryError::InvalidDestination};
    const auto* d = cat.item(r.item);
    if (!d)
        return {InventoryError::InvalidItem};
    if (!r.quantity)
        return {InventoryError::InvalidQuantity};
    unsigned remaining = r.quantity;
    for (std::uint16_t i = 0; i < bagCells && remaining; ++i) {
        const auto h = at({r.character, i});
        if (const auto* s = stack(h); s && s->item.index == r.item.index) {
            const unsigned n = std::min(remaining, rules::stackLimit(*d) - s->quantity);
            stacks_[h.slot].quantity += n;
            remaining -= n;
        }
    }
    for (std::uint16_t i = 0; i < bagCells && remaining; ++i) {
        if (at({r.character, i}))
            continue;
        const auto n = std::min(remaining, rules::stackLimit(*d));
        const auto h = create(r.item, n);
        if (!h)
            return {InventoryError::Full};
        locations_[r.character.index][i] = h;
        remaining -= n;
    }
    if (remaining)
        return {InventoryError::Full};
    ++revision_;
    return {InventoryError::None, true};
}
} // namespace blobber
