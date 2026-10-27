#include "assets/image.hpp"
#include "assets/prepared_loot.hpp"
#include "render/frame_pipeline.hpp"
#include "support/check.hpp"
#include "support/npc_fixtures.hpp"
#include "support/recovery_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace blobber;
int main(int argc, char**) {
    try {
        const auto cat = recoveryCatalog();
        OwnedImage sheet(GenImageColor(96, 96, BLANK));
        for (int i = 0; i < 9; ++i)
            ImageDrawCircle(&sheet.value, 16 + 32 * (i % 3), 16 + 32 * (i / 3), 12,
                            {static_cast<unsigned char>(80 + 15 * i), 180, 90, 255});
        auto bank = prepareLoot(sheet.value);
        const auto file =
            std::filesystem::temp_directory_path() /
            ("blobber-loot-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) +
             ".bin");
        struct Cleanup {
            std::filesystem::path p;
            ~Cleanup() {
                std::error_code e;
                std::filesystem::remove(p, e);
            }
        } cleanup{file};
        savePreparedLoot(bank, file);
        auto loaded = loadPreparedLoot(file);
        check(loaded.pixels.size() == bank.pixels.size(), "Offline sprite bank round trip");
        {
            std::fstream out(file, std::ios::in | std::ios::out | std::ios::binary);
            out.seekp(200);
            out.put(char(123));
        }
        rejects([&] { loadPreparedLoot(file); }, "Corrupted sprite bank rejected");
        loaded.bind(cat);
        const SceneRenderer renderer(prepareScene(testMaterials()), PreparedNpcs{}, {}, std::move(loaded));
        const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
        LootSnapshot loot;
        loot.world = 42;
        for (int i = 0; i < 9; ++i)
            loot.stacks[i] = {{3, 2}, {12}, 1, true};
        loot.stacks[9] = {{3, 3}, {13}, 1, true};
        MovementState movement;
        movement.player = {{3, 3}, Heading::North};
        FramePipeline frames;
        LootPage page;
        check(frames.update(renderer, map, movement, nullptr, nullptr, nullptr, 0, nullptr, &loot, &page),
              "Initial floor draw");
        auto visible = [&](const PickingBuffer* pick) {
            std::array<bool, lootCapacity> seen{};
            for (int y = 0; y < viewHeight; ++y)
                for (int x = 0; x < viewWidth; ++x) {
                    const auto hit = pick->pickLoot(x, y);
                    if (hit)
                        seen[hit->slot] = true;
                }
            return seen;
        };
        auto seen = visible(frames.picking());
        for (int i = 0; i < 6; ++i)
            check(seen[i], "All six first-page items independently clickable");
        check(!seen[6] && seen[9], "Paged items hidden, party-cell item visible");
        check(!frames.update(renderer, map, movement, nullptr, nullptr, nullptr, 0, nullptr, &loot, &page),
              "Unchanged loot keeps cached frame");
        page = {{3, 2}, 1, 1};
        check(frames.update(renderer, map, movement, nullptr, nullptr, nullptr, 0, nullptr, &loot, &page),
              "Page invalidates cache");
        seen = visible(frames.picking());
        check(!seen[0] && seen[6] && seen[7] && seen[8], "Second page picks correct stable slots");
        loot.stacks[6].active = false;
        ++loot.revision;
        frames.update(renderer, map, movement, nullptr, nullptr, nullptr, 0, nullptr, &loot, &page);
        check(!visible(frames.picking())[6], "Collected sprite and picking disappear together");
        movement.transition = {movement.player, {{4, 3}, Heading::North}, Action::Right, .05f};
        ++movement.revision;
        frames.update(renderer, map, movement, nullptr, nullptr, nullptr, 0, nullptr, &loot, &page);
        check(!frames.picking(), "Transitions disable floor picking");
        movement.transition = {};
        for (auto heading : {Heading::North, Heading::East, Heading::South, Heading::West}) {
            movement.player.heading = heading;
            ++movement.revision;
            frames.update(renderer, map, movement, nullptr, nullptr, nullptr, 0, nullptr, &loot, &page);
            check(visible(frames.picking())[9], "Party-cell loot visible at every heading");
        }
        const Map blocked({"#######", "#.....#", "#..#..#", "#.....#", "#.....#", "#.....#", "#######"});
        loot.stacks[0] = {{3, 1}, {12}, 1, true};
        for (int i = 1; i < 9; ++i)
            loot.stacks[i].active = false;
        Frame frame;
        PickingBuffer picking;
        renderer.compose(blocked, {{3, 3}, Heading::North}, frame, nullptr, nullptr, &picking, nullptr,
                         nullptr, &loot);
        check(!visible(&picking)[0], "Wall occludes distant floor item");
        for (auto cell : {Cell{3, 3}, Cell{2, 2}, Cell{4, 2}}) {
            for (int i = 0; i < 6; ++i)
                loot.stacks[i] = {cell, {12}, 1, true};
            for (std::size_t i = 6; i < lootCapacity; ++i)
                loot.stacks[i].active = false;
            renderer.compose(cell == Cell{3, 3} ? blocked : map, {{3, 3}, Heading::North}, frame, nullptr,
                             nullptr, &picking, nullptr, nullptr, &loot);
            seen = visible(&picking);
            for (int i = 0; i < 6; ++i)
                check(seen[i], "Six party/diagonal items remain pickable against viewport/wall boundaries");
        }
        // Actual prepared body pixels occlude items behind them, including death poses.
        const auto appearance = testReactionAppearance();
        auto bodyBank = prepareLoot(sheet.value);
        bodyBank.bind(cat);
        SceneRenderer bodies(prepareScene(testMaterials()), PreparedNpcs({appearance}), {},
                             std::move(bodyBank));
        const NpcWorld places(map, {3, 3}, {"goblin"}, {{{0}, {3, 2}, {0, 1}, Heading::South}});
        ActorWorld actors(map, cat, places);
        NpcAnimator animator(actors, {npcTimings(appearance)});
        for (int i = 0; i < 6; ++i)
            loot.stacks[i] = {{3, 2}, {12}, 1, true};
        Frame bare;
        PickingBuffer barePick;
        renderer.compose(map, {{3, 3}, Heading::North}, bare, nullptr, nullptr, &barePick, nullptr, nullptr,
                         &loot);
        for (int death = 0; death < 2; ++death) {
            if (death)
                actors.damage(actors.id(0), 100, 0);
            animator.setTime(death ? 650 : 0);
            bodies.compose(map, {{3, 3}, Heading::North}, frame, &actors, &animator.snapshot(), &picking,
                           nullptr, nullptr, &loot);
            unsigned occluded = 0;
            for (int y = 0; y < viewHeight; ++y)
                for (int x = 0; x < viewWidth; ++x)
                    if (barePick.pickLoot(x, y) && picking.slot(x, y) == 0) {
                        ++occluded;
                        check(!picking.pickLoot(x, y), "Body pixels cannot select obscured loot");
                    }
            check(occluded > 0, "Living and corpse fixture overlap loot pixels");
        }
        if (argc > 1) {
            // Full 2,112-entry publication stress, bounded sort and no dropped visible-page records.
            for (std::size_t i = 0; i < lootCapacity; ++i)
                loot.stacks[i] = {{3, 3}, {12}, 1, true};
            renderer.compose(map, {{3, 3}, Heading::North}, frame, nullptr, nullptr, &picking, nullptr,
                             nullptr, &loot);
            seen = visible(&picking);
            for (int i = 0; i < 6; ++i)
                check(seen[i], "Capacity stress retains visible six");
        }
        std::cout << "Prepared floor loot checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
