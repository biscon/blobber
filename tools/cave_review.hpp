#pragma once
#include "assets/world_assets.hpp"
#include "game/arena.hpp"
#include "generation/caves.hpp"
#include <iosfwd>

namespace blobber {
class WorldRenderer;
class GameSession;
class CampaignRun;
class Presenter;
}
namespace blobber::cave_review {
void walkTo(GameSession&, Cell, std::optional<Cell> avoid = {}, std::vector<Cell>* trace = nullptr,
            bool stopAtContact = false, const generation::CellMask* forbidden = nullptr);
Party::Saved guardianParty(const rules::Catalog&, const Party::Saved&, unsigned depth);
Party::Saved populationParty(const rules::Catalog&, unsigned depth, bool fullEntry);
enum class ExpeditionRoute { NestControl, NestTrial, ThroughVault };
void checkCaveExpedition(CampaignRun&, ExpeditionRoute);
void checkPressure(const generation::CaveFloor&, const rules::Catalog&, const NpcWorld&, const std::vector<std::string>& types,
                   const std::vector<NpcTimings>&, std::uint32_t seed);
void checkShortcut(GameSession&, const generation::CaveFloor&);
void checkPopulation(GameSession&, const generation::CaveFloor&, const RecoveryLevel&, bool full);
void placeGalleries(const generation::CaveSites&, const std::vector<std::string>&,
                    std::vector<NpcInstance>&);
void equipMeleeProofParty(rules::PresetParty&);
enum class VaultRoute { Suppress, Burst, OpeningVolley, DoorwayVolley, Replacement };
ArenaScenario prepareVault(const generation::CaveSites&, const std::vector<std::string>&,
                           const rules::Catalog&, std::vector<NpcInstance>&);
void checkVault(GameSession&, const generation::CaveFloor&, const std::vector<NpcTimings>&, VaultRoute,
                const std::filesystem::path&, WorldRenderer*, Presenter*);
void checkGalleries(GameSession&, const generation::CaveFloor&, const std::vector<NpcTimings>&, bool north, bool meleeOnly,
                    const std::filesystem::path&, WorldRenderer*, Presenter*, bool soloOnly = false);
void checkSpider(GameSession&, const generation::CaveFloor&, const std::vector<NpcTimings>&,
                 const std::filesystem::path&, WorldRenderer* = nullptr, Presenter* = nullptr);
void biomeChange(WorldRenderer&, const generation::CaveFloor&, const std::filesystem::path&,
                 std::uint32_t seed);
void checkPreparedDressing(const world_assets::CpuAssets&, const generation::CaveFloor&);
void writePlan(std::ostream&, const generation::CaveFloor&, std::uint32_t seed, bool fallback);
} // namespace blobber::cave_review
