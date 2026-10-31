#include "render/world_scene.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/world_fixtures.hpp"
#include <cmath>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace blobber;
using namespace blobber::world_render;
int main() {
    try {
        const Map one({"###", "#.#", "###"}), two({"####", "#..#", "####"}), empty({"###", "###", "###"});
        check(prepare(one).indices.size() == 36 && prepare(two).indices.size() == 60,
              "Exposed faces exclude shared walls");
        check(prepare(empty).indices.empty(), "Enclosed solid cells produce no hidden faces");
        auto map = test::worldRoom(19, 27);
        const auto scene = prepare(map);
        check(scene.vertices.size() / 4 == 2u * 17 * 25 + 2u * (17 + 25),
              "Chunk seams introduce no duplicate faces");
        for (std::size_t i = 0; i < scene.indices.size(); i += 3) {
            const auto a = scene.vertices[scene.indices[i]], b = scene.vertices[scene.indices[i + 1]],
                       c = scene.vertices[scene.indices[i + 2]];
            const Vec u{b.position.x - a.position.x, b.position.y - a.position.y,
                        b.position.z - a.position.z},
                v{c.position.x - a.position.x, c.position.y - a.position.y, c.position.z - a.position.z};
            check((u.y * v.z - u.z * v.y) * a.normal.x + (u.z * v.x - u.x * v.z) * a.normal.y +
                          (u.x * v.y - u.y * v.x) * a.normal.z >
                      0,
                  "Triangle winding faces accessible volume");
        }
        std::vector<Surfaces> materials(19 * 27);
        materials[9 * 19 + 8].floor = 3;
        const auto varied = prepare(map, {}, materials);
        check(std::any_of(varied.ranges.begin(), varied.ranges.end(),
                          [](const Range& r) { return r.material == 3; }),
              "Material override retained across chunks");
        rejects([&] { prepare(one, {}, {}, {5, 65536, 64u * 1024 * 1024}); },
                "Reject insufficient quad capacity");
        rejects([&] { prepare(one, {}, {}, {6, 2, 64u * 1024 * 1024}); },
                "Reject insufficient material range capacity");
        rejects([&] { prepare(one, {}, {}, {6, 3, 1}); }, "Reject insufficient CPU bytes");
        rejects([&] { prepare(one, {32, 1, 2}); }, "Reject material outside palette");
        rejects([&] { prepare(one, {}, materials); }, "Reject mismatched material grid");
        MovementState state;
        state.player = {{9, 13}, Heading::North};
        for (unsigned heading = 0; heading < 4; ++heading) {
            state.player.heading = Heading(heading);
            const auto c = camera(state);
            const auto p = project(c, {c.position.x + c.forward.x * 2, .5f, c.position.z + c.forward.z * 2});
            check(std::abs(p.x - 250) < .001f && std::abs(p.y - 282 * .34f) < .001f,
                  "Independent centre/horizon projection");
        }
        DrawList list;
        std::vector<MovementState> samples;
        for (auto action : {Action::Forward, Action::Back, Action::Left, Action::Right, Action::TurnLeft,
                            Action::TurnRight})
            for (unsigned heading = 0; heading < 4; ++heading) {
                Controller controller({{9, 13}, Heading(heading)});
                controller.submit(map, action);
                for (unsigned t = 0; t <= 16; ++t) {
                    const auto s = controller.state();
                    samples.push_back(s);
                    const auto c = camera(s);
                    cull(scene, c, list);
                    check(list.tested <= 49 && list.count <= 49 * 32, "Bounded culling work");
                    // Independent screen-space samples must never be removed with their chunk.
                    for (int z = 1; z < 26; ++z)
                        for (int x = 1; x < 18; ++x) {
                            const auto p = project(c, {x + .5f, 0, z + .5f});
                            if (p.depth > .025f && p.depth < 16 && p.x > 0 && p.x < 500 && p.y > 0 &&
                                p.y < 282) {
                                const auto& chunk = scene.chunks[(z / 8) * scene.columns + x / 8];
                                bool included = false;
                                for (unsigned j = 0; j < list.count; ++j)
                                    included |= list.ranges[j] >= chunk.first &&
                                                list.ranges[j] < chunk.first + chunk.count;
                                check(included, "Interpolated culling never drops projected floor sample");
                            }
                        }
                    controller.update(map, .01f);
                }
            }
        watching = true;
        for (const auto& s : samples)
            cull(scene, camera(s), list);
        watching = false;
        check(allocations == 0, "Camera/culling allocate nothing");
        test::WorldFixture fixture;
        auto publication = world_assets::load(fixture.directory);
        check(publication.textures.size() == 4 && publication.read(0).size() == world_assets::mipBytes(8, 8),
              "Prepared payload roundtrip");
        const auto& actor = publication.appearances.at(0);
        check(actor.clips[0].timing.count == 2 && actor.clips[0].timing.durationMs[1] == 200 &&
                  actor.clips[0].textures[3][1] == 3 && actor.clips[0].width == .4f &&
                  actor.attachments.release[2].x == .25f && actor.cutoff == 15,
              "Prepared clips, directions, widths and attachments roundtrip independently of source files");
        const auto pixels = publication.read(3), alpha = publication.read(3, true);
        check(pixels[0] == 0 && pixels[3] == 0 && alpha[0] == 0 && alpha[1] == 120 && pixels[4] == 94,
              "Cutoff precedes premultiplication and mask retention");
        check(publication.anchors[3].x == .25f && publication.anchors[3].y == .75f,
              "Noncentral anchors retained");
        std::vector<unsigned char> odd(3 * 1 * 4, 255), mask;
        odd[0] = 0;
        odd[4] = 30;
        odd[8] = 60;
        auto mip = world_assets::preparePixels(odd, 3, 1, 0, false, mask);
        check(mip.size() == 16 && mip[12] == 30, "Odd dimension mip includes every texel");
        const auto path = fixture.directory / "metadata.json";
        nlohmann::json original;
        {
            std::ifstream in(path);
            in >> original;
        }
        auto invalid = [&](nlohmann::json j) {
            std::ofstream out(path);
            out << j.dump();
            out.close();
            rejects([&] { world_assets::load(fixture.directory); }, "Malformed publication rejected");
        };
        auto j = original;
        j["appearances"][0]["clips"][0]["textures"][2][1] = UINT16_MAX;
        invalid(j);
        j = original;
        j["format"] = 99;
        invalid(j);
        j = original;
        j["textures"][0]["levels"] = 1;
        invalid(j);
        j = original;
        j["textures"][1]["offset"] = 0;
        invalid(j);
        j = original;
        j["textures"][0]["width"] = UINT32_MAX;
        invalid(j);
        j = original;
        j["textures"][0]["width"] = 8.5;
        invalid(j);
        j = original;
        j["textures"][0]["anchor"][0] = 2;
        invalid(j);
        {
            std::ofstream out(path);
            out << original.dump();
        }
        {
            std::fstream payload(fixture.directory / "textures.bin",
                                 std::ios::in | std::ios::out | std::ios::binary);
            payload.put('x');
        }
        rejects([&] { world_assets::load(fixture.directory); }, "Corrupt texture payload rejected");
        std::cout << "World CPU/publication checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
