#include "probe/scene.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace blobber::probe {
float focal() noexcept {
    return width * .5f / std::tan(84 * pi / 360);
}
Camera camera(const MovementState& s) noexcept {
    const auto& t = s.transition;
    const Player a = t.active() ? t.from : s.player, b = t.active() ? t.to : s.player;
    const float p = t.active() ? t.progress() : 1, e = p * p * (3 - 2 * p);
    float ya = float(a.heading) * pi / 2, yb = float(b.heading) * pi / 2;
    float delta = yb - ya;
    if (delta > pi)
        delta -= 2 * pi;
    if (delta < -pi)
        delta += 2 * pi;
    const float yaw = ya + delta * e;
    const Vec f{std::sin(yaw), 0, -std::cos(yaw)}, r{std::cos(yaw), 0, std::sin(yaw)};
    return {{a.cell.x + .5f + (b.cell.x - a.cell.x) * e - .4f * f.x, .5f,
             a.cell.y + .5f + (b.cell.y - a.cell.y) * e - .4f * f.z},
            f,
            r,
            yaw};
}
ScreenPoint project(const Camera& c, Vec p) noexcept {
    const float dx = p.x - c.position.x, dz = p.z - c.position.z;
    const float d = dx * c.forward.x + dz * c.forward.z;
    return {width * .5f + focal() * (dx * c.right.x + dz * c.right.z) / d,
            height * .34f - focal() * (p.y - c.position.y) / d, d};
}
Vec billboardCenter(const Camera& c, Vec feet, float w, float h, float anchorX, float anchorY) noexcept {
    return {feet.x + c.right.x * (.5f - anchorX) * w, feet.y + (anchorY - .5f) * h,
            feet.z + c.right.z * (.5f - anchorX) * w};
}
bool clearance(const Map& m, const Camera& c) noexcept {
    const float half = nearPlane * std::tan(84 * pi / 360);
    for (float side : {-1.f, 0.f, 1.f}) {
        const float x = c.position.x + c.forward.x * nearPlane + c.right.x * half * side;
        const float z = c.position.z + c.forward.z * nearPlane + c.right.z * half * side;
        if (m.solid({int(std::floor(x)), int(std::floor(z))}))
            return false;
    }
    return !m.solid({int(std::floor(c.position.x)), int(std::floor(c.position.z))});
}
std::vector<Quad> buildFaces(const Map& m, bool specimen) {
    if (m.width() > 32 || m.height() > 32)
        throw std::runtime_error("Probe maps exceed 32 x 32");
    std::vector<Quad> out;
    out.reserve(std::size_t(m.width()) * m.height() * 6);
    auto add = [&](std::array<Vec, 4> p, Vec n, unsigned material) {
        if (out.size() >= maxQuads)
            throw std::runtime_error("Probe exceeds 8192 quads");
        Quad q{};
        q.material = material;
        for (int i = 0; i < 4; ++i) {
            float u = material == 0 ? (n.x != 0 ? p[i].z : p[i].x) : p[i].x;
            float v = material == 0 ? 1 - p[i].y : p[i].z;
            q.vertices[i] = {p[i], n, u, v};
        }
        out.push_back(q);
    };
    for (int z = 0; z < m.height(); ++z)
        for (int x = 0; x < m.width(); ++x) {
            if (m.solid({x, z}))
                continue;
            const float a = float(x), b = float(z);
            add({Vec{a, 0, b}, Vec{a, 0, b + 1}, Vec{a + 1, 0, b + 1}, Vec{a + 1, 0, b}}, {0, 1, 0}, 1);
            if (!specimen || z >= 12)
                add({Vec{a, 1, b}, Vec{a + 1, 1, b}, Vec{a + 1, 1, b + 1}, Vec{a, 1, b + 1}}, {0, -1, 0}, 2);
            if (m.solid({x, z - 1}))
                add({Vec{a, 0, b}, Vec{a + 1, 0, b}, Vec{a + 1, 1, b}, Vec{a, 1, b}}, {0, 0, 1}, 0);
            if (m.solid({x, z + 1}))
                add({Vec{a + 1, 0, b + 1}, Vec{a, 0, b + 1}, Vec{a, 1, b + 1}, Vec{a + 1, 1, b + 1}},
                    {0, 0, -1}, 0);
            if (m.solid({x - 1, z}))
                add({Vec{a, 0, b + 1}, Vec{a, 0, b}, Vec{a, 1, b}, Vec{a, 1, b + 1}}, {1, 0, 0}, 0);
            if (m.solid({x + 1, z}))
                add({Vec{a + 1, 0, b}, Vec{a + 1, 0, b + 1}, Vec{a + 1, 1, b + 1}, Vec{a + 1, 1, b}},
                    {-1, 0, 0}, 0);
        }
    return out;
}
Scene makeScene(bool specimen) {
    std::vector<std::string> rows(20, std::string(15, '#'));
    for (int z = 2; z <= 12; ++z)
        for (int x = 2; x <= 12; ++x)
            rows[z][x] = '.';
    for (int z = 13; z <= 18; ++z)
        for (int x = 4; x <= 6; ++x)
            rows[z][x] = '.';
    rows[7][7] = rows[8][7] = '#';
    // A side opening and a near-wall corner within the long approach.
    for (int x = 7; x <= 10; ++x)
        rows[15][x] = '.';
    Scene s{Map(rows), specimen, {{5, 16}, Heading::North}, {5, 9}, {}};
    s.faces = buildFaces(s.map, specimen);
    return s;
}
float faceLight(Vec n, Vec f) noexcept {
    if (n.y > .5f)
        return .85f;
    if (n.y < -.5f)
        return .58f;
    return .78f + .22f * std::abs(n.x * f.x + n.z * f.z);
}
std::array<float, 3> shade(std::array<float, 3> c, float depth, float face) noexcept {
    const float light = face / (1 + .115f * depth + .015f * depth * depth);
    const float fog = std::clamp((depth - 9.4f) / 3.5f, 0.f, 1.f);
    constexpr float dark[]{15 / 255.f, 18 / 255.f, 20 / 255.f};
    for (int i = 0; i < 3; ++i)
        c[i] = c[i] * light * (1 - fog) + dark[i] * fog;
    return c;
}
namespace {
constexpr std::array<const char*, shotCount> names{
    "forward",     "backward",      "strafe-left", "strafe-right",  "turn-left-corner", "turn-right-corner",
    "close-actor", "distant-actor", "paused",      "animation-off", "door-opening",     "courtyard"};
}
const char* shotName(unsigned ms) noexcept {
    return names[(ms / shotMs) % shotCount];
}
MovementState scripted(const Scene& s, unsigned ms) {
    const unsigned shot = (ms / shotMs) % shotCount, local = ms % shotMs;
    Player start = s.spawn;
    Action action = Action::Forward;
    bool animate = true, move = true;
    switch (shot) {
    case 0:
        start = {{5, 15}, Heading::North};
        break;
    case 1:
        start = {{5, 14}, Heading::North};
        action = Action::Back;
        break;
    case 2:
        start = {{6, 10}, Heading::North};
        action = Action::Left;
        break;
    case 3:
        start = {{5, 10}, Heading::North};
        action = Action::Right;
        break;
    case 4:
        start = {{2, 2}, Heading::North};
        action = Action::TurnLeft;
        break;
    case 5:
        start = {{2, 2}, Heading::West};
        action = Action::TurnRight;
        break;
    case 6:
        start = {{5, 10}, Heading::North};
        move = false;
        break;
    case 7:
        start = {{5, 16}, Heading::North};
        move = false;
        break;
    case 8:
        start = {{6, 10}, Heading::North};
        action = Action::Left;
        break;
    case 9:
        start = {{5, 15}, Heading::North};
        animate = false;
        break;
    case 10:
        start = {{5, 12}, Heading::North};
        move = false;
        break;
    case 11:
        start = {{5, 12}, Heading::North};
        break;
    }
    Controller controller(start);
    controller.setAnimated(animate);
    if (move && local >= 200) {
        controller.submit(s.map, action, true);
        unsigned elapsed = local - 200;
        if (shot == 8)
            elapsed = std::min(elapsed, 80u);
        for (unsigned tick = 0; tick < elapsed / 10; ++tick)
            controller.update(s.map, .01f);
    }
    auto state = controller.state();
    // Stable identity for cached legacy and GPU frames across shot resets.
    state.revision = 1 + shot * 2 + (local >= 200 && move ? 1 : 0);
    return state;
}
float doorAngle(unsigned ms) noexcept {
    float t = std::clamp((float(ms % shotMs) - 200) / 400, 0.f, 1.f);
    return 90 * t * t * (3 - 2 * t);
}
} // namespace blobber::probe
