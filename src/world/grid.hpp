#pragma once

#include <array>

namespace blobber {
struct Cell {
    int x = 0, y = 0;
};
inline Cell operator+(Cell a, Cell b) {
    return {a.x + b.x, a.y + b.y};
}
inline Cell operator*(Cell a, int n) {
    return {a.x * n, a.y * n};
}
inline bool operator==(Cell a, Cell b) {
    return a.x == b.x && a.y == b.y;
}
enum class Heading { North, East, South, West };
inline Cell forward(Heading heading) {
    constexpr std::array<Cell, 4> directions{{{0, -1}, {1, 0}, {0, 1}, {-1, 0}}};
    return directions.at(static_cast<unsigned>(heading));
}
inline Cell right(Heading heading) {
    return forward(static_cast<Heading>((static_cast<int>(heading) + 1) % 4));
}
inline const char* headingName(Heading heading) {
    constexpr std::array<const char*, 4> names{"NORTH", "EAST", "SOUTH", "WEST"};
    return names.at(static_cast<unsigned>(heading));
}
} // namespace blobber
