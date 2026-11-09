#include "assets/blueprint_loader.hpp"
#include "assets/dialogue.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "assets/shops.hpp"
#include "game/campaign.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "render/dialogue_camera.hpp"
#include "ui/dialogue_preparation.hpp"
#include "ui/interaction.hpp"
#include "ui/inventory_layout.hpp"
#include "ui/trade_layout.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>

using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3 && argc != 4)
        throw std::runtime_error("Usage: blobber_review_trade ASSETS [PREPARED] NEW_OUTPUT");
    const std::filesystem::path root(argv[1]), output(argv[argc - 1]);
    const auto prepared = argc == 4 ? std::filesystem::path(argv[2]) : root / "prepared";
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new output directory");
    const auto roomBlueprints = blueprints::load(root / "blueprints/underkeep.json");
    auto catalog = loadRules(root / "rules/catalog.json");
    auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    auto dialogue = loadDialogue(root / "dialogue");
    auto shops = loadShops(root / "shops", catalog);
    validateShops(dialogue, shops);
    auto run = std::make_unique<CampaignRun>(roomBlueprints, catalog, *catalog.preset(), definitions.types,
                                             definitions.timings, 5);
    run->game().restorePose(sanctuaryCity().doorway("smithy").approach, false);
    {
        auto ticket = run->prepareTravel(run->doorwayRequest());
        if (!run->commitTravel(ticket))
            throw std::runtime_error("Cannot enter review smithy");
    }
    auto& game = run->game();
    game.restorePose({{6, 5}, Heading::North}, false);
    const auto grant = [&](const char* item, unsigned quantity, std::uint16_t who = 0) {
        if (!run->party().acquire({{who},
                                   catalog.findItem(item),
                                   std::uint16_t(quantity),
                                   run->party().inventory().revision()}))
            throw std::runtime_error("Cannot grant isolated review fixture");
    };
    const auto initialGold = run->party().gold();
    grant("gold", 1000);
    grant("dagger", 1);
    Window window(1920, 1080, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Review window must remain hidden");
    GpuResources resources;
    Presenter presenter(resources, root, nullptr, prepared);
    prepareDialogueUi(dialogue, presenter.fonts());
    WorldRenderer renderer(resources, world_assets::load(prepared / "world"));
    auto view = std::make_unique<GameView>(renderer);
    NpcAnimator animation(game.actors(), definitions.timings);
    game.discover();
    animation.setTime(game.timeMs(), &game.effects());
    view->update(game, animation.snapshot(), {});
    view->activate();
    CommandController controls(game, definitions.timings);
    controls.bindDialogue(&dialogue, &run->dialogue());
    controls.bindTrade(&shops, &run->merchants());
    controls.residentVisibility(view->picking()->residents());
    controls.refresh(view->picking()->targets(), false, false, true);
    if (!controls.startDialogue())
        throw std::runtime_error("Cannot start review conversation");
    controls.tickDialogue(.7, true);
    const auto original = world_render::camera(game.movement(), game.geometry());
    const auto target = world_render::dialogueCamera(*renderer.scene(), renderer.assets(), game.geometry(),
                                                     original, *game.actors().actor(game.actors().id(0)),
                                                     *controls.dialoguePlayer().character());
    const auto clock = game.gameTime().elapsedMs;
    std::ofstream report(output / "report.txt");
    const auto shot = [&](const char* file, bool maximumBalance = false) {
        const auto& player = controls.dialoguePlayer();
        animation.setTime(game.timeMs(), &game.effects(), controls.dialogueSpeaker(), player.clockMs(),
                          &controls.dialogueAnimation());
        view->update(game, animation.snapshot(), {}, target, false);
        controls.residentVisibility(view->picking()->residents());
        controls.refresh(view->picking()->targets(), false, false, true);
        auto hud = controls.snapshot();
        if (maximumBalance) {
            hud.trade.gold = UINT64_MAX;
            hud.inventory.gold = UINT64_MAX;
        }
        view->present(presenter, game, run->map(), false, true, &hud);
        presenter.capture(output / (std::string(file) + ".png"));
        const auto& t = hud.trade;
        report << file << " trade=" << t.open << " character=" << t.characterName
               << " tooltip=" << t.tooltipVisible << " inventory_tooltip=" << hud.inventory.tooltipVisible
               << " gold=" << (hud.inventory.open ? hud.inventory.gold : t.gold)
               << " maximum_balance_fixture=" << maximumBalance << " stock_revision=" << t.merchantRevision
               << " game_ms=" << game.gameTime().elapsedMs << '\n';
        if (game.gameTime().elapsedMs != clock || !(game.movement().player == Player{{6, 5}, Heading::North}))
            throw std::runtime_error("Review trading advanced time or moved party");
    };
    const auto key = [&](UiKey k) {
        UiInput in;
        in.pressed[unsigned(k)] = true;
        routeHudInput(controls, in);
    };
    const auto cell = [&](TradeSide side, unsigned index, bool click) {
        const auto r = tradeLayout::cell(side, index);
        UiInput in;
        in.pointer = ArtPoint{float(r.x + 8), float(r.y + 8)};
        in.pressed[unsigned(UiKey::Click)] = click;
        routeHudInput(controls, in);
    };
    key(UiKey::One); // Duncan's authored Trade topic.
    if (!controls.snapshot().trade.open)
        throw std::runtime_error("Trade topic did not open");
    cell(TradeSide::Merchant, 0, true);
    shot("trade-buy");
    shot("trade-large-purse", true);
    key(UiKey::Confirm);
    if (run->party().gold() != initialGold + 970)
        throw std::runtime_error("Review purchase failed");
    key(UiKey::Two);
    cell(TradeSide::Merchant, 0, true);
    cell(TradeSide::Merchant, 0, false);
    controls.tickDialogue(.35, true);
    shot("trade-weapon-comparison");
    key(UiKey::Three);
    cell(TradeSide::Merchant, 7, true);
    cell(TradeSide::Merchant, 7, false);
    controls.tickDialogue(.35, true);
    shot("trade-armor-comparison");
    key(UiKey::One);
    unsigned dagger = 48;
    for (unsigned i = 0; i < 48; ++i) {
        const auto* item = controls.snapshot().trade.player[i].definition;
        if (item && item->id == "dagger")
            dagger = i;
    }
    if (dagger == 48)
        throw std::runtime_error("Review dagger not found");
    cell(TradeSide::Player, dagger, true);
    cell(TradeSide::Player, dagger, false);
    controls.tickDialogue(.35, true);
    shot("trade-sell-comparison");
    key(UiKey::Confirm);
    if (run->party().gold() != initialGold + 972)
        throw std::runtime_error("Review sale failed");
    unsigned buyback = 48;
    for (unsigned i = 0; i < 48; ++i)
        if (controls.snapshot().trade.merchant[i].buyback &&
            controls.snapshot().trade.merchant[i].definition->id == "dagger")
            buyback = i;
    if (buyback == 48)
        throw std::runtime_error("Review dagger buyback not found");
    cell(TradeSide::Merchant, buyback, true);
    cell(TradeSide::Merchant, buyback, false);
    controls.tickDialogue(.35, true);
    shot("trade-buyback");
    if (controls.snapshot().trade.merchant[buyback].price != 3)
        throw std::runtime_error("Review buyback markup failed");
    UiInput drag;
    const auto source = tradeLayout::cell(TradeSide::Merchant, buyback);
    drag.pointerTracked = true;
    drag.pointerHeld = true;
    drag.pointer = ArtPoint{float(source.x + 8), float(source.y + 8)};
    drag.pressed[unsigned(UiKey::Click)] = true;
    routeHudInput(controls, drag);
    drag.pressed.fill(false);
    drag.pointer = ArtPoint{360, 242};
    routeHudInput(controls, drag);
    if (!controls.snapshot().trade.gesture.dragging)
        throw std::runtime_error("Review drag did not capture");
    shot("trade-drag");
    drag.pointerHeld = false;
    drag.pointerReleased = true;
    routeHudInput(controls, drag);
    if (run->party().gold() != initialGold + 969)
        throw std::runtime_error("Review drag buyback failed");
    UiInput close;
    close.pointerTracked = true;
    close.pointerHeld = true;
    close.pointer = ArtPoint{float(tradeLayout::close.x + 4), float(tradeLayout::close.y + 4)};
    close.pressed[unsigned(UiKey::Click)] = true;
    routeHudInput(controls, close);
    close.pressed.fill(false);
    close.pointerHeld = false;
    close.pointerReleased = true;
    routeHudInput(controls, close);
    if (controls.snapshot().trade.open || !controls.snapshot().dialogue.open)
        throw std::runtime_error("Return to conversation failed");
    report << "mouse_close_returned_to_dialogue=1 drag_buyback_gold_delta=969\n";
    controls.finishDialogue();
    controls.openInventory();
    shot("inventory-purse");
    shot("inventory-large-purse", true);
    grant("longsword_plus1", 1);
    grant("belt_of_vigor", 1);
    grant("chain_mail", 1, 3);
    const auto inventoryComparison = [&](unsigned who, const char* item, const char* file) {
        controls.inventoryCharacter({std::uint16_t(who)});
        controls.refresh(view->picking()->targets(), false, false, true);
        unsigned cell = bagCells;
        for (unsigned n = 0; n < bagCells; ++n) {
            const auto* definition = controls.snapshot().inventory.cells[n].definition;
            if (definition && definition->id == item)
                cell = n;
        }
        if (cell == bagCells)
            throw std::runtime_error("Inventory review item missing");
        const auto r = inventoryLayout::cell(cell);
        UiInput input;
        input.pointer = ArtPoint{float(r.x + 8), float(r.y + 8)};
        routeHudInput(controls, input);
        controls.tickInventory(.349, true);
        if (controls.snapshot().inventory.tooltipVisible)
            throw std::runtime_error("Inventory tooltip opened before 350 ms");
        controls.tickInventory(.001, true);
        if (!controls.snapshot().inventory.tooltipVisible)
            throw std::runtime_error("Inventory tooltip did not open at 350 ms");
        shot(file);
    };
    inventoryComparison(0, "longsword_plus1", "inventory-weapon-comparison");
    inventoryComparison(0, "belt_of_vigor", "inventory-wearable-comparison");
    inventoryComparison(3, "chain_mail", "inventory-restricted-comparison");
    auto lootHud = controls.snapshot();
    lootHud.inventory.open = false;
    lootHud.loot = {};
    auto& loot = lootHud.loot;
    loot.open = true;
    loot.name = "Alden";
    loot.count = 1;
    loot.hovered = 0;
    loot.canTake = loot.canTakeAny = true;
    loot.reason = "Click to add to the party purse.";
    loot.entries[0].definition = catalog.item(catalog.findItem("gold"));
    loot.entries[0].quantity = 43;
    view->present(presenter, game, run->map(), false, true, &lootHud);
    presenter.capture(output / "loot-purse.png");
    auto checkpoint = run->checkpoint();
    if (checkpoint->merchants.size() != 1 ||
        checkpoint->merchants[0].stock[catalog.findItem("longsword").index] != 1)
        throw std::runtime_error("Review merchant state not checkpointed");
    view.reset();
    const auto memory = renderer.memory();
    report << "released_textures=" << memory.residentTextures << " released_alpha=" << memory.residentAlpha
           << '\n';
    if (memory.residentTextures || memory.residentAlpha)
        throw std::runtime_error("Review retained world resources");
    std::cout << "Hidden merchant review passed\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
