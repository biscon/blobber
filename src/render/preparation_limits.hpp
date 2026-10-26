#pragma once
#include <cstddef>
#include <stdexcept>

namespace blobber {
struct PreparationLimits {
    std::size_t residentBytes = 512u * 1024u * 1024u;
    std::size_t loadingBytes = 512u * 1024u * 1024u;
};
inline void requirePreparationBytes(std::size_t used, std::size_t additional, std::size_t limit) {
    if (used > limit || additional > limit - used)
        throw std::runtime_error("Prepared assets exceed the configured memory budget");
}
} // namespace blobber
