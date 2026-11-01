#include "game/commands.hpp"
#include "input/buttons.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "ui/button_targets.hpp"
#include "ui/interaction.hpp"
#include <iostream>
using namespace blobber;
namespace {
struct Pointer {
    UiInputEdges edges;
    UiInput sample(bool down, ArtPoint point = {20, 20}, bool focused = true) {
        std::array<bool, uiKeyCount> keys{};
        keys[std::size_t(UiKey::Click)] = down;
        return edges.sample(keys, focused, point);
    }
};
void transitions() {
    ButtonTargets targets;
    targets.add({10, 10, 40, 20});
    ButtonPress button;
    Pointer pointer;
    check(!button.route(pointer.sample(true), targets).has(UiKey::Click) && button.feedback,
          "down arms and paints without activating");
    for (unsigned i = 0; i < 5; ++i)
        check(!button.route(pointer.sample(true), targets).has(UiKey::Click), "held never repeats");
    check(button.route(pointer.sample(false), targets).has(UiKey::Click) && !button.feedback,
          "release inside activates exactly once");
    check(!button.route(pointer.sample(false), targets).has(UiKey::Click), "release does not repeat");
    button.route(pointer.sample(true), targets);
    button.route(pointer.sample(true, {80, 20}), targets);
    button.route(pointer.sample(true), targets);
    check(!button.feedback && !button.route(pointer.sample(false), targets).has(UiKey::Click),
          "leaving then reentering cancels permanently");
    button.route(pointer.sample(true), targets);
    targets.entries[0].enabled = false;
    check(!button.route(pointer.sample(false), targets).has(UiKey::Click), "disabling cancels");
    targets.entries[0].enabled = true;
    button.route(pointer.sample(true), targets);
    ++targets.context[0];
    check(!button.route(pointer.sample(false), targets).has(UiKey::Click), "context change cancels");
    button.route(pointer.sample(true), targets);
    button.route(pointer.sample(true, {20, 20}, false), targets);
    button.route(pointer.sample(true), targets);
    check(!button.route(pointer.sample(false), targets).has(UiKey::Click),
          "blur and refocus cannot activate");
    button.route(pointer.sample(false), targets);
    targets.entries[0].raw = true;
    check(button.route(pointer.sample(true), targets).has(UiKey::Click) && !button.feedback,
          "drag/text targets receive raw down");
    check(button.route(pointer.sample(false), targets).pointerReleased, "raw release survives routing");
    targets.overflow = true;
    check(!button.route(pointer.sample(true), targets).has(UiKey::Click), "overflow fails closed");
    button.route(pointer.sample(false), targets);
    ButtonTargets overlap;
    overlap.add({20, 10, 10, 20});
    overlap.add({0, 0, 100, 100});
    button.route(pointer.sample(true, {15, 20}), overlap);
    button.route(pointer.sample(true, {25, 20}), overlap);
    check(!button.route(pointer.sample(false, {25, 20}), overlap).has(UiKey::Click),
          "release over a child control cannot activate a captured background");
}
void hudAndFlow() {
    const auto map = hybridRoom();
    const auto catalog = creationCatalog(true);
    const auto places = spellPlacements(map, 0);
    GameSession game(map, catalog, places, 5, spellSpawn, false);
    CommandController commands(game);
    commands.refresh({}, false, false, true);
    Pointer pointer;
    const ArtPoint gear{625, 8};
    routeHudInput(commands, pointer.sample(true, gear));
    check(!commands.snapshot().menuOpen && commands.snapshot().buttons.feedback,
          "HUD gear does not open pause until release");
    routeHudInput(commands, pointer.sample(false, gear));
    check(commands.snapshot().menuOpen, "HUD release opens pause");
    commands.closeOverlay();
    routeHudInput(commands, pointer.sample(true, gear), {}, false);
    routeHudInput(commands, pointer.sample(false, gear));
    check(!commands.snapshot().menuOpen, "unpresented-turn press cannot arm a later activation");
    commands.openInventory();
    commands.inventorySelect(0);
    commands.refresh({}, false, false, true);
    const auto targets = hudButtonTargets(commands.snapshot());
    const ButtonTarget* transfer = nullptr;
    for (unsigned i = 0; i < targets.count; ++i)
        if (targets.entries[i].id == 0x10000u + unsigned(InventoryAction::Transfer))
            transfer = &targets.entries[i];
    check(transfer && transfer->enabled, "owned inventory has a transfer command");
    const ArtPoint transferPoint{float(transfer->rect.x + 2), float(transfer->rect.y + 2)};
    routeHudInput(commands, pointer.sample(true, transferPoint));
    const auto revision = game.party().inventory().revision();
    commands.inventorySelect(48);
    routeHudInput(commands, pointer.sample(false, transferPoint));
    check(game.party().inventory().revision() == revision &&
              commands.snapshot().inventory.mode == InventoryMode::Browse,
          "changing the selected item cancels a press instead of activating a replacement command");
    commands.closeOverlay();
    FlowControls flow;
    flow.add({10, 10, 40, 20}, FlowAction::Next);
    check(flow.input(pointer.sample(true)).action == FlowAction::None && flow.buttons.feedback,
          "shared creator/advancement controls expose pressed state");
    check(flow.input(pointer.sample(false)).action == FlowAction::Next, "flow activates on release");
    flow.add({60, 10, 40, 20}, FlowAction::Save, 0, false);
    check(flow.input(pointer.sample(true, {70, 20})).action == FlowAction::None && !flow.buttons.feedback,
          "disabled flow control never arms");
    flow.input(pointer.sample(false, {70, 20}));
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 20; ++i) {
        routeHudInput(commands, pointer.sample(true, gear));
        routeHudInput(commands, pointer.sample(false, {600, 8}));
    }
    watching = false;
    check(!allocations, "physical button routing allocates no engine C++ memory");
}
} // namespace
int main() {
    try {
        transitions();
        hudAndFlow();
        std::cout << "Button press/release and integration checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
