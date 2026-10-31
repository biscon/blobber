#include "assets/fingerprint.hpp"
#include "assets/image.hpp"
#include "assets/npc_sheet.hpp"
#include "assets/world_assets.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
namespace {
namespace fs = std::filesystem;
using namespace blobber;
using Json = nlohmann::json;
std::vector<unsigned char> pixels(const fs::path& path, unsigned& w, unsigned& h) {
    OwnedImage image(LoadImage(path.string().c_str()));
    if (!image.value.data || image.value.width <= 0 || image.value.height <= 0 || image.value.width > 4096 ||
        image.value.height > 4096)
        throw std::runtime_error("Invalid world source image: " + path.string());
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    w = unsigned(image.value.width);
    h = unsigned(image.value.height);
    const auto* first = static_cast<const unsigned char*>(image.value.data);
    return {first, first + std::size_t(w) * h * 4};
}
Json read(const fs::path& p) {
    if (fs::file_size(p) > 1024 * 1024)
        throw std::runtime_error("Source JSON exceeds 1 MiB");
    std::ifstream in(p);
    return Json::parse(in);
}
fs::path relative(const std::string& text) {
    fs::path p(text);
    if (p.empty() || p.is_absolute())
        throw std::runtime_error("Expected relative world asset path");
    for (const auto& e : p)
        if (e == "..")
            throw std::runtime_error("World asset escapes source directory");
    return p;
}
std::map<std::string, std::string> sourceFingerprints(const fs::path& source) {
    std::map<std::string, std::string> result;
    for (const auto* folder : {"npcs", "textures", "fixtures", "effects", "packs"})
        for (const auto& entry : fs::recursive_directory_iterator(source / folder))
            if (entry.is_regular_file() &&
                (entry.path().extension() == ".png" || entry.path().extension() == ".json"))
                result.emplace(fs::relative(entry.path(), source).generic_string(),
                               fingerprintFile(entry.path()));
    result.emplace("ui/items/loot-bag.png", fingerprintFile(source / "ui/items/loot-bag.png"));
    return result;
}
int run(const fs::path& source, const fs::path& output, bool cropNpcs) {
    const auto inputs = sourceFingerprints(source);
    world_assets::Writer writer(output);
    auto add = [&](const std::string& name, const fs::path& path, bool sprite, unsigned cutoff = 0,
                   world_assets::Anchor anchor = {}, bool crop = false) {
        unsigned w = 0, h = 0;
        auto rgba = pixels(path, w, h);
        return writer.add(name, w, h, sprite, rgba, cutoff, anchor, true, crop);
    };
    std::vector<world_assets::Pack> packs;
    std::vector<fs::path> packDirectories;
    for (const auto& entry : fs::directory_iterator(source / "packs"))
        if (entry.is_directory())
            packDirectories.push_back(entry.path());
    std::sort(packDirectories.begin(), packDirectories.end());
    for (const auto& directory : packDirectories) {
        const auto manifest = read(directory / "pack.json");
        world_assets::Pack pack;
        pack.id = manifest.at("id");
        pack.worldScale = manifest.value("worldScale", 1.f);
        pack.horizon = manifest.at("horizon").get<std::array<float, 3>>();
        const auto addPack = [&](const std::string& role, bool sprite, unsigned maximum, bool material) {
            const auto path = source / relative(manifest.at(role).get<std::string>());
            OwnedImage image(LoadImage(path.string().c_str()));
            if (!image.value.data || image.value.width > 4096 || image.value.height > 4096)
                throw std::runtime_error("Invalid pack artwork: " + path.string());
            const float scale =
                std::min(1.f, float(maximum) / std::max(image.value.width, image.value.height));
            if (scale < 1)
                ImageResize(&image.value, std::max(1, int(image.value.width * scale)),
                            std::max(1, int(image.value.height * scale)));
            ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            const auto* first = static_cast<const unsigned char*>(image.value.data);
            std::vector<unsigned char> rgba(first,
                                            first + std::size_t(image.value.width) * image.value.height * 4);
            if (!sprite)
                for (std::size_t i = 3; i < rgba.size(); i += 4)
                    rgba[i] = 255;
            // Join generated panorama edges without changing the central cloud composition.
            if (role == "sky") {
                const auto width = unsigned(image.value.width), height = unsigned(image.value.height);
                const auto band = std::min(32u, width / 4);
                for (unsigned y = 0; y < height; ++y)
                    for (unsigned c = 0; c < 3; ++c) {
                        const float seam = (rgba[(std::size_t(y) * width) * 4 + c] +
                                            rgba[(std::size_t(y) * width + width - 1) * 4 + c]) *
                                           .5f;
                        for (unsigned x = 0; x < band; ++x) {
                            const float t = float(x) / band;
                            for (auto column : {x, width - 1 - x}) {
                                auto& value = rgba[(std::size_t(y) * width + column) * 4 + c];
                                value = static_cast<unsigned char>(std::round(seam * (1 - t) + value * t));
                            }
                        }
                    }
            }
            return std::uint16_t(writer.add("pack/" + pack.id + "/" + role, unsigned(image.value.width),
                                            unsigned(image.value.height), sprite, rgba, sprite ? 16 : 0, {},
                                            material));
        };
        for (const auto* role : {"wall", "floor", "ceiling", "stairWall", "stairFloor"})
            pack.materials[pack.materialCount++] = addPack(role, false, 256, true);
        pack.door = addPack("door", false, 512, false);
        pack.lockedDoor = addPack("lockedDoor", false, 512, false);
        pack.doorEdge = addPack("doorEdge", false, 256, false);
        pack.crest = addPack("crest", true, 256, false);
        pack.vine = addPack("vine", true, 384, false);
        pack.sky = addPack("sky", false, 2048, false);
        if (manifest.contains("guardianEmblem"))
            pack.guardianEmblem = addPack("guardianEmblem", true, 256, false);
        packs.push_back(std::move(pack));
    }
    if (packs.empty() || packs.size() > 8)
        throw std::runtime_error("Expected one to eight world asset packs");
    for (const auto* name : {"grease", "web", "spiritual_weapon"})
        add(std::string("effect/") + (std::string(name) == "spiritual_weapon" ? "weapon" : name),
            source / "effects" / (std::string(name) + ".png"), true);
    // Code-authored source artwork; reusable base images replace perspective rasters.
    for (unsigned kind = 0; kind < 7; ++kind) {
        constexpr const char* names[]{"arrow", "tip", "feathers", "slash", "glint", "preview", "white"};
        std::vector<unsigned char> rgba(64 * 64 * 4);
        for (unsigned y = 0; y < 64; ++y)
            for (unsigned x = 0; x < 64; ++x) {
                const float u = (x + .5f) / 64 - .5f, v = (y + .5f) / 64 - .5f;
                bool inside = false;
                unsigned char r = 242, g = 231, b = 194, a = 255;
                if (kind == 0) {
                    inside = (std::abs(v) < .035f && u > -.4f && u < .32f) ||
                             (u > .22f && u < .48f && std::abs(v) < (.48f - u) * .5f) ||
                             (u < -.22f && u > -.46f && std::abs(v) < (.47f + u) * .55f + .03f);
                    r = 205;
                    g = 196;
                    b = 163;
                }
                if (kind == 1)
                    inside = u * u + v * v < .02f || (std::abs(u) < .035f && std::abs(v) < .2f);
                if (kind == 2)
                    inside = (std::abs(v) < .045f && std::abs(u) < .25f) ||
                             (std::abs(u) < .045f && std::abs(v) < .25f);
                if (kind == 3) {
                    inside = u > -.46f && u < .46f && std::abs(v - .12f * u * u) < .035f;
                    a = 230;
                }
                if (kind == 4) {
                    inside = (std::abs(v) < .035f && std::abs(u) < .42f) ||
                             (std::abs(u) < .035f && std::abs(v) < .42f) || u * u + v * v < .012f;
                    r = 255;
                    g = b = 255;
                    a = 240;
                }
                if (kind >= 5) {
                    inside = true;
                    r = g = b = 255;
                    a = kind == 6 ? 255 : ((x < 2 || y < 2 || x >= 62 || y >= 62) ? 155 : 38);
                }
                const auto at = (y * 64 + x) * 4;
                rgba[at] = r;
                rgba[at + 1] = g;
                rgba[at + 2] = b;
                rgba[at + 3] = inside ? a : 0;
            }
        writer.add(std::string("effect/") + names[kind], 64, 64, true, rgba);
    }
    const Map dummy({"###", "#.#", "###"});
    const auto parsed = parseNpcMetadata(source / "npcs/level.json", dummy, {1, 1}, false);
    for (const auto& definition : parsed.definitions)
        if (definition.animations[unsigned(NpcAnimation::Prone)].timing.count != 1)
            throw std::runtime_error(
                "Every gameplay NPC appearance requires a single-frame four-direction prone clip");
    const auto level = read(source / "npcs/level.json");
    const auto scratch = output / "source-frames.tmp";
    fs::remove_all(scratch);
    fs::create_directories(scratch);
    struct Cleanup {
        fs::path p;
        ~Cleanup() {
            std::error_code ec;
            fs::remove_all(p, ec);
        }
    } cleanup{scratch};
    std::vector<world_assets::Appearance> appearances;
    for (unsigned i = 0; i < parsed.definitions.size(); ++i) {
        const auto definition = source / "npcs" / relative(level.at("definitions")[i].get<std::string>());
        const auto json = read(definition);
        const auto& def = parsed.definitions[i];
        const bool sheets = json.contains("source_sheets");
        const auto generated = scratch / std::to_string(i);
        if (sheets) {
            const auto registration = relative(json.at("source_sheets").get<std::string>());
            prepareNpcAnimation(definition.parent_path() / registration,
                                {ImageSampling::Nearest, 1, generated / registration.parent_path(), nullptr});
        }
        world_assets::Appearance a;
        a.id = parsed.world.typeId({std::uint16_t(i)});
        a.width = def.width;
        a.height = def.height;
        a.cutoff = def.alphaCutoff;
        a.attachments = def.attachments;
        std::map<std::string, unsigned> images;
        for (unsigned c = 0; c < npcAnimationCount; ++c) {
            auto& out = a.clips[c];
            out.timing = def.animations[c].timing;
            out.width = def.animations[c].width;
            // Legacy static definitions are an idle clip, like npcTimings().
            if (c == 0 && !out.timing.count)
                out.timing = npcTimings(def)[0];
            for (unsigned side = 0; side < 4; ++side)
                for (unsigned frame = 0; frame < out.timing.count; ++frame) {
                    const auto& refs = def.animations[c].views[side];
                    if (refs.empty() && !(c == 0 && !def.animations[0].timing.count))
                        continue;
                    const auto& ref = refs.empty() ? def.views[side] : refs[frame];
                    const auto rel =
                        relative(ref.path.lexically_relative(definition.parent_path()).generic_string());
                    const std::string name = a.id + "/" + rel.generic_string();
                    auto it = images.find(name);
                    unsigned id;
                    if (it == images.end()) {
                        id = add(name, sheets ? generated / rel : ref.path, true, def.alphaCutoff,
                                 {ref.anchorX, ref.anchorY}, cropNpcs);
                        images.emplace(name, id);
                    } else
                        id = it->second;
                    out.textures[side][frame] = std::uint16_t(id);
                }
        }
        appearances.push_back(std::move(a));
    }
    add("loot/bag", source / "ui/items/loot-bag.png", true);
    for (const auto* name : {"key-bronze", "key-silver", "endpoint", "threshold"})
        add(std::string("fixture/") + name, source / "fixtures" / (std::string(name) + ".png"), true);
    if (sourceFingerprints(source) != inputs)
        throw std::runtime_error("World source assets changed during preparation; rebuild");
    writer.finish(appearances, parsed.definitionFingerprint, packs);
    {
        std::ofstream provenance(output / "sources.json");
        provenance << Json(inputs).dump(2) << '\n';
        if (!provenance)
            throw std::runtime_error("Cannot write world provenance");
    }
    const auto publication = world_assets::load(output);
    std::cout << "World assets: textures=" << publication.textures.size()
              << " mip_bytes=" << publication.textureBytes << " alpha_bytes=" << publication.alphaBytes
              << '\n';
    return 0;
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc != 3 && !(argc == 4 && std::string(argv[3]) == "--uncropped-npcs"))
            throw std::runtime_error("Usage: blobber_bake_world SOURCE OUTPUT [--uncropped-npcs]");
        SetTraceLogLevel(LOG_WARNING);
        return run(fs::absolute(argv[1]), fs::absolute(argv[2]), argc == 3);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
