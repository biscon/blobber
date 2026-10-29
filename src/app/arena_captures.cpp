#include "app/arena_setup.hpp"
#include "app/menu.hpp"
#include "ui/menu_screen.hpp"
#include <fstream>
namespace blobber {
void captureArena(MenuView& v, Presenter& presenter, const Options& o, ArenaScenarioStore& store) {
    std::filesystem::create_directories(o.captureDirectory);
    auto capture = [&](const char* name) {
        presenter.presentMenu(drawMenu, &v);
        presenter.capture(o.captureDirectory / (std::string(name) + ".png"));
        presenter.capture(o.captureDirectory / (std::string(name) + "-1280.png"), 1280, 720);
        presenter.capture(o.captureDirectory / (std::string(name) + "-4x3.png"), 1024, 768);
    };
    capture("main-arena");
    openArena(v, o, store);
    v.arena.scenario.seed = 5;
    v.arena.syncSeed();
    capture("arena-empty");
    for (unsigned i = 0; i < 4; ++i)
        arenaAction(v, 4100, o, store, presenter);
    if (v.catalog.monsterCount() > 1)
        arenaAction(v, 4101, o, store, presenter);
    v.arena.nameEdit.focus(true, v.arena.scenario.name);
    capture("arena-setup");
    v.arena.blur();
    arenaAction(v, 4001, o, store, presenter);
    capture("arena-picker-snapshots");
    v.model.screen = MenuScreen::Arena;
    v.model.arenaPicker = false;
    v.model.choices = v.dungeonChoices;
    arenaAction(v, 4002, o, store, presenter);
    if (!v.arena.currentFile.file[0])
        throw std::runtime_error(v.error.data());
    arenaAction(v, 4004, o, store, presenter);
    capture("arena-scenarios");
    arenaAction(v, 4300, o, store, presenter);
    capture("arena-load-confirm");
    arenaAction(v, 4500, o, store, presenter);
    capture("arena-loaded");
    std::snprintf(v.arena.search.data(), 65, "archer");
    v.arena.filter(v.catalog);
    capture("arena-search");
    v.arena.search = {};
    v.arena.filter(v.catalog);
    std::snprintf(v.arena.seedText.data(), 65, "4294967296");
    capture("arena-invalid-seed");
    v.arena.syncSeed();
    const auto saved = v.arena.scenario;
    v.arena.scenario.count = 1;
    v.arena.scenario.roster[0].quantity = 256;
    capture("arena-capacity");
    v.arena.scenario = saved;
    std::ofstream(store.directory() / "broken.json") << "{";
    arenaAction(v, 4004, o, store, presenter);
    capture("arena-invalid-file");
    v.model.screen = MenuScreen::Arena;
    v.message("");
    if (!launchArena(o, v.catalog, v.arena.scenario, presenter))
        throw std::runtime_error("Capture launch failed");
}
} // namespace blobber
