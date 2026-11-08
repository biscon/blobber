#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "ui/interaction.hpp"
#include <iostream>
using namespace blobber;
int main(int argc, char** argv) {
    try {
        if (argc != 3)
            return 2;
        const std::filesystem::path assets(argv[1]), output(argv[2]);
        if (!std::filesystem::create_directory(output))
            throw std::runtime_error("Use a new capture directory");
        const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
        const auto catalog = loadRules(assets / "rules/catalog.json");
        auto archer = std::find(definitions.types.begin(), definitions.types.end(), "goblin.archer");
        check(archer != definitions.types.end(), "Production archer appearance");
        WorldDefinition definition;
        definition.assetPack = "underkeep";
        const Map map({"#########", "###...###", "###...###", "###...###", "###...###", "###...###",
                       "###...###", "###...###", "#########"},
                      definition);
        const Player pose{{4, 6}, Heading::North};
        NpcWorld places(
            map, pose.cell, definitions.types,
            {{{0}, {4, 5}, {1, 1}, Heading::South},
             {{std::uint16_t(archer - definitions.types.begin())}, {4, 5}, {1, 0}, Heading::South},
             {{0}, {4, 1}, {1, 1}, Heading::South}});
        GameSession game(map, catalog, places, 5, {{4, 7}, Heading::North}, false, definitions.timings);
        Window window(1920, 1080, true);
        check(IsWindowState(FLAG_WINDOW_HIDDEN), "Capture window stays hidden");
        GpuResources resources;
        Presenter presenter(resources, assets);
        WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
        NpcAnimator animator(game.actors(), definitions.timings);
        animator.setTime(0);
        GameView view(renderer);
        view.update(game, animator.snapshot(), {});
        CommandController ui(game, definitions.timings);
        ui.refresh(view.picking()->targets(), false, false, true);
        auto capture = [&](const char* name) {
            animator.setTime(game.timeMs(), &game.effects());
            view.update(game, animator.snapshot(), {});
            ui.refresh(view.picking()->targets(), false, false, true);
            view.present(presenter, game, map, false, true, &ui.snapshot());
            presenter.capture(output / (std::string(name) + ".png"));
            presenter.capture(output / (std::string(name) + "-1280.png"), 1280, 720);
            using GlError = unsigned (*)();
            check(reinterpret_cast<GlError>(rlGetProcAddress("glGetError"))() == 0,
                  "Target picker GL integration");
        };
        ui.toggleTargetPicker();
        check(!ui.snapshot().targetPicker.available && !ui.snapshot().targetPicker.open,
              "Production exploration hides Targets");
        capture("exploration");
        game.restorePose(pose, false);
        game.update(0, false);
        spellReady(game, 2);
        animator.setTime(game.timeMs(), &game.effects());
        view.update(game, animator.snapshot(), {});
        ui.refresh(view.picking()->targets(), false, false, true);
        ui.selectCharacter(2);
        ui.selectAction(PartyAction::Ranged);
        ui.cycleEnemyTarget(1);
        ui.cycleEnemyTarget(1);
        check(ui.snapshot().selection.target && ui.snapshot().selection.target->slot == 1 &&
                  ui.snapshot().targetPicker.canConfirm,
              "Production rear archer is an eligible bow target");
        ui.toggleTargetPicker();
        check(ui.snapshot().targets.candidateCount == 3 && ui.snapshot().targetPicker.candidateCount == 2 &&
                  !game.combat().member(game.actors().id(2)),
              "Production picker excludes unrelated visible enemy");
        capture("bow");
        ui.closeTargetPicker();
        capture("bow-tab");
        ui.selectAction(PartyAction::Melee);
        ui.toggleTargetPicker();
        capture("blocked");
        ui.closeTargetPicker();
        spellReady(game, 3);
        animator.setTime(game.timeMs(), &game.effects());
        view.update(game, animator.snapshot(), {});
        ui.refresh(view.picking()->targets(), false, false, true);
        ui.selectCharacter(3);
        ui.openSpellbook();
        for (unsigned i = 0; i < ui.snapshot().magic.count; ++i)
            if (ui.snapshot().magic.entries[i].kind == rules::SpellKind::MagicMissile) {
                ui.chooseSpellbook(i);
                break;
            }
        ui.cycleEnemyTarget(1);
        ui.cycleEnemyTarget(1);
        ui.toggleTargetPicker();
        ui.activateTargetRow(1);
        check(ui.snapshot().magic.assigned == 1 && ui.snapshot().magic.pending.enemies[0].slot == 1,
              "Production archer receives a pending dart without casting");
        capture("magic-missile");
        std::cout << "Hidden targeting captures exported\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
