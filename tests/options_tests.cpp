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
        check(parseOptions(1, normal).supersampling, "Game defaults to supersampling");
        char quality[] = "--supersampling", off[] = "off", bad[] = "nearest";
        char* lower[]{program, quality, off};
        check(!parseOptions(3, lower).supersampling, "Off selects native filtered quality for normal game");
        char* raw[]{program, quality, bad};
        rejects([&] { parseOptions(3, raw); }, "Raw nearest is not a production quality option");
        for (const char* option : {"--renderer", "--world-preview", "--combat-demo", "--ui-demo", "--capture",
                                   "--capture-menu", "--renderer-fixture"}) {
            char* retired[]{program, const_cast<char*>(option)};
            rejects([&] { parseOptions(2, retired); }, "Removed selectors and demos reject");
        }
        std::cout << "Command-line option checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
