#include "automation/world_visual.hpp"
#include "game/commands.hpp"
#include "persistence/campaign_codec.hpp"
#include "support/automation_fixtures.hpp"
#include "support/cpu_view.hpp"
#include "support/fixture_world.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <cstdlib>
#include <iostream>
#include <new>
namespace {
bool tracking = false;
std::size_t allocations = 0, requestedBytes = 0, failAt = 0;
void* allocate(std::size_t size) {
    if (tracking) {
        ++allocations;
        requestedBytes += size;
        if (failAt && allocations == failAt)
            throw std::bad_alloc();
    }
    if (void* p = std::malloc(size ? size : 1))
        return p;
    throw std::bad_alloc();
}
} // namespace
void* operator new(std::size_t size) {
    return allocate(size);
}
void* operator new[](std::size_t size) {
    return allocate(size);
}
void operator delete(void* p) noexcept {
    std::free(p);
}
void operator delete[](void* p) noexcept {
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
    std::free(p);
}
void* operator new(std::size_t size, std::align_val_t alignment) {
    if (tracking) {
        ++allocations;
        requestedBytes += size;
        if (failAt && allocations == failAt)
            throw std::bad_alloc();
    }
    const auto align = static_cast<std::size_t>(alignment);
#ifdef _MSC_VER
    void* p = _aligned_malloc(size ? size : 1, align);
#else
    void* p = std::aligned_alloc(align, ((size ? size : 1) + align - 1) / align * align);
#endif
    if (!p)
        throw std::bad_alloc();
    return p;
}
void* operator new[](std::size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}
void operator delete(void* p, std::align_val_t) noexcept {
#ifdef _MSC_VER
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void operator delete[](void* p, std::align_val_t a) noexcept {
    ::operator delete(p, a);
}
void operator delete(void* p, std::size_t, std::align_val_t a) noexcept {
    ::operator delete(p, a);
}
void operator delete[](void* p, std::size_t, std::align_val_t a) noexcept {
    ::operator delete(p, a);
}

using namespace blobber;
using namespace blobber::automation;
using namespace blobber::automation::test;
int main(int argc, char**) {
    try {
        tracking = true;
        const auto preparedFloor = generation::generate({5});
        tracking = false;
        check(bool(preparedFloor), "allocation measurement generates fixture");
        std::cout << "Generation preparation: " << allocations << " allocations / " << requestedBytes
                  << " requested bytes (not peak resident memory)\n";
        allocations = requestedBytes = 0;
        const auto cat = catalog();
        auto defs = definitions();
        auto setup = scenario(cat, argc > 1 ? 256 : 1);
        Session session(cat, defs, setup, Mode::Diagnostic);
        blobber::test::VisibilityFixture fixture(2);
        const world_assets::CpuAssets assets(fixture.publication());
        defs.timings = fixture.timings;
        WorldVisualContext context{assets, defs.timings};
        Session visual(cat, defs, setup, Mode::Visual, makeWorldVisual, &context);
        const GeneratedScenario generatedSetup{setup.party, {5}};
        Session generated(cat, defs, generatedSetup, Mode::Diagnostic);
        Session generatedVisual(cat, defs, generatedSetup, Mode::Visual, makeWorldVisual, &context);
        Observation observation;
        unsigned events = 0, accepted = 0;
        bool okay = true;
        const auto exercise = [&](Session& s, unsigned steps) {
            for (unsigned i = 0; i < steps && !s.terminal(); ++i) {
                events += s.tick().count;
                s.observe(observation);
                const auto& g = s.game();
                const auto& st = g.combat().snapshot();
                ActionRequest request;
                if (realTime(st.phase) && !g.movement().transition.active()) {
                    request.operation = Operation::Move;
                    request.movement = Action::Forward;
                } else if (st.awaitingAction) {
                    request.combat.character = *st.activeCharacter;
                    request.combat.world = st.world;
                    request.combat.turn = st.turn;
                    request.combat.action = PartyAction::Melee;
                    for (std::size_t n = 0; n < g.actors().snapshot().count; ++n) {
                        request.combat.target = g.actors().id(n);
                        if (s.action(request, true).accepted)
                            break;
                    }
                    if (!s.action(request, true).accepted) {
                        request.combat.action = PartyAction::Defend;
                        request.combat.target.reset();
                    }
                } else
                    continue;
                const auto preview = s.action(request, true);
                if (preview.accepted) {
                    const auto result = s.action(request, false);
                    okay &= result.accepted;
                    accepted += result.accepted;
                    events += s.events().count;
                }
            }
        };
        const auto walk = [&](Session& s) {
            for (unsigned i = 0; i < 6; ++i) {
                ActionRequest r;
                r.operation = Operation::Move;
                constexpr Action actions[]{Action::Forward, Action::Left, Action::TurnRight,
                                           Action::Right,   Action::Back, Action::TurnLeft};
                r.movement = actions[i % 6];
                (void)s.action(r, true);
                (void)s.action(r, false);
                for (unsigned tick = 0; tick < 17; ++tick) {
                    s.tick();
                    s.observe(observation);
                }
                okay &= !s.terminal();
            }
        };
        const auto& fixtureRules = cat;
        const auto fixtureFloor = fixtureMap();
        const NpcWorld empty(fixtureFloor, {2, 3}, defs.types, {});
        auto fixtureGame = std::make_unique<GameSession>(
            fixtureFloor, fixtureRules, empty, 5, Player{{2, 3}, Heading::East}, false,
            std::vector<NpcTimings>{}, RecoveryLevel{}, fixtureDefinitions());
        CommandController fixtureUi(*fixtureGame);
        blobber::test::CpuView fixtureFrames(*fixtureGame, assets, defs.timings);
        const auto fixtureFrame = [&] { fixtureFrames.update(*fixtureGame); };
        CommandController guardianUi(generated.replayGame());
        blobber::test::CpuView guardianFrames(generated.replayGame(), assets, defs.timings);
        auto campaign = std::make_unique<CampaignRun>(cat, *cat.preset(), defs.types, defs.timings, 5);
        for (unsigned visit = 0; visit < 3; ++visit) {
            auto travel = campaign->prepareTravel(campaign->request(campaign->game().nearbyFixture()));
            check(campaign->commitTravel(travel), "allocation fixture prepares repeated travel");
        }
        // Inject each preparation allocation failure in turn, before touching the live owner.
        for (const auto kind : {TravelKind::Stairs, TravelKind::Recall, TravelKind::Waypoint}) {
            auto rollback = std::make_unique<CampaignRun>(cat, *cat.preset(), defs.types, defs.timings, 5);
            if (kind != TravelKind::Stairs) {
                auto enter = rollback->prepareTravel(rollback->request(rollback->game().nearbyFixture()));
                check(rollback->commitTravel(enter), "fault fixture enters floor");
                if (kind == TravelKind::Waypoint) {
                    auto back = rollback->prepareTravel(rollback->request(rollback->game().nearbyFixture()));
                    check(rollback->commitTravel(back), "fault fixture unlocks entry and returns");
                }
            }
            const auto location = rollback->activeLocation();
            const auto kits = rollback->party().campSupplies().kits;
            const auto invRevision = rollback->party().inventory().revision();
            const auto request = kind == TravelKind::Recall ? rollback->recallRequest()
                                 : kind == TravelKind::Waypoint
                                     ? rollback->waypointRequest(1)
                                     : rollback->request(rollback->game().nearbyFixture());
            auto before = std::make_unique<FloorState>();
            rollback->game().exportFloor(*before);
            const auto fingerprint = before->digest();
            const auto world = rollback->game().actors().snapshot().world;
            const auto stage = [&] {
                auto ticket = rollback->prepareTravel(request);
                blobber::test::CpuView frames(ticket.game(), assets, defs.timings);
                frames.update(ticket.game());
            };
            allocations = requestedBytes = 0;
            tracking = true;
            stage();
            tracking = false;
            const auto preparationAllocations = allocations;
            for (std::size_t failure = 1; failure <= preparationAllocations; ++failure) {
                allocations = 0;
                failAt = failure;
                bool rejected = false;
                tracking = true;
                try {
                    stage();
                } catch (const std::bad_alloc&) {
                    rejected = true;
                }
                tracking = false;
                failAt = 0;
                rollback->game().exportFloor(*before);
                check(rejected && rollback->activeLocation() == location &&
                          rollback->party().campSupplies().kits == kits &&
                          rollback->party().inventory().revision() == invRevision &&
                          rollback->game().actors().snapshot().world == world &&
                          before->digest() == fingerprint,
                      "every destination/source/view allocation failure preserves the current floor");
            }
            std::cout << travelKindName(kind) << " rollback: " << preparationAllocations
                      << " allocation failure points checked\n";
        }
        // Restore and optional view preparation are explicit allocation boundaries.
        auto checkpoint = campaign->checkpoint();
        const auto saveText = encodeCampaign(cat, defs, {"owned", 0}, *checkpoint);
        const auto restoreStage = [&](std::unique_ptr<CampaignCheckpoint> state) {
            auto restored = std::make_unique<CampaignRun>(cat, std::move(*state), defs.timings);
            blobber::test::CpuView frames(restored->game(), assets, defs.timings);
            frames.update(restored->game());
        };
        auto stagedState = decodeCampaign(cat, defs, {"owned", 0}, saveText);
        allocations = 0;
        tracking = true;
        restoreStage(std::move(stagedState));
        tracking = false;
        const auto restoreAllocations = allocations;
        const auto oldWorld = campaign->game().actors().snapshot().world;
        for (std::size_t failure = 1; failure <= restoreAllocations; ++failure) {
            auto state = campaign->checkpoint();
            allocations = 0;
            failAt = failure;
            bool rejected = false;
            tracking = true;
            try {
                restoreStage(std::move(state));
            } catch (const std::bad_alloc&) {
                rejected = true;
            }
            tracking = false;
            failAt = 0;
            check(rejected && campaign->game().actors().snapshot().world == oldWorld,
                  "every restore/view allocation failure preserves current owner");
        }
        std::cout << "restore rollback: " << restoreAllocations << " allocation failure points checked\n";
        auto resumedState = decodeCampaign(cat, defs, {"owned", 0}, saveText);
        campaign = std::make_unique<CampaignRun>(cat, std::move(*resumedState), defs.timings);
        allocations = requestedBytes = 0;
        CommandController campaignUi(campaign->game(), defs.timings);
        campaignUi.saveMenu().enabled = true;
        blobber::test::CpuView campaignFrames(campaign->game(), assets, defs.timings);
        tracking = true;
        for (unsigned i = 0; i < 40; ++i) {
            campaign->update(.01, false);
            campaignUi.refresh({}, false, false, true);
            campaignUi.hover(HoverKind::Travel, 0, {}, 579, 3);
            okay &= campaignUi.snapshot().hover.kind == HoverKind::Travel;
            campaignUi.toggleMenu();
            campaignUi.routeSaveMenu(UiInput{});
            campaignUi.closeOverlay();
            okay &= !campaign->saveSafety();
            campaignUi.openTravel();
            campaignUi.refreshTravel();
            campaignUi.selectTravel(0);
            campaignUi.confirmTravel();
            campaignUi.cancelTravel();
            campaignUi.closeOverlay();
            okay &= campaign->previewTravel(campaign->recallRequest()).accepted;
            campaignFrames.update(campaign->game());
        }
        auto& guardianGame = generated.replayGame();
        guardianGame.actors().damage(guardianGame.actors().id(2), 100000, 0);
        guardianGame.actors().damage(guardianGame.actors().id(3), 100000, 0);
        guardianGame.update(0, false);
        guardianGame.update(.3, false);
        okay &= guardianGame.objective().completed && guardianGame.landingSafety(0).safe;
        guardianUi.refresh({}, false, false, true);
        guardianFrames.update(guardianGame);
        fixtureFrame();
        collectFixtureKey(*fixtureGame);
        okay &= fixtureGame->party().questKeys() == 4;
        okay &= fixtureGame->interact(fixtureRequest(*fixtureGame, 0), true).accepted;
        okay &= fixtureGame->interact(fixtureRequest(*fixtureGame, 0)).changed;
        fixtureFrame();
        fixtureUi.refresh({}, false, false, true);
        fixtureUi.interactFixture();
        fixtureGame->update(.3, false);
        fixtureGame->move(Action::Forward);
        fixtureUi.refresh({}, false, true, true);
        walk(generated);
        walk(generatedVisual);
        exercise(session, argc > 1 ? 60000 : 6000);
        // Representative visual decisions; full runs are covered by integration tests.
        exercise(visual, 40);
        tracking = false;
        check(okay && accepted > 10 && events > 0, "allocation workload executes actual movement/combat");
        check(allocations == 0,
              "prepared runner simulation and typed observations must allocate no C++ storage");
        std::cout << "Automation: zero tracked C++ allocations, " << accepted << " actions, " << events
                  << " events\n";
    } catch (const std::exception& e) {
        tracking = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
