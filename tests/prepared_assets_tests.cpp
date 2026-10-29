#include "assets/fingerprint.hpp"
#include "assets/image.hpp"
#include "assets/npc_derivatives.hpp"
#include "assets/npc_loader.hpp"
#include "assets/preparation_profile.hpp"
#include "assets/prepared_assets.hpp"
#include "assets/prepared_binary.hpp"
#include "support/actor_fixtures.hpp"
#include "support/check.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace blobber;
namespace fs = std::filesystem;
using Json = nlohmann::json;
namespace {
struct Directory {
    fs::path path =
        fs::temp_directory_path() /
        ("blobber-bundles-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Directory() { fs::create_directories(path); }
    ~Directory() {
        std::error_code error;
        fs::remove_all(path, error);
    }
};
void write(const fs::path& path, const Json& value) {
    std::ofstream(path) << value.dump(2);
}
void metadataAndDerivatives(const fs::path& root) {
    const Map map({"#####", "#...#", "#...#", "#...#", "#####"});
    Json definition{
        {"id", "future-npc"},
        {"width", .4},
        {"height", .6},
        {"views",
         {{"front", "pose.png"}, {"back", "pose.png"}, {"left", "pose.png"}, {"right", "pose.png"}}}};
    write(root / "type.json", definition);
    write(root / "level.json", {{"definitions", {"type.json"}}, {"instances", Json::array()}});
    auto metadata = loadNpcMetadata(root / "level.json", map, {2, 2});
    check(metadata.timings.size() == 1 && metadata.timings[0][0].count == 1,
          "Metadata loading does not require source art");
    rejects([&] { loadNpcs(root / "level.json", map, {2, 2}); },
            "Offline source loader still requires images");
    OwnedImage image(GenImageColor(8, 12, Color{200, 60, 20, 255}));
    auto* pixels = static_cast<Color*>(image.value.data);
    pixels[0] = {0, 255, 255, 0};
    check(ExportImage(image.value, (root / "pose.png").string().c_str()), "Export owned fixture");
    const auto generated = prepareNpcDerivatives(root / "level.json", root / "derived");
    auto sources = loadNpcs(generated, map, {2, 2});
    check(sources.definitionFingerprint == metadata.definitionFingerprint,
          "Relocation preserves definition identity");
    const auto& frame = sources.appearances[0].views[0];
    check(frame.width == 4 && frame.height == 6 && frame.pixels[0].r == 200 && frame.pixels[0].g == 60 &&
              frame.pixels[0].a == 191,
          "Future NPC inherits half-size alpha-aware source filtering without per-type setting");
    definition["width"] = .3;
    write(root / "type.json", definition);
    check(loadNpcMetadata(root / "level.json", map, {2, 2}).definitionFingerprint !=
              metadata.definitionFingerprint,
          "Definition edits invalidate bundle even when timings are unchanged");
    definition["views"]["front"] = {{"image", "absent.png"}, {"anchor", {2, 1}}};
    write(root / "type.json", definition);
    rejects([&] { loadNpcMetadata(root / "level.json", map, {2, 2}); },
            "Metadata-only path still validates anchors");
    const Json profile{
        {"version", 1}, {"npc_filter", "area"}, {"npc_resolution", .5}, {"surface_filter", "nearest"}};
    write(root / "profile.json", profile);
    const auto hash = loadPreparationProfile(root / "profile.json");
    std::ofstream(root / "profile.json") << profile.dump();
    check(loadPreparationProfile(root / "profile.json") == hash, "Profile hash ignores formatting");
    auto bad = profile;
    bad["surface_filter"] = "area";
    write(root / "profile.json", bad);
    rejects([&] { loadPreparationProfile(root / "profile.json"); }, "Unsupported surface profile rejected");
    fs::create_directories(root / "author/sheets");
    check(ExportImage(image.value, (root / "author/sheets/pose.png").string().c_str()), "Sheet fixture");
    write(root / "author/sheets.json", Json::array({{{"name", "pose"},
                                                     {"split_x", 4},
                                                     {"scale", 1},
                                                     {"pivot_x", {2, 2}},
                                                     {"canvas_width", 8},
                                                     {"canvas_height", 12},
                                                     {"anchor_x", 4},
                                                     {"anchor_y", 12}}}));
    definition["source_sheets"] = "author/sheets.json";
    for (auto& spec : definition["views"])
        spec = "author/frames/pose-0.png";
    write(root / "type.json", definition);
    prepareNpcDerivatives(root / "level.json", root / "sheets-derived");
    write(root / "author/sheets.json", Json::array());
    rejects([&] { prepareNpcDerivatives(root / "level.json", root / "sheets-derived"); },
            "Removing a registered sheet cannot reuse stale frames from a previous bake");
}
std::vector<unsigned char> read(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
void put64(std::vector<unsigned char>& bytes, std::size_t offset, std::uint64_t value) {
    for (int i = 0; i < 8; ++i)
        bytes.at(offset + i) = static_cast<unsigned char>(value >> (8 * i));
}
void file(const fs::path& path, const std::vector<unsigned char>& bytes) {
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}
void corruptions(const fs::path& root, const std::vector<NpcTimings>& timings) {
    const auto original = read(root / "npcs.bin");
    const auto load = [&] { return loadPreparedNpcs(root / "bad.bin", timings, 17, 29); };
    // Valid checksums on semantic corruption ensure range validation is exercised.
    for (const auto offset : {std::size_t(8), std::size_t(52), std::size_t(60), std::size_t(84),
                              std::size_t(92), std::size_t(100), std::size_t(700), std::size_t(724)}) {
        auto bytes = original;
        put64(bytes, offset, ~std::uint64_t{0});
        auto hash = fingerprintBasis;
        fingerprintBytes(hash, bytes.data(), bytes.size() - 8);
        put64(bytes, bytes.size() - 8, hash);
        file(root / "bad.bin", bytes);
        rejects(load,
                "Corrupt version/profile/definitions/count/NaN/timing/index rejected despite valid checksum");
    }
    auto bytes = original;
    bytes[bytes.size() - 20] ^= 1;
    file(root / "bad.bin", bytes);
    rejects(load, "Payload checksum rejects altered pixel/kernel");
    for (const auto size : {std::size_t(0), std::size_t(75), original.size() / 2, original.size() - 1}) {
        file(root / "bad.bin", {original.begin(), original.begin() + size});
        rejects(load, "Truncated bundle rejected");
    }
    bytes = original;
    bytes.push_back(0);
    file(root / "bad.bin", bytes);
    rejects(load, "Trailing bytes rejected");
    rejects([&] { loadPreparedNpcs(root / "npcs.bin", timings, 17, 29, {1024, 512u * 1024u * 1024u}); },
            "Resident budget preflight");
    rejects([&] { loadPreparedNpcs(root / "npcs.bin", timings, 17, 29, {512u * 1024u * 1024u, 1024}); },
            "Loading budget preflight");
    rejects([&] { loadPreparedNpcs(root / "absent.bin", timings, 17, 29); }, "Missing bundle does not bake");
}
void roundTrip(const fs::path& root) {
    {
        auto transparent = testNpcAppearance();
        for (auto& image : transparent.views)
            std::fill(image.pixels.begin(), image.pixels.end(), BLANK);
        const PreparedNpcs empty({transparent});
        check(empty.stationaryPixelBytes() == 0 && empty.travelPixelBytes() == 0 &&
                  empty.preparedPixelBytes() > 4096,
              "Empty crops retain independent floor shadows without allocating body pixels");
        savePreparedNpcs(root / "empty.bin", empty, {npcTimings(transparent)}, 17, 29);
        const auto restored = loadPreparedNpcs(root / "empty.bin", {npcTimings(transparent)}, 17, 29);
        check(restored.preparedPixelBytes() == empty.preparedPixelBytes(),
              "Empty crops survive bundle validation");
    }
    auto appearance = testProneAppearance();
    appearance.sampling = ImageSampling::Area;
    // Asymmetric padding and off-center pivot make crop offsets observable.
    for (auto& clip : appearance.animations)
        for (auto& side : clip.views)
            for (auto& image : side) {
                image.anchorX = .375f;
                for (int y = 0; y < image.height; ++y)
                    for (int x = 0; x < image.width; ++x)
                        if (x < 2 || y < 2)
                            image.pixels[y * image.width + x].a = 0;
            }
    const std::vector<NpcTimings> timings{npcTimings(appearance)};
    auto prepared = PreparedNpcs({appearance});
    savePreparedNpcs(root / "npcs.bin", prepared, timings, 17, 29);
    savePreparedNpcs(root / "repeat.bin", prepared, timings, 17, 29);
    check(fingerprintFile(root / "npcs.bin") == fingerprintFile(root / "repeat.bin"),
          "Deterministic NPC bundle");
    auto loaded = loadPreparedNpcs(root / "npcs.bin", timings, 17, 29);
    check(loaded.preparedPixelBytes() == prepared.preparedPixelBytes(),
          "Roundtrip owns exact compact pixels");
    corruptions(root, timings);
    auto scene = prepareScene(testGreaseMaterials());
    rejects([&] { prepareScene(testMaterials(), {1, 1}); },
            "Offline scene preflight rejects inadequate budget");
    savePreparedScene(root / "scene.bin", scene, 29);
    rejects([&] { loadPreparedScene(root / "scene.bin", 28); }, "Scene profile mismatch");
    rejects([&] { loadPreparedScene(root / "scene.bin", 29, {1, 1}); },
            "Scene bound before fixed frame allocation");
    auto sceneLoaded = loadPreparedScene(root / "scene.bin", 29);
    rejects([&] { validatePreparedPublication(root, 29, 17); }, "An interrupted bake has no publication");
    Json publication{{"format", prepared_io::formatVersion},
                     {"renderer_revision", prepared_io::rendererRevision},
                     {"profile", 29},
                     {"definitions", 17}};
    for (const auto* name : {"scene.bin", "npcs.bin"})
        publication["outputs"][name] = {{"bytes", fs::file_size(root / name)},
                                        {"checksum", preparedBundleChecksum(root / name)}};
    write(root / "metadata.json", publication);
    validatePreparedPublication(root, 29, 17);
    publication["outputs"]["npcs.bin"]["checksum"] = 0;
    write(root / "metadata.json", publication);
    rejects([&] { validatePreparedPublication(root, 29, 17); }, "Mixed generations cannot be published");
    const SceneRenderer direct(std::move(scene), std::move(prepared));
    const SceneRenderer restored(std::move(sceneLoaded), std::move(loaded));
    const Map map({"#########", "#.......#", "#...#...#", "#.......#", "#.......#", "#.......#", "#.......#",
                   "#.......#", "#########"});
    auto actors = testActors(map, {4, 6}, {"test"}, {{{0}, {4, 4}, {1, 0}, Heading::South}});
    NpcAnimator animation(actors, timings);
    Frame expected, actual;
    PickingBuffer expectedPick, actualPick;
    for (std::size_t clip = 0; clip < npcAnimationCount; ++clip)
        for (unsigned frame = 0; frame < timings[0][clip].count; ++frame)
            for (const float offset : {0.f, .2f}) {
                auto snapshot = animation.snapshot();
                snapshot.frames[0].clip = static_cast<NpcAnimation>(clip);
                snapshot.frames[0].active = frame;
                snapshot.frames[0].offsetX = offset;
                const Player player{{4, 6}, Heading::North};
                direct.compose(map, player, expected, &actors, &snapshot, &expectedPick);
                restored.compose(map, player, actual, &actors, &snapshot, &actualPick);
                check(same(expected, actual),
                      "Loaded reaction and continuous travel pixels match direct bake");
                for (int y = 0; y < viewHeight; ++y)
                    for (int x = 0; x < viewWidth; ++x)
                        check(expectedPick.slot(x, y) == actualPick.slot(x, y),
                              "Loaded body picking remains pixel exact");
            }
    SpellArea ground;
    ground.ground.areas[0] = {{1, 1}, {4, 5}, {3}, 10, 60000, true};
    for (int heading = 0; heading < 4; ++heading)
        for (unsigned time : {0, 60, 140, 300}) {
            animation.setTime(time);
            Player player{{4, 6}, static_cast<Heading>(heading)};
            direct.compose(map, player, expected, &actors, &animation.snapshot());
            restored.compose(map, player, actual, &actors, &animation.snapshot());
            check(same(expected, actual), "Loaded bundle preserves stationary pixels/occlusion/headings");
            auto to = player;
            to.cell = to.cell + right(player.heading);
            for (int phase = 0; phase <= 8; ++phase) {
                direct.composeStrafe(map, player, to, phase / 8.f, expected, &actors, &animation.snapshot(),
                                     nullptr, nullptr, &ground);
                restored.composeStrafe(map, player, to, phase / 8.f, actual, &actors, &animation.snapshot(),
                                       nullptr, nullptr, &ground);
                check(same(expected, actual), "Loaded bundle preserves every strafe phase and endpoint");
            }
        }
}
void distinctAppearances(const fs::path& root) {
    auto melee = testReactionAppearance();
    auto archer = testArcherAppearance();
    melee.sampling = archer.sampling = ImageSampling::Area;
    const std::vector<NpcTimings> timings{npcTimings(melee), npcTimings(archer)};
    PreparedNpcs bank({melee, archer});
    savePreparedNpcs(root / "roles.bin", bank, timings, 41, 29);
    auto loaded = loadPreparedNpcs(root / "roles.bin", timings, 41, 29);
    auto wrongTimings = timings;
    std::swap(wrongTimings[0], wrongTimings[1]);
    rejects([&] { loadPreparedNpcs(root / "roles.bin", wrongTimings, 41, 29); },
            "Swapped appearance clip tables cannot load the wrong bank");
    const SceneRenderer direct(loadPreparedScene(root / "scene.bin", 29), std::move(bank));
    const SceneRenderer restored(loadPreparedScene(root / "scene.bin", 29), std::move(loaded));
    const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
    auto actors = testActors(map, {3, 5}, {"near", "far"},
                             {{{0}, {2, 3}, {1, 1}, Heading::South}, {{1}, {4, 3}, {0, 1}, Heading::South}});
    NpcAnimator animator(actors, timings);
    actors.attack(actors.id(0), 0);
    actors.attack(actors.id(1), 0, true);
    Frame expected, actual;
    PickingBuffer expectedPick, actualPick;
    const Player player{{3, 5}, Heading::North};
    auto compare = [&](std::uint64_t time) {
        animator.setTime(time);
        direct.compose(map, player, expected, &actors, &animator.snapshot(), &expectedPick);
        restored.compose(map, player, actual, &actors, &animator.snapshot(), &actualPick);
        check(same(expected, actual), "Two distinct loaded appearances preserve rendered pixels");
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x)
                check(expectedPick.slot(x, y) == actualPick.slot(x, y),
                      "Both loaded appearances preserve body ownership in picking");
    };
    compare(0);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Attack &&
              animator.snapshot().frames[1].clip == NpcAnimation::RangedAttack &&
              actualPick.targets().count == 2,
          "Each role selects its own attack clip and both remain visible targets");
    for (int direction = 0; direction < 4; ++direction) {
        actors.face(actors.id(1), static_cast<Heading>(direction));
        for (const auto time : {599u, 600u, 999u, 1000u}) {
            compare(time);
            const auto& frame = animator.snapshot().frames[1];
            check(frame.clip == (time < 1000 ? NpcAnimation::RangedAttack : NpcAnimation::Idle),
                  "Bow-only clip ends in its own idle without a melee clip");
            if (time < 1000)
                check(frame.active == (time >= 600), "Bow release uses the archer's own timing");
        }
    }
    check(actors.walk(actors.id(1), {3, 3}, {0, 1}, player.cell, 1100).changed,
          "Archer fixture starts a committed travel segment");
    compare(1400);
    actors.damage(actors.id(1), 1, 1400);
    compare(1550);
    check(animator.snapshot().frames[1].clip == NpcAnimation::Hurt,
          "Moving bow-only appearance selects its own hurt reaction");
    actors.damage(actors.id(1), 100, 1800);
    for (const auto time : {1900u, 2000u, 2200u, 2400u, 2999u, 3000u}) {
        compare(time);
        check(actualPick.targets().count == 1, "Archer corpse never becomes a living target");
    }
    check(!animator.snapshot().frames[1].visible && !actors.occupied({3, 3}),
          "Archer corpse expires independently of live melee actor and occupancy");
}
} // namespace
int main() {
    try {
        SetTraceLogLevel(LOG_WARNING);
        Directory root;
        metadataAndDerivatives(root.path);
        roundTrip(root.path);
        distinctAppearances(root.path);
        std::cout << "Prepared asset checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
