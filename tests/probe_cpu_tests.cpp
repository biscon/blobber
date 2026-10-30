#include "probe/scene.hpp"
#include "support/check.hpp"
#include "support/probe_allocations.hpp"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <new>
#ifdef _MSC_VER
#include <malloc.h>
#endif
using namespace blobber;
using namespace blobber::probe;
int main() {
    try {
        Map one({"###", "#.#", "###"}), two({"####", "#..#", "####"});
        check(buildFaces(one, false).size() == 6, "Single open cell has six inward surfaces");
        check(buildFaces(two, false).size() == 10, "No internal wall between connected open cells");
        check(buildFaces(one, true).size() == 5, "Outdoor cell has no ceiling");
        rejects([] { buildFaces(Map(std::vector<std::string>(33, std::string(33, '#'))), false); },
                "Oversize prototype rejected");
        auto scene = makeScene(false);
        for (const auto& face : scene.faces) {
            auto a = face.vertices[0].p, b = face.vertices[1].p, c = face.vertices[2].p,
                 n = face.vertices[0].normal;
            Vec u{b.x - a.x, b.y - a.y, b.z - a.z}, v{c.x - a.x, c.y - a.y, c.z - a.z};
            const float dot =
                (u.y * v.z - u.z * v.y) * n.x + (u.z * v.x - u.x * v.z) * n.y + (u.x * v.y - u.y * v.x) * n.z;
            check(dot > .99f, "Winding faces open volume");
        }
        for (int heading = 0; heading < 4; ++heading) {
            MovementState s;
            s.player = {{5, 10}, Heading(heading)};
            auto c = camera(s);
            auto f = forward(s.player.heading), r = right(s.player.heading);
            const Vec p{5.5f + f.x * 3 + r.x * .2f, 1, 10.5f + f.y * 3 + r.y * .2f};
            const auto point = project(c, p);
            check(std::abs(point.depth - 3.4f) < .00001f, "Setback adds forward depth");
            check(std::abs(point.x - (250 + focal() * .2f / 3.4f)) < .001f, "Cardinal lateral projection");
            check(std::abs(point.y - (height * .34f - focal() * .5f / 3.4f)) < .001f,
                  "Raised horizon projection");
            for (Action action : {Action::TurnLeft, Action::TurnRight}) {
                Controller control({{2, 2}, Heading(heading)});
                control.submit(scene.map, action, true);
                for (int i = 0; i <= 160; ++i) {
                    check(clearance(scene.map, camera(control.state())),
                          "Turn near plane stays outside walls");
                    control.update(scene.map, .001f);
                }
            }
        }
        auto left = scripted(scene, 2 * shotMs + 280), rightState = scripted(scene, 3 * shotMs + 280);
        check(std::abs(camera(left).position.x - camera(rightState).position.x) < .001f,
              "Opposite strafe midpoints coincide");
        check(!scripted(scene, 9 * shotMs + 200).transition.active(),
              "Animation-off snaps to accepted endpoint");
        auto pause1 = camera(scripted(scene, 8 * shotMs + 280)),
             pause2 = camera(scripted(scene, 8 * shotMs + 700));
        check(pause1.position.x == pause2.position.x, "Paused scripted pose remains fixed");
        for (int heading = 0; heading < 4; ++heading) {
            MovementState state;
            state.player = {{5, 10}, Heading(heading)};
            const auto c = camera(state);
            const Vec feet{5.25f, 0, 9.25f};
            const auto center = billboardCenter(c, feet, .45f, .65f, .3f, .9f);
            check(std::abs(center.x + c.right.x * (.3f - .5f) * .45f - feet.x) < .00001f &&
                      std::abs(center.z + c.right.z * (.3f - .5f) * .45f - feet.z) < .00001f &&
                      std::abs(center.y + (.5f - .9f) * .65f - feet.y) < .00001f,
                  "Metadata foot anchor remains at world feet for every heading");
        }
        auto dark = shade({1, 1, 1}, 16, 1);
        check(std::abs(dark[0] - 15 / 255.f) < .00001f, "Far geometry merges into fog");
        check(std::abs(faceLight({1, 0, 0}, {0, 0, -1}) - .78f) < .00001f, "Side shading baseline");
        check(faceLight({0, 0, 1}, {0, 0, -1}) == 1, "Front shading baseline");
        watching = true;
        for (unsigned ms = 0; ms < sequenceMs; ms += 10) {
            auto s = scripted(scene, ms);
            auto c = camera(s);
            check(clearance(scene.map, c), "Script camera clearance");
            (void)project(c, {5, 0, 9});
            (void)shade({1, 1, 1}, 3, .8f);
        }
        watching = false;
        check(allocations == 0, "CPU playback does not allocate");
        std::cout << "Probe CPU invariants and allocation guard pass\n";
        return 0;
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
