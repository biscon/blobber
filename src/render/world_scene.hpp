#pragma once
#include "assets/world_assets.hpp"
#include "game/movement.hpp"
#include "render/layout.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace blobber::world_render {
constexpr float pi = 3.14159265358979323846f, nearPlane = .025f, farPlane = 16;
constexpr unsigned chunkSize = 8, materialCapacity = 32;
// Far-frustum diagonal reach is ~21.54 cells; architecture adds up to four.
// Round the candidate search up to a chunk multiple, including interpolated turns.
constexpr unsigned buildingCullRadius = 32, buildingCullChunks = 81;
struct Vec {
    float x = 0, y = 0, z = 0;
};
struct Camera {
    Vec position, forward, right;
    float yaw = 0;
};
struct ScreenPoint {
    float x, y, depth;
};
float focal() noexcept;
Camera camera(const MovementState&) noexcept;
Camera camera(const MovementState&, GeometryView) noexcept;
float daylight(const MovementState&, GeometryView) noexcept;
ScreenPoint project(const Camera&, Vec) noexcept;
bool sameCamera(const Camera&, const Camera&) noexcept;
struct Vertex {
    Vec position, normal;
    float u, v;
    float shade = 1;
};
static_assert(sizeof(Vertex) == 36);
struct Surfaces {
    std::uint8_t wall = 0, floor = 1, ceiling = 2, stairWall = 0, stairFloor = 1;
};
struct Range {
    std::uint32_t first = 0, count = 0;
    std::uint8_t material = 0;
    std::uint16_t id = UINT16_MAX;
    bool painted = false;
    std::uint8_t variant = noContainer;
    bool cutout = false;
    bool water = false;
};
struct Chunk {
    std::uint32_t first = 0, count = 0;
    float low = 0, high = 1;
    int left = 0, top = 0, width = 8, depth = 8;
};
struct Limits {
    std::size_t quads = 262144, ranges = 65536, cpuBytes = 64u * 1024 * 1024;
};
struct Decal {
    std::array<Vertex, 4> vertices;
    unsigned motif = 0;
    std::uint16_t texture = UINT16_MAX;
};
struct Scene {
    const Map* map = nullptr;
    bool outdoor = false;
    unsigned cullRadius = 24;
    std::vector<Decal> decals;
    std::uint64_t identity = 0;
    int width = 0, height = 0, columns = 0, rows = 0;
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    std::vector<Range> ranges;
    std::vector<Chunk> chunks;
    std::size_t bytes() const noexcept;
    std::size_t meshBytes() const noexcept;
};
// Optional overrides are row-major, one per cell. No simulation/schema mutation.
Scene prepare(GeometryView, Surfaces = {}, const std::vector<Surfaces>& overrides = {}, Limits = {},
              const world_assets::CpuAssets* assets = nullptr);
bool visible(const Camera&, int x, int z, int width, int height, float low = 0, float high = 1) noexcept;
struct DrawList {
    std::array<std::uint32_t,
               buildingCullChunks * materialCapacity * 4 + fixtureCapacity * 3 + 128 + containerCapacity * 2>
        ranges{};
    unsigned count = 0, tested = 0;
};
void cull(const Scene&, const Camera&, DrawList&) noexcept;
} // namespace blobber::world_render
