#pragma once
#include "render/world_visibility.hpp"
#include "support/underkeep_assets.hpp"
#include "support/world_fixtures.hpp"
namespace blobber::test {
struct VisibilityFixture {
    WorldFixture location;
    std::vector<NpcTimings> timings;
    VisibilityFixture(unsigned types = 1, unsigned padding = 0, bool crop = false, bool opaque = false,
                      bool hearth = false) {
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
            constexpr const char* names[]{"goblin",    "goblin.archer", "skeleton", "zombie",
                                          "giant.rat", "goblin_shaman", "bugbear",  "innkeeper",
                                          "mayor",     "boric",         "balin"};
            a.id = names[type % 11];
            a.width = .5f;
            a.height = .7f;
            for (unsigned clip = 0; clip < npcAnimationCount; ++clip) {
                // Only the generic first appearance needs the new dialogue reactions.
                // Keep multi-type fixtures within their existing bounded texture workload.
                if (clip >= unsigned(NpcAnimation::PullDocument) && type != 0)
                    continue;
                if (type % 11 >= 7 && clip != unsigned(NpcAnimation::Idle) &&
                    clip != unsigned(NpcAnimation::Talk))
                    continue;
                auto& c = a.clips[clip];
                c.timing =
                    clip == 6 ? NpcClipTiming{{{200, 0, 0, 0}}, 1} : NpcClipTiming{{{80, 120, 0, 0}}, 2};
                c.width = clip == 4 ? .9f : 0;
                for (unsigned side = 0; side < 4; ++side) {
                    if (type % 11 >= 7 && clip == unsigned(NpcAnimation::Talk) && side != 0)
                        continue;
                    for (unsigned frame = 0; frame < c.timing.count; ++frame) {
                        auto rgba = pixels;
                        for (unsigned y = 0; y < 12; ++y)
                            for (unsigned x = 0; x < 8; ++x) {
                                auto& alpha = rgba[(y * 8 + x) * 4 + 3];
                                if (!opaque && frame == 0 && (x == 3 || x == 4))
                                    alpha = 0;
                                if (!opaque && x == 0)
                                    alpha = 80;
                                if (clip == 6 && y < 8)
                                    alpha = 0;
                                rgba[(y * 8 + x) * 4] = static_cast<unsigned char>(40 + clip * 25);
                                rgba[(y * 8 + x) * 4 + 1] = static_cast<unsigned char>(40 + side * 45);
                            }
                        const auto name = a.id + "/" + std::to_string(clip) + "/" + std::to_string(side) +
                                          "/" + std::to_string(frame);
                        const unsigned width = 8 + 2 * padding, height = 12 + 2 * padding;
                        std::vector<unsigned char> padded(std::size_t(width) * height * 4);
                        for (unsigned y = 0; y < 12; ++y)
                            std::copy_n(rgba.begin() + y * 8 * 4, 8 * 4,
                                        padded.begin() + ((y + padding) * width + padding) * 4);
                        c.textures[side][frame] = std::uint16_t(
                            writer.add(name, width, height, true, padded, 0,
                                       {clip == 3 && side == 1 && frame == 1 ? .25f : .5f, 1}, true, crop));
                    }
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
        const auto door = writer.add("door", 8, 12, false, pixels, 0, {}, false);
        const auto locked = writer.add("door-locked", 8, 12, false, pixels, 0, {}, false);
        for (const auto* n :
             {"arrow", "tip", "feathers", "slash", "glint", "grease", "web", "weapon", "preview", "white"})
            writer.add(std::string("effect/") + n, 8, 12, true, pixels);
        if (hearth)
            writer.add("effect/hearth-flame", 8, 12, true, pixels);
        const auto crest = writer.add("decoration/crest", 8, 12, true, pixels);
        const auto vine = writer.add("decoration/vine", 8, 12, true, pixels);
        finishWithUnderkeep(writer, actors, 0, door, locked, crest, vine);
    }
    world_assets::Publication publication() const { return world_assets::load(location.directory); }
};
} // namespace blobber::test
