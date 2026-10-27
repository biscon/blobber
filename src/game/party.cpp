#include "game/party.hpp"
#include <stdexcept>
#include <utility>

namespace blobber {
Party::Party(const rules::Catalog& catalog) : catalog_(&catalog) {
    const auto* preset = catalog.preset();
    if (!preset)
        throw std::runtime_error("Starting party: catalog has no preset_party (schema 2 required)");
    state_.formation = preset->formation;
    for (std::size_t i = 0; i < rules::partySize; ++i) {
        auto& c = state_.characters[i];
        c.build = preset->characters[i];
        const auto stats = rules::deriveCharacter(catalog, c.build);
        if (!stats)
            throw std::runtime_error(rules::characterErrorMessage(stats.error));
        c.stats = stats.value;
        c.hitPoints = c.stats.maxHitPoints;
        c.availableHitDice = c.build.level;
    }
}
const PartyCharacter* Party::character(rules::CharacterId id) const noexcept {
    return id.index < rules::partySize ? &state_.characters[id.index] : nullptr;
}
const PartyCharacter* Party::occupant(FormationSlot slot) const noexcept {
    const auto index = static_cast<unsigned>(slot);
    return index < rules::partySize ? character(state_.formation[index]) : nullptr;
}
bool Party::swapFormation(FormationSlot a, FormationSlot b) noexcept {
    const auto x = static_cast<unsigned>(a), y = static_cast<unsigned>(b);
    if (formationLocked_ || x >= rules::partySize || y >= rules::partySize)
        return false;
    std::swap(state_.formation[x], state_.formation[y]);
    return true;
}
rules::HealthResult Party::commit(rules::CharacterId id, rules::HealthResult result) noexcept {
    if (result && result.changed)
        static_cast<rules::HealthState&>(state_.characters[id.index]) = result.state;
    return result;
}
rules::HealthResult Party::damage(rules::CharacterId id, std::int64_t amount, bool critical) noexcept {
    const auto* c = character(id);
    return c ? commit(id, rules::takeDamage(*c, c->stats.maxHitPoints, amount, critical))
             : rules::HealthResult{{}, rules::HealthError::InvalidCharacter};
}
rules::HealthResult Party::heal(rules::CharacterId id, std::int64_t amount) noexcept {
    const auto* c = character(id);
    return c ? commit(id, rules::receiveHealing(*c, c->stats.maxHitPoints, amount))
             : rules::HealthResult{{}, rules::HealthError::InvalidCharacter};
}
rules::HealthResult Party::stabilize(rules::CharacterId id) noexcept {
    const auto* c = character(id);
    return c ? commit(id, rules::stabilize(*c, c->stats.maxHitPoints))
             : rules::HealthResult{{}, rules::HealthError::InvalidCharacter};
}
rules::HealthResult Party::deathSave(rules::CharacterId id, int face, int modifier) noexcept {
    const auto* c = character(id);
    return c ? commit(id, rules::deathSave(*c, c->stats.maxHitPoints, face, modifier))
             : rules::HealthResult{{}, rules::HealthError::InvalidCharacter};
}
} // namespace blobber
