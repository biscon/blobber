#include "generation/stone_layout.hpp"
#include <algorithm>
namespace blobber::generation {
namespace {
// Frozen, reviewed geometry records. These never call the random generator.
namespace compact {
constexpr const char* rows[]{
    "####################################", "####################################",
    "##############88888#99999###########", "##############8#888#99999###########",
    "##############8#888#99999###########", "##############8#####99999###########",
    "##############8#####99999###########", "##############.#####.###############",
    "##############..####555#############", "###############.####555#333333######",
    "###############.####555#333333######", "###############.####555#33#333######",
    "###############.####555#333333######", "###############.####555#333333######",
    "###############.#####.###.###.######", "##############00000000.111###.######",
    "##############00000000#111###4444###", "##############00000000#111###4444###",
    "###################.###111###4444###", "###################.###11111#4444###",
    "###################.###11111.4444###", "###################.###11111#.######",
    "###################.#########.######", "############666666#...#######.######",
    "############666666###.#######.######", "############666666#222........######",
    "##777#######666666.222##############", "##777###....666666#222##############",
    "##777###.##########222##############", "##7#####.###########################",
    "##77777..###########################", "####################################",
    "####################################", "####################################",
    "####################################", "####################################",
};
constexpr Room rooms[]{
    {{14, 15}, {8, 3}, {17, 16}, Role::Junction, 0, 3, 0, 1, Heading::East},
    {{23, 15}, {5, 7}, {24, 18}, Role::Junction, 1, 2, 0, 1, Heading::West},
    {{19, 25}, {3, 4}, {20, 26}, Role::Junction, 2, 0, 0, 1, Heading::North},
    {{24, 9}, {6, 5}, {28, 11}, Role::Junction, 3, 1, 0, 1, Heading::South},
    {{29, 16}, {4, 5}, {30, 18}, Role::Junction, 4, 0, 0, 1, Heading::West},
    {{20, 8}, {3, 6}, {21, 10}, Role::Junction, 5, 0, 0, 1, Heading::South},
    {{12, 23}, {6, 5}, {14, 25}, Role::Approach, 6, 0, 0, 1, Heading::East},
    {{2, 26}, {5, 5}, {3, 27}, Role::Objective, 7, 4, 0, 1, Heading::East},
    {{14, 2}, {5, 5}, {17, 3}, Role::Entrance, 8, 4, 1, 1, Heading::South},
    {{20, 2}, {5, 5}, {22, 4}, Role::Optional, 9, 0, 0, 1, Heading::South},
};
constexpr Connection connections[]{
    {0, 1, {22, 15}, {22, 15}, Heading::East, Heading::West, 0, 1},
    {0, 2, {19, 18}, {21, 24}, Heading::South, Heading::North, 1, 9},
    {1, 3, {25, 14}, {25, 14}, Heading::North, Heading::South, 10, 1},
    {1, 4, {28, 20}, {28, 20}, Heading::East, Heading::West, 11, 1},
    {0, 5, {21, 14}, {21, 14}, Heading::North, Heading::South, 12, 1},
    {3, 4, {29, 14}, {29, 15}, Heading::South, Heading::North, 13, 2},
    {4, 2, {29, 21}, {22, 25}, Heading::South, Heading::East, 15, 12},
    {2, 6, {18, 26}, {18, 26}, Heading::West, Heading::East, 27, 1},
    {6, 7, {11, 27}, {7, 30}, Heading::West, Heading::East, 28, 8},
    {0, 8, {15, 14}, {14, 7}, Heading::North, Heading::South, 36, 9},
    {5, 9, {20, 7}, {20, 7}, Heading::North, Heading::South, 45, 1},
};
constexpr std::uint16_t paths[]{562, 667, 703, 739, 775, 811, 847, 848, 849,  885,  529,  748,
                                525, 533, 569, 785, 821, 857, 893, 929, 928,  927,  926,  925,
                                924, 923, 922, 954, 983, 982, 981, 980, 1016, 1052, 1088, 1087,
                                519, 483, 447, 411, 375, 339, 303, 302, 266,  272};
constexpr unsigned loop = 5;
} // namespace compact
namespace chambers {
constexpr const char* rows[]{
    "####################################", "####################5555############",
    "####################5555#####6666###", "################111#5555....#6666###",
    "################111.5555###.#6666###", "################111########.#6666###",
    "################111########..6666###", "####999#########11111########.######",
    "####999#########11111########.######", "####999#########11111####.....######",
    "####9###########.###.####.##########", "####99999...88888###.####.##77777###",
    "############88888###.#4444##77777###", "############88888###.#4444##77777###",
    "############88888#000.4444####777###", "############88888#000#4444....777###",
    "##################000#4444####.#####", "##################000#########.#####",
    "##################000#########.#####", "##################000#######222#####",
    "##################000......#222#####", "##################000#####.#222#####",
    "###################.######..222#####", "###################.#########.######",
    "###################.#########.######", "###################.#########.######",
    "###################.#######...######", "###################...#####.########",
    "#####################.#####33333####", "#####################A#BBB#33333####",
    "#####################A#BBB#33#33####", "#################AAA#A#BBB.33333####",
    "#################AAA#A#BBB#33333####", "#################AAAAA#BBB##########",
    "#######################BBB##########", "####################################",
};
constexpr Room rooms[]{
    {{18, 14}, {3, 8}, {19, 17}, Role::Junction, 0, 3, 0, 1, Heading::East},
    {{16, 3}, {5, 7}, {17, 6}, Role::Junction, 1, 2, 0, 1, Heading::South},
    {{28, 19}, {3, 4}, {29, 20}, Role::Junction, 2, 0, 0, 1, Heading::West},
    {{27, 28}, {5, 5}, {29, 29}, Role::Junction, 3, 1, 0, 0, Heading::North},
    {{22, 12}, {4, 5}, {23, 14}, Role::Junction, 4, 0, 0, 1, Heading::West},
    {{20, 1}, {4, 4}, {21, 2}, Role::Junction, 5, 0, 0, 1, Heading::West},
    {{29, 2}, {4, 5}, {30, 4}, Role::Junction, 6, 0, 0, 1, Heading::West},
    {{28, 11}, {5, 5}, {30, 12}, Role::Junction, 7, 2, 0, 1, Heading::West},
    {{12, 11}, {5, 5}, {14, 13}, Role::Approach, 8, 0, 0, 1, Heading::North},
    {{4, 7}, {5, 5}, {5, 8}, Role::Objective, 9, 4, 0, 1, Heading::East},
    {{17, 29}, {5, 5}, {18, 32}, Role::Entrance, 10, 4, 3, 1, Heading::North},
    {{23, 29}, {3, 6}, {24, 31}, Role::Optional, 11, 0, 0, 1, Heading::East},
};
constexpr Connection connections[]{
    {0, 1, {20, 13}, {20, 10}, Heading::North, Heading::South, 0, 4},
    {0, 2, {21, 20}, {27, 22}, Heading::East, Heading::West, 4, 9},
    {2, 3, {29, 23}, {27, 27}, Heading::South, Heading::North, 13, 7},
    {0, 4, {21, 14}, {21, 14}, Heading::East, Heading::West, 20, 1},
    {1, 5, {19, 4}, {19, 4}, Heading::East, Heading::West, 21, 1},
    {5, 6, {24, 3}, {28, 6}, Heading::East, Heading::West, 22, 8},
    {4, 7, {26, 15}, {29, 15}, Heading::East, Heading::West, 30, 4},
    {6, 4, {29, 7}, {25, 11}, Heading::South, Heading::North, 34, 9},
    {7, 2, {30, 16}, {30, 18}, Heading::South, Heading::North, 43, 3},
    {1, 8, {16, 10}, {16, 10}, Heading::South, Heading::North, 46, 1},
    {8, 9, {11, 11}, {9, 11}, Heading::West, Heading::East, 47, 3},
    {0, 10, {19, 22}, {21, 28}, Heading::South, Heading::North, 50, 9},
    {3, 11, {26, 31}, {26, 31}, Heading::West, Heading::East, 59, 1},
};
constexpr std::uint16_t paths[]{488, 452, 416, 380, 741, 742, 743, 744, 745, 746, 782, 818, 819, 857,  893,
                                929, 965, 964, 963, 999, 525, 163, 132, 133, 134, 135, 171, 207, 243,  244,
                                566, 567, 568, 569, 281, 317, 353, 352, 351, 350, 349, 385, 421, 606,  642,
                                678, 376, 407, 406, 405, 811, 847, 883, 919, 955, 991, 992, 993, 1029, 1142};
constexpr unsigned loop = 7;
} // namespace chambers
template <std::size_t R, std::size_t E, std::size_t P>
void load(FloorLayout& f, const char* const* rows, const Room (&rooms)[R], const Connection (&edges)[E],
          const std::uint16_t (&paths)[P], unsigned loop) noexcept {
    static_assert(R <= maxRooms && E <= maxConnections && P <= maxCells, "Authored layout capacity");
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
        load(f, compact::rows, compact::rooms, compact::connections, compact::paths, compact::loop);
    else
        load(f, chambers::rows, chambers::rooms, chambers::connections, chambers::paths, chambers::loop);
    return true;
}
} // namespace blobber::generation
