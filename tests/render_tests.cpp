#include "generation/floor.hpp"
#include "support/check.hpp"
#include "support/render_fixtures.hpp"
#include <cmath>
#include <iostream>
using namespace blobber;
namespace {
Map openMap() {
    return Map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
}
void testTransitions() {
    const Frame a({200, 0, 0, 255}), b({0, 200, 0, 255});
    Frame output;
    rejects([&] { composeTransition(a, b, Action::Left, 0.5f, output); },
            "Side steps require prepared views");
    rejects([&] { composeTransition(output, b, Action::Forward, 0.5f, output); }, "Aliased output rejected");
    for (int action = 0; action < 6; ++action) {
        if (action == static_cast<int>(Action::Left) || action == static_cast<int>(Action::Right))
            continue;
        check(same(transition(a, b, static_cast<Action>(action), 0), a), "Exact source endpoint");
        check(same(transition(a, b, static_cast<Action>(action), 1), b), "Exact destination endpoint");
        for (float phase : {0.01f, 0.25f, 0.5f, 0.75f, 0.99f}) {
            const Frame frame = transition(a, b, static_cast<Action>(action), phase);
            check(frame.size() == a.size(), "Transition dimensions");
            check(std::all_of(frame.begin(), frame.end(),
                              [](Color c) { return c.a == 255 && (c.r == 200 || c.g == 200); }),
                  "Transitions never expose uninitialized or transparent pixels");
        }
    }
}
// Test-only geometric oracle. Intersect every map block independently rather than
// sharing the renderer's prepared face/column tables or its visibility algorithm.
float firstBlock(const Map& map, Player p, float slope, float offset = 0) {
    const Cell f = forward(p.heading), r = right(p.heading);
    const float vx = f.x + r.x * slope, vy = f.y + r.y * slope;
    float nearest = 1000;
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x) {
            if (!map.solid({x, y}))
                continue;
            float enter = 0, leave = 1000;
            auto slab = [&](float origin, float direction, float minimum, float maximum) {
                if (std::abs(direction) < 0.000001f)
                    return origin >= minimum && origin <= maximum;
                float a = (minimum - origin) / direction, b = (maximum - origin) / direction;
                if (a > b)
                    std::swap(a, b);
                enter = std::max(enter, a);
                leave = std::min(leave, b);
                return enter < leave;
            };
            if (slab(p.cell.x + 0.5f - f.x * ViewProjection::cameraSetback + r.x * offset, vx, float(x),
                     float(x + 1)) &&
                slab(p.cell.y + 0.5f - f.y * ViewProjection::cameraSetback + r.y * offset, vy, float(y),
                     float(y + 1)))
                nearest = std::min(nearest, enter);
        }
    return nearest;
}
void generatedRooms() {
    const auto generated = generation::generate({5});
    check(bool(generated), "generated render fixture");
    const auto& f = *generated.floor;
    SceneRenderer renderer(testMaterials());
    for (unsigned room = 0; room < f.layout.roomCount; ++room) {
        for (unsigned heading = 0; heading < 4; ++heading) {
            const Player from{f.layout.rooms[room].anchor, Heading(heading)};
            const auto frame = compose(renderer, f.map, from);
            for (int x = 0; x < viewWidth; x += 5) {
                const float focal = ViewProjection::focalLength();
                const auto depth = firstBlock(f.map, from, (x + .5f - viewWidth * .5f) / focal);
                if (depth >= 9)
                    continue;
                const auto top = ViewProjection::horizon -
                                 focal * (ViewProjection::wallHeight - ViewProjection::eyeHeight) / depth;
                const auto bottom = ViewProjection::horizon + focal * ViewProjection::eyeHeight / depth;
                for (int y = 0; y < viewHeight; y += 5) {
                    if (std::abs(y + .5f - top) < .0001f || std::abs(y + .5f - bottom) < .0001f)
                        continue;
                    const auto color = frame[y * viewWidth + x];
                    check((y + .5f > top && y + .5f < bottom) == (color.r > 0 && !color.g && !color.b),
                          "generated room and pillar occlusion agree with independent ray geometry");
                }
            }
            const Player to{from.cell + right(from.heading), from.heading};
            if (!f.map.solid(to.cell)) {
                check(same(strafe(renderer, f.map, from, to, 0), frame), "generated strafe source");
                check(same(strafe(renderer, f.map, from, to, 1), compose(renderer, f.map, to)),
                      "generated strafe destination");
                check(same(strafe(renderer, f.map, from, to, .5f), strafe(renderer, f.map, to, from, .5f)),
                      "generated strafe reversal matches");
            }
        }
    }
}
void testOcclusion(const Map& map) {
    SceneRenderer renderer(testMaterials());
    for (Cell cell : {Cell{3, 21}, Cell{5, 8}, Cell{18, 20}, Cell{11, 11}, Cell{2, 2}, Cell{8, 8},
                      Cell{3, 16}, Cell{3, 15}, Cell{3, 14}}) {
        for (int heading = 0; heading < 4; ++heading) {
            const Player p{cell, static_cast<Heading>(heading)};
            const Frame frame = compose(renderer, map, p);
            for (int x = 0; x < viewWidth; ++x) {
                const float focal = ViewProjection::focalLength();
                const float depth = firstBlock(map, p, (x + 0.5f - viewWidth * 0.5f) / focal);
                if (depth >= 9)
                    continue; // Fog deliberately mixes material colors farther away.
                for (int y = 0; y < viewHeight; y += 3) {
                    const float top =
                        ViewProjection::horizon -
                        focal * (ViewProjection::wallHeight - ViewProjection::eyeHeight) / depth;
                    const float bottom = ViewProjection::horizon + focal * ViewProjection::eyeHeight / depth;
                    // Exact edge ties can round in opposite directions in independent
                    // slab intersection and sprite projection; only interior pixels decide visibility.
                    if (std::abs(y + 0.5f - top) < 0.0001f || std::abs(y + 0.5f - bottom) < 0.0001f)
                        continue;
                    const bool wall = y + 0.5f > top && y + 0.5f < bottom;
                    const Color c = frame[y * viewWidth + x];
                    const bool drawnWall = c.r > 0 && c.g == 0 && c.b == 0;
                    if (wall != drawnWall)
                        std::cerr << "pose=" << cell.x << ',' << cell.y << " heading=" << heading
                                  << " pixel=" << x << ',' << y << " depth=" << depth << " expected=" << wall
                                  << " rgb=" << int(c.r) << ',' << int(c.g) << ',' << int(c.b) << '\n';
                    check(wall == drawnWall, "Prepared sprite occlusion must match independent geometry");
                }
            }
        }
    }

    // User-facing framing regressions: test visible results, not just projection math.
    const Map sealed({"#######", "###.###", "###.###", "###.###", "###.###", "###.###", "#######"});
    const Map rightOpening({"#######", "###.###", "###.###", "###...#", "###.###", "###.###", "#######"});
    const Map leftOpening({"#######", "###.###", "###.###", "#...###", "###.###", "###.###", "#######"});
    const Player atJunction{{3, 3}, Heading::North};
    const Frame closed = compose(renderer, sealed, atJunction);
    const Frame openRight = compose(renderer, rightOpening, atJunction);
    const Frame openLeft = compose(renderer, leftOpening, atJunction);
    int visibleRight = 0, visibleLeft = 0;
    const int horizonRow = static_cast<int>(ViewProjection::horizon);
    for (int x = 0; x < viewWidth / 4; ++x) {
        const int li = horizonRow * viewWidth + x, ri = horizonRow * viewWidth + viewWidth - 1 - x;
        visibleRight += openRight[ri].r != closed[ri].r;
        visibleLeft += openLeft[li].r != closed[li].r;
        check(openRight[li].r == closed[li].r,
              "Right passage must not reveal geometry through the left wall");
        check(openLeft[ri].r == closed[ri].r, "Left passage must not reveal geometry through the right wall");
    }
    check(visibleRight >= 80 && visibleLeft >= 80,
          "Adjacent side passages must occupy a readable strip of the view");
    // With walls immediately around this cell, all visible floor belongs to the
    // current cell. Check a usable central strip in every cardinal view.
    const Map enclosedCell({"###", "#.#", "###"});
    for (int heading = 0; heading < 4; ++heading) {
        const Frame nearWall = compose(renderer, enclosedCell, {{1, 1}, static_cast<Heading>(heading)});
        for (int y = viewHeight - 32; y < viewHeight; ++y)
            for (int x = viewWidth / 4; x < 3 * viewWidth / 4; ++x) {
                const Color c = nearWall[y * viewWidth + x];
                check(c.g > 0 && c.r == 0 && c.b == 0 && c.a == 255,
                      "Every heading must expose 32 rows of current-cell floor across the central half");
            }
    }
    const Frame room = compose(renderer, openMap(), {{3, 3}, Heading::North});
    int floorPixels = 0, ceilingPixels = 0;
    for (const Color c : room) {
        floorPixels += c.g > 0 && c.r == 0 && c.b == 0;
        ceilingPixels += c.b > 0 && c.r == 0 && c.g == 0;
    }
    check(floorPixels > ceilingPixels * 2, "Room framing should favor visible floor over ceiling");

    // Side steps hold heading fixed. Compare intermediate views with an independent
    // translated-eye oracle, including the reported (8,4) room position and junctions.
    for (Cell cell : {Cell{8, 4}, Cell{3, 15}, Cell{18, 20}}) {
        for (int heading = 0; heading < 4; ++heading)
            for (int sign : {-1, 1}) {
                const Player from{cell, static_cast<Heading>(heading)};
                const Player to{cell + right(from.heading) * sign, from.heading};
                if (map.solid(to.cell))
                    continue;
                check(same(strafe(renderer, map, from, to, 0), compose(renderer, map, from)),
                      "Exact strafe source endpoint");
                check(same(strafe(renderer, map, from, to, 1), compose(renderer, map, to)),
                      "Exact strafe destination endpoint");
                for (float t : {0.25f, 0.5f, 0.75f}) {
                    const Frame frame = strafe(renderer, map, from, to, t);
                    check(same(frame, strafe(renderer, map, to, from, 1 - t)),
                          "Strafing back retraces the same views");
                    const float eased = t * t * (3 - 2 * t);
                    const float offset =
                        sign * std::round(eased * ViewProjection::strafeSteps) / ViewProjection::strafeSteps;
                    const float focal = ViewProjection::focalLength();
                    for (int x = 0; x < viewWidth; ++x) {
                        const float depth =
                            firstBlock(map, from, (x + 0.5f - viewWidth * 0.5f) / focal, offset);
                        if (depth >= 9)
                            continue;
                        const float top =
                            ViewProjection::horizon -
                            focal * (ViewProjection::wallHeight - ViewProjection::eyeHeight) / depth;
                        const float bottom =
                            ViewProjection::horizon + focal * ViewProjection::eyeHeight / depth;
                        for (int y = 0; y < viewHeight; y += 3) {
                            if (std::abs(y + 0.5f - top) < 0.0001f || std::abs(y + 0.5f - bottom) < 0.0001f)
                                continue;
                            const Color c = frame[y * viewWidth + x];
                            const bool expected = y + 0.5f > top && y + 0.5f < bottom;
                            check(expected == (c.r > 0 && c.g == 0 && c.b == 0),
                                  "Strafe parallax and occlusion must match translation, not rotation");
                            check(c.a == 255, "Strafes have no holes while revealing geometry");
                        }
                    }
                }
            }
    }
}
} // namespace
int main(int argc, char** argv) {
    try {
        check(argc == 2, "Expected test map fixture");
        testTransitions();
        generatedRooms();
        testOcclusion(Map::load(argv[1]));
        std::cout << "Render checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
