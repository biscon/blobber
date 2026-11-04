#include "ui/map_model.hpp"
#include "generation/floor.hpp"
#include <algorithm>
namespace blobber {
MapGridView MapGridView::fit(MapBounds bounds, Cell party, int cx, int cy, int w, int h) noexcept {
    MapGridView result;
    const auto axis = [](int start, int extent, int player, int origin, int available, int& first, int& count,
                         float& offset) {
        extent = std::max(0, extent);
        count = std::min(extent, std::max(0, available / cellSize));
        first = count ? std::clamp(player - count / 2, start, start + extent - count) : start;
        offset = float(origin) + float(available - count * cellSize) * .5f;
    };
    axis(bounds.first.x, bounds.width, party.x, cx, w, result.first.x, result.columns, result.x);
    axis(bounds.first.y, bounds.height, party.y, cy, h, result.first.y, result.rows, result.y);
    return result;
}
MapMarker mapFixtureMarker(const FloorGeometry& floor, unsigned slot, bool reveal) noexcept {
    if (slot >= floor.definition().count || (!reveal && !floor.known(slot)))
        return MapMarker::None;
    const auto kind = floor.definition().entries[slot].kind;
    const bool open = reveal ? floor.open(slot) : floor.rememberedOpen(slot);
    const bool done = reveal ? floor.completed(slot) : floor.rememberedComplete(slot);
    switch (kind) {
    case FixtureKind::Door:
        return open ? MapMarker::OpenDoor : MapMarker::ClosedDoor;
    case FixtureKind::GuardianGate:
        return done ? MapMarker::OpenDoor : MapMarker::GuardianGate;
    case FixtureKind::Key:
        return done ? MapMarker::None : MapMarker::Key;
    case FixtureKind::Upstairs:
        return MapMarker::Upstairs;
    case FixtureKind::Downstairs:
        return MapMarker::Downstairs;
    case FixtureKind::Endpoint:
        return MapMarker::Objective;
    }
    return MapMarker::None;
}
bool MapOverlayModel::inside(Cell c) const noexcept {
    return map_ && c.x >= 0 && c.y >= 0 && c.x < map_->width() && c.y < map_->height();
}
void MapOverlayModel::prepare(const Map& map, const generation::FloorLayout* rooms) noexcept {
    map_ = &map;
    Cell minimum{map.width(), map.height()}, maximum{-1, -1};
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x)
            if (!map.solid({x, y})) {
                minimum.x = std::min(minimum.x, x);
                minimum.y = std::min(minimum.y, y);
                maximum.x = std::max(maximum.x, x);
                maximum.y = std::max(maximum.y, y);
            }
    bounds_ = {{0, 0}, map.width(), map.height()};
    if (maximum.x >= 0) {
        bounds_.first = {std::max(0, minimum.x - 1), std::max(0, minimum.y - 1)};
        bounds_.width = std::min(map.width(), maximum.x + 2) - bounds_.first.x;
        bounds_.height = std::min(map.height(), maximum.y + 2) - bounds_.first.y;
    }
    ready_ = false;
    roomCount_ = 0;
    rooms_.fill(generation::noRegion);
    if (!rooms || rooms->width != map.width() || rooms->height != map.height() ||
        map.width() * map.height() > int(discoveryCapacity) || rooms->roomCount > generation::maxRooms)
        return;
    for (int n = 0; n < map.width() * map.height(); ++n) {
        const auto id = rooms->membership[unsigned(n)];
        if (id != generation::noRegion &&
            (id >= rooms->roomCount || map.solid({n % map.width(), n / map.width()})))
            return;
    }
    roomCount_ = rooms->roomCount;
    std::copy_n(rooms->membership.begin(), map.width() * map.height(), rooms_.begin());
}
void MapOverlayModel::refresh(const FloorGeometry* floor) noexcept {
    if (!map_)
        return;
    if (floor && &floor->view().base() != map_)
        floor = nullptr;
    const auto world = floor ? floor->world() : 0;
    const auto revision = floor ? floor->discoveryRevision() : 0;
    if (ready_ && world_ == world && discovery_ == revision)
        return;
    ready_ = true;
    world_ = world;
    discovery_ = revision;
    fog_ = floor && floor->definition().discovery;
    floors_.reset();
    walls_.reset();
    if (!fog_)
        return;
    std::array<bool, generation::maxRooms> seen{};
    for (int y = 0; y < map_->height(); ++y)
        for (int x = 0; x < map_->width(); ++x) {
            const Cell c{x, y};
            const auto n = std::size_t(y * map_->width() + x);
            if (!floor->discovered(c))
                continue;
            if (map_->solid(c)) {
                walls_.set(n);
                continue;
            }
            floors_.set(n);
            const auto slot = floor->at(c);
            const bool threshold =
                slot >= 0 && (floor->definition().entries[unsigned(slot)].kind == FixtureKind::Door ||
                              floor->definition().entries[unsigned(slot)].kind == FixtureKind::GuardianGate);
            if (!threshold && rooms_[n] < roomCount_)
                seen[rooms_[n]] = true;
        }
    for (int y = 0; y < map_->height(); ++y)
        for (int x = 0; x < map_->width(); ++x) {
            const auto n = std::size_t(y * map_->width() + x);
            if (rooms_[n] < roomCount_ && seen[rooms_[n]])
                floors_.set(n);
        }
    for (int y = 0; y < map_->height(); ++y)
        for (int x = 0; x < map_->width(); ++x) {
            if (!floors_[std::size_t(y * map_->width() + x)])
                continue;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    const Cell wall{x + dx, y + dy};
                    if (inside(wall) && map_->solid(wall))
                        walls_.set(std::size_t(wall.y * map_->width() + wall.x));
                }
        }
}
MapTile MapOverlayModel::tile(Cell c, bool reveal) const noexcept {
    if (!inside(c))
        return MapTile::Hidden;
    if (fog_ && !reveal) {
        const auto n = std::size_t(c.y * map_->width() + c.x);
        return floors_[n] ? MapTile::Floor : walls_[n] ? MapTile::Wall : MapTile::Hidden;
    }
    if (!map_->solid(c))
        return MapTile::Floor;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
            const Cell adjacent{c.x + dx, c.y + dy};
            if (inside(adjacent) && !map_->solid(adjacent))
                return MapTile::Wall;
        }
    return MapTile::Hidden;
}
} // namespace blobber
