#pragma once
#include "render/picking.hpp"
#include "render/world_scene.hpp"
namespace blobber::world_render {
struct DoorBox {
    Vec origin{}, x{}, y{}, z{};
    std::uint16_t id = PickingBuffer::empty, texture = 0;
};
constexpr unsigned doorBoxCapacity = fixtureCapacity * 4;
unsigned doorBoxes(GeometryView, const DoorSnapshot*, std::array<DoorBox, doorBoxCapacity>&,
                   unsigned ordinary, unsigned locked, unsigned frame);
std::array<Vec, 8> corners(const DoorBox&) noexcept;
} // namespace blobber::world_render
