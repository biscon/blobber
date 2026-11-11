#include "app/gameplay.hpp"
#include "app/loading.hpp"
#include "app/menu.hpp"
#include "app/options.hpp"
#include "assets/blueprint_loader.hpp"
#include "assets/npc_metadata.hpp"
#include "game/campaign.hpp"
#include "game/party.hpp"
#include "generation/diagnostic_view.hpp"
#include "generation/population.hpp"
#include "persistence/campaign_store.hpp"
#include "platform/audio.hpp"
#include "platform/game_view.hpp"
#include "platform/presenter.hpp"
#include "ui/interaction.hpp"
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>

namespace blobber {
namespace {
struct MenuMusicSession {
    AudioRuntime* audio;
    explicit MenuMusicSession(Presenter& presenter) : audio(presenter.audio) {
        if (audio)
            audio->endMenuMusic();
    }
    ~MenuMusicSession() {
        if (audio)
            audio->beginMenuMusic();
    }
};
} // namespace
static bool launchSession(const Options& options, const rules::Catalog& rules,
                          const rules::PresetParty* starting, Presenter& presenter,
                          const ArenaScenario* arena, CampaignStore* saves = nullptr,
                          std::unique_ptr<CampaignRun> restored = {}) {
    auto loading = std::make_unique<LoadingScreen>(presenter.loadingWorker(), &presenter);
    const auto seed =
        arena ? arena->seed
              : chooseCampaignSeed(options.seed,
                                   restored ? std::optional<std::uint32_t>(restored->location(0).seed)
                                            : std::nullopt,
                                   [] { return std::random_device{}(); });
    auto definitions = prepareCpu([&] {
        preparationPoint("Reading creature definitions");
        return loadNpcDefinitions(options.assets / "npcs/level.json");
    });
    std::optional<blueprints::BlueprintCatalog> roomCatalog;
    if (!arena && !saves)
        roomCatalog =
            prepareCpu([&] { return blueprints::load(options.assets / "blueprints/underkeep.json"); });
    std::unique_ptr<CampaignRun> campaign = std::move(restored);
    const bool showWelcome = !arena && !campaign;
    std::optional<Map> arenaFloor;
    if (arena)
        arenaFloor.emplace(arenaMap());
    else if (!campaign)
        campaign = prepareCpu([&] {
            preparationPoint("Building campaign");
            return std::make_unique<CampaignRun>(saves ? saves->blueprints() : *roomCatalog, rules, *starting,
                                                 definitions.types, definitions.timings, seed);
        });
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
    auto publication = prepareCpu([&] {
        preparationPoint("Validating prepared world assets");
        if (!arena)
            blueprints::checkPrepared(options.assets, options.preparedAssets / "world");
        return world_assets::load(options.preparedAssets / "world");
    });
    if (publication.definitions != loadedNpcs.definitionFingerprint)
        throw std::runtime_error("World NPC definitions differ; rebuild world assets");
    WorldRenderer world(presenter.resources(), std::move(publication), options.supersampling,
                        options.ambientOcclusion);
    const GameplayLaunch launch{spawn,   runtimeSeed,    bool(arena), true,  nullptr,    nullptr,
                                nullptr, campaign.get(), saves,       arena, showWelcome};
    if (loading->closing())
        return false;
    loading.reset();
    presenter.fade(0);
    return runGameplay(map, rules, loadedNpcs.world, timings, presenter, options, world, recovery, starting,
                       &launch) != PlayOutcome::Quit;
}
bool launchMenuParty(const Options& o, const rules::Catalog& c, const rules::PresetParty& p,
                     Presenter& presenter, CampaignStore* saves) {
    MenuMusicSession music(presenter);
    return launchSession(o, c, &p, presenter, nullptr, saves);
}
bool launchArena(const Options& o, const rules::Catalog& c, const ArenaScenario& arena,
                 Presenter& presenter) {
    MenuMusicSession music(presenter);
    const auto party = arenaParty(c, arena);
    return launchSession(o, c, &party, presenter, &arena);
}
bool launchSavedCampaign(const Options& o, const rules::Catalog& c, Presenter& presenter,
                         CampaignStore& saves) {
    MenuMusicSession music(presenter);
    std::unique_ptr<CampaignRun> run;
    {
        LoadingScreen loading(presenter.loadingWorker(), &presenter);
        run = prepareCpu([&] {
            preparationPoint("Restoring campaign");
            return saves.load();
        });
    }
    presenter.fade(0);
    return launchSession(o, c, nullptr, presenter, nullptr, &saves, std::move(run));
}
} // namespace blobber
