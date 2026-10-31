#pragma once
#include "game/movement.hpp"
#include "render/layout.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace blobber::world_render {
constexpr float pi = 3.14159265358979323846f, nearPlane = .025f, farPlane = 16;
constexpr unsigned chunkSize = 8, materialCapacity = 32;
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
};
static_assert(sizeof(Vertex) == 32);
struct Surfaces {
    std::uint8_t wall = 0, floor = 1, ceiling = 2;
};
struct Range {
    std::uint32_t first = 0, count = 0;
    std::uint8_t material = 0;
};
struct Chunk {
    std::uint32_t first = 0, count = 0;
    float low = 0, high = 1;
};
struct Limits {
    std::size_t quads = 262144, ranges = 65536, cpuBytes = 64u * 1024 * 1024;
};
struct Decal {
    std::array<Vertex, 4> vertices;
    unsigned motif = 0;
};
struct Scene {
    const Map* map = nullptr;
    bool outdoor = false;
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
Scene prepare(GeometryView, Surfaces = {}, const std::vector<Surfaces>& overrides = {}, Limits = {});
bool visible(const Camera&, int x, int z, int width, int height, float low = 0, float high = 1) noexcept;
struct DrawList {
    std::array<std::uint32_t, 49 * materialCapacity> ranges{};
    unsigned count = 0, tested = 0;
};
void cull(const Scene&, const Camera&, DrawList&) noexcept;
} // namespace blobber::world_render
