#pragma once
#include "render/picking.hpp"
#include "render/world_scene.hpp"
namespace blobber::world_render {
struct DoorBox {
    Vec origin{}, x{}, y{}, z{};
    std::uint16_t id = PickingBuffer::empty, texture = 0, edgeTexture = 0;
    bool worldMapped = false;
    std::uint16_t emblemTexture = UINT16_MAX;
};
constexpr unsigned doorBoxCapacity = fixtureCapacity * 4;
unsigned doorBoxes(GeometryView, const DoorSnapshot*, std::array<DoorBox, doorBoxCapacity>&,
                   unsigned ordinary, unsigned locked, unsigned frame, unsigned edge = 0,
                   unsigned guardianEmblem = UINT16_MAX, unsigned guardianDoor = UINT16_MAX);
std::array<Vec, 8> corners(const DoorBox&) noexcept;
std::array<float, 2> doorUvSpan(const DoorBox&, unsigned textureWidth, unsigned textureHeight) noexcept;
// Horizontal ray distance to an oriented box over the sprite's visible height.
float doorClearance(const DoorBox&, Vec origin, Vec direction, float limit, float low, float high) noexcept;
std::array<Vec, 4> doorEmblemQuad(const DoorBox&, bool back) noexcept;
} // namespace blobber::world_render
