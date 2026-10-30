#include "support/automation_fixtures.hpp"
#include <cstdlib>
#include <iostream>
#include <new>
#ifdef BLOBBER_AUTOMATION_VISUAL
#include "automation/visual.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#endif
namespace {
bool tracking = false;
std::size_t allocations = 0;
void* allocate(std::size_t size) {
    if (tracking)
        ++allocations;
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
    if (tracking)
        ++allocations;
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
        const auto cat = catalog();
        auto defs = definitions();
        auto setup = scenario(cat, argc > 1 ? 256 : 1);
        Session session(cat, defs, setup, Mode::Diagnostic);
#ifdef BLOBBER_AUTOMATION_VISUAL
        auto art = testNpcAppearance();
        SceneRenderer renderer(testMaterials(), {art});
        defs.timings = {npcTimings(art)};
        VisualContext context{renderer, defs.timings};
        Session visual(cat, defs, setup, Mode::Visual, makeVisual, &context);
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
        tracking = true;
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
