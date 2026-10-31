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
    for (const auto* folder : {"npcs", "textures", "fixtures", "effects"})
        for (const auto& entry : fs::recursive_directory_iterator(source / folder))
            if (entry.is_regular_file() &&
                (entry.path().extension() == ".png" || entry.path().extension() == ".json"))
                result.emplace(fs::relative(entry.path(), source).generic_string(),
                               fingerprintFile(entry.path()));
    result.emplace("ui/items/loot-bag.png", fingerprintFile(source / "ui/items/loot-bag.png"));
    return result;
}
int run(const fs::path& source, const fs::path& output) {
    const auto inputs = sourceFingerprints(source);
    world_assets::Writer writer(output);
    auto add = [&](const std::string& name, const fs::path& path, bool sprite, unsigned cutoff = 0,
                   world_assets::Anchor anchor = {}) {
        unsigned w = 0, h = 0;
        auto rgba = pixels(path, w, h);
        return writer.add(name, w, h, sprite, rgba, cutoff, anchor);
    };
    for (const auto* name : {"wall", "floor", "ceiling"})
        add(name, source / "textures" / (std::string(name) + ".png"), false);
    add("door", source / "textures/door.png", false);
    add("door-locked", source / "textures/door-locked.png", false);
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
                                 {ref.anchorX, ref.anchorY});
                        images.emplace(name, id);
                    } else
                        id = it->second;
                    out.textures[side][frame] = std::uint16_t(id);
                }
        }
        appearances.push_back(std::move(a));
    }
    add("loot/bag", source / "ui/items/loot-bag.png", true);
    for (const auto* name : {"key-bronze", "key-silver", "upstairs", "downstairs", "endpoint", "threshold"})
        add(std::string("fixture/") + name, source / "fixtures" / (std::string(name) + ".png"), true);
    if (sourceFingerprints(source) != inputs)
        throw std::runtime_error("World source assets changed during preparation; rebuild");
    writer.finish(appearances, parsed.definitionFingerprint);
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
        if (argc != 3)
            throw std::runtime_error("Usage: blobber_bake_world SOURCE OUTPUT");
        SetTraceLogLevel(LOG_WARNING);
        return run(fs::absolute(argv[1]), fs::absolute(argv[2]));
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
