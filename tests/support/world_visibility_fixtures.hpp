#pragma once
#include "render/world_visibility.hpp"
#include "support/world_fixtures.hpp"
namespace blobber::test {
struct VisibilityFixture {
    WorldFixture location;
    std::vector<NpcTimings> timings;
    VisibilityFixture(unsigned types = 1) {
        world_assets::Writer writer(location.directory);
        std::vector<unsigned char> pixels(8 * 12 * 4, 255);
        for (unsigned i = 0; i < 8 * 12; ++i) {
            pixels[i * 4] = 180;
            pixels[i * 4 + 1] = 80;
            pixels[i * 4 + 2] = 40;
        }
        for (const auto* n : {"wall", "floor", "ceiling"})
            writer.add(n, 8, 12, false, pixels);
        std::vector<world_assets::Appearance> actors;
        for (unsigned type = 0; type < types; ++type) {
            world_assets::Appearance a;
            a.id = type ? "goblin.archer" : "goblin";
            a.width = .5f;
            a.height = .7f;
            for (unsigned clip = 0; clip < npcAnimationCount; ++clip) {
                auto& c = a.clips[clip];
                c.timing =
                    clip == 6 ? NpcClipTiming{{{200, 0, 0, 0}}, 1} : NpcClipTiming{{{80, 120, 0, 0}}, 2};
                c.width = clip == 4 ? .9f : 0;
                for (unsigned side = 0; side < 4; ++side)
                    for (unsigned frame = 0; frame < c.timing.count; ++frame) {
                        auto rgba = pixels;
                        for (unsigned y = 0; y < 12; ++y)
                            for (unsigned x = 0; x < 8; ++x) {
                                auto& alpha = rgba[(y * 8 + x) * 4 + 3];
                                if (frame == 0 && (x == 3 || x == 4))
                                    alpha = 0;
                                if (x == 0)
                                    alpha = 80;
                                if (clip == 6 && y < 8)
                                    alpha = 0;
                                rgba[(y * 8 + x) * 4] = static_cast<unsigned char>(40 + clip * 25);
                                rgba[(y * 8 + x) * 4 + 1] = static_cast<unsigned char>(40 + side * 45);
                            }
                        const auto name = a.id + "/" + std::to_string(clip) + "/" + std::to_string(side) +
                                          "/" + std::to_string(frame);
                        c.textures[side][frame] =
                            std::uint16_t(writer.add(name, 8, 12, true, rgba, 0,
                                                     {clip == 3 && side == 1 && frame == 1 ? .25f : .5f, 1}));
                    }
            }
            a.attachments.release[0] = {.2f, .35f};
            NpcTimings timing{};
            for (unsigned i = 0; i < npcAnimationCount; ++i)
                timing[i] = a.clips[i].timing;
            timings.push_back(timing);
            actors.push_back(a);
        }
        for (const auto* n : {"loot/bag", "fixture/key-bronze", "fixture/key-silver", "fixture/upstairs",
                              "fixture/downstairs", "fixture/endpoint", "fixture/threshold"})
            writer.add(n, 8, 12, true, pixels);
        for (const auto* n : {"door", "door-locked"})
            writer.add(n, 8, 12, false, pixels);
        for (const auto* n :
             {"arrow", "tip", "feathers", "slash", "glint", "grease", "web", "weapon", "preview", "white"})
            writer.add(std::string("effect/") + n, 8, 12, true, pixels);
        writer.finish(actors);
    }
    world_assets::Publication publication() const { return world_assets::load(location.directory); }
};
} // namespace blobber::test
