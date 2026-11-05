#pragma once
#include "assets/world_assets.hpp"
namespace blobber::test {
inline void elementalAssets(world_assets::Writer& writer) {
    std::vector<unsigned char> rgba(128 * 128 * 4, 0);
    for (unsigned y = 0; y < 128; ++y)
        for (unsigned x = 0; x < 128; ++x) {
            const auto at = (y * 128 + x) * 4;
            const auto u = x % 16, v = y % 16;
            if (u >= 3 && u <= 12 && v >= 3 && v <= 12 && !(u == 8 && v < 8)) {
                rgba[at] = 210;
                rgba[at + 1] = 90;
                rgba[at + 2] = 30;
                rgba[at + 3] = 220;
            }
        }
    const auto body = writer.add("effect/se01/body", 128, 128, true, rgba, 0, {}, false);
    const auto emission = writer.add("effect/se01/emission", 128, 128, true, rgba, 0, {}, false);
    writer.effects(world_assets::atlasEffectClips(body, emission, 16, 1));
}
} // namespace blobber::test
