#include "probe/review.hpp"
#include <iostream>
int main(int argc, char** argv) {
    try {
        auto s = blobber::probe::options(argc, argv);
        if (s.help) {
            std::cout
                << "blobber_renderer_probe [--art generated|shipped] [--assets DIR]\n  [--capture DIR | "
                   "--benchmark DIR] [--profile nearest|native|supersampled|legacy]\nNo output mode: manual "
                   "viewer. Capture/benchmark create hidden windows. Output DIR must be empty.\n";
            return 0;
        }
        if (!s.capture.empty())
            blobber::probe::captures(s);
        else if (!s.benchmark.empty())
            blobber::probe::benchmark(s);
        else
            blobber::probe::interactive(s);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Renderer probe: " << e.what() << '\n';
        return 1;
    }
}
