#pragma once
#include "game/inventory.hpp"
#include "world/grid.hpp"
namespace blobber {
enum class RestKind : std::uint8_t { Short, Long };
enum class RestError : std::uint8_t {
    None,
    Stale,
    Unsafe,
    Moving,
    Dying,
    NoLeader,
    InvalidDice,
    NoBenefit,
    NoSupplies,
    ClockFull
};
const char* restMessage(RestError) noexcept;
struct RestRequest {
    RestKind kind = RestKind::Short;
    std::array<std::uint8_t, 4> dice{};
    std::uint64_t world = 0, token = 0;
};
struct RestPreview {
    RestError error = RestError::None;
    ItemHandle supply{};
    rules::CharacterId supplier{};
    unsigned kits = 0;
    bool sanctuary = false;
    std::uint64_t elapsedMs = 0;
    explicit operator bool() const noexcept { return error == RestError::None; }
};
struct RestResult {
    RestError error = RestError::None;
    std::array<int, 4> healed{}, mana{}, diceSpent{};
    bool usedSupply = false;
    explicit operator bool() const noexcept { return error == RestError::None; }
};
struct GroundPlacement {
    Cell cell;
    rules::ItemId item;
    std::uint16_t quantity = 1;
};
struct RecoveryLevel {
    std::array<Cell, 16> sanctuaries{};
    std::size_t sanctuaryCount = 0;
    std::array<GroundPlacement, 64> ground{};
    std::size_t groundCount = 0;
};
} // namespace blobber
