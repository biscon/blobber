#pragma once
#include "game/inventory_ui.hpp"
#include "ui/layout.hpp"
#include <cstdlib>
namespace blobber::inventoryLayout {
constexpr UiRect panel{4, 2, 632, 356}, close{606, 9, 20, 17};
// rightAligned has a two-unit inset; compensate so text ends at the grid border.
constexpr UiRect gold{178, 72, 303, 10};
constexpr UiRect levelUp{16, 249, 152, 16};
constexpr UiRect inventoryPage{14, 9, 65, 17}, characterPage{88, 9, 78, 17};
constexpr UiRect tab(int i) {
    return {14 + i * 154, 33, 150, 28};
}
constexpr bool isTab(UiRect r) {
    const auto same = [r](UiRect b) {
        return r.x == b.x && r.y == b.y && r.width == b.width && r.height == b.height;
    };
    if (same(inventoryPage) || same(characterPage))
        return true;
    for (int i = 0; i < 4; ++i)
        if (same(tab(i)))
            return true;
    return false;
}
constexpr UiRect cell(int i) {
    return {178 + (i % 8) * 38, 83 + (i / 8) * 38, 35, 35};
}
// Slot captions are inside their own card. Storage order retains the original four indices.
inline constexpr std::array<UiRect, equipmentSlots> equipmentFrames{{{16, 120, 40, 36},
                                                                     {20, 272, 40, 36},
                                                                     {71, 272, 40, 36},
                                                                     {122, 272, 40, 36},
                                                                     {71, 82, 40, 36},
                                                                     {16, 194, 40, 36},
                                                                     {71, 231, 40, 36},
                                                                     {16, 157, 40, 36},
                                                                     {126, 157, 40, 36},
                                                                     {16, 231, 40, 36},
                                                                     {126, 231, 40, 36},
                                                                     {126, 120, 40, 36},
                                                                     {126, 194, 40, 36}}};
constexpr UiRect equipmentFrame(int i) {
    return i >= 0 && unsigned(i) < equipmentFrames.size() ? equipmentFrames[unsigned(i)] : UiRect{};
}
constexpr UiRect equipment(int i) {
    const auto frame = equipmentFrame(i);
    return frame.width ? UiRect{frame.x + 8, frame.y + 9, 24, 24} : UiRect{};
}
constexpr UiRect equipmentLabel(int i) {
    const auto frame = equipmentFrame(i);
    return {frame.x + 1, frame.y + 1, frame.width - 2, 8};
}
inline constexpr UiRect silhouette{54, 119, 74, 112};
inline constexpr std::array<const char*, equipmentSlots> equipmentLabels{
    "BODY", "MAIN HAND", "SHIELD", "RANGED", "HEAD",   "HANDS", "FEET",
    "BELT", "CLOAK",     "RING 1", "RING 2", "AMULET", "WRISTS"};
struct EquipmentLeader {
    int slot;
    ArtPoint start, elbow, target;
};
// Anatomical order on each side prevents crossings. Feet shares one fork to both shoes.
// Landmarks account for UiArt's aspect-fit padding inside the silhouette rectangle.
inline constexpr std::array<EquipmentLeader, 11> equipmentLeaders{{{4, {91, 118}, {91, 119}, {91, 123}},
                                                                   {0, {56, 141}, {68, 141}, {80, 149}},
                                                                   {7, {56, 178}, {62, 178}, {80, 164}},
                                                                   {5, {56, 215}, {62, 215}, {67, 178}},
                                                                   {9, {56, 252}, {63, 252}, {68, 184}},
                                                                   {11, {126, 141}, {110, 141}, {91, 139}},
                                                                   {8, {126, 178}, {120, 178}, {104, 144}},
                                                                   {12, {126, 215}, {122, 215}, {114, 178}},
                                                                   {10, {126, 252}, {120, 252}, {114, 184}},
                                                                   {6, {91, 231}, {91, 229}, {78, 225}},
                                                                   {6, {91, 231}, {91, 229}, {104, 225}}}};
// Bounded spatial navigation uses the same rectangles as painting and pointer hits.
inline std::uint16_t equipmentNeighbor(std::uint16_t cell, int dx, int dy) noexcept {
    const auto current = equipment(int(cell) - int(bagCells));
    int best = 100000;
    auto result = cell;
    for (unsigned n = 0; n < equipmentSlots; ++n) {
        const auto candidate = equipment(int(n));
        const int x = candidate.x - current.x, y = candidate.y - current.y;
        const int forward = x * dx + y * dy;
        if (forward <= 0)
            continue;
        const int side = std::abs(x * dy - y * dx);
        const int score = forward + 4 * side;
        if (score < best) {
            best = score;
            result = std::uint16_t(bagCells + n);
        }
    }
    return result;
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
