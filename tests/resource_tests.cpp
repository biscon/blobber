#include "assets/image.hpp"
#include "assets/material_loader.hpp"
#include "core/resource_pool.hpp"
#include "support/check.hpp"
#include "world/map.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <type_traits>
using namespace blobber;
namespace fs = std::filesystem;
struct First;
struct Second;
int main() {
    try {
        static_assert(!std::is_convertible_v<Handle<First>, Handle<Second>>);
        ResourcePool<int, First, 2> pool, other;
        const auto a = pool.insert(10), b = pool.insert(20);
        check(a && b && !pool.available() && !pool.insert(30), "Explicit capacity exhaustion");
        check(!other.get(a) && !pool.get({}), "Foreign and invalid handles rejected");
        check(*pool.get(a) == 10 && pool.release(a) == 10, "Resource lookup and release");
        const auto replacement = pool.insert(30);
        check(replacement && !pool.get(a) && !pool.release(a) && *pool.get(replacement) == 30,
              "Stale generation stays invalid after reuse");
        int total = 0;
        pool.clear([&](int value) { total += value; });
        check(total == 50 && !pool.get(b) && pool.available(),
              "Clear releases every live resource exactly once");
        pool.clear([&](int) { check(false, "No double destruction"); });
        rejects([] { blobber::Material invalid(2, 2, std::vector<Color>(3)); },
                "Reject invalid material storage");
        rejects([] { blobber::Material invalid(0, 1, {}); }, "Reject zero size material");
        const fs::path directory =
            fs::temp_directory_path() /
            ("blobber-resource-tests-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directories(directory);
        struct Cleanup {
            fs::path path;
            ~Cleanup() {
                std::error_code error;
                fs::remove_all(path, error);
            }
        } cleanup{directory};
        {
            OwnedImage image(GenImageColor(8, 8, Color{12, 34, 56, 255}));
            check(ExportImage(image.value, (directory / "image.png").string().c_str()),
                  "Export own material fixture");
        }
        auto material = loadMaterial(directory / "image.png");
        check(material.sample(0.2f, 0.7f).g == 34, "Load material pixel data");
        rejects([&] { loadMaterial(directory / "missing.png"); }, "Missing material rejected");
        {
            std::ofstream file(directory / "bad.png");
            file << "not an image";
        }
        rejects([&] { loadMaterial(directory / "bad.png"); }, "Corrupt material rejected");
        {
            std::ofstream file(directory / "map.txt", std::ios::binary);
            file << "###\r\n#.#\r\n###";
        }
        check(Map::load((directory / "map.txt").string()).connected({1, 1}),
              "CRLF and final unterminated line accepted");
        {
            std::ofstream file(directory / "map.txt");
            file << std::string(Map::maxDimension + 2, '#');
        }
        rejects([&] { Map::load((directory / "map.txt").string()); }, "Oversize map input rejected");
        {
            std::ofstream file(directory / "map.txt", std::ios::binary);
            const char bytes[] = "###\n#.#\0ignored\n###\n";
            file.write(bytes, sizeof(bytes) - 1);
        }
        rejects([&] { Map::load((directory / "map.txt").string()); },
                "Embedded NUL is not silently truncated");
        rejects([] { Map invalid(std::vector<std::string>(Map::maxDimension + 1, "###")); },
                "Map height limit");
        const Map split({"#####", "#.#.#", "#####"});
        check(!split.connected({1, 1}) && !split.connected({0, 0}),
              "Disconnected and blocked spawns rejected");
        std::cout << "Resource checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
