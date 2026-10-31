#include "world/fixtures.hpp"
#include "game/perception.hpp"
#include <atomic>
#include <stdexcept>
namespace blobber {
namespace {
std::uint64_t nextFloor() {
    static std::atomic<std::uint64_t> next{1};
    auto n = next.load(std::memory_order_relaxed);
    for (;;) {
        if (n == UINT64_MAX)
            throw std::runtime_error("Floor identity exhausted");
        if (next.compare_exchange_weak(n, n + 1, std::memory_order_relaxed))
            return n;
    }
}
} // namespace
const char* fixtureName(const FixtureDefinition& f) noexcept {
    switch (f.kind) {
    case FixtureKind::Door:
        return f.key == 2 ? "Bronze seal door" : f.key == 3 ? "Silver seal door" : "Dungeon door";
    case FixtureKind::Key:
        return f.key == 2 ? "Bronze seal key" : "Silver seal key";
    case FixtureKind::Upstairs:
        return "Stairs up";
    case FixtureKind::Downstairs:
        return "Stairs down";
    case FixtureKind::GuardianGate:
        return "Guardian gate";
    case FixtureKind::Endpoint:
        return "Floor objective";
    }
    return "Fixture";
}
FloorGeometry::FloorGeometry(const Map& map, const FloorFixtures& defs)
    : map_(map), definition_(defs), world_(nextFloor()) {
    if (defs.depth > 3 || defs.count > fixtureCapacity || defs.landingCount > defs.landings.size() ||
        ((defs.landingCount || defs.npcExcluded.any()) &&
         map.width() * map.height() > int(discoveryCapacity)) ||
        (defs.discovery && map.width() * map.height() > int(discoveryCapacity)))
        throw std::invalid_argument("Floor fixture/discovery capacity exceeded");
    if (defs.count)
        index_.assign(std::size_t(map.width()) * map.height(), UINT8_MAX);
    unsigned keys = 0, locks = 0;
    for (unsigned i = 0; i < defs.count; ++i) {
        const auto& f = defs.entries[i];
        if (map.solid(f.cell) || unsigned(f.kind) > unsigned(FixtureKind::GuardianGate) ||
            unsigned(f.facing) > 3 || (f.key != 0 && f.key != 2 && f.key != 3) ||
            (f.kind == FixtureKind::Key && !f.key) ||
            (f.kind != FixtureKind::Key && f.kind != FixtureKind::Door && f.key))
            throw std::invalid_argument("Invalid floor fixture");
        auto& cell = index_[std::size_t(f.cell.y * map.width() + f.cell.x)];
        if (cell != UINT8_MAX)
            throw std::invalid_argument("Overlapping floor fixtures");
        cell = std::uint8_t(i);
        if (f.key) {
            auto& bits = f.kind == FixtureKind::Key ? keys : locks;
            if (bits & (1u << f.key))
                throw std::invalid_argument("Duplicate key or lock");
            bits |= 1u << f.key;
        }
    }
    for (unsigned i = 0; i < defs.landingCount; ++i) {
        const auto c = defs.landings[i];
        if (map.solid(c) || !defs.npcExcluded[std::size_t(c.y * map.width() + c.x)])
            throw std::invalid_argument("Invalid protected landing");
    }
    if (keys != locks)
        throw std::invalid_argument("Unmatched key and lock");
}
const FixtureDefinition* FloorGeometry::find(FixtureId id) const noexcept {
    return id.world == world_ && id.slot < definition_.count ? &definition_.entries[id.slot] : nullptr;
}
int FloorGeometry::at(Cell c) const noexcept {
    if (index_.empty() || c.x < 0 || c.y < 0 || c.x >= map_.width() || c.y >= map_.height())
        return -1;
    auto i = index_[std::size_t(c.y * map_.width() + c.x)];
    return i == UINT8_MAX ? -1 : int(i);
}
bool GeometryView::solid(Cell c) const noexcept {
    if (map_->solid(c))
        return true;
    const int i = fixtures_ ? fixtures_->at(c) : -1;
    if (i >= 0 && doors_ && doors_->world == world() && doors_->active(unsigned(i)))
        return true;
    return i >= 0 &&
           fixtureBlocks(fixtures_->definition().entries[unsigned(i)], fixtures_->completed(unsigned(i)));
}
bool GeometryView::npcAllowed(Cell c) const noexcept {
    if (solid(c))
        return false;
    if (!fixtures_)
        return true;
    const auto index = std::size_t(c.y * width() + c.x);
    return index >= discoveryCapacity || !fixtures_->definition().npcExcluded[index];
}
std::uint64_t GeometryView::world() const noexcept {
    return fixtures_ ? fixtures_->world() : 0;
}
std::uint64_t GeometryView::revision() const noexcept {
    return (fixtures_ ? fixtures_->geometryRevision() : 0) +
           (doors_ ? doors_->revision * 2 + unsigned(doors_->active()) : 0);
}
bool FloorGeometry::complete(unsigned i) noexcept {
    if (i >= definition_.count || completed_[i] || revision_ == UINT64_MAX || geometryRevision_ == UINT64_MAX)
        return false;
    completed_[i] = true;
    ++revision_;
    if (definition_.entries[i].kind == FixtureKind::Door ||
        definition_.entries[i].kind == FixtureKind::GuardianGate)
        ++geometryRevision_;
    return true;
}
bool FloorGeometry::discovered(Cell c) const noexcept {
    return definition_.discovery && c.x >= 0 && c.y >= 0 && c.x < map_.width() && c.y < map_.height() &&
           discovered_[std::size_t(c.y * map_.width() + c.x)];
}
void FloorGeometry::discover(Cell cell, Heading heading) noexcept {
    if (!definition_.discovery)
        return;
    bool changed = false;
    for (int y = std::max(0, cell.y - 6); y <= std::min(map_.height() - 1, cell.y + 6); ++y)
        for (int x = std::max(0, cell.x - 6); x <= std::min(map_.width() - 1, cell.x + 6); ++x) {
            const Cell c{x, y};
            if (!(c == cell) && !discoveryVisible(view(), cell, heading, c))
                continue;
            const auto n = std::size_t(y * map_.width() + x);
            changed |= !discovered_[n];
            discovered_.set(n);
            const auto i = at(c);
            if (i >= 0) {
                changed |= !known_[i] || remembered_[i] != completed_[i];
                known_[i] = true;
                remembered_[i] = completed_[i];
            }
        }
    if (changed && discoveryRevision_ != UINT64_MAX)
        ++discoveryRevision_;
}
} // namespace blobber
