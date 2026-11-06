#pragma once
#include "ui/layout.hpp"
namespace blobber::dialogueLayout {
constexpr UiRect response{4, 224, 306, 134}, topics{314, 224, 190, 134};
constexpr UiRect body{12, 247, 286, 81}, skip{226, 339, 77, 13};
constexpr UiRect previous{320, 345, 18, 10}, next{479, 345, 18, 10};
constexpr UiRect row(unsigned i) {
    return {320, 243 + int(i) * 14, 178, 13};
}
} // namespace blobber::dialogueLayout
