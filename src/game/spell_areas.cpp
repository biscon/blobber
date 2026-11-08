#include "game/combat.hpp"
#include "game/perception.hpp"
namespace blobber {
bool spellCell(GeometryView map, Player pose, Cell cell) noexcept {
    return !map.solid(cell) && (cell == pose.cell || inBowRange(pose.cell, cell)) &&
           (cell == pose.cell || inPartyView(map, pose.cell, pose.heading, cell)) &&
           lineOfSight(map, pose.cell, cell);
}
SpellArea spellArea(const ActorWorld& actors, const Party& party, const EffectController& effects,
                    Player pose, rules::SpellKind kind, std::optional<Cell> center,
                    std::optional<SpellAim> aim, unsigned formation) noexcept {
    SpellArea result;
    result.kind = kind;
    const bool sleep = kind == rules::SpellKind::Sleep;
    const bool grease = kind == rules::SpellKind::Grease;
    const bool shatter = kind == rules::SpellKind::Shatter, web = kind == rules::SpellKind::Web;
    const bool color = kind == rules::SpellKind::ColorSpray;
    const bool forwardArea =
        color || kind == rules::SpellKind::BurningHands || kind == rules::SpellKind::Thunderwave;
    if (!forwardArea && !aim && !center)
        return result;
    result.footprint = spellFootprint(actors.map(), pose, formation, kind,
                                      aim.value_or(cellAim(center.value_or(pose.cell))));
    std::size_t cells = 0;
    for (unsigned bit = 0; bit < 64; ++bit) {
        if (!(result.footprint.mask & (UINT64_C(1) << bit)))
            continue;
        const auto c = dungeonCell(result.footprint.tile(bit));
        bool seen = false;
        for (std::size_t i = 0; i < cells; ++i)
            seen |= result.cells[i] == c;
        if (!seen && cells < result.cells.size()) {
            result.cells[cells] = c;
            result.eligible[cells++] = true;
        }
    }
    const auto eligible = [&](EffectTarget t) {
        if (grease || web || shatter)
            return true;
        if (effects.has(t, EffectKind::Sleep) ||
            (!t.npc && party.character(t.character)->life != rules::Life::Conscious))
            return false;
        return sleep ? !effects.sleepImmune(t) : !effects.has(t, EffectKind::Blind);
    };
    for (std::uint16_t slot = 0; slot < rules::partySize; ++slot) {
        const auto i = party.snapshot().formation[slot].index;
        if (!forwardArea && result.footprint.contains(formationSubcell(pose, slot)) &&
            (party.character({i})->life == rules::Life::Conscious ||
             ((web || shatter) && party.character({i})->life != rules::Life::Dead)) &&
            eligible(rules::CharacterId{i})) {
            result.allies[i] = true;
            result.recipients[result.recipientCount++] = rules::CharacterId{i};
        }
    }
    for (std::size_t i = 0; i < actors.snapshot().count; ++i) {
        const auto id = actors.id(i);
        const auto* a = actors.actor(id);
        if (!a || !a->hostile() || a->life != ActorLife::Alive ||
            !result.footprint.contains(globalSubcell(a->npc.cell, a->npc.subcell)) || !eligible(id))
            continue;
        result.actors[result.count++] = id;
        result.recipients[result.recipientCount++] = id;
    }
    return result;
}
SpellArea CombatController::area(const PartyCommand& c, Player pose) const noexcept {
    const auto* spell = party_.catalog().spell(c.spell);
    unsigned formation = 0;
    for (unsigned i = 0; i < rules::partySize; ++i)
        if (party_.snapshot().formation[i].index == c.character.index)
            formation = i;
    auto result = spell ? spellArea(actors_, party_, effects_, pose, spell->kind, c.aimedCell, c.aimedSubcell,
                                    formation)
                        : SpellArea{};
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
