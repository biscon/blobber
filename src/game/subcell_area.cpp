#include "game/subcell_area.hpp"
#include "game/perception.hpp"
#include <cstdlib>
namespace blobber {
Cell formationSubcell(Player p, unsigned slot) noexcept {
    // North-facing FL/FR/BL/BR; rotate around the centre of the two-by-two formation.
    Cell s{int(slot % 2), int(slot / 2)};
    for (unsigned h = 0; h < unsigned(p.heading); ++h)
        s = {1 - s.y, s.x};
    return globalSubcell(p.cell, s);
}
bool validSpellAim(GeometryView map, Player p, unsigned formation, SpellAim aim) noexcept {
    if (aim.x < 0 || aim.y < 0 || aim.x > map.width() * 2 || aim.y > map.height() * 2)
        return false;
    const auto caster = formationSubcell(p, formation);
    const std::int64_t dx = 2LL * aim.x - (2LL * caster.x + 1), dy = 2LL * aim.y - (2LL * caster.y + 1);
    if (dx * dx + dy * dy > 24 * 24)
        return false;
    for (int y = -1; y <= 0; ++y)
        for (int x = -1; x <= 0; ++x) {
            const auto cell = dungeonCell({aim.x + x, aim.y + y});
            if (!map.solid(cell) && (cell == p.cell || inPartyView(map, p.cell, p.heading, cell)) &&
                lineOfSight(map, p.cell, cell))
                return true;
        }
    return false;
}
SubcellFootprint spellFootprint(GeometryView map, Player p, unsigned formation, rules::SpellKind kind,
                                SpellAim aim) noexcept {
    using K = rules::SpellKind;
    const bool cone = kind == K::BurningHands || kind == K::ColorSpray;
    const bool forwardArea = cone || kind == K::Thunderwave;
    const auto f = forward(p.heading), r = right(p.heading);
    SubcellFootprint result;
    Cell source = p.cell;
    if (forwardArea) {
        // Use a fixed party-front reference: selected caster/formation never moves the template.
        const auto frontLeft = formationSubcell(p, 0);
        result.origin = p.cell * 2 + Cell{-3, -3};
        for (int d = 1; d <= 2; ++d) {
            const int first = cone && d == 1 ? 0 : -1;
            const int last = cone && d == 1 ? 1 : 2;
            for (int lateral = first; lateral <= last; ++lateral) {
                const auto tile = frontLeft + f * d + r * lateral;
                const auto c = dungeonCell(tile);
                if (!(c == p.cell) && !map.solid(c) && lineOfSight(map, p.cell, c))
                    result.add(tile);
            }
        }
        return result;
    }
    if (!validSpellAim(map, p, formation, aim))
        return result;
    const int radius = kind == K::Sleep ? 3 : kind == K::Grease ? 1 : 2;
    result.origin = {aim.x - radius, aim.y - radius};
    // Choose a visible walkable incident square for conservative centre-to-area LOS.
    bool found = false;
    for (int y = -1; y <= 0 && !found; ++y)
        for (int x = -1; x <= 0 && !found; ++x) {
            const auto c = dungeonCell({aim.x + x, aim.y + y});
            if (!map.solid(c) && lineOfSight(map, p.cell, c)) {
                source = c;
                found = true;
            }
        }
    for (int y = -radius; y < radius; ++y)
        for (int x = -radius; x < radius; ++x) {
            if (kind == K::Shatter && (2 * x + 1) * (2 * x + 1) + (2 * y + 1) * (2 * y + 1) > 16)
                continue;
            const Cell tile{aim.x + x, aim.y + y}, c = dungeonCell(tile);
            if (!map.solid(c) && lineOfSight(map, source, c))
                result.add(tile);
        }
    return result;
}
} // namespace blobber
