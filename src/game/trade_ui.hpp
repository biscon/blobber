#pragma once
#include "game/trade.hpp"
namespace blobber {
struct TradeCell {
    const rules::ItemDefinition* definition = nullptr;
    ItemHandle item;
    unsigned quantity = 0, price = 0, offer = 0;
    bool buyback = false;
    std::uint64_t expiresMs = 0;
};
struct TradeGesture {
    bool down = false, dragging = false, clickArmed = false;
    unsigned source = 48, lastCell = 48;
    TradeSide sourceSide = TradeSide::Merchant, lastSide = TradeSide::Merchant;
    float startX = 0, startY = 0;
    std::uint64_t lastClickMs = 0;
    TradeRequest request;
};
struct TradeHud {
    bool open = false, canTrade = false;
    DialogueKey resident = 0, shop = 0;
    const char *merchantName = "", *characterName = "";
    rules::CharacterId character;
    std::array<TradeCell, 48> merchant{}, player{};
    std::uint64_t merchantRevision = 0, inventoryRevision = 0, gold = 0, restockMs = 0, nowMs = 0;
    std::int64_t weight = 0, capacity = 0;
    unsigned page = 0, pages = 1, selected = 48, quantity = 1, maximum = 1;
    TradeSide side = TradeSide::Merchant;
    TradeResult quote;
    std::array<char, 160> feedback{};
    int hovered = -1;
    TradeSide hoverSide = TradeSide::Merchant;
    std::uint64_t hoverSinceMs = 0;
    bool tooltipVisible = false;
    float pointerX = 0, pointerY = 0;
    ItemComparison comparison;
    TradeGesture gesture;
};
} // namespace blobber
