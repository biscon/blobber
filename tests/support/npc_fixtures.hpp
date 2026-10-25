#pragma once
#include "render/npc_sprites.hpp"
inline blobber::NpcAppearance testNpcAppearance() {
    blobber::NpcAppearance a;
    const std::array<Color, 4> colors{
        {{240, 40, 20, 255}, {20, 240, 40, 255}, {40, 20, 240, 255}, {240, 220, 20, 255}}};
    for (int side = 0; side < 4; ++side) {
        auto& image = a.views[side];
        image.width = 8;
        image.height = 12;
        image.pixels.assign(96, colors[side]);
        // A transparent slit plus asymmetric translucent edge expose compositing errors.
        for (int y = 0; y < 12; ++y) {
            image.pixels[y * 8 + 3].a = 0;
            image.pixels[y * 8 + 4].a = 0;
            image.pixels[y * 8].a = 128;
        }
    }
    return a;
}

inline blobber::NpcAppearance testAnimatedNpcAppearance() {
    auto source = testNpcAppearance();
    blobber::NpcAppearance a;
    for (std::size_t clip = 0; clip < blobber::npcAnimationCount; ++clip) {
        auto& c = a.animations[clip];
        c.timing.count = clip == 2 ? 4 : 2;
        c.timing.durationMs = clip == 2 ? std::array<std::uint32_t, 4>{70, 25, 25, 35}
                                        : std::array<std::uint32_t, 4>{60, 60, 0, 0};
        for (std::size_t side = 0; side < 4; ++side) {
            if (clip == 2 && side != 0)
                continue;
            for (std::size_t frame = 0; frame < c.timing.count; ++frame) {
                auto image = source.views[side];
                for (auto& pixel : image.pixels) {
                    pixel.r = static_cast<unsigned char>(30 + clip * 50 + frame * 20);
                    pixel.g = static_cast<unsigned char>(40 + side * 40);
                    pixel.b = static_cast<unsigned char>(20 + frame * 60);
                }
                c.views[side].push_back(std::move(image));
            }
        }
    }
    a.animations[1].timing.durationMs = {30, 30, 0, 0};
    return a;
}
