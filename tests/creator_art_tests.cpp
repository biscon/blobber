#include "assets/creator_art.hpp"
#include "assets/image.hpp"
#include "support/check.hpp"
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace blobber;
namespace {
struct Temp {
    std::filesystem::path path;
    Temp() {
        char name[] = "/tmp/blobber-creator-art-XXXXXX";
        auto* dir = mkdtemp(name);
        if (!dir)
            throw std::runtime_error("mkdtemp");
        path = dir;
    }
    ~Temp() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};
std::string bytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
} // namespace
int main() {
    try {
        SetTraceLogLevel(LOG_WARNING);
        Temp tmp;
        const auto source = tmp.path / "source", output = tmp.path / "output";
        std::filesystem::create_directories(source / "spells");
        std::filesystem::create_directories(source / "items");
        for (unsigned i = 0; i < creatorArtNames.size(); ++i) {
            OwnedImage image(GenImageColor(
                324, 324, {static_cast<unsigned char>(i * 7), static_cast<unsigned char>(200 - i), 51, 255}));
            check(ExportImage(image.value,
                              (source / (std::string(creatorArtNames[i]) + ".png")).string().c_str()),
                  "export fixture");
        }
        prepareCreatorArt(source, output);
        const auto first = bytes(output / "creator.qoi");
        {
            OwnedImage atlas(LoadImage((output / "creator.qoi").string().c_str()));
            check(atlas.value.width == 2048 && atlas.value.height == 2048, "bounded atlas dimensions");
            for (unsigned i = 0; i < creatorArtNames.size(); ++i) {
                const int x = int(i % 6) * 328, y = int(i / 6) * 328;
                for (const auto pos : {Vector2{float(x), float(y)}, Vector2{float(x + 325), float(y + 325)},
                                       Vector2{float(x + 162), float(y + 162)}}) {
                    const auto pixel = GetImageColor(atlas.value, int(pos.x), int(pos.y));
                    check(pixel.r == i * 7 && pixel.g == 200 - i && pixel.b == 51 && pixel.a == 255,
                          "atlas identity and extruded padding");
                }
            }
        }
        prepareCreatorArt(source, output);
        check(first == bytes(output / "creator.qoi"), "atlas reproducibility");
        std::filesystem::remove(source / "spells/fire_bolt.png");
        bool rejected = false;
        try {
            prepareCreatorArt(source, output);
        } catch (const std::exception&) {
            rejected = true;
        }
        check(rejected && first == bytes(output / "creator.qoi"),
              "missing master rejects before publishing replacement");
        {
            std::ofstream fake(source / "spells/fire_bolt.png", std::ios::binary);
            fake << "invalid PNG header";
        }
        rejected = false;
        try {
            prepareCreatorArt(source, output);
        } catch (const std::exception&) {
            rejected = true;
        }
        check(rejected, "invalid PNG header rejects");
        std::cout << "Creator atlas bounds, padding and reproduction passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
