#pragma once
#include "ui/layout.hpp"
namespace blobber::travelLayout {
constexpr UiRect panel{118, 60, 404, 222};
constexpr UiRect cancel{136, 249, 84, 20}, confirm{278, 249, 224, 20};
constexpr UiRect row(unsigned i) {
    return {136, 116 + int(i) * 27, 366, 23};
}
} // namespace blobber::travelLayout
