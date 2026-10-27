#pragma once
#include "ui/layout.hpp"
namespace blobber::inventoryLayout {
constexpr UiRect panel{4, 2, 632, 356}, close{607, 9, 20, 17};
constexpr UiRect tab(int i) {
    return {12 + i * 153, 33, 149, 28};
}
constexpr UiRect cell(int i) {
    return {178 + (i % 8) * 38, 83 + (i / 8) * 38, 35, 35};
}
constexpr UiRect equipment(int i) {
    return i == 0   ? UiRect{68, 108, 40, 40}
           : i == 1 ? UiRect{16, 170, 40, 40}
           : i == 2 ? UiRect{120, 170, 40, 40}
                    : UiRect{68, 237, 40, 40};
}
constexpr UiRect action(int i) {
    return {490, 222 + i * 23, 136, 20};
}
constexpr UiRect dialog{210, 120, 220, 116}, minus{226, 162, 28, 22}, plus{385, 162, 28, 22};
constexpr UiRect accept{225, 202, 90, 22}, cancel{325, 202, 90, 22};
constexpr UiRect inspectorButton{159, 250, 110, 17};
constexpr UiRect useBack{220, 330, 75, 20}, useCancel{300, 330, 75, 20};
} // namespace blobber::inventoryLayout
