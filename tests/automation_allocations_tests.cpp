#include "game/commands.hpp"
#include "support/automation_fixtures.hpp"
#include "support/fixture_world.hpp"
#include <cstdlib>
#include <iostream>
#include <new>
#ifdef BLOBBER_AUTOMATION_VISUAL
#include "automation/visual.hpp"
#include "render/frame_pipeline.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#endif
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
#ifdef BLOBBER_AUTOMATION_VISUAL
        auto art = testNpcAppearance();
        PreparedLoot markerBank;
        markerBank.pixels.assign(PreparedLoot::icons * PreparedLoot::pixelsPerIcon, Color{220, 190, 90, 255});
        SceneRenderer renderer(prepareScene(testMaterials()), PreparedNpcs({art, art}), {},
                               std::move(markerBank));
        defs.timings = {npcTimings(art), npcTimings(art)};
        VisualContext context{renderer, defs.timings};
        Session visual(cat, defs, setup, Mode::Visual, makeVisual, &context);
#endif
        const GeneratedScenario generatedSetup{setup.party, {5}};
        Session generated(cat, defs, generatedSetup, Mode::Diagnostic);
#ifdef BLOBBER_AUTOMATION_VISUAL
        Session generatedVisual(cat, defs, generatedSetup, Mode::Visual, makeVisual, &context);
#endif
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
            for (unsigned i = 0; i < 48; ++i) {
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
#ifdef BLOBBER_AUTOMATION_VISUAL
        FramePipeline fixtureFrames;
        const auto fixtureFrame = [&] {
            fixtureFrames.update(renderer, fixtureGame->geometry(), fixtureGame->movement(),
                                 &fixtureGame->actors(), nullptr, nullptr, fixtureGame->timeMs(), nullptr,
                                 &fixtureGame->loot().snapshot());
        };
#endif
        CommandController guardianUi(generated.replayGame());
#ifdef BLOBBER_AUTOMATION_VISUAL
        FramePipeline guardianFrames;
#endif
        auto campaign = std::make_unique<CampaignRun>(cat, *cat.preset(), defs.types, defs.timings, 5);
        for (unsigned visit = 0; visit < 3; ++visit) {
            auto travel = campaign->prepareTravel(campaign->request(campaign->game().nearbyFixture()));
            check(campaign->commitTravel(travel), "allocation fixture prepares repeated travel");
        }
        // Inject each preparation allocation failure in turn, before touching the live owner.
        auto rollback = std::make_unique<CampaignRun>(cat, *cat.preset(), defs.types, defs.timings, 5);
        auto before = std::make_unique<FloorState>();
        rollback->game().exportFloor(*before);
        const auto fingerprint = before->digest();
        const auto world = rollback->game().actors().snapshot().world;
        const auto stage = [&] {
            auto ticket = rollback->prepareTravel(rollback->request(rollback->game().nearbyFixture()));
#ifdef BLOBBER_AUTOMATION_VISUAL
            FramePipeline frames;
            frames.update(renderer, ticket.game().geometry(), ticket.game().movement(),
                          &ticket.game().actors());
#endif
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
            check(rejected && rollback->activeLocation() == 0 &&
                      rollback->game().actors().snapshot().world == world && before->digest() == fingerprint,
                  "every destination/source/view allocation failure preserves the current floor");
        }
        std::cout << "Travel rollback: " << preparationAllocations << " allocation failure points checked\n";
        allocations = requestedBytes = 0;
        CommandController campaignUi(campaign->game(), defs.timings);
#ifdef BLOBBER_AUTOMATION_VISUAL
        FramePipeline campaignFrames;
#endif
        tracking = true;
        for (unsigned i = 0; i < 40; ++i) {
            campaign->update(.01, false);
            campaignUi.refresh({}, false, false, true);
#ifdef BLOBBER_AUTOMATION_VISUAL
            campaignFrames.update(renderer, campaign->game().geometry(), campaign->game().movement(),
                                  &campaign->game().actors(), nullptr, nullptr, campaign->game().timeMs(),
                                  nullptr, &campaign->game().loot().snapshot());
#endif
        }
        auto& guardianGame = generated.replayGame();
        guardianGame.actors().damage(guardianGame.actors().id(2), 100000, 0);
        guardianGame.actors().damage(guardianGame.actors().id(3), 100000, 0);
        guardianGame.update(0, false);
        okay &= guardianGame.objective().completed && guardianGame.landingSafety(0).safe;
        guardianUi.refresh({}, false, false, true);
#ifdef BLOBBER_AUTOMATION_VISUAL
        guardianFrames.update(renderer, guardianGame.geometry(), guardianGame.movement(),
                              &guardianGame.actors(), nullptr, nullptr, 0, nullptr,
                              &guardianGame.loot().snapshot());
        fixtureFrame();
#endif
        collectFixtureKey(*fixtureGame);
        okay &= fixtureGame->party().questKeys() == 4;
        okay &= fixtureGame->interact(fixtureRequest(*fixtureGame, 0), true).accepted;
        okay &= fixtureGame->interact(fixtureRequest(*fixtureGame, 0)).changed;
#ifdef BLOBBER_AUTOMATION_VISUAL
        fixtureFrame();
#endif
        fixtureUi.refresh({}, false, false, true);
        fixtureUi.interactFixture();
        fixtureGame->move(Action::Forward);
        fixtureUi.refresh({}, false, true, true);
        walk(generated);
#ifdef BLOBBER_AUTOMATION_VISUAL
        walk(generatedVisual);
#endif
        exercise(session, argc > 1 ? 60000 : 6000);
#ifdef BLOBBER_AUTOMATION_VISUAL
        // Representative visual decisions; full runs are covered by integration tests.
        exercise(visual, 100);
#endif
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
