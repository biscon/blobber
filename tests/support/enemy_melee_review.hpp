#pragma once
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include <fstream>

namespace blobber::test {
// Opt-in production-art review; ordinary enemy timelines, no desktop input.
inline void enemyMeleeReview(const std::filesystem::path& assets, const std::filesystem::path& output,
                             std::string_view onlySpecies = {}) {
    if (std::filesystem::exists(output) && !std::filesystem::is_empty(output))
        throw std::runtime_error("Enemy melee review output must be new or empty");
    std::filesystem::create_directories(output);
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    const auto catalog = loadRules(assets / "rules/catalog.json");
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Enemy melee review stays hidden");
    GpuResources resources;
    Presenter presenter(resources, assets);
    WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
    std::ofstream report(output / "checks.txt");
    for (const char* species : {"goblin", "giant.rat", "bugbear", "zombie"}) {
        if (!onlySpecies.empty() && onlySpecies != species)
            continue;
        const auto found = std::find(definitions.types.begin(), definitions.types.end(), species);
        check(found != definitions.types.end(), "Review species has production art");
        const NpcTypeId type{std::uint16_t(found - definitions.types.begin())};
        WorldDefinition definition;
        definition.assetPack = "underkeep";
        const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"},
                      definition);
        const Player pose{{3, 3}, Heading::North};
        NpcWorld places(map, pose.cell, definitions.types, {{type, {3, 2}, {0, 1}, Heading::South}});
        GameSession game(map, catalog, places, 5, pose, false, definitions.timings);
        CommandController commands(game, definitions.timings);
        NpcAnimator animator(game.actors(), definitions.timings);
        GameView view(renderer);
        auto present = [&] {
            animator.setTime(game.timeMs(), &game.effects());
            view.update(game, animator.snapshot(), {});
            commands.refresh(view.picking()->targets(), false, false, true);
            view.present(presenter, game, map, false, true, &commands.snapshot());
        };
        AttackVisual attack;
        for (unsigned i = 0; i < 2000; ++i) {
            const auto& pending = game.combat().attacks().attacks[0];
            if (pending.stage == AttackStage::Windup && !pending.ranged &&
                pending.spell == rules::SpellKind::Count) {
                attack = pending;
                break;
            }
            const auto& combat = game.combat().snapshot();
            if (combat.awaitingAction && combat.activeCharacter)
                commands.record(game.submit(turnCommand(game)));
            commands.record(game.update(.02, false));
        }
        check(attack.id.serial && game.timeMs() <= attack.impactMs - 160,
              "Ordinary enemy melee starts before review window");
        const auto actor = game.actors().actor(game.actors().id(0))->npc;
        const auto directory = output / species;
        std::filesystem::create_directory(directory);
        unsigned impacts = 0;
        for (unsigned frame = 0; frame <= 20; ++frame) {
            const auto time = attack.impactMs - 160 + frame * 20;
            const auto& events = game.update(double(time - game.timeMs()) / 1000, false);
            for (const auto& event : events)
                impacts += event.event == CombatEvent::Attack && event.attackId.serial == attack.id.serial;
            commands.record(events);
            present();
            check(view.picking()->targets().count == 1, "Lunging creature remains pickable");
            const auto& current = game.actors().actor(game.actors().id(0))->npc;
            check(current.cell == actor.cell && current.subcell == actor.subcell,
                  "Lunge preserves authoritative cell and subcell");
            if (frame == 8) {
                allocations = 0;
                watching = true;
                present();
                watching = false;
                check(!allocations, "Warmed melee lunge presentation allocates nothing");
            }
            presenter.capture(directory / (std::to_string(frame) + ".png"));
            using Error = unsigned (*)();
            check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                  "Enemy lunge render has no GL errors");
        }
        check(impacts == 1, "Lunge preserves exactly one authoritative attack impact");
        report << species << " impact=" << attack.impactMs - attack.startedMs << "ms hit=" << attack.hit
               << " captures=-160..240ms at 20ms; picking, allocations and GL pass\n";
    }
}
} // namespace blobber::test
