#include "app/options.hpp"
#include "support/check.hpp"
#include <iostream>
#include <stdexcept>
using namespace blobber;
int main() {
    try {
        unsigned draws = 0;
        auto entropy = [&] { return 100 + ++draws; };
        check(chooseCampaignSeed({}, {}, entropy) == 101 && chooseCampaignSeed({}, {}, entropy) == 102,
              "each new unseeded campaign samples fresh entropy");
        check(chooseCampaignSeed(5, {}, entropy) == 5 && chooseCampaignSeed(5, 8, entropy) == 8 && draws == 2,
              "explicit overrides and Continue do not draw entropy; Continue retains its saved seed");
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
        check(!parseOptions(1, normal).uncapped, "Normal launch retains frame pacing");
        char uncapped[] = "--uncapped";
        char* fast[]{program, uncapped, help};
        check(parseOptions(3, fast).uncapped && parseOptions(3, fast).help,
              "Uncapped launch does not consume the following option");
        char city[] = "--sanctuary-city-review", save[] = "--campaign-save", path[] = "unused.json";
        char* review[]{program, city, help};
        check(parseOptions(3, review).sanctuaryCityReview && parseOptions(3, review).help,
              "City review launch is explicit and preserves help");
        char* conflicting[]{program, city, save, path};
        rejects([&] { parseOptions(4, conflicting); }, "Review cannot consume a campaign save");
        char quality[] = "--supersampling", off[] = "off", bad[] = "nearest";
        char* lower[]{program, quality, off};
        check(!parseOptions(3, lower).supersampling, "Off selects native filtered quality for normal game");
        char* raw[]{program, quality, bad};
        rejects([&] { parseOptions(3, raw); }, "Raw nearest is not a production quality option");
        check(parseOptions(1, normal).ambientOcclusion, "AO defaults on");
        char ao[] = "--ambient-occlusion", on[] = "on";
        char* aoOff[]{program, ao, off};
        char* aoOn[]{program, ao, on};
        char* aoBad[]{program, ao, bad};
        char* aoMissing[]{program, ao};
        check(!parseOptions(3, aoOff).ambientOcclusion && parseOptions(3, aoOn).ambientOcclusion,
              "AO can be explicitly enabled or disabled");
        rejects([&] { parseOptions(3, aoBad); }, "Invalid AO value rejects");
        rejects([&] { parseOptions(2, aoMissing); }, "Missing AO value rejects");
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
