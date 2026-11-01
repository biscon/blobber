#pragma once
#include "app/loading.hpp"
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "ui/interaction.hpp"
#include "ui/inventory_layout.hpp"
#include "ui/menu_screen.hpp"
namespace blobber::test {
inline void pc072ControlsReview(const std::filesystem::path& source, const std::filesystem::path& output) {
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Review output must be new");
    const auto catalog = loadRules(source / "rules/catalog.json");
    const auto defs = loadNpcDefinitions(source / "npcs/level.json");
    Window window(1920, 1080, true);
    GpuResources resources;
    Presenter presenter(resources, source);
    presenter.prepareMenu();
    CharacterLibrary library(catalog, output / "unused-library");
    MenuView menu(catalog, library);
    presenter.presentMenu(drawMenu, &menu);
    auto sample = [](UiInputEdges& edges, bool down, ArtPoint point) {
        std::array<bool, uiKeyCount> keys{};
        keys[std::size_t(UiKey::Click)] = down;
        return edges.sample(keys, true, point);
    };
    UiInputEdges menuEdges;
    check(!menu.pointerInput(sample(menuEdges, true, {320, 148})).has(UiKey::Click),
          "menu down does not activate");
    presenter.presentMenu(drawMenu, &menu);
    presenter.capture(output / "menu-pressed.png", 1280, 720);
    check(!menu.pointerInput(sample(menuEdges, false, {20, 20})).has(UiKey::Click),
          "outside release cancels menu press");
    presenter.presentMenu(drawMenu, &menu);
    presenter.capture(output / "menu-cancelled.png", 1280, 720);
    CreatorModel model;
    model.begin(catalog);
    std::snprintf(model.draft.name.data(), model.draft.name.size(), "Iris");
    model.refresh(catalog);
    CreatorView creator{catalog, model, {}};
    presenter.presentMenu(drawCreator, &creator);
    UiRect next{};
    for (unsigned i = 0; i < creator.controls.count; ++i)
        if (creator.controls.entries[i].action == FlowAction::Next && creator.controls.entries[i].enabled)
            next = creator.controls.entries[i].rect;
    check(next.width > 0, "creator review has an enabled Next control");
    UiInputEdges creatorEdges;
    const auto step = model.step;
    routeCreator(model, creator.controls, catalog,
                 sample(creatorEdges, true, {float(next.x + 5), float(next.y + 5)}), {}, .016f);
    check(model.step == step && creator.controls.buttons.feedback, "creator down only paints feedback");
    presenter.presentMenu(drawCreator, &creator);
    presenter.capture(output / "creator-pressed.png");
    routeCreator(model, creator.controls, catalog, sample(creatorEdges, false, {20, 20}), {}, .016f);
    check(model.step == step, "creator outside release cancels");
    const Map map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                   "#.......#", "#########"});
    const Player spawn{{4, 6}, Heading::North};
    NpcWorld none(map, spawn.cell, defs.types, {});
    GameSession game(map, catalog, none, 233, spawn, false, defs.timings);
    check(game.party().campSupplies().kits == 1, "production party starts with one kit total");
    WorldRenderer renderer(resources, world_assets::load(source / "prepared/world"));
    NpcAnimator animation(game.actors(), defs.timings);
    animation.setTime(0);
    GameView view(renderer);
    view.update(game, animation.snapshot(), {});
    CommandController commands(game, defs.timings);
    commands.refresh({}, false, false, true);
    const auto show = [&] { view.present(presenter, game, map, false, true, &commands.snapshot()); };
    show();
    UiInputEdges hudEdges;
    const auto action = hudLayout::action(4);
    const ArtPoint point{float(action.x + 8), float(action.y + 8)};
    const auto turn = game.combat().snapshot().turn;
    routeHudInput(commands, sample(hudEdges, true, point));
    check(game.combat().snapshot().turn == turn && commands.snapshot().buttons.feedback,
          "HUD press is visible before activation");
    show();
    presenter.capture(output / "hud-pressed.png");
    routeHudInput(commands, sample(hudEdges, false, {20, 20}));
    commands.openInventory();
    commands.inventorySelect(0);
    commands.refresh({}, false, false, true);
    const auto& inv = commands.snapshot().inventory;
    UiRect operation{};
    for (unsigned i = 0; i < inv.actions.size(); ++i)
        if (inv.actions[i].visible && inv.actions[i].enabled) {
            operation = inventoryLayout::action(inv, InventoryAction(i));
            break;
        }
    check(operation.width > 0, "inventory review has an enabled command");
    UiInputEdges invEdges;
    routeHudInput(commands, sample(invEdges, true, {float(operation.x + 4), float(operation.y + 4)}));
    check(commands.snapshot().buttons.feedback.has_value(), "inventory command shows pressed feedback");
    show();
    presenter.capture(output / "inventory-pressed.png");
    routeHudInput(commands, sample(invEdges, false, {630, 350}));
    commands.closeOverlay();
    commands.refresh({}, false, false, true);
    const auto counts = resources.counts();
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 6; ++i) {
        routeHudInput(commands, sample(hudEdges, true, point));
        show();
        routeHudInput(commands, sample(hudEdges, false, {20, 20}));
        show();
    }
    watching = false;
    check(!allocations && resources.counts().textures == counts.textures &&
              resources.counts().targets == counts.targets,
          "pressed composition creates no engine allocations or GPU resources");
    using GlError = unsigned (*)();
    check(reinterpret_cast<GlError>(rlGetProcAddress("glGetError"))() == 0,
          "pressed UI composition has no GL errors");
}
// Explicit static presentation review, outside routine tests and balance studies.
inline void pc072Review(const std::filesystem::path& source, const std::filesystem::path& output) {
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Review output must be a new directory");
    const auto catalog = loadRules(source / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(source / "npcs/level.json");
    Window window(1920, 1080, true);
    PreparationWorker worker;
    GpuResources resources;
    Presenter presenter(resources, source, &worker);
    WorldRenderer renderer(resources, world_assets::load(source / "prepared/world"));
    const auto type = [&](const char* name) {
        for (unsigned i = 0; i < definitions.types.size(); ++i)
            if (definitions.types[i] == name)
                return NpcTypeId{std::uint16_t(i)};
        throw std::runtime_error("Missing review species");
    };
    const Map map({"#########", "#.......#", "#.......#", "#....####", "#....####", "#....####", "#.......#",
                   "#.......#", "#########"});
    const Player spawn{{4, 6}, Heading::North};
    NpcWorld placements(map, spawn.cell, definitions.types,
                        {{type("goblin"), {4, 4}, {0, 1}, Heading::South},
                         {type("bugbear"), {4, 4}, {1, 1}, Heading::South},
                         {type("goblin_shaman"), {4, 4}, {0, 0}, Heading::South}});
    GameSession game(map, catalog, placements, 5, spawn, false, definitions.timings);
    NpcAnimator animation(game.actors(), definitions.timings);
    animation.setTime(0);
    GameView view(renderer);
    view.update(game, animation.snapshot(), {});
    CommandController commands(game, definitions.timings);
    commands.refresh(view.picking()->targets(), false, false, true);
    auto hud = commands.snapshot();
    // Fixed presentation-only notices model the crowded screenshot without
    // inventing combat decisions, RNG outcomes or a new expedition.
    hud.combat.phase = CombatPhase::Turns;
    hud.combat.upcomingCount = 6;
    for (unsigned i = 0; i < 6; ++i)
        hud.combat.upcoming[i].character = rules::CharacterId{std::uint16_t(i % 4)};
    hud.conditions[2] = 1u << unsigned(EffectKind::Bless);
    view.present(presenter, game, map, false, true, &hud);
    presenter.capture(output / "guardian-wall-bless.png");
    hud.magic.armed = true;
    hud.magic.area = true;
    hud.magic.kind = rules::SpellKind::BurningHands;
    hud.magic.affectedCount = 3;
    for (unsigned i = 0; i < 3; ++i)
        hud.magic.affected[i] = game.actors().id(i);
    view.present(presenter, game, map, false, true, &hud);
    presenter.capture(output / "shaman-targets.png");
    for (unsigned i = 0; i < 3; ++i) {
        const auto id = game.actors().id(i);
        check(game.actors().move(id, {4, 5}, game.actors().actor(id)->npc.subcell, spawn.cell).changed,
              "close review preserves three distinct subcells");
    }
    animation.setTime(0);
    view.update(game, animation.snapshot(), {});
    commands.refresh(view.picking()->targets(), false, false, true);
    hud.targets = commands.snapshot().targets;
    view.present(presenter, game, map, false, true, &hud);
    presenter.capture(output / "close-wall-targets.png");
    hud.magic.armed = false;
    view.present(presenter, game, map, false, true, &hud);
    presenter.capture(output / "close-wall-bless.png");
    presenter.presentSaving();
    presenter.capture(output / "saving.png");
    presenter.capture(output / "saving-1280.png", 1280, 720);
    const auto before = resources.counts();
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 8; ++i)
        presenter.presentSaving();
    watching = false;
    const auto after = resources.counts();
    check(!allocations && before.textures == after.textures && before.targets == after.targets,
          "cached saving frames allocate no engine C++ or GPU resources");
    {
        SavingScreen saving(&worker, presenter, [] { return true; });
        const auto completed = prepareCpu([] {
            preparationPoint("Validating staged save");
            return 42;
        });
        check(saving.closing() && completed == 42 && !worker.cancelled(),
              "native-close intent is deferred until save work joins without cancellation");
    }
    using GlError = unsigned (*)();
    check(reinterpret_cast<GlError>(rlGetProcAddress("glGetError"))() == 0,
          "review composition produces no OpenGL errors");
}
} // namespace blobber::test
