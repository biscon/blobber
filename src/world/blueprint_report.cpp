#include "world/blueprint_report.hpp"
#include <iomanip>
namespace blobber::blueprints {
void writePlan(std::ostream& out, const BlueprintCatalog& catalog, const ReviewScene& scene,
               std::string_view title) {
    constexpr int tile = 32, left = 42, top = 100;
    const int mapWidth = scene.map.width() * tile, mapHeight = scene.map.height() * tile;
    const int width = mapWidth + 440, height = std::max(760, mapHeight + 200);
    auto text = [&](int x, int y, const auto& value, int size = 14, const char* color = "#dbe4e7") {
        out << "<text x='" << x << "' y='" << y << "' font-family='sans-serif' font-size='" << size
            << "' fill='" << color << "'>" << value << "</text>\n";
    };
    auto cell = [&](Cell p, const char* color, const char* stroke = "#35434c") {
        out << "<rect x='" << left + p.x * tile << "' y='" << top + p.y * tile
            << "' width='32' height='32' fill='" << color << "' stroke='" << stroke << "'/>\n";
    };
    out << "<svg xmlns='http://www.w3.org/2000/svg' width='" << width << "' height='" << height
        << "' viewBox='0 0 " << width << ' ' << height << "'>\n"
        << "<rect width='100%' height='100%' fill='#14212a'/>\n";
    text(left, 36, "BR01 / " + std::string(title), 25);
    text(left, 65, "Authored blockout · layout review pending · cell units", 15, "#a9bdc8");
    for (int y = 0; y < scene.map.height(); ++y)
        for (int x = 0; x < scene.map.width(); ++x)
            cell({x, y}, scene.map.solid({x, y}) ? "#39444b" : "#d8d2c0");
    for (int x = 0; x < scene.map.width(); ++x)
        text(left + x * tile + 9, top - 10, x, 11);
    for (int y = 0; y < scene.map.height(); ++y)
        text(17, top + y * tile + 20, y, 11);
    auto region = [&](const ReviewRoom& rr, Rect r, const char* fill) {
        for (int y = 0; y < r.size.y; ++y)
            for (int x = 0; x < r.size.x; ++x)
                cell(rr.origin + r.origin + Cell{x, y}, fill);
    };
    for (unsigned n = 0; n < scene.roomCount; ++n) {
        const auto& rr = scene.rooms[n];
        const auto& r = rr.blueprint;
        for (unsigned i = 0; i < r.laneCount; ++i)
            region(rr, r.lanes[i], "#b3cfb3");
        for (unsigned i = 0; i < r.encounterCount; ++i)
            region(rr, r.encounters[i], "#a2bdce");
        for (unsigned i = 0; i < r.placementCount; ++i) {
            const auto& p = r.placements[i];
            const auto& k = catalog.kits[p.kit.index];
            for (unsigned y = 0; y < unsigned(k.size.y); ++y)
                for (unsigned x = 0; x < unsigned(k.size.x); ++x)
                    cell(rr.origin + footprint(k, p, x, y), "#ac8260");
            const auto a = rr.origin + p.anchor;
            text(left + a.x * tile + 8, top + a.y * tile + 21, p.kit.index + 1, 14, "#ffffff");
        }
        for (unsigned i = 0; i < r.rewardCount; ++i) {
            auto c = rr.origin + r.rewards[i].cell, a = rr.origin + r.rewards[i].access;
            cell(c, "#ceac61");
            text(left + c.x * tile + 10, top + c.y * tile + 22, "R", 14, "#202b34");
            text(left + a.x * tile + 10, top + a.y * tile + 22, "A", 14, "#202b34");
        }
        for (unsigned i = 0; i < r.socketCount; ++i) {
            const auto p = rr.origin + r.sockets[i].cell;
            out << "<rect x='" << left + p.x * tile + 2 << "' y='" << top + p.y * tile + 2
                << "' width='28' height='28' fill='none' stroke='#e7ca8c' stroke-width='2' "
                   "stroke-dasharray='4 3'/>\n";
        }
    }
    for (unsigned i = 0; i < scene.fixtures.count; ++i) {
        auto c = scene.fixtures.entries[i].cell;
        cell(c, "#4c8e8b");
        text(left + c.x * tile + 10, top + c.y * tile + 22, "D");
    }
    const auto spawn = scene.spawn.cell;
    text(left + spawn.x * tile + 10, top + spawn.y * tile + 22, "P", 15, "#233844");
    int lx = mapWidth + 75, y = 110;
    text(lx, y, "ROOM CONTRACTS", 16);
    y += 30;
    for (unsigned n = 0; n < scene.roomCount; ++n) {
        const auto& r = scene.rooms[n].blueprint;
        text(lx, y, r.id.data(), 17);
        y += 24;
        text(lx, y,
             std::to_string(r.size.x) + " × " + std::to_string(r.size.y) + " interior / ceiling " +
                 std::to_string(r.ceiling) + " ticks");
        y += 30;
    }
    text(lx, y, "KIT / blocked footprints", 16);
    y += 28;
    for (unsigned i = 0; i < catalog.kitCount; ++i) {
        const auto& k = catalog.kits[i];
        text(lx, y, std::to_string(i + 1) + "  " + k.id.data());
        y += 22;
    }
    y += 15;
    for (const char* label :
         {"Green: reserved walking lane", "Blue: usable encounter area", "R: empty chest reservation",
          "A: chest access (walkable)", "D: active door / P: start", "Dashed: alternative socket",
          "Brown: whole-cell movement block"}) {
        text(lx, y, label, 13, "#b5c7ce");
        y += 23;
    }
    text(left, height - 56, "All rotations: 0° / 90° / 180° / 270° · no reflection", 14);
    text(left, height - 31,
         "Prepared geometry, discrete movement. Furniture gaps inside brown cells are not traversable.", 13,
         "#a9bdc8");
    out << "</svg>\n";
    if (!out)
        throw Failure(Error::Content, "Cannot write blueprint diagram");
}
} // namespace blobber::blueprints
