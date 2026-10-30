#pragma once
#include "automation/protocol.hpp"
#include "automation/trace.hpp"
#include "check.hpp"
#include "creation_fixtures.hpp"
#include "persistence/arena_store.hpp"
#include <fstream>
#include <sstream>
#include <unistd.h>
namespace blobber::automation::test {
struct Directory {
    std::filesystem::path path;
    Directory() {
        char pattern[] = "/tmp/blobber-automation-XXXXXX";
        auto* p = mkdtemp(pattern);
        if (!p)
            throw std::runtime_error("mkdtemp");
        path = p;
    }
    ~Directory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
};
inline rules::Catalog catalog(bool deadly = false, unsigned xp = 1200, int hp = 5) {
    const auto base = creationCatalog(true);
    std::vector<rules::RaceDefinition> races;
    std::vector<rules::ClassDefinition> classes;
    std::vector<rules::ItemDefinition> items;
    std::vector<rules::SpellDefinition> spells;
    std::vector<rules::CreationClass> creation;
    for (std::uint16_t i = 0; i < base.raceCount(); ++i)
        races.push_back(*base.race({i}));
    for (std::uint16_t i = 0; i < base.classCount(); ++i) {
        classes.push_back(*base.characterClass({i}));
        creation.push_back(*base.creation({i}));
    }
    for (std::uint16_t i = 0; i < base.itemCount(); ++i)
        items.push_back(*base.item({i}));
    for (std::uint16_t i = 0; i < base.spellCount(); ++i)
        spells.push_back(*base.spell({i}));
    auto monster = *base.monster({0});
    monster.hitPoints = deadly ? 10000 : hp;
    monster.armorClass = 10;
    monster.experience = xp;
    monster.loot[0] = {{13}, 1, 0, 1, 100};
    monster.lootCount = 1;
    auto attack = *base.attack({0});
    attack.bonus = deadly ? 100 : -100;
    attack.damage = {1, 4, deadly ? 1000 : -100};
    return {{attack}, {monster}, races, classes, items, *base.preset(), spells, creation};
}
inline NpcDefinitions definitions() {
    NpcDefinitions d;
    d.types = {"goblin"};
    d.timings.resize(1);
    d.attachments.resize(1);
    d.timings[0][0] = {{{100, 0, 0, 0}}, 1};
    for (auto clip :
         {NpcAnimation::Attack, NpcAnimation::Hurt, NpcAnimation::Death, NpcAnimation::RangedAttack})
        d.timings[0][std::size_t(clip)] = {{{50, 50, 0, 0}}, 2};
    return d;
}
inline ArenaScenario scenario(const rules::Catalog& c, unsigned count = 1) {
    auto s = defaultArena(c);
    s.adjust({0}, 1);
    s.roster[0].quantity = count;
    s.seed = 5;
    return s;
}
struct Sink : Output {
    Json events = Json::array();
    void emit(const Json& full, const Json&) override {
        for (const auto& e : full.at("events"))
            events.push_back(e);
    }
};
struct Client {
    Protocol protocol;
    Sink sink;
    unsigned next = 0;
    TraceWriter* trace = nullptr;
    std::ostringstream recordedOutput;
    Client(const rules::Catalog& c, const NpcDefinitions& d, Mode m = Mode::Diagnostic,
           VisibilityFactory f = nullptr, void* context = nullptr)
        : protocol(c, d, m, f, context) {}
    Json request(const char* op, Json data = Json::object()) {
        data["version"] = 1;
        data["id"] = std::to_string(next++);
        data["op"] = op;
        if (protocol.session() && (std::string_view(op) == "act" || std::string_view(op) == "preview" ||
                                   std::string_view(op) == "advance" || std::string_view(op) == "reset" ||
                                   std::string_view(op) == "start")) {
            const auto state = protocol.state();
            data["session"] = state["session"];
            data["revision"] = state["revision"];
        }
        if (trace) {
            trace->append("request", data);
            RecordedOutput out(recordedOutput, trace);
            auto result = protocol.request(data, out);
            out.response(result, protocol.state());
            return result;
        }
        return protocol.request(data, sink);
    }
    Json start(const rules::Catalog& c, unsigned count = 1) {
        return request("start", {{"scenario", Json::parse(encodeArena(c, scenario(c, count)))}});
    }
    Json act(const Json& action) { return request("act", {{"action", action}}); }
    Json advance(unsigned ticks = 1000, bool decision = true) {
        return request("advance", {{"ticks", ticks}, {"until_decision", decision}});
    }
};
inline Json melee(unsigned character, unsigned target) {
    return {{"kind", "combat"}, {"action", "melee"}, {"character", character}, {"target", target}};
}
inline void approach(Client& c) {
    for (int i = 0; i < 12 && realTime(c.protocol.session()->game().combat().snapshot().phase); ++i) {
        const auto moved = c.act({{"kind", "move"}, {"direction", "forward"}});
        check(moved.at("ok") && moved.at("data").at("accepted"), "scripted arena approach accepts movement");
        check(c.advance().at("ok"), "approach advancement");
        if (c.protocol.session()->game().combat().snapshot().awaitingAction)
            break;
    }
    c.advance();
}
inline void fight(Client& c, bool attack = true, unsigned limit = 2000, bool stopAtResult = false) {
    for (unsigned i = 0;
         i < limit && !c.protocol.session()->terminal() && !(stopAtResult && c.protocol.session()->result());
         ++i) {
        const auto& g = c.protocol.session()->game();
        const auto& st = g.combat().snapshot();
        if (st.phase == CombatPhase::Victory) {
            c.act({{"kind", "continue"}});
            continue;
        }
        if (st.awaitingAction) {
            const unsigned actor = st.activeCharacter->index;
            bool accepted = false;
            if (attack)
                for (unsigned n = 0; n < g.actors().snapshot().count; ++n) {
                    if (g.actors().snapshot().actors[n].life != ActorLife::Alive)
                        continue;
                    auto command = melee(actor, n);
                    const auto decoded = decodeAction(command, g);
                    if (g.combat().validate(decoded.combat, g.movement().player) == CommandReason::Ready) {
                        accepted = c.act(command).at("data").at("accepted");
                        break;
                    }
                }
            if (!accepted) {
                const auto defended = c.act({{"kind", "combat"}, {"character", actor}, {"action", "defend"}});
                if (!defended.at("data").at("accepted"))
                    check(c.act({{"kind", "combat"},
                                 {"character", actor},
                                 {"action", "feature"},
                                 {"feature", "end_turn"}})
                              .at("data")
                              .at("accepted"),
                          "skip optional allowance");
            }
        } else if (realTime(st.phase) && !g.movement().transition.active()) {
            c.act({{"kind", "move"}, {"direction", "forward"}});
            c.advance(100, false);
        } else
            c.advance();
    }
    check(c.protocol.session()->terminal() || (stopAtResult && c.protocol.session()->result()),
          "script reaches arena outcome within bound");
}
} // namespace blobber::automation::test
