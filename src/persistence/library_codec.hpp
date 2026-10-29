#pragma once
#include "persistence/character_library.hpp"
#include <string>
namespace blobber {
constexpr std::size_t libraryMaxBytes = 1024 * 1024;
std::string encodeLibrary(const rules::Catalog&, const LibrarySnapshot&);
LibrarySnapshot decodeLibrary(const rules::Catalog&, const std::string&);
struct LibraryException : std::runtime_error {
    LibraryError code;
    LibraryException(LibraryError c, const std::string& text) : std::runtime_error(text), code(c) {}
};
} // namespace blobber
