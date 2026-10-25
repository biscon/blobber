#include "game/party.hpp"
#include "input/key_repeat.hpp"
#include "render/frame_pipeline.hpp"
#include "render/transitions.hpp"
#include "rules/content.hpp"
#include "support/check.hpp"
#include "support/npc_fixtures.hpp"
#include "support/party_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include <cstdlib>
#include <iostream>
#include <new>
#include <type_traits>
#ifdef _MSC_VER
#include <malloc.h>
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
int main() {
    try {
        static_assert(!std::is_copy_constructible_v<Frame>);
        const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
        const auto appearance = testAnimatedNpcAppearance();
        const SceneRenderer renderer(testMaterials(), {appearance});
        const NpcWorld npcs(map, {3, 3}, {"test"},
                            {{{0}, {3, 1}, {0, 0}, Heading::South},
                             {{0}, {3, 1}, {1, 1}, Heading::West, NpcAnimation::Attack},
                             {{0}, {3, 1}, {0, 1}, Heading::East, NpcAnimation::Walk},
                             {{0}, {3, 1}, {1, 0}, Heading::North}});
        NpcAnimator animator(npcs, {npcTimings(appearance)});
        FramePipeline frames;
        Frame expected, source, destination;
        Controller controller({{3, 3}, Heading::North});
        KeyRepeat repeat;
        bool okay = true;
        rules::MonsterDefinition monster;
        monster.id = "allocation-fixture";
        monster.abilities.fill(10);
        monster.armorClass = 12;
        monster.hitPoints = 8;
        monster.attacks[0] = {0};
        monster.attackCount = 1;
        const rules::Catalog catalog(
            {{"fixture-attack", rules::AttackKind::Melee, 3, {1, 6, 2}, rules::DamageType::Slashing}},
            {monster});
        const auto monsterId = catalog.findMonster("allocation-fixture");
        rules::DiceRng dice(42);
        const auto partyCatalog = partyFixture().catalog();
        Party party(partyCatalog);
        auto invalidBuild = party.character({0})->build;
        invalidBuild.equipment.body = {16};
        tracking = true;
        for (int i = 0; i < 1000; ++i) {
            const auto& snapshot = party.snapshot();
            const auto* member = party.character({static_cast<std::uint16_t>(i % rules::partySize)});
            const auto derived = rules::deriveCharacter(partyCatalog, member->build);
            okay &= bool(derived) && derived.value.maxHitPoints == member->hitPoints;
            okay &= partyCatalog.item(member->build.items[0].item) != nullptr;
            okay &= snapshot.formation[0].index < rules::partySize;
            okay &= !rules::deriveCharacter(partyCatalog, invalidBuild);
            okay &= party.swapFormation(FormationSlot::FrontLeft, FormationSlot::BackRight);
            okay &= !party.swapFormation(FormationSlot::FrontLeft, static_cast<FormationSlot>(255));
            okay &= party.occupant(FormationSlot::FrontLeft) != nullptr;
        }
        for (int i = 0; i < 1000; ++i) {
            const auto* actor = catalog.monster(monsterId);
            const auto* attack = catalog.attack(actor->attacks[0]);
            const auto roll = dice.d20(rules::rollMode(i % 2 == 0, i % 3 == 0));
            const auto hit = rules::resolveAttack(roll.value, attack->bonus, actor->armorClass);
            const auto save = rules::resolveSave(roll.value, 2, 12);
            const auto damage =
                dice.damage(attack->damage, hit.value.outcome == rules::AttackOutcome::Critical);
            const auto bulk = dice.roll({32, 100, -3});
            okay &= bool(roll) && bool(hit) && bool(save) && bool(damage) && bool(bulk);
            okay &= damage.value.total >= 0 &&
                    rules::abilityModifier(actor->abilities, rules::Ability::Strength).value == 0;
            okay &= rules::proficiencyBonus(3).value == 2;
        }
        okay &= frames.update(renderer, map, controller.state(), &npcs);
        okay &= !frames.update(renderer, map, controller.state(), &npcs);
        for (int action = 0; action < 6; ++action) {
            controller = Controller({{3, 3}, Heading::North});
            // Revision belongs to one controller/level lifetime, so use a new pipeline
            // only at startup, and synthetic monotonically increasing states here.
            controller.submit(map, static_cast<Action>(action), true, &npcs);
            for (int step = 0; step < 12; ++step) {
                auto state = controller.state();
                state.revision = action + 1;
                frames.update(renderer, map, state, &npcs);
                renderer.compose(map, state.player, destination, &npcs);
                if (state.transition.active()) {
                    const auto& t = state.transition;
                    if (isStrafe(t.action))
                        renderer.composeStrafe(map, t.from, t.to, t.progress(), expected, &npcs);
                    else {
                        renderer.compose(map, t.from, source, &npcs);
                        composeTransition(source, destination, t.action, t.progress(), expected);
                    }
                } else
                    expected.copyFrom(destination);
                okay &= same(frames.frame(), expected);
                controller.update(map, 0.02f, &npcs);
            }
        }
        std::array<bool, 6> down{}, pressed{};
        down[0] = pressed[0] = true;
        repeat.sample(down, pressed, 0.016f);
        pressed.fill(false);
        for (int i = 0; i < 120; ++i)
            repeat.sample(down, pressed, 0.016f);
        repeat.clear();
        controller.clearPending();
        controller = Controller({{1, 1}, Heading::North});
        controller.submit(map, Action::Forward);
        okay &= controller.state().blocked;
        controller.submit(map, Action::Right);
        controller.submit(map, Action::TurnRight);
        controller.update(map, 1, &npcs);
        controller.setAnimated(false);
        auto state = controller.state();
        state.revision = 100;
        frames.update(renderer, map, state, &npcs);
        renderer.compose(map, state.player, expected, &npcs);
        okay &= same(frames.frame(), expected);
        okay &= !frames.update(renderer, map, state, &npcs);
        controller = Controller({{3, 2}, Heading::North});
        controller.submit(map, Action::Forward, true, &npcs);
        okay &= controller.state().blocked;
        controller.submit(map, Action::Back, true, &npcs);
        controller.submit(map, Action::Forward, true, &npcs);
        controller.update(map, 1, &npcs);
        controller.update(map, 1, &npcs);
        controller.submit(map, Action::Forward, true, &npcs);
        okay &= controller.state().blocked;
        state.player = {{3, 3}, Heading::North};
        state.transition.elapsed = Transition::duration;
        state.revision = 101;
        for (std::uint64_t ms = 0; ms < 5000; ms += 17) {
            animator.setTime(ms);
            frames.update(renderer, map, state, &npcs, &animator.snapshot());
            renderer.compose(map, state.player, expected, &npcs, &animator.snapshot());
            okay &= same(frames.frame(), expected);
            okay &= !frames.update(renderer, map, state, &npcs, &animator.snapshot());
        }
        for (int action = 0; action < 6; ++action) {
            const auto kind = static_cast<Action>(action);
            const Player from{{3, 3}, Heading::North};
            Player to = from;
            okay &= tryAction(map, to, kind, &npcs);
            state.player = to;
            ++state.revision;
            for (std::uint64_t ms : {70u, 95u, 120u, 155u}) {
                animator.setTime(ms);
                state.transition = {from, to, kind, Transition::duration * 0.5f};
                frames.update(renderer, map, state, &npcs, &animator.snapshot());
                if (isStrafe(kind))
                    renderer.composeStrafe(map, from, to, 0.5f, expected, &npcs, &animator.snapshot());
                else {
                    renderer.compose(map, from, source, &npcs, &animator.snapshot());
                    renderer.compose(map, to, destination, &npcs, &animator.snapshot());
                    composeTransition(source, destination, kind, 0.5f, expected);
                }
                okay &= same(frames.frame(), expected);
            }
        }
        tracking = false;
        check(okay, "Frame pipeline endpoints, idle reuse, and blocked movement");
        check(allocations == 0, "Engine-owned CPU frame work must not allocate");
        std::cout << "Frame processing: zero C++ allocations\n";
    } catch (const std::exception& e) {
        tracking = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
