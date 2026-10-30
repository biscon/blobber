#include "automation/codec.hpp"
#include "persistence/arena_store.hpp"
#include "persistence/starting_codec.hpp"
#include <algorithm>
#include <stdexcept>
namespace blobber::automation {
Scenario decodeScenario(const rules::Catalog& catalog, const Json& j) {
    if (j.dump().size() > arenaMaxBytes)
        throw std::invalid_argument("Scenario exceeds 1 MiB");
    if (!j.contains("kind"))
        return decodeArena(catalog, j.dump());
    if (j.at("kind") == "arena") {
        auto arena = j;
        arena.erase("kind");
        return decodeArena(catalog, arena.dump());
    }
    if (j.at("kind") == "campaign") {
        fields(j, {"kind", "schema_version", "generator_version", "template_version", "seed", "party"});
        auto floor = j;
        floor["kind"] = "generated_floor";
        floor["depth"] = 1;
        const auto decoded = decodeScenario(catalog, floor);
        const auto& generated = std::get<GeneratedScenario>(decoded);
        return CampaignScenario{generated.party, generated.generation.seed};
    }
    if (j.at("kind") != "generated_floor")
        throw std::invalid_argument("Unknown scenario kind");
    fields(j, {"kind", "schema_version", "generator_version", "template_version", "seed", "depth", "party",
               "force_fallback"});
    if (bounded(j.at("schema_version"), 1) != 1)
        throw std::invalid_argument("Unsupported scenario schema");
    GeneratedScenario s;
    if (j.contains("force_fallback")) {
        if (!j.at("force_fallback").is_boolean())
            throw std::invalid_argument("force_fallback must be boolean");
        s.forceFallback = j.at("force_fallback").get<bool>();
    }
    s.generation = {bounded(j.at("seed"), UINT32_MAX), bounded(j.at("depth"), 3),
                    bounded(j.at("generator_version"), generation::generatorVersion),
                    bounded(j.at("template_version"), generation::templateVersion)};
    if (!s.generation.depth || s.generation.generator != generation::generatorVersion ||
        s.generation.templates != generation::templateVersion)
        throw std::invalid_argument("Unsupported generation depth/version");
    const auto& party = j.at("party");
    if (!party.is_array() || party.size() != 4)
        throw std::invalid_argument("Expected four starting characters");
    for (unsigned i = 0; i < 4; ++i) {
        s.party[i].draft = decodeStartingDraft(catalog, party[i]);
        const auto id = party[i].at("id").get<std::string>();
        if (id.empty() || id.size() > 128 || id.find('\0') != std::string::npos)
            throw std::invalid_argument("Invalid character identity");
        std::copy(id.begin(), id.end(), s.party[i].id.begin());
    }
    (void)startingParty(catalog, s.party);
    return s;
}
} // namespace blobber::automation
