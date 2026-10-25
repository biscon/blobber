#pragma once
#include <stdexcept>
inline void check(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}
template <class F> void rejects(F function, const char* message) {
    bool rejected = false;
    try {
        function();
    } catch (const std::exception&) {
        rejected = true;
    }
    check(rejected, message);
}
