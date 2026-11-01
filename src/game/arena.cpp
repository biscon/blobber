#include "game/arena.hpp"
#include "game/perception.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>
namespace blobber {
unsigned ArenaScenario::total() const noexcept {
    unsigned n = 0;
    for (std::size_t i = 0; i < std::min(count, roster.size()); ++i) {
        if (roster[i].quantity > NpcWorld::maxInstances - n)
            return NpcWorld::maxInstances + 1;
        n += roster[i].quantity;
    }
    return n;
}
bool ArenaScenario::adjust(rules::MonsterId id, int delta) noexcept {
    if (count > roster.size())
        return false;
    if (delta != 1 && delta != -1)
        return false;
    if (delta > 0 && total() >= NpcWorld::maxInstances)
        return false;
    for (std::size_t i = 0; i < count; ++i)
        if (roster[i].monster.index == id.index) {
            if (delta < 0 && !roster[i].quantity)
                return false;
            roster[i].quantity = unsigned(int(roster[i].quantity) + delta);
            if (!roster[i].quantity) {
                for (std::size_t j = i + 1; j < count; ++j)
                    roster[j - 1] = roster[j];
                roster[--count] = {};
            }
            return true;
        }
    if (delta < 0 || count == roster.size())
        return false;
    roster[count++] = {id, 1};
    return true;
}
ArenaScenario defaultArena(const rules::Catalog& c) {
    ArenaScenario s;
    std::snprintf(s.name.data(), s.name.size(), "New scenario");
    if (!c.preset())
        throw std::runtime_error("Default party unavailable");
    for (unsigned i = 0; i < 4; ++i) {
        const auto& b = c.preset()->characters[c.preset()->formation[i].index];
        if (!rules::draftFromBuild(c, b, s.party[i].draft))
            throw std::runtime_error("Invalid default character");
        std::snprintf(s.party[i].id.data(), s.party[i].id.size(), "%s", b.id.c_str());
    }
    return s;
}
const char* arenaError(const rules::Catalog& c, const ArenaScenario& s) noexcept {
    if (!rules::validDisplayText(s.name.data(), 64))
        return "Enter a scenario name.";
    for (unsigned i = 0; i < 4; ++i) {
        if (!s.party[i].id[0] ||
            std::find(s.party[i].id.begin(), s.party[i].id.end(), '\0') == s.party[i].id.end() ||
            !rules::previewBuild(c, s.party[i].draft))
            return "Choose four valid level-one characters.";
        for (unsigned j = 0; j < i; ++j)
            if (!std::strcmp(s.party[i].id.data(), s.party[j].id.data()))
                return "Choose four distinct characters.";
    }
    if (!s.count || s.count > s.roster.size() || !s.total() || s.total() > NpcWorld::maxInstances)
        return "Choose between 1 and 256 monsters.";
    for (std::size_t i = 0; i < s.count; ++i) {
        if (!s.roster[i].quantity || !c.monster(s.roster[i].monster))
            return "Monster unavailable.";
        for (std::size_t j = 0; j < i; ++j)
            if (s.roster[i].monster.index == s.roster[j].monster.index)
                return "Duplicate monster roster entry.";
    }
    return arenaPreparationError(c, s.party, s.preparation);
}
rules::PresetParty arenaParty(const rules::Catalog& c, const ArenaScenario& s) {
    if (const auto* error = arenaError(c, s))
        throw std::runtime_error(error);
    return startingParty(c, s.party);
}
const char* arenaPreparationError(const rules::Catalog& c, const StartingParty& starting,
                                  const ArenaPreparation& p) noexcept {
    if (p.targetLevel < 1 || p.targetLevel > 3)
        return "Arena level must be between 1 and 3.";
    for (unsigned i = 0; i < 4; ++i) {
        const auto preview = rules::previewBuild(c, starting[i].draft);
        if (!preview)
            return "Choose four valid level-one characters.";
        auto identity = rules::draftIdentity(starting[i].draft);
        auto spells = starting[i].draft.spells;
        for (unsigned step = 0; step < 2; ++step) {
            const auto choices = p.advancements[step][i];
            if (step + 1 >= p.targetLevel) {
                if (choices.learned || choices.prepared)
                    return "Unexpected arena advancement choices above the target level.";
                continue;
            }
            const auto next = rules::previewAdvancement(c, identity, spells, preview.loadout, choices);
            if (!next.valid)
                return "Complete each arena character's advancement spell choices.";
            ++identity.level;
            spells = next.spells;
        }
    }
    return nullptr;
}
std::unique_ptr<Party> prepareArenaParty(const rules::Catalog& c, const ArenaScenario& s) {
    auto party = std::make_unique<Party>(c, arenaParty(c, s));
    for (unsigned step = 0; step + 1 < s.preparation.targetLevel; ++step) {
        party->grantNextLevelExperience();
        for (std::uint16_t i = 0; i < 4; ++i) {
            auto request = party->advancementRequest({i});
            request.choices = s.preparation.advancements[step][i];
            if (!party->advance(request))
                throw std::invalid_argument("Arena party advancement failed.");
        }
    }
    return party;
}
Map arenaMap() {
    std::vector<std::string> rows(25, std::string(21, '.'));
    rows.front() = rows.back() = std::string(21, '#');
    for (auto& row : rows)
        row.front() = row.back() = '#';
    return Map(rows);
}
Player arenaSpawn() noexcept {
    return {{10, 21}, Heading::North};
}
NpcWorld arenaPlacements(const Map& map, const rules::Catalog& c, const ArenaScenario& s,
                         const std::vector<std::string>& types) {
    if (const auto* error = arenaError(c, s))
        throw std::runtime_error(error);
    if (!map.connected(arenaSpawn().cell))
        throw std::runtime_error("Disconnected arena");
    std::array<Cell, 21 * 25> queue{};
    std::array<bool, 21 * 25> visited{};
    std::size_t head = 0, tail = 1;
    queue[0] = {10, 7};
    visited[7 * 21 + 10] = true;
    constexpr Cell directions[]{{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
    while (head < tail && head < 64) {
        const auto here = queue[head++];
        for (auto d : directions) {
            const auto next = here + d;
            if (next.x <= 0 || next.x >= 20 || next.y <= 0 || next.y >= 24)
                continue;
            auto& seen = visited[std::size_t(next.y * 21 + next.x)];
            if (!seen) {
                seen = true;
                queue[tail++] = next;
            }
        }
    }
    std::vector<NpcInstance> actors;
    actors.reserve(s.total());
    for (std::size_t r = 0; r < s.count; ++r) {
        const auto& entry = s.roster[r];
        const auto& id = c.monster(entry.monster)->id;
        const auto found = std::find(types.begin(), types.end(), monsterAppearance(id));
        if (found == types.end())
            throw std::runtime_error("Monster appearance unavailable: " + id);
        for (unsigned n = 0; n < entry.quantity; ++n) {
            const auto slot = actors.size();
            const auto cell = queue[slot / 4];
            if (map.solid(cell) || inBowRange(cell, arenaSpawn().cell))
                throw std::runtime_error("Unsafe arena spawn");
            NpcInstance actor{{std::uint16_t(found - types.begin())},
                              cell,
                              {int(slot % 2), int((slot % 4) / 2)},
                              Heading::South};
            actor.monster = entry.monster;
            actors.push_back(actor);
        }
    }
    return NpcWorld(map, arenaSpawn().cell, types, std::move(actors));
}
bool arenaCleared(const ActorSnapshot& s) noexcept {
    if (!s.count)
        return false;
    for (std::size_t i = 0; i < s.count; ++i)
        if (!s.actors[i].hasDeath)
            return false;
    return true;
}
} // namespace blobber
