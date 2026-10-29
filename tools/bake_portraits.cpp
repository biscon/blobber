#include "assets/portrait_atlas.hpp"
#include "assets/portrait_preparation.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace blobber;
int main(int argc, char** argv) {
    try {
        if (argc != 3)
            throw std::invalid_argument("usage: blobber_bake_portraits UI_SOURCE OUTPUT_DIRECTORY");
        preparePortraits(argv[1], argv[2]);
        std::cout << "Prepared " << rules::portraitCount << " portraits; resident=" << portraitResidentBytes
                  << " workspace_limit=" << portraitWorkspaceLimit << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
