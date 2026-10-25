#include "game/movement.hpp"
#include "input/key_repeat.hpp"
#include "render/layout.hpp"
#include "support/check.hpp"
#include <iostream>
using namespace blobber;
namespace {
Map openMap() {
    return Map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
}
void testMovement() {
    const Map map = openMap();
    for (int h = 0; h < 4; ++h) {
        const Player origin{{3, 3}, static_cast<Heading>(h)};
        for (int a = 0; a < 4; ++a) {
            Player p = origin;
            const Cell direction = a < 2 ? forward(p.heading) : right(p.heading);
            const int sign = (a == 1 || a == 2) ? -1 : 1;
            check(tryAction(map, p, static_cast<Action>(a)), "Open movement must succeed");
            check(p.cell == origin.cell + direction * sign, "Movement must be heading-relative");
            check(p.heading == origin.heading, "Translation must preserve heading");
        }
        Player p = origin;
        for (int i = 0; i < 4; ++i)
            tryAction(map, p, Action::TurnLeft);
        check(p == origin, "Four left turns restore pose");
        for (int i = 0; i < 4; ++i)
            tryAction(map, p, Action::TurnRight);
        check(p == origin, "Four right turns restore pose");
        p.cell = {1, 1};
        for (int a = 0; a < 4; ++a) {
            Player q = p;
            const bool moved = tryAction(map, q, static_cast<Action>(a));
            check(!map.solid(q.cell), "Cannot enter a wall");
            if (!moved)
                check(q == p, "Blocked movement preserves entire pose");
        }
    }
    check(map.solid({-1, 3}) && map.solid({3, -1}) && map.solid({7, 3}) && map.solid({3, 7}),
          "Outside map is solid");
    bool rejected = false;
    try {
        Map invalid({"###", "#.", "###"});
    } catch (...) {
        rejected = true;
    }
    check(rejected, "Ragged maps rejected");
    rejected = false;
    try {
        Map invalid({"###", "#..", "###"});
    } catch (...) {
        rejected = true;
    }
    check(rejected, "Open perimeter rejected");
    rejected = false;
    try {
        Map invalid({"###", "#x#", "###"});
    } catch (...) {
        rejected = true;
    }
    check(rejected, "Invalid symbols rejected");
}
void testController() {
    const Map map = openMap();
    Controller c({{3, 3}, Heading::North});
    c.submit(map, Action::Forward);
    check(c.state().transition.active() && c.state().player.cell == Cell{3, 2},
          "Move starts one discrete transition");
    c.submit(map, Action::Right, false);
    c.update(map, 1);
    check(c.state().player.cell == Cell{3, 2}, "Held command does not buffer during transition");
    c.submit(map, Action::TurnLeft);
    c.submit(map, Action::Forward);
    c.submit(map, Action::Back);
    c.update(map, 1);
    check(c.state().player.cell == Cell{4, 2}, "Only newest fresh command buffers, relative to new heading");
    c.submit(map, Action::Back);
    c.clearPending();
    c.update(map, 1);
    check(c.state().player.cell == Cell{4, 2}, "Focus clear drops pending move");
    c.setAnimated(false);
    c.submit(map, Action::Forward);
    check(!c.state().transition.active() && c.state().player.cell == Cell{3, 2},
          "Instant mode completes immediately");
    c = Controller({{1, 1}, Heading::North});
    const auto revision = c.state().revision;
    c.submit(map, Action::Forward);
    check(c.state().blocked && revision == c.state().revision, "Blocked move does not start animation");
    KeyRepeat repeat;
    std::array<bool, 6> down{}, pressed{};
    down[0] = pressed[0] = true;
    check(repeat.sample(down, pressed, 0) == Action::Forward, "Key press immediate");
    pressed.fill(false);
    check(!repeat.sample(down, pressed, 0.29f), "Initial repeat delay");
    check(repeat.sample(down, pressed, 0.02f) == Action::Forward, "First repeat after 300 ms");
    check(!repeat.sample(down, pressed, 0.17f), "Repeat interval");
    check(repeat.sample(down, pressed, 0.02f) == Action::Forward, "Repeat after 180 ms");
    repeat.clear();
    check(!repeat.sample(down, pressed, 1), "Focus reset requires fresh key press");
}
void testFit() {
    auto fit = aspectFit(1920, 1080);
    check(fit.x == 0 && fit.y == 0 && fit.width == 1920 && fit.height == 1080, "Native fit");
    fit = aspectFit(1280, 720);
    check(fit.x == 0 && fit.y == 0 && fit.width == 1280 && fit.height == 720, "720p fit");
    fit = aspectFit(1024, 768);
    check(fit.y == 96 && fit.x == 0 && fit.height == 576, "4:3 letterbox");
    fit = aspectFit(2560, 1080);
    check(fit.x == 320 && fit.y == 0 && fit.width == 1920, "Ultrawide pillarbox");
    fit = aspectFit(0, 0);
    check(fit.width == 0 && fit.height == 0, "Minimized window");
}
} // namespace
int main() {
    try {
        testMovement();
        testController();
        testFit();
        std::cout << "Core checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
