#include "assets/world_assets.hpp"
#include "assets/fingerprint.hpp"
#include "core/preparation.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
namespace blobber::world_assets {
namespace {
using Json = nlohmann::json;
void require(bool v) {
    if (!v)
        throw std::runtime_error("Invalid world publication; rebuild blobber_bake_world_assets");
}
std::uint64_t integer(const Json& value, std::uint64_t limit = UINT64_MAX) {
    require(value.is_number_integer());
    if (!value.is_number_unsigned())
        require(value.get<std::int64_t>() >= 0);
    const auto n = value.get<std::uint64_t>();
    require(n <= limit);
    return n;
}
void dimensions(unsigned w, unsigned h) {
    require(w > 0 && h > 0 && w <= 4096 && h <= 4096);
}
void boundedAdd(std::uint64_t& n, std::uint64_t add, std::uint64_t limit) {
    require(n <= limit && add <= limit - n);
    n += add;
}
void validAnchor(Anchor a) {
    require(std::isfinite(a.x) && std::isfinite(a.y) && a.x >= 0 && a.x <= 1 && a.y > 0 && a.y <= 1);
}
void validCanvas(const Texture& t) {
    dimensions(t.width, t.height);
    dimensions(t.canvas.width, t.canvas.height);
    require(t.width <= t.canvas.width && t.height <= t.canvas.height &&
            t.canvas.x <= t.canvas.width - t.width && t.canvas.y <= t.canvas.height - t.height);
    require(t.clamp ||
            (t.canvas.x == 0 && t.canvas.y == 0 && t.canvas.width == t.width && t.canvas.height == t.height));
}
void validPack(const Pack& p, const std::vector<Texture>& textures) {
    require(!p.id.empty() && p.id.size() <= 32 && p.materialCount >= 3 && p.materialCount <= 32 &&
            std::all_of(p.id.begin(), p.id.end(),
                        [](unsigned char c) {
                            return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
                        }) &&
            std::isfinite(p.worldScale) && p.worldScale > 0 && p.worldScale <= 16);
    for (unsigned i = 0; i < p.materialCount; ++i)
        require(p.materials[i] < textures.size() && textures[p.materials[i]].material &&
                (i >= 3 || !textures[p.materials[i]].clamp));
    for (auto i : {p.door, p.lockedDoor, p.doorEdge, p.sky})
        require(i < textures.size() && !textures[i].clamp && !textures[i].material);
    for (auto i : {p.crest, p.vine})
        require(i < textures.size() && textures[i].clamp && !textures[i].material);
    if (p.guardianEmblem != UINT16_MAX)
        require(p.guardianEmblem < textures.size() && textures[p.guardianEmblem].clamp &&
                !textures[p.guardianEmblem].material);
    require(std::isfinite(p.viewDistance) && p.viewDistance >= 16 && p.viewDistance <= 40);
    for (float c : p.horizon)
        require(std::isfinite(c) && c >= 0 && c <= 1);
    for (float c : p.daylightFog)
        require(std::isfinite(c));
    require(p.daylightFog[0] >= 0 && p.daylightFog[0] <= 64 && p.daylightFog[1] > 0 &&
            p.daylightFog[1] <= 64 && p.daylightFog[2] >= 0 && p.daylightFog[2] <= 1);
}

void validMeshes(const std::vector<PropMesh>& meshes, const std::vector<Texture>& textures,
                 const std::vector<Pack>& packs) {
    require(meshes.size() <= meshCapacity);
    std::set<std::string> names;
    for (const auto& m : meshes) {
        require(!m.name.empty() && m.name.size() <= 64 && names.insert(m.name).second &&
                m.texture < textures.size() && textures[m.texture].material);
        require(textures[m.texture].clamp == m.cutout && (!m.cutout || m.architecture));
        require(!m.water || (m.architecture && !m.cutout && textures[m.texture].width % 4 == 0 &&
                             textures[m.texture].height % 2 == 0));
        const auto count = m.vertices.empty() ? m.vertexCount : m.vertices.size();
        require(count > 0 && count <= 6000 && count % 3 == 0);
        for (const auto& v : m.vertices) {
            for (float n : v)
                require(std::isfinite(n));
            const float extent = m.architecture ? 4.f : 1.f;
            require(std::abs(v[0]) <= extent && v[1] >= (m.architecture ? 0.f : -.001f) && v[1] <= extent &&
                    std::abs(v[2]) <= extent && v[6] >= 0 && v[6] <= 1 && v[7] >= 0 && v[7] <= 1);
            require(!m.water || (v[6] <= .25f && v[7] <= .5f));
            const float n = v[3] * v[3] + v[4] * v[4] + v[5] * v[5];
            require(n > .99f && n < 1.01f);
        }
        for (unsigned i = 0; i < m.vertices.size(); i += 3) {
            const auto& a = m.vertices[i];
            const auto& b = m.vertices[i + 1];
            const auto& c = m.vertices[i + 2];
            const float x = (b[1] - a[1]) * (c[2] - a[2]) - (b[2] - a[2]) * (c[1] - a[1]);
            const float y = (b[2] - a[2]) * (c[0] - a[0]) - (b[0] - a[0]) * (c[2] - a[2]);
            const float z = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
            require(x * a[3] + y * a[4] + z * a[5] > 1e-8f);
        }
    }
    for (const auto& p : packs) {
        require(p.materialCount <= p.materials.size());
        for (auto id : p.decorations)
            if (id != UINT16_MAX)
                require(id < textures.size() && textures[id].clamp && !textures[id].material);
        for (unsigned slot = 0; slot < p.models.size(); ++slot) {
            const auto id = p.models[slot];
            if (id != UINT16_MAX) {
                require(id < meshes.size() && !meshes[id].architecture);
                bool bound = false;
                for (unsigned i = 0; i < p.materialCount; ++i)
                    bound |= p.materials[i] == meshes[id].texture;
                require(bound);
                for (const auto& v : meshes[id].vertices)
                    require(std::abs(v[0]) <= .5f && std::abs(v[2]) <= (slot == 3 ? 1.f : .5f));
            }
        }
        require(p.buildings.size() <= staticModelCapacity);
        std::set<std::string> buildingIds;
        for (const auto& b : p.buildings) {
            require(!b.id.empty() && b.id.size() <= 32 && buildingIds.insert(b.id).second &&
                    std::all_of(b.id.begin(), b.id.end(),
                                [](unsigned char c) {
                                    return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
                                           c == '_';
                                }) &&
                    !b.parts.empty() && b.parts.size() <= staticPartCapacity);
            unsigned vertices = 0;
            std::set<unsigned> parts;
            for (auto id : b.parts) {
                require(id < meshes.size() && meshes[id].architecture && parts.insert(id).second);
                const auto& mesh = meshes[id];
                vertices += unsigned(mesh.vertices.empty() ? mesh.vertexCount : mesh.vertices.size());
                require(vertices <= staticVertexCapacity);
                require(std::find(p.materials.begin(), p.materials.begin() + p.materialCount, mesh.texture) !=
                        p.materials.begin() + p.materialCount);
            }
        }
    }
}
Json appearanceJson(const Appearance& a) {
    Json clips = Json::array();
    for (const auto& c : a.clips)
        clips.push_back({{"width", c.width},
                         {"count", c.timing.count},
                         {"durations", c.timing.durationMs},
                         {"textures", c.textures}});
    Json attachments = Json::array();
    for (unsigned s = 0; s < 4; ++s)
        attachments.push_back({a.attachments.release[s].x, a.attachments.release[s].y,
                               a.attachments.torso[s].x, a.attachments.torso[s].y});
    return {{"id", a.id},         {"width", a.width}, {"height", a.height},
            {"cutoff", a.cutoff}, {"clips", clips},   {"attachments", attachments}};
}
} // namespace
unsigned mipLevels(unsigned w, unsigned h) {
    dimensions(w, h);
    unsigned n = 1;
    while (w > 1 || h > 1) {
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
        ++n;
    }
    return n;
}
std::size_t mipBytes(unsigned w, unsigned h) {
    dimensions(w, h);
    std::size_t n = 0;
    for (;;) {
        n += std::size_t(w) * h * 4;
        if (w == 1 && h == 1)
            return n;
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
    }
}
std::vector<unsigned char> preparePixels(const std::vector<unsigned char>& rgba, unsigned w, unsigned h,
                                         unsigned cutoff, bool sprite, std::vector<unsigned char>& alpha) {
    dimensions(w, h);
    require(rgba.size() == std::size_t(w) * h * 4 && cutoff < 255);
    std::vector<unsigned char> out;
    out.reserve(mipBytes(w, h));
    out = rgba;
    alpha.clear();
    if (sprite)
        alpha.resize(std::size_t(w) * h);
    for (std::size_t p = 0; p < rgba.size(); p += 4) {
        const unsigned a = sprite ? (rgba[p + 3] <= cutoff ? 0 : rgba[p + 3]) : 255;
        if (sprite)
            alpha[p / 4] = static_cast<unsigned char>(a);
        else
            require(rgba[p + 3] == 255); // World surfaces are opaque in R02.
        for (unsigned c = 0; c < 3; ++c)
            out[p + c] = static_cast<unsigned char>((unsigned(rgba[p + c]) * a + 127) / 255);
        out[p + 3] = static_cast<unsigned char>(a);
    }
    std::size_t previous = 0;
    while (w > 1 || h > 1) {
        const unsigned nw = std::max(1u, w / 2), nh = std::max(1u, h / 2);
        const auto start = out.size();
        out.resize(start + std::size_t(nw) * nh * 4);
        // Integer rational-area weights include odd trailing rows/columns exactly.
        for (unsigned y = 0; y < nh; ++y)
            for (unsigned x = 0; x < nw; ++x) {
                const unsigned l = x * w, r = (x + 1) * w, t = y * h, b = (y + 1) * h;
                for (unsigned c = 0; c < 4; ++c) {
                    std::uint64_t sum = 0;
                    for (unsigned sy = t / nh; sy < (b + nh - 1) / nh; ++sy)
                        for (unsigned sx = l / nw; sx < (r + nw - 1) / nw; ++sx) {
                            const unsigned wx = std::min(r, (sx + 1) * nw) - std::max(l, sx * nw);
                            const unsigned wy = std::min(b, (sy + 1) * nh) - std::max(t, sy * nh);
                            sum +=
                                std::uint64_t(out[previous + (std::size_t(sy) * w + sx) * 4 + c]) * wx * wy;
                        }
                    out[start + (std::size_t(y) * nw + x) * 4 + c] =
                        static_cast<unsigned char>((sum + std::uint64_t(w) * h / 2) / (std::uint64_t(w) * h));
                }
            }
        previous = start;
        w = nw;
        h = nh;
    }
    return out;
}
Writer::Writer(const std::filesystem::path& dir) : directory_(dir), temporary_(dir / "textures.bin.tmp") {
    std::filesystem::create_directories(dir);
    out_.open(temporary_, std::ios::binary | std::ios::trunc);
    require(bool(out_));
}
Writer::~Writer() {
    out_.close();
    std::error_code ec;
    std::filesystem::remove(temporary_, ec);
}
void Writer::bytes(const std::vector<unsigned char>& b) {
    require(offset_ <= installedBudget && b.size() <= installedBudget - offset_);
    out_.write(reinterpret_cast<const char*>(b.data()), std::streamsize(b.size()));
    require(bool(out_));
    fingerprintBytes(hash_, b.data(), b.size());
    offset_ += b.size();
}
unsigned Writer::add(const std::string& name, unsigned w, unsigned h, bool sprite,
                     const std::vector<unsigned char>& rgba, unsigned cutoff, Anchor anchor, bool material,
                     bool crop, bool cutoutMaterial) {
    require(publication_.textures.size() < catalogTextureCapacity && !name.empty() && name.size() <= 256);
    for (const auto& t : publication_.textures)
        require(t.name != name);
    validAnchor(anchor);
    dimensions(w, h);
    require(rgba.size() == std::size_t(w) * h * 4 && cutoff < 255 && (!crop || sprite));
    require(!cutoutMaterial || (sprite && material && !crop));
    SpriteCanvas canvas{w, h, 0, 0};
    std::vector<unsigned char> cropped;
    if (crop) {
        unsigned left = w, top = h, right = 0, bottom = 0;
        for (unsigned y = 0; y < h; ++y)
            for (unsigned x = 0; x < w; ++x)
                if (rgba[(std::size_t(y) * w + x) * 4 + 3] > cutoff) {
                    left = std::min(left, x);
                    top = std::min(top, y);
                    right = std::max(right, x + 1);
                    bottom = std::max(bottom, y + 1);
                }
        require(left < right && top < bottom);
        canvas.x = left > 2 ? left - 2 : 0;
        canvas.y = top > 2 ? top - 2 : 0;
        right = std::min(w, right + 2);
        bottom = std::min(h, bottom + 2);
        w = right - canvas.x;
        h = bottom - canvas.y;
        cropped.resize(std::size_t(w) * h * 4);
        for (unsigned y = 0; y < h; ++y) {
            const auto start = rgba.begin() + (std::size_t(y + canvas.y) * canvas.width + canvas.x) * 4;
            std::copy_n(start, std::size_t(w) * 4, cropped.begin() + std::size_t(y) * w * 4);
        }
    }
    std::vector<unsigned char> alpha;
    auto pixels = preparePixels(crop ? cropped : rgba, w, h, cutoff, sprite, alpha);
    boundedAdd(publication_.textureBytes, pixels.size(), installedBudget);
    boundedAdd(publication_.alphaBytes, alpha.size(), installedBudget);
    Texture t{name,        w, h, mipLevels(w, h), sprite, offset_, pixels.size(), offset_ + pixels.size(),
              alpha.size()};
    t.material = material && (!sprite || cutoutMaterial);
    t.canvas = canvas;
    t.checksum = fingerprintBasis;
    t.alphaChecksum = fingerprintBasis;
    fingerprintBytes(t.checksum, pixels.data(), pixels.size());
    fingerprintBytes(t.alphaChecksum, alpha.data(), alpha.size());
    bytes(pixels);
    bytes(alpha);
    publication_.textures.push_back(t);
    publication_.anchors.push_back(anchor);
    return unsigned(publication_.textures.size() - 1);
}
void Writer::finish(const std::vector<Appearance>& appearances, std::uint64_t definitions,
                    const std::vector<Pack>& packs, const std::vector<PropMesh>& meshes) {
    for (const auto& mesh : meshes)
        require(!mesh.vertices.empty()); // Writer consumes authored payloads, not catalog descriptors.
    validMeshes(meshes, publication_.textures, packs);
    require(appearances.size() <= 32 && packs.size() <= packCapacity);
    std::set<std::string> ids;
    for (const auto& p : packs) {
        validPack(p, publication_.textures);
        require(ids.insert(p.id).second);
    }
    Json textures = Json::array(), actors = Json::array();
    for (unsigned i = 0; i < publication_.textures.size(); ++i) {
        const auto& t = publication_.textures[i];
        const auto a = publication_.anchors[i];
        textures.push_back({{"name", t.name},
                            {"width", t.width},
                            {"height", t.height},
                            {"canvas", {t.canvas.width, t.canvas.height, t.canvas.x, t.canvas.y}},
                            {"levels", t.levels},
                            {"clamp", t.clamp},
                            {"material", t.material},
                            {"offset", t.offset},
                            {"bytes", t.bytes},
                            {"alpha_offset", t.alphaOffset},
                            {"alpha_bytes", t.alphaBytes},
                            {"checksum", t.checksum},
                            {"alpha_checksum", t.alphaChecksum},
                            {"anchor", {a.x, a.y}}});
    }
    for (const auto& a : appearances)
        actors.push_back(appearanceJson(a));
    Json packJson = Json::array();
    for (const auto& p : packs) {
        packJson.push_back({{"id", p.id},
                            {"materials", std::vector<std::uint16_t>(p.materials.begin(),
                                                                     p.materials.begin() + p.materialCount)},
                            {"door", p.door},
                            {"lockedDoor", p.lockedDoor},
                            {"doorEdge", p.doorEdge},
                            {"crest", p.crest},
                            {"vine", p.vine},
                            {"sky", p.sky},
                            {"worldScale", p.worldScale},
                            {"viewDistance", p.viewDistance},
                            {"horizon", p.horizon},
                            {"daylightFog", p.daylightFog}});
        packJson.back()["decorations"] = p.decorations;
        packJson.back()["models"] = p.models;
        packJson.back()["buildings"] = Json::array();
        for (const auto& b : p.buildings)
            packJson.back()["buildings"].push_back({{"id", b.id}, {"parts", b.parts}});
        if (p.guardianEmblem != UINT16_MAX)
            packJson.back()["guardianEmblem"] = p.guardianEmblem;
    }
    Json meshJson = Json::array();
    for (const auto& m : meshes) {
        std::vector<unsigned char> payload;
        payload.reserve(m.vertices.size() * 32);
        static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);
        for (const auto& vertex : m.vertices)
            for (float f : vertex) {
                std::uint32_t bits;
                std::memcpy(&bits, &f, sizeof(bits));
                for (unsigned n = 0; n < 4; ++n)
                    payload.push_back(static_cast<unsigned char>(bits >> (8 * n)));
            }
        auto checksum = fingerprintBasis;
        fingerprintBytes(checksum, payload.data(), payload.size());
        meshJson.push_back({{"name", m.name},
                            {"architecture", m.architecture},
                            {"cutout", m.cutout},
                            {"water", m.water},
                            {"texture", m.texture},
                            {"count", m.vertices.size()},
                            {"offset", offset_},
                            {"checksum", checksum}});
        bytes(payload);
    }
    const Json manifest{
        {"meshes", meshJson},         {"format", version},     {"appearance_revision", appearanceRevision},
        {"definitions", definitions}, {"bytes", offset_},      {"checksum", hash_},
        {"textures", textures},       {"appearances", actors}, {"packs", packJson}};
    const auto text = manifest.dump(2);
    require(text.size() <= metadataBudget);
    out_.close();
    require(bool(out_));
    // Invalidate publication before replacing payload; interrupted/mixed output rejects.
    std::filesystem::remove(directory_ / "metadata.json");
    std::filesystem::rename(temporary_, directory_ / "textures.bin");
    const auto temp = directory_ / "metadata.json.tmp";
    {
        std::ofstream out(temp);
        out << text << '\n';
        out.close();
        require(bool(out));
    }
    std::filesystem::rename(temp, directory_ / "metadata.json");
    load(directory_).verify(); // Full offline verification; runtime opening is metadata-only.
}
Publication load(const std::filesystem::path& directory) {
    try {
        require(std::filesystem::file_size(directory / "metadata.json") <= metadataBudget);
        std::ifstream in(directory / "metadata.json");
        std::array<std::set<std::string>, 33> keys;
        auto j = Json::parse(in, [&](int depth, Json::parse_event_t event, Json& value) {
            require(depth <= 32);
            if (event == Json::parse_event_t::object_start)
                keys.at(depth).clear();
            if (event == Json::parse_event_t::key)
                require(keys.at(depth - 1).insert(value.get<std::string>()).second);
            return true;
        });
        require(integer(j.at("format")) == version &&
                integer(j.at("appearance_revision")) == appearanceRevision);
        Publication p;
        p.payload = directory / "textures.bin";
        p.payloadChecksum = integer(j.at("checksum"));
        p.definitions = integer(j.at("definitions"));
        const auto length = integer(j.at("bytes"));
        require(length <= installedBudget && std::filesystem::file_size(p.payload) == length);
        // Publication identity includes bindings/sampling as well as the checked payload checksum.
        const auto canonical = j.dump();
        p.fingerprint = fingerprintBasis;
        fingerprintBytes(p.fingerprint, canonical.data(), canonical.size());
        const auto& ts = j.at("textures");
        require(ts.is_array() && !ts.empty() && ts.size() <= catalogTextureCapacity);
        p.textures.reserve(ts.size());
        p.anchors.reserve(ts.size());
        std::uint64_t offset = 0;
        std::set<std::string> names;
        for (const auto& v : ts) {
            Texture t{v.at("name").get<std::string>(),
                      unsigned(integer(v.at("width"), UINT32_MAX)),
                      unsigned(integer(v.at("height"), UINT32_MAX)),
                      unsigned(integer(v.at("levels"), UINT32_MAX)),
                      v.at("clamp").get<bool>(),
                      integer(v.at("offset")),
                      integer(v.at("bytes")),
                      integer(v.at("alpha_offset")),
                      integer(v.at("alpha_bytes")),
                      integer(v.at("checksum")),
                      integer(v.at("alpha_checksum"))};
            require(!t.name.empty() && t.name.size() <= 256 && names.insert(t.name).second);
            const auto& canvas = v.at("canvas");
            require(canvas.is_array() && canvas.size() == 4);
            t.canvas = {unsigned(integer(canvas[0], 4096)), unsigned(integer(canvas[1], 4096)),
                        unsigned(integer(canvas[2], 4096)), unsigned(integer(canvas[3], 4096))};
            validCanvas(t);
            require(t.levels == mipLevels(t.width, t.height) && t.bytes == mipBytes(t.width, t.height) &&
                    t.offset == offset);
            boundedAdd(offset, t.bytes, length);
            require(t.alphaOffset == offset);
            require(t.alphaBytes == (t.clamp ? std::uint64_t(t.width) * t.height : 0));
            boundedAdd(offset, t.alphaBytes, length);
            boundedAdd(p.textureBytes, t.bytes, installedBudget);
            boundedAdd(p.alphaBytes, t.alphaBytes, installedBudget);
            t.material = v.at("material").get<bool>();
            const auto& a = v.at("anchor");
            require(a.is_array() && a.size() == 2);
            Anchor anchor{a[0].get<float>(), a[1].get<float>()};
            validAnchor(anchor);
            p.textures.push_back(t);
            p.anchors.push_back(anchor);
        }
        require(j.at("packs").is_array() && j.at("packs").size() <= packCapacity);
        std::set<std::string> packIds;
        for (const auto& v : j.at("packs")) {
            Pack pack;
            pack.id = v.at("id").get<std::string>();
            require(!pack.id.empty() && pack.id.size() <= 32 && packIds.insert(pack.id).second &&
                    std::all_of(pack.id.begin(), pack.id.end(), [](unsigned char c) {
                        return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
                    }));
            const auto ref = [&](const char* name, bool sprite) {
                const auto n = unsigned(integer(v.at(name), catalogTextureCapacity - 1));
                require(n < p.textures.size() && p.textures[n].clamp == sprite);
                return std::uint16_t(n);
            };
            const auto& palette = v.at("materials");
            require(palette.is_array() && palette.size() >= 3 && palette.size() <= 32);
            for (const auto& n : palette) {
                const auto i = unsigned(integer(n, catalogTextureCapacity - 1));
                require(i < p.textures.size() && p.textures[i].material);
                pack.materials[pack.materialCount++] = std::uint16_t(i);
            }
            pack.door = ref("door", false);
            pack.lockedDoor = ref("lockedDoor", false);
            pack.doorEdge = ref("doorEdge", false);
            pack.crest = ref("crest", true);
            pack.vine = ref("vine", true);
            pack.sky = ref("sky", false);
            if (v.contains("guardianEmblem"))
                pack.guardianEmblem = ref("guardianEmblem", true);
            require(v.at("decorations").size() == decorationKinds && v.at("models").size() == 4);
            for (unsigned i = 0; i < decorationKinds; ++i)
                pack.decorations[i] = std::uint16_t(integer(v.at("decorations")[i], UINT16_MAX));
            for (unsigned i = 0; i < 4; ++i)
                pack.models[i] = std::uint16_t(integer(v.at("models")[i], UINT16_MAX));
            require(v.at("buildings").is_array() && v.at("buildings").size() <= staticModelCapacity);
            for (const auto& b : v.at("buildings")) {
                StaticModel model;
                model.id = b.at("id").get<std::string>();
                require(b.at("parts").is_array() && b.at("parts").size() <= staticPartCapacity);
                for (const auto& part : b.at("parts"))
                    model.parts.push_back(std::uint16_t(integer(part, meshCapacity - 1)));
                pack.buildings.push_back(std::move(model));
            }
            pack.viewDistance = v.at("viewDistance").get<float>();
            pack.worldScale = v.at("worldScale").get<float>();
            require(std::isfinite(pack.worldScale) && pack.worldScale > 0 && pack.worldScale <= 16);
            require(v.at("horizon").size() == 3);
            for (unsigned i = 0; i < 3; ++i) {
                pack.horizon[i] = v.at("horizon")[i].get<float>();
                require(std::isfinite(pack.horizon[i]) && pack.horizon[i] >= 0 && pack.horizon[i] <= 1);
            }
            pack.daylightFog = v.at("daylightFog").get<std::array<float, 3>>();
            validPack(pack, p.textures);
            p.packs.push_back(std::move(pack));
        }
        require(j.at("meshes").is_array() && j.at("meshes").size() <= meshCapacity);
        for (const auto& v : j.at("meshes")) {
            PropMesh m;
            m.name = v.at("name").get<std::string>();
            require(v.at("architecture").is_boolean());
            m.architecture = v.at("architecture").get<bool>();
            require(v.at("cutout").is_boolean());
            m.cutout = v.at("cutout").get<bool>();
            require(v.at("water").is_boolean());
            m.water = v.at("water").get<bool>();
            m.texture = std::uint16_t(integer(v.at("texture"), catalogTextureCapacity - 1));
            m.vertexCount = unsigned(integer(v.at("count"), 6000));
            m.offset = integer(v.at("offset"));
            m.checksum = integer(v.at("checksum"));
            require(m.offset == offset);
            boundedAdd(offset, std::uint64_t(m.vertexCount) * 32, length);
            p.meshes.push_back(std::move(m));
        }
        require(offset == length);
        validMeshes(p.meshes, p.textures, p.packs);
        const auto& actors = j.at("appearances");
        require(actors.is_array() && actors.size() <= 32);
        names.clear();
        for (const auto& v : actors) {
            Appearance a;
            a.id = v.at("id").get<std::string>();
            a.width = v.at("width");
            a.height = v.at("height");
            a.cutoff = unsigned(integer(v.at("cutoff"), 254));
            require(!a.id.empty() && a.id.size() <= 128 && names.insert(a.id).second &&
                    std::isfinite(a.width) && std::isfinite(a.height) && a.width > 0 && a.width <= .5f &&
                    a.height > 0 && a.height <= 1 && a.cutoff < 255);
            const auto& attachments = v.at("attachments");
            require(attachments.size() == 4);
            for (unsigned s = 0; s < 4; ++s) {
                require(attachments[s].size() == 4);
                for (const auto& n : attachments[s])
                    require(n.is_number() && std::isfinite(n.get<float>()) && n >= 0 && n <= 1);
                a.attachments.release[s] = {attachments[s][0], attachments[s][1]};
                a.attachments.torso[s] = {attachments[s][2], attachments[s][3]};
            }
            const auto& clips = v.at("clips");
            require(clips.size() == npcAnimationCount);
            NpcTimings timings{};
            for (unsigned c = 0; c < npcAnimationCount; ++c) {
                auto& clip = a.clips[c];
                const auto& vclip = clips[c];
                const unsigned count = unsigned(integer(vclip.at("count"), 4));
                require(count <= 4);
                clip.timing.count = std::uint8_t(count);
                clip.width = vclip.at("width");
                require(std::isfinite(clip.width) && clip.width >= 0 && clip.width <= 1);
                require(vclip.at("durations").size() == 4);
                for (unsigned f = 0; f < 4; ++f)
                    clip.timing.durationMs[f] = std::uint32_t(integer(vclip.at("durations")[f], 60000));
                const auto& refs = vclip.at("textures");
                require(refs.size() == 4);
                for (unsigned s = 0; s < 4; ++s) {
                    require(refs[s].size() == 4);
                    for (unsigned f = 0; f < 4; ++f) {
                        const auto id = unsigned(integer(refs[s][f], UINT16_MAX));
                        require(id == UINT16_MAX ||
                                (f < count && id < p.textures.size() && p.textures[id].clamp));
                        clip.textures[s][f] = std::uint16_t(id);
                    }
                }
                for (unsigned f = 0; f < count; ++f)
                    require(clip.timing.durationMs[f] > 0 && clip.timing.durationMs[f] <= 60000 &&
                            clip.textures[0][f] != UINT16_MAX);
                for (unsigned side = 0; side < 4; ++side) {
                    const bool optional = c == unsigned(NpcAnimation::Attack) && side != 0;
                    for (unsigned frame = 0; frame < count; ++frame)
                        require(optional || clip.textures[side][frame] != UINT16_MAX);
                }
                timings[c] = clip.timing;
            }
            validateNpcTimings(timings);
            p.appearances.push_back(std::move(a));
        }
        return p;
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Cannot load prepared world assets; rebuild blobber_bake_world_assets: ") + e.what());
    }
}
std::vector<unsigned char> Publication::read(unsigned index, bool alpha) const {
    if (index >= textures.size())
        throw std::out_of_range("World texture index");
    const auto& t = textures[index];
    const auto bytes = alpha ? t.alphaBytes : t.bytes;
    require(bytes <= 128 * MiB);
    std::vector<unsigned char> data(static_cast<std::size_t>(bytes));
    std::ifstream in(payload, std::ios::binary);
    in.seekg(std::streamoff(alpha ? t.alphaOffset : t.offset));
    in.read(reinterpret_cast<char*>(data.data()), std::streamsize(data.size()));
    require(bool(in));
    auto hash = fingerprintBasis;
    fingerprintBytes(hash, data.data(), data.size());
    require(hash == (alpha ? t.alphaChecksum : t.checksum));
    return data;
}
PropMesh Publication::readMesh(unsigned index) const {
    if (index >= meshes.size())
        throw std::out_of_range("World mesh index");
    auto mesh = meshes[index];
    if (mesh.vertices.empty()) {
        require(mesh.vertexCount && mesh.vertexCount <= 6000 && mesh.vertexCount % 3 == 0);
        std::vector<unsigned char> data(std::size_t(mesh.vertexCount) * 32);
        std::ifstream in(payload, std::ios::binary);
        in.seekg(std::streamoff(mesh.offset));
        in.read(reinterpret_cast<char*>(data.data()), std::streamsize(data.size()));
        require(bool(in));
        auto checksum = fingerprintBasis;
        fingerprintBytes(checksum, data.data(), data.size());
        require(checksum == mesh.checksum);
        mesh.vertices.resize(mesh.vertexCount);
        std::size_t at = 0;
        for (auto& vertex : mesh.vertices)
            for (float& f : vertex) {
                std::uint32_t bits = 0;
                for (unsigned n = 0; n < 4; ++n)
                    bits |= std::uint32_t(data[at++]) << (8 * n);
                std::memcpy(&f, &bits, sizeof(f));
            }
    }
    validMeshes({mesh}, textures, {});
    for (const auto& pack : packs)
        for (unsigned slot = 0; slot < pack.models.size(); ++slot)
            if (pack.models[slot] == index)
                for (const auto& v : mesh.vertices)
                    require(std::abs(v[0]) <= .5f && std::abs(v[2]) <= (slot == 3 ? 1.f : .5f));
    return mesh;
}
void Publication::verify() const {
    require(fingerprintFile(payload) == std::to_string(payloadChecksum));
    for (unsigned i = 0; i < textures.size(); ++i) {
        (void)read(i);
        if (textures[i].alphaBytes)
            (void)read(i, true);
    }
    for (unsigned i = 0; i < meshes.size(); ++i)
        (void)readMesh(i);
}

} // namespace blobber::world_assets

namespace blobber::world_assets {
CpuAssets::CpuAssets(Publication p) : publication(std::move(p)) {
    require(publication.textures.size() <= catalogTextureCapacity && publication.appearances.size() <= 32 &&
            publication.anchors.size() == publication.textures.size() &&
            publication.alphaBytes <= installedBudget && publication.textureBytes <= installedBudget);
    require(publication.packs.size() <= packCapacity);
    validMeshes(publication.meshes, publication.textures, publication.packs);
    std::set<std::string> ids;
    for (const auto& p : publication.packs) {
        validPack(p, publication.textures);
        require(ids.insert(p.id).second);
    }
    std::uint64_t totalAlpha = 0, totalColor = 0;
    for (const auto& t : publication.textures) {
        validCanvas(t);
        require(t.width && t.height && t.width <= 4096 && t.height <= 4096 &&
                t.alphaBytes == (t.clamp ? std::uint64_t(t.width) * t.height : 0));
        require(t.levels == mipLevels(t.width, t.height) && t.bytes == mipBytes(t.width, t.height));
        boundedAdd(totalAlpha, t.alphaBytes, installedBudget);
        boundedAdd(totalColor, t.bytes, installedBudget);
    }
    require(totalAlpha == publication.alphaBytes && totalColor == publication.textureBytes);
    for (const auto& anchor : publication.anchors)
        validAnchor(anchor);
    for (const auto& appearance : publication.appearances) {
        require(std::isfinite(appearance.width) && std::isfinite(appearance.height) && appearance.width > 0 &&
                appearance.width <= .5f && appearance.height > 0 && appearance.height <= 1);
        NpcTimings timings{};
        for (unsigned c = 0; c < npcAnimationCount; ++c) {
            const auto& clip = appearance.clips[c];
            timings[c] = clip.timing;
            require(std::isfinite(clip.width) && clip.width >= 0 && clip.width <= 1);
            for (const auto& side : clip.textures)
                for (unsigned f = 0; f < side.size(); ++f)
                    require(side[f] == UINT16_MAX ||
                            (f < clip.timing.count && side[f] < publication.textures.size() &&
                             publication.textures[side[f]].clamp));
        }
        validateNpcTimings(timings);
        for (unsigned c = 0; c < npcAnimationCount; ++c)
            for (unsigned side = 0; side < 4; ++side)
                if (c != unsigned(NpcAnimation::Attack) || side == 0)
                    for (unsigned f = 0; f < timings[c].count; ++f)
                        require(appearance.clips[c].textures[side][f] != UINT16_MAX);
    }
    masks.resize(publication.textures.size());
    bounds.resize(publication.textures.size());
    textureRefs_.resize(publication.textures.size());
    meshRefs_.resize(publication.meshes.size());
    meshes_.resize(publication.meshes.size());
    for (unsigned i = 0; i < publication.appearances.size(); ++i) {
        const auto& a = publication.appearances[i];
        for (const auto& c : a.clips)
            for (const auto& side : c.textures)
                for (auto t : side)
                    if (t != UINT16_MAX) {
                        const auto anchor = publication.anchors[t];
                        edgeHalfWidths[i] = std::max(edgeHalfWidths[i], (c.width ? c.width : a.width) *
                                                                            std::max(anchor.x, 1 - anchor.x));
                    }
    }
    if (bytes() > worldCpuBudget)
        throw std::runtime_error("World shared CPU storage budget exceeded");
}
unsigned CpuAssets::named(const char* name) const {
    for (unsigned i = 0; i < publication.textures.size(); ++i)
        if (publication.textures[i].name == name)
            return i;
    throw std::invalid_argument(std::string("Missing world sprite: ") + name);
}
Pack CpuAssets::pack(const std::string& id) const {
    for (const auto& p : publication.packs)
        if (p.id == id)
            return p;
    // Small standalone generated publications used by CPU/GPU fixtures retain a local palette.
    if (!publication.packs.empty() || id != "dungeon")
        throw std::invalid_argument("Unavailable world asset pack: " + id);
    Pack p;
    for (unsigned i = 0; i < publication.textures.size(); ++i)
        if (publication.textures[i].material && !publication.textures[i].clamp) {
            require(p.materialCount < p.materials.size());
            p.materials[p.materialCount++] = std::uint16_t(i);
        }
    auto optional = [&](const char* name) {
        for (unsigned i = 0; i < publication.textures.size(); ++i)
            if (publication.textures[i].name == name)
                return std::uint16_t(i);
        return std::uint16_t(0);
    };
    p.door = optional("door");
    p.lockedDoor = optional("door-locked");
    p.doorEdge = p.door;
    p.crest = optional("decoration/crest");
    p.vine = optional("decoration/vine");
    p.sky = optional("sky");
    return p;
}
StaticModelId Pack::building(const std::string& name) const {
    for (unsigned i = 0; i < buildings.size(); ++i)
        if (buildings[i].id == name)
            return {std::uint16_t(i)};
    throw std::invalid_argument("Unavailable building model: " + id + "/" + name);
}
std::size_t CpuAssets::bytes() const noexcept {
    std::size_t meshBytes = publication.meshes.capacity() * sizeof(PropMesh);
    for (const auto& m : publication.meshes)
        meshBytes += m.name.capacity() + m.vertices.capacity() * sizeof(std::array<float, 8>);
    std::size_t n = sizeof(*this) + meshBytes + publication.packs.capacity() * sizeof(Pack) +
                    masks.capacity() * sizeof(masks[0]) + bounds.capacity() * sizeof(AlphaBounds) +
                    publication.textures.capacity() * sizeof(Texture) +
                    publication.anchors.capacity() * sizeof(Anchor) +
                    publication.appearances.capacity() * sizeof(Appearance);
    for (const auto& m : masks)
        n += m.capacity();
    for (const auto& t : publication.textures)
        n += t.name.capacity();
    for (const auto& a : publication.appearances)
        n += a.id.capacity();
    for (const auto& p : publication.packs) {
        n += p.dynamicBytes();
    }
    n += textureRefs_.capacity() * sizeof(unsigned) + meshRefs_.capacity() * sizeof(unsigned) +
         meshes_.capacity() * sizeof(PropMesh);
    for (const auto& m : meshes_)
        n += m.name.capacity() + m.vertices.capacity() * sizeof(std::array<float, 8>);
    return n;
}
} // namespace blobber::world_assets
