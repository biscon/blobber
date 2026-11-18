#pragma once
#include "assets/rules_loader.hpp"
#include "game/commands.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "support/check.hpp"
#include "ui/creator_screen.hpp"
namespace blobber::test {
// Opt-in production UI capture. Small owned world; no desktop input or user files.
inline void levelFourReview(const std::filesystem::path& source, const std::filesystem::path& output,
                            const std::filesystem::path& prepared = {}) {
    check(std::filesystem::create_directory(output), "Level-four review output must be new");
    const auto catalog = loadRules(source / "rules/catalog.json");
    const Map map({"#######", "#.....#", "#.....#", "#.....#", "#######"});
    const Player spawn{{3, 2}, Heading::North};
    const NpcWorld none(map, spawn.cell, {"goblin"}, {});
    GameSession game(map, catalog, none, 5, spawn, false);
    for (unsigned level = 2; level <= 3; ++level) {
        game.party().grantNextLevelExperience();
        for (std::uint16_t i = 0; i < 4; ++i) {
            const auto& c = *game.party().character({i});
            auto r = game.advancementRequest({i});
            const auto context = rules::advancementChoices(catalog, c.build.view(), c.build.portrait,
                                                           c.build.spells, {}, true);
            r.choices.learned = rules::recommendedChoices(catalog, context, rules::SpellChoice::Learned);
            check(bool(game.advance(r)), "Review prepares level-three characters");
        }
    }
    game.party().grantNextLevelExperience();
    CommandController commands(game);
    commands.refresh({}, false, false, true);
    commands.openAdvancement({3});
    auto click = [&](FlowAction action, int value = -1) {
        FlowControls controls;
        layoutAdvancement(controls, commands.snapshot().advancement);
        for (unsigned i = 0; i < controls.count; ++i) {
            const auto& c = controls.entries[i];
            if (c.action != action || !c.enabled || (value >= 0 && c.value != value))
                continue;
            UiInput input;
            input.pointer = ArtPoint{float(c.rect.x + 2), float(c.rect.y + 2)};
            input.pressed[std::size_t(UiKey::Click)] = true;
            commands.routeAdvancement(input);
            return;
        }
        throw std::runtime_error("Level-four review route blocked");
    };
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Level-four review remains hidden");
    GpuResources resources;
    Presenter presenter(resources, source, nullptr, prepared);
    presenter.prepareMenu();
    const auto draw = [](void* context, const UiFonts& fonts, const UiArt& art) {
        const auto& hud = *static_cast<const HudSnapshot*>(context);
        HudPainter painter(&fonts, &art);
        if (hud.advancement.open)
            drawAdvancement(painter, hud.advancement);
        else
            painter.inventory(hud);
    };
    auto capture = [&](const char* name) {
        presenter.presentMenu(draw, const_cast<HudSnapshot*>(&commands.snapshot()), 0.0);
        presenter.capture(output / name);
    };
    click(FlowAction::Next);
    click(FlowAction::Next);
    click(FlowAction::Plus, 3);
    click(FlowAction::Plus, 3);
    capture("ability-scores.png");
    presenter.capture(output / "ability-scores-1280.png", 1280, 720);
    presenter.capture(output / "ability-scores-1024.png", 1024, 768);
    click(FlowAction::Next);
    click(FlowAction::Recommend);
    click(FlowAction::Next);
    click(FlowAction::Recommend);
    click(FlowAction::Next);
    check(commands.snapshot().advancement.selection.context.maximum == 2,
          "Production wizard earns two preparation slots");
    click(FlowAction::Recommend);
    capture("two-preparations.png");
    click(FlowAction::Next);
    capture("review.png");
    click(FlowAction::Done);
    commands.refresh({}, false, false, true);
    check(game.party().character({3})->build.level == 4, "Review commits level-four character");
    capture("character-sheet.png");
}
} // namespace blobber::test
