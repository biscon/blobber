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
