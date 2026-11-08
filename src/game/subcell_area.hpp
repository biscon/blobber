#pragma once
#include "game/movement.hpp"
#include "rules/spells.hpp"
#include "world/fixtures.hpp"
#include <cstdint>
namespace blobber {
// Grid intersections in five-foot units, distinct from occupied subcell squares.
struct SpellAim {
    int x = 0, y = 0;
};
inline bool operator==(SpellAim a, SpellAim b) noexcept {
    return a.x == b.x && a.y == b.y;
}
inline int floorHalf(int n) noexcept {
    return n >= 0 ? n / 2 : (n - 1) / 2;
}
inline Cell dungeonCell(Cell tile) noexcept {
    return {floorHalf(tile.x), floorHalf(tile.y)};
}
inline Cell globalSubcell(Cell cell, Cell sub) noexcept {
    return cell * 2 + sub;
}
inline SpellAim cellAim(Cell cell) noexcept {
    return {cell.x * 2 + 1, cell.y * 2 + 1};
}
Cell formationSubcell(Player, unsigned slot) noexcept;
struct SubcellFootprint {
    Cell origin{}; // Global subcell-square coordinates; eight bits per row.
    std::uint64_t mask = 0;
    bool contains(Cell tile) const noexcept {
        const int x = tile.x - origin.x, y = tile.y - origin.y;
        return x >= 0 && x < 8 && y >= 0 && y < 8 && (mask & (UINT64_C(1) << (y * 8 + x)));
    }
    Cell tile(unsigned bit) const noexcept { return origin + Cell{int(bit % 8), int(bit / 8)}; }
    void add(Cell tile) noexcept {
        const int x = tile.x - origin.x, y = tile.y - origin.y;
        if (x >= 0 && x < 8 && y >= 0 && y < 8)
            mask |= UINT64_C(1) << (y * 8 + x);
    }
    Cell firstTile() const noexcept {
        for (unsigned bit = 0; bit < 64; ++bit)
            if (mask & (UINT64_C(1) << bit))
                return tile(bit);
        return origin;
    }
    unsigned count() const noexcept {
        auto bits = mask;
        unsigned n = 0;
        while (bits) {
            bits &= bits - 1;
            ++n;
        }
        return n;
    }
};
inline bool operator==(const SubcellFootprint& a, const SubcellFootprint& b) noexcept {
    return a.origin == b.origin && a.mask == b.mask;
}
inline SubcellFootprint wholeCellFootprint(Cell c) noexcept {
    return {c * 2, UINT64_C(0x303)};
}
bool validSpellAim(GeometryView, Player, unsigned formation, SpellAim) noexcept;
SubcellFootprint spellFootprint(GeometryView, Player, unsigned formation, rules::SpellKind,
                                SpellAim) noexcept;
} // namespace blobber
