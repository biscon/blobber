#include "assets/image.hpp"
#include "assets/map_icons.hpp"
#include "assets/portrait_atlas.hpp"
#include "platform/gpu_resources.hpp"
#include "platform/window.hpp"
#include "support/check.hpp"
#include "ui/hud_painter.hpp"
#include "ui/map_model.hpp"
#include "ui/equipment_art.hpp"
#include "rlgl.h"
#include <cmath>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string_view>
using namespace blobber;
namespace {
struct FixtureDirectory {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("blobber-native-ui-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    FixtureDirectory() {
        path /= "ui";
        std::filesystem::create_directories(path.parent_path() / "prepared");
        std::filesystem::create_directories(path / "spells");
        std::filesystem::create_directories(path / "items");
        std::filesystem::create_directories(path / "camp");
        std::filesystem::create_directories(path / "map");
    }
    ~FixtureDirectory() {
        std::error_code e;
        std::filesystem::remove_all(path.parent_path(), e);
    }
};
void nativeUi() {
    FixtureDirectory fixture;
    constexpr const char* names[]{"alden",
                                  "brunna",
                                  "merrin",
                                  "lethiel",
                                  "goblin",
                                  "melee",
                                  "defend",
                                  "ranged",
                                  "spell",
                                  "backpack",
                                  "aid",
                                  "spells/sacred_flame",
                                  "spells/spare_the_dying",
                                  "spells/bless",
                                  "spells/bane",
                                  "spells/shield_of_faith",
                                  "spells/cure_wounds",
                                  "spells/guiding_bolt",
                                  "spells/inflict_wounds",
                                  "spells/fire_bolt",
                                  "spells/acid_splash",
                                  "spells/mage_armor",
                                  "spells/magic_missile",
                                  "spells/burning_hands",
                                  "spells/thunderwave",
                                  "spells/second_wind",
                                  "spells/arcane_recovery",
                                  "spells/dismiss_concentration",
                                  "class_features",
                                  "items/longsword",
                                  "items/chain_mail",
                                  "items/shield",
                                  "items/mace",
                                  "items/scale_mail",
                                  "items/holy_symbol",
                                  "items/shortsword",
                                  "items/leather",
                                  "items/shortbow",
                                  "items/arrow",
                                  "items/dagger",
                                  "items/spellbook",
                                  "items/arcane_focus",
                                  "items/healing_potion",
                                  "items/mana_potion",
                                  "items/silhouette",
                                  "items/loot-sheet",
                                  "camp/bonfire-sheet",
                                  "rest",
                                  "spells/ray_of_frost",
                                  "spells/shocking_grasp",
                                  "spells/poison_spray",
                                  "spells/sleep",
                                  "spells/false_life",
                                  "spells/grease",
                                  "spells/color_spray",
                                  "spells/guidance",
                                  "spells/resistance",
                                  "spells/sanctuary",
                                  "spells/hold_person",
                                  "items/quarterstaff",
                                  "travel",
                                  "skeleton",
                                  "bugbear",
                                  "giant-rat",
                                  "zombie",
                                  "goblin-shaman"};
    for (unsigned i = 0; i < std::size(names); ++i) {
        OwnedImage image(GenImageColor(i < 4     ? 216
                                       : i >= 29 ? 168
                                                 : 108,
                                       i < 4     ? 252
                                       : i >= 29 ? 168
                                                 : 108,
                                       GREEN));
        for (int y = 0; y < image.value.height; ++y)
            for (int x = 0; x < image.value.width; ++x)
                ImageDrawPixel(&image.value, x, y,
                               std::string_view(names[i]) == "travel" ? RED
                               : i < 4                                ? (x < 32 || x >= 184 ? RED : GREEN)
                                                                      : (x % 2 ? WHITE : BLACK));
        check(ExportImage(image.value, (fixture.path / (std::string(names[i]) + ".png")).u8string().c_str()),
              "Export generated UI fixture");
    }
    // Own every icon consumed by UiArt, including equipment added after this fixture.
    // These synthetic inputs stay independent of the game's editable artwork.
    for (const auto* id : equipmentIconIds) {
        OwnedImage icon(GenImageColor(168, 168, WHITE));
        check(ExportImage(icon.value, (fixture.path / "items" / (std::string(id) + ".png")).u8string().c_str()),
              "Export owned equipment icon fixture");
    }
    for (bool large : {false, true}) {
        OwnedImage atlas(
            GenImageColor(portraitAtlasWidth, large ? largePortraitHeight : smallPortraitHeight, GREEN));
        check(ExportImage(atlas.value, (fixture.path.parent_path() / "prepared" /
                                        (large ? "portraits-large.qoi" : "portraits-small.qoi"))
                                           .u8string()
                                           .c_str()),
              "Export test-owned prepared portrait atlas");
    }
    {
        OwnedImage atlas(GenImageColor(2048, 2048, BLUE));
        check(
            ExportImage(atlas.value, (fixture.path.parent_path() / "prepared/creator.qoi").u8string().c_str()),
            "Generated creator atlas");
    }
    {
        OwnedImage icons(GenImageColor(128, 64, BLANK));
        for (int i = 0; i < mapIconCount; ++i) {
            const int x = (i % 4) * 32, y = (i / 4) * 32;
            ImageDrawRectangle(&icons.value, x + 8, y + 8, 16, 16, i == 0 ? RED : i == 1 ? WHITE : ORANGE);
            if (i == 0)
                ImageDrawRectangle(&icons.value, x + 8, y + 8, 8, 8, GREEN);
        }
        check(ExportImage(icons.value, (fixture.path / "map/markers-v1.png").u8string().c_str()),
              "Export owned map marker fixture");
    }
    for (const auto& spec :
         {std::make_pair("se02/portraits.png", std::pair<int, int>{1024, 512}),
          std::make_pair("se03/portraits.png", std::pair<int, int>{1024, 1024}),
          std::make_pair("combat-feedback/backdrops.png", std::pair<int, int>{768, 128})}) {
        const auto path = fixture.path.parent_path() / "effects" / spec.first;
        std::filesystem::create_directories(path.parent_path());
        OwnedImage atlas(GenImageColor(spec.second.first, spec.second.second, WHITE));
        check(ExportImage(atlas.value, path.u8string().c_str()), "Export owned effect atlas fixture");
    }
    GpuResources resources;
    UiArt art(resources, fixture.path);
    OwnedImage extra(GenImageColor(2, 2, WHITE));
    const auto viewport = resources.createTexture(extra.value), menu = resources.createTexture(extra.value);
    check(bool(viewport) && bool(menu), "Prepared UI artwork plus viewport and menu fit the registry");
    const auto overlayFile = (fixture.path / "overlay.png").u8string();
    OwnedImage overlayImage(GenImageColor(8, 4, BLANK));
    ImageDrawRectangle(&overlayImage.value, 2, 0, 4, 4, GREEN);
    check(ExportImage(overlayImage.value, overlayFile.c_str()), "Export owned overlay fixture");
    const auto baseline = resources.counts();
    art.prepareDialogue({overlayFile});
    check(resources.counts().textures == baseline.textures + 1, "Overlay acquires one texture");
    resources.failAfter(1);
    rejects([&] { art.prepareDialogue({overlayFile, overlayFile}); }, "Staged overlay GPU failure rejects");
    resources.failAfter(-1);
    check(resources.counts().textures == baseline.textures + 1 &&
              resources.counts().bytes == baseline.bytes + 8 * 4 * 4,
          "Failed replacement rolls back new leases and preserves old set");
    rejects([&] { art.prepareDialogue({overlayFile + ".missing"}); }, "Missing overlay rejects");
    rejects([&] { art.prepareDialogue(std::vector<std::string>(17, overlayFile)); },
            "Overlay texture capacity rejects");
    const auto corrupt = (fixture.path / "corrupt.png").u8string();
    std::filesystem::copy_file(overlayFile, corrupt);
    { // Keep a valid signature but make IHDR width 65536; reject before decoding.
        std::fstream file(corrupt, std::ios::in | std::ios::out | std::ios::binary);
        const char wide[]{0, 1, 0, 0};
        file.seekp(16);
        file.write(wide, 4);
    }
    rejects([&] { art.prepareDialogue({corrupt}); }, "Oversized PNG header rejects before decode");
    {
        std::ofstream file(corrupt, std::ios::binary);
        file << "not a PNG";
    }
    rejects([&] { art.prepareDialogue({corrupt}); }, "Corrupt PNG rejects");
    const auto large = (fixture.path / "large.png").u8string();
    {
        OwnedImage budget(GenImageColor(2048, 2048, GREEN));
        check(ExportImage(budget.value, large.c_str()), "Export owned budget fixture");
    }
    rejects([&] { art.prepareDialogue({large, large, overlayFile}); }, "Decoded overlay byte budget rejects");
    check(resources.counts().textures == baseline.textures + 1, "Budget failure releases staged textures");
    const HudPainter painter(nullptr, &art);
    const auto target = resources.createTarget(400, 400);
    const auto transfers = resources.transfers().uploads;
    BeginTextureMode(resources.target(target));
    ClearBackground(MAGENTA);
    art.dialogueOverlay({0}, {0, 0, 40, 40});
    art.dialogueOverlay({}, {0, 0, 40, 40}); // invalid ID is harmless
    EndTextureMode();
    {
        OwnedImage pixels(LoadImageFromTexture(resources.target(target).texture));
        ImageFlipVertical(&pixels.value);
        const auto letterbox = GetImageColor(pixels.value, 60, 10);
        const auto transparent = GetImageColor(pixels.value, 5, 60);
        const auto center = GetImageColor(pixels.value, 60, 60);
        check(letterbox.r > 240 && letterbox.b > 240 && transparent.r > 240 && transparent.b > 240,
              "Overlay aspect fit and transparent margins preserve the scene");
        check(center.g > 200 && center.r < 50, "Prepared overlay draws at native UI scale");
    }
    check(resources.transfers().uploads == transfers, "Overlay drawing never uploads textures");
    art.clearDialogue();
    art.clearDialogue();
    check(resources.counts().textures == baseline.textures, "Overlay teardown is complete and idempotent");

    BeginTextureMode(resources.target(target));
    ClearBackground(MAGENTA);
    painter.picture(5, {0, 0, 36, 36});
    painter.picture(29, {40, 80, 56, 56});
    painter.picture(0, {0, 40, 30, 84});
    // A wide icon destination must fit rather than stretch the square artwork.
    painter.picture(6, {40, 0, 60, 36});
    painter.picture(UiArt::travelIcon, {100, 0, 16, 13});
    painter.overlayBox({100, 40, 20, 20});
    painter.fill({101, 41, 18, 18}, RED);
    painter.overlayBox({100, 40, 20, 20});
    art.creator(10, {100, 80, 25, 25});
    art.mapMarker(MapMarker::ClosedDoor, {0, 110, 12, 12});
    art.mapMarker(MapMarker::OpenDoor, {16, 110, 12, 12});
    art.mapMarker(MapMarker::Party, {32, 110, 12, 12}, Heading::East);
    EndTextureMode();
    OwnedImage result(LoadImageFromTexture(resources.target(target).texture));
    ImageFlipVertical(&result.value);
    const auto closed = GetImageColor(result.value, 18, 348), open = GetImageColor(result.value, 66, 348);
    check(closed.r > 240 && closed.g > 240 && closed.b > 240 && open.r > 240 && open.g > 100 &&
              open.g < 190 && open.b < 20,
          "Closed and open markers draw their own prepared raster regions");
    const auto rotated = GetImageColor(result.value, 123, 339), other = GetImageColor(result.value, 105, 357);
    check(rotated.g > 200 && rotated.r < 50 && other.r > 200 && other.g < 150,
          "Party heading rotates the prepared texture about its cell center");
    for (int x = 10; x < 100; ++x) {
        const auto c = GetImageColor(result.value, x, 50);
        check(x % 2 ? c.r > 240 : c.r < 15, "One-native-pixel source detail survives the UI atlas and draw");
    }
    const auto creatorPixel = GetImageColor(result.value, 330, 270);
    const auto travelPixel = GetImageColor(result.value, 320, 20);
    check(travelPixel.r > 200 && travelPixel.g < 80 && travelPixel.b < 80,
          "Travel icon resolves to its own atlas region through the native HUD pipeline");
    check(creatorPixel.b > 200 && creatorPixel.r < 15, "Large creator atlas draws through native pipeline");
    const auto left = GetImageColor(result.value, 0, 50), right = GetImageColor(result.value, 107, 50);
    check(left.r < 15 && left.b < 15 && right.r > 240 && right.g > 240,
          "Atlas edge padding does not bleed neighboring/blank texels");
    const auto margin = GetImageColor(result.value, 125, 50);
    check(margin.r == MAGENTA.r && margin.b == MAGENTA.b, "Icons aspect-fit inside wide destinations");
    for (int x : {0, 45, 89}) {
        const auto c = GetImageColor(result.value, x, 200);
        check(c.g > 200 && c.r < 15, "Portrait crop fills destination without stretching edge bands");
    }
    for (int x = 130; x < 278; ++x) {
        const auto c = GetImageColor(result.value, x, 260);
        check(x % 2 ? c.r > 240 : c.r < 15, "Item atlas preserves native single-pixel detail");
    }
    const auto covered = GetImageColor(result.value, 315, 135);
    check(covered.r == hudInk::panel.r && covered.g == hudInk::panel.g,
          "Opaque native modal covers previously drawn content");
    const auto edge = GetImageColor(result.value, 300, 130);
    const auto inset = GetImageColor(result.value, 301, 130);
    check(edge.r == hudInk::gold.r && inset.r == hudInk::panel.r, "Native border is one pixel, not three");
}
void depthCopy() {
    GpuResources resources;
    GpuLease<TargetHandle> normal(resources, resources.createTarget(32, 32));
    GpuLease<TargetHandle> sampled(resources, resources.createTarget(32, 32, true));
    using ClearDepth = void (*)(double);
    using Clear = void (*)(unsigned);
    using Read = void (*)(int, int, int, int, unsigned, unsigned, void*);
    const auto depth = reinterpret_cast<ClearDepth>(rlGetProcAddress("glClearDepth"));
    const auto clear = reinterpret_cast<Clear>(rlGetProcAddress("glClear"));
    const auto read = reinterpret_cast<Read>(rlGetProcAddress("glReadPixels"));
    check(depth && clear && read, "OpenGL depth readback is available");
    const auto verify = [&](TargetHandle from, TargetHandle to, double value) {
        rlEnableFramebuffer(resources.target(from).id);
        depth(value);
        clear(0x00000100); // GL_DEPTH_BUFFER_BIT
        rlEnableFramebuffer(resources.target(to).id);
        depth(1 - value);
        clear(0x00000100);
        resources.copyDepth(from, to);
        rlEnableFramebuffer(resources.target(to).id);
        float actual = -1;
        read(16, 16, 1, 1, 0x1902, 0x1406, &actual); // GL_DEPTH_COMPONENT, GL_FLOAT
        check(std::abs(actual - value) < 1e-6, "Copied depth survives renderbuffer/texture transfer");
        resources.finishPreparation();
    };
    verify(normal.get(), sampled.get(), .25);
    verify(sampled.get(), normal.get(), .625);
    depth(1);
    rlDisableFramebuffer();
    normal.reset();
    sampled.reset();
    check(resources.counts().targets == 0 && resources.counts().bytes == 0, "Depth targets release all owned storage");
}
} // namespace
int main(int argc, char** argv) {
    try {
        const Window window(320, 240, true);
        check(IsWindowState(FLAG_WINDOW_HIDDEN), "GPU diagnostics use a hidden window");
        depthCopy();
        if (argc == 2 && std::string_view(argv[1]) == "--depth-copy") {
            std::cout << "Bidirectional depth copy, readback and resource cleanup passed\n";
            return 0;
        }
        check(argc == 1, "Usage: blobber_gpu_smoke [--depth-copy]");
        nativeUi();
        GpuResources resources;
        rejects([&] { resources.createTarget(0, 32); }, "Invalid target size rejected");
        rejects([&] { resources.createTexture({}); }, "Invalid texture image rejected");
        OwnedImage image(GenImageColor(8, 8, RED));
        const auto texture = resources.createTexture(image.value);
        UpdateTexture(resources.texture(texture), image.value.data);
        check(resources.destroy(texture) && !resources.destroy(texture), "Texture destruction is once-only");
        const auto replacement = resources.createTexture(image.value);
        rejects([&] { resources.texture(texture); }, "Stale GPU texture rejected after reuse");
        const auto target = resources.createTarget(32, 32);
        BeginTextureMode(resources.target(target));
        ClearBackground(BLACK);
        DrawTexture(resources.texture(replacement), 0, 0, WHITE);
        EndTextureMode();
        {
            OwnedImage readback(LoadImageFromTexture(resources.target(target).texture));
            check(readback.value.data != nullptr, "GPU readback succeeds");
        }
        check(resources.destroy(target), "Target released");
        resources.createTarget(32, 32);
        rejects([&] { resources.target(target); }, "Stale GPU target rejected after reuse");
        for (unsigned i = 1; i < gpuTargetCapacity; ++i)
            resources.createTarget(32, 32);
        rejects([&] { resources.createTarget(32, 32); }, "Target capacity fails before allocation");
        for (unsigned i = 1; i < gpuTextureCapacity; ++i)
            resources.createTexture(image.value);
        rejects([&] { resources.createTexture(image.value); }, "Texture capacity fails before allocation");
        BeginDrawing();
        ClearBackground(BLACK);
        EndDrawing();
        std::cout << "GPU lifecycle checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
