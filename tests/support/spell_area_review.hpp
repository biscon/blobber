#pragma once
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "support/spell_parties.hpp"
#include <iostream>
namespace blobber::test {
inline void spellAreaReview(const std::filesystem::path& assets, const std::filesystem::path& output,
                            bool forwardOnly = false, bool thunderOnly = false) {
    if (std::filesystem::exists(output) && !std::filesystem::is_empty(output))
        throw std::runtime_error("Choose an empty spell area review directory");
    std::filesystem::create_directories(output);
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    const auto catalog = loadRules(assets / "rules/catalog.json");
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Spell area captures stay hidden");
    GpuResources resources;
    {
        Presenter presenter(resources, assets);
        const auto baseline = resources.counts();
        {
            WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
            const std::array<rules::SpellKind, 4> groundKinds{rules::SpellKind::Shatter,
                                                              rules::SpellKind::Grease,
                                                              rules::SpellKind::Sleep, rules::SpellKind::Web};
            const std::array<rules::SpellKind, 3> forwardKinds{
                rules::SpellKind::BurningHands, rules::SpellKind::Thunderwave, rules::SpellKind::ColorSpray};
            for (unsigned example = 0; example < (forwardOnly ? forwardKinds.size() : groundKinds.size());
                 ++example) {
                const auto kind = forwardOnly ? forwardKinds[example] : groundKinds[example];
                if (thunderOnly && kind != rules::SpellKind::Thunderwave)
                    continue;
                WorldDefinition definition;
                definition.assetPack = "underkeep";
                std::vector<std::string> rows(13, "#...........#");
                rows.front() = rows.back() = "#############";
                const Map map(rows, definition);
                const Player pose{{6, 8}, Heading::North};
                const auto type = NpcTypeId{
                    std::uint16_t(std::find(definitions.types.begin(), definitions.types.end(), "goblin") -
                                  definitions.types.begin())};
                const std::vector<NpcInstance> groundPlaces{{type, {5, 5}, {1, 1}, Heading::South},
                                                            {type, {6, 5}, {0, 1}, Heading::South},
                                                            {type, {5, 6}, {1, 0}, Heading::South},
                                                            {type, {6, 6}, {0, 0}, Heading::South}};
                const std::vector<NpcInstance> forwardPlaces{
                    {type, {6, 7}, {0, 1}, Heading::South}, {type, {6, 7}, {1, 1}, Heading::South},
                    {type, {6, 7}, {0, 0}, Heading::South}, {type, {6, 7}, {1, 0}, Heading::South},
                    {type, {5, 7}, {1, 0}, Heading::South}, {type, {7, 7}, {0, 0}, Heading::South}};
                NpcWorld places(map, pose.cell, definitions.types,
                                forwardOnly ? forwardPlaces : groundPlaces);
                GameSession game(map, catalog, leveledSpellParty(catalog, kind), places, 5, pose, false,
                                 definitions.timings);
                prepareSpellParty(game.party(), kind);
                if (forwardOnly)
                    spellReady(game, 3);
                CommandController commands(game, definitions.timings);
                NpcAnimator animator(game.actors(), definitions.timings);
                GameView view(renderer);
                auto draw = [&] {
                    animator.setTime(game.timeMs(), &game.effects());
                    view.update(game, animator.snapshot(), commands.floorPreview());
                    commands.refresh(view.picking()->targets(), false, false, true);
                    view.present(presenter, game, map, false, true, &commands.snapshot());
                    using Error = unsigned (*)();
                    check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                          "Precise footprint shader has no GL errors");
                };
                draw();
                commands.selectCharacter(3);
                commands.openSpellbook();
                std::size_t index = 0;
                while (index < commands.snapshot().magic.count &&
                       commands.snapshot().magic.entries[index].kind != kind)
                    ++index;
                check(index < commands.snapshot().magic.count, "Review spell is available");
                commands.chooseSpellbook(index);
                commands.magicAim(SpellAim{12, 12}, true);
                draw();
                const auto area = commands.floorPreview();
                const unsigned expected = forwardOnly ? (kind == rules::SpellKind::Thunderwave ? 8 : 6)
                                          : kind == rules::SpellKind::Shatter ? 12
                                          : kind == rules::SpellKind::Grease  ? 4
                                          : kind == rules::SpellKind::Sleep   ? 36
                                                                              : 16;
                check(area.footprint.count() == expected && area.count == (forwardOnly ? 6u : 4u),
                      "Production preview and actual occupied squares agree");
                const std::string name = rules::spellRules(kind)->id;
                presenter.capture(output / (name + "-preview.png"));
                allocations = 0;
                watching = true;
                draw();
                watching = false;
                check(!allocations, "Prepared area rendering allocates no engine C++ storage");
                if (forwardOnly || kind == rules::SpellKind::Shatter || kind == rules::SpellKind::Grease) {
                    check(commands.confirm() == CommandReason::Resolved, "Reviewed ground cast resolves");
                    commands.record(game.update(.16, false));
                    draw();
                    presenter.capture(output / (name + "-result.png"));
                }
            }
        }
        const auto released = resources.counts();
        check(released.textures == baseline.textures && released.targets == baseline.targets &&
                  released.meshes == baseline.meshes && released.shaders == baseline.shaders,
              "Spell area review releases GPU resources");
    }
    std::cout << "Hidden precise spell area rendering, allocation and GPU lifecycle checks passed\n";
}
} // namespace blobber::test
