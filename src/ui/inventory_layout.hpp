#pragma once
#include "game/inventory_ui.hpp"
#include "ui/layout.hpp"
namespace blobber::inventoryLayout {
constexpr UiRect panel{4, 2, 632, 356}, close{606, 9, 20, 17};
constexpr UiRect levelUp{16, 249, 152, 16};
constexpr UiRect inventoryPage{14, 9, 65, 17}, characterPage{88, 9, 78, 17};
constexpr UiRect tab(int i) {
    return {14 + i * 154, 33, 150, 28};
}
constexpr UiRect cell(int i) {
    return {178 + (i % 8) * 38, 83 + (i / 8) * 38, 35, 35};
}
constexpr UiRect equipment(int i) {
    return i == 0   ? UiRect{71, 143, 40, 40}
           : i == 1 ? UiRect{16, 170, 40, 40}
           : i == 2 ? UiRect{120, 170, 40, 40}
                    : UiRect{16, 237, 40, 40};
}
constexpr UiRect action(int i) {
    return {490, 232 + i * 33, 136, 20};
}
inline UiRect action(const InventoryHud& inventory, InventoryAction kind) {
    int row = 0;
    for (unsigned n = 0; n < static_cast<unsigned>(kind); ++n)
        row += inventory.actions[n].visible;
    return action(row);
}
constexpr UiRect dialog{210, 120, 220, 116}, minus{226, 162, 28, 22}, plus{385, 162, 28, 22};
constexpr UiRect accept{225, 202, 90, 22}, cancel{325, 202, 90, 22};
constexpr UiRect useBack{220, 330, 75, 20}, useCancel{300, 330, 75, 20};
} // namespace blobber::inventoryLayout
