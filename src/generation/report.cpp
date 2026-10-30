#include "generation/report.hpp"
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
    const auto& l = f.layout;
    j["family"] = l.family;
    j["rotation"] = l.rotation;
    j["width"] = l.width;
    j["height"] = l.height;
    j["map"] = ascii(l);
    j["spawn"] = {{"x", l.spawn.cell.x}, {"y", l.spawn.cell.y}, {"heading", unsigned(l.spawn.heading)}};
    j["loop_connection"] = l.loopConnection;
    j["rooms"] = nlohmann::json::array();
    for (unsigned i = 0; i < l.roomCount; ++i) {
        const auto& r = l.rooms[i];
        j["rooms"].push_back({{"id", r.id},
                              {"site", {r.site.x, r.site.y}},
                              {"anchor", {r.anchor.x, r.anchor.y}},
                              {"role", roleName(r.role)},
                              {"template", r.shape},
                              {"rotation", r.rotation},
                              {"ports", r.ports},
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
    j["connections"] = nlohmann::json::array();
    for (unsigned i = 0; i < l.connectionCount; ++i)
        j["connections"].push_back({l.connections[i].a, l.connections[i].b});
    return j;
}
nlohmann::json report(const GenerationResult& result, GenerationRequest request, bool detail) {
    auto j = result ? report(*result.floor, detail) : base(request, result.diagnostics, detail);
    j["ok"] = bool(result);
    return j;
}
} // namespace blobber::generation
