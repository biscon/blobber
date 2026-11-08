#pragma once
#include "assets/dialogue.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "assets/shops.hpp"
#include "game/campaign.hpp"
#include "game/emporium_interior.hpp"
#include "persistence/campaign_codec.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "render/dialogue_camera.hpp"
#include "ui/dialogue_preparation.hpp"
#include "ui/interaction.hpp"
#include "ui/trade_layout.hpp"
#include "world/sanctuary_city.hpp"
#include <algorithm>
#include <fstream>

// Hidden production integration review. Inputs belong to the application only.
inline void reviewShadowtart(const std::filesystem::path& root, const std::filesystem::path& output) {
    using namespace blobber;
    const auto require = [](bool valid, const char* message) {
        if (!valid)
            throw std::runtime_error(message);
    };
    const auto catalog = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    auto dialogue = loadDialogue(root / "dialogue");
    const auto shops = loadShops(root / "shops", catalog);
    validateShops(dialogue, shops);
    constexpr auto host = dialogueKey("sanctuary.shadowtart");
    const auto* character = dialogue.find(host);
    require(character && character->name == "Shadowtart" && character->topics.size() == 5,
            "Shadowtart requires three conversation topics, Trade and Farewell");
    const auto* shop = shops.find(character->shop);
    require(shop && shop->key == "sanctuary_emporium", "Shadowtart shop binding missing");
    require(shop->stock.size() == 2, "Emporium stocks exactly the two current potion types");
    for (const auto* id : {"healing_potion", "mana_potion"}) {
        const auto item = catalog.findItem(id);
        const auto stocked = std::find_if(shop->stock.begin(), shop->stock.end(),
                                          [item](const ShopStock& s) { return s.item.index == item.index; });
        require(stocked != shop->stock.end() && stocked->quantity == 6, "Missing initial potion stock");
    }
    const auto* inn = shops.find(dialogueKey("sanctuary_inn"));
    require(inn && inn->stock.size() == 1 && inn->stock[0].item.index == catalog.findItem("camp_kit").index,
            "Inn should retain provisions after potion stock moves");
    auto run =
        std::make_unique<CampaignRun>(catalog, *catalog.preset(), definitions.types, definitions.timings, 5);
    run->game().restorePose(sanctuaryCity().doorway("emporium").approach, false);
    {
        auto ticket = run->prepareTravel(run->doorwayRequest());
        require(run->commitTravel(ticket), "Cannot enter emporium");
    }
    const auto grant = [&](const char* item, unsigned quantity) {
        require(
            bool(run->party().acquire(
                {{0}, catalog.findItem(item), std::uint16_t(quantity), run->party().inventory().revision()})),
            "Cannot create isolated trade fixture");
    };
    grant("gold", 1000);
    grant("mana_potion", 1);
    const auto initialGold = run->party().gold();
    Window window(1920, 1080, true);
    require(IsWindowState(FLAG_WINDOW_HIDDEN), "Review window must stay hidden");
    GpuResources resources;
    Presenter presenter(resources, root);
    prepareDialogueUi(dialogue, presenter.fonts());
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    auto view = std::make_unique<GameView>(renderer);
    std::ofstream report(output / "shadowtart.txt");
    {
        auto& game = run->game();
        require(game.actors().snapshot().count == 1 &&
                    game.actors().snapshot().actors[0].npc.residentId == host,
                "Emporium requires exactly one Shadowtart");
        NpcAnimator animation(game.actors(), definitions.timings);
        CommandController commands(game, definitions.timings);
        commands.bindDialogue(&dialogue, &run->dialogue());
        commands.bindTrade(&shops, &run->merchants());
        const auto refresh = [&](std::optional<world_render::Camera> camera = {}) {
            game.discover();
            animation.setTime(game.timeMs(), &game.effects(), commands.dialogueSpeaker(),
                              commands.dialoguePlayer().clockMs(), &commands.dialogueAnimation());
            view->update(game, animation.snapshot(), {}, camera);
            view->activate();
            commands.residentVisibility(view->picking()->residents());
            commands.refresh(view->picking()->targets(), false, false, true);
            require(!commands.snapshot().targets.count, "Shadowtart entered offensive picking");
        };
        const auto shot = [&](const char* name, std::optional<world_render::Camera> camera = {}) {
            refresh(camera);
            view->present(presenter, game, run->map(), false, true, &commands.snapshot());
            presenter.capture(output / (std::string(name) + ".png"));
            const auto m = renderer.memory();
            report << name << " clip=" << unsigned(animation.snapshot().frames[0].clip)
                   << " frame=" << unsigned(animation.snapshot().frames[0].active)
                   << " world_gpu=" << m.worldGpu << " world_cpu=" << renderer.cpuBytes()
                   << " game_ms=" << game.timeMs() << '\n';
        };
        const auto key = [&](UiKey k) {
            UiInput input;
            input.pressed[unsigned(k)] = true;
            routeHudInput(commands, input);
        };
        game.restorePose(emporiumCustomer, false);
        shot("front");
        require(std::string_view(commands.snapshot().fixturePrompt.data()).find("Shadowtart") !=
                    std::string_view::npos,
                "Shadowtart's interaction prompt is missing");
        game.restorePose({{7, 3}, Heading::West}, false);
        shot("left");
        game.restorePose({{5, 3}, Heading::East}, false);
        shot("right");
        game.restorePose(emporiumCustomer, false);
        refresh();
        require(commands.startDialogue(), "Shadowtart conversation failed");
        const auto original = world_render::camera(game.movement(), game.geometry());
        const auto target =
            world_render::dialogueCamera(*renderer.scene(), renderer.assets(), game.geometry(), original,
                                         *game.actors().actor(game.actors().id(0)), *character);
        commands.tickDialogue(.7, true);
        const auto time = game.timeMs();
        bool talkingFrames[4]{};
        for (unsigned tick = 0; tick < 8; ++tick) {
            commands.tickDialogue(.1, true);
            game.update(.1, false);
            refresh(target);
            const auto frame = animation.snapshot().frames[0];
            require(frame.clip == NpcAnimation::Talk && frame.active < 4 && game.timeMs() == time,
                    "Speech animation or frozen simulation failed");
            talkingFrames[frame.active] = true;
            if (tick == 1)
                shot("talk", target);
        }
        for (bool seen : talkingFrames)
            require(seen, "Talk loop did not present all four frames");
        // Check all three authored topics and completion/Talk-to-Idle behavior.
        for (auto topic : {UiKey::Two, UiKey::Three, UiKey::Four}) {
            key(topic);
            for (unsigned i = 0; i < 24; ++i)
                key(UiKey::DialogueAdvance);
            refresh(target);
            require(!commands.snapshot().dialogue.revealing &&
                        animation.snapshot().frames[0].clip == NpcAnimation::Idle,
                    "Completed response did not return to Idle");
        }
        require(run->dialogue().get(host)->completedCount == 3, "Three topic histories missing");
        shot("dialogue", target);
        key(UiKey::One);
        require(commands.snapshot().trade.open &&
                    std::string_view(commands.snapshot().trade.merchantName) == "Shadowtart",
                "Trade topic did not open Shadowtart's shop");
        const auto cell = [&](TradeSide side, unsigned index) {
            const auto r = tradeLayout::cell(side, index);
            UiInput input;
            input.pointer = ArtPoint{float(r.x + 8), float(r.y + 8)};
            input.pressed[unsigned(UiKey::Click)] = true;
            routeHudInput(commands, input);
        };
        require(commands.snapshot().trade.merchant[0].definition->id == "healing_potion",
                "Review healing potion offer missing");
        cell(TradeSide::Merchant, 0);
        key(UiKey::Confirm);
        require(run->party().gold() == initialGold - 50 &&
                    run->merchants().find(host)->stock[catalog.findItem("healing_potion").index] == 5,
                "Shadowtart purchase did not conserve finite stock/payment");
        cell(TradeSide::Merchant, 1);
        key(UiKey::Confirm);
        require(run->party().gold() == initialGold - 115 &&
                    run->merchants().find(host)->stock[catalog.findItem("mana_potion").index] == 5,
                "Mana potion purchase did not conserve finite stock/payment");
        unsigned potion = bagCells;
        for (unsigned i = 0; i < bagCells; ++i) {
            const auto* item = commands.snapshot().trade.player[i].definition;
            if (item && item->id == "mana_potion")
                potion = i;
        }
        require(potion < bagCells, "Review mana potion fixture missing");
        cell(TradeSide::Player, potion);
        key(UiKey::Confirm);
        require(run->party().gold() == initialGold - 99 &&
                    run->merchants().find(host)->buyback[0].price == 20,
                "Shadowtart sale/buyback failed");
        cell(TradeSide::Merchant, 2);
        key(UiKey::Confirm);
        require(run->party().gold() == initialGold - 119 && !run->merchants().find(host)->buyback[0].quantity,
                "Potion buyback did not restore the sold item and consume its offer");
        unsigned healing = bagCells;
        for (unsigned i = 0; i < bagCells; ++i) {
            const auto* item = commands.snapshot().trade.player[i].definition;
            if (item && item->id == "healing_potion")
                healing = i;
        }
        require(healing < bagCells, "Purchased healing potion missing");
        cell(TradeSide::Player, healing);
        key(UiKey::Confirm);
        require(run->party().gold() == initialGold - 107 &&
                    run->merchants().find(host)->buyback[0].quantity == 1 &&
                    run->merchants().find(host)->buyback[0].price == 15,
                "Healing potion sale did not create a persistent buyback offer");
        shot("trade", target);
        commands.finishDialogue();
        require(game.timeMs() == time && game.movement().player == emporiumCustomer,
                "Dialogue/trading changed game time or grid pose");
        report << "Both potion types stocked; three topics, purchases, sale, buyback and frozen simulation "
                  "verified\n";
    }
    const auto checkpoint = run->checkpoint();
    const CampaignContent content{"shadowtart-review", definitions.definitionFingerprint};
    const auto encoded = encodeCampaign(catalog, definitions, content, *checkpoint);
    auto decoded = decodeCampaign(catalog, definitions, content, encoded);
    view.reset();
    run = std::make_unique<CampaignRun>(catalog, std::move(*decoded), definitions.timings);
    require(run->game().actors().snapshot().count == 1 && run->dialogue().get(host)->introduced &&
                run->dialogue().get(host)->completedCount == 3 &&
                run->merchants().find(host)->stock[catalog.findItem("healing_potion").index] == 5 &&
                run->merchants().find(host)->stock[catalog.findItem("mana_potion").index] == 5 &&
                run->merchants().find(host)->buyback[0].quantity == 1 &&
                run->merchants().find(host)->buyback[0].price == 15 &&
                run->merchants().find(host)->buyback[0].item.index ==
                    catalog.findItem("healing_potion").index &&
                run->party().gold() == initialGold - 107,
            "Production save/Continue lost Shadowtart state");
    require(!renderer.memory().residentTextures && !renderer.memory().residentAlpha,
            "Shadowtart review leaked world leases");
    report << "Production Continue retains Shadowtart, history, both potion stocks, buyback and purse; "
              "leases released\n";
}
