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
        auto bounded = definition;
        for (const auto* side : {"front", "back", "left", "right"})
            bounded["attachments"]["body_bounds"][side] = {.2, .1, .8, .95};
        save(bounded);
        const auto cosmetic = loadNpcDefinitions(dir.path / "level.json");
        check(cosmetic.definitionFingerprint == loaded.definitionFingerprint &&
                  cosmetic.attachments[0].bodyBounds[0].left == .2f,
              "Cosmetic bounds load without invalidating existing saves");
        for (const auto& box : {Json{.8,.1,.2,.95}, Json{-.1,0,1,1}, Json{0,0,1,1.1}, Json{0,0,1}}) {
            auto bad = bounded;
            bad["attachments"]["body_bounds"]["front"] = box;
            save(bad);
            rejects([&] { loadNpcDefinitions(dir.path / "level.json"); }, "Malformed body bounds reject");
        }
        auto wideCanvas = definition;
        wideCanvas["animations"]["idle"]["width"] = .8;
        save(wideCanvas);
        check(loadNpcDefinitions(dir.path / "level.json").types.size() == 1,
              "Human-height art canvas does not enlarge the one-subcell body");
        wideCanvas["animations"]["idle"]["width"] = 1.01;
        save(wideCanvas);
        rejects([&] { loadNpcDefinitions(dir.path / "level.json"); }, "Clip stays within publication bound");
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
        const auto placement =
            Json{{"type", "metadata-only"}, {"cell", {2, 2}}, {"subcell", {0, 0}}, {"facing", "south"}};
        auto level = Json{{"definitions", {"npc.json"}}, {"instances", {placement}}};
        const auto placed = [&](const Json& value) {
            {
                std::ofstream out(dir.path / "level.json");
                out << value;
            }
            return loadNpcMetadata(dir.path / "level.json", map, {1, 1});
        };
        check(placed(level).world.occupied({2, 2}), "metadata placement retains occupancy without images");
        for (unsigned fault = 0; fault < 5; ++fault) {
            auto bad = level;
            if (fault == 0)
                bad["instances"].push_back(placement);
            if (fault == 1)
                bad["instances"][0]["type"] = "unknown";
            if (fault == 2)
                bad["instances"][0]["cell"] = {0, 0};
            if (fault == 3)
                bad["instances"][0]["subcell"] = {2, 0};
            if (fault == 4)
                bad["instances"][0]["cell"] = {1, 1};
            rejects([&] { placed(bad); }, "duplicate, unknown, wall, subcell and spawn placements reject");
        }
        std::cout << "Graphics-independent metadata checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
