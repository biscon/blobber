#pragma once
#include "assets/world_assets.hpp"
#include <chrono>
#include <filesystem>
namespace blobber::test {
struct WorldFixture {
    std::filesystem::path directory =
        std::filesystem::temp_directory_path() /
        ("blobber-world-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    WorldFixture() {
        world_assets::Writer writer(directory);
        std::vector<unsigned char> rgba(8 * 8 * 4);
        for (unsigned i = 0; i < 64; ++i) {
            rgba[i * 4] = 200;
            rgba[i * 4 + 1] = 100;
            rgba[i * 4 + 2] = 50;
            rgba[i * 4 + 3] = 255;
        }
        for (const auto* name : {"wall", "floor", "ceiling"})
            writer.add(name, 8, 8, false, rgba);
        rgba[0] = 255;
        rgba[3] = 0;
        rgba[7] = 120;
        writer.add("sprite", 8, 8, true, rgba, 15, {.25f, .75f});
        world_assets::Appearance actor;
        actor.id = "fixture";
        actor.cutoff = 15;
        actor.attachments.release[2] = {.25f, .75f};
        actor.clips[0].timing = {{{100, 200, 0, 0}}, 2};
        actor.clips[0].width = .4f;
        for (auto& side : actor.clips[0].textures) {
            side[0] = 3;
            side[1] = 3;
        }
        writer.finish({actor});
    }
    ~WorldFixture() {
        std::error_code ec;
        std::filesystem::remove_all(directory, ec);
    }
    WorldFixture(const WorldFixture&) = delete;
};
inline Map worldRoom(int width = 25, int height = 25) {
    std::vector<std::string> rows(height, std::string(width, '.'));
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            if (!x || !y || x == width - 1 || y == height - 1)
                rows[y][x] = '#';
    return Map(rows);
}
} // namespace blobber::test
