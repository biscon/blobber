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
using Scenario = std::variant<ArenaScenario, GeneratedScenario>;
inline void replaceSeed(Scenario& s, std::uint32_t seed) {
    if (auto* a = std::get_if<ArenaScenario>(&s))
        a->seed = seed;
    else
        std::get<GeneratedScenario>(s).generation.seed = seed;
}
} // namespace blobber::automation
