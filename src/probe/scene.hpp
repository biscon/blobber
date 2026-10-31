#pragma once
#include "render/world_scene.hpp"
#include <array>
#include <vector>
namespace blobber::probe {
constexpr int width = 500, height = 282, maxQuads = 8192;
constexpr float pi = 3.14159265358979323846f, nearPlane = .025f, farPlane = 16;
using Vec = world_render::Vec;
struct Vertex {
    Vec p, normal;
    float u = 0, v = 0;
};
struct Quad {
    std::array<Vertex, 4> vertices;
    unsigned material = 0;
};
using Camera = world_render::Camera;
using ScreenPoint = world_render::ScreenPoint;
struct Scene {
    Map map;
    bool specimen = false;
    Player spawn{{5, 16}, Heading::North};
    Cell actor{5, 9};
    std::vector<Quad> faces;
};
using world_render::focal;
using world_render::camera;
using world_render::project;
Vec billboardCenter(const Camera&, Vec feet, float w, float h, float anchorX, float anchorY) noexcept;
bool clearance(const Map&, const Camera&) noexcept;
std::vector<Quad> buildFaces(const Map&, bool specimen);
Scene makeScene(bool specimen);
float faceLight(Vec normal, Vec forward) noexcept;
std::array<float, 3> shade(std::array<float, 3> color, float depth, float face) noexcept;
// Snapshot script uses fixed 10 ms ticks; one independently reset shot per 800 ms.
constexpr unsigned shotCount = 12, shotMs = 800, sequenceMs = shotCount * shotMs;
MovementState scripted(const Scene&, unsigned ms);
const char* shotName(unsigned ms) noexcept;
float doorAngle(unsigned ms) noexcept;
} // namespace blobber::probe
