#pragma once
#include "game/trade.hpp"
#include "ui/layout.hpp"
namespace blobber::tradeLayout {
constexpr UiRect panel{4, 100, 500, 258};
constexpr UiRect cell(TradeSide side, unsigned index) {
    return {int(side == TradeSide::Merchant ? 12 : 264) + int(index % 8) * 28, 132 + int(index / 8) * 28, 26,
            26};
}
constexpr UiRect grid(TradeSide side) {
    return {side == TradeSide::Merchant ? 12 : 264, 132, 222, 166};
}
constexpr UiRect close{482, 105, 15, 15};
inline constexpr unsigned doubleClickMs = 400;
inline constexpr unsigned labelAlpha = 128;
constexpr UiRect previous{203, 105, 13, 12}, next{236, 105, 13, 12};
constexpr UiRect minus{204, 327, 15, 15}, plus{263, 327, 15, 15};
constexpr UiRect accept{372, 326, 121, 19}, back{12, 346, 118, 10};
} // namespace blobber::tradeLayout
