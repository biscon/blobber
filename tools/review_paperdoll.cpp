#include "assets/rules_loader.hpp"
#include "game/commands.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "ui/interaction.hpp"
#include "ui/inventory_layout.hpp"
#include <iostream>

using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3)
        throw std::runtime_error("Usage: blobber_review_paperdoll ASSETS NEW_OUTPUT");
    const std::filesystem::path assets(argv[1]), output(argv[2]);
    check(std::filesystem::create_directory(output), "Use a new capture directory");
    const auto catalog = loadRules(assets / "rules/catalog.json");
    const Map map({"#########", "#.......#", "#.......#", "#.......#", "#########"});
    const Player spawn{{4, 2}, Heading::North};
    const NpcWorld empty(map, spawn.cell, {}, {});
    GameSession session(map, catalog, empty, 5, spawn, false);
    CommandController ui(session);
    ui.refresh({}, false, false, true);
    ui.openInventory(rules::CharacterId{0});
    SetTraceLogLevel(LOG_WARNING);
    Window window(1920, 1080, true, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Review window must stay hidden");
    GpuResources resources;
    {
        Presenter presenter(resources, assets);
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
            in.pointer = ArtPoint{float(from.x + 5), float(from.y + 5)};
            in.pointerHeld = true;
            in.pressed[unsigned(UiKey::Click)] = true;
            routeHudInput(ui, in);
            in.pressed.fill(false);
            in.pointer = ArtPoint{float(to.x + 5), float(to.y + 5)};
            routeHudInput(ui, in);
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
        check(IsWindowState(FLAG_WINDOW_HIDDEN), "Capture never exposes or controls a desktop window");
        check(resources.destroy(background), "Review-owned viewport target releases");
    }
    check(resources.counts().externalBytes == 0, "Font ownership releases accounted atlas memory");
    using GlError = unsigned (*)();
    const auto glError = reinterpret_cast<GlError>(rlGetProcAddress("glGetError"));
    check(glError && glError() == 0, "Paperdoll drawing and resource lifetimes have no OpenGL errors");
    std::cout
        << "Four production inventory captures; compact drag/drop, AC, first/repeated C++ allocations, "
           "prepared GPU residency and OpenGL lifecycle checks passed. Driver/C allocations are excluded.\n";
    return 0;
} catch (const std::exception& error) {
    watching = false;
    std::cerr << error.what() << '\n';
    return 1;
}
