#pragma once
#include "ui/layout.hpp"
namespace blobber::menuLayout {
// Native pixels preserve the approved image's footprint independently of font layout.
constexpr UiRect mainTitle(bool canContinue) {
    return {450, canContinue ? 88 : 165, 1020, 134};
}
constexpr UiRect mainHint{210, 302, 220, 22};
constexpr UiRect mainError{20, 328, 600, 25};
constexpr UiRect search{20, 48, 250, 22};
constexpr UiRect available(int i) {
    return {20 + (i % 3) * 130, 80 + (i / 3) * 112, 125, 108};
}
constexpr UiRect formation(int i) {
    return {415 + (i % 2) * 103, 80 + (i / 2) * 108, 98, 106};
}
constexpr UiRect remove(int i) {
    const auto r = formation(i);
    return {r.x, r.y + 88, r.width, 18};
}
inline int dropSlot(ArtPoint p) noexcept {
    for (int i = 0; i < 4; ++i)
        if (formation(i).contains(p))
            return i;
    return -1;
}
} // namespace blobber::menuLayout
