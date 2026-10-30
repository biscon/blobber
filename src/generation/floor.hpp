#pragma once
#include "game/movement.hpp"
#include "world/map.hpp"
#include <array>
#include <cstdint>
#include <optional>
namespace blobber::generation {
constexpr unsigned generatorVersion = 3, templateVersion = 3;
constexpr unsigned maxCells = 1600, maxRooms = 16, maxConnections = 24, maxAttempts = 16;
constexpr int floorDimension = 36;
constexpr std::size_t fixtureScratchBytes = maxCells * 4 * (sizeof(std::uint8_t) + sizeof(std::uint16_t));
enum class Role : std::uint8_t { Junction, Entrance, Approach, Objective, Optional };
enum class Domain : std::uint32_t { Topology = 1, Interior, Encounter, Reward, Runtime };
struct GenerationRequest {
    std::uint32_t seed = 0;
    unsigned depth = 1, generator = generatorVersion, templates = templateVersion;
};
struct RoomTemplate {
    std::array<std::uint8_t, 49> floor{}; // Closed rim; cardinal sockets are opened by stamping.
    Cell anchor{3, 3};
};
using TemplateCatalog = std::array<RoomTemplate, 5>;
const TemplateCatalog& templates() noexcept;
struct Room {
    Cell site;
    Cell anchor;
    Role role = Role::Junction;
    std::uint8_t id = 0, shape = 0, rotation = 0, ports = 0;
    Heading facing = Heading::North;
};
struct Connection {
    std::uint8_t a = 0, b = 0;
};
enum class ActorRecipe : std::uint8_t { Goblin, Archer, Guardian };
struct PopulationPlacement {
    Cell cell{}, subcell{};
    Heading facing = Heading::North;
    ActorRecipe recipe = ActorRecipe::Goblin;
    std::uint8_t id = 0;
    bool guardian = false;
};
struct FloorLayout {
    int width = floorDimension, height = floorDimension;
    std::array<std::uint8_t, maxCells> floor{};
    std::array<Room, maxRooms> rooms{};
    std::array<Connection, maxConnections> connections{};
    unsigned roomCount = 0, connectionCount = 0, family = 0, rotation = 0;
    unsigned loopConnection = 0;
    Player spawn;
    unsigned depth = 1;
    FloorFixtures fixtures;
    std::array<PopulationPlacement, 32> population{};
    unsigned populationCount = 0;
    Cell optionalSupply{}, guardianReward{}, guardianApproach{};
    std::uint8_t approachDoor = 0, guardianGate = 0;
};
enum class Error : std::uint8_t {
    None,
    Request,
    Capacity,
    Template,
    Bounds,
    Room,
    Port,
    Connection,
    Anchor,
    Disconnected,
    Loop,
    Branch,
    Fixture,
    Progression,
    Population,
    Landing,
    ForcedFallback
};
struct Failure {
    Error code = Error::None;
    int room = -1, connection = -1;
    explicit operator bool() const noexcept { return code != Error::None; }
};
struct GenerationDiagnostics {
    unsigned attempts = 0;
    std::array<Failure, maxAttempts> rejected{};
    bool fallback = false;
    unsigned fallbackId = 0;
    Failure failure;
};
struct GeneratedFloor {
    GenerationRequest request;
    FloorLayout layout;
    Map map;
    GenerationDiagnostics diagnostics;
    std::uint64_t fingerprint = 0;
    std::uint32_t runtimeSeed = 0;
};
struct GenerationResult {
    std::optional<GeneratedFloor> floor;
    GenerationDiagnostics diagnostics;
    explicit operator bool() const noexcept { return floor.has_value(); }
};
// All generation/Map construction is a preparation phase. No runtime borrowing.
GenerationResult generate(GenerationRequest, const TemplateCatalog& = templates(),
                          bool forceFallback = false);
Failure validate(const FloorLayout&) noexcept;
Failure validateFixtures(const FloorLayout&) noexcept;
void placeFixtures(FloorLayout&) noexcept;
void placePopulation(FloorLayout&, GenerationRequest, unsigned attempt, bool fallback) noexcept;
Failure validateTemplates(const TemplateCatalog&) noexcept;
std::uint64_t seedFor(GenerationRequest, Domain, unsigned stableId = 0, unsigned attempt = 0) noexcept;
std::uint64_t fingerprint(const FloorLayout&) noexcept;
const char* errorName(Error) noexcept;
const char* roleName(Role) noexcept;
Cell rotate(Cell, int extent, unsigned quarterTurns) noexcept;
std::size_t scratchBytes() noexcept;
// Diagnostic helpers allocate only during inspection/export.
std::string ascii(const FloorLayout&);
} // namespace blobber::generation
