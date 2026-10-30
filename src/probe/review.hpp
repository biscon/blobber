#pragma once
#include "probe/gpu.hpp"
#include "probe/legacy.hpp"
#include <functional>
namespace blobber::probe {
struct Settings {
    bool shipped = false, help = false;
    std::filesystem::path assets, capture, benchmark;
    int mode = 2; // User-preferred supersampling; native filtered remains profile 2 in the viewer.
    bool legacy = false;
};
Settings options(int argc, char** argv);
void captures(const Settings&);
void benchmark(const Settings&);
void interactive(const Settings&);
} // namespace blobber::probe
