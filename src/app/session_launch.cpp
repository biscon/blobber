#include "app/gameplay.hpp"
#include "app/menu.hpp"
#include "app/options.hpp"
#include "assets/npc_metadata.hpp"
#include "game/campaign.hpp"
#include "game/party.hpp"
#include "generation/diagnostic_view.hpp"
#include "generation/population.hpp"
#include "persistence/campaign_store.hpp"
#include "platform/game_view.hpp"
#include "platform/presenter.hpp"
#include "ui/interaction.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>

namespace blobber {
static bool launchSession(const Options& options, const rules::Catalog& rules,
                          const rules::PresetParty* starting, Presenter& presenter,
                          const ArenaScenario* arena, CampaignStore* saves = nullptr,
                          std::unique_ptr<CampaignRun> restored = {}) {
    const auto seed = arena ? arena->seed : options.seed ? *options.seed : std::random_device{}();
    auto definitions = loadNpcDefinitions(options.assets / "npcs/level.json");
    std::unique_ptr<CampaignRun> campaign = std::move(restored);
    std::optional<Map> arenaFloor;
    if (arena)
        arenaFloor.emplace(arenaMap());
    else if (!campaign)
        campaign =
            std::make_unique<CampaignRun>(rules, *starting, definitions.types, definitions.timings, seed);
    const Map& map = arena ? *arenaFloor : campaign->map();
    const Player spawn = arena ? arenaSpawn() : campaign->game().movement().player;
    const auto runtimeSeed = seed;
    LoadedNpcMetadata loadedNpcs{arena ? arenaPlacements(map, rules, *arena, definitions.types)
                                       : *campaign->location(campaign->activeLocation()).placements,
                                 std::move(definitions.timings), std::move(definitions.attachments),
                                 definitions.definitionFingerprint};
    const RecoveryLevel recovery =
        campaign ? campaign->location(campaign->activeLocation()).recovery : RecoveryLevel{};

    const auto& timings = loadedNpcs.timings;
    auto publication = world_assets::load(options.assets / "prepared/world");
    if (publication.definitions != loadedNpcs.definitionFingerprint)
        throw std::runtime_error("World NPC definitions differ; rebuild world assets");
    WorldRenderer world(presenter.resources(), std::move(publication));
    world.supersampling(options.supersampling);
    const GameplayLaunch launch{spawn,   runtimeSeed, bool(arena),    true, nullptr,
                                nullptr, nullptr,     campaign.get(), saves};
    return runGameplay(map, rules, loadedNpcs.world, timings, presenter, options, world, recovery, starting,
                       &launch) != PlayOutcome::Quit;
}
bool launchMenuParty(const Options& o, const rules::Catalog& c, const rules::PresetParty& p,
                     Presenter& presenter, CampaignStore* saves) {
    return launchSession(o, c, &p, presenter, nullptr, saves);
}
bool launchArena(const Options& o, const rules::Catalog& c, const ArenaScenario& arena,
                 Presenter& presenter) {
    const auto party = arenaParty(c, arena);
    return launchSession(o, c, &party, presenter, &arena);
}
bool launchSavedCampaign(const Options& o, const rules::Catalog& c, Presenter& presenter,
                         CampaignStore& saves) {
    auto run = saves.load();
    return launchSession(o, c, nullptr, presenter, nullptr, &saves, std::move(run));
}
} // namespace blobber
