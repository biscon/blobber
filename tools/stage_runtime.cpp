#include "distribution/runtime_inventory.hpp"
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    namespace dist = blobber::distribution;
    try {
        if (argc < 2)
            throw std::runtime_error(
                "usage: blobber_stage_runtime raw|dist|verify --manifest FILE --source ASSETS --prepared DIR "
                "--build DIR --config CONFIG [--executable FILE] [--multi-config] [--skip-debug]");
        const std::string operation(argv[1]);
        dist::Settings settings;
        bool skipDebug = false;
        for (int i = 2; i < argc; ++i) {
            const std::string arg(argv[i]);
            if (arg == "--multi-config")
                settings.multiConfig = true;
            else if (arg == "--skip-debug")
                skipDebug = true;
            else {
                if (i + 1 == argc)
                    throw std::runtime_error("Missing value for " + arg);
                const std::string value(argv[++i]);
                if (arg == "--manifest")
                    settings.manifest = value;
                else if (arg == "--source")
                    settings.source = value;
                else if (arg == "--prepared")
                    settings.prepared = value;
                else if (arg == "--build")
                    settings.build = value;
                else if (arg == "--config")
                    settings.configuration = value;
                else if (arg == "--executable")
                    settings.executable = value;
                else
                    throw std::runtime_error("Unknown option: " + arg);
            }
        }
        if (operation == "raw") {
            if (!(skipDebug && settings.configuration == "Debug"))
                dist::synchronizeRaw(settings);
        } else if (operation == "dist")
            dist::stageDistribution(settings);
        else if (operation == "verify") {
            const auto report = dist::verifyDistribution(settings);
            std::cout << "Verified staged content: " << report.at("files").size() << " files, "
                      << report.at("total_bytes") << " bytes\n";
        } else
            throw std::runtime_error("Unknown staging operation: " + operation);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Runtime staging failed: " << e.what() << '\n';
        return 1;
    }
}
