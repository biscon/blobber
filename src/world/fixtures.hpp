#pragma once
#include "world/map.hpp"
#include <array>
#include <bitset>
#include <limits>

namespace blobber {
constexpr unsigned fixtureCapacity = 32, discoveryCapacity = 1600;
enum class FixtureKind : std::uint8_t { Door, Key, Upstairs, Downstairs, Endpoint, GuardianGate };
struct FixtureDefinition {
    Cell cell{};
    Heading facing = Heading::North;
    FixtureKind kind = FixtureKind::Door;
    std::uint8_t key = 0; // Zero is unlocked; 2 and 3 identify the campaign keys.
};
struct FloorFixtures {
    std::array<FixtureDefinition, fixtureCapacity> entries{};
    unsigned count = 0, depth = 0;
    bool discovery = false;
    std::array<Cell, 2> landings{};
    unsigned landingCount = 0;
    std::bitset<discoveryCapacity> npcExcluded;
};
struct FixtureId {
    std::uint64_t world = 0;
    std::uint16_t slot = UINT16_MAX;
};
inline bool operator==(FixtureId a, FixtureId b) noexcept {
    return a.world == b.world && a.slot == b.slot;
}
struct FixtureRequest {
    FixtureId id;
    std::uint64_t revision = 0;
};
struct FixtureResult {
    bool accepted = false, changed = false;
    const char* reason = "Fixture unavailable.";
    explicit operator bool() const noexcept { return accepted; }
};
inline bool fixtureReachable(const FixtureDefinition& f, Cell cell, Heading heading) noexcept {
    return f.cell == cell + forward(heading) ||
           (f.kind != FixtureKind::Door && f.kind != FixtureKind::GuardianGate && f.cell == cell);
}
inline bool fixtureBlocks(const FixtureDefinition& f, bool completed) noexcept {
    return (f.kind == FixtureKind::Door || f.kind == FixtureKind::GuardianGate) && !completed;
}
const char* fixtureName(const FixtureDefinition&) noexcept;
// Transient session-owned presentation, never persisted as world geometry.
struct DoorSnapshot {
    static constexpr std::uint64_t durationMs = 300;
    std::uint64_t world = 0, revision = 0, now = 0;
    std::array<std::uint64_t, fixtureCapacity> until{};
    bool active(unsigned i) const noexcept { return i < until.size() && until[i] > now; }
    std::uint64_t deadline() const noexcept {
        std::uint64_t result = now;
        for (auto value : until)
            if (value > result)
                result = value;
        return result;
    }
    bool active() const noexcept { return deadline() > now; }
    float progress(unsigned i) const noexcept {
        return active(i) ? 1.f - float(until[i] - now) / float(durationMs) : 1.f;
    }
};
class FloorGeometry;
// Cheap read-only borrow. Map-only views support authored/arena/test maps without fixtures.
class GeometryView {
  public:
    GeometryView(const Map& map) noexcept : map_(&map) {}
    GeometryView(Map&&) = delete;
    GeometryView(const Map&&) = delete;
    bool solid(Cell) const noexcept;
    // Render-only borrow: retain closed legacy proxies until their opening settles.
    GeometryView presentation(const DoorSnapshot& doors) const noexcept {
        auto result = *this;
        result.doors_ = &doors;
        return result;
    }
    const DoorSnapshot* doors() const noexcept { return doors_; }
    bool npcAllowed(Cell) const noexcept;
    int width() const noexcept { return map_->width(); }
    int height() const noexcept { return map_->height(); }
    const Map& base() const noexcept { return *map_; }
    const FloorGeometry* fixtures() const noexcept { return fixtures_; }
    std::uint64_t world() const noexcept;
    std::uint64_t revision() const noexcept;

  private:
    friend class FloorGeometry;
    GeometryView(const Map& map, const FloorGeometry* fixtures) noexcept : map_(&map), fixtures_(fixtures) {}
    const Map* map_;
    const FloorGeometry* fixtures_ = nullptr;
    const DoorSnapshot* doors_ = nullptr;
};
class GameSession;
class FloorGeometry {
  public:
    FloorGeometry(const Map&, const FloorFixtures& = {});
    FloorGeometry(Map&&, const FloorFixtures& = {}) = delete;
    FloorGeometry(const Map&&, const FloorFixtures& = {}) = delete;
    FloorGeometry(const FloorGeometry&) = delete;
    FloorGeometry& operator=(const FloorGeometry&) = delete;
    GeometryView view() const noexcept { return {map_, this}; }
    const FloorFixtures& definition() const noexcept { return definition_; }
    FixtureId id(unsigned slot) const noexcept {
        return slot < definition_.count ? FixtureId{world_, std::uint16_t(slot)} : FixtureId{};
    }
    const FixtureDefinition* find(FixtureId) const noexcept;
    int at(Cell) const noexcept;
    bool completed(unsigned slot) const noexcept { return slot < definition_.count && completed_[slot]; }
    std::uint64_t world() const noexcept { return world_; }
    std::uint64_t revision() const noexcept { return revision_; }
    std::uint64_t geometryRevision() const noexcept { return geometryRevision_; }
    bool discovered(Cell) const noexcept;
    bool known(unsigned slot) const noexcept {
        return slot < definition_.count && (!definition_.discovery || known_[slot]);
    }
    bool rememberedComplete(unsigned slot) const noexcept {
        return slot < definition_.count && (definition_.discovery ? remembered_[slot] : completed_[slot]);
    }
    std::uint64_t discoveryRevision() const noexcept { return discoveryRevision_; }
    std::size_t storageBytes() const noexcept { return sizeof(*this) + index_.capacity(); }

    // Preparation-only owned values; no borrowed pointers or navigation workspace.
    struct Saved {
        std::array<bool, fixtureCapacity> completed{}, known{}, remembered{};
        std::bitset<discoveryCapacity> discovered;
        std::uint64_t revision = 0, geometryRevision = 0, discoveryRevision = 0;
    };

  private:
    friend class GameSession;
    friend class GameSession;
    const Map& map_;
    FloorFixtures definition_;
    std::vector<std::uint8_t> index_;
    std::array<bool, fixtureCapacity> completed_{}, known_{}, remembered_{};
    std::bitset<discoveryCapacity> discovered_;
    std::uint64_t world_, revision_ = 0, geometryRevision_ = 0, discoveryRevision_ = 0;
    bool complete(unsigned) noexcept;
    void discover(Cell, Heading) noexcept;
};
} // namespace blobber
