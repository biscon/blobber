#pragma once
#include "assets/rules_loader.hpp"
#include "game/commands.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "render/picking.hpp"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "ui/creator_screen.hpp"
#include "ui/hud.hpp"

namespace blobber::test {
inline void stageRangerReviewTarget(PickingBuffer& picking, ActorId enemy) {
    constexpr int center = viewWidth / 2, top = viewHeight / 2 - 20;
    for (int y = top; y < top + 40; ++y)
        for (int x = center - 10; x < center + 10; ++x)
            picking.writeDepth(std::size_t(y) * viewWidth + x, enemy.slot, 3.f);
    picking.badgeAnchor(enemy.slot, center, top, 3.f);
}

inline void rangerReview(const std::filesystem::path& source, const std::filesystem::path& output,
                         const std::filesystem::path& prepared) {
    using namespace rules;
    check(std::filesystem::create_directory(output), "Ranger capture output must be new");
    const auto catalog = loadRules(source / "rules/catalog.json");
    auto draft = recommendedDraft(catalog, catalog.findRace("human"), catalog.findClass("ranger"));
    std::snprintf(draft.name.data(), draft.name.size(), "Rowan");
    auto party = *catalog.preset();
    party.characters[0] = finishBuild(catalog, draft, "review.ranger");
    const Map map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#", "#########"});
    const Player spawn{{4, 4}, Heading::North};
    const NpcWorld placements(map, spawn.cell, {"goblin"}, {{{0}, {4, 1}, {0, 1}, Heading::North}});
    GameSession game(map, catalog, party, placements, 5, spawn, false);
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Ranger capture window stays hidden");
    GpuResources resources;
    Presenter presenter(resources, source, nullptr, prepared);
    presenter.prepareMenu();
    CreatorModel model;
    model.draft = model.baseline = draft;
    model.step = CreatorStep::Class;
    model.refresh(catalog);
    CreatorView view{catalog, model, {}};
    layoutCreator(view.controls, model, catalog);
    presenter.presentMenu(drawCreator, &view, 0);
    presenter.capture(output / "creation.png");

    CommandController commands(game);
    const auto enemy = game.actors().id(0);
    PickingBuffer picking;
    stageRangerReviewTarget(picking, enemy);
    const auto refresh = [&] {
        picking.finish(&game.actors());
        commands.refresh(picking.targets(), false, false, true);
    };
    struct Context { CommandController& commands; GameSession& game; const PickingBuffer& picking; };
    Context context{commands, game, picking};
    const auto draw = [](void* data, const UiFonts& fonts, const UiArt& art) {
        const auto& ctx = *static_cast<Context*>(data);
        const auto& hud = ctx.commands.snapshot();
        if (hud.advancement.open)
            drawAdvancement(HudPainter(&fonts, &art), hud.advancement);
        else {
            drawUI(ctx.game.movement(), false, ctx.game.geometry().base(), &hud, &fonts, &art);
            drawHudOverlay(&hud, &ctx.picking, &fonts, &art, &fonts);
        }
    };
    const auto capture = [&](const char* name) {
        presenter.presentMenu(draw, &context, 0);
        presenter.capture(output / name);
    };
    const auto click = [&](FlowAction action) {
        FlowControls controls;
        layoutAdvancement(controls, commands.snapshot().advancement);
        for (unsigned i = 0; i < controls.count; ++i) {
            const auto& row = controls.entries[i];
            if (row.action == action && row.enabled) {
                UiInput input;
                input.pointer = ArtPoint{float(row.rect.x + 2), float(row.rect.y + 2)};
                input.pressed[std::size_t(UiKey::Click)] = true;
                commands.routeAdvancement(input);
                return;
            }
        }
        throw std::runtime_error("Ranger capture advancement route blocked");
    };
    game.party().grantNextLevelExperience();
    refresh();
    commands.openAdvancement({0});
    click(FlowAction::Next);
    click(FlowAction::Next);
    check(commands.snapshot().advancement.step == AdvancementStep::Learned, "Ranger healing choice opens");
    click(FlowAction::Recommend);
    capture("advancement.png");
    click(FlowAction::Next);
    click(FlowAction::Done);
    for (int level = 3; level <= 4; ++level) {
        game.party().grantNextLevelExperience();
        auto request = game.advancementRequest({0});
        if (level == 4)
            request.choices.abilityImprovement.increases[1] = 2;
        check(bool(game.advance(request)), "Production Ranger advances");
    }
    refresh();
    commands.openInventory(CharacterId{0}, CharacterPage::Character);
    capture("character-sheet.png");
    commands.closeOverlay();
    commands.selectCharacter(0);
    commands.openSpellbook();
    capture("spell-picker.png");
    for (std::size_t i = 0; i < commands.snapshot().magic.count; ++i)
        if (commands.snapshot().magic.entries[i].kind == SpellKind::HuntersMark) {
            commands.chooseSpellbook(i);
            break;
        }
    commands.magicEnemy(enemy);
    check(commands.confirm() == CommandReason::Resolved && game.effects().markedBy({0}, enemy),
          "Production target picker casts Hunter's Mark");
    refresh();
    commands.hover(HoverKind::Enemy, 0, enemy, 255, 155);
    game.update(.26, false);
    refresh();
    check(commands.snapshot().hover.tooltipVisible &&
              std::string_view(commands.snapshot().hover.lines[2].data()).find("Hunter's Mark") != std::string_view::npos,
          "Marked target exposes the caster-specific tooltip");
    capture("marked-target.png");
    presenter.capture(output / "marked-target-1280.png", 1280, 720);
}

inline void rangedScreeningReview(const std::filesystem::path& source, const std::filesystem::path& output,
                                  const std::filesystem::path& prepared) {
    using namespace rules;
    check(std::filesystem::create_directory(output), "Screening capture output must be new");
    const auto catalog = loadRules(source / "rules/catalog.json");
    auto draft = recommendedDraft(catalog, catalog.findRace("human"), catalog.findClass("ranger"));
    std::snprintf(draft.name.data(), draft.name.size(), "Rowan");
    auto party = *catalog.preset();
    party.characters[0] = finishBuild(catalog, draft, "review.ranger");
    const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
    const Player spawn{{3, 3}, Heading::North};
    const NpcWorld placements(map, spawn.cell, {"goblin"}, {{{0}, {3, 2}, {0, 1}, Heading::South}});
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Screening capture window stays hidden");
    GpuResources resources;
    Presenter presenter(resources, source, nullptr, prepared);
    presenter.prepareMenu();
    for (const bool screened : {false, true}) {
        if (screened)
            std::swap(party.formation[0], party.formation[2]);
        GameSession game(map, catalog, party, placements, 5, spawn, false);
        spellReady(game, 0);
        const auto enemy = game.actors().id(0);
        PickingBuffer picking;
        stageRangerReviewTarget(picking, enemy);
        CommandController ui(game);
        const auto refresh = [&] {
            picking.finish(&game.actors());
            ui.refresh(picking.targets(), false, false, true);
        };
        refresh();
        ui.selectCharacter(screened ? 2 : 0);
        ui.selectAction(PartyAction::Ranged);
        ui.hover(HoverKind::Enemy, 0, enemy, 255, 155);
        game.update(.26, false);
        refresh();
        const auto& hover = ui.snapshot().hover;
        check(hover.tooltipVisible && hover.reason == CommandReason::Ready &&
                  std::string_view(hover.lines[5].data()) ==
                      (screened ? "" : "Disadvantage: enemy in melee reach"),
              "Native hover warns only for the exposed Ranger");
        struct Context { const GameSession& game; const HudSnapshot& hud; const PickingBuffer& picking; };
        Context context{game, ui.snapshot(), picking};
        presenter.presentMenu([](void* opaque, const UiFonts& fonts, const UiArt& art) {
            const auto& ctx = *static_cast<Context*>(opaque);
            drawUI(ctx.game.movement(), false, ctx.game.geometry().base(), &ctx.hud, &fonts, &art);
            drawHudOverlay(&ctx.hud, &ctx.picking, &fonts, &art, &fonts);
        }, &context, 0);
        presenter.capture(output / (screened ? "screened.png" : "exposed.png"));
    }
}
} // namespace blobber::test
