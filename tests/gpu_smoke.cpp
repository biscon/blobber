#include "assets/image.hpp"
#include "platform/gpu_resources.hpp"
#include "platform/window.hpp"
#include "support/check.hpp"
#include "ui/hud_painter.hpp"
#include <chrono>
#include <iostream>
using namespace blobber;
namespace {
struct FixtureDirectory {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("blobber-native-ui-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    FixtureDirectory() {
        std::filesystem::create_directories(path / "spells");
        std::filesystem::create_directories(path / "items");
    }
    ~FixtureDirectory() {
        std::error_code e;
        std::filesystem::remove_all(path, e);
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
                                  "items/silhouette"};
    for (unsigned i = 0; i < 45; ++i) {
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
                               i < 4 ? (x < 32 || x >= 184 ? RED : GREEN) : (x % 2 ? WHITE : BLACK));
        check(ExportImage(image.value, (fixture.path / (std::string(names[i]) + ".png")).string().c_str()),
              "Export generated UI fixture");
    }
    GpuResources resources;
    const UiArt art(resources, fixture.path);
    const HudPainter painter(nullptr, &art);
    const auto target = resources.createTarget(400, 400);
    BeginTextureMode(resources.target(target));
    ClearBackground(MAGENTA);
    painter.picture(5, {0, 0, 36, 36});
    painter.picture(29, {40, 80, 56, 56});
    painter.picture(0, {0, 40, 30, 84});
    // A wide icon destination must fit rather than stretch the square artwork.
    painter.picture(6, {40, 0, 60, 36});
    painter.overlayBox({100, 40, 20, 20});
    painter.fill({101, 41, 18, 18}, RED);
    painter.overlayBox({100, 40, 20, 20});
    EndTextureMode();
    OwnedImage result(LoadImageFromTexture(resources.target(target).texture));
    ImageFlipVertical(&result.value);
    for (int x = 10; x < 100; ++x) {
        const auto c = GetImageColor(result.value, x, 50);
        check(x % 2 ? c.r > 240 : c.r < 15, "One-native-pixel source detail survives the UI atlas and draw");
    }
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
} // namespace
int main() {
    try {
        const Window window(320, 240, true);
        check(IsWindowState(FLAG_WINDOW_HIDDEN), "GPU diagnostics use a hidden window");
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
        for (int i = 0; i < 3; ++i)
            resources.createTarget(32, 32);
        rejects([&] { resources.createTarget(32, 32); }, "Target capacity fails before allocation");
        for (int i = 0; i < 7; ++i)
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
