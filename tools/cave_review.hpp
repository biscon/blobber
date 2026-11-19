#pragma once
#include "assets/world_assets.hpp"
#include "generation/caves.hpp"
#include <iosfwd>

namespace blobber {
class WorldRenderer;
class GameSession;
class Presenter;
}
namespace blobber::cave_review {
void walkTo(GameSession&, Cell, std::optional<Cell> avoid = {}, std::vector<Cell>* trace = nullptr,
            bool stopAtContact = false);
void placeGalleries(const generation::CaveSites&, const std::vector<std::string>&,
                    std::vector<NpcInstance>&);
void equipMeleeProofParty(rules::PresetParty&);
void checkGalleries(GameSession&, const generation::CaveFloor&, const std::vector<NpcTimings>&, bool north, bool meleeOnly,
                    const std::filesystem::path&, WorldRenderer*, Presenter*);
void checkSpider(GameSession&, const generation::CaveFloor&, const std::vector<NpcTimings>&,
                 const std::filesystem::path&, WorldRenderer* = nullptr, Presenter* = nullptr);
void biomeChange(WorldRenderer&, const generation::CaveFloor&, const std::filesystem::path&,
                 std::uint32_t seed);
void checkPreparedDressing(const world_assets::CpuAssets&, const generation::CaveFloor&);
void writePlan(std::ostream&, const generation::CaveFloor&, std::uint32_t seed, bool fallback);
} // namespace blobber::cave_review
