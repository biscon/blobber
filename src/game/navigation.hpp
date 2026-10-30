#pragma once
#include "game/actors.hpp"

namespace blobber {
// One incremental BFS workspace shared fairly by fixed, stable actor slots.
class Navigation {
  public:
    static constexpr std::size_t nodeBudget = 4096, requestBudget = 8;
    struct Result {
        bool ready = false, found = false;
        Cell from{}, target{}, step{};
        bool adjacent = false;
        std::optional<Cell> preferred{};
    };
    Navigation(GeometryView map, const ActorWorld& actors);
    Navigation(Map&&, const ActorWorld&) = delete;
    Navigation(const Map&&, const ActorWorld&) = delete;
    Navigation(GeometryView, ActorWorld&&) = delete;
    Navigation(GeometryView, const ActorWorld&&) = delete;
    Navigation(const Navigation&) = delete;
    Navigation& operator=(const Navigation&) = delete;
    void request(std::size_t slot, Cell from, Cell target, bool adjacent,
                 std::optional<Cell> preferred = {}) noexcept;
    void cancel(std::size_t slot) noexcept;
    Result take(std::size_t slot) noexcept;
    void update(Cell party) noexcept;
    std::size_t lastWork() const noexcept { return work_; }
    std::size_t lastCompleted() const noexcept { return completed_; }
    std::size_t storageBytes() const noexcept { return queue_.size() * 6; }

  private:
    struct Request {
        bool pending = false;
        Cell from{}, target{};
        bool adjacent = false;
        std::optional<Cell> preferred{};
    };
    GeometryView map_;
    const ActorWorld& actors_;
    std::vector<std::uint32_t> queue_;
    std::vector<std::uint8_t> seen_, first_;
    std::array<Request, NpcWorld::maxInstances> requests_{};
    std::array<Result, NpcWorld::maxInstances> results_{};
    Request active_{};
    std::uint64_t geometryRevision_ = 0;
    void invalidateGeometry() noexcept;
    std::size_t owner_ = NpcWorld::maxInstances, cursor_ = 0;
    std::size_t head_ = 0, tail_ = 0, cleanup_ = 0, work_ = 0, completed_ = 0;
    bool cleaning_ = false, canceled_ = false, preferAvailable_ = false, centerAvailable_ = false;
    std::optional<Cell> fallbackStep_{}, frontStep_{};
    std::uint32_t index(Cell cell) const noexcept;
    Cell cell(std::uint32_t index) const noexcept;
    bool start() noexcept;
    void finish(bool found, Cell step) noexcept;
};
} // namespace blobber
