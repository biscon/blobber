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
    const bool shatter = kind == rules::SpellKind::Shatter, web = kind == rules::SpellKind::Web;
    const bool color = kind == rules::SpellKind::ColorSpray;
    if (!sleep && !grease && !color && !shatter && !web)
        return spellArea(actors, pose);
    if (web) {
        if (!center || !spellCell(actors.map(), pose, *center))
            return result;
        unsigned n = 0;
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 2; ++x) {
                const auto c = *center + forward(pose.heading) * y + right(pose.heading) * x;
                result.cells[n] = c;
                result.eligible[n++] = !actors.map().solid(c) && lineOfSight(actors.map(), *center, c);
            }
    } else if (sleep || grease || shatter) {
        if (!center || !spellCell(actors.map(), pose, *center))
            return result;
        unsigned n = 0;
        const int radius = sleep || shatter ? 1 : 0;
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
        if (grease || web || shatter)
            return true;
        if (effects.has(t, EffectKind::Sleep) ||
            (!t.npc && party.character(t.character)->life != rules::Life::Conscious))
            return false;
        return sleep ? !effects.sleepImmune(t) : !effects.has(t, EffectKind::Blind);
    };
    for (std::uint16_t i = 0; i < rules::partySize; ++i)
        if (contains(pose.cell) &&
            (party.character({i})->life == rules::Life::Conscious ||
             ((web || shatter) && party.character({i})->life != rules::Life::Dead)) &&
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
    auto result = spell ? spellArea(actors_, party_, effects_, pose, spell->kind, c.aimedCell) : SpellArea{};
    if (spell && spell->kind == rules::SpellKind::Shatter &&
        party_.progressionClass(c.character) == rules::ProgressionClass::Wizard &&
        party_.character(c.character)->build.level >= 2) {
        for (std::size_t i = 0; i < result.recipientCount;) {
            const auto t = result.recipients[i];
            if (!t.npc && t.character.index != c.character.index) {
                result.allies[t.character.index] = false;
                for (std::size_t j = i + 1; j < result.recipientCount; ++j)
                    result.recipients[j - 1] = result.recipients[j];
                --result.recipientCount;
            } else
                ++i;
        }
    }
    return result;
}
} // namespace blobber
