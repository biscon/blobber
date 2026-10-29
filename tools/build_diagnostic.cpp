#include "assets/rules_loader.hpp"
#include "game/party.hpp"
#include "persistence/character_library.hpp"
#include <cstdio>
#include <iostream>
int main(int argc, char** argv) {
    try {
        using namespace blobber;
        using namespace rules;
        if (argc != 3)
            throw std::invalid_argument("usage: blobber_build_diagnostic CATALOG_JSON EXPLICIT_LIBRARY_PATH");
        const auto catalog = loadRules(argv[1]);
        for (std::uint16_t r = 0; r < catalog.raceCount(); ++r)
            for (std::uint16_t c = 0; c < catalog.classCount(); ++c) {
                auto draft = recommendedDraft(catalog, {r}, {c});
                std::snprintf(draft.name.data(), draft.name.size(), "Diagnostic character");
                const auto preview = previewBuild(catalog, draft);
                if (!preview)
                    throw std::runtime_error("Invalid class/race recommendation");
                const auto build = finishBuild(catalog, draft, "diagnostic");
                std::cout << catalog.race({r})->id << '/' << catalog.characterClass({c})->id
                          << " points=" << preview.points << " hp=" << preview.stats.maxHitPoints
                          << " ac=" << preview.stats.armorClass << " weight=" << preview.stats.carriedWeight
                          << '/' << preview.stats.carryingCapacity
                          << " portrait=" << portrait(draft.portrait)->id << '\n';
                for (std::uint16_t i = 0; i < catalog.spellCount(); ++i) {
                    const auto access = spellAccess(catalog, build, preview.stats, {i});
                    if (access)
                        std::cout << "  " << catalog.spell({i})->id << " source=" << unsigned(access.source)
                                  << " ability=" << (access.casting == Casting::Wisdom ? "WIS" : "INT")
                                  << " attack=" << access.attack << " dc=" << access.difficulty
                                  << " mana=" << access.cost << '\n';
                }
            }
        CharacterLibrary library(catalog, argv[2]);
        auto loaded = library.initialize();
        if (!loaded)
            throw std::runtime_error(loaded.message.data());
        CharacterLibrary reread(catalog, argv[2]);
        if (!reread.load() || reread.snapshot().count != library.snapshot().count)
            throw std::runtime_error("Library round-trip failed");
        if (library.snapshot().count >= 4) {
            std::array<SavedCharacterId, 4> ids;
            for (unsigned i = 0; i < 4; ++i)
                ids[i] = library.snapshot().records[i].id;
            auto starting = library.startingParty(ids, {{{0}, {1}, {2}, {3}}});
            Party a(catalog, starting), b(catalog, starting);
            a.damage({0}, 1);
            if (a.character({0})->hitPoints == b.character({0})->hitPoints)
                throw std::runtime_error("Party copies are not independent");
            unsigned kits = 0;
            for (std::uint16_t i = 0; i < 4; ++i)
                for (std::uint16_t j = 0; j < bagCells; ++j)
                    if (const auto* stack = a.inventory().stack(a.inventory().at({{i}, j})))
                        if (catalog.item(stack->item)->id == "camp_kit")
                            kits += stack->quantity;
            if (kits != 3)
                throw std::runtime_error("Launch kit grant mismatch");
            std::cout << "Independent four-character launch; camp_kits=" << kits << '\n';
        }
        std::cout << "Library records=" << library.snapshot().count
                  << " revision=" << library.snapshot().revision << " round_trip=passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
