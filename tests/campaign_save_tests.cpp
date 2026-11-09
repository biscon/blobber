#include "app/campaign_saves.hpp"
#include "app/result_flow.hpp"
#include "core/preparation.hpp"
#include "persistence/campaign_store.hpp"
#include "persistence/character_library.hpp"
#include "support/campaign_fixtures.hpp"
#include "support/early_equipment_fixtures.hpp"
#include "ui/interaction.hpp"
#include "ui/save_layout.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
using namespace blobber;
using namespace blobber::automation::test;
using namespace blobber::campaign_test;
namespace {
template <class F> void rejects(F f, const char* message) {
    bool rejected = false;
    try {
        f();
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected, message);
}
// Mutable access to schema-20 indexed changes for corruption fixtures.
blobber::automation::Json& savedEntry(blobber::automation::Json& entries, unsigned index) {
    using Json = blobber::automation::Json;
    if (entries.is_null())
        entries = Json::array();
    for (auto& entry : entries)
        if (entry[0] == index)
            return entry[1];
    const auto at = std::find_if(entries.begin(), entries.end(), [&](const auto& entry) {
        return entry[0].template get<unsigned>() > index;
    });
    const auto inserted = entries.insert(at, Json::array({index, Json::object()}));
    return (*inserted)[1];
}
blobber::automation::Json& savedLocation(blobber::automation::Json& save, unsigned index) {
    return savedEntry(save["campaign"]["locations"], index);
}
void ignoredCampaigns() {
    const auto c = catalog();
    const auto d = definitions();
    Directory directory;
    const auto path = directory.path / "campaign.json";
    CampaignDisk disk(path);
    CampaignStore store(c, d, {"fixture", 0}, disk);
    for (const auto* bytes : {"{", "{\"schema\":0,\"simulation\":0}"}) {
        std::ofstream(path) << bytes;
        const auto slot = store.inspect();
        check(slot.exists && !slot.active && !slot.error.empty(),
              "incompatible/corrupt campaign disables only Continue");
        CampaignRun fresh(c, *c.preset(), d.types, d.timings, 5);
        rejects([&] { store.save(fresh, slot.revision); }, "old slot still needs replacement confirmation");
        check(store.save(fresh, slot.revision, true).durable,
              "fresh game can replace ignored slot after confirmation");
        check(store.inspect().active && store.load()->runId() == fresh.runId(),
              "new campaign is loadable after ignored save");
    }
}
rules::Catalog wearableSaveCatalog() {
    const auto base = catalog();
    std::vector<rules::AttackDefinition> attacks;
    std::vector<rules::MonsterDefinition> monsters;
    std::vector<rules::RaceDefinition> races;
    std::vector<rules::ClassDefinition> classes;
    std::vector<rules::ItemDefinition> items;
    std::vector<rules::SpellDefinition> spells;
    std::vector<rules::CreationClass> creation;
    for (std::uint16_t i = 0; i < base.attackCount(); ++i)
        attacks.push_back(*base.attack({i}));
    for (std::uint16_t i = 0; i < base.monsterCount(); ++i)
        monsters.push_back(*base.monster({i}));
    for (std::uint16_t i = 0; i < base.raceCount(); ++i)
        races.push_back(*base.race({i}));
    for (std::uint16_t i = 0; i < base.classCount(); ++i) {
        classes.push_back(*base.characterClass({i}));
        if (const auto* value = base.creation({i}))
            creation.push_back(*value);
    }
    for (std::uint16_t i = 0; i < base.itemCount(); ++i)
        items.push_back(*base.item({i}));
    for (std::uint16_t i = 0; i < base.spellCount(); ++i)
        spells.push_back(*base.spell({i}));
    rules::ItemDefinition wrists;
    wrists.id = "fixture.bracers";
    wrists.name = "Fixture bracers";
    wrists.description = "Test-owned mundane wrist equipment";
    wrists.wearSlot = rules::WearSlot::Wrists;
    items.push_back(wrists);
    return {attacks, monsters, races, classes, items, *base.preset(), spells, creation};
}
void environmentRoundtrip() {
    const auto c = wearableSaveCatalog();
    const auto d = definitions();
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    check(bool(run.party().acquire(
              {{0}, c.findItem("fixture.bracers"), 1, run.party().inventory().revision()})),
          "Acquire save-owned wrist fixture");
    ItemHandle wearable;
    for (std::uint16_t cell = 0; cell < bagCells; ++cell) {
        const auto handle = run.party().inventory().at({{0}, cell});
        const auto* stack = run.party().inventory().stack(handle);
        if (stack && stack->item.index == c.findItem("fixture.bracers").index)
            wearable = handle;
    }
    check(bool(run.party().inventoryChange({InventoryOp::Move,
                                            wearable,
                                            {{0}, unsigned(EquipmentSlot::Wrists)},
                                            0,
                                            run.party().inventory().revision()})),
          "Equip last runtime slot before serialization");
    auto state = run.checkpoint();
    auto& world = state->locations[1].world;
    const auto fixtureCell = state->locations[1].entry.cell;
    const auto index = std::size_t(fixtureCell.y * state->locations[1].rows[0].size() + fixtureCell.x);
    // Preserve level stair surfaces while round-tripping an authored scenery base elsewhere.
    Cell sceneryCell{};
    bool found = false;
    const auto& map = run.location(1).map();
    for (int y = 2; y + 2 < map.height() && !found; ++y)
        for (int x = 2; x + 2 < map.width() && !found; ++x) {
            const Cell c{x, y};
            if (!map.solid(c) && !map.movementBlocked(c) &&
                !state->locations[1].fixtures.npcExcluded[std::size_t(y * map.width() + x)]) {
                bool occupied = false;
                for (const auto& npc : state->locations[1].placements)
                    occupied |= npc.cell == c;
                for (unsigned f = 0; f < state->locations[1].fixtures.count; ++f)
                    occupied |= state->locations[1].fixtures.entries[f].cell == c;
                for (std::size_t g = 0; g < state->locations[1].recovery.groundCount; ++g)
                    occupied |= state->locations[1].recovery.ground[g].cell == c;
                // Use a side cell with at least three exits to avoid disconnecting corridors.
                unsigned exits = 0;
                for (unsigned h = 0; h < 4; ++h)
                    exits += map.canStep(c, c + forward(Heading(h)));
                if (!occupied && exits >= 3) {
                    sceneryCell = c;
                    found = true;
                }
            }
        }
    check(found, "Owned scenery fixture has an unoccupied open cell");
    world.surfaces[index].ceiling = noCeiling;
    world.scenery.push_back({"owned-tree", sceneryCell, {0, 0}, Heading::East, 1, 1});
    // Own a one-cell static shell on an existing generated wall, away from dressing.
    auto& architecture = state->locations[1];
    bool placed = false;
    for (unsigned y = 2; y + 2 < architecture.rows.size() && !placed; ++y)
        for (unsigned x = 2; x + 2 < architecture.rows[y].size() && !placed; ++x) {
            const Cell cell{int(x), int(y)};
            if (architecture.rows[y][x] != '#')
                continue;
            bool conflict = false;
            for (const auto& decoration : architecture.world.decorations)
                conflict |= decoration.cell == cell || decoration.cell + forward(decoration.face) == cell;
            if (conflict)
                continue;
            architecture.world.buildings.push_back({"owned-house", cell, {0, 0}, Heading::West, 1, 1});
            placed = true;
        }
    check(placed, "Owned save fixture has a wall for static architecture");
    architecture.worldFingerprint = architecture.world.fingerprint();
    architecture.layout.dressingFingerprint = architecture.worldFingerprint;
    architecture.fingerprint = generation::fingerprint(architecture.layout);
    const auto fingerprint = world.fingerprint();
    const auto text = encodeCampaign(c, d, {"fixture", 0}, *state);
    auto decoded = decodeCampaign(c, d, {"fixture", 0}, text);
    Party restoredParty(c, decoded->party);
    check(restoredParty.equipped({0}, EquipmentSlot::Wrists) == c.item(c.findItem("fixture.bracers")) &&
              !restoredParty.inventory().stack(wearable),
          "Campaign codec preserves final equipment cell with fresh runtime identity");
    auto previous = blobber::automation::Json::parse(text);
    previous["schema"] = 17;
    previous["simulation"] = 21;
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, previous.dump()); },
            "Pre-paperdoll campaign format rejects under strict save policy");
    const auto& restoredWorld = decoded->locations[1].world;
    check(restoredWorld.fingerprint() == fingerprint && restoredWorld.buildings.size() == 1 &&
              restoredWorld.buildings[0].model == "owned-house" &&
              restoredWorld.buildings[0].facing == Heading::West,
          "Owned world geometry and architecture survive campaign codec roundtrip");
    check(restoredWorld.scenery.size() == 1 && restoredWorld.scenery[0].model == "owned-tree" &&
              restoredWorld.scenery[0].facing == Heading::East,
          "Scenery identity, orientation and physical base survive campaign codec roundtrip");
    auto bad = blobber::automation::Json::parse(text);
    savedEntry(savedLocation(bad, 0)["world"]["surfaces"]["entries"], 20)["floor"] = 99;
    rejects(
        [&] {
            auto v = decodeCampaign(c, d, {"fixture", 0}, bad.dump());
            CampaignRun invalid(c, std::move(*v), d.timings);
        },
        "Malformed surface retains prior session");
    bad = blobber::automation::Json::parse(text);
    bad["schema"] = 7;
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, bad.dump()); }, "Pre-architecture schema rejects");
    bad["schema"] = 21;
    savedLocation(bad, 1)["world"]["buildings"] = {{"size", buildingCapacity + 1},
                                                   {"entries", blobber::automation::Json::array()}};
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, bad.dump()); },
            "Placement decode capacity rejects before resizing");
    bad = blobber::automation::Json::parse(text);
    savedLocation(bad, 0)["world"]["scenery"] = {{"size", sceneryCapacity + 1},
                                                 {"entries", blobber::automation::Json::array()}};
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, bad.dump()); },
            "Scenery decode capacity rejects before resizing");
    bad = blobber::automation::Json::parse(text);
    bad["schema"] = 8;
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, bad.dump()); }, "Pre-scenery saves reject clearly");
    bad = blobber::automation::Json::parse(text);
    bad["schema"] = 9;
    bad["simulation"] = 13;
    Directory directory;
    CampaignDisk disk(directory.path / "campaign.json");
    {
        std::ofstream file(directory.path / "campaign.json");
        file << bad.dump();
    }
    CampaignStore store(c, d, {"fixture", 0}, disk);
    check(!store.inspect().active, "Pre-inn campaign is incompatible");
    std::ifstream unchanged(directory.path / "campaign.json");
    const std::string original((std::istreambuf_iterator<char>(unchanged)), {});
    check(original == bad.dump(), "Rejecting old simulation never rewrites its file");
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, bad.dump()); },
            "Previous schema/simulation 9/13 rejects explicitly");
    bad = blobber::automation::Json::parse(text);
    bad["simulation"] = 19;
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, bad.dump()); },
            "Pre-Speed simulation 19 rejects without implicit migration");
    bad = blobber::automation::Json::parse(text);
    bad["generator"] = 9;
    {
        std::ofstream file(directory.path / "campaign.json");
        file << bad.dump();
    }
    check(!store.inspect().active, "Campaign predating room-safe props is incompatible");
    {
        std::ifstream file(directory.path / "campaign.json");
        const std::string bytes((std::istreambuf_iterator<char>(file)), {});
        check(bytes == bad.dump(), "Rejecting old prop generation never rewrites the save");
    }
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, bad.dump()); },
            "Generator 9 campaign rejects explicitly");
    bad = blobber::automation::Json::parse(text);
    bad["simulation"] = 3;
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, bad.dump()); },
            "Pre-B01 simulation rejects explicitly");
}
void roundtrip() {
    const auto c = catalog();
    const auto d = definitions();
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    walk(run.game(), {12, 20});
    face(run.game(), Heading::South);
    const CampaignContent content{"owned-fixture", 0};
    ShopDefinition shop;
    shop.id = dialogueKey("fixture.shop");
    shop.stock.push_back({{0}, 4});
    auto* merchant = run.merchants().visit(dialogueKey("fixture.merchant"), shop, run.gameTime());
    merchant->stock[0] = 2;
    merchant->buyback[0] = {{0}, 1, 13, gameDuration::day};
    auto before = run.checkpoint();
    before->party.gold = UINT64_MAX;
    const auto text = encodeCampaign(c, d, content, *before);
    auto decoded = decodeCampaign(c, d, content, text);
    CampaignRun loaded(c, std::move(*decoded), d.timings);
    auto after = loaded.checkpoint();
    check(encodeCampaign(c, d, content, *after) == text, "hub exact canonical round trip");
    check(loaded.party().inventory().identity() != run.party().inventory().identity(),
          "fresh inventory epoch");
    const auto* restoredMerchant = loaded.merchants().find(dialogueKey("fixture.merchant"));
    check(restoredMerchant && restoredMerchant->stock[0] == 2 && restoredMerchant->buyback[0].price == 13 &&
              restoredMerchant->buyback[0].expiresMs == gameDuration::day,
          "Visited merchant stock, marked-up buyback and deadlines survive campaign codec/restore");
    for (int schema : {12, 13, 14}) {
        auto legacy = blobber::automation::Json::parse(text);
        legacy["schema"] = schema;
        rejects([&] { decodeCampaign(c, d, content, legacy.dump()); },
                "Older schemas reject without conversion");
    }
    check(loaded.party().gold() == UINT64_MAX, "Largest party purse survives canonical save round trip");
    auto badGold = blobber::automation::Json::parse(text);
    badGold["campaign"]["party"]["gold"] = "18446744073709551616";
    rejects([&] { decodeCampaign(c, d, content, badGold.dump()); }, "Purse overflow rejects on decode");
    badGold["campaign"]["party"].erase("gold");
    rejects([&] { decodeCampaign(c, d, content, badGold.dump()); }, "Missing purse rejects on decode");
    auto badMerchant = blobber::automation::Json::parse(text);
    savedEntry(savedEntry(badMerchant["campaign"]["merchants"]["entries"], 0)["buyback"], 0)["quantity"] =
        65535;
    rejects(
        [&] {
            auto state = decodeCampaign(c, d, content, badMerchant.dump());
            CampaignRun invalid(c, std::move(*state), d.timings);
        },
        "Invalid persisted merchant quantity rejects before publication");
    entrance(run);
    auto travel = run.prepareTravel(run.request(run.game().nearbyFixture()));
    check(run.commitTravel(travel), "enter dungeon");
    run.party().damage({0}, 2);
    auto floor = run.checkpoint();
    auto duplicate = run.checkpoint();
    auto& actors = duplicate->locations[1].state->actors;
    check(actors.count >= 2, "owned generated floor contains a pair");
    actors.actors[1].npc.cell = actors.actors[0].npc.cell;
    actors.actors[1].npc.subcell = actors.actors[0].npc.subcell;
    rejects([&] { CampaignRun invalid(c, std::move(*duplicate), d.timings); },
            "campaign restoration rejects duplicate living actor subcells");
    auto floorText = encodeCampaign(c, d, content, *floor);
    auto floorDecoded = decodeCampaign(c, d, content, floorText);
    CampaignRun resumed(c, std::move(*floorDecoded), d.timings);
    check(resumed.activeLocation() == 1 &&
              resumed.party().character({0})->hitPoints == run.party().character({0})->hitPoints,
          "floor and resource restore");
    for (unsigned i = 0; i < 100; ++i) {
        run.update(.01, false);
        resumed.update(.01, false);
    }
    auto x = std::make_unique<FloorState>(), y = std::make_unique<FloorState>();
    run.game().exportFloor(*x);
    resumed.game().exportFloor(*y);
    check(x->digest() == y->digest(), "continued floor RNG/AI/effects/clock equality");
    rejects([&] { decodeCampaign(c, d, content, text.substr(0, text.size() / 2)); }, "truncated rejected");
    auto bad = blobber::automation::Json::parse(text);
    bad["campaign"]["activeLocation"] = 999;
    rejects(
        [&] {
            auto v = decodeCampaign(c, d, content, bad.dump());
            CampaignRun invalid(c, std::move(*v), d.timings);
        },
        "bad location rejected");
    Directory directory;
    CampaignDisk disk(directory.path / "campaign.json");
    CampaignStore store(c, d, content, disk);
    auto info = store.inspect();
    check(!info.exists, "missing slot read only");
    auto result = store.save(run, info.revision);
    check(result.published && result.durable, "atomic save published");
    auto save = store.inspect();
    check(save.active && save.depth == 1, "validated slot summary");
    auto restored = store.load();
    check(restored->runId() == run.runId(), "run identity preserved");
    CampaignRun other(c, *c.preset(), d.types, d.timings, 5);
    rejects([&] { store.save(other, save.revision); }, "different run requires confirmation");
    for (unsigned i = 0; i < 4; ++i)
        run.party().damage({std::uint16_t(i)}, 10000);
    auto closed = store.terminal(run);
    check(closed.published && closed.durable, "terminal marker published");
    check(store.inspect().terminal, "terminal slot unavailable");
    rejects([&] { store.load(); }, "terminal load rejected");
    auto overwrite = store.save(other, store.inspect().revision, true);
    check(overwrite.durable, "explicit new run replaces marker");
    std::cout << "save bytes=" << text.size() << " checkpoint=" << sizeof(CampaignCheckpoint) << '\n';
}
std::unique_ptr<CampaignRun> copy(const rules::Catalog& c, const NpcDefinitions& d, CampaignRun& run) {
    auto s = run.checkpoint();
    auto text = encodeCampaign(c, d, {"fixture", 0}, *s);
    auto restored = decodeCampaign(c, d, {"fixture", 0}, text);
    auto result = std::make_unique<CampaignRun>(c, std::move(*restored), d.timings);
    auto after = result->checkpoint();
    check(encodeCampaign(c, d, {"fixture", 0}, *after) == text, "exact logical campaign round trip");
    return result;
}
void magicRewardPersistence() {
    const auto c = blobber::test::withMagicItems(catalog());
    const auto d = definitions();
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    std::array<std::array<rules::ItemId, 2>, 3> rewards{};
    std::array<rules::ItemId, 2> weapons{};
    for (unsigned depth = 1; depth <= 3; ++depth) {
        const auto& location = run.location(depth);
        for (unsigned i = 0; i < location.objective.rewardCount; ++i)
            rewards[depth - 1][i] = location.objective.rewards[i].item;
        for (unsigned i = 0; i < location.recovery.groundCount; ++i) {
            const auto id = location.recovery.ground[i].item;
            if (c.item(id)->enchantment)
                weapons[depth - 2] = id;
        }
    }
    // Starting eligibility remains immutable after both mana users die.
    run.party().damage({1}, 100000);
    run.party().damage({3}, 100000);
    auto restored = copy(c, d, run);
    check(restored->status() == CampaignStatus::Active, "Remaining companions keep campaign active");
    for (unsigned depth = 1; depth <= 3; ++depth) {
        const auto& location = restored->location(depth);
        for (unsigned i = 0; i < location.objective.rewardCount; ++i)
            check(location.objective.rewards[i].item.index == rewards[depth - 1][i].index,
                  "Unvisited wearable rewards preserve original party eligibility after deaths and reload");
        for (unsigned i = 0; i < location.recovery.groundCount; ++i) {
            const auto id = location.recovery.ground[i].item;
            if (c.item(id)->enchantment)
                check(id.index == weapons[depth - 2].index, "Unvisited optional weapons do not reroll");
        }
    }
    auto state = restored->checkpoint();
    auto text = encodeCampaign(c, d, {"fixture", 0}, *state);
    auto bad = blobber::automation::Json::parse(text);
    bad["reward_profile"][0] = UINT32_MAX;
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, bad.dump()); },
            "Invalid reward training mask rejects");
}
void spellPresentationLifecycle() {
    const auto c = catalog();
    const auto d = definitions();
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    const auto emptyJournals = [](const GameSession& game) {
        for (const auto& cue : game.supportVisuals().records)
            check(!cue.id.serial, "Campaign entry never replays support application bursts");
        for (const auto& cue : game.combat().spellVisuals().casts)
            check(!cue.id.serial, "Campaign entry never replays spell casts");
    };
    emptyJournals(run.game());
    CommandController hud(run.game());
    hud.refresh({}, false, false, true);
    EffectRequest ward;
    ward.world = run.game().actors().snapshot().world;
    ward.kind = EffectKind::ShieldOfFaith;
    ward.caster = rules::CharacterId{1};
    ward.targets[0] = rules::CharacterId{0};
    ward.count = 1;
    check(bool(run.game().applyEffect(ward)), "Campaign ward applies normally");
    hud.record(run.game().events());
    hud.refresh({}, false, false, true);
    const auto now = run.game().timeMs();
    const auto revision = run.game().supportVisuals().revision;
    const auto presentation = hud.snapshot().portraitFeedback[0][0];
    check(revision && presentation.kind != PortraitFeedbackKind::None,
          "Live application creates visible portrait feedback before pause");
    run.update(10, true);
    hud.refresh({}, false, false, true);
    check(run.game().timeMs() == now && run.game().supportVisuals().revision == revision &&
              hud.snapshot().portraitFeedback[0][0].startedMs == presentation.startedMs &&
              hud.snapshot().portraitFeedback[0][0].kind == presentation.kind,
          "Pause preserves clock and application feedback without replay or expiry");
    auto resumed = copy(c, d, run);
    const auto retainedWard = [&](const GameSession& game) {
        const auto state = game.effects().visualConditions();
        check(state.world == game.actors().snapshot().world &&
                  (state.party[0] & (1u << unsigned(EffectKind::ShieldOfFaith))),
              "Sustained ward reconstructs with the destination world's identity");
    };
    emptyJournals(resumed->game());
    retainedWard(resumed->game());
    CommandController continuedHud(resumed->game());
    continuedHud.refresh({}, false, false, true);
    check(continuedHud.snapshot().portraitFeedback[0][0].kind == PortraitFeedbackKind::None,
          "Continue retains status without replaying the old portrait flash");
    run.update(.7, false);
    hud.refresh({}, false, false, true);
    check(hud.snapshot().portraitFeedback[0][0].kind == PortraitFeedbackKind::None,
          "Resuming allows the application flash to expire normally");
    retainedWard(run.game());
    // Use the normal campaign doorway transaction, including a return to an old location.
    enterInn(*resumed);
    emptyJournals(resumed->game());
    retainedWard(resumed->game());
    leaveInn(*resumed);
    emptyJournals(resumed->game());
    retainedWard(resumed->game());
}
void chestRoundtrip() {
    const auto c = catalog(false, 50, 7);
    const auto d = definitions();
    auto run = std::make_unique<CampaignRun>(c, *c.preset(), d.types, d.timings, 5);
    travel(*run, FixtureKind::Downstairs);
    auto& game = run->game();
    const auto& layout = run->location(1).generated->layout;
    const auto chest = layout.caches[0];
    kill(game); // Persistence fixture: no live encounters on the return route.
    game.update(2, false);
    game.restorePose({chest.cell + forward(chest.facing) * -1, chest.facing}, false);
    check(bool(game.openContainer({game.loot().snapshot().world, 0})),
          "Opened supply chest in owned campaign fixture");
    game.move(Action::Forward);
    check(game.movement().player.cell == chest.cell &&
              bool(game.containerAccess({game.loot().snapshot().world, 0})),
          "Campaign party can stand on its chest tile and collect contents");
    const auto slot = std::uint16_t(NpcWorld::maxInstances * lootPerActor);
    check(bool(game.pickup(
              {{game.loot().snapshot().world, slot, 1}, {0}, game.party().inventory().revision()})),
          "Partially loot campaign chest");
    auto loaded = copy(c, d, *run);
    check(loaded->game().loot().snapshot().containers[0].opened &&
              loaded->game().movement().player.cell == chest.cell &&
              bool(loaded->game().containerAccess({loaded->game().loot().snapshot().world, 0})) &&
              !loaded->game().loot().snapshot().stacks[slot].active &&
              loaded->game().loot().snapshot().stacks[slot + 1].active,
          "Continue retains open state and exact remaining contents");
    travel(*loaded, FixtureKind::Upstairs);
    auto inactive = copy(c, d, *loaded);
    travel(*inactive, FixtureKind::Downstairs);
    check(inactive->game().loot().snapshot().containers[0].opened &&
              !inactive->game().loot().snapshot().stacks[slot].active,
          "Inactive floor chest survives save and revisit without reroll");
}
void progressionAndFloors() {
    const auto c = catalog(false, 50, 7);
    const auto d = definitions();
    auto run = std::make_unique<CampaignRun>(c, *c.preset(), d.types, d.timings, 5);
    run->party().grantNextLevelExperience();
    check(bool(run->game().advance(run->game().advancementRequest({0}))), "level-two warrior fixture");
    run = copy(c, d, *run);
    check(run->party().character({0})->build.level == 2, "restore level two without starter constructor");
    run->party().grantNextLevelExperience();
    check(bool(run->game().advance(run->game().advancementRequest({0}))), "level-three warrior fixture");
    run = copy(c, d, *run);
    check(run->party().character({0})->build.level == 3, "restore level three");
    for (unsigned depth = 1; depth <= 3; ++depth) {
        travel(*run, FixtureKind::Downstairs);
        run = copy(c, d, *run);
        auto& g = run->game();
        if (depth > 1) {
            for (unsigned i = 0; i < g.actors().snapshot().count; ++i)
                if (!g.guardian(g.actors().id(i)))
                    g.actors().damage(g.actors().id(i), 100000, g.timeMs());
            g.update(0, false);
            for (unsigned i = 0; i < g.fixtures().definition().count; ++i)
                if (g.fixtures().definition().entries[i].kind == FixtureKind::Key) {
                    walk(g, g.fixtures().definition().entries[i].cell);
                    check(g.interact({g.fixtures().id(i), g.fixtures().revision()}).accepted,
                          "key collected");
                    break;
                }
            run = copy(c, d, *run);
        }
        // Mutate a finite owned fixture, never a combat/balance experiment.
        if (depth < 3) {
            kill(run->game());
            run = copy(c, d, *run);
            check(run->game().objective().completed, "guardian ledger restored");
        }
    }
    const auto kits = run->party().campSupplies().kits;
    auto recall = run->prepareTravel(run->recallRequest());
    check(run->commitTravel(recall), "paid recall from third floor");
    run = copy(c, d, *run);
    check(run->party().campSupplies().kits == kits - 1, "recall charge persists");
    entrance(*run);
    auto waypoint = run->prepareTravel(run->waypointRequest(2));
    check(run->commitTravel(waypoint), "restored unlocked entry return");
    run = copy(c, d, *run);
    check(run->activeLocation() == 2, "revisited mutated floor restores");
}
void effects() {
    const auto c = catalog(false, 50, 7);
    const auto d = definitions();
    for (const auto kind :
         {EffectKind::Bless, EffectKind::Bane, EffectKind::ShieldOfFaith, EffectKind::MageArmor,
          EffectKind::HoldPerson, EffectKind::Guidance, EffectKind::Resistance, EffectKind::Sanctuary,
          EffectKind::Aid, EffectKind::MirrorImage, EffectKind::SpiritualWeapon, EffectKind::Web}) {
        CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
        EffectRequest e;
        e.world = run.game().actors().snapshot().world;
        e.kind = kind;
        e.caster = rules::CharacterId{0};
        e.count = 1;
        e.difficulty = 1000;
        const rules::CharacterId target{
            std::uint16_t(kind == EffectKind::MirrorImage || kind == EffectKind::Aid ||
                                  kind == EffectKind::Web || kind == EffectKind::SpiritualWeapon
                              ? 0
                              : 3)};
        e.targets[0] = target;
        check(bool(run.game().applyEffect(e)), "persistent effect fixture applies");
        if (kind == EffectKind::SpiritualWeapon)
            check(const_cast<EffectController&>(run.game().effects())
                      .moveWeapon({0}, run.game().movement().player.cell, true),
                  "place fixture spectral weapon");
        auto restored = copy(c, d, run);
        check(restored->game().effects().has(target, kind), "persistent instance retained");
        auto before = std::make_unique<FloorState>(), after = std::make_unique<FloorState>();
        for (unsigned i = 0; i < 80; ++i) {
            run.update(.0973, false);
            restored->update(.0973, false);
        }
        run.game().exportFloor(*before);
        restored->game().exportFloor(*after);
        check(before->digest() == after->digest(), "effect continuation including fractional clocks");
    }
    for (const auto kind : {EffectKind::Sleep, EffectKind::Blind, EffectKind::Slow, EffectKind::Knockdown}) {
        CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
        CombatEvents events;
        const_cast<EffectController&>(run.game().effects())
            .condition(rules::CharacterId{1}, kind, {0}, 0, events);
        auto restored = copy(c, d, run);
        check(restored->game().effects().has(rules::CharacterId{1}, kind), "condition retained");
    }
    CampaignRun local(c, *c.preset(), d.types, d.timings, 5);
    travel(local, FixtureKind::Downstairs);
    EffectRequest e;
    e.world = local.game().actors().snapshot().world;
    e.kind = EffectKind::Bane;
    e.caster = rules::CharacterId{0};
    e.count = 2;
    e.difficulty = 1000;
    e.targets[0] = rules::CharacterId{1};
    e.targets[1] = local.game().actors().id(0);
    check(bool(local.game().applyEffect(e)), "mixed concentration fixture applies");
    check(local.game().travelSafety() && !local.saveSafety(), "save in place does not require departure");
    auto restored = copy(c, d, local);
    check(restored->game().travelSafety(), "local concentration still blocks departure after load");
}
CampaignIoStage fault = CampaignIoStage::Read;
bool fail(CampaignIoStage stage) {
    return stage == fault;
}
void failures() {
    const auto c = catalog();
    const auto d = definitions();
    const CampaignContent content{"fixture", 0};
    Directory root;
    const auto file = root.path / "campaign.json";
    CampaignDisk disk(file);
    CampaignStore store(c, d, content, disk);
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    check(store.save(run, "missing").durable, "establish previous good file");
    const auto initial = store.inspect();
    for (const auto stage : {CampaignIoStage::Write, CampaignIoStage::Flush, CampaignIoStage::Close,
                             CampaignIoStage::Rename, CampaignIoStage::DirectoryFlush}) {
        fault = stage;
        CampaignDisk failed(file, {fail});
        CampaignStore broken(c, d, content, failed);
        run.party().damage({0}, 1);
        const auto current = store.inspect();
        auto result = broken.save(run, current.revision);
        check(!result.durable, "injected write failure reported");
        if (stage != CampaignIoStage::DirectoryFlush)
            check(!result.published && store.inspect().revision == current.revision,
                  "old file retained before rename");
        else
            check(result.published && store.inspect().revision != current.revision,
                  "post-rename uncertainty distinguished");
        check(store.load()->status() == CampaignStatus::Active, "surviving slot remains loadable");
    }
    rejects([&] { store.save(run, initial.revision); }, "stale slot rejected");
    for (unsigned i = 0; i < 4; ++i)
        run.party().damage({std::uint16_t(i)}, 100000);
    fault = CampaignIoStage::Rename;
    CampaignDisk failed(file, {fail});
    CampaignStore broken(c, d, content, failed);
    check(!broken.terminal(run).durable, "terminal marker failure reported");
    check(broken.inspect().terminal, "failed marker still denies in-process Continue");
    CampaignRun unsaved(c, *c.preset(), d.types, d.timings, 9);
    for (unsigned i = 0; i < 4; ++i)
        unsaved.party().damage({std::uint16_t(i)}, 100000);
    check(broken.terminal(unsaved).durable && broken.inspect().terminal,
          "an unsaved terminal run cannot forget the pending saved-run denial");
    rejects([&] { broken.load(); }, "failed terminal marker cannot resume in process");
    check(store.inspect().active, "old disk save remains honestly visible to another process");
    check(store.terminal(run).durable && store.inspect().terminal, "terminal retry closes saved run");
}
void malformed() {
    const auto c = catalog();
    const auto d = definitions();
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    auto state = run.checkpoint();
    const auto text = encodeCampaign(c, d, {"fixture", 0}, *state);
    const auto original = blobber::automation::Json::parse(text);
    const auto bad = [&](auto mutate) {
        auto j = original;
        mutate(j);
        rejects(
            [&] {
                auto s = decodeCampaign(c, d, {"fixture", 0}, j.dump());
                CampaignRun candidate(c, std::move(*s), d.timings);
            },
            "malformed state rejected transactionally");
    };
    bad([](auto& j) { j["schema"] = 1; });
    bad([](auto& j) { j["schema"] = 6; });
    bad([](auto& j) { savedEntry(savedLocation(j, 1)["layout"]["membership"], 0) = 0; });
    bad([](auto& j) { savedEntry(savedLocation(j, 1)["layout"]["paths"], 0) = 0; });
    bad([](auto& j) { savedLocation(j, 1)["layout"]["pathCount"] = 1601; });
    bad([](auto& j) { savedEntry(savedLocation(j, 1)["layout"]["connections"], 0)["facingA"] = 255; });
    bad([](auto& j) { savedLocation(j, 1)["layout"]["profile"] = 255; });
    bad([](auto& j) { j["campaign"]["gameplayMs"] = "18446744073709551615"; });
    bad([](auto& j) { j["catalog"] = "other"; });
    bad([](auto& j) { savedEntry(j["campaign"]["party"]["state"]["formation"], 1) = 0; });
    bad([](auto& j) {
        savedEntry(savedEntry(j["campaign"]["party"]["inventory"]["locations"], 0), 1) =
            savedEntry(savedEntry(j["campaign"]["party"]["inventory"]["locations"], 0), 0);
    });
    bad([](auto& j) { savedEntry(j["campaign"]["party"]["state"]["characters"], 0)["mana"] = 999; });
    bad([](auto& j) { j["campaign"]["pose"]["cell"]["x"] = -1; });
    bad([](auto& j) { savedLocation(j, 0)["state"]["value"]["combat"]["state"]["enemyCount"] = "999"; });
    bad([](auto& j) { savedLocation(j, 0)["state"]["value"]["rng"] = "1"; });
    bad([](auto& j) { j["campaign"]["navigation"]["tail"] = "1601"; });
    bad([](auto& j) { savedEntry(savedLocation(j, 1)["layout"]["population"], 0)["recipe"] = 255; });
    bad([](auto& j) { savedEntry(savedLocation(j, 1)["layout"]["encounters"], 0)["count"] = 31; });
    bad([](auto& j) { savedLocation(j, 1)["layout"]["routeCount"] = 33; });
    bad([](auto& j) { savedEntry(savedLocation(j, 1)["recovery"]["ground"], 0)["quantity"] = 19; });
    bad([](auto& j) { j["simulation"] = 7; });
    bad([](auto& j) { savedLocation(j, 1)["fixtures"]["npcExcluded"] = std::string(1600, '0'); });
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, std::string(campaignMaxBytes + 1, 'x')); },
            "oversized input rejected");
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, "{\"schema\":1,\"schema\":1}"); },
            "duplicate JSON rejected");
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, std::string(40, '[') + std::string(40, ']')); },
            "excess nesting rejected");
}
void saveUi() {
    const auto c = catalog();
    const auto d = definitions();
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    CommandController ui(run.game(), d.timings);
    ui.saveMenu().enabled = true;
    ui.refresh({}, false, false, true);
    ui.toggleMenu();
    UiInput down;
    down.pressed[std::size_t(UiKey::Down)] = true;
    routeHudInput(ui, down);
    UiInput confirm;
    confirm.pressed[std::size_t(UiKey::Confirm)] = true;
    routeHudInput(ui, confirm);
    check(ui.saveMenu().intent == SaveIntent::Save, "keyboard explicit Save intent");
    ui.saveMenu().intent = SaveIntent::None;
    UiInput click;
    click.pressed[std::size_t(UiKey::Click)] = true;
    click.pointer = ArtPoint{200, float(saveLayout::row(ui.saveMenu(), 2).y + 2)};
    routeHudInput(ui, click);
    check(ui.saveMenu().intent == SaveIntent::SaveMenu, "click Save and Menu intent");
    ui.saveMenu().intent = SaveIntent::None;
    ui.saveMenu().confirmation = SaveConfirmation::Overwrite;
    ui.saveMenu().cursor = 0;
    routeHudInput(ui, confirm);
    check(ui.saveMenu().intent == SaveIntent::None && !ui.saveMenu().replace, "overwrite defaults Cancel");
    ui.saveMenu().confirmation = SaveConfirmation::Overwrite;
    const auto replace = saveLayout::row(ui.saveMenu(), 1);
    click.pointer = ArtPoint{float(replace.x + 5), float(replace.y + 5)};
    routeHudInput(ui, click);
    check(ui.saveMenu().replace && ui.saveMenu().intent == SaveIntent::Save,
          "compact overwrite button uses the displayed hit area");
    ui.saveMenu().intent = SaveIntent::None;
    ui.saveMenu().replace = false;
    ui.saveMenu().terminalWarning = true;
    ui.saveMenu().cursor = 1;
    routeHudInput(ui, confirm);
    check(!ui.snapshot().menuRequested && ui.saveMenu().confirmation == SaveConfirmation::Leave,
          "terminal warning requires acknowledgement");
}

// Owned disk with observable I/O counts: pause frames must never read/restore a slot.
class CountedCampaignDisk final : public CampaignFiles {
  public:
    explicit CountedCampaignDisk(const std::filesystem::path& path) : disk_(path) {}
    CampaignIoReply exchange(const CampaignIoRequest& request) override {
        ioThread = std::this_thread::get_id();
        if (delayMs)
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        reads += request.operation == "read";
        writes += request.operation == "replace";
        return disk_.exchange(request);
    }
    unsigned reads = 0, writes = 0, delayMs = 0;
    std::thread::id ioThread;

  private:
    CampaignDisk disk_;
};
void pausePolling() {
    const auto c = catalog();
    const auto d = definitions();
    Directory directory;
    CountedCampaignDisk disk(directory.path / "campaign.json");
    CampaignStore store(c, d, {"fixture", 0}, disk);
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    const auto initial = store.save(run, "missing");
    check(initial.durable, "pause fixture owns an existing valid campaign save");
    CommandController commands(run.game(), d.timings);
    commands.saveMenu().enabled = true;
    commands.refresh({}, false, false, true);
    CampaignSaveFlow flow(&store);
    const auto reads = disk.reads, writes = disk.writes;
    const auto clock = run.gameplayMs();
    for (unsigned visit = 0; visit < 2; ++visit) {
        commands.toggleMenu();
        for (unsigned frame = 0; frame < 60; ++frame) {
            run.update(1.0 / 60, true);
            commands.refresh({}, false, false, true);
            check(!flow.poll(run, commands), "idle pause never leaves gameplay");
        }
        commands.closeOverlay();
        flow.poll(run, commands);
    }
    check(disk.reads == reads && disk.writes == writes && run.gameplayMs() == clock,
          "opening/reopening and 120 paused frames perform no slot I/O or clock advancement");
    commands.toggleMenu();
    commands.saveMenu().intent = SaveIntent::Save;
    bool left = false;
    unsigned pumps = 0;
    {
        PreparationWorker worker;
        PreparationScope saving(
            {[](void* p, const char*) { ++*static_cast<unsigned*>(p); }, &pumps, &worker});
        left = flow.poll(run, commands);
    }
    check(pumps && disk.ioThread != std::this_thread::get_id(),
          "save inspection/publication use the worker while the caller pumps presentation");
    check(!left && disk.reads > reads && disk.writes == writes + 1 &&
              std::string_view(commands.saveMenu().message.data()) == "Campaign saved.",
          "explicit Save still inspects and publishes the current run");
    CampaignRun other(c, *c.preset(), d.types, d.timings, 7);
    CommandController otherUi(other.game(), d.timings);
    otherUi.saveMenu().enabled = true;
    otherUi.refresh({}, false, false, true);
    otherUi.toggleMenu();
    CampaignSaveFlow overwrite(&store);
    overwrite.poll(other, otherUi);
    otherUi.saveMenu().intent = SaveIntent::Save;
    overwrite.poll(other, otherUi);
    check(otherUi.saveMenu().confirmation == SaveConfirmation::Overwrite,
          "different saved run still requires explicit overwrite review");
    const auto reviewed = store.inspect();
    run.party().damage({0}, 1);
    check(store.save(run, reviewed.revision).durable, "owned concurrent writer changes reviewed slot");
    const auto concurrentWrites = disk.writes;
    otherUi.saveMenu().replace = true;
    otherUi.saveMenu().intent = SaveIntent::Save;
    check(!overwrite.poll(other, otherUi) && disk.writes == concurrentWrites &&
              std::string_view(otherUi.saveMenu().message.data()) == "Couldn't save. Please try again.",
          "confirmed overwrite retains its reviewed revision and rejects concurrent changes");
    check(store.inspect().runId == run.runId(), "failed stale overwrite preserves the valid saved owner");
}

void applicationFlow() {
    const auto c = catalog();
    const auto d = definitions();
    Directory directory;
    CampaignDisk disk(directory.path / "campaign.json");
    CampaignStore store(c, d, {"fixture", 0}, disk);
    CampaignRun original(c, *c.preset(), d.types, d.timings, 5);
    CommandController commands(original.game(), d.timings);
    commands.saveMenu().enabled = true;
    commands.refresh({}, false, false, true);
    commands.toggleMenu();
    CampaignSaveFlow flow(&store);
    check(!flow.poll(original, commands), "opening pause does not save");
    commands.saveMenu().intent = SaveIntent::SaveMenu;
    check(flow.poll(original, commands) && store.inspect().active,
          "Save and Menu leaves only after durable publication");
    const auto oldRevision = store.inspect().revision;
    CampaignRun next(c, *c.preset(), d.types, d.timings, 7);
    CommandController nextUi(next.game(), d.timings);
    nextUi.saveMenu().enabled = true;
    nextUi.refresh({}, false, false, true);
    nextUi.toggleMenu();
    CampaignSaveFlow nextFlow(&store);
    nextFlow.poll(next, nextUi);
    nextUi.saveMenu().intent = SaveIntent::SaveMenu;
    check(!nextFlow.poll(next, nextUi) && nextUi.saveMenu().confirmation == SaveConfirmation::Overwrite &&
              store.inspect().revision == oldRevision,
          "new run preserves slot until reviewed save");
    UiInput confirm;
    confirm.pressed[std::size_t(UiKey::Confirm)] = true;
    routeHudInput(nextUi, confirm);
    check(nextUi.saveMenu().intent == SaveIntent::None, "cancel overwrite retains prior run");
    nextUi.saveMenu().intent = SaveIntent::SaveMenu;
    nextFlow.poll(next, nextUi);
    UiInput down;
    down.pressed[std::size_t(UiKey::Down)] = true;
    routeHudInput(nextUi, down);
    routeHudInput(nextUi, confirm);
    check(nextFlow.poll(next, nextUi) && store.inspect().runId == next.runId(),
          "explicit overwrite publishes new run");
    nextUi.closeOverlay();
    check(!nextFlow.closeRequested(next, nextUi) && nextUi.saveMenu().confirmation == SaveConfirmation::Quit,
          "native close also requires confirmation while active");
    UiInput cancel;
    cancel.pressed[std::size_t(UiKey::Cancel)] = true;
    routeHudInput(nextUi, cancel);
    nextUi.closeOverlay();
    next.game().move(Action::Back);
    check(next.saveSafety() != nullptr, "moving save denied");
    rejects([&] { next.checkpoint(); }, "moving export cannot mutate slot");
    next.game().update(.2, false);
    nextUi.refresh({}, false, false, true);
    nextUi.toggleMenu();
    UiInput blur = confirm;
    blur.focused = false;
    nextUi.saveMenu().cursor = 1;
    routeHudInput(nextUi, blur);
    check(nextUi.saveMenu().intent == SaveIntent::None, "blur suppresses save intent");
}

void unsavedTerminal() {
    const auto c = catalog();
    const auto d = definitions();
    Directory directory;
    const auto path = directory.path / "campaign.json";
    CampaignDisk disk(path);
    CampaignStore store(c, d, {"fixture", 0}, disk);
    CampaignRun unsaved(c, *c.preset(), d.types, d.timings, 7);
    for (unsigned i = 0; i < 4; ++i)
        unsaved.party().damage({std::uint16_t(i)}, 100000);
    const auto missing = store.terminal(unsaved);
    check(missing.durable && !missing.published && !std::filesystem::exists(path),
          "unsaved ending needs no marker when no slot exists");
    CampaignRun previous(c, *c.preset(), d.types, d.timings, 5);
    auto state = previous.checkpoint();
    auto old = blobber::automation::Json::parse(encodeCampaign(c, d, {"fixture", 0}, *state));
    // Exactly the pre-refresh versions: identity is readable but restore is forbidden.
    old["schema"] = 1;
    old["simulation"] = 1;
    old["generator"] = 3;
    old["templates"] = 3;
    const auto original = old.dump();
    {
        std::ofstream output(path);
        output << original;
    }
    check(!store.inspect().active, "previous-version slot is incompatible");
    CommandController commands(unsaved.game(), d.timings);
    commands.saveMenu().enabled = true;
    CampaignSaveFlow flow(&store);
    check(!flow.poll(unsaved, commands) && !commands.saveMenu().terminalWarning &&
              !commands.snapshot().menuOpen,
          "unsaved expedition ending over incompatible slot does not show write failure");
    std::ifstream input(path);
    const std::string retained((std::istreambuf_iterator<char>(input)), {});
    check(retained == original && !store.inspect().active,
          "incompatible previous save remains byte-identical and unavailable");
    // Identity parsing has the same hostile-input limits as normal restoration.
    auto duplicate = original;
    duplicate.insert(1, "\"schema\":1,");
    rejects([&] { savedCampaignRunId(duplicate); }, "ambiguous slot association rejects duplicate keys");
    old["campaign"]["runId"] = "invalid";
    rejects([&] { savedCampaignRunId(old.dump()); }, "invalid slot association rejects");
    // A matching association still closes the slot even if content admission changed.
    old["campaign"]["runId"] = unsaved.runId();
    {
        std::ofstream output(path);
        output << old.dump();
    }
    const auto matching = store.terminal(unsaved);
    check(matching.durable && matching.published && store.inspect().terminal,
          "matching old slot is closed using identity without restoring its simulation");
}

void libraryIndependence() {
    const auto c = catalog();
    const auto d = definitions();
    Directory directory;
    CharacterLibrary library(c, directory.path / "characters.json");
    check(bool(library.initialize()), "owned library fixture initializes");
    std::array<SavedCharacterId, 4> ids;
    for (unsigned i = 0; i < 4; ++i)
        ids[i] = library.snapshot().records[i].id;
    const auto starting = library.startingParty(ids, {{{0}, {1}, {2}, {3}}});
    CampaignRun run(c, starting, d.types, d.timings, 5);
    CampaignDisk disk(directory.path / "campaign.json");
    CampaignStore store(c, d, {"fixture", 0}, disk);
    check(store.save(run, "missing").durable, "library-derived party saved");
    for (auto id : ids)
        check(bool(library.erase(id, library.snapshot().revision)),
              "starting blueprint deleted independently");
    auto loaded = store.load();
    check(loaded->party().character({0})->build.id == starting.characters[0].id &&
              loaded->party().character({0})->build.name == starting.characters[0].name,
          "save restores identities after library deletion");
    CampaignRun abandoned(c, *c.preset(), d.types, d.timings, 7);
    for (unsigned i = 0; i < 4; ++i)
        abandoned.party().damage({std::uint16_t(i)}, 100000);
    const auto revision = store.inspect().revision;
    const auto ended = store.terminal(abandoned);
    check(ended.durable && !ended.published && store.inspect().revision == revision && store.inspect().active,
          "unsaved different run cannot invalidate prior active save");
}

void postCombatSave() {
    const auto c = catalog(false, 50, 5);
    const auto d = definitions();
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    travel(run, FixtureKind::Downstairs);
    auto& g = run.game();
    const auto enemy = g.actors().snapshot().actors[0].npc.cell;
    bool placed = false;
    for (unsigned h = 0; h < 4 && !placed; ++h) {
        const auto cell = enemy + forward(Heading(h));
        if (!g.geometry().solid(cell) && !g.actors().occupied(cell)) {
            g.restorePose({cell, Heading((h + 2) % 4)}, false);
            placed = true;
        }
    }
    check(placed, "owned combat fixture has an adjacent approach");
    g.update(.1, false);
    check(run.saveSafety(), "active encounter cannot save");
    for (unsigned tick = 0; tick < 10000 && g.combat().snapshot().phase != CombatPhase::Victory; ++tick) {
        const auto& state = g.combat().snapshot();
        if (state.awaitingAction && state.activeCharacter) {
            PartyCommand command;
            command.world = state.world;
            command.turn = state.turn;
            command.character = *state.activeCharacter;
            bool attack = false;
            for (unsigned i = 0; i < g.actors().snapshot().count && !attack; ++i) {
                command.action = PartyAction::Melee;
                command.target = g.actors().id(i);
                if (g.combat().validate(command, g.movement().player) == CommandReason::Ready)
                    attack = true;
            }
            if (!attack) {
                command.action = PartyAction::Defend;
                command.target.reset();
            }
            check(g.submit(command).reason == CommandReason::Resolved, "owned combat uses checked commands");
        }
        g.update(.1, false);
    }
    check(g.combat().snapshot().phase == CombatPhase::Victory, "owned encounter reaches victory");
    const auto hp = g.party().character({0})->hitPoints;
    check(continueLocalVictory(g, false) && realTime(g.combat().snapshot().phase),
          "application returns to exploration without acknowledging ordinary victory");
    check(g.party().character({0})->hitPoints == hp && !continueLocalVictory(g, false),
          "automatic continuation is once-only and does not heal");
    g.update(2, false);
    check(!run.saveSafety(), "settled exploration after combat can save");
    const auto xp = run.party().character({0})->experience;
    auto resumed = copy(c, d, run);
    check(resumed->party().character({0})->experience == xp,
          "fought encounter XP retained without duplication");
}

void groundAndInactive() {
    const auto c = catalog(false, 50, 7);
    const auto d = definitions();
    auto run = std::make_unique<CampaignRun>(c, *c.preset(), d.types, d.timings, 5);
    travel(*run, FixtureKind::Downstairs);
    auto& game = run->game();
    auto& effects = const_cast<EffectController&>(game.effects());
    CombatEvents events;
    const auto grease = game.actors().snapshot().actors[0].npc.cell;
    const auto web = game.actors().actor(game.objective().members[0])->npc.cell;
    check(!(web == grease), "owned ground-effect fixtures use distinct cells");
    check(bool(effects.createArea(grease, {0}, 10, 0, events).serial), "owned Grease area created");
    check(effects.createWeb(&web, 1, {2}, 1000, 0, events), "owned Web area/restraint created");
    effects.ignite(SubcellFootprint{web * 2, 1});
    effects.condition(game.actors().id(0), EffectKind::Sleep, {0}, 0, events);
    effects.condition(rules::CharacterId{1}, EffectKind::Blind, {0}, 0, events);
    effects.guide(game.actors().id(0), 0, events);
    run = copy(c, d, *run);
    check(run->game().effects().area(grease) && run->game().effects().area(web),
          "nonempty ground effects retain logical references");
    bool restoredBurn = false;
    for (const auto& a : run->game().effects().ground().areas)
        if (a.active && a.web)
            restoredBurn = a.footprint == wholeCellFootprint(web) && a.burnDue[0] == 6000 && a.burnMs == 6000;
    check(restoredBurn, "Precise Web mask and partial burning deadlines survive campaign roundtrip");
    check(bool(run->game().endConcentration(run->game().actors().snapshot().world, {2})),
          "local Web explicitly dismissed before departure");
    travel(*run, FixtureKind::Upstairs);
    enterInn(*run);
    RestRequest rest;
    rest.kind = RestKind::Long;
    rest.world = run->game().actors().snapshot().world;
    rest.token = run->game().restToken();
    run->party().damage({0}, 1);
    check(bool(run->game().rest(rest)), "inn rest advances only portable campaign effects");
    run = copy(c, d, *run);
    leaveInn(*run);
    travel(*run, FixtureKind::Downstairs);
    check(run->game().effects().area(grease) && run->game().effects().guiding(run->game().actors().id(0)) &&
              run->game().effects().has(run->game().actors().id(0), EffectKind::Sleep) &&
              !run->game().effects().has(rules::CharacterId{1}, EffectKind::Blind),
          "restored inactive local clocks freeze while party clocks advance");
    run = copy(c, d, *run);
}

void npcSourceTravel() {
    const auto c = catalog(false, 50, 7, false, true);
    const auto d = definitions();
    auto run = std::make_unique<CampaignRun>(c, *c.preset(), d.types, d.timings, 5);
    travel(*run, FixtureKind::Downstairs);
    const auto caster = run->game().actors().id(0);
    EffectRequest request;
    request.world = caster.world;
    request.caster = caster;
    request.targets[0] = caster;
    request.count = 1;
    check(bool(run->game().applyEffect(request)), "Owned campaign fixture applies NPC Bless");
    auto floor = std::make_unique<FloorState>();
    run->game().exportFloor(*floor);
    floor->combat.enemyMana[0] = 2;
    floor->combat.enemyTarget[0] = 3;
    run->game().importFloor(*floor);
    run = copy(c, d, *run);
    const auto rebound = run->game().actors().id(0);
    check(run->game().effects().concentrationSource(rebound).serial &&
              !run->game().effects().concentrationSource(caster).serial &&
              run->game().combat().enemyMana(rebound) == 2 && run->game().combat().enemyTarget(rebound) == 3,
          "Disk roundtrip rebinds NPC caster and retains spent mana/target cursor");
    travel(*run, FixtureKind::Upstairs);
    for (const auto& e : run->game().effects().snapshot())
        check(!e.active || !e.caster.npc, "NPC effects never follow party to hub");
    RestRequest rest;
    rest.kind = RestKind::Long;
    rest.world = run->game().actors().snapshot().world;
    rest.token = run->game().restToken();
    run->party().damage({0}, 1);
    check(bool(run->game().rest(rest)), "Party can rest with local NPC concentration on inactive floor");
    run = copy(c, d, *run);
    travel(*run, FixtureKind::Downstairs);
    const auto returned = run->game().actors().id(0);
    check(run->game().effects().has(returned, EffectKind::Bless) &&
              run->game().combat().enemyMana(returned) == 2 &&
              run->game().combat().enemyTarget(returned) == 3,
          "NPC buff clock/resources stay frozen on their location through hub rest and save");
}

class TestVisibility final : public blobber::automation::Visibility {
    TargetSnapshot targets_;

  public:
    const TargetSnapshot& sample(const Map&, const GameSession& game) override {
        targets_ = {};
        targets_.world = game.actors().snapshot().world;
        targets_.revision = game.actors().snapshot().revision;
        return targets_;
    }
    bool lootVisible(LootId) const noexcept override { return false; }
};
std::unique_ptr<blobber::automation::Visibility> prepareVisibility(void* context, const Map&,
                                                                   const GameSession&) {
    if (*static_cast<bool*>(context))
        throw std::bad_alloc();
    return std::make_unique<TestVisibility>();
}
void protocolLoadRollback() {
    const auto c = catalog();
    const auto d = definitions();
    Directory directory;
    CampaignDisk disk(directory.path / "campaign.json");
    CampaignStore store(c, d, {"fixture", 0}, disk);
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    check(store.save(run, "missing").durable, "protocol rollback establishes slot");
    bool fail = false;
    blobber::automation::Protocol protocol(c, d, blobber::automation::Mode::Visual, prepareVisibility, &fail,
                                           &store);
    Sink sink;
    auto loaded = protocol.request({{"version", 1}, {"id", "first"}, {"op", "load"}}, sink);
    check(loaded["ok"], "first load prepares visibility");
    const auto before = protocol.state();
    const auto* owner = protocol.session();
    fail = true;
    const auto rejected = protocol.request({{"version", 1},
                                            {"id", "replace"},
                                            {"op", "load"},
                                            {"replace", true},
                                            {"session", before["session"]},
                                            {"revision", before["revision"]}},
                                           sink);
    check(!rejected["ok"].get<bool>() && rejected["error"]["code"] == "preparation_failed" &&
              protocol.session() == owner,
          "load visibility allocation failure is a recoverable response");
    check(protocol.state() == before, "failed load retains exact source and tokens");
}

void navigationContinuation() {
    const auto c = catalog();
    const auto d = definitions();
    std::vector<std::string> rows(40, std::string(40, '.'));
    rows.front() = rows.back() = std::string(40, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    Map map(rows);
    std::vector<NpcInstance> instances;
    for (unsigned i = 0; i < 8; ++i)
        instances.push_back({{0}, {int(i) + 1, 1}, {0, 0}, Heading::South});
    NpcWorld placements(map, {2, 2}, d.types, std::move(instances));
    ActorWorld actors(map, c, placements);
    Navigation original(map, actors), resumed(map, actors);
    for (unsigned i = 0; i < 8; ++i)
        original.request(i, {int(i) + 1, 1}, {38, 38}, false);
    original.update({2, 2});
    auto checkpoint = original.checkpoint();
    check(checkpoint.work == Navigation::nodeBudget && checkpoint.owner < 8 &&
              checkpoint.head < checkpoint.tail,
          "fixture retains a partially processed navigation search");
    CampaignRun transport(c, *c.preset(), d.types, d.timings, 5);
    auto payload = transport.checkpoint();
    payload->navigation = checkpoint;
    const auto text = encodeCampaign(c, d, {"fixture", 0}, *payload);
    auto decoded = decodeCampaign(c, d, {"fixture", 0}, text);
    // Validate transported navigation against the 40x40 fixture that owns it.
    resumed.restore(decoded->navigation);
    for (unsigned tick = 0; tick < 10; ++tick) {
        for (unsigned i = 0; i < 8; ++i) {
            const auto a = original.take(i), b = resumed.take(i);
            check(a.ready == b.ready && a.found == b.found && a.step == b.step,
                  "restored navigation yields matching results on matching ticks");
        }
        original.update({2, 2});
        resumed.update({2, 2});
        check(original.lastWork() == resumed.lastWork() &&
                  original.lastCompleted() == resumed.lastCompleted(),
              "navigation work scheduling continues exactly");
    }
    checkpoint.seen[0] = 1;
    rejects([&] { resumed.restore(checkpoint); }, "inconsistent search workspace rejected");
}

void sparsePersistence() {
    using Json = blobber::automation::Json;
    const auto c = catalog();
    const auto d = definitions();
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    auto capture = run.captureSave();
    const auto text = encodeCampaign(c, d, {"fixture", 0}, *capture);
    auto json = Json::parse(text);
    check(text.size() < 64 * 1024, "Fresh campaign is small without compression");
    check(json["campaign"]["locations"].size() == 1,
          "Unvisited locations have no snapshots or copied definitions");
    auto& city = savedLocation(json, 0);
    check(city.size() == 2 && city.contains("visited") && city.contains("state"),
          "Unchanged map, fixtures, population and generator data inherit baseline");
    auto& state = city["state"]["value"];
    check(!state.contains("actors"), "Unchanged actors inherit their initial baseline");
    check(!state.contains("effects"), "Empty effect/condition/area pools write no records");
    check(!state["loot"]["state"].contains("stacks"), "Empty loot pool writes no records");
    check(state["ai"]["state"]["actors"].size() == run.game().ai().count, "AI arrays use actor count");
    check(json["campaign"]["party"]["inventory"]["stacks"].size() < inventoryCapacity,
          "Inventory capacity does not become empty JSON stacks");

    EffectRequest ward;
    ward.world = run.game().actors().snapshot().world;
    ward.kind = EffectKind::ShieldOfFaith;
    ward.caster = rules::CharacterId{1};
    ward.targets[0] = rules::CharacterId{0};
    ward.count = 1;
    check(bool(run.game().applyEffect(ward)), "Sparse effect fixture applies a live ward");
    auto checkpoint = run.checkpoint();
    auto& floor = *checkpoint->locations[0].state;
    const auto item = checkpoint->party.inventory.stacks[0].item;
    check(checkpoint->party.inventory.stacks[0].quantity != 0, "Owned inventory source exists");
    constexpr auto last = inventoryCapacity - 1;
    checkpoint->party.inventory.stacks[last] = checkpoint->party.inventory.stacks[0];
    checkpoint->party.inventory.stacks[last].generation = 7;
    checkpoint->party.inventory.stacks[0].quantity = 0;
    checkpoint->party.inventory.stacks[0].generation = UINT64_MAX;
    for (auto& row : checkpoint->party.inventory.locations)
        for (auto& slot : row)
            if (slot == 0)
                slot = last;
    floor.loot.state.stacks.back() = {checkpoint->pose.cell, item, 1, true, 17};
    // Dormant payloads must not leak onto disk. Keep generation/container metadata.
    floor.loot.state.stacks[lootReservedCapacity] = {{9, 9}, item, 0, false, 21};
    floor.effects.instances.back() = floor.effects.instances[0];
    floor.effects.instances[0] = {};
    floor.effects.instances[floor.effects.instances.size() - 2].id.serial = 1234; // Inactive old effect.
    floor.effects.conditions.back().back().untilMs = 1234;
    floor.effects.ground.areas.back().untilMs = 1234;
    checkpoint->dialogue.complete(dialogueKey("fixture.resident"), dialogueKey("fixture.topic"),
                                  dialogueKey("fixture.fact"));
    checkpoint->dialogue.records.back().id = 999; // Outside count, not meaningful.
    checkpoint->dialogue.records[0].completed.back() = 999;
    MerchantState merchant;
    merchant.resident = 10;
    merchant.shop = 20;
    merchant.restockMs = 1000;
    merchant.stock[item.index] = 1;
    merchant.buyback.back() = {item, 1, 10, 1000};
    checkpoint->merchants.push_back(merchant);
    auto sparse = Json::parse(encodeCampaign(c, d, {"fixture", 0}, *checkpoint));
    auto& saved = savedLocation(sparse, 0)["state"]["value"];
    check(saved["effects"]["instances"].size() == 1 && !saved["effects"].contains("conditions") &&
              !saved["effects"].contains("ground"),
          "High active effect slot survives while expired effects/conditions/areas are omitted");
    check(saved["loot"]["state"]["stacks"].size() == 2,
          "High loot slot and recycled empty metadata are sparse records");
    check(savedEntry(saved["loot"]["state"]["stacks"], lootReservedCapacity).size() == 1,
          "Empty recycled loot retains generation without dead item/cell payload");
    check(sparse["campaign"]["dialogue"]["records"].size() == 1 &&
              savedEntry(sparse["campaign"]["dialogue"]["records"], 0)["completed"].size() == 1,
          "Dialogue record and nested topic capacities are omitted");
    check(savedEntry(sparse["campaign"]["merchants"]["entries"], 0)["buyback"].size() == 1,
          "Merchant buyback saves only its occupied slot");
    auto decoded = decodeCampaign(c, d, {"fixture", 0}, sparse.dump());
    CampaignRun restored(c, std::move(*decoded), d.timings);
    auto after = restored.checkpoint();
    check(after->party.inventory.stacks[last].generation == 7 &&
              after->party.inventory.stacks[0].generation == UINT64_MAX &&
              after->locations[0].state->loot.state.stacks.back().generation == 17 &&
              after->locations[0].state->loot.state.stacks[lootReservedCapacity].generation == 21 &&
              after->locations[0].state->effects.instances.back().active &&
              !after->locations[0].state->effects.instances[0].active,
          "Sparse restore preserves high slots, recycled generations and retired slots");
    check(encodeCampaign(c, d, {"fixture", 0}, *after) == sparse.dump() + "\n",
          "Discarding dormant payloads preserves the canonical save");

    const auto reject = [&](Json invalidJson) {
        rejects(
            [&] {
                auto decodedBad = decodeCampaign(c, d, {"fixture", 0}, invalidJson.dump());
                CampaignRun invalid(c, std::move(*decodedBad), d.timings);
            },
            "Malformed sparse values reject before publication");
    };
    auto bad = sparse;
    auto& slots = bad["campaign"]["party"]["inventory"]["stacks"];
    slots.push_back(slots[0]);
    reject(bad);
    bad = sparse;
    bad["campaign"]["party"]["inventory"]["stacks"][0][0] = inventoryCapacity;
    reject(bad);
    bad = sparse;
    savedLocation(bad, 0)["state"]["value"]["actors"]["count"] = "257";
    reject(bad);
    bad = sparse;
    bad["baseline"][0] = "1";
    reject(bad);
    bad = sparse;
    bad["summary"]["party"][0] = "Different name";
    reject(bad);
    bad = sparse;
    savedLocation(bad, 0)["state"]["value"]["effects"]["conditions"] =
        Json::array({Json::array({259, Json::array({Json::array({0, {{"untilMs", "1"}}})})})});
    reject(bad); // Dormant conditions cannot smuggle fields into skipped payloads.

    Directory directory;
    CampaignDisk disk(directory.path / "campaign.json");
    CampaignStore store(c, d, {"fixture", 0}, disk);
    unsigned constructionPoints = 0;
    PreparationScope scope({[](void* owner, const char* phase) {
                                const std::string_view label(phase);
                                if (label == "Generating dungeon floors" ||
                                    label == "Restoring campaign locations" ||
                                    label == "Preparing campaign session")
                                    ++*static_cast<unsigned*>(owner);
                            },
                            &constructionPoints, nullptr});
    check(store.save(run, "missing").durable && store.inspect().active,
          "Save and metadata inspection succeed");
    check(store.save(run, store.inspect().revision).durable && !constructionPoints,
          "Save/Inspect never generate floors or reconstruct campaign/session owners");
    // A structurally readable slot can have a semantic error. Inspection is metadata;
    // only explicit Load performs full admission, without losing the current slot.
    auto corrupt = Json::parse(text);
    savedEntry(corrupt["campaign"]["party"]["state"]["characters"], 0)["mana"] = 999;
    std::ofstream(directory.path / "campaign.json") << corrupt.dump();
    check(store.inspect().active && !constructionPoints,
          "Metadata inspection does not reconstruct owners to preflight Continue");
    rejects([&] { store.load(); }, "Explicit Load rejects semantically invalid state");
}

std::unique_ptr<CampaignRun> representativeCampaign(const rules::Catalog& c, const NpcDefinitions& d) {
    auto run = std::make_unique<CampaignRun>(c, *c.preset(), d.types, d.timings, 5);
    const auto position = [&](Player pose) {
        auto checkpoint = run->checkpoint();
        checkpoint->pose = pose;
        checkpoint->animated = false;
        run = std::make_unique<CampaignRun>(c, std::move(*checkpoint), d.timings);
        run->game().discover();
    };
    for (const auto* name : {"inn", "smithy", "emporium"}) {
        position(sanctuaryCity().doorway(name).approach);
        auto enter = run->prepareTravel(run->doorwayRequest());
        check(run->commitTravel(enter), "Measurement enters interior");
        position(run->location(run->activeLocation()).exit);
        auto leave = run->prepareTravel(run->doorwayRequest());
        check(run->commitTravel(leave), "Measurement leaves interior");
    }
    for (unsigned depth = 0; depth < 3; ++depth) {
        if (depth)
            kill(run->game());
        const auto request = stair(run->game(), FixtureKind::Downstairs);
        const auto fixture = *run->game().fixtures().find(request.id);
        position({stairLanding(fixture), fixture.facing});
        auto ticket = run->prepareTravel(run->request(stair(run->game(), FixtureKind::Downstairs)));
        check(run->commitTravel(ticket), "Measurement descends");
    }
    return run;
}
void terminalWorker() {
    const auto c = catalog();
    const auto d = definitions();
    for (bool victory : {false, true}) {
        Directory directory;
        CountedCampaignDisk disk(directory.path / "campaign.json");
        CampaignStore store(c, d, {"fixture", 0}, disk);
        auto run = victory ? representativeCampaign(c, d)
                           : std::make_unique<CampaignRun>(c, *c.preset(), d.types, d.timings, 5);
        check(store.save(*run, "missing").durable, "Terminal fixture owns a saved run");
        if (victory)
            kill(run->game());
        else
            for (unsigned i = 0; i < rules::partySize; ++i)
                run->party().damage({std::uint16_t(i)}, 100000);
        if (victory) {
            check(run->status() == CampaignStatus::Active && run->game().objective().completed,
                  "Final guardian completion remains active");
            CommandController commands(run->game(), d.timings);
            commands.refresh({}, false, false, true);
            CampaignSaveFlow flow(&store);
            check(!flow.terminalPending(*run, commands), "Completed content does not publish terminal state");
            check(store.save(*run, store.inspect().revision).durable, "Save after final guardian");
            auto restored = store.load();
            check(restored->status() == CampaignStatus::Active && restored->game().objective().completed,
                  "Continue restores completed final floor as active");
            continue;
        }
        check(run->status() == CampaignStatus::Defeat, "Defeat remains terminal");
        CommandController commands(run->game(), d.timings);
        commands.refresh({}, false, false, true);
        CampaignSaveFlow flow(&store);
        CampaignSaveFlow disabled(nullptr);
        check(!disabled.terminalPending(*run, commands),
              "Disabled persistence never starts a terminal presentation job");
        const auto beforeReads = disk.reads;
        check(flow.terminalPending(*run, commands) && !flow.closeRequested(*run, commands) &&
                  disk.reads == beforeReads,
              "Native close defers terminal I/O until the result can be presented");
        const auto clock = run->gameplayMs();
        disk.delayMs = 20;
        unsigned pumps = 0;
        PreparationWorker worker;
        {
            PreparationScope scope(
                {[](void* owner, const char*) { ++*static_cast<unsigned*>(owner); }, &pumps, &worker});
            check(!flow.poll(*run, commands), "Terminal publication does not navigate away");
        }
        check(pumps > 1 && disk.ioThread != std::this_thread::get_id() &&
                  !commands.saveMenu().terminalWarning && run->gameplayMs() == clock,
              "Victory/defeat file work runs on the worker while the frozen result pumps");
        disk.delayMs = 0;
        check(store.inspect().terminal && !flow.terminalPending(*run, commands) &&
                  flow.closeRequested(*run, commands),
              "Durable terminal completion denies Continue and releases deferred close");
    }
}
void measureSave(const std::filesystem::path& path) {
    const auto c = catalog();
    const auto d = definitions();
    auto run = representativeCampaign(c, d);
    using Clock = std::chrono::steady_clock;
    const auto ms = [](auto from, auto to) {
        return std::chrono::duration<double, std::milli>(to - from).count();
    };
    const auto start = Clock::now();
    auto snapshot = run->captureSave();
    const auto captured = Clock::now();
    auto text = encodeCampaign(c, d, {"fixture", 0}, *snapshot);
    const auto encoded = Clock::now();
    // The same fixture encoded by schema 19 occupied 5,831,821 bytes.
    check(text.size() < 583182 && text.size() < 256 * 1024,
          "Representative save is at least 90% smaller and below 256 KiB");
    CampaignDisk disk(path);
    CampaignStore store(c, d, {"fixture", 0}, disk);
    const auto before = store.inspect();
    const auto writing = Clock::now();
    check(disk.exchange({"replace", text, before.revision}).durable, "Measurement writes atomically");
    const auto saving = Clock::now();
    check(store.save(*run, campaignRevision(text), true).durable, "Measurement saves safely");
    const auto saved = Clock::now();
    check(store.save(*run, store.inspect().revision).durable, "Measurement overwrites same run");
    const auto overwritten = Clock::now();
    std::cout << "bytes=" << text.size() << " capture_ms=" << ms(start, captured)
              << " encode_ms=" << ms(captured, encoded) << " write_flush_ms=" << ms(writing, saving)
              << " save_ms=" << ms(saving, saved) << " inspect_and_overwrite_ms=" << ms(saved, overwritten)
              << '\n';
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string(argv[1]) == "--measure") {
            measureSave(argv[2]);
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--smoke") {
            terminalWorker();
            sparsePersistence();
            magicRewardPersistence();
            ignoredCampaigns();
            environmentRoundtrip();
            roundtrip();
            saveUi();
            applicationFlow();
            pausePolling();
            spellPresentationLifecycle();
            std::cout << "campaign save smoke tests passed\n";
            return 0;
        }
        if (argc != 1)
            throw std::invalid_argument("Expected no arguments or --smoke");
        terminalWorker();
        sparsePersistence();
        magicRewardPersistence();
        ignoredCampaigns();
        environmentRoundtrip();
        roundtrip();
        std::cerr << "progression/floors\n";
        progressionAndFloors();
        std::cerr << "chest containers\n";
        chestRoundtrip();
        spellPresentationLifecycle();
        std::cerr << "effects\n";
        effects();
        std::cerr << "I/O failures\n";
        failures();
        std::cerr << "malformed\n";
        malformed();
        saveUi();
        applicationFlow();
        pausePolling();
        unsavedTerminal();
        navigationContinuation();
        libraryIndependence();
        postCombatSave();
        groundAndInactive();
        npcSourceTravel();
        protocolLoadRollback();
        std::cout << "campaign save tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
