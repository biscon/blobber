#include "app/arena_setup.hpp"
#include "ui/player_text.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstring>
namespace blobber {
bool ArenaSetup::replaceParty(const StartingParty& selected) noexcept {
    bool changed = false;
    for (unsigned i = 0; i < 4; ++i) {
        const auto& a = scenario.party[i];
        const auto& b = selected[i];
        const auto& x = a.draft;
        const auto& y = b.draft;
        changed |= a.id != b.id || x.name != y.name || x.race.index != y.race.index ||
                   x.characterClass.index != y.characterClass.index || x.portrait.index != y.portrait.index ||
                   x.abilities != y.abilities || x.defense != y.defense || x.lifeDomain != y.lifeDomain ||
                   x.gear != y.gear || x.spells.cantrips != y.spells.cantrips ||
                   x.spells.learned != y.spells.learned || x.spells.prepared != y.spells.prepared ||
                   x.spells.domain != y.spells.domain || x.spells.racial != y.spells.racial;
    }
    if (changed) {
        scenario.party = selected;
        scenario.preparation = {};
    }
    return changed;
}
void ArenaSetup::filter(const rules::Catalog& c) noexcept {
    visibleCount = 0;
    const auto n = std::strlen(search.data());
    const auto matches = [&](std::string_view name) {
        for (std::size_t p = 0; p + n <= name.size(); ++p) {
            std::size_t j = 0;
            while (j < n && std::tolower(static_cast<unsigned char>(name[p + j])) ==
                                std::tolower(static_cast<unsigned char>(search[j])))
                ++j;
            if (j == n)
                return true;
        }
        return false;
    };
    for (std::uint16_t i = 0; i < c.monsterCount(); ++i) {
        const auto& id = c.monster({i})->id;
        if (matches(monsterName(id)) || matches(id))
            visible[visibleCount++] = i;
    }
    first = std::min(first, visibleCount > 5 ? visibleCount - 5 : 0);
}
void ArenaSetup::syncSeed() noexcept {
    std::snprintf(seedText.data(), seedText.size(), "%u", scenario.seed);
}
bool ArenaSetup::readSeed() noexcept {
    std::uint32_t seed = 0;
    const auto* end = seedText.data() + std::strlen(seedText.data());
    auto result = std::from_chars(seedText.data(), end, seed);
    if (result.ec != std::errc{} || result.ptr != end)
        return false;
    scenario.seed = seed;
    return true;
}
const char* ArenaSetup::error(const rules::Catalog& c) noexcept {
    if (!readSeed())
        return "Seed must be a whole number from 0 to 4294967295.";
    if (const auto* error = arenaError(c, scenario))
        return error;
    for (std::size_t i = 0; i < scenario.count; ++i)
        if (!available[scenario.roster[i].monster.index])
            return "A selected monster is unavailable. Choose another monster.";
    return nullptr;
}
} // namespace blobber
