#pragma once
#include <filesystem>
#include <random>
#include <stdexcept>
#include <string>
inline std::filesystem::path testDirectory() {
    std::random_device entropy;
    for (unsigned attempt = 0; attempt < 64; ++attempt) {
        auto path = std::filesystem::temp_directory_path() /
                    std::filesystem::u8path(u8"blobber-test æ 日本-" + std::to_string(entropy()) + "-" +
                                            std::to_string(entropy()));
        std::error_code error;
        if (std::filesystem::create_directory(path, error))
            return path;
        if (error && error != std::errc::file_exists)
            throw std::filesystem::filesystem_error("Create fixture directory", path, error);
    }
    throw std::runtime_error("Fixture directory attempts exhausted");
}
inline unsigned testLowestBit(unsigned value) {
    if (!value)
        throw std::runtime_error("Empty bit selection");
    unsigned bit = 0;
    while (!(value & 1u)) {
        value >>= 1;
        ++bit;
    }
    return bit;
}
