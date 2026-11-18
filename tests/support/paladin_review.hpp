#pragma once
#include "assets/rules_loader.hpp"
#include "game/commands.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "support/check.hpp"
#include "ui/creator_screen.hpp"
#include "ui/hud.hpp"
#include "ui/interaction.hpp"
namespace blobber::test {
inline void paladinReview(const std::filesystem::path& source, const std::filesystem::path& output,
                          const std::filesystem::path& prepared = {}) {
    check(std::filesystem::create_directory(output), "Paladin capture output must be new");
    const auto catalog = loadRules(source / "rules/catalog.json");
    const auto cls = catalog.findClass("paladin");
    check(catalog.characterClass(cls), "Production paladin catalog loads");
    auto draft = rules::recommendedDraft(catalog, catalog.findRace("human"), cls);
    std::snprintf(draft.name.data(), draft.name.size(), "Ser Rowan");
    auto party = *catalog.preset();
    party.characters[0] = rules::finishBuild(catalog, draft, "review.paladin");
    const Map map({"#######", "#.....#", "#.....#", "#.....#", "#######"});
    const Player spawn{{3, 2}, Heading::North};
    const NpcWorld none(map, spawn.cell, {"goblin"}, {});
    GameSession game(map, catalog, party, none, 5, spawn, false);
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Paladin capture window stays hidden");
    GpuResources resources;
    Presenter presenter(resources, source, nullptr, prepared); presenter.prepareMenu();
    CreatorModel model; model.draft = draft; model.baseline = draft; model.step = CreatorStep::Class;
    model.refresh(catalog); CreatorView view{catalog, model, {}}; layoutCreator(view.controls, model, catalog);
    presenter.presentMenu(drawCreator, &view, 0); presenter.capture(output / "creation.png");
    for (unsigned level = 2; level <= 3; ++level) {
        game.party().grantNextLevelExperience();
        auto request = game.advancementRequest({0}); const auto& c = *game.party().character({0});
        const auto ctx = rules::advancementChoices(catalog, c.build.view(), c.build.portrait, c.build.spells, {}, rules::SpellChoice::Prepared);
        request.choices.prepared = rules::recommendedChoices(catalog, ctx, rules::SpellChoice::Prepared);
        check(bool(game.advance(request)), "Production paladin advances");
    }
    game.party().grantNextLevelExperience();
    CommandController commands(game); commands.refresh({}, false, false, true); commands.openAdvancement({0});
    struct Context { CommandController& commands; GameSession& game; } context{commands, game};
    auto draw = [](void* context, const UiFonts& fonts, const UiArt& art) {
        const auto& ctx = *static_cast<const Context*>(context);
        const auto& hud = ctx.commands.snapshot();
        if (hud.advancement.open) drawAdvancement(HudPainter(&fonts, &art), hud.advancement);
        else {
            drawUI(ctx.game.movement(), false, ctx.game.geometry().base(), &hud, &fonts, &art);
            drawHudOverlay(&hud, nullptr, &fonts, &art);
        }
    };
    auto capture = [&](const char* name) { commands.refresh({}, false, false, true); presenter.presentMenu(draw, &context, 0); presenter.capture(output / name); };
    auto click = [&](FlowAction action, int value = -1) {
        FlowControls controls; layoutAdvancement(controls, commands.snapshot().advancement);
        for (unsigned i = 0; i < controls.count; ++i) {
            const auto& c = controls.entries[i];
            if (c.action != action || !c.enabled || (value >= 0 && c.value != value)) continue;
            UiInput input; input.pointer = ArtPoint{float(c.rect.x + 2), float(c.rect.y + 2)};
            input.pressed[std::size_t(UiKey::Click)] = true; commands.routeAdvancement(input); return;
        }
        throw std::runtime_error("Paladin capture advancement route blocked");
    };
    click(FlowAction::Next); click(FlowAction::Next); click(FlowAction::Plus, 5); click(FlowAction::Plus, 5);
    capture("advancement.png"); click(FlowAction::Next);
    if (commands.snapshot().advancement.step == AdvancementStep::Prepared) { click(FlowAction::Recommend); click(FlowAction::Next); }
    click(FlowAction::Done); commands.refresh({}, false, false, true);
    check(game.party().character({0})->build.level == 4, "Production paladin reaches level four");
    commands.openInventory(rules::CharacterId{0}, CharacterPage::Character); capture("character-sheet.png"); commands.closeOverlay();
    commands.selectCharacter(0); commands.openSpellbook(); commands.selectSpellbookTab(MagicHud::abilityTab); capture("abilities.png");
    std::size_t healing = 0;
    for (std::size_t i = 0; i < commands.snapshot().magic.count; ++i)
        if (commands.snapshot().magic.entries[i].feature == rules::Feature::LayOnHands) healing = i;
    game.party().damage({1}, 8); commands.refresh({}, false, false, true); commands.chooseSpellbook(healing);
    commands.magicAlly(1);
    check(commands.snapshot().magic.pending.healingAmount == 8, "Healing defaults to a full useful heal");
    capture("healing.png");
    presenter.capture(output / "healing-1280.png", 1280, 720);
    // Check the same renderer at level one, before spell mana exists.
    GameSession novice(map, catalog, party, none, 5, spawn, false);
    CommandController noviceUi(novice); noviceUi.refresh({}, false, false, true);
    noviceUi.openInventory(rules::CharacterId{0}, CharacterPage::Character);
    Context noviceContext{noviceUi, novice};
    presenter.presentMenu(draw, &noviceContext, 0);
    presenter.capture(output / "level-one-character-sheet.png");
    noviceUi.closeOverlay(); novice.party().damage({1}, 8);
    check(novice.party().useLayOnHands({0}, {1}, 3), "Prepare two remaining production healing points");
    noviceUi.refresh({}, false, false, true); noviceUi.selectCharacter(0); noviceUi.openSpellbook();
    for (std::size_t i = 0; i < noviceUi.snapshot().magic.count; ++i)
        if (noviceUi.snapshot().magic.entries[i].feature == rules::Feature::LayOnHands) {
            noviceUi.chooseSpellbook(i); break;
        }
    noviceUi.magicAlly(1);
    UiInputEdges edges; std::array<bool, uiKeyCount> down{};
    const auto handle = hudLayout::healingHandle(2, 2);
    ArtPoint point{float(handle.x + handle.width - 1), float(handle.y + 2)};
    auto frame = [&] {
        noviceUi.refresh({}, false, false, true);
        const auto logical = routeHudButtons(noviceUi, edges.sample(down, true, point));
        routeHudInput(noviceUi, logical);
    };
    down[std::size_t(UiKey::Click)] = true; frame();
    check(noviceUi.snapshot().magic.healingDragging && noviceUi.snapshot().magic.pending.healingAmount == 2,
          "Production handle press preserves healing and captures physical input");
    point.x -= hudLayout::healingTrack.width * .75f / 2; frame();
    check(noviceUi.snapshot().magic.pending.healingAmount == 1 &&
              std::abs(noviceUi.snapshot().magic.healingDragAmount - 1.25f) < .001f &&
              novice.party().character({0})->layOnHands == 2,
          "Production held drag shows a fractional position with integer spending");
    presenter.presentMenu(draw, &noviceContext, 0);
    presenter.capture(output / "level-one-healing.png");
    presenter.capture(output / "level-one-healing-1280.png", 1280, 720);
    down[std::size_t(UiKey::Click)] = false; frame();
    check(!noviceUi.snapshot().magic.healingDragging && noviceUi.snapshot().magic.pending.healingAmount == 1,
          "Production release snaps to the nearest point");
}
} // namespace blobber::test
