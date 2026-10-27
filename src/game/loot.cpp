#include "game/loot.hpp"
#include <stdexcept>
namespace blobber {
LootWorld::LootWorld(const ActorWorld& actors, const RecoveryLevel& level, std::uint32_t seed) {
    state_.world = actors.snapshot().world;
    if (level.groundCount > level.ground.size() || level.sanctuaryCount > level.sanctuaries.size())
        throw std::runtime_error("Recovery level capacity exceeded");
    for (std::size_t i = 0; i < level.sanctuaryCount; ++i)
        if (actors.map().solid(level.sanctuaries[i]))
            throw std::runtime_error("Sanctuary must be walkable");
    for (std::size_t i = 0; i < actors.snapshot().count; ++i) {
        rules::DiceRng rng(seed ^ 0x4c4f4f54u ^ (std::uint32_t(i) * 0x9e3779b9u));
        const auto* m = actors.catalog().monster(actors.snapshot().actors[i].monster);
        const auto roll = rng.roll({1, 100, 0}).value.total;
        for (std::size_t j = 0; j < m->lootCount; ++j) {
            const auto& e = m->loot[j];
            if (roll < e.first || roll > e.last)
                continue;
            const auto q = e.sides ? rng.roll({e.quantity, e.sides, 0}).value.total : e.quantity;
            state_.stacks[i * lootPerActor + j] = {{}, e.item, static_cast<std::uint16_t>(q), false};
        }
    }
    for (std::size_t i = 0; i < level.groundCount; ++i) {
        const auto& p = level.ground[i];
        const auto* d = actors.catalog().item(p.item);
        if (actors.map().solid(p.cell) || !d || !p.quantity || p.quantity > rules::stackLimit(*d))
            throw std::runtime_error("Invalid ground loot placement");
        state_.stacks[NpcWorld::maxInstances * lootPerActor + i] = {p.cell, p.item, p.quantity, true};
    }
}
const FloorLoot* LootWorld::find(LootId id) const noexcept {
    if (id.world != state_.world || id.slot >= lootCapacity)
        return nullptr;
    const auto& s = state_.stacks[id.slot];
    return s.active && s.quantity ? &s : nullptr;
}
void LootWorld::reconcile(const ActorWorld& actors) noexcept {
    if (actors.snapshot().world != state_.world || actorRevision_ == actors.snapshot().revision)
        return;
    actorRevision_ = actors.snapshot().revision;
    for (std::size_t i = 0; i < actors.snapshot().count; ++i) {
        const auto& a = actors.snapshot().actors[i];
        if (!a.hasDeath || deaths_[i])
            continue;
        deaths_[i] = true;
        for (std::size_t j = 0; j < lootPerActor; ++j) {
            auto& s = state_.stacks[i * lootPerActor + j];
            if (!s.quantity)
                continue;
            s.cell = a.npc.cell;
            s.active = true;
        }
        ++state_.revision;
    }
}
bool LootWorld::collect(LootId id) noexcept {
    if (!find(id) || state_.revision == UINT64_MAX)
        return false;
    state_.stacks[id.slot].active = false;
    state_.stacks[id.slot].quantity = 0;
    ++state_.revision;
    return true;
}
} // namespace blobber
