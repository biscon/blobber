#include "game/trade.hpp"
#include "game/party.hpp"
#include <algorithm>
#include <stdexcept>

namespace blobber {
const ShopDefinition* ShopCatalog::find(DialogueKey id) const noexcept {
    for (const auto& s : shops)
        if (s.id == id)
            return &s;
    return nullptr;
}
const MerchantState* MerchantLedger::find(DialogueKey id) const noexcept {
    for (unsigned i = 0; i < count_; ++i)
        if (merchants_[i].resident == id)
            return &merchants_[i];
    return nullptr;
}
MerchantState* MerchantLedger::visit(DialogueKey resident, const ShopDefinition& shop,
                                     GameTime now) noexcept {
    for (unsigned i = 0; i < count_; ++i)
        if (merchants_[i].resident == resident) {
            auto& state = merchants_[i];
            if (state.shop != shop.id)
                return nullptr;
            refresh(state, shop, now);
            return &state;
        }
    if (!resident || !shop.id || count_ == merchantCapacity || shop.stock.size() > shopStockCapacity)
        return nullptr;
    auto& state = merchants_[count_++];
    state = {};
    state.resident = resident;
    state.shop = shop.id;
    state.restockMs = now.after(std::uint64_t(shop.restockDays) * gameDuration::day).elapsedMs;
    for (const auto& entry : shop.stock)
        if (entry.item.index < state.stock.size())
            state.stock[entry.item.index] = entry.quantity;
    return &state;
}
bool MerchantLedger::refresh(MerchantState& state, const ShopDefinition& shop, GameTime now) noexcept {
    if (state.revision >= UINT64_MAX - 1 || state.shop != shop.id)
        return false;
    bool changed = false;
    for (auto& entry : state.buyback)
        if (entry.quantity && now.elapsedMs >= entry.expiresMs) {
            entry = {};
            changed = true;
        }
    const auto period = std::uint64_t(shop.restockDays) * gameDuration::day;
    if (period && now.elapsedMs >= state.restockMs) {
        state.stock = {};
        for (const auto& entry : shop.stock)
            if (entry.item.index < state.stock.size())
                state.stock[entry.item.index] = entry.quantity;
        const auto remainder = (now.elapsedMs - state.restockMs) % period;
        state.restockMs = now.after(period - remainder).elapsedMs;
        changed = true;
    }
    if (changed)
        ++state.revision;
    return changed;
}
std::vector<MerchantState> MerchantLedger::checkpoint() const {
    return {merchants_.begin(), merchants_.begin() + count_};
}
void MerchantLedger::restore(const std::vector<MerchantState>& saved, const rules::Catalog& catalog) {
    if (saved.size() > merchantCapacity)
        throw std::invalid_argument("Too many saved merchants");
    for (unsigned i = 0; i < saved.size(); ++i) {
        const auto& state = saved[i];
        if (!state.resident || !state.shop || !state.revision || state.revision == UINT64_MAX ||
            !state.restockMs || state.restockMs > UINT64_MAX / 2 + 365 * gameDuration::day)
            throw std::invalid_argument("Invalid saved merchant identity/deadline/revision");
        for (unsigned j = 0; j < i; ++j)
            if (saved[j].resident == state.resident)
                throw std::invalid_argument("Duplicate saved merchant");
        for (unsigned item = 0; item < state.stock.size(); ++item)
            if (state.stock[item] > UINT16_MAX ||
                (state.stock[item] &&
                 (!catalog.item({std::uint16_t(item)}) ||
                  catalog.item({std::uint16_t(item)})->kind == rules::ItemKind::Currency)))
                throw std::invalid_argument("Invalid saved merchant stock");
        for (const auto& entry : state.buyback) {
            const auto* item = catalog.item(entry.item);
            if (entry.quantity &&
                (!item || item->kind == rules::ItemKind::Currency ||
                 entry.quantity > rules::stackLimit(*item) || !entry.price || entry.price > 1000000 ||
                 !entry.expiresMs || entry.expiresMs > UINT64_MAX / 2 + gameDuration::day))
                throw std::invalid_argument("Invalid saved buyback stack");
        }
    }
    merchants_ = {};
    count_ = unsigned(saved.size());
    std::copy(saved.begin(), saved.end(), merchants_.begin());
}
std::uint64_t characterGold(const Party& party, rules::CharacterId who) noexcept {
    const auto gold = party.catalog().findItem("gold");
    std::uint64_t total = 0;
    for (std::uint16_t cell = 0; cell < bagCells; ++cell)
        if (const auto* s = party.inventory().stack(party.inventory().at({who, cell}));
            s && s->item.index == gold.index)
            total += s->quantity;
    return total;
}
const char* tradeMessage(TradeError error) noexcept {
    switch (error) {
    case TradeError::None:
        return "Trade completed.";
    case TradeError::Unavailable:
        return "Trading is unavailable.";
    case TradeError::Stale:
        return "Inventory or stock changed; select the item again.";
    case TradeError::Quantity:
        return "Choose an available quantity.";
    case TradeError::SoldOut:
        return "That item is sold out.";
    case TradeError::NotForSale:
        return "The merchant does not buy this item.";
    case TradeError::Funds:
        return "This character does not have enough gold.";
    case TradeError::Full:
        return "Not enough room in this character's bag.";
    case TradeError::Overweight:
        return "This character cannot carry that weight.";
    case TradeError::BuybackFull:
        return "The merchant cannot hold more sold items right now.";
    }
    return "Trading is unavailable.";
}
namespace {
TradeError inventoryError(InventoryError error) noexcept {
    switch (error) {
    case InventoryError::None:
        return TradeError::None;
    case InventoryError::Funds:
        return TradeError::Funds;
    case InventoryError::Full:
        return TradeError::Full;
    case InventoryError::Overweight:
        return TradeError::Overweight;
    case InventoryError::Stale:
        return TradeError::Stale;
    default:
        return TradeError::Unavailable;
    }
}
} // namespace
TradeResult trade(Party& party, MerchantLedger& ledger, const ShopCatalog& catalog, const TradeRequest& r,
                  GameTime now, bool preview) noexcept {
    const auto* shop = catalog.find(r.shop);
    const auto* existing = ledger.find(r.resident);
    if (!shop || !existing || existing->shop != r.shop || !party.character(r.character) ||
        (r.side != TradeSide::Merchant && r.side != TradeSide::Player))
        return {TradeError::Unavailable};
    // Refresh on a local value. Rejected/preview transactions do not mutate either owner.
    auto candidate = *existing;
    MerchantLedger::refresh(candidate, *shop, now);
    if (r.merchantRevision != candidate.revision || candidate.revision >= UINT64_MAX - 1 ||
        r.inventoryRevision != party.inventory().revision() ||
        r.inventoryIdentity != party.inventory().identity())
        return {TradeError::Stale};
    if (!r.quantity || r.quantity > UINT16_MAX)
        return {TradeError::Quantity};
    InventoryExchange exchange;
    exchange.character = r.character;
    exchange.revision = r.inventoryRevision;
    exchange.quantity = r.quantity;
    unsigned price = 0;
    if (r.side == TradeSide::Merchant) {
        if (r.offer < shopStockCapacity) {
            if (r.offer >= shop->stock.size())
                return {TradeError::Stale};
            const auto item = shop->stock[r.offer].item;
            price = shop->prices[item.index].buy;
            auto& quantity = candidate.stock[item.index];
            if (!quantity)
                return {TradeError::SoldOut};
            if (r.quantity > quantity)
                return {TradeError::Quantity};
            quantity -= r.quantity;
            exchange.bought = item;
        } else {
            const auto index = r.offer - shopStockCapacity;
            if (index >= candidate.buyback.size() || !candidate.buyback[index].quantity)
                return {TradeError::Stale};
            auto& entry = candidate.buyback[index];
            if (r.quantity > entry.quantity)
                return {TradeError::Quantity};
            price = entry.price;
            exchange.bought = entry.item;
            entry.quantity -= r.quantity;
            if (!entry.quantity)
                entry = {};
        }
        exchange.gold = -std::int64_t(std::uint64_t(price) * r.quantity);
    } else {
        const auto* s = party.inventory().stack(r.item);
        const auto location = party.inventory().locate(r.item);
        if (!s || location.character.index != r.character.index || location.cell >= bagCells)
            return {TradeError::Stale};
        const auto* item = party.catalog().item(s->item);
        price = shop->prices[s->item.index].sell;
        if (!item || !price || item->kind == rules::ItemKind::Currency)
            return {TradeError::NotForSale};
        if (r.quantity > s->quantity)
            return {TradeError::Quantity};
        const auto buybackPrice = unsigned((std::uint64_t(price) * (100 + shop->markupPercent) + 99) / 100);
        auto slot = std::find_if(candidate.buyback.begin(), candidate.buyback.end(),
                                 [](const auto& entry) { return !entry.quantity; });
        if (slot == candidate.buyback.end())
            return {TradeError::BuybackFull};
        *slot = {s->item, r.quantity, buybackPrice,
                 now.after(std::uint64_t(shop->buybackHours) * gameDuration::hour).elapsedMs};
        exchange.sold = r.item;
        exchange.gold = std::int64_t(std::uint64_t(price) * r.quantity);
    }
    const auto total = std::uint64_t(price) * r.quantity;
    if (!price || total > 1000000000)
        return {TradeError::Quantity};
    const auto result = party.exchange(exchange, preview);
    if (!result)
        return {inventoryError(result.error), price, total};
    if (!preview) {
        ++candidate.revision;
        // The record exists and has the same shop, so visit cannot allocate or fail here.
        *ledger.visit(r.resident, *shop, now) = candidate;
    }
    return {TradeError::None, price, total};
}
} // namespace blobber
