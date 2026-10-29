#include "assets/creator_art.hpp"
#include <iostream>
#include <stdexcept>
int main(int argc, char** argv) {
    try {
        if (argc != 3)
            throw std::invalid_argument("usage: blobber_bake_creator UI_SOURCE OUTPUT_DIRECTORY");
        blobber::prepareCreatorArt(argv[1], argv[2]);
        std::cout << "Creator atlas: 16 MiB resident; 64 MiB workspace limit\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
