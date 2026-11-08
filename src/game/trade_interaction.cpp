#include "game/commands.hpp"
#include "ui/trade_layout.hpp"
#include <algorithm>

namespace blobber {
void CommandController::tradeClick(TradeSide side, unsigned cell) noexcept {
    auto& t = state_.trade;
    auto& g = t.gesture;
    const auto now = dialogue_.clockMs();
    const bool twice = g.clickArmed && g.lastSide == side && g.lastCell == cell && now >= g.lastClickMs &&
                       now - g.lastClickMs <= tradeLayout::doubleClickMs;
    selectTrade(side, cell);
    if (twice) {
        confirmTrade();
        return;
    }
    const auto& entry = (side == TradeSide::Merchant ? t.merchant : t.player)[cell];
    g.clickArmed = entry.definition && entry.quantity;
    g.lastCell = cell;
    g.lastSide = side;
    g.lastClickMs = now;
}
void CommandController::routeTrade(const UiInput& input) noexcept {
    auto& t = state_.trade;
    if (!t.open)
        return;
    if (!input.focused) {
        clearTradeHover();
        clearTradeGesture();
        cancelPointerButtons();
        return;
    }
    const auto inside = [&](UiRect rect) { return input.pointer && rect.contains(*input.pointer); };
    const bool click = input.has(UiKey::Click);
    if (input.has(UiKey::Cancel) || (click && (inside(tradeLayout::back) || inside(tradeLayout::close)))) {
        closeTrade();
        return;
    }
    if (input.has(UiKey::RightClick)) {
        clearTradeGesture();
        clearTradeHover();
        return;
    }
    for (unsigned i = 0; i < 4; ++i)
        if (input.has(UiKey(i)) || (click && inside(hudLayout::card(int(i))))) {
            clearTradeGesture();
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
        clearTradeGesture();
        cancelPointerButtons();
        refreshTrade();
        return;
    }
    auto& g = t.gesture;
    if (input.pointer) {
        t.pointerX = input.pointer->x;
        t.pointerY = input.pointer->y;
    } else {
        clearTradeHover();
        clearTradeGesture();
    }
    if (g.down) {
        // An armed drag owns pointer release. Keyboard confirmation cannot turn
        // an unfinished drag into a separate button transaction.
        if (!input.pointer || input.has(UiKey::Confirm) || input.has(UiKey::Left) ||
            input.has(UiKey::Right)) {
            clearTradeGesture();
            return;
        }
        const float dx = t.pointerX - g.startX, dy = t.pointerY - g.startY;
        if (dx * dx + dy * dy >= 9) {
            g.dragging = true;
            g.clickArmed = false;
        }
        if (input.pointerReleased) {
            const auto source = g.source;
            const auto side = g.sourceSide;
            const auto pending = g.request;
            const bool dragging = g.dragging;
            g.down = g.dragging = false;
            if (dragging) {
                clearTradeGesture();
                const auto destination =
                    side == TradeSide::Merchant ? TradeSide::Player : TradeSide::Merchant;
                if (inside(tradeLayout::grid(destination)))
                    executeTrade(pending);
            } else if (inside(tradeLayout::cell(side, source)))
                tradeClick(side, source);
            else
                clearTradeGesture();
            return;
        }
        if (!input.pointerHeld)
            clearTradeGesture();
        clearTradeHover();
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
                    if (input.pointerHeld && entry.definition && entry.quantity) {
                        if (g.lastSide != side || g.lastCell != cell)
                            g.clickArmed = false;
                        selectTrade(side, cell);
                        g.down = true;
                        g.dragging = false;
                        g.source = cell;
                        g.sourceSide = side;
                        g.startX = t.pointerX;
                        g.startY = t.pointerY;
                        g.request = tradeRequest();
                    } else if (!input.pointerHeld)
                        tradeClick(side, cell); // Completed click from a logical input client.
                    else {
                        clearTradeGesture();
                        selectTrade(side, cell);
                    }
                    return;
                }
            }
    if (input.pointerHeld)
        g.clickArmed = false; // A press on another control/outside interrupts the sequence.
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
    refreshTrade();
}
} // namespace blobber
