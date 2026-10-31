#pragma once
#include "core/preparation.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>
namespace blobber {
// One bounded, preparation-only payload; consumed before another file is requested.
inline std::vector<unsigned char> preparationFile(const std::filesystem::path& path,
                                                  std::size_t limit = 64u * 1024 * 1024) {
    return prepareCpu([&] {
        preparationPoint("Loading artwork");
        const auto size = std::filesystem::file_size(path);
        if (!size || size > limit || size > 64u * 1024 * 1024)
            throw std::runtime_error("Artwork file exceeds loading limit: " + path.string());
        std::vector<unsigned char> bytes(static_cast<std::size_t>(size));
        std::ifstream input(path, std::ios::binary);
        for (std::size_t offset = 0; offset < bytes.size();) {
            preparationPoint("Loading artwork");
            const auto count = std::min<std::size_t>(1024 * 1024, bytes.size() - offset);
            input.read(reinterpret_cast<char*>(bytes.data() + offset), std::streamsize(count));
            if (!input)
                throw std::runtime_error("Cannot read artwork: " + path.string());
            offset += count;
        }
        return bytes;
    });
}
} // namespace blobber
