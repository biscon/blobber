#pragma once
#include "game/arena.hpp"
#include "generation/floor.hpp"
#include <variant>
namespace blobber::automation {
struct GeneratedScenario {
    StartingParty party;
    generation::GenerationRequest generation;
    bool forceFallback = false; // Diagnostic fixture/capture input only.
};
struct CampaignScenario {
    StartingParty party;
    std::uint32_t seed = 5;
};
struct RendererFixtureScenario {
    StartingParty party;
    std::uint32_t seed = 5;
};
using Scenario = std::variant<ArenaScenario, GeneratedScenario, CampaignScenario, RendererFixtureScenario>;
inline void replaceSeed(Scenario& s, std::uint32_t seed) {
    if (auto* a = std::get_if<ArenaScenario>(&s))
        a->seed = seed;
    else if (auto* g = std::get_if<GeneratedScenario>(&s))
        g->generation.seed = seed;
    else if (auto* c = std::get_if<CampaignScenario>(&s))
        c->seed = seed;
    else
        std::get<RendererFixtureScenario>(s).seed = seed;
}
} // namespace blobber::automation
