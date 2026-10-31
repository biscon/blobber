#include "assets/world_assets.hpp"
#include "assets/fingerprint.hpp"
#include <algorithm>
#include <cmath>
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
    out_.write(reinterpret_cast<const char*>(b.data()), std::streamsize(b.size()));
    require(bool(out_));
    fingerprintBytes(hash_, b.data(), b.size());
    offset_ += b.size();
}
unsigned Writer::add(const std::string& name, unsigned w, unsigned h, bool sprite,
                     const std::vector<unsigned char>& rgba, unsigned cutoff, Anchor anchor) {
    require(publication_.textures.size() < textureCapacity && !name.empty() && name.size() <= 256);
    for (const auto& t : publication_.textures)
        require(t.name != name);
    validAnchor(anchor);
    unsigned sprites = 0, materials = 0;
    for (const auto& t : publication_.textures)
        t.clamp ? ++sprites : ++materials;
    require(sprite ? sprites < 448 : materials < 32);
    std::vector<unsigned char> alpha;
    auto pixels = preparePixels(rgba, w, h, cutoff, sprite, alpha);
    boundedAdd(publication_.textureBytes, pixels.size(), textureBudget);
    boundedAdd(publication_.alphaBytes, alpha.size(), alphaBudget);
    Texture t{name,        w, h, mipLevels(w, h), sprite, offset_, pixels.size(), offset_ + pixels.size(),
              alpha.size()};
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
void Writer::finish(const std::vector<Appearance>& appearances, std::uint64_t definitions) {
    require(appearances.size() <= 32);
    Json textures = Json::array(), actors = Json::array();
    for (unsigned i = 0; i < publication_.textures.size(); ++i) {
        const auto& t = publication_.textures[i];
        const auto a = publication_.anchors[i];
        textures.push_back({{"name", t.name},
                            {"width", t.width},
                            {"height", t.height},
                            {"levels", t.levels},
                            {"clamp", t.clamp},
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
    const Json manifest{{"format", version},    {"appearance_revision", 1}, {"definitions", definitions},
                        {"bytes", offset_},     {"checksum", hash_},        {"textures", textures},
                        {"appearances", actors}};
    const auto text = manifest.dump(2);
    require(text.size() <= 256 * 1024);
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
    (void)load(directory_); // Validate writer and loader contract before reporting success.
}
Publication load(const std::filesystem::path& directory) {
    try {
        require(std::filesystem::file_size(directory / "metadata.json") <= 256 * 1024);
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
        require(integer(j.at("format")) == version && integer(j.at("appearance_revision")) == 1);
        Publication p;
        p.payload = directory / "textures.bin";
        p.fingerprint = integer(j.at("checksum"));
        p.definitions = integer(j.at("definitions"));
        const auto length = integer(j.at("bytes"));
        require(length <= textureBudget + alphaBudget && std::filesystem::file_size(p.payload) == length);
        require(fingerprintFile(p.payload) == std::to_string(p.fingerprint));
        const auto& ts = j.at("textures");
        require(ts.is_array() && !ts.empty() && ts.size() <= textureCapacity);
        p.textures.reserve(ts.size());
        p.anchors.reserve(ts.size());
        std::uint64_t offset = 0;
        unsigned sprites = 0, materials = 0;
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
            require(t.levels == mipLevels(t.width, t.height) && t.bytes == mipBytes(t.width, t.height) &&
                    t.offset == offset);
            boundedAdd(offset, t.bytes, length);
            require(t.alphaOffset == offset);
            require(t.alphaBytes == (t.clamp ? std::uint64_t(t.width) * t.height : 0));
            boundedAdd(offset, t.alphaBytes, length);
            boundedAdd(p.textureBytes, t.bytes, textureBudget);
            boundedAdd(p.alphaBytes, t.alphaBytes, alphaBudget);
            t.clamp ? ++sprites : ++materials;
            require(sprites <= 448 && materials <= 32);
            const auto& a = v.at("anchor");
            require(a.is_array() && a.size() == 2);
            Anchor anchor{a[0].get<float>(), a[1].get<float>()};
            validAnchor(anchor);
            p.textures.push_back(t);
            p.anchors.push_back(anchor);
        }
        require(offset == length);
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
} // namespace blobber::world_assets

namespace blobber::world_assets {
CpuAssets::CpuAssets(Publication p) : publication(std::move(p)) {
    require(publication.textures.size() <= textureCapacity && publication.appearances.size() <= 32 &&
            publication.anchors.size() == publication.textures.size() &&
            publication.alphaBytes <= alphaBudget && publication.textureBytes <= textureBudget);
    std::uint64_t totalAlpha = 0;
    for (const auto& t : publication.textures) {
        require(t.width && t.height && t.width <= 4096 && t.height <= 4096 &&
                t.alphaBytes == (t.clamp ? std::uint64_t(t.width) * t.height : 0));
        boundedAdd(totalAlpha, t.alphaBytes, alphaBudget);
    }
    require(totalAlpha == publication.alphaBytes);
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
    masks.reserve(publication.textures.size());
    bounds.reserve(publication.textures.size());
    for (unsigned i = 0; i < publication.textures.size(); ++i) {
        const auto& t = publication.textures[i];
        masks.push_back(publication.read(i, true));
        AlphaBounds b{t.width, t.height, 0, 0};
        if (t.clamp)
            for (unsigned y = 0; y < t.height; ++y)
                for (unsigned x = 0; x < t.width; ++x)
                    if (masks.back()[std::size_t(y) * t.width + x]) {
                        b.left = std::min(b.left, x);
                        b.top = std::min(b.top, y);
                        b.right = std::max(b.right, x + 1);
                        b.bottom = std::max(b.bottom, y + 1);
                    }
        bounds.push_back(b);
    }
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
    if (bytes() > 68 * MiB)
        throw std::runtime_error("World shared CPU storage budget exceeded");
}
unsigned CpuAssets::named(const char* name) const {
    for (unsigned i = 0; i < publication.textures.size(); ++i)
        if (publication.textures[i].name == name)
            return i;
    throw std::invalid_argument(std::string("Missing world sprite: ") + name);
}
std::size_t CpuAssets::bytes() const noexcept {
    std::size_t n =
        sizeof(*this) + masks.capacity() * sizeof(masks[0]) + bounds.capacity() * sizeof(AlphaBounds) +
        publication.textures.capacity() * sizeof(Texture) + publication.anchors.capacity() * sizeof(Anchor) +
        publication.appearances.capacity() * sizeof(Appearance);
    for (const auto& m : masks)
        n += m.capacity();
    for (const auto& t : publication.textures)
        n += t.name.capacity();
    for (const auto& a : publication.appearances)
        n += a.id.capacity();
    return n;
}
} // namespace blobber::world_assets
