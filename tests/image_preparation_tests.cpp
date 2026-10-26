#include "assets/fingerprint.hpp"
#include "assets/image.hpp"
#include "assets/npc_sheet.hpp"
#include "render/image_sampling.hpp"
#include "support/check.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>

using namespace blobber;
namespace fs = std::filesystem;
using Json = nlohmann::json;
namespace {
void sampling() {
    const Color colors[]{{255, 0, 0, 255}, {0, 255, 255, 0}, {0, 255, 255, 0}, {0, 255, 255, 0}};
    AreaSampler sampler(colors, 2, 2, 2);
    const auto average = sampler.sample(0, 0, 2, 2);
    check(average.r == 255 && average.g == 0 && average.b == 0 && average.a == 64,
          "Transparent hidden RGB cannot create dark or colored fringes");
    check(sampler.sample(-1, 0, 1, 1).a == 128, "Outside the source contributes transparent coverage");
    check(sampler.sample(.25, .25, .75, .75).a == 255, "Subpixel integral preserves solid coverage");
    check(sampler.sample(4, 4, 5, 5).a == 0, "Fully outside footprints are transparent");
    rejects([&] { sampler.sample(1, 0, 0, 1); }, "Negative footprint rejected");
    rejects([&] { sampler.sample(0, 0, std::numeric_limits<double>::infinity(), 1); },
            "Infinite footprint rejected");
    const Color faint[]{{255, 0, 0, 15}, {0, 0, 255, 255}};
    AreaSampler cutoff(faint, 2, 1, 2, 15);
    const auto cleaned = cutoff.sample(0, 0, 2, 1);
    check(cleaned.r == 0 && cleaned.b == 255 && cleaned.a == 128, "Cutoff precedes premultiplied filtering");

    // Independent direct box integration, including fractional bounds and stride padding.
    std::vector<Color> pixels(11 * 7);
    for (int y = 0; y < 7; ++y)
        for (int x = 0; x < 11; ++x)
            pixels[y * 11 + x] = {static_cast<unsigned char>(x * 23), static_cast<unsigned char>(y * 37),
                                  static_cast<unsigned char>((x + y) * 13),
                                  static_cast<unsigned char>((x * 71 + y * 29) % 256)};
    AreaSampler actual(pixels.data(), 9, 7, 11, 15);
    for (int n = 0; n < 80; ++n) {
        const double left = (n % 10) - .63, top = (n % 8) - .27;
        const double right = left + .43 + n % 4, bottom = top + .71 + n % 3;
        double alpha = 0, rgb[3]{};
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 9; ++x) {
                const double weight = std::max(0.0, std::min(right, x + 1.0) - std::max(left, double(x))) *
                                      std::max(0.0, std::min(bottom, y + 1.0) - std::max(top, double(y)));
                const auto c = pixels[y * 11 + x];
                if (c.a <= 15)
                    continue;
                alpha += weight * c.a;
                rgb[0] += weight * c.a * c.r;
                rgb[1] += weight * c.a * c.g;
                rgb[2] += weight * c.a * c.b;
            }
        const auto got = actual.sample(left, top, right, bottom);
        const int expectedAlpha = int(std::round(alpha / ((right - left) * (bottom - top))));
        check(std::abs(int(got.a) - expectedAlpha) <= 1,
              "Area coverage agrees with direct independent integral");
        if (expectedAlpha > 0)
            check(std::abs(got.r - std::round(rgb[0] / alpha)) <= 1 &&
                      std::abs(got.g - std::round(rgb[1] / alpha)) <= 1 &&
                      std::abs(got.b - std::round(rgb[2] / alpha)) <= 1,
                  "Alpha-weighted colors agree with direct independent integral");
    }
}
void derivatives() {
    const auto root = fs::temp_directory_path() /
                      ("blobber-preparation-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        fs::path p;
        ~Cleanup() {
            std::error_code e;
            fs::remove_all(p, e);
        }
    } cleanup{root};
    fs::create_directories(root / "sheets");
    OwnedImage sheet(GenImageColor(32, 16, BLANK));
    ImageDrawRectangle(&sheet.value, 4, 2, 8, 12, Color{240, 30, 50, 220});
    ImageDrawRectangle(&sheet.value, 20, 6, 8, 8, Color{240, 30, 50, 220});
    check(ExportImage(sheet.value, (root / "sheets/poses.png").string().c_str()), "Save test-owned sheet");
    const Json registration = Json::array({{{"name", "poses"},
                                            {"split_x", 16},
                                            {"scale", 1},
                                            {"pivot_x", {8, 8}},
                                            {"ground_y", {14, 14}},
                                            {"canvas_width", 16},
                                            {"canvas_height", 16},
                                            {"anchor_x", 8},
                                            {"anchor_y", 14}}});
    std::ofstream(root / "sheets.json") << registration.dump();
    const auto original = fingerprintFile(root / "sheets/poses.png");
    for (const auto filter : {ImageSampling::Nearest, ImageSampling::Area}) {
        NpcSheetOptions options{filter, .5, root / "derived"};
        prepareNpcAnimation(root / "sheets.json", options);
        const auto first = fingerprintFile(root / "derived/frames/poses-0.png");
        const auto metadata = fingerprintFile(root / "derived/derivation.json");
        prepareNpcAnimation(root / "sheets.json", options);
        check(first == fingerprintFile(root / "derived/frames/poses-0.png") &&
                  metadata == fingerprintFile(root / "derived/derivation.json"),
              "Derivatives and provenance reproduce");
        OwnedImage image(LoadImage((root / "derived/frames/poses-0.png").string().c_str()));
        check(image.value.width == 8 && image.value.height == 8 &&
                  GetImageColor(image.value, 3, 6).a == 220 && GetImageColor(image.value, 3, 7).a == 0,
              "Half-size derivative preserves ground and alpha");
        Json frames;
        std::ifstream(root / "derived/frames.json") >> frames;
        check(frames["poses"][0]["anchor"] == Json::array({.5, .875}),
              "Normalized anchors survive common resizing");
    }
    check(original == fingerprintFile(root / "sheets/poses.png") && !fs::exists(root / "frames"),
          "Separate output preserves masters and original derivative destination");
    rejects([&] { prepareNpcAnimation(root / "sheets.json", {ImageSampling::Area, .3, root / "bad"}); },
            "Unsupported profile rejected before output");
}
} // namespace
int main() {
    try {
        SetTraceLogLevel(LOG_WARNING);
        sampling();
        derivatives();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
