#pragma once
#include "game/movement.hpp"
#include "world/map.hpp"
#include <array>
#include <cstdint>
#include <optional>
namespace blobber::generation {
constexpr unsigned generatorVersion = 10, templateVersion = 7;
constexpr unsigned maxCells = 1600, maxRooms = 16, maxConnections = 24, maxAttempts = 16;
using CellMask = std::array<bool, maxCells>;
constexpr int floorDimension = 36;
constexpr std::size_t fixtureScratchBytes = maxCells * 4 * (sizeof(std::uint8_t) + sizeof(std::uint16_t));
enum class Role : std::uint8_t { Junction, Entrance, Approach, Objective, Optional };
enum class Domain : std::uint32_t { Topology = 1, Interior, Encounter, Reward, Runtime, Dressing };
struct GenerationRequest {
    std::uint32_t seed = 0;
    unsigned depth = 1, generator = generatorVersion, templates = templateVersion;
};
enum class BiomeId : std::uint8_t { Underkeep };
enum class AlgorithmId : std::uint8_t { StoneRooms };
enum class LayoutProfile : std::uint8_t { CompactWings, ChamberNetwork };
struct BiomeDefinition {
    BiomeId id;
    AlgorithmId algorithm;
    const char* assetPack;
    std::array<LayoutProfile, 2> profiles;
};
struct FloorSpec {
    unsigned depth;
    int width, height;
    BiomeId biome;
    unsigned populationBand;
    bool keyedSector, finalFloor;
};
const BiomeDefinition& underkeepBiome() noexcept;
std::optional<FloorSpec> floorSpec(unsigned depth) noexcept;
const char* profileName(LayoutProfile) noexcept;
constexpr std::uint8_t noRegion = 255;
struct Room {
    Cell origin{}, size{}, anchor{};
    Role role = Role::Junction;
    std::uint8_t id = 0, shape = 0, rotation = 0, clearance = 1;
    Heading facing = Heading::North;
};
struct Connection {
    std::uint8_t a = 0, b = 0;
    Cell socketA{}, socketB{};
    Heading facingA = Heading::North, facingB = Heading::South;
    std::uint16_t first = 0, count = 0;
};
enum class ActorRecipe : std::uint8_t { Goblin, Archer, Skeleton, Zombie, Rat, Shaman, Bugbear };
enum class EncounterRecipe : std::uint8_t { GoblinPair, RatPack, UndeadPair, GoblinSupport, BugbearSupport };
enum class EncounterRole : std::uint8_t { Direct, Optional, Guardian };
struct Encounter {
    EncounterRecipe recipe = EncounterRecipe::GoblinPair;
    EncounterRole role = EncounterRole::Direct;
    std::uint8_t room = 0, first = 0, count = 0;
};
struct Recipe {
    const char* name;
    std::array<ActorRecipe, 4> members;
    unsigned count;
};
const Recipe& encounterRecipe(EncounterRecipe) noexcept;
const char* speciesName(ActorRecipe) noexcept;
struct PopulationPlacement {
    Cell cell{}, subcell{};
    Heading facing = Heading::North;
    ActorRecipe recipe = ActorRecipe::Goblin;
    std::uint8_t id = 0;
    bool guardian = false;
};
struct CacheSite {
    Cell cell{};
    Heading facing = Heading::North;
};
struct FloorLayout {
    std::array<CacheSite, 3> caches{};
    unsigned cacheCount = 0;
    std::uint64_t dressingFingerprint = 0;
    int width = floorDimension, height = floorDimension;
    std::array<std::uint8_t, maxCells> floor{};
    std::array<std::uint8_t, maxCells> membership{};
    std::array<std::uint16_t, maxCells> paths{};
    unsigned pathCount = 0;
    BiomeId biome = BiomeId::Underkeep;
    AlgorithmId algorithm = AlgorithmId::StoneRooms;
    LayoutProfile profile = LayoutProfile::CompactWings;
    FloorLayout() noexcept { membership.fill(noRegion); }
    std::array<Room, maxRooms> rooms{};
    std::array<Connection, maxConnections> connections{};
    unsigned roomCount = 0, connectionCount = 0, family = 0, rotation = 0;
    unsigned loopConnection = 0;
    Player spawn;
    unsigned depth = 1;
    FloorFixtures fixtures;
    std::array<PopulationPlacement, 32> population{};
    unsigned populationCount = 0;
    std::array<Encounter, 5> encounters{};
    unsigned encounterCount = 0;
    std::array<std::uint8_t, maxRooms * 2> route{};
    unsigned routeCount = 0;
    Cell optionalSupply{}, guardianReward{}, guardianApproach{}, supply{};
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
GenerationResult generate(GenerationRequest, bool forceFallback = false);
Failure validate(const FloorLayout&) noexcept;
Failure validateFixtures(const FloorLayout&, const CellMask* blocked = nullptr) noexcept;
void placeFixtures(FloorLayout&) noexcept;
void placePopulation(FloorLayout&, GenerationRequest, unsigned attempt, bool fallback) noexcept;
Failure validatePopulation(const FloorLayout&) noexcept;
bool inside(const FloorLayout&, Cell) noexcept;
bool floorCell(const FloorLayout&, Cell) noexcept;
unsigned regionAt(const FloorLayout&, Cell) noexcept;
Cell pathCell(const FloorLayout&, const Connection&, unsigned step) noexcept;
unsigned sectorJunction(const FloorLayout&) noexcept;
const Connection* connectionBetween(const FloorLayout&, unsigned, unsigned) noexcept;
Heading incomingFacing(const Connection&, unsigned room) noexcept;
Cell threshold(const Connection&) noexcept;
Failure validateStone(const FloorLayout&) noexcept;
std::uint64_t seedFor(GenerationRequest, Domain, unsigned stableId = 0, unsigned attempt = 0) noexcept;
std::uint64_t fingerprint(const FloorLayout&) noexcept;
const char* errorName(Error) noexcept;
const char* roleName(Role) noexcept;
Cell rotate(Cell, int extent, unsigned quarterTurns) noexcept;
struct LayoutMetrics {
    unsigned walkableCells = 0, minRoomArea = 0, maxRoomArea = 0, distinctRoomAreas = 0;
    unsigned distinctConnectionLengths = 0, progressionSteps = 0;
};
LayoutMetrics layoutMetrics(const FloorLayout&) noexcept;
std::size_t scratchBytes() noexcept;
// Diagnostic helpers allocate only during inspection/export.
std::string ascii(const FloorLayout&);
} // namespace blobber::generation
