#include "core/mt19937.hpp"
#include "support/check.hpp"
#include <iostream>
#include <random>
#include <sstream>
int main() {
    try {
        check(blobber::Mt19937{}() == std::mt19937{}(), "standard default seed preserved");
        for (auto seed : {0u, 1u, 5u, 5489u, UINT32_MAX}) {
            blobber::Mt19937 engine(seed);
            std::mt19937 reference(seed);
            for (unsigned draw = 0; draw < 1300; ++draw) {
                if (draw == 0 || draw == 1 || draw == 623 || draw == 624 || draw == 625 || draw == 1299) {
                    std::ostringstream saved;
                    saved << engine;
                    blobber::Mt19937 restored;
                    std::istringstream input(saved.str());
                    input >> restored;
                    check(bool(input) && engine == restored, "portable RNG roundtrip");
#ifdef __GLIBCXX__
                    std::ostringstream old;
                    old << reference;
                    check(saved.str() == old.str(), "existing Linux save bytes preserved");
#endif
                    auto a = restored;
                    auto b = reference;
                    for (unsigned i = 0; i < 630; ++i)
                        check(a() == b(), "restored RNG continuation across twist");
                }
                check(engine() == reference(), "standard MT sequence preserved");
            }
        }
        blobber::Mt19937 unchanged(5), value = unchanged;
        for (const auto* bad : {"0", "4294967296", "-1"}) {
            std::istringstream in(bad);
            in >> value;
            check(in.fail() && value == unchanged, "invalid RNG restore leaves owner unchanged");
        }
        std::cout << "Portable RNG and existing Linux state contract pass\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
