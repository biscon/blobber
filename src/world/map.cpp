#include "world/map.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>

namespace blobber {
Map::Map(const std::vector<std::string>& rows, WorldDefinition world) : world_(std::move(world)) {
    if (rows.empty() || rows.front().empty())
        throw std::runtime_error("Map is empty");
    if (rows.size() > maxDimension || rows.front().size() > maxDimension)
        throw std::runtime_error("Map exceeds 1024 x 1024 cell limit");
    width_ = static_cast<int>(rows.front().size());
    height_ = static_cast<int>(rows.size());
    cells_.reserve(static_cast<std::size_t>(width_ * height_));
    for (const auto& row : rows) {
        if (row.size() != static_cast<std::size_t>(width_))
            throw std::runtime_error("Map rows have unequal widths");
        for (char c : row) {
            if (c != '#' && c != '.')
                throw std::runtime_error("Map accepts only # and .");
            cells_.push_back(c == '#');
        }
    }
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x)
            if ((x == 0 || y == 0 || x == width_ - 1 || y == height_ - 1) && !solid({x, y}))
                throw std::runtime_error("Map perimeter must be enclosed");
    validateWorld();
}
Map Map::load(const std::string& path) {
    std::ifstream stream(path);
    if (!stream)
        throw std::runtime_error("Cannot read map: " + path);
    // A bounded line buffer prevents an invalid input line from growing a string
    // arbitrarily before the dimension check. One extra character permits CRLF.
    std::vector<std::string> rows;
    char line[maxDimension + 2];
    while (stream.getline(line, sizeof(line))) {
        // gcount includes the delimiter when one was consumed. Preserve embedded
        // NUL bytes so symbol validation rejects them instead of truncating a row.
        const auto count = static_cast<std::size_t>(stream.gcount());
        std::string row(line, count - (stream.eof() ? 0 : 1));
        if (!row.empty() && row.back() == '\r')
            row.pop_back();
        if (rows.size() == maxDimension || row.size() > maxDimension)
            throw std::runtime_error("Map exceeds 1024 x 1024 cell limit");
        rows.push_back(std::move(row));
    }
    if (stream.bad() || !stream.eof())
        throw std::runtime_error("Map line exceeds limit or could not be read: " + path);
    return Map(rows);
}
bool Map::solid(Cell c) const noexcept {
    return c.x < 0 || c.y < 0 || c.x >= width_ || c.y >= height_ || cells_[c.y * width_ + c.x] != 0;
}
CellSurface Map::surface(Cell c) const noexcept {
    return c.x < 0 || c.y < 0 || c.x >= width_ || c.y >= height_
               ? CellSurface{}
               : world_.surfaces[std::size_t(c.y) * width_ + c.x];
}
bool Map::canStep(Cell a, Cell b) const noexcept {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y) == 1 && !solid(a) && !solid(b) &&
           surfacePassage(surface(a), surface(b));
}
void Map::validateWorld() {
    const auto count = std::size_t(width_) * height_;
    if (world_.surfaces.empty())
        world_.surfaces.resize(count);
    if (world_.surfaces.size() != count || world_.decorations.size() > 384 ||
        world_.bytes() > worldDefinitionBudget)
        throw std::invalid_argument("World definition capacity/dimensions exceeded");
    for (int y = 0; y < height_; ++y)
        for (int x = 0; x < width_; ++x) {
            const auto s = surface({x, y});
            if (s.floor < -32 || s.floor > 32 ||
                (s.ceiling != noCeiling && (s.ceiling < -28 || s.ceiling > 48 || s.ceiling - s.floor < 4)) ||
                s.wallMaterial >= 32 || s.floorMaterial >= 32 || s.ceilingMaterial >= 32 ||
                unsigned(s.environment) > 1)
                throw std::invalid_argument("Invalid quantized world surface");
            if (!solid({x, y})) {
                flat_ &= s.floor == 0;
                lower_ = std::min(lower_, elevation(s.floor));
                upper_ = std::max(upper_, elevation(s.floor) + 1);
                if (s.ceiling != noCeiling)
                    upper_ = std::max(upper_, elevation(s.ceiling));
            }
        }
    unsigned decals = 0, props = 0;
    for (std::size_t i = 0; i < world_.decorations.size(); ++i) {
        const auto& d = world_.decorations[i];
        if (unsigned(d.face) > 3 || unsigned(d.kind) > 3 || d.material >= 32 || d.cell.x < 0 ||
            d.cell.y < 0 || d.cell.x >= width_ || d.cell.y >= height_ || !std::isfinite(d.offset) ||
            !std::isfinite(d.elevation) || !std::isfinite(d.width) || !std::isfinite(d.height) ||
            !std::isfinite(d.depth) || d.width <= 0 || d.width > 1 || d.height <= 0 || d.height > 4 ||
            d.depth <= 0 || d.depth > 1 || d.offset - d.width * .5f < 0 || d.offset + d.width * .5f > 1)
            throw std::invalid_argument("Invalid world decoration");
        if (d.cutout()) {
            ++decals;
            const auto s = surface(d.cell);
            if (solid(d.cell) || !solid(d.cell + forward(d.face)) || d.elevation - d.height * .5f < 0 ||
                elevation(s.floor) + d.elevation + d.height * .5f >
                    (s.ceiling == noCeiling ? upper_ : elevation(s.ceiling)))
                throw std::invalid_argument("Invalid wall decoration attachment");
            for (std::size_t j = 0; j < i; ++j) {
                const auto& o = world_.decorations[j];
                if (o.cutout() && o.cell == d.cell && o.face == d.face &&
                    std::abs(o.offset - d.offset) < (o.width + d.width) * .5f &&
                    std::abs(o.elevation - d.elevation) < (o.height + d.height) * .5f)
                    throw std::invalid_argument("Overlapping wall decorations");
            }
        } else {
            ++props;
            if (!solid(d.cell) && (d.height > .25f || d.depth > .25f || !solid(d.cell + forward(d.face))))
                throw std::invalid_argument("Nonblocking props require a low perimeter strip");
        }
    }
    if (decals > 256 || props > 128)
        throw std::invalid_argument("World decoration capacity exceeded");
}
bool Map::connected(Cell start) const {
    if (solid(start))
        return false;
    std::vector<std::uint8_t> visited(cells_.size(), 0);
    std::vector<Cell> todo;
    todo.reserve(cells_.size());
    todo.push_back(start);
    visited[start.y * width_ + start.x] = 1;
    for (std::size_t head = 0; head < todo.size(); ++head) {
        for (int d = 0; d < 4; ++d) {
            const Cell next = todo[head] + forward(static_cast<Heading>(d));
            if (canStep(todo[head], next) && !visited[next.y * width_ + next.x]) {
                visited[next.y * width_ + next.x] = 1;
                todo.push_back(next);
            }
        }
    }
    for (std::size_t i = 0; i < cells_.size(); ++i)
        if (!cells_[i] && !visited[i])
            return false;
    return true;
}
} // namespace blobber
