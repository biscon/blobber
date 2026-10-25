#include "world/map.hpp"
#include <fstream>
#include <stdexcept>

namespace blobber {
Map::Map(const std::vector<std::string>& rows) {
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
            if (!solid(next) && !visited[next.y * width_ + next.x]) {
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
