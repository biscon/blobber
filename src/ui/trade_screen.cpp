#include "ui/hud_painter.hpp"
#include "ui/trade_layout.hpp"
#include <cinttypes>

namespace blobber {
using namespace hudInk;
namespace {
const char* name(const rules::ItemDefinition& item) {
    return item.id == "camp_kit" ? "Ration pack" : item.name.c_str();
}
void remaining(char* output, std::size_t bytes, std::uint64_t deadline, std::uint64_t now) {
    const auto hours = deadline > now ? (deadline - now + gameDuration::hour - 1) / gameDuration::hour : 0;
    if (hours >= 24)
        std::snprintf(output, bytes, "%" PRIu64 "d %" PRIu64 "h", hours / 24, hours % 24);
    else
        std::snprintf(output, bytes, "%" PRIu64 "h", hours);
}
} // namespace
void HudPainter::trade(const HudSnapshot& h) const {
    const auto& t = h.trade;
    const Color labelBackground{background.r, background.g, background.b, tradeLayout::labelAlpha};
    overlayBox(tradeLayout::panel);
    line({254, 104}, {254, 317}, edge);
    clipped(t.merchantName, 12, 106, 183, gold, UiTextStyle::Display);
    char label[160];
    std::snprintf(label, sizeof(label), "%s's bag", t.characterName);
    clipped(label, 264, 106, 210, gold, UiTextStyle::Display);
    std::snprintf(label, sizeof(label), "Gold: %" PRIu64, t.gold);
    rightAligned(label, tradeLayout::gold, gold);
    const int hintWidth =
        fonts ? std::max(0, std::min(140, 216 - int(fonts->width(label, UiTextStyle::Small) + 1))) : 0;
    box(tradeLayout::close);
    centered("X", tradeLayout::close, gold);
    clipped("Stock & buyback", 12, 120, 220, muted, UiTextStyle::Small);
    clipped("Party portraits select trader", 264, 120, hintWidth, muted, UiTextStyle::Small);
    if (t.pages > 1) {
        box(tradeLayout::previous);
        centered("<", tradeLayout::previous, t.page ? gold : muted, UiTextStyle::Small);
        box(tradeLayout::next);
        centered(">", tradeLayout::next, t.page + 1 < t.pages ? gold : muted, UiTextStyle::Small);
        std::snprintf(label, sizeof(label), "%u/%u", t.page + 1, t.pages);
        centered(label, {216, 105, 20, 12}, gold, UiTextStyle::Small);
    }
    for (const auto side : {TradeSide::Merchant, TradeSide::Player}) {
        const auto& cells = side == TradeSide::Merchant ? t.merchant : t.player;
        for (unsigned i = 0; i < cells.size(); ++i) {
            const auto& cell = cells[i];
            const auto r = tradeLayout::cell(side, i);
            const bool selected = t.selected == i && t.side == side;
            box(r, selected ? bright : edge);
            if (!cell.definition)
                continue;
            const auto tint = cell.quantity ? WHITE : Color{100, 100, 100, 130};
            picture(
                UiArt::itemIcon(*cell.definition),
                {r.x + 2, r.y + 1, r.width - 4, side == TradeSide::Merchant ? r.height - 9 : r.height - 3},
                tint);
            if (side == TradeSide::Merchant) {
                fill({r.x + 1, r.y + r.height - 9, r.width - 2, 8}, labelBackground);
                std::snprintf(label, sizeof(label), cell.quantity ? "%ug" : "Out", cell.price);
                centered(label, {r.x + 1, r.y + r.height - 9, r.width - 2, 8}, cell.quantity ? gold : muted,
                         UiTextStyle::Small);
            }
            if (cell.quantity > 1 || side == TradeSide::Merchant) {
                std::snprintf(label, sizeof(label), "%u", cell.quantity);
                const int y = side == TradeSide::Merchant ? r.y + 1 : r.y + r.height - 10;
                fill({r.x + 12, y, 13, 8}, labelBackground);
                rightAligned(label, {r.x + 9, y, 16, 8}, hudInk::text, UiTextStyle::Small);
            }
            if (selected)
                border({r.x + 1, r.y + 1, r.width - 2, r.height - 2}, bright);
        }
    }
    char duration[48];
    remaining(duration, sizeof(duration), t.restockMs, t.nowMs);
    std::snprintf(label, sizeof(label), "Restocks in %s", duration);
    text(label, 12, 302, gold, UiTextStyle::Small);
    std::snprintf(label, sizeof(label), "Weight: %.2f / %.0f lb", double(t.weight) / 100,
                  double(t.capacity) / 100);
    text(label, 264, 302, gold, UiTextStyle::Small);
    clipped("Hover an item to inspect and compare", 264, 313, 230, muted, UiTextStyle::Small);
    line({8, 324}, {500, 324}, edge);
    if (t.selected < 48) {
        const auto& item = (t.side == TradeSide::Merchant ? t.merchant : t.player)[t.selected];
        if (item.definition) {
            clipped(name(*item.definition), 12, 327, 184, gold);
            std::snprintf(label, sizeof(label), "%ug each / %u available", item.price, item.quantity);
            clipped(label, 12, 338, 184, muted, UiTextStyle::Small);
        }
    } else
        clipped("Select an item to buy or sell", 12, 329, 184, muted, UiTextStyle::Small);
    box(tradeLayout::minus);
    centered("-", tradeLayout::minus, t.quantity > 1 ? gold : muted);
    std::snprintf(label, sizeof(label), "Qty %u", t.quantity);
    centered(label, {219, 327, 44, 15}, hudInk::text, UiTextStyle::Small);
    box(tradeLayout::plus);
    centered("+", tradeLayout::plus, t.selected < 48 && t.quantity < t.maximum ? gold : muted);
    std::snprintf(label, sizeof(label), "Total: %" PRIu64 "g", t.quote.total);
    clipped(label, 286, 331, 82, gold, UiTextStyle::Small);
    box(tradeLayout::accept, t.canTrade ? gold : edge);
    std::snprintf(label, sizeof(label), "%s FOR %s", t.side == TradeSide::Merchant ? "BUY" : "SELL",
                  t.characterName);
    label[sizeof(label) - 1] = 0;
    this->label(label, tradeLayout::accept, t.canTrade ? gold : muted, UiTextStyle::Small);
    text("Back to conversation / Esc", 12, 348, gold, UiTextStyle::Small);
    const auto* feedback = t.feedback[0]                    ? t.feedback.data()
                           : t.selected < 48 && !t.canTrade ? tradeMessage(t.quote.error)
                                                            : "";
    clipped(feedback, 144, 348, 350, t.canTrade || t.feedback[0] ? muted : red, UiTextStyle::Small);
}
void HudPainter::tradeTooltip(const HudSnapshot& h) const {
    const auto& t = h.trade;
    if (t.open && t.gesture.dragging && t.gesture.source < 48) {
        const auto& g = t.gesture;
        const auto& cell = (g.sourceSide == TradeSide::Merchant ? t.merchant : t.player)[g.source];
        const auto opposite = g.sourceSide == TradeSide::Merchant ? TradeSide::Player : TradeSide::Merchant;
        const auto destination = tradeLayout::grid(opposite);
        if (destination.contains({t.pointerX, t.pointerY}))
            border(destination, t.canTrade ? teal : red);
        const UiRect ghost{std::clamp(int(t.pointerX) - 12, 4, 474),
                           std::clamp(int(t.pointerY) - 12, 18, 330), 26, 26};
        itemSlot(cell.definition, 0, ghost, gold);
        char quantity[16];
        std::snprintf(quantity, sizeof(quantity), "%u", g.request.quantity);
        fill({ghost.x + 12, ghost.y + 16, 13, 8},
             {background.r, background.g, background.b, tradeLayout::labelAlpha});
        rightAligned(quantity, {ghost.x + 9, ghost.y + 16, 16, 8}, hudInk::text, UiTextStyle::Small);
        return;
    }
    if (!t.open || !t.tooltipVisible || t.hovered < 0 || t.hovered >= 48 || h.suspended)
        return;
    const auto& cell = (t.hoverSide == TradeSide::Merchant ? t.merchant : t.player)[unsigned(t.hovered)];
    if (!cell.definition)
        return;
    char summary[160];
    std::snprintf(summary, sizeof(summary), "%s: %ug each / quantity %u",
                  cell.buyback                         ? "Buyback"
                  : t.hoverSide == TradeSide::Merchant ? "Buy"
                                                       : "Sell",
                  cell.price, cell.quantity);
    itemTooltip(*cell.definition, t.comparison,
                t.hoverSide == TradeSide::Merchant ? "MERCHANT ITEM" : "BAG ITEM", t.characterName,
                {t.pointerX, t.pointerY}, {8, 20, 492, 336},
                cell.price ? summary : "The merchant does not buy this item.",
                cell.buyback ? "Available until you close trading." : nullptr);
}
} // namespace blobber
