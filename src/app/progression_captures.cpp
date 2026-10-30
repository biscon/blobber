#include "app/progression_captures.hpp"
#include "app/starting_build_diagnostics.hpp"
#include "game/arena.hpp"
#include <fstream>
#include <stdexcept>
namespace blobber {
void captureProgression(const rules::Catalog& catalog, const NpcWorld& source,
                        const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                        Presenter& presenter, const std::filesystem::path& directory) {
    const auto map = arenaMap();
    const auto spawn = arenaSpawn();
    std::vector<std::string> types;
    for (std::uint16_t i = 0; i < source.typeCount(); ++i)
        types.push_back(source.typeId({i}));
    auto scenario = defaultArena(catalog);
    scenario.adjust(catalog.findMonster("goblin"), 1);
    auto builds = arenaParty(catalog, scenario);
    const NpcWorld placements(
        map, spawn.cell, types,
        {{{0}, spawn.cell + forward(spawn.heading) * 3, {0, 0}, Heading((unsigned(spawn.heading) + 2) % 4)}});
    RecoveryLevel recovery;
    recovery.sanctuaries[recovery.sanctuaryCount++] = spawn.cell;
    GameSession s(map, catalog, builds, placements, 5, spawn, false, timings, recovery);
    s.enableDevelopment(true);
    CommandController ui(s, timings);
    FramePipeline frames;
    NpcAnimator animator(s.actors(), timings);
    std::ofstream trace(directory / "progression.tsv");
    auto refresh = [&] {
        animator.setTime(s.timeMs(), &s.effects());
        const auto area = ui.floorPreview();
        frames.update(renderer, map, s.movement(), &s.actors(), &animator.snapshot(), &s.combat().attacks(),
                      s.timeMs(), &area, &s.loot().snapshot());
        ui.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false, true);
    };
    auto capture = [&](const std::string& name) {
        refresh();
        presenter.present(frames.frame(), s.movement(), map, false, true, &ui.snapshot(), frames.picking());
        presenter.capture(directory / (name + ".png"));
        presenter.capture(directory / (name + "-1280.png"), 1280, 720);
        presenter.capture(directory / (name + "-4x3.png"), 1024, 768);
    };
    auto click = [&](FlowAction action, int value = 0) {
        FlowControls controls;
        layoutAdvancement(controls, ui.snapshot().advancement);
        for (unsigned i = 0; i < controls.count; ++i)
            if (controls.entries[i].action == action && controls.entries[i].value == value) {
                if (!controls.entries[i].enabled)
                    throw std::runtime_error("Disabled advancement capture control");
                UiInput input;
                input.focused = true;
                input.pressed[std::size_t(UiKey::Click)] = true;
                const auto r = controls.entries[i].rect;
                input.pointer = ArtPoint{float(r.x + r.width / 2), float(r.y + r.height / 2)};
                ui.routeAdvancement(input);
                return;
            }
        throw std::runtime_error("Missing advancement capture control");
    };
    refresh();
    capture("13-start-level-one");
    for (int level = 2; level <= 3; ++level) {
        ui.developmentExperience();
        refresh();
        ui.openInventory(rules::CharacterId{0}, CharacterPage::Character);
        capture("13-xp-level-" + std::to_string(level));
        ui.closeOverlay();
        for (std::uint16_t id = 0; id < 4; ++id) {
            ui.openAdvancement({id});
            if (!ui.snapshot().advancement.open)
                throw std::runtime_error("Capture advancement unavailable");
            if (!id)
                capture("13-overview-" + std::to_string(level));
            click(FlowAction::Next);
            if (level == 3)
                capture("13-features-" + std::to_string(id));
            click(FlowAction::Next);
            if (ui.snapshot().advancement.step == AdvancementStep::Learned) {
                click(FlowAction::Recommend);
                capture("13-learned-" + std::to_string(level));
                if (level == 3) {
                    click(FlowAction::Toggle, catalog.findSpell("web").index);
                    capture("13-web-description");
                    UiInput wheel;
                    wheel.pointer = ArtPoint{500, 240};
                    wheel.scroll = -30;
                    ui.routeAdvancement(wheel);
                    const auto& detail = ui.snapshot().advancement.selection;
                    if (detail.detailOffset != (detail.detailLines > detail.detailPageLines
                                                    ? detail.detailLines - detail.detailPageLines
                                                    : 0))
                        throw std::runtime_error("Description scrolling exceeded its measured range");
                }
                click(FlowAction::Next);
            }
            if (ui.snapshot().advancement.step == AdvancementStep::Prepared) {
                capture("13-prepared-empty-" + std::to_string(level) + "-" + std::to_string(id));
                click(FlowAction::Recommend);
                capture("13-prepared-" + std::to_string(level) + "-" + std::to_string(id));
                if (level == 2 && id == 3) {
                    const auto& picker = ui.snapshot().advancement.selection;
                    rules::SpellId first;
                    for (unsigned i = 0; i < picker.count; ++i)
                        if (!(picker.selected & rules::spellBit(picker.visible[i]))) {
                            first = picker.visible[i];
                            break;
                        }
                    click(FlowAction::Toggle, first.index);
                    if (ui.snapshot().advancement.selection.selected != rules::spellBit(first))
                        throw std::runtime_error("Single preparation did not replace the previous pick");
                    capture("13-prepared-replacement");
                    click(FlowAction::Recommend);
                }
                click(FlowAction::Next);
            }
            if (id == 3)
                capture("13-review-" + std::to_string(level));
            click(FlowAction::Done);
            if (s.party().character({id})->build.level != level)
                throw std::runtime_error("Capture advancement did not commit");
            trace << id << '\t' << level << '\t' << s.party().character({id})->experience << '\n';
            if (level == 3)
                capture("13-sheet-" + std::to_string(id));
            ui.closeOverlay();
            refresh();
        }
        if (level == 2) {
            ui.openCamp();
            ui.campKind(RestKind::Long);
            ui.openPreparation();
            capture("13-level-two-rest-preparation");
            ui.closeOverlay();
        }
    }
    ui.openCamp();
    ui.campKind(RestKind::Long);
    ui.openPreparation();
    capture("13-long-rest-preparation");
    ui.closeOverlay();
    // Separate read-only capture setup validates actual legal advancement selections for Web.
    GameSession webSession(map, catalog, builds, placements, 5, spawn, false, timings, recovery);
    for (int level = 2; level <= 3; ++level) {
        webSession.party().grantNextLevelExperience();
        for (std::uint16_t id = 0; id < 4; ++id) {
            auto r = webSession.party().advancementRequest({id});
            const auto& c = *webSession.party().character({id});
            auto context = rules::advancementChoices(catalog, c.build.view(), c.build.portrait,
                                                     c.build.spells, {}, true);
            if (id == 3 && level == 3)
                r.choices.learned = rules::spellBit(catalog.findSpell("web")) |
                                    rules::spellBit(catalog.findSpell("mirror_image"));
            else
                r.choices.learned = rules::recommendedChoices(catalog, context, rules::SpellChoice::Learned);
            if (id == 3 && level == 3)
                r.choices.prepared = rules::spellBit(catalog.findSpell("web"));
            if (!webSession.party().advance(r))
                throw std::runtime_error("Capture Web advancement failed");
        }
    }
    PartyCommand weapon;
    weapon.character = {1};
    weapon.action = PartyAction::Spell;
    weapon.spell = catalog.findSpell("spiritual_weapon");
    weapon.enemies[0] = webSession.actors().id(0);
    weapon.targetCount = 1;
    weapon.world = webSession.actors().snapshot().world;
    weapon.turn = webSession.combat().snapshot().turn;
    if (webSession.submit(weapon).reason != CommandReason::Resolved)
        throw std::runtime_error("Capture spectral cast failed");
    PartyCommand web;
    web.character = {3};
    web.action = PartyAction::Spell;
    web.spell = catalog.findSpell("web");
    web.aimedCell = spawn.cell + forward(spawn.heading) * 2;
    web.world = weapon.world;
    web.turn = webSession.combat().snapshot().turn;
    if (webSession.submit(web).reason != CommandReason::Resolved)
        throw std::runtime_error("Capture Web cast failed");
    CommandController webUi(webSession, timings);
    NpcAnimator webAnimator(webSession.actors(), timings);
    FramePipeline webFrames;
    webAnimator.setTime(0, &webSession.effects());
    const auto area = webUi.floorPreview();
    webFrames.update(renderer, map, webSession.movement(), &webSession.actors(), &webAnimator.snapshot(),
                     &webSession.combat().attacks(), 0, &area);
    webUi.refresh(webFrames.picking()->targets(), false, false, true);
    presenter.present(webFrames.frame(), webSession.movement(), map, false, true, &webUi.snapshot(),
                      webFrames.picking());
    presenter.capture(directory / "13-web-weapon.png");
    presenter.capture(directory / "13-web-weapon-1280.png", 1280, 720);
    presenter.capture(directory / "13-web-weapon-4x3.png", 1024, 768);
    // Frozen review fixture: four sleepers behind two standing enemies. Compare
    // with the front pair removed to verify the same badges reappear unobscured.
    std::vector<NpcInstance> layered;
    const auto facing = Heading((unsigned(spawn.heading) + 2) % 4);
    const auto nearCell = spawn.cell + forward(spawn.heading);
    const auto farCell = spawn.cell + forward(spawn.heading) * 2;
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
            layered.push_back({{0}, farCell, {x, y}, facing});
    for (int x = 0; x < 2; ++x)
        layered.push_back({{0}, nearCell + right(spawn.heading) * 2, {x, 1}, facing});
    const NpcWorld layeredPlacements(map, spawn.cell, types, layered);
    GameSession sleepers(map, catalog, spellCaptureParty(catalog, rules::SpellKind::Sleep), layeredPlacements,
                         5, spawn, false, timings);
    for (unsigned i = 0; i < 4; ++i) {
        const auto id = sleepers.actors().id(i);
        sleepers.actors().damage(id, sleepers.actors().actor(id)->hitPoints - 1, 0);
    }
    PartyCommand sleep;
    sleep.character = {3};
    sleep.action = PartyAction::Spell;
    sleep.spell = catalog.findSpell("sleep");
    sleep.aimedCell = farCell + forward(spawn.heading);
    sleep.world = sleepers.actors().snapshot().world;
    sleep.turn = sleepers.combat().snapshot().turn;
    if (sleepers.submit(sleep).reason != CommandReason::Resolved)
        throw std::runtime_error("Occlusion capture Sleep cast failed");
    for (unsigned i = 0; i < 6; ++i)
        if (sleepers.effects().has(sleepers.actors().id(i), EffectKind::Sleep) != (i < 4))
            throw std::runtime_error("Occlusion capture must sleep only rear enemies");
    for (unsigned i = 4; i < 6; ++i)
        for (const auto cell : {nearCell + right(spawn.heading), nearCell})
            if (!sleepers.actors().move(sleepers.actors().id(i), cell, {int(i - 4), 1}, spawn.cell).changed)
                throw std::runtime_error("Occlusion capture front placement failed");
    CommandController sleepUi(sleepers, timings);
    NpcAnimator sleepAnimator(sleepers.actors(), timings);
    Frame sleepFrame;
    PickingBuffer sleepPicking;
    const auto captureSleep = [&](const char* name) {
        sleepAnimator.setTime(500, &sleepers.effects());
        renderer.compose(map, spawn, sleepFrame, &sleepers.actors(), &sleepAnimator.snapshot(),
                         &sleepPicking);
        sleepUi.refresh(sleepPicking.targets(), false, false, true);
        presenter.present(sleepFrame, sleepers.movement(), map, false, true, &sleepUi.snapshot(),
                          &sleepPicking);
        presenter.capture(directory / (std::string(name) + ".png"));
        presenter.capture(directory / (std::string(name) + "-1280.png"), 1280, 720);
        presenter.capture(directory / (std::string(name) + "-4x3.png"), 1024, 768);
    };
    captureSleep("13-status-occluded");
    sleepers.actors().remove(sleepers.actors().id(4));
    sleepers.actors().remove(sleepers.actors().id(5));
    captureSleep("13-status-revealed");
    GameSession rematch(map, catalog, builds, placements, 5, spawn, false, timings, recovery);
    for (const auto& c : rematch.party().snapshot().characters)
        if (c.build.level != 1 || c.experience != 0)
            throw std::runtime_error("Rematch inherited progression");
    trace << "rematch\tlevel-one\tzero-xp\n";
}
} // namespace blobber
