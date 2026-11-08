#pragma once
#include "game/actors.hpp"
#include "game/movement.hpp"
#include <vector>

namespace blobber {
class EffectController;
// One combat search workspace, reused across active enemies; never serialized.
class TacticalNavigation {
  public:
    enum class Status { Pending, Step, AtGoal, Blocked };
    struct Result {
        Status status = Status::Blocked;
        Cell cell{}, subcell{};
        unsigned costFeet = 0;
    };
    TacticalNavigation(GeometryView, const ActorWorld&);
    Result next(ActorId, Player, Cell target, bool retreat, const EffectController*,
                std::size_t& work) noexcept;
    void committed() noexcept;
    void cancel() noexcept { valid_ = false; }

  private:
    static constexpr std::uint32_t missing = UINT32_MAX;
    GeometryView map_;
    const ActorWorld& actors_;
    std::vector<std::uint32_t> distance_, parent_, heap_, heapPosition_, route_, stamp_;
    std::uint32_t epoch_ = 0;
    struct Goal {
        std::uint32_t node = 0;
        unsigned rank = 0;
    };
    std::array<Goal, 64> goals_{};
    std::size_t goalCount_ = 0, heapCount_ = 0, routeCount_ = 0, reset_ = 0;
    std::uint32_t root_ = 0, best_ = missing, building_ = missing;
    unsigned bestRank_ = 3, wantedRank_ = 3;
    enum class Phase { Reset, Search, Build, Ready, Blocked } phase_ = Phase::Reset;
    ActorId actor_{};
    Player party_{};
    Cell target_{};
    bool valid_ = false, retreat_ = false;
    std::uint64_t geometry_ = 0, occupancy_ = 0, ground_ = 0;
    std::uint32_t index(Cell cell, Cell subcell) const noexcept;
    Cell cell(std::uint32_t) const noexcept;
    Cell subcell(std::uint32_t) const noexcept;
    bool legal(std::uint32_t from, std::uint32_t to) const noexcept;
    unsigned cost(std::uint32_t to, const EffectController*) const noexcept;
    void goals() noexcept;
    void push(std::uint32_t) noexcept;
    std::uint32_t pop() noexcept;
    bool less(std::uint32_t a, std::uint32_t b) const noexcept;
};
} // namespace blobber
