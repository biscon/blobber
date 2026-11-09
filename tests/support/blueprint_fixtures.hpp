#pragma once
#include "assets/blueprint_loader.hpp"
#include <nlohmann/json.hpp>
namespace blobber::test {
inline nlohmann::json blueprintJson() {
    using J = nlohmann::json;
    J root = {{"schema_version", 1},
              {"kits", J::array({{{"id", "fixture"}, {"size", {1, 2}}, {"height", .3}}})},
              {"rooms", J::array()}};
    for (const auto* id : {"kitchen", "mead-hall", "barracks", "armory"}) {
        const int w = std::string_view(id) == "mead-hall" ? 7 : 5,
                  h = std::string_view(id) == "barracks" ? 7 : w;
        const int side = w == 7 ? 4 : h / 2;
        root["rooms"].push_back(
            {{"id", id},
             {"purpose", id},
             {"size", {w, h}},
             {"ceiling", 6},
             {"rotations", {0, 90, 180, 270}},
             {"solid", J::array()},
             {"placements", J::array({{{"kit", "fixture"}, {"anchor", {0, 0}}, {"facing", "north"}}})},
             {"sockets", J::array({{{"id", "north"}, {"cell", {w / 2, -1}}, {"outward", "north"}},
                                   {{"id", "east"}, {"cell", {w, side}}, {"outward", "east"}},
                                   {{"id", "south"}, {"cell", {w / 2, h}}, {"outward", "south"}},
                                   {{"id", "west"}, {"cell", {-1, side}}, {"outward", "west"}}})},
             {"lanes", J::array({{{"origin", {w / 2, 0}}, {"size", {1, h}}}})},
             {"encounters", J::array({{{"origin", {1, 2}}, {"size", {2, 2}}}})},
             {"rewards", J::array({{{"cell", {w - 1, h - 1}}, {"access", {w - 2, h - 1}}}})}});
    }
    return root;
}
inline blueprints::BlueprintCatalog blueprintCatalog() {
    return blueprints::decode(blueprintJson().dump());
}
} // namespace blobber::test
