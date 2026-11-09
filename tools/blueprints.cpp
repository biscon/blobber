#include "assets/blueprint_loader.hpp"
#include "world/blueprint_report.hpp"
#include <fstream>
#include <iostream>
using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3 && argc != 4)
        throw std::runtime_error("Usage: blobber_blueprints validate CATALOG | plans CATALOG NEW_OUTPUT");
    const std::string mode = argv[1];
    const auto catalog = blueprints::load(argv[2]);
    if (mode != "validate" && mode != "plans")
        throw std::runtime_error("Unknown blueprint mode");
    unsigned variants = 0;
    for (unsigned i = 0; i < catalog.roomCount; ++i) {
        const auto& r = catalog.rooms[i];
        for (unsigned t = 0; t < 4; ++t)
            if (r.rotations & (1u << t))
                for (unsigned s = 1; s < (1u << r.socketCount); ++s) {
                    (void)blueprints::assemble(catalog, r.id.data(), t, s);
                    ++variants;
                }
    }
    for (unsigned t = 0; t < 4; ++t)
        (void)blueprints::assemble(catalog, "kitchen-hall", t);
    std::cout << "Validated " << catalog.roomCount << " rooms, " << variants
              << " socket/rotation variants and four pair orientations\n";
    if (mode == "plans") {
        if (argc != 4 || !std::filesystem::create_directory(argv[3]))
            throw std::runtime_error("Plans require a new output directory");
        for (const auto* selection : {"kitchen", "mead-hall", "barracks", "armory", "kitchen-hall"}) {
            const auto scene = blueprints::assemble(catalog, selection);
            std::ofstream out(std::filesystem::path(argv[3]) / (std::string(selection) + ".svg"));
            blueprints::writePlan(out, catalog, scene, selection);
        }
    } else if (argc != 3)
        throw std::runtime_error("validate does not take an output path");
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
