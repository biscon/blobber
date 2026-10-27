#pragma once
#include "ui/layout.hpp"
namespace blobber::lootLayout {
constexpr UiRect panel{48, 48, 416, 232}, close{440, 56, 16, 16};
constexpr UiRect info{324, 116, 128, 116};
constexpr UiRect cell(unsigned i) {
    return {60 + int(i % 5) * 51, 116 + int(i / 5) * 57, 45, 45};
}
constexpr UiRect previous{60, 234, 24, 15}, next{286, 234, 24, 15};
constexpr UiRect takeAll{290, 253, 78, 18}, dismiss{376, 253, 76, 18};
} // namespace blobber::lootLayout
