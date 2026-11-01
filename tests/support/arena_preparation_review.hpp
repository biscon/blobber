#pragma once
#include "assets/rules_loader.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "support/arena_preparation_fixtures.hpp"
#include "ui/menu_screen.hpp"
namespace blobber::test {
// Explicit production UI export, outside the ordinary GPU correctness test.
inline void arenaPreparationReview(const std::filesystem::path& source, const std::filesystem::path& output) {
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Review output must be a new directory");
    const auto catalog = loadRules(source / "rules/catalog.json");
    CharacterLibrary library(catalog, output / "unused-library"); // Never loaded/written.
    Window window(1920, 1080, true);
    GpuResources resources;
    Presenter presenter(resources, source);
    presenter.prepareMenu();
    MenuView menu(catalog, library);
    menu.model.screen = MenuScreen::Arena;
    menu.arena.scenario = defaultArena(catalog);
    menu.arena.scenario.adjust(catalog.findMonster("bugbear"), 1);
    menu.arena.scenario.adjust(catalog.findMonster("goblin"), 1);
    menu.arena.scenario.adjust(catalog.findMonster("goblin_shaman"), 1);
    menu.arena.syncSeed();
    menu.arena.available.fill(true);
    menu.arena.filter(catalog);
    auto capture = [&](const char* name) {
        presenter.presentMenu(drawMenu, &menu);
        presenter.capture(output / (std::string(name) + ".png"));
    };
    auto& flow = menu.arena.preparation;
    flow.begin(menu.arena.scenario);
    capture("level-selector");
    flow.selectLevel(catalog, 3);
    flow.advancement.step = AdvancementStep::Prepared;
    flow.advancement.sync();
    capture("cleric-empty-slot");
    // Complete the same shared UI route used by manual input; retain selected boundaries.
    bool learned = false;
    for (unsigned n = 0; flow.step != ArenaPreparationStep::Review && n < 100; ++n) {
        flow.layout();
        auto& h = flow.advancement;
        if (!learned && h.step == AdvancementStep::Learned) {
            capture("wizard-learning");
            learned = true;
        }
        const auto action = h.step == AdvancementStep::Learned && !h.selection.valid() ? FlowAction::Recommend
                            : h.step == AdvancementStep::Review                        ? FlowAction::Done
                                                                                       : FlowAction::Next;
        bool found = false;
        for (unsigned i = 0; i < h.controls.count; ++i) {
            const auto control = h.controls.entries[i];
            if (control.action != action || !control.enabled)
                continue;
            UiInput input;
            input.pressed[std::size_t(UiKey::Click)] = true;
            input.pointer = ArtPoint{float(control.rect.x + 2), float(control.rect.y + 2)};
            flow.route(catalog, input, {}, .01f);
            found = true;
            break;
        }
        if (!found)
            throw std::runtime_error("Review preparation route blocked");
    }
    if (flow.step != ArenaPreparationStep::Review)
        throw std::runtime_error("Review did not finish");
    capture("level-three-review");
    if (!flow.commit(catalog, menu.arena.scenario))
        throw std::runtime_error("Review commit failed");
    capture("prepared-arena");
    arenaLevel(catalog, menu.arena.scenario, 2);
    capture("level-two-arena");
    presenter.capture(output / "level-two-arena-1280.png", 1280, 720);
}
} // namespace blobber::test
