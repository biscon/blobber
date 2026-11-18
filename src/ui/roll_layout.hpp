#pragma once
#include "ui/layout.hpp"
namespace blobber::rollLayout {
inline constexpr UiRect panel{125, 110, 390, 132}, keep{333, 207, 101, 22}, spend{206, 207, 101, 22};
constexpr UiRect heldDie(int card) {
    const auto portrait = hudLayout::portrait(card);
    return {portrait.x + 2, portrait.y + portrait.height - 24, 14, 14};
}
constexpr UiRect bard(unsigned index) {
    return {133 + int(index) * 94, 180, 91, 18};
}
} // namespace blobber::rollLayout
