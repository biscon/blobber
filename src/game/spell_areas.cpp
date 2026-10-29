#include "game/combat.hpp"
#include "game/perception.hpp"
namespace blobber {
bool spellCell(const Map& map, Player pose, Cell cell) noexcept {
    return !map.solid(cell) && (cell == pose.cell || inBowRange(pose.cell, cell)) &&
           (cell == pose.cell || inPartyView(map, pose.cell, pose.heading, cell)) &&
           lineOfSight(map, pose.cell, cell);
}
SpellArea spellArea(const ActorWorld& actors, const Party& party, const EffectController& effects,
                    Player pose, rules::SpellKind kind, std::optional<Cell> center) noexcept {
    SpellArea result;
    result.kind = kind;
    const bool sleep = kind == rules::SpellKind::Sleep;
    const bool grease = kind == rules::SpellKind::Grease;
    const bool color = kind == rules::SpellKind::ColorSpray;
    if (!sleep && !grease && !color)
        return spellArea(actors, pose);
    if (sleep || grease) {
        if (!center || !spellCell(actors.map(), pose, *center))
            return result;
        unsigned n = 0;
        const int radius = sleep ? 1 : 0;
        for (int y = -radius; y <= radius; ++y)
            for (int x = -radius; x <= radius; ++x) {
                const Cell c = *center + Cell{x, y};
                result.cells[n] = c;
                result.eligible[n++] = !actors.map().solid(c) && lineOfSight(actors.map(), *center, c);
            }
    } else {
        const auto front = spellArea(actors, pose);
        result.cells = front.cells;
        result.eligible = front.eligible;
    }
    const auto contains = [&](Cell c) {
        for (unsigned i = 0; i < result.cells.size(); ++i)
            if (result.eligible[i] && result.cells[i] == c)
                return true;
        return false;
    };
    const auto eligible = [&](EffectTarget t) {
        if (grease)
            return true;
        if (effects.has(t, EffectKind::Sleep) ||
            (!t.npc && party.character(t.character)->life != rules::Life::Conscious))
            return false;
        return sleep ? !effects.sleepImmune(t) : !effects.has(t, EffectKind::Blind);
    };
    for (std::uint16_t i = 0; i < rules::partySize; ++i)
        if (contains(pose.cell) && party.character({i})->life == rules::Life::Conscious &&
            eligible(rules::CharacterId{i})) {
            result.allies[i] = true;
            result.recipients[result.recipientCount++] = rules::CharacterId{i};
        }
    for (std::size_t i = 0; i < actors.snapshot().count; ++i) {
        const auto id = actors.id(i);
        const auto* a = actors.actor(id);
        if (!a || a->life != ActorLife::Alive || !contains(a->npc.cell) || !eligible(id))
            continue;
        result.actors[result.count++] = id;
        result.recipients[result.recipientCount++] = id;
    }
    return result;
}
SpellArea CombatController::area(const PartyCommand& c, Player pose) const noexcept {
    const auto* spell = party_.catalog().spell(c.spell);
    return spell ? spellArea(actors_, party_, effects_, pose, spell->kind, c.aimedCell) : SpellArea{};
}
} // namespace blobber
