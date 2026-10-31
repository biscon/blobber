#pragma once
#include "game/npc_animation.hpp"
namespace blobber::test {
inline NpcTimings staticTimings() {
    NpcTimings t{};
    t[0] = {{{1000, 0, 0, 0}}, 1};
    return t;
}
inline NpcTimings animatedTimings() {
    auto t = staticTimings();
    t[0] = {{{60, 60, 0, 0}}, 2};
    t[1] = {{{30, 30, 0, 0}}, 2};
    t[2] = {{{70, 25, 25, 35}}, 4};
    return t;
}
inline NpcTimings reactionTimings() {
    auto t = animatedTimings();
    t[3] = {{{150, 150, 0, 0}}, 2};
    t[4] = {{{200, 200, 200, 600}}, 4};
    t[6] = {{{1000, 0, 0, 0}}, 1};
    return t;
}
} // namespace blobber::test
