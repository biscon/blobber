#include "app/options.hpp"
#include "support/check.hpp"
#include <iostream>
#include <stdexcept>
using namespace blobber;
int main() {
    try {
        char program[] = "blobber", development[] = "--development", help[] = "--help";
        char* normal[]{program};
        check(!parseOptions(1, normal).development, "normal launch keeps development disabled");
        char* enabled[]{program, development};
        check(parseOptions(2, enabled).development, "development flag requires no argument");
        char* combined[]{program, development, help};
        const auto options = parseOptions(3, combined);
        check(options.development && options.help, "development does not consume the following option");
        char unknown[] = "--developmen";
        char* invalid[]{program, unknown};
        bool rejected = false;
        try {
            parseOptions(2, invalid);
        } catch (const std::runtime_error&) {
            rejected = true;
        }
        check(rejected, "unknown options still reject");
        char preview[] = "--world-preview", quality[] = "--supersampling", off[] = "off", bad[] = "nearest";
        char* defaults[]{program, development, preview};
        check(parseOptions(3, defaults).supersampling, "World preview defaults to supersampling");
        char* lower[]{program, development, preview, quality, off};
        check(!parseOptions(5, lower).supersampling, "Off selects the native quality enum");
        char* forbidden[]{program, preview};
        rejects([&] { parseOptions(2, forbidden); }, "Incomplete world path requires development mode");
        char* raw[]{program, development, preview, quality, bad};
        rejects([&] { parseOptions(5, raw); }, "Raw nearest is not a production quality option");
        char* unused[]{program, quality, off};
        rejects([&] { parseOptions(3, unused); }, "Quality is not silently ignored by normal launches");
        char renderer[] = "--renderer", world[] = "world", legacy[] = "legacy";
        char* full[]{program, renderer, world, quality, off};
        check(parseOptions(5, full).worldRenderer && !parseOptions(5, full).supersampling,
              "Complete alternative world gameplay is explicitly selectable");
        char* old[]{program, renderer, legacy};
        check(!parseOptions(3, old).worldRenderer, "Legacy remains available");
        std::cout << "Command-line option checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
