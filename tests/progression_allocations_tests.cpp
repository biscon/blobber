#include "game/commands.hpp"
#include "support/check.hpp"
#include "support/progression_fixtures.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>
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
using namespace blobber::rules;
int main() {
    try {
        const auto catalog = creationCatalog(true);
        const auto map = hybridRoom();
        const auto empty = spellPlacements(map, 0), places = spellPlacements(map, 3);
        GameSession progression(map, catalog, empty, 5, spellSpawn, false);
        CommandController ui(progression);
        std::array<std::unique_ptr<GameSession>, 8> casts;
        for (unsigned i = 0; i < casts.size(); ++i) {
            casts[i] = std::make_unique<GameSession>(map, catalog, places, 5, spellSpawn, false);
            levelParty(*casts[i], 3, SpellKind(unsigned(SpellKind::Aid) + i));
        }
        DiceRng rng(5);
        Party party(catalog);
        ActorWorld actors(map, catalog, places);
        EffectController effects(party, actors, rng);
        CombatEvents events;
        effects.partyCell(spellSpawn.cell);
        bool okay = true;
        tracking = true;
        progression.enableDevelopment(true);
        for (int level = 2; level <= 3; ++level) {
            okay &= progression.grantNextLevelExperience();
            for (std::uint16_t id = 0; id < 4; ++id) {
                ui.openAdvancement({id});
                ui.closeOverlay();
                auto r = progression.advancementRequest({id});
                const auto& c = *progression.party().character({id});
                const auto context =
                    advancementChoices(catalog, c.build.view(), c.build.portrait, c.build.spells, {}, true);
                r.choices.learned = recommendedChoices(catalog, context, SpellChoice::Learned);
                okay &= bool(progression.previewAdvancement(r));
                okay &= bool(progression.advance(r));
                okay &= !progression.advance(r);
                ui.refresh({}, false, false, true);
                ui.openInventory(CharacterId{id}, CharacterPage::Character);
                ui.closeOverlay();
            }
        }
        for (unsigned i = 0; i < casts.size(); ++i) {
            auto& s = *casts[i];
            const auto kind = SpellKind(unsigned(SpellKind::Aid) + i);
            auto c = spellCommand(s, kind);
            if (kind == SpellKind::Aid) {
                c.targetCount = 3;
                c.allies = {{{0}, {1}, {2}}};
            }
            if (kind == SpellKind::HoldPerson || kind == SpellKind::SpiritualWeapon ||
                kind == SpellKind::ScorchingRay) {
                c.targetCount = 1;
                c.enemies[0] = s.actors().id(0);
                c.darts[0] = 3;
            }
            if (kind == SpellKind::Shatter || kind == SpellKind::Web)
                c.aimedCell = Cell{6, 4};
            if (kind == SpellKind::LesserRestoration) {
                EffectRequest r;
                r.world = s.actors().snapshot().world;
                r.caster = {3};
                r.kind = EffectKind::HoldPerson;
                r.targets[0] = CharacterId{0};
                r.count = 1;
                r.difficulty = 100;
                okay &= bool(s.applyEffect(r));
                c.targetCount = 1;
                c.allies[0] = {0};
                c.restoreParalysis = true;
            }
            okay &= s.submit(c).reason == CommandReason::Resolved;
            okay &= s.submit(c).reason != CommandReason::Resolved;
            for (unsigned j = 0; j < 8; ++j)
                s.update(.25, false);
        }
        SpellSelection selection;
        selection.context.available = classSpells(catalog, {3}, 1);
        selection.filter(catalog);
        selection.detailLines = 12;
        selection.detailPageLines = 3;
        UiInput scroll;
        scroll.pointer = ArtPoint{500, 240};
        scroll.scroll = -1;
        okay &= scrollSelection(selection, catalog, scroll, {});
        okay &= selection.detailOffset == 1;
        scroll.pointer = ArtPoint{300, 150};
        okay &= scrollSelection(selection, catalog, scroll, {});
        okay &= selection.offset > 0;
        for (unsigned repeat = 0; repeat < 10; ++repeat) {
            events.clear();
            EffectRequest r;
            r.world = actors.snapshot().world;
            r.caster = {3};
            r.kind = EffectKind::MirrorImage;
            r.targets[0] = CharacterId{3};
            r.count = 1;
            const auto result = effects.apply(r, 0, events);
            okay &= bool(result);
            for (unsigned n = 0; n < 6; ++n)
                effects.resolveMirror(effects.mirror(actors.id(0), {3}, 20, 100), n % 2, 0, events);
            const std::array<Cell, 4> cells{{{5, 4}, {6, 4}, {5, 5}, {6, 5}}};
            okay &= effects.createWeb(cells.data(), 4, {3}, 10, 0, events);
            effects.ignite({6, 4});
            effects.advance(effects.gameplayMs() + 6000, (repeat + 1) * 6000, true, events);
        }
        tracking = false;
        check(okay, "Progression allocation workload must execute successfully");
        check(allocations == 0, "Progression, spell and effect operations must not allocate");
        std::cout << "Progression and level spells: zero engine-owned C++ allocations\n";
    } catch (const std::exception& e) {
        tracking = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
