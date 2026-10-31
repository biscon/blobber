#include "assets/npc_metadata.hpp"
#include "support/automation_fixtures.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::automation;
using namespace blobber::automation::test;
int main() {
    try {
        Directory dir;
        Json definition = {{"id", "metadata-only"},
                           {"width", .45},
                           {"height", .65},
                           {"animations",
                            {{"idle",
                              {{"durations_ms", {40, 60}},
                               {"views",
                                {{"front", {"missing.png", "missing2.png"}},
                                 {"back", {"missing.png", "missing2.png"}},
                                 {"left", {"missing.png", "missing2.png"}},
                                 {"right", {"missing.png", "missing2.png"}}}}}}}}};
        const auto save = [&](const Json& d) {
            std::ofstream out(dir.path / "npc.json");
            out << d;
        };
        {
            std::ofstream out(dir.path / "level.json");
            out << Json{{"definitions", {"npc.json"}}, {"instances", Json::array()}};
        }
        save(definition);
        const auto loaded = loadNpcDefinitions(dir.path / "level.json");
        check(loaded.types.size() == 1 && loaded.timings[0][0].count == 2 &&
                  loaded.timings[0][0].durationMs[1] == 60,
              "metadata loads real timing without image files");
        for (unsigned fault = 0; fault < 6; ++fault) {
            auto bad = definition;
            switch (fault) {
            case 0:
                bad["width"] = .51;
                break;
            case 1:
                bad["height"] = 1.01;
                break;
            case 2:
                bad["animations"]["idle"]["durations_ms"] = {0, 60};
                break;
            case 3:
                bad["animations"]["idle"]["views"].erase("left");
                break;
            case 4:
                bad["animations"]["idle"]["views"]["front"][0] = {{"image", "not-opened.png"},
                                                                  {"anchor", {.5, 0}}};
                break;
            case 5:
                bad["sampling"] = "unknown";
                break;
            }
            save(bad);
            bool rejected = false;
            try {
                loadNpcDefinitions(dir.path / "level.json");
            } catch (...) {
                rejected = true;
            }
            check(rejected, "malformed NPC metadata rejects without decoding");
        }
        save(definition);
        check(loadNpcDefinitions(dir.path / "level.json").definitionFingerprint ==
                  loaded.definitionFingerprint,
              "identical metadata retains fingerprint");
        const Map map({"#####", "#...#", "#...#", "#...#", "#####"});
        const auto placement = Json{{"type", "metadata-only"}, {"cell", {2, 2}},
                                    {"subcell", {0, 0}}, {"facing", "south"}};
        auto level = Json{{"definitions", {"npc.json"}}, {"instances", {placement}}};
        const auto placed = [&](const Json& value) {
            { std::ofstream out(dir.path / "level.json"); out << value; }
            return loadNpcMetadata(dir.path / "level.json", map, {1, 1});
        };
        check(placed(level).world.occupied({2, 2}), "metadata placement retains occupancy without images");
        for (unsigned fault = 0; fault < 5; ++fault) {
            auto bad = level;
            if (fault == 0) bad["instances"].push_back(placement);
            if (fault == 1) bad["instances"][0]["type"] = "unknown";
            if (fault == 2) bad["instances"][0]["cell"] = {0, 0};
            if (fault == 3) bad["instances"][0]["subcell"] = {2, 0};
            if (fault == 4) bad["instances"][0]["cell"] = {1, 1};
            rejects([&] { placed(bad); }, "duplicate, unknown, wall, subcell and spawn placements reject");
        }
        std::cout << "Graphics-independent metadata checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
