#include "support/automation_fixtures.hpp"
#include <iostream>
#include <sys/wait.h>
using namespace blobber;
using namespace blobber::automation;
using namespace blobber::automation::test;
namespace {
const Json header{{"protocol", 1}, {"fixture", "pc01"}};
void run(const char* executable) {
    const auto cat = catalog();
    const auto defs = definitions();
    const auto setup = generatedScenario(cat);
    Client c(cat, defs);
    check(c.request("start", {{"scenario", setup}}).at("ok"), "prototype start");
    const auto first = c.protocol.state();
    check(first.at("generation").contains("map") && !first.at("terminal") &&
              c.protocol.session()->game().actors().snapshot().count == 0,
          "empty generated floor stays nonterminal");
    const auto pose = c.protocol.session()->game().movement().player;
    const auto movement = Json{{"kind", "move"}, {"direction", "forward"}};
    check(c.request("preview", {{"action", movement}}).at("data").at("accepted") &&
              c.protocol.state() == first,
          "movement preview leaves complete state unchanged");
    check(c.act(movement).at("data").at("accepted"), "generated entrance faces walkable interior");
    c.advance();
    check(!(c.protocol.session()->game().movement().player == pose),
          "normal movement advances generated party");
    for (const char* key : {"depth", "generator_version", "template_version"}) {
        auto bad = setup;
        bad[key] = 99;
        const auto before = c.protocol.state();
        check(!c.request("start", {{"scenario", bad}}).at("ok") && c.protocol.state() == before,
              "invalid generation replacement preserves run/tokens");
    }
    auto bad = setup;
    bad["monsters"] = Json::array();
    check(!c.request("start", {{"scenario", bad}}).at("ok"), "prototype rejects arena fields");
    auto duplicate = setup;
    duplicate["party"][1]["id"] = duplicate["party"][0]["id"];
    check(!c.request("start", {{"scenario", duplicate}}).at("ok"), "duplicate characters rejected");
    auto invalidParty = setup;
    invalidParty["party"].erase(0);
    check(!c.request("start", {{"scenario", invalidParty}}).at("ok"), "incomplete party rejected");
    const auto old = c.protocol.state();
    check(c.request("reset").at("ok") && c.protocol.session()->game().movement().player == pose,
          "reset restores same floor/spawn");
    check(c.protocol.state().at("generation") == first.at("generation"),
          "reset preserves full generation identity");
    check(c.protocol.state().at("party") == first.at("party") &&
              c.protocol.state().at("formation") == first.at("formation"),
          "reset retains starting resources and formation without duplicated supplies");
    Json stale{{"version", 1},
               {"id", "stale"},
               {"op", "act"},
               {"session", old["session"]},
               {"revision", old["revision"]},
               {"action", movement}};
    check(!c.protocol.request(stale, c.sink).at("ok"), "old tokens reject after reset");
    check(c.request("reset", {{"seed", 8}}).at("ok") &&
              c.protocol.state().at("generation").at("fingerprint") !=
                  first.at("generation").at("fingerprint"),
          "explicit new seed replaces prototype deterministically");
    check(c.start(cat).at("ok") && c.protocol.session()->arena(),
          "same host can replace prototype with arena");
    Directory dir;
    const auto path = dir.path / "walk.jsonl";
    {
        TraceWriter writer(path);
        writer.append("header", header);
        Client recording(cat, defs);
        recording.trace = &writer;
        check(recording.request("start", {{"scenario", setup}}).at("ok"), "recorded prototype start");
        for (const char* direction : {"forward", "left", "turn_right", "right", "back", "turn_left"}) {
            recording.act({{"kind", "move"}, {"direction", direction}});
            recording.advance();
        }
        // Repeated forward walking must eventually hit a wall, without terminal/victory.
        bool blocked = false;
        for (unsigned i = 0; i < 40 && !blocked; ++i) {
            blocked = !recording.act(movement).at("data").at("accepted").get<bool>();
            recording.advance();
        }
        check(blocked && !recording.protocol.session()->terminal(),
              "finite corridor ends in normal blocked move");
        recording.request("reset");
        recording.request("reset", {{"seed", 8}});
        recording.request("close");
        writer.finish();
    }
    const auto child = fork();
    check(child >= 0, "fork replay");
    if (!child) {
        execl(executable, executable, "--replay", path.c_str(), nullptr);
        _exit(127);
    }
    int status = 0;
    waitpid(child, &status, 0);
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "fresh-process strict movement/reset replay");
    // Change recorded provenance, without changing commands: strict replay must reject.
    std::ifstream in(path);
    std::string text((std::istreambuf_iterator<char>(in)), {});
    const auto field = std::string("\"fingerprint\":\"");
    const auto at = text.find(field);
    check(at != std::string::npos, "trace contains canonical generation fingerprint");
    text[at + field.size()] = text[at + field.size()] == '9' ? '8' : '9';
    std::ofstream(dir.path / "changed.jsonl") << text;
    Protocol changed(cat, defs, Mode::Diagnostic);
    bool rejected = false;
    try {
        replay(changed, dir.path / "changed.jsonl", header);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    check(rejected, "strict replay detects changed generation provenance");
    for (unsigned i = 0; i < 8; ++i) {
        check(c.request("start", {{"scenario", setup}}).at("ok"), "repeated borrower construction");
        c.act(movement);
        c.advance();
    }
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 3 && std::string(argv[1]) == "--replay") {
            const auto cat = catalog();
            const auto defs = definitions();
            Protocol p(cat, defs, Mode::Diagnostic);
            replay(p, argv[2], header);
            return 0;
        }
        run(argv[0]);
        std::cout << "Generated-floor session/replay contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
