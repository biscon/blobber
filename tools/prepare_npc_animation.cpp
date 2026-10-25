#include "assets/npc_sheet.hpp"
#include "raylib.h"
#include <iostream>
#include <stdexcept>
int main(int argc, char** argv) {
    try {
        if (argc != 2)
            throw std::runtime_error("Usage: blobber_prepare_npc_animation SHEETS_JSON");
        SetTraceLogLevel(LOG_WARNING);
        blobber::prepareNpcAnimation(argv[1]);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
