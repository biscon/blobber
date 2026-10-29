#include "app/arena_setup.hpp"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <cstring>
namespace blobber {
void ArenaSetup::filter(const rules::Catalog& c) noexcept {
    visibleCount = 0;
    for (std::uint16_t i = 0; i < c.monsterCount(); ++i) {
        const auto& id = c.monster({i})->id;
        const auto n = std::strlen(search.data());
        bool match = !n;
        for (std::size_t p = 0; !match && p + n <= id.size(); ++p) {
            std::size_t j = 0;
            while (j < n && std::tolower(static_cast<unsigned char>(id[p + j])) ==
                                std::tolower(static_cast<unsigned char>(search[j])))
                ++j;
            match = j == n;
        }
        if (match)
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
            return "A selected monster has no prepared appearance.";
    return nullptr;
}
} // namespace blobber
