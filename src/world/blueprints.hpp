#pragma once
#include "game/movement.hpp"
#include "world/fixtures.hpp"
#include <array>
#include <stdexcept>
#include <string_view>
namespace blobber::blueprints {
constexpr unsigned maxRooms = 8, maxKits = 16, maxSide = 9, maxCells = 81;
constexpr unsigned maxPlacements = 16, maxSockets = 4, maxRegions = 2, maxLanes = 8;
using Name = std::array<char, 33>;
using Mask = std::array<bool, maxCells>;
struct KitId {
    unsigned index = maxKits;
};
struct RoomId {
    unsigned index = maxRooms;
};
enum class Purpose { Kitchen, MeadHall, Barracks, Armory };
enum class Error { Content, Capacity, Bounds, Overlap, Clearance, Disconnected, Socket, Reference };
class Failure : public std::runtime_error {
  public:
    Failure(Error code, const std::string& context) : std::runtime_error(context), code(code) {}
    Error code;
};
struct Rect {
    Cell origin{}, size{};
};
struct Kit {
    Name id{};
    Cell size{};
    float height = 0;
    bool blocking = true;
    std::optional<AmbientFire> fire; // Mesh-local position; authored in native cell units.
};
struct Placement {
    KitId kit;
    Cell anchor{};
    Heading facing = Heading::North;
};
struct Socket {
    Name id{};
    Cell cell{};
    Heading outward = Heading::North;
};
struct Reward {
    Cell cell{}, access{};
};
struct RoomBlueprint {
    Name id{};
    Purpose purpose = Purpose::Kitchen;
    Cell size{};
    unsigned ceiling = 6, rotations = 15;
    Mask solid{}; // Fixed stride maxSide, independent of room width.
    std::array<Placement, maxPlacements> placements{};
    std::array<Socket, maxSockets> sockets{};
    std::array<Rect, maxLanes> lanes{};
    std::array<Rect, maxRegions> encounters{};
    std::array<Reward, maxRegions> rewards{};
    unsigned placementCount = 0, socketCount = 0, laneCount = 0, encounterCount = 0, rewardCount = 0;
};
struct BlueprintCatalog {
    std::array<Kit, maxKits> kits{};
    std::array<RoomBlueprint, maxRooms> rooms{};
    unsigned kitCount = 0, roomCount = 0;
    KitId kit(std::string_view) const;
    RoomId room(std::string_view) const;
};
constexpr unsigned index(Cell c) noexcept {
    return unsigned(c.y * int(maxSide) + c.x);
}
bool inside(Cell c, Cell size) noexcept;
Cell rotate(Cell c, Cell size, unsigned turns) noexcept;
Cell rotatedSize(Cell size, unsigned turns) noexcept;
Heading rotate(Heading, unsigned turns) noexcept;
Cell footprint(const Kit&, const Placement&, unsigned x, unsigned y) noexcept;
RoomBlueprint transform(const RoomBlueprint&, unsigned turns);
void validate(const BlueprintCatalog&);
void validateRoom(const BlueprintCatalog&, const RoomBlueprint&);
Mask occupied(const BlueprintCatalog&, const RoomBlueprint&);
struct ReviewRoom {
    RoomBlueprint blueprint;
    Cell origin;
};
struct ReviewScene {
    Map map;
    FloorFixtures fixtures;
    Player spawn;
    std::array<ReviewRoom, 2> rooms{};
    unsigned roomCount = 0;
    std::array<Cell, 2> combatCells{};
    Heading combatFacing = Heading::South;
};
// sockets=0 selects south (or first socket); pair uses shared and south doors.
// Returned Map/metadata own all data; no catalog borrowing reaches runtime.
ReviewScene assemble(const BlueprintCatalog&, std::string_view selection, unsigned turns = 0,
                     unsigned sockets = 0, std::string_view assetPack = "blueprint-blockouts");
} // namespace blobber::blueprints
