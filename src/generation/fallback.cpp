#include "generation/stone_layout.hpp"
#include <algorithm>
namespace blobber::generation {
namespace {
// Explicit BR03 geometry. No random generation is called by the fallback path.
namespace compact {
constexpr const char* rows[]{
    "####################################", "########33333#######################",
    "########33333#############555555####", "########33333#############555555####",
    "########33333####.........555555####", "########33333####.########.#########",
    "##########.######.########..########", "########22222####.#########.#99999##",
    "########22222####.#########.#9#999##", "########22222####.#########.#9#999##",
    "########22222####.#########.#9######", "########22222####.#########.#9######",
    "########22222####.#########.#.######", "########22222#0000000######4444#####",
    "##########.###0000000######4444#####", "########11111#0000000####..4444#####",
    "########11111#0000000####.#4444#####", "########11111.0000000.....#4444#####",
    "########11111#0#000#0#########.#####", "########11111#0000000####AAAAAA#####",
    "##########.######.#######AAAAAA#####", "##########666.....#######AAAAAA#####",
    "##########666###############AAA#####", "##########666###############AAA#####",
    "##########66666#####################", "##########66666#####################",
    "##########66666#####################", "###########.########################",
    "#######77777########################", "#######77777##..88888###############",
    "#######77777...#####8###############", "#######77777######888###############",
    "#######77777######888###############", "##################888###############",
    "####################################", "####################################",
};
constexpr Room rooms[]{
    {{14, 13}, {7, 7}, {17, 16}, Role::Junction, 0, 5, 0, 0, Heading::North},
    {{8, 15}, {5, 5}, {10, 17}, Role::Junction, 1, 5, 2, 0, Heading::North},
    {{8, 7}, {5, 7}, {10, 10}, Role::Junction, 2, 5, 2, 0, Heading::North},
    {{8, 1}, {5, 5}, {10, 3}, Role::Junction, 3, 5, 3, 0, Heading::North},
    {{27, 13}, {4, 5}, {28, 15}, Role::Junction, 4, 0, 0, 1, Heading::West},
    {{26, 2}, {6, 3}, {28, 3}, Role::Junction, 5, 3, 0, 1, Heading::South},
    {{10, 21}, {5, 6}, {11, 23}, Role::Junction, 6, 2, 0, 1, Heading::North},
    {{7, 28}, {5, 5}, {9, 30}, Role::Approach, 7, 0, 0, 1, Heading::North},
    {{16, 29}, {5, 5}, {19, 32}, Role::Objective, 8, 4, 2, 1, Heading::North},
    {{29, 7}, {5, 5}, {32, 8}, Role::Entrance, 9, 4, 1, 1, Heading::West},
    {{25, 19}, {6, 5}, {27, 20}, Role::Optional, 10, 2, 0, 1, Heading::North},
};
constexpr Connection connections[]{
    {0, 1, {13, 17}, {13, 17}, Heading::West, Heading::East, 0, 1},
    {1, 2, {10, 14}, {10, 14}, Heading::North, Heading::South, 1, 1},
    {2, 3, {10, 6}, {10, 6}, Heading::North, Heading::South, 2, 1},
    {0, 4, {21, 17}, {26, 15}, Heading::East, Heading::West, 3, 8},
    {4, 5, {27, 12}, {26, 5}, Heading::North, Heading::South, 11, 9},
    {1, 6, {10, 20}, {10, 20}, Heading::South, Heading::North, 20, 1},
    {0, 6, {17, 20}, {13, 21}, Heading::South, Heading::East, 21, 6},
    {0, 5, {17, 12}, {25, 4}, Heading::North, Heading::West, 27, 17},
    {6, 7, {11, 27}, {11, 27}, Heading::South, Heading::North, 44, 1},
    {7, 8, {12, 30}, {15, 29}, Heading::East, Heading::West, 45, 5},
    {4, 9, {29, 12}, {29, 12}, Heading::North, Heading::South, 50, 1},
    {4, 10, {30, 18}, {30, 18}, Heading::South, Heading::North, 51, 1},
};
constexpr std::uint16_t paths[]{625, 514, 226, 633, 634, 635, 636,  637,  601,  565,  566,  459, 423,
                                387, 351, 315, 279, 243, 242, 206,  730,  737,  773,  772,  771, 770,
                                769, 449, 413, 377, 341, 305, 269,  233,  197,  161,  162,  163, 164,
                                165, 166, 167, 168, 169, 983, 1092, 1093, 1094, 1058, 1059, 461, 678};
constexpr BlueprintInstance blueprints[]{
    {{"mead-hall"}, blueprints::Purpose::MeadHall, 0, 0, 15, 8},
    {{"kitchen"}, blueprints::Purpose::Kitchen, 1, 2, 13, 6},
    {{"barracks"}, blueprints::Purpose::Barracks, 2, 2, 3, 7},
    {{"armory"}, blueprints::Purpose::Armory, 3, 3, 2, 6},
};
constexpr unsigned loop = 6;
} // namespace compact
namespace chambers {
constexpr const char* rows[]{
    "####################################", "####################################",
    "####################################", "#####77777##..88888#################",
    "#####77777##.#####8#################", "#####77777##.###888#################",
    "#####77777...###888#################", "#####77777######888#################",
    "#########.############AAA###########", "#######6666###########AAA###########",
    "#######6666###########AAA###########", "#######6666###########AAA###########",
    "#######6666......#####AAA###########", "#######.########.#####AAA###########",
    "#######..#######.#####AAA###########", "########.#######.#######.###########",
    "########.####0000000##11111#2222222#", "########.####0#000#0##11111#2222222#",
    "#####5555555.0000000..11111.2222222#", "#####5555555#0000000##11111#2222222#",
    "#####55#5555#0000000##11111#2222222#", "#####5555555#0000000####.###########",
    "#####5555555#0000000####.###########", "#########.#.####.#######.###########",
    "#########.#..###.#####33333#########", "########..##.###.#####33333#########",
    "########.###.###.#####33333#########", "######BBBB##.#...#####33333#########",
    "##999#BBBB##.#.#######33333#########", "##999#BBBB##44444###################",
    "##999#######44444###################", "##9#########44444###################",
    "##99999.....44444###################", "####################################",
    "####################################", "####################################",
};
constexpr Room rooms[]{
    {{13, 16}, {7, 7}, {16, 19}, Role::Junction, 0, 5, 2, 0, Heading::North},
    {{22, 16}, {5, 5}, {24, 18}, Role::Junction, 1, 5, 1, 0, Heading::North},
    {{28, 16}, {7, 5}, {31, 18}, Role::Junction, 2, 5, 1, 0, Heading::North},
    {{22, 24}, {5, 5}, {24, 26}, Role::Junction, 3, 5, 1, 0, Heading::North},
    {{12, 29}, {5, 4}, {14, 30}, Role::Junction, 4, 0, 0, 1, Heading::North},
    {{5, 18}, {7, 5}, {9, 20}, Role::Junction, 5, 1, 0, 1, Heading::East},
    {{7, 9}, {4, 4}, {8, 10}, Role::Junction, 6, 0, 0, 1, Heading::South},
    {{5, 3}, {5, 5}, {7, 5}, Role::Approach, 7, 0, 0, 1, Heading::South},
    {{14, 3}, {5, 5}, {17, 6}, Role::Objective, 8, 4, 2, 1, Heading::North},
    {{2, 28}, {5, 5}, {3, 29}, Role::Entrance, 9, 4, 0, 1, Heading::South},
    {{22, 8}, {3, 7}, {23, 11}, Role::Optional, 10, 3, 0, 1, Heading::South},
    {{6, 27}, {4, 3}, {7, 28}, Role::Optional, 11, 0, 0, 1, Heading::North},
};
constexpr Connection connections[]{
    {0, 1, {20, 18}, {21, 18}, Heading::East, Heading::West, 0, 2},
    {1, 2, {27, 18}, {27, 18}, Heading::East, Heading::West, 2, 1},
    {1, 3, {24, 21}, {24, 23}, Heading::South, Heading::North, 3, 3},
    {0, 4, {16, 23}, {14, 28}, Heading::South, Heading::North, 6, 8},
    {0, 5, {12, 18}, {12, 18}, Heading::West, Heading::East, 14, 1},
    {5, 6, {8, 17}, {7, 13}, Heading::North, Heading::South, 15, 6},
    {4, 5, {12, 28}, {11, 23}, Heading::North, Heading::South, 21, 7},
    {0, 6, {16, 15}, {11, 12}, Heading::North, Heading::East, 28, 9},
    {6, 7, {9, 8}, {9, 8}, Heading::North, Heading::South, 37, 1},
    {7, 8, {10, 6}, {13, 3}, Heading::East, Heading::West, 38, 7},
    {4, 9, {11, 32}, {7, 32}, Heading::West, Heading::East, 45, 5},
    {1, 10, {24, 15}, {24, 15}, Heading::North, Heading::South, 50, 1},
    {5, 11, {9, 23}, {8, 26}, Heading::South, Heading::North, 51, 5},
};
constexpr std::uint16_t paths[]{668, 669, 675, 780,  816,  852,  844,  880,  916, 952, 988, 987, 986, 1022,
                                660, 620, 584, 548,  512,  511,  475,  1020, 984, 948, 912, 876, 875, 839,
                                556, 520, 484, 448,  447,  446,  445,  444,  443, 297, 226, 227, 228, 192,
                                156, 120, 121, 1163, 1162, 1161, 1160, 1159, 564, 837, 873, 909, 908, 944};
constexpr BlueprintInstance blueprints[]{
    {{"mead-hall"}, blueprints::Purpose::MeadHall, 0, 2, 15, 8},
    {{"kitchen"}, blueprints::Purpose::Kitchen, 1, 1, 15, 6},
    {{"barracks"}, blueprints::Purpose::Barracks, 2, 1, 2, 7},
    {{"armory"}, blueprints::Purpose::Armory, 3, 1, 2, 6},
};
constexpr unsigned loop = 6;
} // namespace chambers
template <std::size_t R, std::size_t E, std::size_t P>
void load(FloorLayout& f, const char* const* rows, const Room (&rooms)[R], const Connection (&edges)[E],
          const std::uint16_t (&paths)[P], const BlueprintInstance (&blueprints)[4], unsigned loop) noexcept {
    static_assert(R <= maxRooms && E <= maxConnections && P <= maxCells, "Authored layout capacity");
    f.blueprintCount = 4;
    std::copy_n(blueprints, 4, f.blueprints.begin());
    f.roomCount = R;
    f.connectionCount = E;
    f.pathCount = P;
    f.loopConnection = loop;
    std::copy_n(rooms, R, f.rooms.begin());
    std::copy_n(edges, E, f.connections.begin());
    std::copy_n(paths, P, f.paths.begin());
    for (int y = 0; y < f.height; ++y)
        for (int x = 0; x < f.width; ++x) {
            const char c = rows[y][x];
            const auto i = unsigned(y * f.width + x);
            f.floor[i] = std::uint8_t(c != '#');
            f.membership[i] = c >= '0' && c <= '9'   ? std::uint8_t(c - '0')
                              : c >= 'A' && c <= 'F' ? std::uint8_t(c - 'A' + 10)
                                                     : noRegion;
        }
    for (unsigned i = 0; i < f.roomCount; ++i)
        if (f.rooms[i].role == Role::Entrance || f.rooms[i].role == Role::Objective)
            f.rooms[i].facing = Heading((2 + f.rooms[i].rotation) % 4);
    for (unsigned i = 0; i < f.roomCount; ++i)
        if (f.rooms[i].role == Role::Entrance)
            f.spawn = {f.rooms[i].anchor, f.rooms[i].facing};
}
} // namespace
bool buildFallback(FloorLayout& f) noexcept {
    if (f.profile == LayoutProfile::CompactWings)
        load(f, compact::rows, compact::rooms, compact::connections, compact::paths, compact::blueprints,
             compact::loop);
    else
        load(f, chambers::rows, chambers::rooms, chambers::connections, chambers::paths, chambers::blueprints,
             chambers::loop);
    return true;
}
} // namespace blobber::generation
