#include "assets/blueprint_loader.hpp"
// Dedicated diagnostic runner: generated guardian geometry with fresh L1/L2 parties.
// Setup is deliberately absent from the normal headless protocol/game application.
#include "assets/rules_loader.hpp"
#include "automation/protocol.hpp"
#include "automation/trace.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::automation;
namespace {
void setup(Protocol& protocol, const rules::Catalog& catalog, const Json& scenario) {
    const auto decoded = decodeScenario(catalog, scenario);
    const auto& generated = std::get<GeneratedScenario>(decoded);
    auto arena = defaultArena(catalog);
    arena.party = generated.party;
    arena.adjust(catalog.findMonster("goblin"), 1);
    arena.preparation.targetLevel = generated.generation.depth == 1 ? 1 : 2;
    for (unsigned i = 0; i < 4; ++i)
        if (arena.preparation.targetLevel > 1 &&
            catalog.characterClass(arena.party[i].draft.characterClass)->progression ==
                rules::ProgressionClass::Wizard)
            arena.preparation.advancements[0][i].learned = rules::spellBit(catalog.findSpell("false_life")) |
                                                           rules::spellBit(catalog.findSpell("color_spray"));
    const auto prepared = prepareArenaParty(catalog, arena);
    auto& session = *protocol.replaySession();
    auto& game = session.replayGame();
    if (arena.preparation.targetLevel == 2) {
        game.party().grantNextLevelExperience();
        for (std::uint16_t i = 0; i < 4; ++i) {
            auto request = game.party().advancementRequest({i});
            request.choices = arena.preparation.advancements[0][i];
            if (!game.party().advance(request))
                throw std::runtime_error("Trial advancement failed");
        }
    }
    for (std::uint16_t i = 0; i < 4; ++i) {
        const auto* expected = prepared->character({i});
        const auto* actual = game.party().character({i});
        if (actual->hitPoints != expected->hitPoints || actual->mana != expected->mana ||
            actual->build.level != expected->build.level)
            throw std::runtime_error("Trial preparation differs from arena factory");
    }
    for (unsigned i = 0; i < game.actors().snapshot().count; ++i)
        if (!game.guardian(game.actors().id(i)))
            game.actors().remove(game.actors().id(i));
    const auto& layout = session.generated()->layout;
    const auto door = layout.fixtures.entries[layout.approachDoor];
    game.restorePose({door.cell + forward(door.facing) * -1, door.facing}, false);
    game.update(0, false);
}
} // namespace
int main(int argc, char** argv) {
    try {
        std::filesystem::path assets = "assets", record;
        for (int i = 1; i < argc; ++i) {
            const std::string key = argv[i];
            if (i + 1 == argc)
                throw std::runtime_error("Missing value");
            const std::string value = argv[++i];
            if (key == "--assets")
                assets = value;
            else if (key == "--record")
                record = value;
            else if (key != "--mode" || value != "diagnostic")
                throw std::runtime_error("Diagnostic only");
        }
        const auto blueprints = blueprints::load(assets / "blueprints/underkeep.json");
        const auto catalog = loadRules(assets / "rules/catalog.json");
        const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
        Protocol protocol(blueprints, catalog, definitions, Mode::Diagnostic);
        std::unique_ptr<TraceWriter> trace;
        if (!record.empty()) {
            trace = std::make_unique<TraceWriter>(record);
            trace->append("header", {{"guardian_trial", 1}});
        }
        RecordedOutput output(std::cout, trace.get());
        std::string line;
        while (!protocol.closed() && readLine(std::cin, line)) {
            const auto request = parseRequest(line);
            if (trace)
                trace->append("request", request);
            auto response = protocol.request(request, output);
            if (request.value("op", "") == "start" && response.value("ok", false)) {
                setup(protocol, catalog, request.at("scenario"));
                response["data"] = protocol.state();
            }
            output.response(response, trace ? protocol.state() : Json(nullptr));
        }
        if (trace)
            trace->finish();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
