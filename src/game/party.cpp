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
    if (x >= rules::partySize || y >= rules::partySize)
        return false;
    std::swap(state_.formation[x], state_.formation[y]);
    return true;
}
} // namespace blobber
