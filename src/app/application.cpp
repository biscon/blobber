#include "app/application.hpp"
#include "app/city_review.hpp"
#include "app/menu.hpp"
#include "app/options.hpp"
#include <iostream>
namespace blobber {
int run(int argc, char** argv) {
    const auto options = parseOptions(argc, argv);
    if (options.help) {
        std::cout
            << "blobber [--size WIDTH HEIGHT] [--assets DIRECTORY] [--campaign-save FILE] "
               "[--no-audio] [--settings FILE] [--seed UINT32] [--development] [--supersampling on|off] "
               "[--uncapped] [--sanctuary-city-review]\n";
        return 0;
    }
    return options.sanctuaryCityReview ? runCityReview(options) : runMenus(options);
}
} // namespace blobber
