#pragma once
#include <cstdlib>
#include <new>
#ifdef _MSC_VER
#include <malloc.h>
#endif
// Per-executable C++ allocation guard; does not intercept C allocation or driver internals.
namespace {
bool watching = false;
std::size_t allocations = 0;
} // namespace
void* operator new(std::size_t n) {
    if (watching)
        ++allocations;
    if (auto* p = std::malloc(n ? n : 1))
        return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    return ::operator new(n);
}
void operator delete(void* p) noexcept {
    std::free(p);
}
void operator delete[](void* p) noexcept {
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
    std::free(p);
}
void* operator new(std::size_t n, std::align_val_t alignment) {
    if (watching)
        ++allocations;
    const auto align = std::size_t(alignment);
#ifdef _MSC_VER
    void* p = _aligned_malloc(n ? n : 1, align);
#else
    void* p = std::aligned_alloc(align, ((n ? n : 1) + align - 1) / align * align);
#endif
    if (!p)
        throw std::bad_alloc();
    return p;
}
void* operator new[](std::size_t n, std::align_val_t a) {
    return ::operator new(n, a);
}
void operator delete(void* p, std::align_val_t) noexcept {
#ifdef _MSC_VER
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void operator delete[](void* p, std::align_val_t a) noexcept {
    ::operator delete(p, a);
}
void operator delete(void* p, std::size_t, std::align_val_t a) noexcept {
    ::operator delete(p, a);
}
void operator delete[](void* p, std::size_t, std::align_val_t a) noexcept {
    ::operator delete(p, a);
}
