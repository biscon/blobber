#include "assets/prepared_image.hpp"
#include "assets/world_assets.hpp"
#include "distribution/runtime_inventory.hpp"
#include <iostream>
int main(int argc, char** argv) {
    try {
        if (argc != 4)
            throw std::runtime_error("usage: validate_prepared MANIFEST ASSETS PREPARED");
        namespace d = blobber::distribution;
        d::Settings s;
        s.manifest = std::filesystem::u8path(argv[1]);
        s.source = std::filesystem::u8path(argv[2]);
        s.prepared = std::filesystem::u8path(argv[3]);
        d::resolve(s, true);
        auto world = blobber::world_assets::load(s.prepared / "world");
        world.verify();
        for (const auto* file : {"creator.qoi", "portraits-small.qoi", "portraits-large.qoi"}) {
            auto image = blobber::preparedImage(s.prepared / file);
            if (!IsImageValid(image.value))
                throw std::runtime_error("Cannot decode prepared atlas");
        }
        std::cout << d::validateContent(s, s.source, s.prepared, true).dump(2) << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
