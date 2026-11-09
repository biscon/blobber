#pragma once
#include "assets/blueprint_loader.hpp"
#include "assets/dialogue.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "assets/shops.hpp"
#include "game/campaign.hpp"
#include "game/smithy_interior.hpp"
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
inline void reviewBlacksmith(const std::filesystem::path& root, const std::filesystem::path& output) {
    using namespace blobber;
    const auto require = [](bool valid, const char* message) {
        if (!valid)
            throw std::runtime_error(message);
    };
    const auto catalog = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    const auto roomBlueprints = blueprints::load(root / "blueprints/underkeep.json");
    auto dialogue = loadDialogue(root / "dialogue");
    const auto shops = loadShops(root / "shops", catalog);
    validateShops(dialogue, shops);
    constexpr auto host = dialogueKey("sanctuary.blacksmith");
    const auto* character = dialogue.find(host);
    require(character && character->name == "Duncan Ironballs" && character->topics.size() == 5,
            "Duncan requires three conversation topics, Trade and Farewell");
    const auto* shop = shops.find(character->shop);
    require(shop && shop->key == "sanctuary_smithy", "Duncan shop binding missing");
    unsigned equipment = 0;
    for (std::uint16_t i = 0; i < catalog.itemCount(); ++i) {
        const auto kind = catalog.item({i})->kind;
        if (kind != rules::ItemKind::Weapon && kind != rules::ItemKind::Armor &&
            kind != rules::ItemKind::Shield && kind != rules::ItemKind::Ammunition)
            continue;
        ++equipment;
        const auto stocked = std::find_if(shop->stock.begin(), shop->stock.end(),
                                          [i](const ShopStock& s) { return s.item.index == i; });
        require(stocked != shop->stock.end() && stocked->quantity && shop->prices[i].buy,
                "Current equipment absent from Duncan's stock");
    }
    require(equipment == shop->stock.size(), "Unexpected smithy merchandise");
    auto run = std::make_unique<CampaignRun>(roomBlueprints, catalog, *catalog.preset(), definitions.types,
                                             definitions.timings, 5);
    run->game().restorePose(sanctuaryCity().doorway("smithy").approach, false);
    {
        auto ticket = run->prepareTravel(run->doorwayRequest());
        require(run->commitTravel(ticket), "Cannot enter smithy");
    }
    const auto grant = [&](const char* item, unsigned quantity) {
        require(
            bool(run->party().acquire(
                {{0}, catalog.findItem(item), std::uint16_t(quantity), run->party().inventory().revision()})),
            "Cannot create isolated trade fixture");
    };
    grant("gold", 1000);
    grant("dagger", 1);
    const auto initialGold = run->party().gold();
    Window window(1920, 1080, true);
    require(IsWindowState(FLAG_WINDOW_HIDDEN), "Review window must stay hidden");
    GpuResources resources;
    Presenter presenter(resources, root);
    prepareDialogueUi(dialogue, presenter.fonts());
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    auto view = std::make_unique<GameView>(renderer);
    std::ofstream report(output / "blacksmith.txt");
    {
        auto& game = run->game();
        require(game.actors().snapshot().count == 1 &&
                    game.actors().snapshot().actors[0].npc.residentId == host,
                "Smithy requires exactly one Duncan");
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
            require(!commands.snapshot().targets.count, "Duncan entered offensive picking");
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
        game.restorePose(smithyCustomer, false);
        shot("front");
        require(std::string_view(commands.snapshot().fixturePrompt.data()).find("Duncan Ironballs") !=
                    std::string_view::npos,
                "Duncan's interaction prompt is missing");
        game.restorePose({{7, 3}, Heading::West}, false);
        shot("left");
        game.restorePose({{5, 3}, Heading::East}, false);
        shot("right");
        game.restorePose(smithyCustomer, false);
        refresh();
        require(commands.startDialogue(), "Duncan conversation failed");
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
                    std::string_view(commands.snapshot().trade.merchantName) == "Duncan Ironballs",
                "Trade topic did not open Duncan's shop");
        const auto cell = [&](TradeSide side, unsigned index) {
            const auto r = tradeLayout::cell(side, index);
            UiInput input;
            input.pointer = ArtPoint{float(r.x + 8), float(r.y + 8)};
            input.pressed[unsigned(UiKey::Click)] = true;
            routeHudInput(commands, input);
        };
        require(commands.snapshot().trade.merchant[0].definition->id == "longsword",
                "Review longsword offer missing");
        cell(TradeSide::Merchant, 0);
        key(UiKey::Confirm);
        require(run->party().gold() == initialGold - 30 &&
                    run->merchants().find(host)->stock[catalog.findItem("longsword").index] == 1,
                "Duncan purchase did not conserve finite stock/payment");
        unsigned dagger = bagCells;
        for (unsigned i = 0; i < bagCells; ++i) {
            const auto* item = commands.snapshot().trade.player[i].definition;
            if (item && item->id == "dagger")
                dagger = i;
        }
        require(dagger < bagCells, "Review dagger fixture missing");
        cell(TradeSide::Player, dagger);
        key(UiKey::Confirm);
        require(run->party().gold() == initialGold - 28 && run->merchants().find(host)->buyback[0].price == 3,
                "Duncan sale/buyback failed");
        shot("trade", target);
        commands.finishDialogue();
        require(game.timeMs() == time && game.movement().player == smithyCustomer,
                "Dialogue/trading changed game time or grid pose");
        report << "All " << equipment
               << " current equipment types stocked; three topics, purchase, sale, "
                  "buyback and frozen simulation verified\n";
    }
    const auto checkpoint = run->checkpoint();
    const CampaignContent content{"blacksmith-review", definitions.definitionFingerprint, &roomBlueprints};
    const auto encoded = encodeCampaign(catalog, definitions, content, *checkpoint);
    auto decoded = decodeCampaign(catalog, definitions, content, encoded);
    view.reset();
    run = std::make_unique<CampaignRun>(roomBlueprints, catalog, std::move(*decoded), definitions.timings);
    require(run->game().actors().snapshot().count == 1 && run->dialogue().get(host)->introduced &&
                run->dialogue().get(host)->completedCount == 3 &&
                run->merchants().find(host)->stock[catalog.findItem("longsword").index] == 1 &&
                run->merchants().find(host)->buyback[0].price == 3 && run->party().gold() == initialGold - 28,
            "Production save/Continue lost Duncan state");
    require(!renderer.memory().residentTextures && !renderer.memory().residentAlpha,
            "Blacksmith review leaked world leases");
    report << "Production Continue retains Duncan, history, stock, buyback and purse; leases released\n";
}
