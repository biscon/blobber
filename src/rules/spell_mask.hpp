#pragma once
#include <cstddef>
#include <cstdint>

namespace blobber::rules {
// Catalog-local spell indices never recycle. String IDs remain the persisted form.
using SpellMask = std::uint64_t;
inline constexpr std::size_t spellCapacity = 64;
constexpr SpellMask spellIndexBit(std::size_t index) noexcept {
    return index < spellCapacity ? SpellMask{1} << index : SpellMask{0};
}
constexpr unsigned spellMaskCount(SpellMask mask) noexcept {
    unsigned count = 0;
    while (mask) {
        mask &= mask - 1;
        ++count;
    }
    return count;
}
static_assert(spellIndexBit(31) != 0 && spellIndexBit(32) != 0 && spellIndexBit(63) != 0);
static_assert(spellIndexBit(64) == 0);
static_assert(spellMaskCount(spellIndexBit(31) | spellIndexBit(32) | spellIndexBit(63)) == 3);
} // namespace blobber::rules
