#include "game/campaign.hpp"
#include <algorithm>
#include <cstdio>
#include <random>
#include <stdexcept>
namespace blobber {
namespace {
// Preparation-only progression validation. Heights may remove edges but may not
// make a key depend on its own lock or place a guardian behind its gate.
void validateSurfaceProgression(const CampaignLocation& location) {
    const auto& map = location.map();
    const auto& defs = location.fixtures;
    std::vector<unsigned char> seen(std::size_t(map.width()) * map.height());
    std::vector<Cell> queue;
    queue.reserve(seen.size());
    unsigned keys = 0;
    auto index = [&](Cell c) { return std::size_t(c.y) * map.width() + c.x; };
    for (unsigned pass = 0; pass <= fixtureCapacity; ++pass) {
        std::fill(seen.begin(), seen.end(), 0);
        queue.clear();
        queue.push_back(location.entry.cell);
        seen[index(location.entry.cell)] = 1;
        for (std::size_t head = 0; head < queue.size(); ++head)
            for (unsigned h = 0; h < 4; ++h) {
                const auto from = queue[head], to = from + forward(Heading(h));
                if (!map.canStep(from, to) || seen[index(to)])
                    continue;
                bool blocked = false;
                for (unsigned i = 0; i < defs.count; ++i) {
                    const auto& f = defs.entries[i];
                    if (!(f.cell == to))
                        continue;
                    blocked |= f.kind == FixtureKind::GuardianGate ||
                               (f.kind == FixtureKind::Door && f.key && !(keys & (1u << f.key)));
                }
                if (!blocked) {
                    seen[index(to)] = 1;
                    queue.push_back(to);
                }
            }
        const unsigned old = keys;
        for (unsigned i = 0; i < defs.count; ++i)
            if (defs.entries[i].kind == FixtureKind::Key && seen[index(defs.entries[i].cell)])
                keys |= 1u << defs.entries[i].key;
        if (keys == old)
            break;
    }
    for (unsigned i = 0; i < defs.count; ++i)
        if (defs.entries[i].kind == FixtureKind::Key && !(keys & (1u << defs.entries[i].key)))
            throw std::invalid_argument("Saved heights break key progression");
    if (location.generated)
        for (unsigned i = 0; i < location.generated->layout.populationCount; ++i) {
            const auto& p = location.generated->layout.population[i];
            if (p.guardian && !seen[index(p.cell)])
                throw std::invalid_argument("Saved heights hide guardians behind their gate");
        }
}
} // namespace
std::string campaignRunId() {
    std::random_device entropy;
    char id[33];
    for (unsigned i = 0; i < 4; ++i)
        std::snprintf(id + i * 8, 9, "%08x", unsigned(entropy()));
    return id;
}
void CampaignRun::preparePersistenceIdentity(std::string id) {
    if (id.size() != 32 || id.find_first_not_of("0123456789abcdef") != std::string::npos)
        throw std::invalid_argument("Invalid campaign run identity");
    runId_ = std::move(id);
}
void GameSession::restorePose(Player pose, bool animated) {
    if (unsigned(pose.heading) > 3 || map_.solid(pose.cell) || actors_.occupied(pose.cell))
        throw std::invalid_argument("Invalid saved player pose");
    movement_.state_.player = pose;
    movement_.state_.transition = {};
    movement_.state_.blocked = false;
    movement_.state_.animated = animated;
    movement_.clearPending();
    effects_.partyCell(pose.cell);
    discoveredReady_ = false;
}
std::unique_ptr<CampaignCheckpoint> CampaignRun::checkpoint() const {
    if (const auto* error = saveSafety())
        throw std::invalid_argument(error);
    auto out = std::make_unique<CampaignCheckpoint>();
    out->party = party_.checkpoint();
    out->pose = game().movement().player;
    out->animated = game().movement().animated;
    out->activeLocation = location_;
    out->gameplayMs = gameplayMs();
    out->revision = revision_;
    out->runId = runId_;
    out->navigation = game().navigationCheckpoint();
    for (unsigned i = 0; i < 4; ++i) {
        const auto& from = locations_[i];
        auto& to = out->locations[i];
        to.world = from.map().definition();
        to.worldFingerprint = to.world.fingerprint();
        to.generated = bool(from.generated);
        to.visited = from.visited;
        if (from.generated) {
            to.generation = from.generated->request;
            to.layout = from.generated->layout;
            to.diagnostics = from.generated->diagnostics;
            to.fingerprint = from.generated->fingerprint;
        }
        for (int y = 0; y < from.map().height(); ++y) {
            std::string row;
            for (int x = 0; x < from.map().width(); ++x)
                row += from.map().solid({x, y}) ? '#' : '.';
            to.rows.push_back(std::move(row));
        }
        for (unsigned t = 0; t < from.placements->typeCount(); ++t)
            to.types.push_back(from.placements->typeId({std::uint16_t(t)}));
        to.placements = from.placements->instances();
        to.recovery = from.recovery;
        to.fixtures = from.fixtures;
        to.objective = from.objective;
        to.entry = from.entry;
        to.exit = from.exit;
        to.seed = from.seed;
        if (i == location_) {
            to.state = std::make_unique<FloorState>();
            game().exportFloor(*to.state);
        } else if (from.saved)
            to.state = std::make_unique<FloorState>(*from.saved);
    }
    return out;
}
CampaignRun::CampaignRun(const rules::Catalog& catalog, CampaignCheckpoint&& s,
                         const std::vector<NpcTimings>& timings)
    : runId_(s.runId), party_(catalog, s.party), timings_(timings), location_(s.activeLocation),
      revision_(s.revision), elapsed_(s.gameplayMs) {
    if (runId_.size() != 32 || runId_.find_first_not_of("0123456789abcdef") != std::string::npos ||
        location_ > 3 || elapsed_ > maxCheckpointClock || !revision_ || revision_ == UINT64_MAX ||
        !s.locations[0].visited || !s.locations[location_].visited || !s.locations[location_].state)
        throw std::invalid_argument("Invalid campaign checkpoint");
    for (unsigned i = 0; i < 4; ++i) {
        auto& from = s.locations[i];
        auto& to = locations_[i];
        if (from.generated != (i != 0) || from.rows.empty() || from.rows.size() > 40 ||
            from.rows.front().empty() || from.rows.front().size() > 40 || from.placements.size() > 32 ||
            from.types.empty() || from.types.size() != timings.size() ||
            (i && from.visited && !s.locations[i - 1].visited) || (i && bool(from.state) != from.visited))
            throw std::invalid_argument("Invalid campaign location definition");
        if (from.world.surfaces.empty() || from.world.fingerprint() != from.worldFingerprint)
            throw std::invalid_argument("Saved world definition fingerprint differs");
        Map map(from.rows, std::move(from.world));
        if (!map.connected(from.entry.cell))
            throw std::invalid_argument("Saved surfaces disconnect the floor");
        if (map.width() * map.height() > int(generation::maxCells))
            throw std::invalid_argument("Campaign map capacity exceeded");
        if (i) {
            for (auto value : from.layout.floor)
                if (value > 1)
                    throw std::invalid_argument("Invalid saved tile value");
            if (from.seed != std::uint32_t(generation::seedFor(from.generation, generation::Domain::Runtime)))
                throw std::invalid_argument("Saved runtime seed differs from provenance");
            if (from.generation.depth != i || from.generation.generator != generation::generatorVersion ||
                from.generation.templates != generation::templateVersion || from.layout.depth != i ||
                from.layout.width != map.width() || from.layout.height != map.height() ||
                generation::validate(from.layout) || generation::fingerprint(from.layout) != from.fingerprint)
                throw std::invalid_argument("Invalid saved generated floor");
            for (int y = 0; y < map.height(); ++y)
                for (int x = 0; x < map.width(); ++x)
                    if (map.solid({x, y}) == bool(from.layout.floor[unsigned(y * map.width() + x)]))
                        throw std::invalid_argument("Saved map/layout mismatch");
            to.generated.emplace(generation::GeneratedFloor{from.generation, from.layout, std::move(map),
                                                            from.diagnostics, from.fingerprint, from.seed});
        } else {
            if (map.width() != 9 || map.height() != 9 || !(from.entry == Player{{4, 4}, Heading::East}) ||
                !(from.exit == from.entry) || !from.placements.empty() || from.objective.count ||
                from.recovery.groundCount || from.recovery.sanctuaryCount != 1 ||
                !(from.recovery.sanctuaries[0] == Cell{4, 4}) || from.fixtures.count != 1 ||
                from.fixtures.discovery || from.fixtures.depth || from.fixtures.landingCount != 1 ||
                !(from.fixtures.landings[0] == Cell{4, 4}) ||
                from.fixtures.entries[0].kind != FixtureKind::Downstairs ||
                !(from.fixtures.entries[0].cell == Cell{5, 4}) ||
                from.fixtures.entries[0].facing != Heading::East || from.fixtures.npcExcluded.count() != 2 ||
                !from.fixtures.npcExcluded[40] || !from.fixtures.npcExcluded[41])
                throw std::invalid_argument("Unsupported authored hub definition");
            for (int y = 0; y < 9; ++y)
                for (int x = 0; x < 9; ++x)
                    if (map.solid({x, y}) != (x == 0 || y == 0 || x == 8 || y == 8))
                        throw std::invalid_argument("Unsupported authored hub map");
            to.authored.emplace(std::move(map));
        }
        to.entry = from.entry;
        to.exit = from.exit;
        to.seed = from.seed;
        to.visited = from.visited;
        to.fixtures = from.fixtures;
        to.objective = from.objective;
        to.recovery = from.recovery;
        if (unsigned(to.entry.heading) > 3 || unsigned(to.exit.heading) > 3 ||
            to.map().solid(to.entry.cell) || to.map().solid(to.exit.cell))
            throw std::invalid_argument("Invalid saved arrival");
        to.placements.emplace(to.map(), to.entry.cell, from.types, from.placements);
        to.saved = std::move(from.state);
        auto checked = prepare(i, to.entry, true);
        validateSurfaceProgression(to);
        if (i != location_ && to.saved) {
            for (unsigned c = 0; c < 4; ++c)
                for (const auto& condition : to.saved->effects.conditions[c])
                    if (condition.active)
                        throw std::invalid_argument("Inactive floor owns a party condition");
            for (const auto& e : to.saved->effects.instances) {
                if (!e.active)
                    continue;
                bool local = e.kind == EffectKind::Web || e.kind == EffectKind::SpiritualWeapon;
                for (unsigned t = 0; t < e.count; ++t)
                    local |= e.targets[t].npc;
                if (!local)
                    throw std::invalid_argument("Inactive floor owns a portable effect");
            }
        }
    }
    for (unsigned depth = 1; depth <= 3; ++depth) {
        const auto& l = locations_[depth];
        if (depth > 1 && l.visited &&
            (!locations_[depth - 1].saved || !locations_[depth - 1].saved->objective.completed))
            throw std::invalid_argument("Visited depth bypasses its guardian prerequisite");
        for (unsigned f = 0; f < l.fixtures.count; ++f) {
            const auto& fixture = l.fixtures.entries[f];
            if (fixture.kind == FixtureKind::Key && bool(party_.questKeys() & (1u << fixture.key)) !=
                                                        bool(l.saved && l.saved->fixtures.completed[f]))
                throw std::invalid_argument("Saved campaign key ownership mismatch");
        }
    }
    if (locations_[3].saved && locations_[3].saved->objective.completed)
        throw std::invalid_argument("Completed campaign cannot resume as active");
    active_ = prepare(location_, locations_[location_].entry);
    active_->restorePose(s.pose, s.animated);
    active_->restoreNavigation(s.navigation);
    baseClock_ = game().combat().snapshot().gameplayMs;
    active_->campaign_ = this;
    for (unsigned i = 0; i < 4; ++i) {
        bool aided = false;
        for (const auto& e : game().effects().snapshot())
            if (e.active && e.kind == EffectKind::Aid)
                for (unsigned t = 0; t < e.count; ++t)
                    aided |= !e.targets[t].npc && e.targets[t].character.index == i;
        if (party_.character({std::uint16_t(i)})->aidBonus != (aided ? 5 : 0))
            throw std::invalid_argument("Saved Aid ownership differs from party resources");
    }
    if (status() != CampaignStatus::Active || saveSafety())
        throw std::invalid_argument("Saved campaign is terminal or unsafe");
}
} // namespace blobber
