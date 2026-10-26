#include "assets/prepared_assets.hpp"
#include "assets/prepared_binary.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

namespace blobber {
std::uint64_t preparedBundleChecksum(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    in.seekg(-8, std::ios::end);
    unsigned char bytes[8];
    in.read(reinterpret_cast<char*>(bytes), 8);
    prepared_io::Reader::require(bool(in));
    std::uint64_t hash = 0;
    for (int i = 0; i < 8; ++i)
        hash |= std::uint64_t(bytes[i]) << (i * 8);
    return hash;
}
void validatePreparedPublication(const std::filesystem::path& directory, std::uint64_t profile,
                                 std::uint64_t definitions) {
    try {
        const auto path = directory / "metadata.json";
        prepared_io::Reader::require(std::filesystem::file_size(path) <= 4u * 1024u * 1024u);
        std::ifstream in(path);
        const auto publication = nlohmann::json::parse(in);
        prepared_io::Reader::require(publication.at("format") == prepared_io::formatVersion &&
                                     publication.at("renderer_revision") == prepared_io::rendererRevision &&
                                     publication.at("profile") == profile &&
                                     publication.at("definitions") == definitions);
        for (const auto* name : {"scene.bin", "npcs.bin"}) {
            const auto& expected = publication.at("outputs").at(name);
            prepared_io::Reader::require(expected.at("bytes") ==
                                             std::filesystem::file_size(directory / name) &&
                                         expected.at("checksum") == preparedBundleChecksum(directory / name));
        }
    } catch (const std::exception& error) {
        throw std::runtime_error(
            std::string("Incomplete or stale prepared assets; rebuild with cmake --build --preset debug "
                        "--target blobber_bake_runtime_assets. ") +
            error.what());
    }
}
} // namespace blobber
