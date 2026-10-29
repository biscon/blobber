#include "assets/image.hpp"
#include "assets/portrait_preparation.hpp"
#include "support/check.hpp"
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace blobber;
int main() {
    char pattern[] = "/tmp/blobber-portraits-XXXXXX";
    const auto* temp = mkdtemp(pattern);
    if (!temp)
        return 1;
    const std::filesystem::path root(temp), source = root / "ui", output = root / "prepared";
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code e;
            std::filesystem::remove_all(path, e);
        }
    } cleanup{root};
    try {
        SetTraceLogLevel(LOG_WARNING);
        std::filesystem::create_directories(source / "portraits");
        for (unsigned i = 0; i < rules::portraitCount; ++i) {
            OwnedImage master(
                GenImageColor(432, 648, Color{static_cast<unsigned char>(20 + i * 10), 40, 80, 255}));
            check(ExportImage(master.value, (source / rules::portrait({std::uint8_t(i)})->file).c_str()),
                  "write fixture");
        }
        preparePortraits(source, output);
        for (bool big : {false, true}) {
            OwnedImage atlas(
                LoadImage((output / (big ? "portraits-large.qoi" : "portraits-small.qoi")).c_str()));
            check(atlas.value.width == portraitAtlasWidth &&
                      atlas.value.height == (big ? largePortraitHeight : smallPortraitHeight),
                  "atlas bounds");
            for (unsigned i = 0; i < rules::portraitCount; ++i) {
                auto r = portraitRegion(i, big);
                for (auto xy : {std::pair<int, int>{r.x + r.width / 2, r.y + r.height / 2},
                                {r.x - 2, r.y - 2},
                                {r.x + r.width + 1, r.y + r.height + 1}}) {
                    const auto c = GetImageColor(atlas.value, xy.first, xy.second);
                    check(c.r == 20 + i * 10 && c.g == 40 && c.b == 80 && c.a == 255,
                          "portrait identity and padded edges");
                }
            }
        }
        auto fails = [&] {
            try {
                preparePortraits(source, root / "invalid");
                return false;
            } catch (const std::exception&) {
                return true;
            }
        };
        const auto first = source / rules::portrait({0})->file;
        {
            std::fstream f(first, std::ios::in | std::ios::out | std::ios::binary);
            f.seekp(16);
            const char huge[]{0x7f, 0x7f, 0x7f, 0x7f};
            f.write(huge, 4);
        }
        check(fails(), "oversized dimensions rejected before decode");
        {
            std::ofstream f(first);
            f << "truncated";
        }
        check(fails(), "corrupt source rejected");
        std::filesystem::remove(first);
        check(fails(), "missing source rejected");
        std::cout << "Twenty-portrait preparation, identity, padding and failure checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
