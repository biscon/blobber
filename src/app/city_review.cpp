#include "app/city_review.hpp"
#include "app/gameplay.hpp"
#include "app/loading.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "world/sanctuary_city.hpp"
namespace blobber {
int runCityReview(const Options& options) {
    Window window(options.width, options.height, false, options.uncapped);
    PreparationWorker worker;
    GpuResources resources;
    Presenter presenter(resources, options.assets, &worker);
    auto loading = std::make_unique<LoadingScreen>(&worker, &presenter);
    const auto rules = prepareCpu([&] { return loadRules(options.assets / "rules/catalog.json"); });
    if (!rules.preset())
        throw std::runtime_error("City review requires the preset party");
    const auto definitions =
        prepareCpu([&] { return loadNpcDefinitions(options.assets / "npcs/level.json"); });
    const auto scene = prepareCpu([] { return sanctuaryCity(); });
    const NpcWorld empty(scene.map, scene.spawn.cell, definitions.types, {});
    auto publication = prepareCpu([&] { return world_assets::load(options.assets / "prepared/world"); });
    if (publication.definitions != definitions.definitionFingerprint)
        throw std::runtime_error("City review definitions differ; rebuild world assets");
    WorldRenderer renderer(resources, std::move(publication), options.supersampling);
    GameplayLaunch launch;
    launch.spawn = scene.spawn;
    launch.enableAi = false;
    if (loading->closing())
        return 0;
    loading.reset();
    presenter.fade(0);
    runGameplay(scene.map, rules, empty, definitions.timings, presenter, options, renderer, {},
                &*rules.preset(), &launch);
    return 0;
}
} // namespace blobber
