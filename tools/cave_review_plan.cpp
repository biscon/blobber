#include "cave_review.hpp"
#include <ostream>
namespace blobber::cave_review {
using namespace generation;
void writePlan(std::ostream& out, const CaveFloor& cave, std::uint32_t seed, bool fallback) {
    const auto& f = cave.layout;
    constexpr int tile = 20, left = 22, top = 76;
    auto rect = [&](int x, int y, int w, int h, const char* color) {
        out << "<rect x='" << x << "' y='" << y << "' width='" << w << "' height='" << h << "' fill='"
            << color << "'/>";
    };
    auto label = [&](int x, int y, const auto& value, int size, const char* ink) {
        out << "<text x='" << x << "' y='" << y << "' font-size='" << size << "' fill='" << ink << "'>"
            << value << "</text>";
    };
    out << "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 1700 750' width='1700' height='750' "
           "font-family='DejaVu Sans,sans-serif'>";
    rect(0, 0, 1700, 750, "#152126");
    label(22, 40, std::string(profileName(f.profile)) + "  /  Depth " + std::to_string(f.depth), 28,
          "#f3eee1");
    label(22, 63,
          "Seed " + std::to_string(seed) + (fallback ? "   /   fallback" : "") +
              "   ·   One square per cell   ·   North up",
          15, "#bbc8c8");
    rect(left, top, f.width * tile, f.height * tile, "#27363b");
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x) {
            Cell c{x, y};
            if (!floorCell(f, c))
                continue;
            const auto r = regionAt(f, c);
            const char* color = r == noRegion                       ? "#84938c"
                                : f.rooms[r].role == Role::Optional ? "#c9b692"
                                                                    : "#cfcdc1";
            if (cave.map.movementBlocked(c))
                color = f.depth == 5 ? "#56a5bb" : "#c08169";
            rect(left + x * tile, top + y * tile, tile - 1, tile - 1, color);
        }
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto& e = f.connections[i];
        for (auto c : {e.socketA, e.socketB})
            rect(left + c.x * tile + 7, top + c.y * tile + 7, 5, 5, "#48565a");
    }
    auto glyph = [&](Cell c, const std::string& name, const char* color) {
        rect(left + c.x * tile + 1, top + c.y * tile + 1, tile - 3, tile - 3, color);
        label(left + c.x * tile + 4, top + c.y * tile + 15, name, 13, "white");
    };
    for (unsigned i = 0; i < f.fixtures.count; ++i) {
        const auto& d = f.fixtures.entries[i];
        glyph(d.cell,
              d.kind == FixtureKind::Door       ? "D"
              : d.kind == FixtureKind::Upstairs ? "U"
              : d.kind == FixtureKind::Endpoint ? "V"
                                                : ">",
              "#535569");
    }
    glyph(f.spawn.cell, "@", "#386452");
    const int key = left + f.width * tile + 22;
    label(key, 105, "PLACEMENT RESERVATIONS", 17, "#f3eee1");
    int line = 137;
    for (unsigned i = 0; i < caveSiteCount; ++i) {
        const Cell c = cave.sites.cells[i];
        if (c.x < 0)
            continue;
        std::string letter(1, char('A' + i));
        glyph(c, letter, "#875231");
        label(key, line,
              letter + "   " + siteName(CaveSite(i)) + "  (" + std::to_string(c.x) + ", " +
                  std::to_string(c.y) + ")",
              15, "#e7ddc8");
        line += 25;
    }
    line += 24;
    label(key, line, "TRAVERSAL", 17, "#f3eee1");
    for (const char* text : {"@   Arrival, facing west", "D   Door · F to open", "U   Upstairs",
                             "&gt; / V   Downstairs / vault endpoint", "Dark field   Solid rock",
                             "Small dots   Passage sockets", "Tan room   Optional pocket"}) {
        line += 24;
        label(key, line, text, 14, "#bfcbcb");
    }
    if (f.depth == 5)
        label(key, line + 26, "Blue cells   Water · blocks entry", 14, "#82d5e8");
    else
        label(key, line + 26, "Rust cells   Occupied footprint", 14, "#e3a892");
    line = 105;
    label(key + 300, line, "PASSAGE SOCKETS", 17, "#f3eee1");
    auto socket = [](Cell c, Heading h) {
        return std::to_string(c.x) + "," + std::to_string(c.y) + " " + headingName(h)[0];
    };
    for (unsigned i = 0; i < f.connectionCount; ++i) {
        const auto& e = f.connections[i];
        line += 22;
        label(key + 300, line, socket(e.socketA, e.facingA) + "   ↔   " + socket(e.socketB, e.facingB), 14,
              "#bfcbcb");
    }
    out << "</svg>\n";
    if (!out)
        throw std::runtime_error("Cannot write cave plan");
}
} // namespace blobber::cave_review
