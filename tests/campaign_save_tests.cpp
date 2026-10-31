#include "app/campaign_saves.hpp"
#include "persistence/campaign_store.hpp"
#include "persistence/character_library.hpp"
#include "support/campaign_fixtures.hpp"
#include "ui/interaction.hpp"
#include "ui/save_layout.hpp"
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
void environmentRoundtrip() {
    const auto c = catalog();
    const auto d = definitions();
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    auto state = run.checkpoint();
    auto& world = state->locations[0].world;
    world.surfaces[2 * 9 + 3].floor = 1;
    world.surfaces[2 * 9 + 3].ceiling = noCeiling;
    state->locations[0].worldFingerprint = world.fingerprint();
    const auto fingerprint = world.fingerprint();
    const auto text = encodeCampaign(c, d, {"fixture", 0}, *state);
    auto decoded = decodeCampaign(c, d, {"fixture", 0}, text);
    CampaignRun loaded(c, std::move(*decoded), d.timings);
    check(loaded.map().definition().fingerprint() == fingerprint,
          "Nonflat world and decorations survive Continue");
    auto travel = loaded.prepareTravel(loaded.request(loaded.game().nearbyFixture()));
    check(loaded.commitTravel(travel), "Travel from height-bearing hub");
    check(loaded.location(0).map().definition().fingerprint() == fingerprint,
          "Inactive location retains environment");
    auto away = loaded.checkpoint();
    const auto text2 = encodeCampaign(c, d, {"fixture", 0}, *away);
    auto back = decodeCampaign(c, d, {"fixture", 0}, text2);
    CampaignRun resumed(c, std::move(*back), d.timings);
    check(resumed.location(0).map().definition().fingerprint() == fingerprint,
          "Inactive environment survives disk roundtrip");
    auto bad = blobber::automation::Json::parse(text);
    bad["campaign"]["locations"][0]["world"]["surfaces"][20]["floor"] = 99;
    rejects(
        [&] {
            auto v = decodeCampaign(c, d, {"fixture", 0}, bad.dump());
            CampaignRun invalid(c, std::move(*v), d.timings);
        },
        "Malformed surface retains prior session");
    bad = blobber::automation::Json::parse(text);
    bad["simulation"] = 1;
    rejects([&] { decodeCampaign(c, d, {"fixture", 0}, bad.dump()); }, "Old simulation rejects explicitly");
}
void roundtrip() {
    const auto c = catalog();
    const auto d = definitions();
    CampaignRun run(c, *c.preset(), d.types, d.timings, 5);
    const CampaignContent content{"owned-fixture", 0};
    auto before = run.checkpoint();
    const auto text = encodeCampaign(c, d, content, *before);
    auto decoded = decodeCampaign(c, d, content, text);
    CampaignRun loaded(c, std::move(*decoded), d.timings);
    auto after = loaded.checkpoint();
    check(encodeCampaign(c, d, content, *after) == text, "hub exact canonical round trip");
    check(loaded.party().inventory().identity() != run.party().inventory().identity(),
          "fresh inventory epoch");
    auto travel = run.prepareTravel(run.request(run.game().nearbyFixture()));
    check(run.commitTravel(travel), "enter dungeon");
    run.party().damage({0}, 2);
    auto floor = run.checkpoint();
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
        e.caster = {0};
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
            check(const_cast<EffectController&>(run.game().effects()).moveWeapon({0}, {4, 4}, true),
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
    e.caster = {0};
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
    bad([](auto& j) { j["campaign"]["gameplayMs"] = "18446744073709551615"; });
    bad([](auto& j) { j["catalog"] = "other"; });
    bad([](auto& j) {
        j["campaign"]["party"]["state"]["formation"][1] = j["campaign"]["party"]["state"]["formation"][0];
    });
    bad([](auto& j) {
        j["campaign"]["party"]["inventory"]["locations"][0][1] =
            j["campaign"]["party"]["inventory"]["locations"][0][0];
    });
    bad([](auto& j) { j["campaign"]["party"]["state"]["characters"][0]["mana"] = 999; });
    bad([](auto& j) { j["campaign"]["pose"]["cell"]["x"] = -1; });
    bad([](auto& j) { j["campaign"]["locations"][0]["state"]["combat"]["state"]["enemyCount"] = "999"; });
    bad([](auto& j) { j["campaign"]["locations"][0]["state"]["rng"] = "1"; });
    bad([](auto& j) { j["campaign"]["navigation"]["tail"] = "1601"; });
    bad([](auto& j) { j["campaign"]["locations"][1]["layout"]["population"][0]["recipe"] = 255; });
    bad([](auto& j) { j["campaign"]["locations"][1]["fixtures"]["npcExcluded"] = std::string(1600, '0'); });
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
    next.game().move(Action::Forward);
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
    check(g.combat().continueExploring(), "acknowledge ordinary encounter victory");
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
    const auto web = game.actors().snapshot().actors[1].npc.cell;
    check(bool(effects.createArea(grease, {0}, 10, 0, events).serial), "owned Grease area created");
    check(effects.createWeb(&web, 1, {2}, 1000, 0, events), "owned Web area/restraint created");
    effects.condition(game.actors().id(0), EffectKind::Sleep, {0}, 0, events);
    effects.condition(rules::CharacterId{1}, EffectKind::Blind, {0}, 0, events);
    effects.guide(game.actors().id(0), 0, events);
    run = copy(c, d, *run);
    check(run->game().effects().area(grease) && run->game().effects().area(web),
          "nonempty ground effects retain logical references");
    check(bool(run->game().endConcentration(run->game().actors().snapshot().world, {2})),
          "local Web explicitly dismissed before departure");
    travel(*run, FixtureKind::Upstairs);
    RestRequest rest;
    rest.kind = RestKind::Long;
    rest.world = run->game().actors().snapshot().world;
    rest.token = run->game().restToken();
    run->party().damage({0}, 1);
    check(bool(run->game().rest(rest)), "hub rest advances only portable campaign effects");
    run = copy(c, d, *run);
    travel(*run, FixtureKind::Downstairs);
    check(run->game().effects().area(grease) && run->game().effects().guiding(run->game().actors().id(0)) &&
              run->game().effects().has(run->game().actors().id(0), EffectKind::Sleep) &&
              !run->game().effects().has(rules::CharacterId{1}, EffectKind::Blind),
          "restored inactive local clocks freeze while party clocks advance");
    run = copy(c, d, *run);
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
    resumed.restore(checkpoint);
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

} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 2 && std::string(argv[1]) == "--smoke") {
            environmentRoundtrip();
            roundtrip();
            saveUi();
            std::cout << "campaign save smoke tests passed\n";
            return 0;
        }
        if (argc != 1)
            throw std::invalid_argument("Expected no arguments or --smoke");
        environmentRoundtrip();
        roundtrip();
        std::cerr << "progression/floors\n";
        progressionAndFloors();
        std::cerr << "effects\n";
        effects();
        std::cerr << "I/O failures\n";
        failures();
        std::cerr << "malformed\n";
        malformed();
        saveUi();
        applicationFlow();
        navigationContinuation();
        libraryIndependence();
        postCombatSave();
        groundAndInactive();
        protocolLoadRollback();
        std::cout << "campaign save tests passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
