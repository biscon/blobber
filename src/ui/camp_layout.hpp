#pragma once
#include "ui/layout.hpp"
namespace blobber::campLayout {
constexpr UiRect art{viewX, viewY, viewWidth, 164};
constexpr UiRect panel{viewX, viewY + 164, viewWidth, viewHeight - 164};
constexpr UiRect shortRest{140, 187, 118, 23}, longRest{266, 187, 118, 23};
constexpr UiRect prepare{350,260,146,16};
constexpr UiRect start{410, 278, 86, 18}, cancel{343, 278, 61, 18};
constexpr UiRect minus(int c) {
    return {29 + c * 119, 239, 16, 16};
}
constexpr UiRect plus(int c) {
    return {90 + c * 119, 239, 16, 16};
}
} // namespace blobber::campLayout
