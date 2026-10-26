#include "assets/npc_sheet.hpp"
#include "raylib.h"
#include <iostream>
#include <stdexcept>
int main(int argc, char** argv) {
    try {
        if (argc < 2)
            throw std::runtime_error("Usage: blobber_prepare_npc_animation SHEETS_JSON "
                                     "[--filter nearest|area] [--resolution 1|0.5] [--output DIRECTORY]");
        blobber::NpcSheetOptions options;
        for (int i = 2; i < argc; ++i) {
            const std::string arg = argv[i];
            if (i + 1 >= argc)
                throw std::runtime_error("Missing value for " + arg);
            const std::string value = argv[++i];
            if (arg == "--filter" && (value == "nearest" || value == "area"))
                options.sampling =
                    value == "area" ? blobber::ImageSampling::Area : blobber::ImageSampling::Nearest;
            else if (arg == "--resolution" && (value == "1" || value == "0.5"))
                options.resolution = value == "1" ? 1 : 0.5;
            else if (arg == "--output")
                options.outputDirectory = value;
            else
                throw std::runtime_error("Invalid argument: " + arg + " " + value);
        }
        SetTraceLogLevel(LOG_WARNING);
        blobber::prepareNpcAnimation(argv[1], options);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
