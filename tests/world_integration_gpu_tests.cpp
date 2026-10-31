#include "assets/image.hpp"
#include "assets/rules_loader.hpp"
#include "app/options.hpp"
#include "ui/menu_screen.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/automation_fixtures.hpp"
#include "support/check.hpp"
#include "support/fixture_world.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <iostream>
using namespace blobber;
namespace {
unsigned compare(GpuResources& r, WorldRenderer& renderer, TargetHandle ids) {
    renderer.diagnosticIds(ids);
    OwnedImage image(LoadImageFromTexture(r.target(ids).texture));
    check(image.value.data, "Diagnostic ID readback");
    ImageFlipVertical(&image.value);
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* pixels = static_cast<const unsigned char*>(image.value.data);
    const auto& picking = renderer.visibility()->frame().picking;
    std::array<bool, PickingBuffer::fixtureBase + fixtureCapacity> cpu{}, gpu{};
    unsigned mismatch = 0, interior = 0;
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x) {
            const auto p = (y * viewWidth + x) * 4;
            const auto actual = std::uint16_t(pixels[p] + 256 * pixels[p + 1]), expected = picking.slot(x, y);
            if (actual < gpu.size())
                gpu[actual] = true;
            if (expected < cpu.size())
                cpu[expected] = true;
            if (actual == expected)
                continue;
            ++mismatch;
            bool edge = false;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    edge |= picking.slot(x + dx, y + dy) == actual;
            if (!edge)
                ++interior;
        }
    if (interior)
        std::cerr << "Interior mismatches " << interior << " / total " << mismatch << '\n';
    check(interior == 0, "CPU/GPU ID interiors agree within one-pixel geometric boundary");
    check(cpu == gpu, "CPU/GPU exact visible object membership");
    return mismatch;
}
} // namespace

int main(int argc, char** argv) {
    try {
        test::VisibilityFixture fixture(2);
        auto assets = std::make_shared<const world_assets::CpuAssets>(fixture.publication());
        const auto catalog = automation::test::catalog(false, 50, 7);
        const auto definitions = automation::test::definitions();
        Window window(1920, 1080, true);
        GpuResources resources;
        {
            WorldRenderer renderer(resources, assets);
            GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
            unsigned differences = 0;
            for (unsigned heading = 0; heading < 4; ++heading) {
                auto rotate = [&](Cell c) { return generation::rotate(c, 9, heading); };
                std::vector<std::string> rows(9, std::string(9, '#'));
                for (int y = 1; y < 8; ++y)
                    for (int x = 1; x < 8; ++x) {
                        const auto c = rotate({x, y});
                        rows[c.y][c.x] = (y == 4 && x != 4) ? '#' : '.';
                    }
                const Map map(rows);
                const Player spawn{rotate({4, 5}), Heading(heading)};
                NpcWorld placements(map, spawn.cell, definitions.types, {});
                FloorFixtures fixtures;
                fixtures.count = 1;
                fixtures.entries[0] = {rotate({4, 4}), Heading(heading), FixtureKind::Door, 0};
                GameSession game(map, catalog, placements, 5, spawn, false, fixture.timings, {}, fixtures);
                NpcAnimator animator(game.actors(), fixture.timings);
                SpellArea area;
                area.ground.revision = 1;
                area.ground.areas[0].active = true;
                area.ground.areas[0].cell = spawn.cell;
                area.ground.weapons[0] = {rotate({4, 3}), true};
                auto input = [&] {
                    animator.setTime(game.timeMs());
                    return world_render::FrameInput{game.geometry(),
                                                    game.movement(),
                                                    &game.actors(),
                                                    &animator.snapshot(),
                                                    &game.loot().snapshot(),
                                                    &game.doors(),
                                                    &game.combat().attacks(),
                                                    &area,
                                                    game.timeMs()};
                };
                renderer.stage(input());
                renderer.commit();
                differences += compare(resources, renderer, ids.get());
                AttackVisualSnapshot shots;
                shots.world = game.actors().snapshot().world;
                shots.revision = 1;
                auto& shot = shots.attacks[0];
                shot.id = {shots.world, 1};
                shot.stage = AttackStage::Flight;
                shot.ranged = shot.hit = true;
                shot.impactMs = 1000;
                shot.endMs = 1200;
                shot.partyOrigin = spawn;
                const auto forwardCell = forward(spawn.heading);
                shot.targetPosition = {spawn.cell.x + .5f + forwardCell.x * 3,
                                       spawn.cell.y + .5f + forwardCell.y * 3};
                auto effectDifference = [&] {
                    renderer.render(input());
                    OwnedImage before(
                        LoadImageFromTexture(resources.target(renderer.image().target).texture));
                    auto withShot = input();
                    withShot.attacks = &shots;
                    withShot.now = 500;
                    renderer.render(withShot);
                    OwnedImage after(LoadImageFromTexture(resources.target(renderer.image().target).texture));
                    ImageFormat(&before.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                    ImageFormat(&after.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                    check(before.value.data && after.value.data, "Effect color readback succeeds");
                    const auto* a = static_cast<const Color*>(before.value.data);
                    const auto* b = static_cast<const Color*>(after.value.data);
                    unsigned changed = 0;
                    for (unsigned pixel = 0; pixel < unsigned(viewWidth * viewHeight); ++pixel)
                        changed +=
                            a[pixel].r != b[pixel].r || a[pixel].g != b[pixel].g || a[pixel].b != b[pixel].b;
                    renderer.render(input());
                    return changed;
                };
                check(effectDifference() == 0, "Opaque closed slab clips airborne effect color");
                check(game.interact({game.fixtures().id(0), game.fixtures().revision()}).changed,
                      "GPU scenario opens door");
                renderer.render(input());
                auto before = resources.transfers();
                allocations = 0;
                watching = true;
                for (unsigned i = 0; i < 30; ++i) {
                    game.update(.01, false);
                    renderer.supersampling(i % 2);
                    renderer.render(input());
                    if (i == 7 || i == 14 || i == 22) {
                        watching = false;
                        differences += compare(resources, renderer, ids.get());
                        watching = true;
                    }
                }
                watching = false;
                check(allocations == 0 && resources.transfers().uploads == before.uploads,
                      "Door/effect/quality frames allocate and upload no resources");
                differences += compare(resources, renderer, ids.get());
                check(renderer.picking(), "Opening restores interaction at exact boundary");
                check(effectDifference() > 16, "Open doorway reveals airborne effect color");
                renderer.clear();
            }
            {
                const auto map = test::worldRoom(9, 9);
                const Player spawn{{4, 6}, Heading::North};
                NpcWorld npcs(map, spawn.cell, definitions.types, {{{0}, {4, 3}, {0, 0}, Heading::South}});
                GameSession game(map, catalog, npcs, 5, spawn, false, fixture.timings);
                NpcAnimator animator(game.actors(), fixture.timings);
                animator.setTime(0);
                AttackVisualSnapshot attacks;
                attacks.world = game.actors().snapshot().world;
                attacks.revision = 1;
                world_render::FrameInput input{
                    game.geometry(), game.movement(), &game.actors(), &animator.snapshot(),
                    nullptr,         nullptr,         &attacks};
                renderer.stage(input);
                renderer.commit();
                OwnedImage base(LoadImageFromTexture(resources.target(renderer.image().target).texture));
                ImageFlipVertical(&base.value);
                ImageFormat(&base.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                const auto visibilityRevision = renderer.visibility()->frame().revision;
                const auto* basePixels = static_cast<const Color*>(base.value.data);
                auto& pulse = attacks.spellPulses[0];
                pulse.id = {attacks.world, 1};
                pulse.enemy = game.actors().id(0);
                pulse.stage = AttackStage::Resolved;
                pulse.hit = true;
                pulse.spell = rules::SpellKind::BurningHands;
                pulse.partyOrigin = spawn;
                pulse.endMs = 120;
                ++attacks.revision;
                auto contrast = [&](std::uint64_t now) {
                    input.now = now;
                    renderer.render(input);
                    OwnedImage image(LoadImageFromTexture(resources.target(renderer.image().target).texture));
                    ImageFlipVertical(&image.value);
                    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                    const auto* pixels = static_cast<const Color*>(image.value.data);
                    unsigned sum = 0;
                    for (int y = 0; y < viewHeight; ++y)
                        for (int x = 0; x < viewWidth; ++x) {
                            if (renderer.visibility()->frame().picking.slot(x, y) != 0)
                                continue;
                            const auto at = y * viewWidth + x;
                            sum += std::abs(int(pixels[at].r) - basePixels[at].r) +
                                   std::abs(int(pixels[at].g) - basePixels[at].g) +
                                   std::abs(int(pixels[at].b) - basePixels[at].b);
                        }
                    return sum;
                };
                const auto peak = contrast(0), half = contrast(60), expired = contrast(120);
                check(peak > 1000, "Impact color is visible over its own depth-writing target");
                check(half > peak * .35 && half < peak * .65,
                      "GPU impact color fades during the existing tail");
                check(!expired && renderer.visibility()->frame().revision == visibilityRevision,
                      "Expiry restores color without changing visibility or IDs");
                renderer.clear();
            }
            auto campaign = std::make_unique<CampaignRun>(catalog, *catalog.preset(), definitions.types,
                                                          fixture.timings, 5);
            auto current = std::make_unique<GameView>(renderer);
            NpcAnimator animator(campaign->game().actors(), fixture.timings);
            animator.setTime(0);
            SpellArea area;
            current->update(campaign->game(), animator.snapshot(), area);
            current->activate();
            const auto prior = renderer.image();
            const auto* pick = renderer.picking();
            auto ticket = campaign->prepareTravel(campaign->request(campaign->game().nearbyFixture()));
            auto replacement = std::make_unique<GameView>(renderer);
            NpcAnimator destination(ticket.game().actors(), fixture.timings);
            destination.setTime(ticket.game().timeMs());
            resources.failAfter(0);
            rejects([&] { replacement->update(ticket.game(), destination.snapshot(), area); },
                    "Travel GPU preparation failure rejects");
            resources.failAfter(-1);
            check(renderer.image().revision == prior.revision && renderer.picking() == pick &&
                      campaign->activeLocation() == 0,
                  "Failed travel keeps prior image, picking and simulation");
            unsigned cpuFailures = 0;
            const auto retained = resources.counts();
            for (int point = 0; point < 32; ++point) {
                auto probe = std::make_unique<GameView>(renderer);
                allocationsUntilFailure = point;
                try {
                    probe->update(ticket.game(), destination.snapshot(), area);
                } catch (const std::bad_alloc&) {
                    ++cpuFailures;
                } catch (...) {
                    allocationsUntilFailure = -1;
                    throw;
                }
                allocationsUntilFailure = -1;
                probe.reset();
                const auto after = resources.counts();
                check(!renderer.staged() && renderer.image().revision == prior.revision &&
                          renderer.picking() == pick && after.meshes == retained.meshes &&
                          after.targets == retained.targets && after.textures == retained.textures,
                      "CPU preparation failure/cancel retains active image, picking and resource ownership");
            }
            check(cpuFailures >= 10,
                  "Integrated preparation injection reaches geometry, visibility and effects owners");
            std::cout << "Integrated preparation allocation failures=" << cpuFailures << " of 32 positions\n";
            std::cout << "Fixture active CPU bytes=" << renderer.cpuBytes()
                      << " GPU estimate=" << renderer.gpuBytes() << '\n';
            replacement->update(ticket.game(), destination.snapshot(), area);
            std::cout << "Fixture active+staged CPU bytes=" << renderer.cpuBytes()
                      << " GPU estimate=" << renderer.gpuBytes() << '\n';
            check(campaign->commitTravel(ticket), "GPU staged campaign travel commits");
            replacement->activate();
            current.swap(replacement);
            replacement.reset();
            check(renderer.picking() &&
                      renderer.picking()->targets().world == campaign->game().actors().snapshot().world,
                  "Old view destruction retains committed destination");
            current.reset();
            check(!renderer.visibility(), "View releases GPU borrowers before session destruction");
            check(rlGetProcAddress("glGetError"), "GL integration available");
            using Error = unsigned (*)();
            check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                  "No GL error after integrated passes");
            std::cout << "R04 hidden door/effect GPU, allocation, parity and travel rollback passed; edge "
                         "differences="
                      << differences << '\n';
        }
        const auto counts = resources.counts();
        check(!counts.textures && !counts.targets && !counts.meshes && !counts.shaders,
              "Integrated GPU resources released");
        // Optional review export uses production menu callbacks and real owned content.
        // Normal correctness mode above remains independent of shipped assets.
        if (argc == 3) {
            const std::filesystem::path source(argv[1]), output(argv[2]);
            std::filesystem::create_directories(output);
            const auto rules = loadRules(source / "rules/catalog.json");
            CharacterLibrary library(rules, fixture.location.directory / "characters.json");
            check(bool(library.initialize()), "Review owns its isolated character library");
            MenuView menu(rules, library);
            menu.usable = true;
            menu.model.filter(library.snapshot());
            GpuResources native;
            Presenter presenter(native, source);
            presenter.prepareMenu();
            presenter.presentMenu(drawMenu, &menu);
            presenter.capture(output / "main-menu.png");
            Options options;
            options.assets = source;
            options.seed = 5;
            ArenaScenarioStore store(rules, fixture.location.directory / "scenarios");
            openArena(menu, options, store);
            presenter.presentMenu(drawMenu, &menu);
            presenter.capture(output / "arena-setup.png");
            menu.model.screen = MenuScreen::Library;
            presenter.presentMenu(drawMenu, &menu);
            presenter.capture(output / "character-library.png");
        }
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
