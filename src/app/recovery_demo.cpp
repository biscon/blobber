#include "app/recovery_demo.hpp"
#include "ui/camp_layout.hpp"
#include "ui/interaction.hpp"
#include <stdexcept>
namespace blobber {
namespace {
std::vector<std::string> names(const NpcWorld& source) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < source.typeCount(); ++i)
        out.push_back(source.typeId({static_cast<std::uint16_t>(i)}));
    return out;
}
} // namespace
NpcWorld recoveryDemoPlacements(const Map& map, const NpcWorld& source) {
    const auto types = names(source);
    NpcTypeId melee{}, archer{};
    for (std::uint16_t i = 0; i < types.size(); ++i) {
        if (types[i] == "goblin")
            melee = {i};
        if (types[i] == "goblin.archer")
            archer = {i};
    }
    return {map,
            Player{}.cell,
            types,
            {{melee, {3, 15}, {0, 0}, Heading::North},
             {melee, {13, 19}, {0, 0}, Heading::North},
             {archer, {18, 17}, {0, 0}, Heading::North}}};
}
RecoveryLevel recoveryDemoLevel(const rules::Catalog& catalog) {
    RecoveryLevel out;
    out.sanctuaries[out.sanctuaryCount++] = Player{}.cell;
    for (auto cell : {Cell{7, 15}, Cell{19, 19}})
        out.ground[out.groundCount++] = {cell, catalog.findItem("camp_kit"), 1};
    return out;
}
void captureRecovery(const rules::Catalog& catalog, const NpcWorld& source,
                     const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                     Presenter& presenter, const std::filesystem::path& directory) {
    const Map map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                   "#.......#", "#########"});
    const Player pose{{4, 5}, Heading::North};
    const NpcWorld empty(map, pose.cell, names(source), {});
    RecoveryLevel level;
    const char* items[] = {"scimitar", "leather",        "shield",      "shortbow", "arrow",
                           "gold",     "healing_potion", "mana_potion", "camp_kit"};
    for (int i = 0; i < 9; ++i) {
        const auto id = catalog.findItem(items[i]);
        if (!catalog.item(id))
            throw std::runtime_error(std::string("Missing capture item: ") + items[i]);
        level.ground[level.groundCount++] = {{4, 4}, id, 1};
    }
    level.ground[level.groundCount++] = {pose.cell, catalog.findItem("camp_kit"), 1};
    level.ground[level.groundCount++] = {{3, 4}, catalog.findItem("gold"), 6};
    level.ground[level.groundCount++] = {{5, 4}, catalog.findItem("healing_potion"), 1};
    GameSession s(map, catalog, empty, 5, pose, false, timings, level);
    CommandController ui(s, timings);
    NpcAnimator animator(s.actors(), timings);
    FramePipeline frames;
    auto save = [&](const std::string& name) {
        animator.setTime(s.timeMs());
        frames.update(renderer, map, s.movement(), &s.actors(), &animator.snapshot(), &s.combat().attacks(),
                      s.timeMs(), nullptr, &s.loot().snapshot(), &ui.snapshot().lootPage);
        ui.refresh(frames.picking()->targets(), false, false, true);
        presenter.present(frames.frame(), s.movement(), map, false, true, &ui.snapshot(), frames.picking());
        presenter.capture(directory / (name + ".png"), GetScreenWidth(), GetScreenHeight());
    };
    save("loot-six-items");
    const LootId first{s.actors().snapshot().world, 2048};
    ui.floorHover(first);
    save("loot-hover");
    UiInput wheel;
    wheel.focused = true;
    wheel.scroll = 1;
    routeHudInput(ui, wheel, {}, true, first);
    save("loot-page-two");
    for (int y = 0; y < viewHeight; ++y) {
        bool clicked = false;
        for (int x = 0; x < viewWidth; ++x) {
            const auto hit = frames.picking()->pickLoot(x, y);
            if (hit && hit->slot == 2056) {
                UiInput click;
                click.focused = true;
                click.pointer = ArtPoint{float(x + viewX), float(y + viewY)};
                click.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
                routeHudInput(ui, click, {}, true, hit);
                clicked = true;
                break;
            }
        }
        if (clicked)
            break;
    }
    save("loot-collected");
    for (std::uint16_t i = 0; i < 4; ++i)
        s.party().damage({i}, 3 + i);
    s.party().spendMana({3}, 1);
    UiInput input;
    input.focused = true;
    input.pressed[static_cast<std::size_t>(UiKey::Rest)] = true;
    routeHudInput(ui, input, {});
    save("camp-short-zero");
    for (unsigned i = 0; i < 4; ++i)
        ui.campDice(i, 1);
    save("camp-short-dice");
    ui.startRest();
    for (int frame = 0; frame < 4; ++frame) {
        save("camp-fire-" + std::to_string(frame));
        ui.tickCamp(.1, true);
        ui.tickCamp(.1, true);
    }
    for (int i = 0; i < 10; ++i)
        ui.tickCamp(.1, true);
    save("camp-fade-out");
    for (int i = 0; i < 3; ++i)
        ui.tickCamp(.1, true);
    save("camp-fade-in");
    for (int i = 0; i < 3; ++i)
        ui.tickCamp(.1, true);
    save("camp-short-result");
    ui.openCamp();
    ui.campKind(RestKind::Long);
    save("camp-long-kit");
    ui.startRest();
    for (int i = 0; i < 24; ++i)
        ui.tickCamp(.1, true);
    save("camp-long-result");
    ui.openCamp();
    ui.campKind(RestKind::Long);
    save("camp-no-benefit");
    ui.closeOverlay();
    for (std::uint16_t c = 0; c < 4; ++c)
        for (std::uint16_t cell = 0; cell < bagCells; ++cell) {
            const auto h = s.party().inventory().at({{c}, cell});
            const auto* stack = s.party().inventory().stack(h);
            if (stack && catalog.item(stack->item)->kind == rules::ItemKind::Supply)
                s.party().consumeItem(h, stack->quantity);
        }
    s.party().damage({0}, 2);
    ui.openCamp();
    ui.campKind(RestKind::Long);
    save("camp-no-supplies");
    ui.closeOverlay();
    s.party().damage({1}, s.party().character({1})->hitPoints);
    ui.openCamp();
    save("camp-dying-ally");
    ui.closeOverlay();
    {
        NpcTypeId type{};
        for (std::uint16_t i = 0; i < source.typeCount(); ++i)
            if (source.typeId({i}) == "goblin")
                type = {i};
        const NpcWorld placed(map, pose.cell, names(source), {{type, {4, 4}, {0, 1}, Heading::South}});
        GameSession dead(map, catalog, placed, 5, pose, false, timings);
        CommandController hud(dead, timings);
        NpcAnimator poses(dead.actors(), timings);
        FramePipeline images;
        const auto shot = [&](const char* name) {
            poses.setTime(dead.timeMs());
            images.update(renderer, map, dead.movement(), &dead.actors(), &poses.snapshot(),
                          &dead.combat().attacks(), dead.timeMs(), nullptr, &dead.loot().snapshot(),
                          &hud.snapshot().lootPage);
            hud.refresh(images.picking()->targets(), false, false, true);
            presenter.present(images.frame(), dead.movement(), map, false, true, &hud.snapshot(),
                              images.picking());
            presenter.capture(directory / (std::string(name) + ".png"), GetScreenWidth(), GetScreenHeight());
        };
        hud.refresh({}, false, false, true);
        hud.openCamp();
        shot("camp-threat");
        hud.closeOverlay();
        dead.actors().damage(dead.actors().id(0), 100, 0);
        dead.update(.7, false);
        dead.combat().continueExploring();
        shot("loot-corpse");
        dead.actors().remove(dead.actors().id(0));
        dead.update(0, true);
        shot("loot-after-corpse-removal");
    }
}
} // namespace blobber
