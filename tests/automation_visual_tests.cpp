#include "automation/visual.hpp"
#include "render/frame_pipeline.hpp"
#include "support/automation_fixtures.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::automation;
using namespace blobber::automation::test;
namespace {
void compare(Visibility& visual, const SceneRenderer& renderer, const std::vector<NpcTimings>& timings,
             const Map& map, GameSession& g) {
    NpcAnimator animator(g.actors(), timings);
    animator.setTime(g.timeMs(), &g.effects());
    FramePipeline frames;
    SpellArea area;
    area.ground = g.effects().ground();
    frames.update(renderer, map, g.movement(), &g.actors(), &animator.snapshot(), &g.combat().attacks(),
                  g.timeMs(), &area, &g.loot().snapshot());
    const auto targets = visual.sample(map, g);
    const auto expected = frames.picking() ? frames.picking()->targets() : TargetSnapshot{};
    check(targets.count == expected.count, "visual runner shares exact picking membership");
    for (std::size_t i = 0; i < expected.count; ++i)
        check(sameActor(targets.ids[i], expected.ids[i]), "stable target identity");
    CommandController ui(g, timings);
    ui.refresh(expected, g.movement().transition.active(), false, true);
    for (std::uint16_t i = 0; i < 4; ++i)
        for (std::size_t n = 0; n < g.actors().snapshot().count; ++n) {
            PartyCommand c;
            c.character = {i};
            c.world = g.actors().snapshot().world;
            c.turn = g.combat().snapshot().turn;
            c.action = PartyAction::Ranged;
            c.target = g.actors().id(n);
            auto reason = g.movement().transition.active() ? CommandReason::Moving
                                                           : g.combat().validate(c, g.movement().player);
            if (reason == CommandReason::Ready)
                reason = targetVisibility(c, g.party().catalog(), targets, g.actors().snapshot());
            check(ui.validate(c) == reason,
                  "normal UI and visual adapter agree on complete command eligibility");
        }
}
} // namespace
int main() {
    try {
        const auto cat = catalog();
        auto appearance = testNpcAppearance();
        appearance.width = .5f;
        const SceneRenderer renderer(testMaterials(), {appearance});
        std::vector<NpcTimings> timings{npcTimings(appearance)};
        VisualContext context{renderer, timings};
        auto defs = definitions();
        defs.timings = timings;
        Client prototype(cat, defs, Mode::Visual, makeVisual, &context);
        auto forced = generatedScenario(cat);
        forced["force_fallback"] = true;
        check(!prototype.request("start", {{"scenario", forced}}).at("ok"),
              "forced fallback is diagnostic only");
        const auto generatedStart =
            prototype.request("start", {{"scenario", generatedScenario(cat)}}).at("data");
        check(generatedStart.at("generation").contains("fingerprint") &&
                  !generatedStart.at("generation").contains("map") &&
                  !generatedStart.at("generation").contains("rooms") &&
                  !generatedStart.at("generation").contains("rejections"),
              "player-visible prototype observes identity, not hidden topology/roles");
        check(prototype.request("inspect").at("data").at("generation").contains("rooms"),
              "explicit diagnostic prototype inspection includes role metadata");
        check(!prototype.request("observe").at("data").at("generation").contains("rooms"),
              "diagnostic inspection cannot disable subsequent player filtering");
        check(prototype.act({{"kind", "move"}, {"direction", "forward"}}).at("data").at("accepted"),
              "visual prototype movement uses existing checked actions");
        prototype.advance();
        Client player(cat, defs, Mode::Visual, makeVisual, &context);
        const auto start = player.start(cat).at("data");
        check(start.at("enemies").empty() && !start.contains("attacks"),
              "distant initial roster and pending outcomes are hidden");
        const auto inspection = player.request("inspect").at("data");
        check(inspection.at("enemies").size() == 1 && inspection.at("diagnostic_inspection_used"),
              "explicit inspection is full and labelled");
        check(player.request("observe").at("data").at("enemies").empty(),
              "inspection does not disable player filtering");
        const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
        const Player spawn{{3, 4}, Heading::North};
        const NpcWorld placements(map, spawn.cell, {"goblin"},
                                  {{{0}, {3, 2}, {0, 1}, Heading::South},
                                   {{0}, {3, 1}, {0, 1}, Heading::South},
                                   {{0}, {3, 5}, {0, 0}, Heading::North}});
        GameSession g(map, cat, placements, 5, spawn, false, timings);
        auto visual = makeVisual(&context, map, g);
        compare(*visual, renderer, timings, map, g);
        const auto initial = visual->sample(map, g);
        check(initial.count >= 1 && initial.count < 3, "overlap and offscreen enemies exercise filtering");
        Observation o;
        o.session = &g;
        o.map = &map;
        o.targets = initial;
        for (std::size_t i = 0; i < initial.count; ++i)
            o.visible[initial.ids[i].slot] = true;
        auto observed = encodeObservation(o);
        for (const auto& e : observed.at("enemies"))
            check(!e.contains("hp") && !e.contains("awareness") && e.contains("health_fraction"),
                  "player enemy observations expose HUD information only");
        check(g.actors().damage(g.actors().id(0), 100, 0).died, "fixture lethal hit");
        compare(*visual, renderer, timings, map, g);
        const auto corpse = visual->sample(map, g);
        for (std::size_t i = 0; i < corpse.count; ++i)
            check(corpse.ids[i].slot != 0, "corpse never becomes selectable");
        g.update(.11, false);
        compare(*visual, renderer, timings, map, g);
        g.move(Action::TurnLeft);
        check(g.movement().transition.active(), "fixture turn transition starts");
        compare(*visual, renderer, timings, map, g);
        check(visual->sample(map, g).count == 0, "transition suspends targetability");
        g.update(.2, false);
        compare(*visual, renderer, timings, map, g);
        const Map wall({"#######", "#.....#", "###.###", "#.....#", "#.....#", "#.....#", "#######"});
        const NpcWorld blocked(wall, spawn.cell, {"goblin"}, {{{0}, {1, 1}, {0, 1}, Heading::South}});
        GameSession behindWall(wall, cat, blocked, 5, spawn, false, timings);
        auto hidden = makeVisual(&context, wall, behindWall);
        compare(*hidden, renderer, timings, wall, behindWall);
        check(hidden->sample(wall, behindWall).count == 0, "wall-hidden actor absent");
        auto invisible = hidden->sample(wall, behindWall);
        PartyCommand spectral;
        spectral.action = PartyAction::Feature;
        spectral.feature = rules::Feature::SpiritualWeapon;
        spectral.targetCount = 1;
        spectral.enemies[0] = behindWall.actors().id(0);
        for (std::uint16_t i = 0; i < cat.spellCount(); ++i)
            if (cat.spell({i})->kind == rules::SpellKind::SpiritualWeapon)
                spectral.spell = {i};
        check(targetVisibility(spectral, cat, invisible, behindWall.actors().snapshot()) ==
                  CommandReason::NotVisible,
              "spectral follow-up cannot name a wall-hidden enemy");
        spectral.action = PartyAction::BackMelee;
        spectral.target = behindWall.actors().id(0);
        check(targetVisibility(spectral, cat, invisible, behindWall.actors().snapshot()) ==
                  CommandReason::NotVisible,
              "back-row weapon action cannot bypass body visibility");
        spectral.action = PartyAction::Feature;
        spectral.targetCount = 4;
        check(targetVisibility(spectral, cat, invisible, behindWall.actors().snapshot()) ==
                  CommandReason::InvalidTargets,
              "oversized typed recipient list rejects before indexing");
        CombatOutcome pending;
        pending.event = CombatEvent::AttackStarted;
        pending.roll = 20;
        pending.damage = 100;
        pending.hit = true;
        check(encodeEvent(pending, cat, false).is_null() &&
                  encodeEvent(pending, cat, true).at("damage") == 100,
              "pre-impact rolls are diagnostic only");
        pending.event = CombatEvent::ProjectileReleased;
        check(encodeEvent(pending, cat, false).is_null(), "projectile release does not reveal saved result");
        pending.event = CombatEvent::Attack;
        check(encodeEvent(pending, cat, false).at("damage") == 100,
              "resolved impact appears in player events");
        std::cout << "Windowless visual targeting and observation checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
