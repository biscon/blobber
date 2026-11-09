#include "assets/rules_loader.hpp"
#include "game/commands.hpp"
#include "generation/floor.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "ui/interaction.hpp"
#include "ui/inventory_layout.hpp"
#include "ui/map_model.hpp"
#include <iostream>

using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3 && argc != 4)
        throw std::runtime_error("Usage: blobber_review_paperdoll ASSETS [PREPARED] NEW_OUTPUT");
    const std::filesystem::path assets(argv[1]), output(argv[argc - 1]);
    const auto preparedAssets = argc == 4 ? std::filesystem::path(argv[2]) : assets / "prepared";
    check(std::filesystem::create_directory(output), "Use a new capture directory");
    const auto catalog = loadRules(assets / "rules/catalog.json");
    const Map map({"#########", "#.......#", "#.......#", "#.......#", "#########"});
    const Player spawn{{4, 2}, Heading::North};
    const NpcWorld empty(map, spawn.cell, {}, {});
    FloorFixtures fixtures;
    fixtures.discovery = true;
    fixtures.entries[fixtures.count++] = {{3, 2}, Heading::West, FixtureKind::Upstairs};
    GameSession session(map, catalog, empty, 5, spawn, false, {}, {}, fixtures);
    CommandController ui(session);
    ui.refresh({}, false, false, true);
    ui.openInventory(rules::CharacterId{0});
    SetTraceLogLevel(LOG_WARNING);
    Window window(1920, 1080, true, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Review window must stay hidden");
    GpuResources resources;
    {
        Presenter presenter(resources, assets, nullptr, preparedAssets);
        for (unsigned n = 0; n < equipmentSlots; ++n) {
            const auto label = inventoryLayout::equipmentLabel(int(n));
            check(presenter.fonts().width(inventoryLayout::equipmentLabels[n], UiTextStyle::Small) <=
                      label.width,
                  "Every caption fits its attached label band at the native atlas size");
        }
        const auto background = resources.createTarget(viewWidth, viewHeight);
        BeginTextureMode(resources.target(background));
        ClearBackground(BLACK);
        EndTextureMode();
        const auto prepared = resources.counts();
        const auto uploads = resources.transfers().uploads;
        const auto draw = [&] {
            presenter.presentWorld(background, 1, session.movement(), map, &ui.snapshot());
        };
        allocations = 0;
        watching = true;
        draw();
        for (unsigned n = 0; n < 8; ++n) {
            ui.refresh({}, false, false, true);
            draw();
        }
        watching = false;
        check(allocations == 0, "First/repeated inventory frames allocate no engine-owned C++ storage");
        check(resources.counts().bytes == prepared.bytes && resources.transfers().uploads == uploads,
              "Inventory frames preserve prepared GPU residency without uploads");
        presenter.capture(output / "inventory-native.png");
        presenter.capture(output / "inventory-1280.png", 1280, 720);
        const int ac = ui.snapshot().inventory.armorClass;
        const auto shield = session.party().inventory().at({{0}, unsigned(EquipmentSlot::OffHand)});
        const auto drag = [&](UiRect from, UiRect to) {
            UiInput in;
            in.pointerTracked = true;
            in.pointer = ArtPoint{float(from.x + 5), float(from.y + 5)};
            in.pointerHeld = true;
            in.pressed[unsigned(UiKey::Click)] = true;
            routeHudInput(ui, in);
            in.pressed.fill(false);
            in.pointerTracked = true;
            in.pointer = ArtPoint{float(to.x + 5), float(to.y + 5)};
            routeHudInput(ui, in);
            in.pointerTracked = true;
            in.pointerHeld = false;
            in.pointerReleased = true;
            routeHudInput(ui, in);
        };
        drag(inventoryLayout::equipment(2), inventoryLayout::cell(47));
        check(session.party().inventory().at({{0}, 47}) == shield &&
                  ui.snapshot().inventory.armorClass == ac - 2,
              "Compact shield drag uses production hit tests and recomputes AC");
        drag(inventoryLayout::cell(47), inventoryLayout::equipment(2));
        check(session.party().inventory().at({{0}, unsigned(EquipmentSlot::OffHand)}) == shield &&
                  ui.snapshot().inventory.armorClass == ac,
              "Compact equipment drop restores the shield and AC");
        ui.inventorySelect(unsigned(EquipmentSlot::Wrists));
        draw();
        presenter.capture(output / "inventory-wrists.png");
        ui.inventoryCharacter({2});
        draw();
        presenter.capture(output / "inventory-ranged.png");
        ui.inventoryCharacter({0});
        const auto beltId = catalog.findItem("belt_of_vigor");
        check(bool(session.party().acquire({{0}, beltId, 1, session.party().inventory().revision()})),
              "Grant isolated review belt");
        std::uint16_t bag = 0;
        for (; bag < bagCells; ++bag) {
            const auto* item = session.party().inventory().stack(session.party().inventory().at({{0}, bag}));
            if (item && item->item.index == beltId.index)
                break;
        }
        check(bag < bagCells, "Review belt is in its owner's bag");
        ui.refresh({}, false, false, true);
        const auto beltCard = inventoryLayout::equipmentFrame(int(unsigned(EquipmentSlot::Belt) - bagCells));
        const auto hp = session.party().character({0})->stats.maxHitPoints;
        drag(inventoryLayout::cell(bag), beltCard);
        check(session.party().character({0})->stats.maxHitPoints == hp + 3,
              "Production pointer routing equips the belt and updates HP");
        draw();
        presenter.capture(output / "belt-equipped.png");
        drag(beltCard, inventoryLayout::cell(bag));
        check(session.party().character({0})->stats.maxHitPoints == hp,
              "Production pointer routing unequips the belt and updates HP");
        draw();
        presenter.capture(output / "belt-removed.png");
        ui.closeOverlay();
        session.party().grantNextLevelExperience();
        ui.refresh({}, false, false, true);
        check(ui.snapshot().cards[0].levelEarned && ui.snapshot().cards[0].levelEligible,
              "Safe idle portrait exposes an earned level");
        draw();
        presenter.capture(output / "level-idle.png");
        session.move(Action::TurnRight);
        ui.refresh({}, true, false, true);
        check(session.movement().transition.active() && ui.snapshot().cards[0].levelEarned &&
                  !ui.snapshot().cards[0].levelEligible,
              "Moving portrait keeps earned reminder despite disabled advancement");
        draw();
        presenter.capture(output / "level-moving.png");
        session.update(.2, false);
        generation::FloorLayout layout;
        layout.width = map.width();
        layout.height = map.height();
        layout.roomCount = 1;
        for (int y = 0; y < map.height(); ++y)
            for (int x = 0; x < map.width(); ++x)
                if (!map.solid({x, y}))
                    layout.membership[unsigned(y * map.width() + x)] = 0;
        MapOverlayModel model;
        model.prepare(map, &layout);
        model.refresh(&session.fixtures());
        check(!session.fixtures().known(0) && model.fixtureMarker(session.fixtures(), 0) == MapMarker::Upstairs,
              "Room map shows upstairs the review party has never faced");
        ui.refresh({}, false, true, true);
        allocations = 0;
        watching = true;
        presenter.presentWorld(background, 1, session.movement(), map, &ui.snapshot(), nullptr, true,
                               nullptr, false, &model);
        watching = false;
        check(allocations == 0, "Room fixture map drawing allocates no engine-owned C++ storage");
        presenter.capture(output / "map-unseen-stairs.png");
        check(IsWindowState(FLAG_WINDOW_HIDDEN), "Capture never exposes or controls a desktop window");
        check(resources.destroy(background), "Review-owned viewport target releases");
    }
    check(resources.counts().externalBytes == 0, "Font ownership releases accounted atlas memory");
    using GlError = unsigned (*)();
    const auto glError = reinterpret_cast<GlError>(rlGetProcAddress("glGetError"));
    check(glError && glError() == 0, "Paperdoll drawing and resource lifetimes have no OpenGL errors");
    std::cout
        << "Production inventory/map/level captures; physical drag/drop, HP/AC, first/repeated C++ allocations, "
           "prepared GPU residency and OpenGL lifecycle checks passed. Driver/C allocations are excluded.\n";
    return 0;
} catch (const std::exception& error) {
    watching = false;
    std::cerr << error.what() << '\n';
    return 1;
}
