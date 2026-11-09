#include "assets/dialogue.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "assets/shops.hpp"
#include "game/campaign.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "ui/dialogue_preparation.hpp"
#include "ui/interaction.hpp"
#include "ui/trade_layout.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>
using namespace blobber;
namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
void enter(CampaignRun& run, const std::string& building) {
    const auto approach = sanctuaryCity().doorway(building).approach;
    run.game().restorePose(approach, false);
    auto ticket = run.prepareTravel(run.doorwayRequest());
    require(run.commitTravel(ticket), "Enter review merchant");
    run.game().restorePose({{6, 5}, Heading::North}, false);
}
} // namespace
int main(int argc, char** argv) try {
    if (argc != 4)
        throw std::runtime_error("Usage: blobber_review_early_equipment ASSETS PREPARED NEW_OUTPUT");
    const std::filesystem::path root(argv[1]), prepared(argv[2]), output(argv[3]);
    require(std::filesystem::create_directory(output), "Use new output directory");
    const auto catalog = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    auto dialogue = loadDialogue(root / "dialogue");
    const auto shops = loadShops(root / "shops", catalog);
    validateShops(dialogue, shops);
    auto run =
        std::make_unique<CampaignRun>(catalog, *catalog.preset(), definitions.types, definitions.timings, 5);
    SetTraceLogLevel(LOG_WARNING);
    Window window(1920, 1080, true, true);
    require(IsWindowState(FLAG_WINDOW_HIDDEN), "Capture window must remain hidden");
    GpuResources resources;
    Presenter presenter(resources, root, nullptr, prepared);
    prepareDialogueUi(dialogue, presenter.fonts());
    WorldRenderer renderer(resources, world_assets::load(prepared / "world"));
    std::ofstream report(output / "review.txt");
    const auto capture = [&](const char* name, bool inventory, bool merchant) {
        auto& game = run->game();
        game.discover();
        NpcAnimator animation(game.actors(), definitions.timings);
        animation.setTime(game.timeMs(), &game.effects());
        GameView view(renderer);
        view.update(game, animation.snapshot(), {});
        view.activate();
        CommandController controls(game, definitions.timings);
        controls.bindDialogue(&dialogue, &run->dialogue());
        controls.bindTrade(&shops, &run->merchants());
        controls.residentVisibility(view.picking()->residents());
        controls.refresh(view.picking()->targets(), false, false, true);
        if (merchant) {
            require(controls.startDialogue(), "Start merchant review dialogue");
            controls.tickDialogue(.8, true);
            require(controls.openTrade(), "Open merchant review stock");
            unsigned selected = 1;
            for (unsigned i = 0; i < 48; ++i) {
                const auto* item = controls.snapshot().trade.merchant[i].definition;
                if (item && item->id == "greatsword")
                    selected = i;
            }
            const auto cell = tradeLayout::cell(TradeSide::Merchant, selected);
            UiInput input;
            input.pointer = ArtPoint{float(cell.x + 8), float(cell.y + 8)};
            routeHudInput(controls, input);
            controls.tickDialogue(.36, true);
        }
        if (inventory) {
            controls.openInventory(rules::CharacterId{3});
            controls.inventorySelect(unsigned(EquipmentSlot::Cloak));
        }
        const auto memory = resources.counts().bytes;
        view.present(presenter, game, game.geometry().base(), false, true, &controls.snapshot());
        require(resources.counts().bytes == memory, "Review drawing must not grow GPU residency");
        presenter.capture(output / (std::string(name) + ".png"));
        report << name << " status=" << unsigned(run->status()) << " objective=" << game.objective().completed
               << " gpu_bytes=" << memory << '\n';
    };
    enter(*run, "inn");
    capture("inn-provisions", false, true);
    // Return through the owned doorway; no desktop input or user save access.
    run->game().restorePose(run->location(unsigned(CampaignLocationId::Inn)).exit, false);
    {
        auto ticket = run->prepareTravel(run->doorwayRequest());
        require(run->commitTravel(ticket), "Leave review inn");
    }
    enter(*run, "smithy");
    capture("smithy-weapons", false, true);
    run->game().restorePose(run->location(unsigned(CampaignLocationId::Smithy)).exit, false);
    {
        auto ticket = run->prepareTravel(run->doorwayRequest());
        require(run->commitTravel(ticket), "Leave review smithy");
    }
    const auto grant = [&](const char* name) {
        auto& party = run->party();
        const auto id = catalog.findItem(name);
        require(bool(party.acquire({{3}, id, 1, party.inventory().revision()})),
                "Grant isolated review equipment");
        for (std::uint16_t cell = 0; cell < bagCells; ++cell) {
            const auto handle = party.inventory().at({{3}, cell});
            const auto* stack = party.inventory().stack(handle);
            if (!stack || stack->item.index != id.index)
                continue;
            require(bool(party.inventoryChange(
                        {InventoryOp::Move,
                         handle,
                         {{3}, std::uint16_t(rules::itemEquipmentPosition(*catalog.item(id)))},
                         0,
                         party.inventory().revision()})),
                    "Equip isolated review item");
            return;
        }
        throw std::runtime_error("Missing granted item");
    };
    for (const auto* name : {"circlet_of_focus", "gloves_of_striking", "boots_of_balance", "belt_of_vigor",
                             "cloak_of_protection", "ring_of_resolve", "amulet_of_reserves",
                             "apprentice_bracers", "dagger_plus1"})
        grant(name);
    capture("magic-equipment", true, false);
    for (unsigned destination = 1; destination <= 3; ++destination) {
        auto& game = run->game();
        if (destination > 1) {
            for (unsigned i = 0; i < game.actors().snapshot().count; ++i)
                game.actors().damage(game.actors().id(i), 100000, game.timeMs());
            game.update(1, false);
        }
        bool traveled = false;
        for (unsigned i = 0; i < game.fixtures().definition().count; ++i) {
            const auto& f = game.fixtures().definition().entries[i];
            if (f.kind != FixtureKind::Downstairs)
                continue;
            game.restorePose({stairLanding(f), f.facing}, false);
            game.discover();
            auto ticket =
                run->prepareTravel(run->request({game.fixtures().id(i), game.fixtures().revision()}));
            require(run->commitTravel(ticket), "Travel to owned final-floor review");
            traveled = true;
            break;
        }
        require(traveled, "Missing review stairs");
    }
    auto& game = run->game();
    for (unsigned i = 0; i < game.actors().snapshot().count; ++i)
        game.actors().damage(game.actors().id(i), 100000, game.timeMs());
    game.update(1, false);
    require(run->status() == CampaignStatus::Active && game.runActive(),
            "Final guardian must retain active gameplay");
    const auto chest = game.loot().snapshot().containers[2];
    bool placed = false;
    for (unsigned h = 0; h < 4; ++h) {
        const auto cell = chest.cell + forward(Heading(h));
        if (game.geometry().movementBlocked(cell))
            continue;
        game.restorePose({cell, Heading((h + 2) % 4)}, false);
        if (game.containerAccess({game.actors().snapshot().world, 2})) {
            placed = true;
            break;
        }
    }
    require(placed, "Final reward chest needs a legal approach");
    require(bool(game.openContainer({game.actors().snapshot().world, 2})), "Open final reward chest");
    capture("final-chest-active", false, false);
    auto checkpoint = run->checkpoint();
    require(bool(checkpoint), "Completed final floor can checkpoint");
    using GlError = unsigned (*)();
    require(reinterpret_cast<GlError>(rlGetProcAddress("glGetError"))() == 0,
            "Equipment review OpenGL error");
    require(IsWindowState(FLAG_WINDOW_HIDDEN), "Capture window remained hidden");
    std::cout << "Equipment, merchants and active final chest captured\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
