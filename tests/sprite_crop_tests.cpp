#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace blobber;
namespace {
void payload() {
    test::WorldFixture fixture;
    constexpr unsigned w = 29, h = 23;
    std::vector<unsigned char> rgba(w * h * 4);
    for (unsigned y = 7; y < 19; ++y)
        for (unsigned x = 8; x < 17; ++x) {
            const auto at = (y * w + x) * 4;
            rgba[at] = static_cast<unsigned char>(x * 7);
            rgba[at + 1] = static_cast<unsigned char>(y * 9);
            rgba[at + 2] = 100;
            rgba[at + 3] = x == 12 ? 0 : (x == 8 ? 64 : 255); // hole and thin translucent edge
        }
    rgba[3] = 15; // Below cutoff must not enlarge the crop.
    {
        world_assets::Writer writer(fixture.directory);
        writer.add("full", w, h, true, rgba, 15, {.2f, .9f});
        writer.add("crop", w, h, true, rgba, 15, {.2f, .9f}, false, true);
        auto empty = rgba;
        for (std::size_t i = 3; i < empty.size(); i += 4)
            empty[i] = 15;
        rejects([&] { writer.add("empty", w, h, true, empty, 15, {}, false, true); },
                "All-cutoff NPC art rejects before publication");
        rejects([&] { writer.add("opaque-crop", w, h, false, rgba, 15, {}, true, true); },
                "Surface textures cannot opt into NPC cropping");
        std::vector<unsigned char> edge(7 * 5 * 4, 255);
        writer.add("edge", 7, 5, true, edge, 15, {}, false, true);
        writer.finish({});
    }
    const auto p = world_assets::load(fixture.directory);
    const auto& t = p.textures[1];
    check(t.canvas.width == w && t.canvas.height == h && t.canvas.x == 6 && t.canvas.y == 5 &&
              t.width == 13 && t.height == 16,
          "Asymmetric crop retains two texels and original canvas");
    check(t.bytes < p.textures[0].bytes && t.alphaBytes < p.textures[0].alphaBytes,
          "Cropping reduces actual stored mip and CPU mask bytes");
    const auto full = p.read(0), trimmed = p.read(1), mask = p.read(1, true);
    for (unsigned y = 0; y < t.height; ++y)
        for (unsigned x = 0; x < t.width; ++x) {
            const auto src = (std::size_t(y + t.canvas.y) * w + x + t.canvas.x) * 4;
            const auto dst = (std::size_t(y) * t.width + x) * 4;
            for (unsigned c = 0; c < 4; ++c)
                check(trimmed[dst + c] == full[src + c], "Crop preserves every retained base texel");
            check(mask[dst / 4] == full[src + 3], "Picking alpha is the same prepared base alpha");
        }
    check(p.anchors[1].x == .2f && p.anchors[1].y == .9f,
          "Anchor outside cropped body remains in logical canvas coordinates");
    check(p.textures[2].width == 7 && p.textures[2].height == 5 && p.textures[2].canvas.x == 0,
          "Edge-touching full artwork retains identity crop and odd dimensions");
    nlohmann::json original;
    const auto file = fixture.directory / "metadata.json";
    {
        std::ifstream in(file);
        in >> original;
    }
    for (const auto& value : {nlohmann::json::array({29, 23, 28, 5}), nlohmann::json::array({0, 23, 0, 0}),
                              nlohmann::json::array({29, 23, -1, 5}), nlohmann::json::array({29, 23, .5, 5}),
                              nlohmann::json::array({29, 23, 6})}) {
        auto j = original;
        j["textures"][1]["canvas"] = value;
        {
            std::ofstream out(file);
            out << j;
        }
        rejects([&] { world_assets::load(fixture.directory); }, "Malformed crop metadata rejects");
    }
    {
        std::ofstream out(file);
        out << original;
    }
    auto retired = original;
    retired["format"] = 2;
    {
        std::ofstream out(file);
        out << retired;
    }
    rejects([&] { world_assets::load(fixture.directory); },
            "Pre-crop publications require an explicit rebuild");
    auto changed = original;
    changed["textures"][1]["canvas"][2] = 5;
    {
        std::ofstream out(file);
        out << changed;
    }
    check(world_assets::load(fixture.directory).fingerprint != p.fingerprint,
          "Binding-only crop changes invalidate publication identity");
}
void geometry() {
    test::VisibilityFixture full(1, 6, false), crop(1, 6, true);
    world_assets::CpuAssets a(full.publication()), b(crop.publication());
    const auto aLease = a.acquire(world_assets::allAssets(a.publication));
    const auto bLease = b.acquire(world_assets::allAssets(b.publication));
    const auto catalog = creationCatalog(true);
    const auto map = test::worldRoom(11, 11);
    MovementState movement;
    movement.player = {{5, 8}, Heading::North};
    const NpcWorld placements(map, movement.player.cell, {"goblin"},
                              {{{0}, {5, 6}, {0, 1}, Heading::South}, {{0}, {5, 5}, {0, 0}, Heading::South}});
    ActorWorld actors(map, catalog, placements);
    NpcAnimationSnapshot animation;
    animation.count = 2;
    animation.world = actors.snapshot().world;
    animation.actorRevision = actors.snapshot().revision;
    auto scene = world_render::prepare(map);
    world_render::VisibilityPipeline original(scene, a), trimmed(scene, b);
    for (unsigned clip = 0; clip < npcAnimationCount; ++clip)
        for (unsigned frame = 0; frame < (clip == 6 ? 1u : 2u); ++frame)
            for (const Cell position : {Cell{5, 8}, Cell{5, 9}, Cell{4, 8}}) {
                movement.player.cell = position;
                animation.frames[0].clip = NpcAnimation(clip);
                animation.frames[0].active = std::uint8_t(frame);
                ++animation.revision;
                const world_render::FrameInput input{map, movement, &actors, &animation};
                original.update(input);
                trimmed.update(input);
                for (int y = 0; y < viewHeight; ++y)
                    for (int x = 0; x < viewWidth; ++x)
                        check(original.picking()->slot(x, y) == trimmed.picking()->slot(x, y),
                              "Cropped and full canvases preserve every CPU picking pixel");
                const auto& of = original.frame();
                const auto& cf = trimmed.frame();
                check(of.count == cf.count, "Cropping preserves visible billboard membership");
                for (unsigned i = 0; i < of.count; ++i) {
                    const auto oa =
                        world_render::attachment(of.camera, of.billboards[i], of.billboards[i].release);
                    const auto ca =
                        world_render::attachment(cf.camera, cf.billboards[i], cf.billboards[i].release);
                    check(oa.position.x == ca.position.x && oa.position.y == ca.position.y &&
                              oa.position.z == ca.position.z &&
                              of.billboards[i].shadowRadius == cf.billboards[i].shadowRadius,
                          "Logical attachments and shadows do not follow per-frame crop centers");
                }
                const auto& ot = original.picking()->targets();
                const auto& ct = trimmed.picking()->targets();
                check(ot.count == ct.count, "Picking target counts remain unchanged");
                for (unsigned i = 0; i < ot.count; ++i)
                    check(ot.anchors[i].badgeX == ct.anchors[i].badgeX &&
                              ot.anchors[i].badgeTop == ct.anchors[i].badgeTop,
                          "Standing and prone badge registration remains in original canvas space");
            }
}
} // namespace
int main() {
    try {
        payload();
        geometry();
        std::cout << "Sprite cropping checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
