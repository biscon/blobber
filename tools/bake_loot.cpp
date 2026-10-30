#include "assets/image.hpp"
#include "assets/prepared_loot.hpp"
#include <iostream>
int main(int argc, char** argv) {
    try {
        if (argc != 3 && argc != 4)
            throw std::invalid_argument("Expected atlas and output paths");
        blobber::OwnedImage image(LoadImage(argv[1]));
        std::array<Image, blobber::PreparedLoot::icons - 1> fixtures{};
        struct Cleanup {
            decltype(fixtures)& images;
            ~Cleanup() {
                for (auto i : images)
                    if (i.data)
                        UnloadImage(i);
            }
        } cleanup{fixtures};
        if (argc == 4) {
            const char* names[]{"key-bronze", "key-silver", "upstairs",
                                "downstairs", "endpoint",   "threshold"};
            for (unsigned i = 0; i < fixtures.size(); ++i) {
                fixtures[i] = LoadImage(
                    (std::filesystem::path(argv[3]) / (std::string(names[i]) + ".png")).string().c_str());
                if (!fixtures[i].data)
                    throw std::runtime_error("Missing fixture marker art");
            }
        }
        blobber::savePreparedLoot(blobber::prepareLoot(image.value, fixtures), argv[2]);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
