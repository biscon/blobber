#include "support/automation_fixtures.hpp"
#include <iostream>
#include <sys/wait.h>
using namespace blobber;
using namespace blobber::automation;
using namespace blobber::automation::test;
namespace {
const Json header{{"protocol", 1}, {"fixture", "pc02"}};
class EmptyVisibility final : public Visibility {
    TargetSnapshot targets;

  public:
    const TargetSnapshot& sample(const Map&, const GameSession& g) override {
        targets = {};
        targets.world = g.actors().snapshot().world;
        targets.revision = g.actors().snapshot().revision;
        return targets;
    }
    bool lootVisible(LootId) const noexcept override { return false; }
};
std::unique_ptr<Visibility> visibility(void*, const Map&, const GameSession&) {
    return std::make_unique<EmptyVisibility>();
}
void move(Client& c, const char* action) {
    check(c.act({{"kind", "move"}, {"direction", action}}).at("data").at("accepted"),
          "Fixture walking uses legal movement");
    check(c.advance(17, false).at("ok"), "Fixture movement settles");
}
void face(Client& c, Heading h) {
    for (unsigned i = 0; i < 4 && c.protocol.session()->game().movement().player.heading != h; ++i)
        move(c, "turn_right");
}
void walk(Client& c, Cell target) {
    const auto& g = c.protocol.session()->game();
    const auto map = g.geometry();
    const auto start = g.movement().player.cell;
    std::array<int, generation::maxCells> previous;
    previous.fill(-1);
    std::array<Cell, generation::maxCells> queue{};
    unsigned head = 0, tail = 1;
    queue[0] = start;
    const auto index = [&](Cell p) { return p.y * map.width() + p.x; };
    previous[index(start)] = index(start);
    while (head < tail) {
        auto here = queue[head++];
        for (unsigned d = 0; d < 4; ++d) {
            auto next = here + forward(Heading(d));
            if (map.solid(next) || previous[index(next)] >= 0)
                continue;
            previous[index(next)] = index(here);
            queue[tail++] = next;
        }
    }
    check(previous[index(target)] >= 0, "Diagnostic fixture route exists");
    std::vector<Cell> path;
    for (auto at = target; !(at == start);) {
        path.push_back(at);
        auto n = previous[index(at)];
        at = {n % map.width(), n / map.width()};
    }
    for (auto i = path.rbegin(); i != path.rend(); ++i) {
        const auto now = g.movement().player.cell;
        for (unsigned d = 0; d < 4; ++d)
            if (now + forward(Heading(d)) == *i) {
                face(c, Heading(d));
                break;
            }
        move(c, "forward");
    }
}
void run(const char* exe) {
    const auto cat = catalog();
    const auto defs = definitions();
    auto setup = generatedScenario(cat, 5);
    setup["depth"] = 2;
    Client player(cat, defs, Mode::Visual, visibility);
    auto first = player.request("start", {{"scenario", setup}}).at("data");
    check(!first.at("generation").contains("map") && first.at("fixtures").size() < 4,
          "Player start excludes hidden geometry and fixture totals");
    auto inspect = player.request("inspect").at("data");
    check(inspect.at("fixtures").size() == 4, "Explicit diagnostics contain full fixtures");
    auto after = player.request("observe").at("data");
    check(first.at("discovered") == after.at("discovered") && first.at("fixtures") == after.at("fixtures"),
          "Inspection does not discover hidden cells or fixtures");
    const auto hidden = player.act({{"kind", "interact"}, {"fixture", 1}}).at("data");
    const auto absent = player.act({{"kind", "interact"}, {"fixture", 31}}).at("data");
    check(!hidden.at("accepted") && hidden.at("reason") == absent.at("reason"),
          "Guessed hidden and nonexistent fixture IDs are indistinguishable");
    const auto actions = player.request("actions");
    for (const auto& a : actions.at("data").at("fixtures"))
        check(a.at("fixture") != 1, "Action descriptors do not leak hidden key");
    Directory dir;
    auto path = dir.path / "fixtures.jsonl";
    {
        TraceWriter writer(path);
        writer.append("header", header);
        Client c(cat, defs);
        c.trace = &writer;
        check(c.request("start", {{"scenario", setup}}).at("ok"), "Record keyed floor");
        const auto& floor = c.protocol.session()->game().fixtures().definition();
        walk(c, floor.entries[1].cell);
        const auto before = c.protocol.state();
        Json key{{"kind", "interact"}, {"fixture", 1}};
        check(c.request("preview", {{"action", key}}).at("data").at("accepted") &&
                  c.protocol.state() == before,
              "Key preview preserves full replay state");
        check(c.act(key).at("data").at("accepted"), "Key acquired through transport");
        check(c.act(key).at("data").at("accepted"), "Repeated key safely unchanged");
        const auto door = floor.entries[0];
        walk(c, door.cell + forward(door.facing) * -1);
        face(c, door.facing);
        check(c.act({{"kind", "interact"}, {"fixture", 0}}).at("data").at("accepted"),
              "Transport opens matching door");
        move(c, "forward");
        check(c.protocol.session()->game().movement().player.cell == door.cell,
              "Transport walks into opened cell");
        c.request("reset");
        c.request("close");
        writer.finish();
    }
    const auto child = fork();
    check(child >= 0, "fork fixture replay");
    if (!child) {
        execl(exe, exe, "--replay", path.c_str(), nullptr);
        _exit(127);
    }
    int status = 0;
    waitpid(child, &status, 0);
    check(WIFEXITED(status) && WEXITSTATUS(status) == 0, "Strict fixture/discovery replay in fresh process");
    std::ifstream input(path);
    std::string text((std::istreambuf_iterator<char>(input)), {});
    const auto at = text.find("\"quest_keys\":4");
    check(at != std::string::npos, "Trace records acquired key state");
    text[at + 13] = '0';
    std::ofstream(dir.path / "bad.jsonl") << text;
    Protocol bad(cat, defs, Mode::Diagnostic);
    rejects([&] { replay(bad, dir.path / "bad.jsonl", header); }, "Corrupt quest state fails strict replay");
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc == 3) {
            const auto cat = catalog();
            const auto defs = definitions();
            Protocol p(cat, defs, Mode::Diagnostic);
            replay(p, argv[2], header);
            return 0;
        }
        run(argv[0]);
        std::cout << "Fixture automation contracts passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
