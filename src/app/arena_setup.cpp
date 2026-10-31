#include "app/arena_setup.hpp"
#include "app/menu.hpp"
#include "assets/npc_metadata.hpp"
#include "ui/menu_screen.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstring>
#include <random>
namespace blobber {
void openArena(MenuView& v, const Options& o, ArenaScenarioStore& store) {
    auto& a = v.arena;
    if (!a.initialized) {
        a.scenario = defaultArena(v.catalog);
        a.scenario.seed = o.seed.value_or(std::random_device{}());
        a.syncSeed();
        a.initialized = true;
    }
    a.available.fill(false);
    const auto definitions = loadNpcDefinitions(o.assets / "npcs/level.json");
    for (std::uint16_t i = 0; i < v.catalog.monsterCount(); ++i)
        a.available[i] = std::find(definitions.types.begin(), definitions.types.end(),
                                   monsterAppearance(v.catalog.monster({i})->id)) != definitions.types.end();
    std::snprintf(a.folder.data(), a.folder.size(), "%s", store.directory().c_str());
    a.filter(v.catalog);
    a.blur();
    v.model.screen = MenuScreen::Arena;
    v.message("");
}
bool arenaAction(MenuView& v, int action, const Options& o, ArenaScenarioStore& store, Presenter& presenter) {
    auto& a = v.arena;
    try {
        if (action == 4000)
            openArena(v, o, store);
        else if (action == 4001) {
            v.dungeonChoices = v.model.choices;
            v.model.beginArenaPicker(a.scenario, v.catalog);
        } else if (action == 4002 || action == 4003) {
            if (const auto* error = a.error(v.catalog))
                throw std::runtime_error(error);
            const auto saved =
                store.save(a.scenario, action == 4002 && a.currentFile.file[0] ? &a.currentFile : nullptr);
            a.currentFile = saved.entry;
            v.message(saved.durable ? "Scenario saved."
                                    : "Scenario saved, but durability could not be confirmed. Reload before "
                                      "further changes.");
        } else if (action == 4004 || action == 4009) {
            auto listing = store.list();
            a.listing = listing;
            a.firstFile = 0;
            v.model.screen = MenuScreen::Scenarios;
        } else if (action == 4005) {
            if (const auto* error = a.error(v.catalog))
                throw std::runtime_error(error);
            const auto snapshot = a.scenario;
            v.model.screen = MenuScreen::Loading;
            presenter.presentMenu(drawMenu, &v);
            if (!launchArena(o, v.catalog, snapshot, presenter))
                return false;
            v.model.screen = MenuScreen::Arena;
        } else if (action == 4006) {
            a.scenario.seed = std::random_device{}();
            a.syncSeed();
        } else if (action == 4007 && a.first)
            --a.first;
        else if (action == 4008 && a.first + 5 < a.visibleCount)
            ++a.first;
        else if (action == 4010)
            v.model.screen = MenuScreen::Arena;
        else if (action == 4011)
            a.nameEdit.focus(true, a.scenario.name);
        else if (action == 4012)
            a.seedEdit.focus(true, a.seedText);
        else if (action == 4013)
            a.searchEdit.focus(true, a.search);
        else if (action == 4014 && a.firstFile)
            --a.firstFile;
        else if (action == 4015 && a.firstFile + 7 < a.listing.count)
            ++a.firstFile;
        else if (action >= 4100 && action < 4132) {
            const auto i = std::uint16_t(action - 4100);
            if (a.available[i] && v.catalog.monster({i}))
                a.scenario.adjust({i}, 1);
        } else if (action >= 4200 && action < 4232)
            a.scenario.adjust({std::uint16_t(action - 4200)}, -1);
        else if (action >= 4300 && action < 4428 && action - 4300 < int(a.listing.count)) {
            const auto& entry = a.listing.entries[std::size_t(action - 4300)];
            if (entry.error[0])
                throw std::runtime_error(entry.error.data());
            a.pendingLoad = action - 4300;
        } else if (action == 4500 && a.pendingLoad >= 0) {
            auto loaded = store.load(a.listing.entries[std::size_t(a.pendingLoad)]);
            a.scenario = loaded;
            a.currentFile = a.listing.entries[std::size_t(a.pendingLoad)];
            a.pendingLoad = -1;
            a.syncSeed();
            v.model.screen = MenuScreen::Arena;
            v.message("");
        } else if (action == 4501)
            a.pendingLoad = -1;
    } catch (const std::exception& e) {
        if (v.model.screen == MenuScreen::Loading)
            v.model.screen = MenuScreen::Arena;
        a.pendingLoad = -1;
        v.message(e.what());
    }
    return true;
}
} // namespace blobber
