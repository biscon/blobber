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
#include "ui/trade_layout.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>

using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3)
        throw std::runtime_error("Usage: blobber_review_trade ASSETS NEW_OUTPUT");
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new output directory");
    auto catalog = loadRules(root / "rules/catalog.json");
    auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    auto dialogue = loadDialogue(root / "dialogue");
    auto shops = loadShops(root / "shops", catalog);
    validateShops(dialogue, shops);
    auto run =
        std::make_unique<CampaignRun>(catalog, *catalog.preset(), definitions.types, definitions.timings, 5);
    run->game().restorePose(sanctuaryCity().doorway("inn").approach, false);
    {
        auto ticket = run->prepareTravel(run->doorwayRequest());
        if (!run->commitTravel(ticket))
            throw std::runtime_error("Cannot enter review inn");
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
    grant("gold", 1000);
    grant("gold", 200, 1);
    grant("gold", 200, 2);
    grant("dagger", 1);
    Window window(1920, 1080, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Review window must remain hidden");
    GpuResources resources;
    Presenter presenter(resources, root);
    prepareDialogueUi(dialogue, presenter.fonts());
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
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
    const auto shot = [&](const char* file) {
        const auto& player = controls.dialoguePlayer();
        animation.setTime(game.timeMs(), &game.effects(), controls.dialogueSpeaker(), player.clockMs(),
                          &controls.dialogueAnimation());
        view->update(game, animation.snapshot(), {}, target, false);
        controls.residentVisibility(view->picking()->residents());
        controls.refresh(view->picking()->targets(), false, false, true);
        view->present(presenter, game, run->map(), false, true, &controls.snapshot());
        presenter.capture(output / (std::string(file) + ".png"));
        const auto& t = controls.snapshot().trade;
        report << file << " trade=" << t.open << " character=" << t.characterName
               << " tooltip=" << t.tooltipVisible << " gold=" << t.gold
               << " stock_revision=" << t.merchantRevision << " game_ms=" << game.gameTime().elapsedMs
               << '\n';
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
    key(UiKey::One); // Innkeeper's authored Trade topic.
    if (!controls.snapshot().trade.open)
        throw std::runtime_error("Trade topic did not open");
    cell(TradeSide::Merchant, 0, true);
    shot("trade-buy");
    key(UiKey::Confirm);
    if (characterGold(run->party(), {0}) != 880)
        throw std::runtime_error("Review purchase failed");
    key(UiKey::Two);
    cell(TradeSide::Merchant, 3, true);
    cell(TradeSide::Merchant, 3, false);
    controls.tickDialogue(.35, true);
    shot("trade-weapon-comparison");
    key(UiKey::Three);
    cell(TradeSide::Merchant, 4, true);
    cell(TradeSide::Merchant, 4, false);
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
    if (characterGold(run->party(), {0}) != 882)
        throw std::runtime_error("Review sale failed");
    cell(TradeSide::Merchant, 5, true);
    shot("trade-buyback");
    if (!controls.snapshot().trade.merchant[5].buyback || controls.snapshot().trade.merchant[5].price != 3)
        throw std::runtime_error("Review buyback markup failed");
    key(UiKey::Cancel);
    shot("trade-return");
    if (controls.snapshot().trade.open || !controls.snapshot().dialogue.open)
        throw std::runtime_error("Return to conversation failed");
    controls.finishDialogue();
    auto checkpoint = run->checkpoint();
    if (checkpoint->merchants.size() != 1 ||
        checkpoint->merchants[0].stock[catalog.findItem("camp_kit").index] != 3)
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
