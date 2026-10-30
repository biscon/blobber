#include "assets/prepared_assets.hpp"
#include "assets/prepared_binary.hpp"
#include <algorithm>

namespace blobber {
namespace {
using prepared_io::Reader;
using prepared_io::Writer;
static_assert(sizeof(Color) == 4 && offsetof(Color, r) == 0 && offsetof(Color, g) == 1 &&
                  offsetof(Color, b) == 2 && offsetof(Color, a) == 3,
              "Bundle colors require explicit RGBA8 layout");
constexpr std::size_t backgroundBytes = ViewProjection::strafeSteps * 24u * Frame::pixelCount * sizeof(Color);
void validate(const PreparedScene& scene) {
    for (const auto& view : scene) {
        for (const auto& sprite : view.sprites) {
            Reader::require(sprite.distance >= 0 && sprite.distance <= ViewProjection::visibleRows &&
                            sprite.lateral >= -64 && sprite.lateral <= 64 && sprite.sideNeighbor >= -1 &&
                            sprite.sideNeighbor <= 1 && sprite.firstColumn <= view.columns.size() &&
                            sprite.columnCount <= view.columns.size() - sprite.firstColumn);
        }
        for (const auto& column : view.columns) {
            Reader::require(column.x >= 0 && column.x < viewWidth && column.top >= 0 &&
                            column.top <= viewHeight && column.depth > 0 &&
                            column.depth <= ViewProjection::visibleRows + 2 &&
                            column.length <= static_cast<unsigned>(viewHeight - column.top));
            for (const auto& doors : view.doors)
                Reader::require(column.doorOffset <= doors.size() &&
                                column.length <= doors.size() - column.doorOffset);
            for (const auto first : column.pixelOffsets)
                Reader::require(first <= view.pixels.size() && column.length <= view.pixels.size() - first);
        }
    }
}
} // namespace
void savePreparedScene(const std::filesystem::path& path, const PreparedScene& scene, std::uint64_t profile) {
    validate(scene);
    Writer out(path, "BLBSCN01", profile, 0);
    for (const auto& view : scene) {
        out.u64(view.sprites.size());
        for (const auto& s : view.sprites) {
            out.i32(s.lateral);
            out.i32(s.distance);
            out.i32(s.sideNeighbor);
            out.u32(s.firstColumn);
            out.u32(s.columnCount);
        }
        out.u64(view.columns.size());
        for (const auto& c : view.columns) {
            out.i32(c.x);
            out.i32(c.top);
            out.f32(c.depth);
            out.u32(c.length);
            for (const auto first : c.pixelOffsets)
                out.u32(first);
            out.u32(c.doorOffset);
        }
        out.u64(view.pixels.size());
        out.bytes(view.pixels.data(), view.pixels.size() * sizeof(Color));
        for (const auto& doors : view.doors) {
            out.u64(doors.size());
            out.bytes(doors.data(), doors.size() * sizeof(Color));
        }
        for (const auto& background : view.backgrounds)
            out.bytes(background.data(), Frame::pixelCount * sizeof(Color));
        for (const auto& grease : view.grease)
            out.bytes(grease.data(), Frame::pixelCount * sizeof(Color));
        for (const auto& web : view.web)
            out.bytes(web.data(), Frame::pixelCount * sizeof(Color));
        Reader::require(view.weapon.size() == PreparedView::weaponPixelCount);
        out.bytes(view.weapon.data(), view.weapon.size() * sizeof(Color));
    }
    out.finish();
}
PreparedScene loadPreparedScene(const std::filesystem::path& path, std::uint64_t profile,
                                PreparationLimits limits) {
    Reader in(path, "BLBSCN01", profile, 0, limits);
    in.claim(sizeof(PreparedScene) + backgroundBytes); // Before Frame constructors allocate.
    PreparedScene scene;
    for (auto& view : scene) {
        view.sprites.resize(in.count(PreparedView::maxSprites, sizeof(PreparedSprite), 20));
        for (auto& s : view.sprites) {
            s.lateral = in.i32();
            s.distance = in.i32();
            s.sideNeighbor = in.i32();
            s.firstColumn = in.u32();
            s.columnCount = in.u32();
        }
        view.columns.resize(in.count(PreparedView::maxColumns, sizeof(PreparedColumn), 28));
        for (auto& c : view.columns) {
            c.x = in.i32();
            c.top = in.i32();
            c.depth = in.f32();
            c.length = in.u32();
            for (auto& first : c.pixelOffsets)
                first = in.u32();
            c.doorOffset = in.u32();
        }
        view.pixels.resize(in.count(PreparedView::maxPixels, sizeof(Color), 4));
        in.bytes(view.pixels.data(), view.pixels.size() * sizeof(Color));
        for (auto& doors : view.doors) {
            doors.resize(in.count(PreparedView::maxPixels / 2, sizeof(Color), 4));
            in.bytes(doors.data(), doors.size() * sizeof(Color));
        }
        for (auto& background : view.backgrounds)
            in.bytes(background.data(), Frame::pixelCount * sizeof(Color));
        for (auto& grease : view.grease)
            in.bytes(grease.data(), Frame::pixelCount * sizeof(Color));
        for (auto& web : view.web)
            in.bytes(web.data(), Frame::pixelCount * sizeof(Color));
        in.claim(PreparedView::weaponPixelCount * sizeof(Color));
        view.weapon.resize(PreparedView::weaponPixelCount);
        in.bytes(view.weapon.data(), view.weapon.size() * sizeof(Color));
    }
    in.finish();
    validate(scene);
    requirePreparationBytes(0, sceneStorageBytes(scene), std::min(limits.residentBytes, limits.loadingBytes));
    return scene;
}
} // namespace blobber
