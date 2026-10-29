#pragma once
#include "render/scene_renderer.hpp"
#include "render/transitions.hpp"
#include <algorithm>
inline blobber::Materials testMaterials() {
    using blobber::Material;
    return {Material(8, 8, std::vector<Color>(64, Color{255, 0, 0, 255})),
            Material(8, 8, std::vector<Color>(64, Color{0, 255, 0, 255})),
            Material(8, 8, std::vector<Color>(64, Color{0, 0, 255, 255}))};
}
inline blobber::Materials testGreaseMaterials() {
    auto materials = testMaterials();
    std::vector<Color> pixels(64, BLANK);
    for (int y = 1; y < 7; ++y)
        for (int x = 1; x < 7; ++x)
            pixels[y * 8 + x] = {static_cast<unsigned char>(20 + 30 * x),
                                 static_cast<unsigned char>(10 + 10 * y), 15,
                                 static_cast<unsigned char>((x + y) % 2 ? 180 : 240)};
    materials.grease = blobber::Material(8, 8, std::move(pixels));
    return materials;
}
inline bool same(const blobber::Frame& a, const blobber::Frame& b) {
    return std::equal(a.begin(), a.end(), b.begin(),
                      [](Color x, Color y) { return x.r == y.r && x.g == y.g && x.b == y.b && x.a == y.a; });
}
// Owning return helpers are test-only; production callers reuse their output buffers.
inline blobber::Frame compose(const blobber::SceneRenderer& renderer, const blobber::Map& map,
                              blobber::Player p) {
    blobber::Frame result;
    renderer.compose(map, p, result);
    return result;
}
inline blobber::Frame strafe(const blobber::SceneRenderer& renderer, const blobber::Map& map,
                             blobber::Player a, blobber::Player b, float t) {
    blobber::Frame result;
    renderer.composeStrafe(map, a, b, t, result);
    return result;
}
inline blobber::Frame transition(const blobber::Frame& a, const blobber::Frame& b, blobber::Action action,
                                 float t) {
    blobber::Frame result;
    blobber::composeTransition(a, b, action, t, result);
    return result;
}
