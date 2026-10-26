#include "assets/prepared_assets.hpp"
#include "assets/prepared_binary.hpp"
#include <algorithm>
#include <cmath>

namespace blobber {
using prepared_io::Reader;
using prepared_io::Writer;
class PreparedNpcCodec {
    static constexpr std::size_t maxFrames = NpcWorld::maxTypes * npcAnimationCount * 4 * npcMaxFrames;
    static void validate(const PreparedNpcs& n, const std::vector<NpcTimings>& timings) {
        Reader::require(n.types_.size() == timings.size() && n.shadows_.size() == timings.size());
        const int maxWidth =
            int(std::ceil(ViewProjection::focalLength() / (ViewProjection::cameraSetback - .25f))) + 1;
        const auto rectangle = [&](const auto& s, int width, int height) {
            Reader::require(s.cropX >= 0 && s.cropY >= 0 && s.cropX <= width && s.cropY <= height &&
                            s.cropWidth >= 0 && s.cropHeight >= 0 && s.cropWidth <= width - s.cropX &&
                            s.cropHeight <= height - s.cropY && (s.cropWidth == 0) == (s.cropHeight == 0));
            const auto count = static_cast<std::size_t>(s.cropWidth) * s.cropHeight;
            Reader::require(s.first <= n.pixels_.size() && count <= n.pixels_.size() - s.first);
        };
        for (const auto& s : n.sprites_) {
            Reader::require(s.width > 0 && s.width <= maxWidth && s.top >= 0 && s.top <= viewHeight &&
                            s.rows >= 0 && s.rows <= viewHeight - s.top && s.pivotX >= 0 &&
                            s.pivotX <= s.width);
            rectangle(s, s.width, s.rows);
        }
        for (const auto& s : n.travelSprites_) {
            Reader::require(s.width > 0 && s.width <= 256 && s.height > 0 && s.height <= 256 &&
                            s.worldWidth > 0 && s.worldWidth <= 1 && s.worldHeight > 0 &&
                            s.worldHeight <= ViewProjection::wallHeight && s.anchorX >= 0 && s.anchorX <= 1 &&
                            s.anchorY > 0 && s.anchorY <= 1);
            rectangle(s, s.width, s.height);
        }
        for (std::size_t i = 0; i < n.types_.size(); ++i) {
            validateNpcTimings(timings[i]);
            const auto& t = n.types_[i];
            Reader::require(t.shadowRadius > 0 && t.shadowRadius <= .2251f && t.edgeHalfWidth > 0 &&
                            t.edgeHalfWidth <= 1);
            for (std::size_t c = 0; c < npcAnimationCount; ++c)
                for (int side = 0; side < 4; ++side) {
                    const auto count = t.count[c][side];
                    Reader::require(
                        count == timings[i][c].count ||
                        (c == static_cast<std::size_t>(NpcAnimation::Attack) && side != 0 && count == 0));
                    const auto first = t.first[c][side], travel = t.travelFirst[c][side];
                    Reader::require(first <= n.sprites_.size() &&
                                    count * PreparedNpcs::depthCount <= n.sprites_.size() - first);
                    Reader::require(travel <= n.travelSprites_.size() &&
                                    count <= n.travelSprites_.size() - travel);
                }
            for (const auto& shadow : n.shadows_[i])
                Reader::require(shadow.firstRow <= n.shadowRows_.size() &&
                                shadow.rowCount <= n.shadowRows_.size() - shadow.firstRow);
        }
        for (const auto& row : n.shadowRows_) {
            Reader::require(row.y >= 0 && row.y < viewHeight && row.width > 0 && row.width <= viewWidth * 2 &&
                            row.depth > 0 && row.depth <= ViewProjection::visibleRows + 2 &&
                            row.first <= n.shadowAlpha_.size() &&
                            static_cast<std::size_t>(row.width) <= n.shadowAlpha_.size() - row.first);
        }
    }
    template <class S> static void writeRectangle(Writer& out, const S& s) {
        out.u64(s.first);
        out.i32(s.cropX);
        out.i32(s.cropY);
        out.i32(s.cropWidth);
        out.i32(s.cropHeight);
    }
    template <class S> static void readRectangle(Reader& in, S& s) {
        s.first = in.u64();
        s.cropX = in.i32();
        s.cropY = in.i32();
        s.cropWidth = in.i32();
        s.cropHeight = in.i32();
    }

  public:
    static void save(const std::filesystem::path& path, const PreparedNpcs& n,
                     const std::vector<NpcTimings>& timings, std::uint64_t definitions,
                     std::uint64_t profile) {
        validate(n, timings);
        Writer out(path, "BLBNPC01", profile, definitions);
        out.u64(n.sourceBytes_);
        out.u64(n.loadingHighWater_);
        out.u64(n.types_.size());
        for (std::size_t i = 0; i < n.types_.size(); ++i) {
            const auto& t = n.types_[i];
            out.f32(t.shadowRadius);
            out.f32(t.edgeHalfWidth);
            for (std::size_t c = 0; c < npcAnimationCount; ++c) {
                out.u32(timings[i][c].count);
                for (auto duration : timings[i][c].durationMs)
                    out.u32(duration);
                for (int side = 0; side < 4; ++side) {
                    out.u64(t.first[c][side]);
                    out.u64(t.travelFirst[c][side]);
                    out.u32(t.count[c][side]);
                }
            }
        }
        out.u64(n.sprites_.size());
        for (const auto& s : n.sprites_) {
            out.i32(s.width);
            out.i32(s.top);
            out.i32(s.rows);
            out.f32(s.pivotX);
            writeRectangle(out, s);
        }
        out.u64(n.travelSprites_.size());
        for (const auto& s : n.travelSprites_) {
            out.i32(s.width);
            out.i32(s.height);
            out.f32(s.worldWidth);
            out.f32(s.worldHeight);
            out.f32(s.anchorX);
            out.f32(s.anchorY);
            writeRectangle(out, s);
        }
        out.u64(n.pixels_.size());
        out.bytes(n.pixels_.data(), n.pixels_.size() * sizeof(Color));
        out.u64(n.shadows_.size());
        for (const auto& type : n.shadows_)
            for (const auto& s : type) {
                out.u64(s.firstRow);
                out.u64(s.rowCount);
            }
        out.u64(n.shadowRows_.size());
        for (const auto& row : n.shadowRows_) {
            out.i32(row.y);
            out.i32(row.width);
            out.f32(row.depth);
            out.u64(row.first);
        }
        out.u64(n.shadowAlpha_.size());
        out.bytes(n.shadowAlpha_.data(), n.shadowAlpha_.size());
        out.bytes(n.travelShadow_.data(), n.travelShadow_.size());
        out.finish();
    }
    static PreparedNpcs load(const std::filesystem::path& path, const std::vector<NpcTimings>& timings,
                             std::uint64_t definitions, std::uint64_t profile, PreparationLimits limits) {
        Reader in(path, "BLBNPC01", profile, definitions, limits);
        in.claim(sizeof(PreparedNpcs));
        PreparedNpcs n(PreparedNpcs::LoadedTag{});
        n.sourceBytes_ = in.u64();
        n.loadingHighWater_ = in.u64();
        Reader::require(n.sourceBytes_ <= 512u * 1024u * 1024u &&
                        n.loadingHighWater_ <= 512u * 1024u * 1024u);
        n.types_.resize(
            in.count(NpcWorld::maxTypes, sizeof(PreparedNpcs::Type), 8 + npcAnimationCount * 100));
        Reader::require(n.types_.size() == timings.size());
        for (std::size_t i = 0; i < n.types_.size(); ++i) {
            auto& t = n.types_[i];
            t.shadowRadius = in.f32();
            t.edgeHalfWidth = in.f32();
            for (std::size_t c = 0; c < npcAnimationCount; ++c) {
                Reader::require(in.u32() == timings[i][c].count);
                for (auto duration : timings[i][c].durationMs)
                    Reader::require(in.u32() == duration);
                for (int side = 0; side < 4; ++side) {
                    t.first[c][side] = in.u64();
                    t.travelFirst[c][side] = in.u64();
                    const auto count = in.u32();
                    Reader::require(count <= npcMaxFrames);
                    t.count[c][side] = static_cast<std::uint8_t>(count);
                }
            }
        }
        n.sprites_.resize(in.count(maxFrames * PreparedNpcs::depthCount, sizeof(PreparedNpcs::Sprite), 40));
        for (auto& s : n.sprites_) {
            s.width = in.i32();
            s.top = in.i32();
            s.rows = in.i32();
            s.pivotX = in.f32();
            readRectangle(in, s);
        }
        n.travelSprites_.resize(in.count(maxFrames, sizeof(PreparedNpcs::TravelSprite), 48));
        for (auto& s : n.travelSprites_) {
            s.width = in.i32();
            s.height = in.i32();
            s.worldWidth = in.f32();
            s.worldHeight = in.f32();
            s.anchorX = in.f32();
            s.anchorY = in.f32();
            readRectangle(in, s);
        }
        n.pixels_.resize(in.count(npcPixelBudget, sizeof(Color), 4));
        in.bytes(n.pixels_.data(), n.pixels_.size() * sizeof(Color));
        n.shadows_.resize(in.count(NpcWorld::maxTypes,
                                   sizeof(std::array<PreparedNpcs::Shadow, PreparedNpcs::depthCount>),
                                   PreparedNpcs::depthCount * 16));
        for (auto& type : n.shadows_)
            for (auto& s : type) {
                s.firstRow = in.u64();
                s.rowCount = in.u64();
            }
        n.shadowRows_.resize(in.count(NpcWorld::maxTypes * PreparedNpcs::depthCount * viewHeight,
                                      sizeof(PreparedNpcs::ShadowRow), 20));
        for (auto& row : n.shadowRows_) {
            row.y = in.i32();
            row.width = in.i32();
            row.depth = in.f32();
            row.first = in.u64();
        }
        n.shadowAlpha_.resize(in.count(512u * 1024u * 1024u, 1, 1));
        in.bytes(n.shadowAlpha_.data(), n.shadowAlpha_.size());
        in.bytes(n.travelShadow_.data(), n.travelShadow_.size());
        in.finish();
        validate(n, timings);
        requirePreparationBytes(0, n.storageBytes(), std::min(limits.residentBytes, limits.loadingBytes));
        return n;
    }
};
void savePreparedNpcs(const std::filesystem::path& path, const PreparedNpcs& n,
                      const std::vector<NpcTimings>& timings, std::uint64_t definitions,
                      std::uint64_t profile) {
    PreparedNpcCodec::save(path, n, timings, definitions, profile);
}
PreparedNpcs loadPreparedNpcs(const std::filesystem::path& path, const std::vector<NpcTimings>& timings,
                              std::uint64_t definitions, std::uint64_t profile, PreparationLimits limits) {
    return PreparedNpcCodec::load(path, timings, definitions, profile, limits);
}
} // namespace blobber
