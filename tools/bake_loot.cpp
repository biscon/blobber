#include "assets/image.hpp"
#include "assets/prepared_loot.hpp"
#include <iostream>
int main(int argc, char** argv) {
    try {
        if (argc != 3)
            throw std::invalid_argument("Expected atlas and output paths");
        blobber::OwnedImage image(LoadImage(argv[1]));
        blobber::savePreparedLoot(blobber::prepareLoot(image.value), argv[2]);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
