#pragma once
#include "bard_fixtures.hpp"
#include "progression_fixtures.hpp"
#include "ranger_review.hpp"
#include "ui/interaction.hpp"

namespace blobber::test {
inline void barbarianReview(const std::filesystem::path& source, const std::filesystem::path& output,
                            const std::filesystem::path& prepared) {
    using namespace rules;
    check(std::filesystem::create_directory(output), "Barbarian capture directory must be new");
    const auto catalog = loadRules(source / "rules/catalog.json");
    auto draft = recommendedDraft(catalog, catalog.findRace("human"), catalog.findClass("barbarian"));
    std::snprintf(draft.name.data(), draft.name.size(), "Ulf");
    auto party = *catalog.preset();
    for (unsigned kit : {1u, 0u}) {
        draft.gear[0] = kit;
        party.characters[0] = finishBuild(catalog, draft, "review.barbarian");
    }
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Review window stays hidden");
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

    const auto map = hybridRoom();
    const NpcWorld close(map, spellSpawn.cell, {"goblin"},
                         {{{0}, {6, 7}, {0, 1}, Heading::South}, {{0}, {6, 7}, {1, 1}, Heading::South}});
    GameSession g(map, catalog, party, close, 5, spellSpawn, false);
    advanceBarbarian(g.party(), 3);
    CommandController ui(g);
    const auto refresh = [&] { ui.refresh(allCombatTargets(g), false, false, true); };
    const auto capture = [&](const char* name) {
        struct Context {
            GameSession& g;
            const HudSnapshot& hud;
        } ctx{g, ui.snapshot()};
        presenter.presentMenu(
            [](void* data, const UiFonts& fonts, const UiArt& art) {
                const auto& c = *static_cast<Context*>(data);
                drawUI(c.g.movement(), false, c.g.geometry().base(), &c.hud, &fonts, &art);
                drawHudOverlay(&c.hud, nullptr, &fonts, &art, &fonts);
            },
            &ctx, 0);
        presenter.capture(output / name);
    };
    const auto choose = [&](Feature feature) {
        refresh();
        ui.selectCharacter(0);
        ui.openSpellbook();
        const auto& m = ui.snapshot().magic;
        for (unsigned i = 0; i < m.count; ++i) {
            if (m.entries[i].feature == feature) {
                ui.chooseSpellbook(i);
                return;
            }
        }
        throw std::runtime_error("Barbarian ability missing from picker");
    };
    levelReady(g, 0);
    choose(Feature::Rage);
    check(ui.confirm() == CommandReason::Resolved, "Rage via the player HUD");
    levelReady(g, 0);
    check(g.submit(featureCommand(g, 0, Feature::EndTurn)).reason == CommandReason::Resolved,
          "End activation turn");
    levelReady(g, 0);
    refresh();
    ui.selectCharacter(0);
    ui.openSpellbook();
    ui.selectSpellbookTab(MagicHud::abilityTab);
    const auto& book = ui.snapshot().magic;
    for (unsigned i = 0; i < book.visibleCount && book.entries[book.cursor].feature != Feature::FrenzyStrike;
         ++i)
        ui.browseSpellbook(1);
    check(book.entries[book.cursor].feature == Feature::FrenzyStrike, "Focus the available Frenzy swing");
    capture("abilities.png");
    ui.cancel();
    choose(Feature::RecklessAttack);
    ui.magicEnemy(g.actors().id(0));
    check(ui.confirm() == CommandReason::Resolved, "Reckless strike via the player HUD");
    levelReady(g, 0);
    refresh();
    const auto badge = hudLayout::rageBadge(0);
    UiInput hover;
    hover.pointer = ArtPoint{float(badge.x + 1), float(badge.y + 1)};
    routeHudInput(ui, hover);
    check(ui.snapshot().hover.kind == HoverKind::Barbarian && ui.snapshot().cards[0].frenzyReady,
          "Exposed Berserker retains a ready Frenzy");
    capture("rage-status.png");
    choose(Feature::FrenzyStrike);
    ui.magicEnemy(g.actors().id(1));
    check(ui.confirm() == CommandReason::Resolved && !g.party().character({0})->barbarian.frenzy,
          "Player spends the saved swing on another enemy");
}
} // namespace blobber::test
