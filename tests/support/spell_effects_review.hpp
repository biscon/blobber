#pragma once
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "support/spell_parties.hpp"
#include <fstream>
#include <iostream>
namespace blobber::test {
inline void spellEffectsReview(const std::filesystem::path& assets, const std::filesystem::path& output,
                               const char* selected = nullptr) {
    if (std::filesystem::exists(output) && !std::filesystem::is_empty(output))
        throw std::runtime_error("SE01 review output must be new or empty");
    std::filesystem::create_directories(output);
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    const auto catalog = loadRules(assets / "rules/catalog.json");
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "SE01 review remains hidden");
    GpuResources resources;
    Presenter presenter(resources, assets);
    WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
    struct Case {
        const char* name;
        rules::SpellKind spell;
        int range, count;
        bool motion, bright;
    };
    const Case cases[]{{"fire-bolt", rules::SpellKind::FireBolt, 3, 1, true, false},
                       {"ray-of-frost", rules::SpellKind::RayOfFrost, 2, 1, true, false},
                       {"burning-hands", rules::SpellKind::BurningHands, 1, 3, true, false},
                       {"fire-near", rules::SpellKind::FireBolt, 1, 1, false, false},
                       {"frost-far", rules::SpellKind::RayOfFrost, 4, 1, false, false},
                       {"cone-sparse", rules::SpellKind::BurningHands, 1, 1, false, false},
                       {"fire-bright", rules::SpellKind::FireBolt, 2, 1, false, true}};
    std::ofstream report(output / "checks.txt");
    for (const auto& example : cases) {
        if (selected && std::string_view(selected) != example.name)
            continue;
        WorldDefinition definition;
        definition.assetPack = example.bright ? "sanctuary-city" : "underkeep";
        if (example.bright) {
            definition.surfaces.resize(13 * 13);
            for (auto& s : definition.surfaces) {
                s.environment = Environment::Daylight;
                s.ceiling = noCeiling;
            }
        }
        std::vector<std::string> rows(13, "#...........#");
        rows.front() = rows.back() = "#############";
        if (std::string_view(example.name) == "fire-bolt")
            for (unsigned y = 2; y <= 9; ++y)
                rows[y][5] = rows[y][7] = '#';
        const Map map(rows, definition);
        const Player pose{{6, 8}, Heading::North};
        const auto name = example.spell == rules::SpellKind::BurningHands ? "goblin" : "bugbear";
        const auto found = std::find(definitions.types.begin(), definitions.types.end(), name);
        check(found != definitions.types.end(), "Review species has production art");
        const NpcTypeId type{std::uint16_t(found - definitions.types.begin())};
        std::vector<NpcInstance> instances;
        for (int i = 0; i < example.count; ++i)
            instances.push_back(
                {type, {6 + (example.count > 1 ? i - 1 : 0), 8 - example.range}, {0, 1}, Heading::South});
        NpcWorld places(map, pose.cell, definitions.types, instances);
        const auto party = leveledSpellParty(catalog, example.spell);
        GameSession game(map, catalog, party, places, 5, pose, false, definitions.timings);
        CommandController commands(game, definitions.timings);
        NpcAnimator animator(game.actors(), definitions.timings);
        GameView view(renderer);
        auto present = [&] {
            animator.setTime(game.timeMs(), &game.effects());
            view.update(game, animator.snapshot(), {});
            commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, false, false,
                             true);
            view.present(presenter, game, map, false, true, &commands.snapshot());
        };
        if (example.range == 1)
            spellReady(game, 3);
        present();
        auto command = spellCommand(game, example.spell);
        if (example.spell != rules::SpellKind::BurningHands) {
            command.targetCount = 1;
            command.enemies[0] = game.actors().id(0);
        }
        const auto accepted = game.submit(command);
        commands.record(accepted);
        check(accepted.reason == CommandReason::Resolved, "Review casts through ordinary spell command");
        const auto start = game.timeMs();
        const auto& cue = game.combat().spellVisuals().casts[0];
        const auto impact = cue.impactMs - start;
        report << example.name << " release=" << cue.releaseMs - start << " impact=" << impact
               << " initialHit=" << cue.targets[0].hit << '\n';
        if (example.motion)
            std::filesystem::create_directory(output / example.name);
        // Captures are deterministic application-owned targets, never OS screenshots/input.
        for (unsigned frame = 0; frame <= 22; ++frame) {
            const auto offset = frame * 60u;
            if (offset)
                commands.record(game.update(double(start + offset - game.timeMs()) / 1000, false));
            present();
            if (example.motion) {
                const auto filename = std::string(frame < 10 ? "0" : "") + std::to_string(frame) + ".png";
                presenter.capture(output / example.name / filename);
            }
            if (offset == 120 || offset == 480) {
                if (example.motion || offset == 120)
                    presenter.capture(output /
                                      (std::string(example.name) + "-" + std::to_string(offset) + ".png"));
            }
        }
        if (example.spell == rules::SpellKind::RayOfFrost && example.motion) {
            commands.record(game.update(double(start + 1700 - game.timeMs()) / 1000, false));
            present();
            presenter.capture(output / "ray-of-frost-slow.png");
        }
        const auto memory = renderer.memory();
        report << "textures=" << memory.textureCount << " mip=" << memory.residentTextures
               << " alpha=" << memory.residentAlpha << " worldGpu=" << memory.worldGpu
               << " applicationGpu=" << memory.applicationGpu << " peak=" << memory.peakApplicationGpu
               << '\n';
        if (example.bright) {
            WorldDefinition destinationDefinition;
            destinationDefinition.assetPack = "underkeep";
            const Map destination(rows, destinationDefinition);
            NpcWorld destinationActors(destination, pose.cell, definitions.types, instances);
            GameSession destinationGame(destination, catalog, party, destinationActors, 5, pose, false,
                                        definitions.timings);
            NpcAnimator destinationAnimator(destinationGame.actors(), definitions.timings);
            destinationAnimator.setTime(0);
            GameView staged(renderer);
            staged.update(destinationGame, destinationAnimator.snapshot(), {});
            const auto overlap = renderer.memory();
            report << "city-dungeon staged textures=" << overlap.textureCount
                   << " mip=" << overlap.residentTextures << " alpha=" << overlap.residentAlpha
                   << " worldGpu=" << overlap.worldGpu << " applicationGpu=" << overlap.applicationGpu
                   << " peak=" << overlap.peakApplicationGpu << '\n';
        }
        using Error = unsigned (*)();
        check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
              "Production spell GL integration valid");
    }
}
} // namespace blobber::test
