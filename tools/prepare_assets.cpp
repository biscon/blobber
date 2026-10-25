#include "assets/image.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
int main(int argc, char** argv) {
    try {
        if (argc != 3)
            throw std::invalid_argument("Usage: blobber_prepare_assets ORIGINALS_DIRECTORY OUTPUT_DIRECTORY");
        std::filesystem::create_directories(argv[2]);
        for (const char* name : {"wall.png", "floor.png", "ceiling.png"}) {
            const auto source = std::filesystem::path(argv[1]) / name;
            blobber::OwnedImage image(LoadImage(source.string().c_str()));
            if (!image.value.data)
                throw std::runtime_error("Cannot load image: " + source.string());
            ImageResizeNN(&image.value, 256, 256);
            const auto target = std::filesystem::path(argv[2]) / name;
            if (!ExportImage(image.value, target.string().c_str()))
                throw std::runtime_error("Cannot export image: " + target.string());
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
