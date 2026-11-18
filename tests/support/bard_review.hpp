#pragma once
#include "bard_fixtures.hpp"
#include "ranger_review.hpp"
#include "ui/interaction.hpp"
#include "ui/roll_layout.hpp"

namespace blobber::test {
inline void bardReview(const std::filesystem::path& source, const std::filesystem::path& output,
                       const std::filesystem::path& prepared) {
    using namespace rules;
    check(std::filesystem::create_directory(output), "Bard capture destination must be new");
    const auto catalog = loadRules(source / "rules/catalog.json");
    auto draft = recommendedDraft(catalog, catalog.findRace("human"), catalog.findClass("bard"));
    std::snprintf(draft.name.data(), draft.name.size(), "Lyra");
    auto party = *catalog.preset();
    party.characters[0] = finishBuild(catalog, draft, "review.bard");
    const auto map = hybridRoom();
    const NpcWorld empty(map, spellSpawn.cell, {"goblin"}, {});
    const NpcWorld distant(map, spellSpawn.cell, {"goblin"}, {{{0}, {6, 4}, {0, 0}, Heading::South}});
    const NpcWorld close(map, spellSpawn.cell, {"goblin"}, {{{0}, {6, 7}, {0, 1}, Heading::South}});
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Review owns a hidden window");
    GpuResources resources;
    Presenter presenter(resources, source, nullptr, prepared);
    presenter.prepareMenu();
    const auto capture = [&](const char* name, GameSession& game, CommandController& ui) {
        ui.refresh(allCombatTargets(game), false, false, true);
        struct Context {
            GameSession& game;
            const HudSnapshot& hud;
        } ctx{game, ui.snapshot()};
        presenter.presentMenu(
            [](void* data, const UiFonts& fonts, const UiArt& art) {
                const auto& ctx = *static_cast<Context*>(data);
                drawUI(ctx.game.movement(), false, ctx.game.geometry().base(), &ctx.hud, &fonts, &art);
                drawHudOverlay(&ctx.hud, nullptr, &fonts, &art, &fonts);
            },
            &ctx, 0);
        presenter.capture(output / name);
    };
    {
        GameSession game(map, catalog, party, empty, 2, spellSpawn, false);
        levelBard(game.party(), 4);
        CommandController ui(game);
        ui.refresh({}, false, false, true);
        ui.selectCharacter(0);
        ui.openSpellbook();
        for (unsigned i = 0; i < ui.snapshot().magic.count; ++i)
            if (ui.snapshot().magic.entries[i].feature == Feature::BardicInspiration) {
                ui.chooseSpellbook(i);
                break;
            }
        ui.magicAlly(2);
        check(ui.confirm() == CommandReason::Resolved && game.party().character({2})->inspirationUntil,
              "Grant Inspiration through the live picker");
        ui.cancel();
        ui.refresh(allCombatTargets(game), false, false, true);
        const auto badge = rollLayout::heldDie(2);
        UiInput hover;
        hover.pointer = ArtPoint{float(badge.x + 1), float(badge.y + 1)};
        routeHudInput(ui, hover);
        check(ui.snapshot().hover.kind == HoverKind::Inspiration, "Held die explains its source and expiry");
        capture("inspired-portrait.png", game, ui);
        game.party().damage({0}, 9);
        game.update(7, false);
        ui.refresh({}, false, false, true);
        ui.openCamp();
        ui.campDice(0, 1);
        check(ui.snapshot().camp.preview.song && ui.snapshot().camp.request.dice[0],
              "Song of Rest is previewed");
        capture("song-of-rest.png", game, ui);
    }
    for (bool cutting : {false, true}) {
        bool captured = false;
        for (unsigned seed = 1; seed <= 32 && !captured; ++seed) {
            GameSession game(map, catalog, party, cutting ? close : distant, seed, spellSpawn, cutting);
            if (cutting)
                levelBard(game.party(), 3);
            else {
                check(game.submit(inspire(game, 2)).reason == CommandReason::Resolved, "Grant die");
                check(game.submit(inspiredShot(game)).reason == CommandReason::Resolved,
                      "Shoot from rear row");
            }
            for (unsigned n = 0; n < 100 && cutting && !game.pendingRollDecision().serial; ++n) {
                const auto& s = game.combat().snapshot();
                if (s.awaitingAction && s.activeCharacter)
                    game.submit(featureCommand(game, s.activeCharacter->index, Feature::EndTurn));
                game.update(.15, false);
            }
            auto d = game.pendingRollDecision();
            if (!d.serial)
                continue;
            check(d.purpose == (cutting ? RollPurpose::CuttingWords : RollPurpose::Attack),
                  "Expected decision");
            CommandController ui(game);
            capture(cutting ? "cutting-words.png" : "attack-rescue.png", game, ui);
            UiInput input;
            input.pressed[unsigned(UiKey::Confirm)] = true;
            routeHudInput(ui, input);
            check(!game.pendingRollDecision().serial && game.lastRollDecision().serial == d.serial,
                  "HUD resumes precisely the displayed roll");
            captured = true;
        }
        check(captured, "Reached a rescuable roll in the production party");
    }
}
} // namespace blobber::test
