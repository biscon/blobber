#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <limits>
#include <optional>
#include <type_traits>

namespace blobber {
inline std::atomic<std::uint64_t> nextRegistryId{1};
template <class Tag> struct Handle {
    std::uint64_t owner = 0;
    std::uint32_t slot = 0, generation = 0;
    explicit operator bool() const noexcept { return owner != 0; }
};
// Fixed storage. Values are backend descriptors, never owning pointers.
// A released generation can never resolve again, even in a different registry.
template <class T, class Tag, std::size_t Capacity> class ResourcePool {
    static_assert(std::is_nothrow_copy_constructible_v<T> && std::is_nothrow_copy_assignable_v<T>);
    struct Slot {
        std::optional<T> value;
        std::uint32_t generation = 1;
    };
    std::array<Slot, Capacity> slots_{};
    const std::uint64_t owner_ = nextRegistryId.fetch_add(1);

  public:
    using Id = Handle<Tag>;
    ResourcePool() = default;
    ResourcePool(const ResourcePool&) = delete;
    ResourcePool& operator=(const ResourcePool&) = delete;
    bool available() const noexcept {
        for (const auto& slot : slots_)
            if (!slot.value && slot.generation)
                return true;
        return false;
    }
    Id insert(T value) noexcept {
        for (std::size_t i = 0; i < Capacity; ++i)
            if (!slots_[i].value && slots_[i].generation) {
                slots_[i].value = value;
                return {owner_, static_cast<std::uint32_t>(i), slots_[i].generation};
            }
        return {};
    }
    const T* get(Id id) const noexcept {
        if (id.owner != owner_ || id.slot >= Capacity)
            return nullptr;
        const auto& slot = slots_[id.slot];
        return slot.value && slot.generation == id.generation ? &*slot.value : nullptr;
    }
    std::optional<T> release(Id id) noexcept {
        if (!get(id))
            return {};
        auto& slot = slots_[id.slot];
        auto value = slot.value;
        slot.value.reset();
        // Retire on wrap instead of resurrecting an old handle.
        slot.generation =
            slot.generation == std::numeric_limits<std::uint32_t>::max() ? 0 : slot.generation + 1;
        return value;
    }
    template <class F> void clear(F destroy) noexcept {
        for (std::size_t i = 0; i < Capacity; ++i)
            if (auto value = release({owner_, static_cast<std::uint32_t>(i), slots_[i].generation}))
                destroy(*value);
    }
};
} // namespace blobber
