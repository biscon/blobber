#include "assets/blueprint_art.hpp"
#include "assets/blueprint_blockouts.hpp"
#include "assets/blueprint_loader.hpp"
#include "assets/fingerprint.hpp"
#include "assets/image.hpp"
#include "assets/npc_sheet.hpp"
#include "assets/world_assets.hpp"
#include "assets/world_preparation.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
namespace {
namespace fs = std::filesystem;
using namespace blobber;
using Json = nlohmann::json;
std::vector<unsigned char> pixels(const fs::path& path, unsigned& w, unsigned& h, unsigned maximum = 4096) {
    OwnedImage image(LoadImage(path.string().c_str()));
    if (!image.value.data || image.value.width <= 0 || image.value.height <= 0 || image.value.width > 4096 ||
        image.value.height > 4096)
        throw std::runtime_error("Invalid world source image: " + path.string());
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    limitWorldSprite(image, maximum);
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
std::map<std::string, std::string> sourceFingerprints(const fs::path& source, bool blockouts = false) {
    std::map<std::string, std::string> result;
    for (const auto* folder : {"npcs", "textures", "fixtures", "effects", "packs"})
        for (const auto& entry : fs::recursive_directory_iterator(source / folder))
            if (entry.is_regular_file() &&
                (entry.path().extension() == ".png" || entry.path().extension() == ".json"))
                result.emplace(fs::relative(entry.path(), source).generic_string(),
                               fingerprintFile(entry.path()));
    result.emplace("ui/items/loot-bag.png", fingerprintFile(source / "ui/items/loot-bag.png"));
    if (fs::exists(source / "blueprints"))
        for (const auto& entry : fs::directory_iterator(source / "blueprints"))
            if (entry.is_regular_file() && entry.path().extension() == ".json")
                result.emplace(fs::relative(entry.path(), source).generic_string(),
                               fingerprintFile(entry.path()));
    if (blockouts)
        result.emplace("blueprints/underkeep.json", fingerprintFile(source / "blueprints/underkeep.json"));
    return result;
}
int run(const fs::path& source, const fs::path& output, bool cropNpcs, bool blockouts) {
    if (blockouts && fs::exists(output))
        throw std::runtime_error("Blockout bake requires a new output path; never replace production assets");
    const auto blockoutCatalog = blockouts ? std::optional<blueprints::BlueprintCatalog>(
                                                 blueprints::load(source / "blueprints/underkeep.json"))
                                           : std::nullopt;
    const auto inputs = sourceFingerprints(source, blockouts);
    const Map dummy({"###", "#.#", "###"});
    const auto parsed = parseNpcMetadata(source / "npcs/level.json", dummy, {1, 1}, false);
    for (const auto& definition : parsed.definitions)
        if (definition.role == NpcRole::Hostile &&
            definition.animations[unsigned(NpcAnimation::Prone)].timing.count != 1)
            throw std::runtime_error(
                "Every hostile NPC appearance requires a single-frame four-direction prone clip");
    const auto level = read(source / "npcs/level.json");
    std::vector<std::string> types;
    for (unsigned i = 0; i < parsed.definitions.size(); ++i)
        types.push_back(parsed.world.typeId({std::uint16_t(i)}));
    const auto resolutions = npcPreparationResolutions(source / "npcs/preparation.json", types);
    world_assets::Writer writer(output);
    auto add = [&](const std::string& name, const fs::path& path, bool sprite, unsigned cutoff = 0,
                   world_assets::Anchor anchor = {}, bool crop = false, bool cutoutMaterial = false,
                   unsigned maximum = 4096) {
        unsigned w = 0, h = 0;
        auto rgba = pixels(path, w, h, maximum);
        return writer.add(name, w, h, sprite, rgba, cutoff, anchor, true, crop, cutoutMaterial);
    };
    std::map<std::string, std::uint16_t> preparedPackTextures;
    std::vector<world_assets::Pack> packs;
    std::vector<world_assets::PropMesh> meshes;
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
        pack.viewDistance = manifest.value("viewDistance", pack.viewDistance);
        pack.daylightFog = manifest.value("daylightFog", pack.daylightFog);
        pack.horizon = manifest.at("horizon").get<std::array<float, 3>>();
        const auto addPack = [&](const std::string& role, bool sprite, unsigned maximum, bool material) {
            const auto path = source / relative(manifest.at(role).get<std::string>());
            const auto key = path.lexically_normal().generic_string() + ":" + std::to_string(sprite) + ":" +
                             std::to_string(maximum) + ":" + std::to_string(material) + ":" +
                             std::to_string(role == "sky");
            const auto existing = preparedPackTextures.find(key);
            if (existing != preparedPackTextures.end())
                return existing->second;
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
            const auto id = std::uint16_t(
                writer.add("pack/" + pack.id + "/" + role, unsigned(image.value.width),
                           unsigned(image.value.height), sprite, rgba, sprite ? 16 : 0, {}, material));
            preparedPackTextures.emplace(key, id);
            return id;
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
        if (manifest.contains("decorations")) {
            for (unsigned kind = unsigned(DecorationKind::DampA); kind <= unsigned(DecorationKind::BonesB);
                 ++kind) {
                const auto name = decorationName(DecorationKind(kind));
                if (manifest["decorations"].contains(name)) {
                    const auto role = std::string("decoration-") + name;
                    auto extra = manifest;
                    extra[role] = manifest["decorations"][name];
                    // addPack reads the current manifest, so load bounded source directly.
                    unsigned w, h;
                    auto rgba = pixels(source / relative(extra[role].get<std::string>()), w, h);
                    OwnedImage image(
                        LoadImage((source / relative(extra[role].get<std::string>())).string().c_str()));
                    const float scale = std::min(1.f, 256.f / std::max(w, h));
                    if (scale < 1)
                        ImageResize(&image.value, std::max(1, int(w * scale)), std::max(1, int(h * scale)));
                    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                    const auto* first = static_cast<const unsigned char*>(image.value.data);
                    rgba.assign(first, first + std::size_t(image.value.width) * image.value.height * 4);
                    pack.decorations[kind] =
                        std::uint16_t(writer.add("pack/" + pack.id + "/" + name, image.value.width,
                                                 image.value.height, true, rgba, 16, {}, false));
                }
            }
        }
        if (manifest.contains("models")) {
            unsigned slot = 0;
            for (const auto& path : manifest["models"]) {
                if (slot >= 4 || meshes.size() >= world_assets::meshCapacity)
                    throw std::runtime_error("Too many prop meshes");
                const auto model = read(source / relative(path.get<std::string>()));
                const auto texture = model.at("texture").get<std::string>();
                const auto textureName = "pack/" + pack.id + "/atlas/" + texture;
                auto found = std::find_if(meshes.begin(), meshes.end(), [&](const auto& m) {
                    return m.name == pack.id + "/" + model.at("name").get<std::string>();
                });
                if (found != meshes.end())
                    throw std::runtime_error("Duplicate prop mesh");
                // Chest variants deliberately share their one atlas.
                static_cast<void>(textureName);
                unsigned textureId = UINT16_MAX;
                for (unsigned i = 0; i < slot; ++i) {
                    const auto previous = read(source / relative(manifest["models"][i].get<std::string>()));
                    if (previous.at("texture") == texture)
                        textureId = meshes[pack.models[i]].texture;
                }
                if (textureId == UINT16_MAX) {
                    textureId = add(textureName, source / relative(texture), false);
                    if (pack.materialCount == 32)
                        throw std::runtime_error("Prop material palette full");
                    pack.materials[pack.materialCount++] = std::uint16_t(textureId);
                }
                world_assets::PropMesh mesh;
                mesh.name = pack.id + "/" + model.at("name").get<std::string>();
                mesh.texture = std::uint16_t(textureId);
                if (!model.at("vertices").is_array() || model.at("vertices").size() > 6000)
                    throw std::runtime_error("Prop mesh capacity exceeded");
                mesh.vertices = model.at("vertices").get<std::vector<std::array<float, 8>>>();
                pack.models[slot++] = std::uint16_t(meshes.size());
                meshes.push_back(std::move(mesh));
            }
            if (slot != 4)
                throw std::runtime_error("Expected chest variants, cage and rack");
        }
        if (manifest.contains("buildings")) {
            if (!manifest["buildings"].is_array() ||
                manifest["buildings"].size() > world_assets::staticModelCapacity)
                throw std::runtime_error("Building catalog capacity exceeded");
            std::map<std::string, unsigned> atlases;
            std::map<std::string, std::uint16_t> sharedParts;
            for (const auto& binding : manifest["buildings"]) {
                world_assets::StaticModel building;
                building.id = binding.at("id").get<std::string>();
                const auto& parts = binding.at("parts");
                if (!parts.is_array() || parts.empty() || parts.size() > world_assets::staticPartCapacity)
                    throw std::runtime_error("Building part capacity exceeded");
                for (const auto& path : parts) {
                    const auto sourcePath = path.get<std::string>();
                    const auto reused = sharedParts.find(sourcePath);
                    if (reused != sharedParts.end()) {
                        building.parts.push_back(reused->second);
                        continue;
                    }
                    if (meshes.size() >= world_assets::meshCapacity)
                        throw std::runtime_error("World mesh capacity exceeded");
                    const auto model = read(source / relative(sourcePath));
                    const auto texture = model.at("texture").get<std::string>();
                    const bool cutout = model.value("cutout", false);
                    auto found = atlases.find(texture);
                    unsigned textureId;
                    if (found == atlases.end()) {
                        if (pack.materialCount == pack.materials.size())
                            throw std::runtime_error("Building material palette full");
                        textureId =
                            add("pack/" + pack.id + "/atlas/" + texture, source / relative(texture), cutout,
                                cutout ? world_assets::meshAlphaThreshold - 1 : 0, {}, false, cutout);
                        atlases.emplace(texture, textureId);
                        pack.materials[pack.materialCount++] = std::uint16_t(textureId);
                    } else
                        textureId = found->second;
                    world_assets::PropMesh mesh;
                    mesh.name = pack.id + "/" + model.at("name").get<std::string>();
                    mesh.texture = std::uint16_t(textureId);
                    mesh.architecture = true;
                    mesh.cutout = cutout;
                    mesh.water = model.value("water", false);
                    if (!model.at("vertices").is_array() || model.at("vertices").size() > 6000)
                        throw std::runtime_error("Building mesh capacity exceeded");
                    mesh.vertices = model.at("vertices").get<std::vector<std::array<float, 8>>>();
                    sharedParts.emplace(sourcePath, std::uint16_t(meshes.size()));
                    building.parts.push_back(std::uint16_t(meshes.size()));
                    meshes.push_back(std::move(mesh));
                }
                pack.buildings.push_back(std::move(building));
            }
        }
        if (manifest.contains("blueprintCatalog")) {
            const auto path = relative(manifest.at("blueprintCatalog").get<std::string>());
            if (path.parent_path() != "blueprints" || path.extension() != ".json")
                throw std::runtime_error("Blueprint catalog must be blueprints/NAME.json");
            const auto catalog = blueprints::load(source / path);
            for (const auto& binding : pack.buildings) {
                const auto& kit = catalog.kits[catalog.kit(binding.id).index];
                if (binding.parts.size() != 1)
                    throw std::runtime_error(binding.id + ": expected one grouped kit mesh");
                blueprints::validateKitArt(kit, meshes[binding.parts.front()]);
            }
        }
        packs.push_back(std::move(pack));
    }
    if (packs.empty() || packs.size() > world_assets::packCapacity)
        throw std::runtime_error("World asset pack capacity exceeded");
    if (blockoutCatalog)
        blueprints::appendBlockouts(*blockoutCatalog, writer, packs, meshes);
    for (const auto* name : {"grease", "web", "spiritual_weapon"})
        add(std::string("effect/") + (std::string(name) == "spiritual_weapon" ? "weapon" : name),
            source / "effects" / (std::string(name) + ".png"), true, 0, {}, false, false, 512);
    if (std::filesystem::exists(source / "packs/sanctuary-inn/source/flame.png"))
        add("effect/hearth-flame", source / "packs/sanctuary-inn/source/flame.png", true);
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
                                {resolutions[i] == 1 ? ImageSampling::Nearest : ImageSampling::Area,
                                 resolutions[i], generated / registration.parent_path(), nullptr});
        } else if (resolutions[i] != 1)
            throw std::runtime_error("Reduced NPC preparation requires registered source sheets");
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
    add("loot/bag", source / "ui/items/loot-bag.png", true, 0, {}, false, false, 512);
    for (const auto* name : {"key-bronze", "key-silver", "endpoint", "threshold"})
        add(std::string("fixture/") + name, source / "fixtures" / (std::string(name) + ".png"), true);
    if (sourceFingerprints(source, blockouts) != inputs)
        throw std::runtime_error("World source assets changed during preparation; rebuild");
    unsigned effectWidth = 0, effectHeight = 0;
    const auto bodyPixels = pixels(source / "effects/se01/body.png", effectWidth, effectHeight);
    if (effectWidth != 1024 || effectHeight != 1024)
        throw std::runtime_error("Invalid SE01 body atlas");
    const auto body =
        writer.add("effect/se01/body", effectWidth, effectHeight, true, bodyPixels, 0, {}, false);
    const auto emissionPixels = pixels(source / "effects/se01/emission.png", effectWidth, effectHeight);
    if (effectWidth != 1024 || effectHeight != 1024)
        throw std::runtime_error("Invalid SE01 emission atlas");
    const auto emission =
        writer.add("effect/se01/emission", effectWidth, effectHeight, true, emissionPixels, 0, {}, false);
    auto clips = world_assets::atlasEffectClips(body, emission);
    unsigned first = 8;
    for (const auto* sheet : {"arcane", "contact"}) {
        const auto loadEffect = [&](const char* layer) {
            const auto path = std::string("effects/se02/") + sheet + "-" + layer + ".png";
            const auto data = pixels(source / path, effectWidth, effectHeight);
            if (effectWidth != 1024 || effectHeight != 1024)
                throw std::runtime_error("Invalid SE02 effect atlas");
            return writer.add(std::string("effect/se02/") + sheet + "-" + layer, effectWidth, effectHeight,
                              true, data, 0, {}, false);
        };
        const auto color = loadEffect("body"), glow = loadEffect("emission");
        world_assets::mapEffectSheet(clips, first, color, glow);
        first += 8;
    }
    for (const auto* sheet : {"support", "ground"}) {
        const auto loadEffect = [&](const char* layer) {
            const auto path = std::string("effects/se03/") + sheet + "-" + layer + ".png";
            const auto data = pixels(source / path, effectWidth, effectHeight);
            if (effectWidth != 1024 || effectHeight != 1024)
                throw std::runtime_error("Invalid SE03 effect atlas");
            return writer.add(std::string("effect/se03/") + sheet + "-" + layer, effectWidth, effectHeight,
                              true, data, 0, {}, false);
        };
        const auto color = loadEffect("body"), glow = loadEffect("emission");
        world_assets::mapEffectSheet(clips, first, color, glow);
        first += 8;
    }
    writer.effects(clips);
    writer.finish(appearances, parsed.definitionFingerprint, packs, meshes);
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
        if (argc != 3 && !(argc == 4 && (std::string(argv[3]) == "--uncropped-npcs" ||
                                         std::string(argv[3]) == "--blueprint-blockouts")))
            throw std::runtime_error(
                "Usage: blobber_bake_world SOURCE OUTPUT [--uncropped-npcs|--blueprint-blockouts]");
        SetTraceLogLevel(LOG_WARNING);
        const bool blockouts = argc == 4 && std::string(argv[3]) == "--blueprint-blockouts";
        return run(fs::absolute(argv[1]), fs::absolute(argv[2]), argc == 3 || blockouts, blockouts);
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
