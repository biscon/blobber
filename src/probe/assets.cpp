#include "probe/assets.hpp"
#include "assets/image.hpp"
#include "assets/npc_metadata.hpp"
#include <cmath>
#include <stdexcept>
namespace blobber::probe {
namespace {
Pixels decode(const std::filesystem::path& path) {
    OwnedImage image(LoadImage(path.string().c_str()));
    if (!image.value.data || image.value.width > 4096 || image.value.height > 4096)
        throw std::runtime_error("Invalid probe image: " + path.string());
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* p = static_cast<Color*>(image.value.data);
    return {image.value.width, image.value.height, {p, p + image.value.width * image.value.height}};
}
Pixels generated(unsigned kind) {
    const int w = kind == 5 ? 256 : 64, h = kind == 5 ? 128 : 64;
    Pixels p{w, h, std::vector<Color>(w * h)};
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            Color c{};
            if (kind == 5) {
                const float cloud = std::sin(x * 2 * pi / w * 3) + std::cos(y * .17f) +
                                    std::sin(x * 2 * pi / w * 7 + y * .1f);
                c = cloud > 1.15f ? Color{211, 224, 237, 255} : Color{56, 112, 190, 255};
            } else if (kind == 4) {
                c = {110, 22, 38, 255};
                if (x < 4 || x >= 60 || y < 4 || y >= 60 || std::abs(x - 32) + std::abs(y - 32) < 13)
                    c = {218, 176, 64, 255};
            } else {
                const bool mortar = kind == 0   ? y % 16 < 2 || (x + (y / 16 % 2) * 16) % 32 < 2
                                    : kind == 3 ? x % 16 < 2
                                                : x % 16 < 1 || y % 16 < 1;
                const int noise = (x * 13 + y * 23 + x * y) % 13;
                c = kind == 3 ? Color{static_cast<unsigned char>(102 + noise), 65, 30, 255}
                              : Color{static_cast<unsigned char>(107 + noise),
                                      static_cast<unsigned char>(111 + noise),
                                      static_cast<unsigned char>(107 + noise), 255};
                if (mortar)
                    c = {38, 40, 39, 255};
            }
            p.colors[y * w + x] = c;
        }
    return p;
}
} // namespace
Art loadArt(bool shipped, const std::filesystem::path& root, const Scene& scene) {
    Art art;
    const char* names[]{"wall.png", "floor.png", "ceiling.png", "door.png"};
    for (unsigned i = 0; i < 6; ++i)
        art.surfaces[i] = shipped && i < 4 ? decode(root / "textures" / names[i]) : generated(i);
    if (shipped) {
        auto parsed = parseNpcMetadata(root / "npcs/level.json", scene.map, scene.spawn.cell, false);
        const NpcDefinition* def = nullptr;
        for (std::size_t i = 0; i < parsed.definitions.size(); ++i)
            if (parsed.world.typeId({static_cast<std::uint16_t>(i)}) == "goblin")
                def = &parsed.definitions[i];
        if (!def)
            throw std::runtime_error("Probe requires goblin metadata");
        art.npc.width = def->width;
        art.npc.height = def->height;
        art.npc.alphaCutoff = def->alphaCutoff;
        art.npc.sampling = def->areaSampling ? ImageSampling::Area : ImageSampling::Nearest;
        auto& clip = art.npc.animations[0];
        clip.timing = def->animations[0].timing;
        clip.width = def->animations[0].width;
        for (unsigned side = 0; side < 4; ++side)
            for (const auto& ref : def->animations[0].views[side]) {
                auto p = decode(ref.path);
                clip.views[side].push_back(
                    {p.width, p.height, std::move(p.colors), ref.anchorX, ref.anchorY});
            }
    } else {
        auto& clip = art.npc.animations[0];
        clip.timing = {{600, 600}, 2};
        for (unsigned side = 0; side < 4; ++side)
            for (int frame = 0; frame < 2; ++frame) {
                NpcImage im{32, 48, std::vector<Color>(32 * 48, BLANK), .5f, 1};
                for (int y = 5; y < 48; ++y)
                    for (int x = 4; x < 28; ++x) {
                        if ((y < 14 && (x < 9 || x > 23)) || (y > 36 && x > 12 && x < 20))
                            continue;
                        Color c = side == 1 ? Color{91, 117, 47, 255} : Color{116, 174, 62, 255};
                        if (y == 13 && (x == 13 || x == 21))
                            c = RED;
                        if (x == 4 || x == 27)
                            c.a = 120;
                        if (y > 22 && y < 31 && x > 9 + frame && x < 17 + frame)
                            continue; // alpha hole
                        im.pixels[y * 32 + x] = c;
                    }
                clip.views[side].push_back(std::move(im));
            }
    }
    validateNpcAppearance(art.npc);
    if (!art.npc.animations[0].timing.count)
        throw std::runtime_error("Probe requires an idle clip");
    auto hash = [&](const void* ptr, std::size_t n) {
        const auto* p = static_cast<const unsigned char*>(ptr);
        for (std::size_t i = 0; i < n; ++i) {
            art.fingerprint ^= p[i];
            art.fingerprint *= 1099511628211ull;
        }
    };
    for (const auto& p : art.surfaces) {
        art.bytes += p.colors.size() * 4;
        hash(&p.width, sizeof(p.width));
        hash(&p.height, sizeof(p.height));
        hash(p.colors.data(), p.colors.size() * 4);
    }
    hash(&art.npc.width, sizeof(float));
    hash(&art.npc.height, sizeof(float));
    hash(&art.npc.alphaCutoff, 1);
    for (const auto& views : art.npc.animations[0].views)
        for (const auto& im : views) {
            art.bytes += im.pixels.size() * 4;
            hash(im.pixels.data(), im.pixels.size() * 4);
            hash(&im.anchorX, sizeof(float));
            hash(&im.anchorY, sizeof(float));
        }
    if (art.bytes > 128u * 1024 * 1024)
        throw std::runtime_error("Probe source art exceeds 128 MiB");
    return art;
}
Pixels premultiplied(const NpcImage& im, unsigned char cutoff) {
    Pixels p{im.width, im.height, im.pixels};
    for (auto& c : p.colors) {
        if (c.a <= cutoff)
            c = BLANK;
        else {
            c.r = (unsigned(c.r) * c.a + 127) / 255;
            c.g = (unsigned(c.g) * c.a + 127) / 255;
            c.b = (unsigned(c.b) * c.a + 127) / 255;
        }
    }
    return p;
}
} // namespace blobber::probe
