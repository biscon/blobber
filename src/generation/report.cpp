#include "generation/report.hpp"
#include "generation/architecture.hpp"
namespace blobber::generation {
namespace {
nlohmann::json failure(Failure f) {
    return {{"code", errorName(f.code)}, {"room", f.room}, {"connection", f.connection}};
}
nlohmann::json base(GenerationRequest r, const GenerationDiagnostics& d, bool detail) {
    nlohmann::json j{{"seed", r.seed},
                     {"depth", r.depth},
                     {"generator_version", r.generator},
                     {"template_version", r.templates}};
    if (detail) {
        j["attempts"] = d.attempts;
        j["fallback"] = d.fallback;
        j["fallback_id"] = d.fallbackId;
        j["failure"] = failure(d.failure);
        j["rejections"] = nlohmann::json::array();
        for (unsigned i = 0; i < d.attempts; ++i)
            if (d.rejected[i])
                j["rejections"].push_back({{"attempt", i + 1}, {"reason", failure(d.rejected[i])}});
    }
    return j;
}
} // namespace
nlohmann::json report(const GeneratedFloor& f, bool detail) {
    auto j = base(f.request, f.diagnostics, detail);
    j["fingerprint"] = std::to_string(f.fingerprint);
    if (!detail)
        return j;
    j["runtime_seed"] = f.runtimeSeed;
    j["dressing"] = nlohmann::json::array();
    for (const auto& d : f.map.definition().decorations)
        j["dressing"].push_back({{"kind", decorationName(d.kind)},
                                 {"cell", {d.cell.x, d.cell.y}},
                                 {"facing", unsigned(d.face)},
                                 {"blocking", d.blocks()}});
    const auto& l = f.layout;
    j["biome"] = "underkeep";
    j["algorithm"] = "stone_rooms";
    j["profile"] = profileName(l.profile);
    j["family"] = l.family;
    j["rotation"] = l.rotation;
    const auto m = layoutMetrics(l);
    j["metrics"] = {{"walkable_cells", m.walkableCells},
                    {"min_room_area", m.minRoomArea},
                    {"max_room_area", m.maxRoomArea},
                    {"distinct_room_areas", m.distinctRoomAreas},
                    {"distinct_connection_lengths", m.distinctConnectionLengths},
                    {"progression_steps", m.progressionSteps}};
    const auto pacing = encounterMetrics(l, f.map);
    j["encounter_pacing"] = {
        {"first_steps", pacing.firstEncounterSteps},
        {"longest_empty_steps", pacing.longestEmptySteps},
        {"route_steps", pacing.routeSteps},
        {"alternate_first_steps", pacing.alternateFirstSteps},
        {"alternate_longest_empty_steps", pacing.alternateLongestEmptySteps},
        {"alternate_steps", pacing.alternateSteps},
        {"first_target", 12},
        {"gap_target", 15},
        {"definition", "forward steps to first entry into each populated room; not human search time"}};
    j["width"] = l.width;
    j["height"] = l.height;
    j["map"] = ascii(l);
    j["spawn"] = {{"x", l.spawn.cell.x}, {"y", l.spawn.cell.y}, {"heading", unsigned(l.spawn.heading)}};
    j["loop_connection"] = l.loopConnection;
    j["rooms"] = nlohmann::json::array();
    const auto ceilings = roomCeilings(l);
    for (unsigned i = 0; i < l.roomCount; ++i) {
        const auto& r = l.rooms[i];
        nlohmann::json cells = nlohmann::json::array();
        for (int y = 0; y < l.height; ++y)
            for (int x = 0; x < l.width; ++x)
                if (regionAt(l, {x, y}) == i)
                    cells.push_back({x, y});
        j["rooms"].push_back({{"cells", cells},
                              {"id", r.id},
                              {"origin", {r.origin.x, r.origin.y}},
                              {"size", {r.size.x, r.size.y}},
                              {"anchor", {r.anchor.x, r.anchor.y}},
                              {"role", roleName(r.role)},
                              {"shape", r.shape},
                              {"ceiling_ticks", ceilings[i]},
                              {"rotation", r.rotation},
                              {"clearance", r.clearance},
                              {"facing", unsigned(r.facing)}});
    }
    j["fixtures"] = nlohmann::json::array();
    for (unsigned i = 0; i < l.fixtures.count; ++i) {
        const auto& v = l.fixtures.entries[i];
        j["fixtures"].push_back({{"id", i},
                                 {"kind", unsigned(v.kind)},
                                 {"position", {v.cell.x, v.cell.y}},
                                 {"heading", unsigned(v.facing)},
                                 {"key", v.key},
                                 {"name", fixtureName(v)}});
    }
    j["population"] = nlohmann::json::array();
    for (unsigned i = 0; i < l.populationCount; ++i) {
        const auto& p = l.population[i];
        j["population"].push_back({{"id", p.id},
                                   {"recipe", unsigned(p.recipe)},
                                   {"species", speciesName(p.recipe)},
                                   {"guardian", p.guardian},
                                   {"position", {p.cell.x, p.cell.y}},
                                   {"subcell", {p.subcell.x, p.subcell.y}},
                                   {"heading", unsigned(p.facing)}});
    }
    j["encounters"] = nlohmann::json::array();
    for (unsigned i = 0; i < l.encounterCount; ++i) {
        const auto& e = l.encounters[i];
        constexpr const char* roles[]{"direct", "optional", "guardian"};
        j["encounters"].push_back({{"id", i},
                                   {"recipe", encounterRecipe(e.recipe).name},
                                   {"role", roles[unsigned(e.role)]},
                                   {"room", e.room},
                                   {"first", e.first},
                                   {"count", e.count}});
    }
    j["route"] = nlohmann::json::array();
    for (unsigned i = 0; i < l.routeCount; ++i)
        j["route"].push_back(l.route[i]);
    j["containers"] = nlohmann::json::array();
    for (unsigned i = 0; i < l.cacheCount; ++i)
        j["containers"].push_back({{"slot", i},
                                   {"cell", {l.caches[i].cell.x, l.caches[i].cell.y}},
                                   {"facing", unsigned(l.caches[i].facing)},
                                   {"sealed", i == 2}});
    j["supplies"] = {{"direct", {l.supply.x, l.supply.y}},
                     {"optional", {l.optionalSupply.x, l.optionalSupply.y}}};
    j["landings"] = nlohmann::json::array();
    j["npc_excluded"] = nlohmann::json::array();
    for (unsigned i = 0; i < l.fixtures.landingCount; ++i)
        j["landings"].push_back({l.fixtures.landings[i].x, l.fixtures.landings[i].y});
    for (int y = 0; y < l.height; ++y)
        for (int x = 0; x < l.width; ++x)
            if (l.fixtures.npcExcluded[unsigned(y * l.width + x)])
                j["npc_excluded"].push_back({x, y});
    j["objective"] = {{"gate", l.guardianGate},
                      {"approach_door", l.approachDoor},
                      {"approach", {l.guardianApproach.x, l.guardianApproach.y}},
                      {"reward", {l.guardianReward.x, l.guardianReward.y}}};
    j["connections"] = nlohmann::json::array();
    for (unsigned i = 0; i < l.connectionCount; ++i) {
        const auto& e = l.connections[i];
        nlohmann::json path = nlohmann::json::array();
        for (unsigned k = 0; k < e.count; ++k) {
            const auto c = pathCell(l, e, k);
            path.push_back({c.x, c.y});
        }
        j["connections"].push_back({{"a", e.a},
                                    {"b", e.b},
                                    {"socket_a", {e.socketA.x, e.socketA.y}},
                                    {"socket_b", {e.socketB.x, e.socketB.y}},
                                    {"facing_a", unsigned(e.facingA)},
                                    {"facing_b", unsigned(e.facingB)},
                                    {"path", path}});
    }
    return j;
}
nlohmann::json report(const GenerationResult& result, GenerationRequest request, bool detail) {
    auto j = result ? report(*result.floor, detail) : base(request, result.diagnostics, detail);
    j["ok"] = bool(result);
    return j;
}
} // namespace blobber::generation
