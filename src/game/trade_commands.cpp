#include "game/commands.hpp"
#include "ui/trade_layout.hpp"
#include <algorithm>
#include <cstdio>

namespace blobber {
void CommandController::bindTrade(const ShopCatalog* shops, MerchantLedger* ledger) noexcept {
    closeTrade();
    shops_ = shops;
    merchants_ = ledger;
}
void CommandController::clearTradeHover() noexcept {
    state_.trade.hovered = -1;
    state_.trade.tooltipVisible = false;
    state_.trade.comparison = {};
}
bool CommandController::openTrade() noexcept {
    const auto* c = dialogue_.character();
    const auto* shop = c && shops_ ? shops_->find(c->shop) : nullptr;
    if (!shop || !merchants_ || !dialogue_.active() || dialogue_.leaving() || !session_.inConversation() ||
        state_.suspended || !session_.runActive() || !merchants_->visit(c->id, *shop, session_.gameTime())) {
        state_.log.append("This merchant is unavailable.");
        return false;
    }
    auto& t = state_.trade;
    t = {};
    t.open = true;
    t.resident = c->id;
    t.shop = shop->id;
    t.merchantName = c->name.c_str();
    t.character =
        party_.character(state_.selection.character) ? state_.selection.character : rules::CharacterId{0};
    dialogue_.service();
    syncDialogueSpeech();
    dialogue_.snapshot(state_.dialogue);
    state_.hover = {};
    cancelPointerButtons();
    refreshTrade();
    return true;
}
void CommandController::closeTrade() noexcept {
    state_.trade = {};
    cancelPointerButtons();
}
TradeRequest CommandController::tradeRequest() const noexcept {
    const auto& t = state_.trade;
    TradeRequest request;
    request.resident = t.resident;
    request.shop = t.shop;
    request.character = t.character;
    request.merchantRevision = t.merchantRevision;
    request.inventoryRevision = t.inventoryRevision;
    request.inventoryIdentity = party_.inventory().identity();
    request.side = t.side;
    request.quantity = t.quantity;
    if (t.selected < 48) {
        const auto& cell = (t.side == TradeSide::Merchant ? t.merchant : t.player)[t.selected];
        request.offer = cell.offer;
        request.item = cell.item;
    }
    return request;
}
void CommandController::refreshTrade() noexcept {
    auto& t = state_.trade;
    if (!t.open)
        return;
    const auto* c = dialogue_.character();
    const auto* shop = shops_ ? shops_->find(t.shop) : nullptr;
    if (!c || c->id != t.resident || !shop || !merchants_ || !session_.inConversation() ||
        !session_.runActive() || dialogue_.leaving()) {
        closeTrade();
        return;
    }
    const auto* state = merchants_->visit(t.resident, *shop, session_.gameTime());
    if (!state) {
        closeTrade();
        return;
    }
    if (t.merchantRevision != state->revision || t.inventoryRevision != party_.inventory().revision()) {
        t.selected = 48;
        t.quantity = 1;
        clearTradeHover();
    }
    t.merchantRevision = state->revision;
    t.inventoryRevision = party_.inventory().revision();
    t.gold = characterGold(party_, t.character);
    t.nowMs = session_.gameTime().elapsedMs;
    t.restockMs = state->restockMs;
    t.buybackHours = shop->buybackHours;
    const auto* character = party_.character(t.character);
    t.characterName = character->build.name.c_str();
    t.weight = character->stats.carriedWeight;
    t.capacity = character->stats.carryingCapacity;
    std::array<TradeCell, shopStockCapacity + buybackCapacity> offers{};
    unsigned count = 0;
    for (unsigned i = 0; i < shop->stock.size(); ++i) {
        const auto id = shop->stock[i].item;
        offers[count++] = {
            party_.catalog().item(id), {}, state->stock[id.index], shop->prices[id.index].buy, i};
    }
    for (unsigned i = 0; i < state->buyback.size(); ++i) {
        const auto& b = state->buyback[i];
        if (b.quantity)
            offers[count++] = {party_.catalog().item(b.item), {},   b.quantity, b.price,
                               shopStockCapacity + i,         true, b.expiresMs};
    }
    t.pages = std::max(1u, (count + 47) / 48);
    t.page = std::min(t.page, t.pages - 1);
    t.merchant = {};
    for (unsigned i = 0; i < 48 && t.page * 48 + i < count; ++i)
        t.merchant[i] = offers[t.page * 48 + i];
    for (std::uint16_t i = 0; i < bagCells; ++i) {
        auto& out = t.player[i];
        out = {};
        out.item = party_.inventory().at({t.character, i});
        if (const auto* s = party_.inventory().stack(out.item)) {
            out.definition = party_.catalog().item(s->item);
            out.quantity = s->quantity;
            out.price = shop->prices[s->item.index].sell;
        }
    }
    t.canTrade = false;
    t.quote = {TradeError::Unavailable};
    if (t.selected < 48) {
        const auto& selected = (t.side == TradeSide::Merchant ? t.merchant : t.player)[t.selected];
        t.maximum = std::max(1u, selected.quantity);
        t.quantity = std::clamp(t.quantity, 1u, t.maximum);
        if (selected.definition) {
            t.quote =
                trade(session_.party(), *merchants_, *shops_, tradeRequest(), session_.gameTime(), true);
            t.canTrade = bool(t.quote);
        }
    }
    if (t.hovered >= 0 && t.hovered < 48) {
        const auto& cell = (t.hoverSide == TradeSide::Merchant ? t.merchant : t.player)[unsigned(t.hovered)];
        if (cell.definition) {
            t.tooltipVisible = dialogue_.clockMs() - t.hoverSinceMs >= 350;
            t.comparison = compareItem(party_, t.character, *cell.definition);
        } else
            clearTradeHover();
    }
}
void CommandController::selectTrade(TradeSide side, unsigned cell) noexcept {
    auto& t = state_.trade;
    if (!t.open || cell >= 48)
        return;
    t.side = side;
    t.selected = cell;
    t.quantity = 1;
    t.feedback = {};
    clearTradeHover();
    cancelPointerButtons();
    refreshTrade();
}
void CommandController::tradeCharacter(rules::CharacterId who) noexcept {
    auto& t = state_.trade;
    if (!party_.character(who) || t.character.index == who.index)
        return;
    t.character = who;
    t.selected = 48;
    t.quantity = 1;
    t.feedback = {};
    clearTradeHover();
    cancelPointerButtons();
    refreshTrade();
}
void CommandController::tradeQuantity(int delta) noexcept {
    auto& t = state_.trade;
    t.quantity = unsigned(std::clamp(int(t.quantity) + delta, 1, int(t.maximum)));
    refreshTrade();
}
void CommandController::confirmTrade() noexcept {
    auto& t = state_.trade;
    if (!t.open || !session_.inConversation() || !shops_ || !merchants_ || t.selected >= 48 ||
        state_.suspended || !session_.runActive())
        return;
    const auto result = trade(session_.party(), *merchants_, *shops_, tradeRequest(), session_.gameTime());
    std::snprintf(t.feedback.data(), t.feedback.size(), "%s", tradeMessage(result.error));
    cancelPointerButtons();
    refreshTrade();
}
void CommandController::routeTrade(const UiInput& input) noexcept {
    auto& t = state_.trade;
    if (!t.open)
        return;
    if (!input.focused) {
        clearTradeHover();
        cancelPointerButtons();
        return;
    }
    const auto inside = [&](UiRect rect) { return input.pointer && rect.contains(*input.pointer); };
    const bool click = input.has(UiKey::Click);
    if (input.has(UiKey::Cancel) || input.has(UiKey::Back) || (click && inside(tradeLayout::back))) {
        closeTrade();
        return;
    }
    for (unsigned i = 0; i < 4; ++i)
        if (input.has(UiKey(i)) || (click && inside(hudLayout::card(int(i))))) {
            tradeCharacter(state_.cards[i].id);
            return;
        }
    int pageDelta = 0;
    if (input.has(UiKey::PreviousPage) || (click && inside(tradeLayout::previous)))
        pageDelta = -1;
    if (input.has(UiKey::NextPage) || (click && inside(tradeLayout::next)))
        pageDelta = 1;
    if (input.scroll && input.pointer && input.pointer->x < 254)
        pageDelta = -input.scroll;
    if (pageDelta) {
        t.page = unsigned(std::clamp(int(t.page) + pageDelta, 0, int(t.pages) - 1));
        t.selected = 48;
        t.quantity = 1;
        clearTradeHover();
        cancelPointerButtons();
        refreshTrade();
        return;
    }
    if (input.has(UiKey::Left) || (click && inside(tradeLayout::minus)))
        tradeQuantity(-1);
    else if (input.has(UiKey::Right) || (click && inside(tradeLayout::plus)))
        tradeQuantity(1);
    else if (input.has(UiKey::Confirm) || (click && inside(tradeLayout::accept))) {
        confirmTrade();
        return;
    }
    int hovered = -1;
    TradeSide hoverSide = TradeSide::Merchant;
    for (const auto side : {TradeSide::Merchant, TradeSide::Player})
        for (unsigned cell = 0; cell < 48; ++cell)
            if (inside(tradeLayout::cell(side, cell))) {
                const auto& entry = (side == TradeSide::Merchant ? t.merchant : t.player)[cell];
                if (entry.definition) {
                    hovered = int(cell);
                    hoverSide = side;
                }
                if (click) {
                    selectTrade(side, cell);
                    return;
                }
            }
    if (hovered < 0) {
        clearTradeHover();
        return;
    }
    if (t.hovered != hovered || t.hoverSide != hoverSide) {
        t.hovered = hovered;
        t.hoverSide = hoverSide;
        t.hoverSinceMs = dialogue_.clockMs();
        t.tooltipVisible = false;
    }
    t.pointerX = input.pointer->x;
    t.pointerY = input.pointer->y;
    refreshTrade();
}
} // namespace blobber
