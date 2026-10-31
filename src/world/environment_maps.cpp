#include "world/environment_maps.hpp"
namespace blobber {
Map sanctuaryMap() {
    const std::vector<std::string> rows{"#########", "#.......#", "#.......#", "#.......#", "#.......#",
                                        "#.......#", "#.......#", "#.......#", "#########"};
    WorldDefinition w;
    w.surfaces.resize(81);
    for (int y = 1; y < 8; ++y)
        for (int x = 1; x < 8; ++x) {
            auto& s = w.surfaces[y * 9 + x];
            s.ceiling = noCeiling;
            s.environment = Environment::Daylight;
            if (y == 4 && x >= 4) {
                s.ceiling = 5;
                s.environment = Environment::Dungeon;
            }
        }
    w.decorations.push_back({{1, 2}, Heading::West, DecorationKind::Crest});
    w.decorations.push_back({{7, 2}, Heading::East, DecorationKind::Vine});
    w.decorations.push_back({{1, 6}, Heading::West, DecorationKind::Bench, 0, .5f, 0, .7f, .2f, .2f});
    w.decorations.push_back({{7, 6}, Heading::East, DecorationKind::Urn, 0, .5f, 0, .2f, .25f, .2f});
    return Map(rows, std::move(w));
}
FloorFixtures rendererHeightFixtures() {
    FloorFixtures f;
    f.count = 1;
    f.entries[0] = {{4, 3}, Heading::East, FixtureKind::Door};
    return f;
}
Map rendererHeightMap() {
    const std::vector<std::string> rows{"#########", "#.......#", "#.......#", "#.......#", "#.......#",
                                        "#.......#", "#.......#", "#.......#", "#########"};
    WorldDefinition w;
    w.surfaces.resize(81);
    for (int y = 1; y < 8; ++y)
        for (int x = 1; x < 8; ++x) {
            auto& s = w.surfaces[y * 9 + x];
            s.ceiling = 8;
            if (x >= 4) {
                s.ceiling = noCeiling;
                s.environment = Environment::Daylight;
            }
            if (y <= 3 && x >= 3)
                s.floor = 1;
            if (y <= 2 && x >= 5)
                s.floor = 4;
        }
    w.surfaces[6 * 9 + 3].ceiling = 4;
    w.surfaces[6 * 9 + 4].floor = 1; // Adjacent low ceiling rejects shared clearance.
    w.decorations.push_back({{1, 2}, Heading::West, DecorationKind::Crest});
    w.decorations.push_back({{7, 4}, Heading::East, DecorationKind::Vine});
    w.decorations.push_back({{1, 6}, Heading::West, DecorationKind::Bench, 0, .5f, 0, .7f, .2f, .2f});
    return Map(rows, std::move(w));
}
} // namespace blobber
