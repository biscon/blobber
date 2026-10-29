#include "assets/rules_loader.hpp"
#include "game/party.hpp"
#include "support/check.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace blobber;
using namespace blobber::rules;
using Json = nlohmann::json;
namespace fs = std::filesystem;
namespace {
struct Fixture {
    fs::path directory = fs::temp_directory_path() /
                         ("blobber-rules-tests-" +
                          std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::path path = directory / "catalog.json";
    Fixture() { check(fs::create_directory(directory), "Create isolated rules fixture directory"); }
    ~Fixture() {
        std::error_code error;
        fs::remove_all(directory, error);
    }
    void write(const std::string& text) {
        std::ofstream out(path, std::ios::binary);
        out << text;
        out.close();
        check(bool(out), "Write fixture");
    }
    Catalog load(const Json& value) {
        write(value.dump());
        return loadRules(path);
    }
    void invalid(const Json& value, const std::string& field) {
        write(value.dump());
        invalidText(field);
    }
    void invalidText(const std::string& field) {
        bool caught = false;
        try {
            (void)loadRules(path);
        } catch (const std::runtime_error& e) {
            const std::string message = e.what();
            check(message.find(path.string()) != std::string::npos, "Diagnostic names file");
            if (message.find(field) == std::string::npos)
                throw std::runtime_error("Diagnostic missing " + field + ": " + message);
            caught = true;
        }
        check(caught, "Invalid content must fail");
    }
};
Json fixture() {
    return {
        {"schema_version", 1},
        {"attacks",
         Json::array({{{"id", "fixture.club"},
                       {"kind", "melee"},
                       {"attack_bonus", -2},
                       {"damage", {{"count", 2}, {"sides", 4}, {"modifier", -1}, {"type", "bludgeoning"}}}},
                      {{"id", "fixture.bow"},
                       {"kind", "ranged"},
                       {"attack_bonus", 3},
                       {"damage", {{"count", 1}, {"sides", 8}, {"modifier", 2}, {"type", "piercing"}}}}})},
        {"monsters", Json::array({{{"id", "fixture.actor"},
                                   {"abilities",
                                    {{"strength", 7},
                                     {"dexterity", 12},
                                     {"constitution", 11},
                                     {"intelligence", 13},
                                     {"wisdom", 8},
                                     {"charisma", 9}}},
                                   {"armor_class", 13},
                                   {"hit_points", 9},
                                   {"attacks", Json::array({"fixture.bow", "fixture.club"})}}})}};
}
void loading() {
    Fixture files;
    files.invalidText("cannot open");
    const auto good = fixture();
    const auto catalog = files.load(good);
    const auto* monster = catalog.monster(catalog.findMonster("fixture.actor"));
    check(monster && monster->armorClass == 13 && monster->hitPoints == 9 && monster->abilities[0] == 7 &&
              monster->abilities[5] == 9 && monster->attackCount == 2,
          "Load independent monster fixture");
    const auto* bow = catalog.attack(monster->attacks[0]);
    const auto* club = catalog.attack(monster->attacks[1]);
    check(bow && club && bow->id == "fixture.bow" && bow->kind == AttackKind::Ranged && bow->bonus == 3 &&
              bow->damageType == DamageType::Piercing && club->damage.count == 2 &&
              club->damage.modifier == -1,
          "References preserve requested order and attack data");
    Json empty{{"schema_version", 1}, {"attacks", Json::array()}, {"monsters", Json::array()}};
    const auto none = files.load(empty);
    check(none.monsterCount() == 0 && none.attackCount() == 0, "Explicit empty catalog legal");
    for (int fault = 0; fault < 29; ++fault) {
        auto bad = good;
        std::string field;
        switch (fault) {
        case 0:
            bad["schema_version"] = 9;
            field = "schema_version";
            break;
        case 1:
            bad.erase("attacks");
            field = "attacks";
            break;
        case 2:
            bad["attacks"] = Json::object();
            field = "attacks";
            break;
        case 3:
            bad["attacks"][0]["id"] = "";
            field = "id";
            break;
        case 4:
            bad["attacks"][0]["id"] = std::string(129, 'a');
            field = "id";
            break;
        case 5:
            bad["attacks"][1]["id"] = "fixture.club";
            field = "duplicate";
            bad["monsters"][0]["attacks"] = Json::array({"fixture.club"});
            break;
        case 6:
            bad["monsters"].push_back(bad["monsters"][0]);
            field = "duplicate";
            break;
        case 7:
            bad["monsters"][0]["attacks"][0] = "missing";
            field = "attacks[0]";
            break;
        case 8:
            bad["monsters"][0]["attacks"][0] = "fixture.club";
            field = "duplicate";
            break;
        case 9:
            bad["attacks"][0]["kind"] = "spell";
            field = "kind";
            break;
        case 10:
            bad["attacks"][0]["damage"]["type"] = "fire";
            field = "damage.type";
            break;
        case 11:
            bad["attacks"][0]["damage"]["count"] = 0;
            field = "damage";
            break;
        case 12:
            bad["attacks"][0]["damage"]["count"] = 33;
            field = "damage";
            break;
        case 13:
            bad["attacks"][0]["damage"]["sides"] = 7;
            field = "damage";
            break;
        case 14:
            bad["monsters"][0]["abilities"]["strength"] = 0;
            field = "abilities";
            break;
        case 15:
            bad["monsters"][0]["abilities"]["wisdom"] = 31;
            field = "abilities";
            break;
        case 16:
            bad["monsters"][0]["armor_class"] = 0;
            field = "armor_class";
            break;
        case 17:
            bad["monsters"][0]["hit_points"] = 0;
            field = "hit_points";
            break;
        case 18:
            bad["monsters"][0]["hit_points"] = 1.5;
            field = "hit_points";
            break;
        case 19:
            bad["monsters"][0]["hit_points"] = std::uint64_t(INT32_MAX) + 1;
            field = "hit_points";
            break;
        case 20:
            bad["attacks"][0]["attack_bonus"] = std::int64_t(INT32_MIN) - 1;
            field = "attack_bonus";
            break;
        case 21:
            bad["schema_version"] = true;
            field = "schema_version";
            break;
        case 22:
            bad["monsters"][0]["attacks"] = Json::array();
            field = "attacks";
            break;
        case 23:
            bad["attacks"][0]["damge"] = 1;
            field = "damge";
            break;
        case 24:
            bad["monsters"][0]["abilities"].erase("charisma");
            field = "charisma";
            break;
        case 25:
            bad["monsters"][0]["id"] = "has space";
            field = "id";
            break;
        case 26:
            bad["monsters"][0]["id"] = 1;
            field = "id";
            break;
        case 27:
            bad["monsters"][0]["armor_class"] = 31;
            field = "armor_class";
            break;
        case 28:
            bad["attacks"][0]["damage"]["modifier"] = UINT64_MAX;
            field = "modifier";
            break;
        }
        files.invalid(bad, field);
        // Failure cannot damage a previously published catalog or poison the next load.
        check(catalog.monster(catalog.findMonster("fixture.actor")) == monster && monster->hitPoints == 9,
              "Failed loading preserves existing catalog lifetime");
        check(files.load(good).monsterCount() == 1, "Reload after failed parse/validation succeeds");
    }
    for (const std::string& text : {std::string(""), std::string("{"), good.dump() + " garbage"}) {
        files.write(text);
        files.invalidText("parse_error");
    }
    files.write("{\"schema_version\":1,\"schema_version\":2,\"attacks\":[],\"monsters\":[]}");
    files.invalidText("duplicate object key");
    auto nestedDuplicate = good.dump();
    const auto key = nestedDuplicate.find("\"count\":2");
    check(key != std::string::npos, "Find test-owned JSON key");
    nestedDuplicate.replace(key, 9, "\"count\":2,\"count\":3");
    files.write(nestedDuplicate);
    files.invalidText("duplicate object key");
    files.write(std::string(18, '[') + "0" + std::string(18, ']'));
    files.invalidText("nesting exceeds 16");
    files.write(std::string(1024 * 1024 + 1, ' '));
    files.invalidText("exceeds 1 MiB");
    auto atLimit = good.dump();
    atLimit.resize(1024 * 1024, ' ');
    files.write(atLimit);
    check(loadRules(files.path).monsterCount() == 1, "Exact byte limit accepted");
}
void capacities() {
    Fixture files;
    auto data = fixture();
    const auto attack = data["attacks"][0];
    const auto monster = data["monsters"][0];
    data["attacks"] = Json::array();
    data["monsters"] = Json::array();
    for (int i = 0; i < 64; ++i) {
        auto a = attack;
        a["id"] = "attack." + std::to_string(i);
        data["attacks"].push_back(a);
    }
    for (int i = 0; i < 32; ++i) {
        auto m = monster;
        m["id"] = "monster." + std::to_string(i);
        m["attacks"] = Json::array();
        for (int j = 0; j < 8; ++j)
            m["attacks"].push_back("attack." + std::to_string(j));
        data["monsters"].push_back(m);
    }
    data["monsters"][31]["id"] = std::string(128, 'a');
    data["monsters"][31]["hit_points"] = INT32_MAX;
    data["monsters"][31]["armor_class"] = 30;
    data["monsters"][31]["abilities"]["strength"] = 30;
    data["monsters"][31]["abilities"]["wisdom"] = 1;
    data["attacks"][63]["attack_bonus"] = INT32_MIN;
    data["attacks"][63]["damage"] = {
        {"count", 32}, {"sides", 100}, {"modifier", INT32_MAX}, {"type", "slashing"}};
    const auto result = files.load(data);
    check(result.attackCount() == 64 && result.monsterCount() == 32 && result.monster({31})->attackCount == 8,
          "All capacity limits inclusive");
    auto bad = data;
    bad["attacks"].push_back(attack);
    files.invalid(bad, "at most 64");
    bad = data;
    bad["monsters"].push_back(monster);
    files.invalid(bad, "at most 32");
    bad = data;
    bad["monsters"][0]["attacks"].push_back("attack.8");
    files.invalid(bad, "at most 8");
}
} // namespace
int main(int argc, char** argv) {
    try {
        // Explicit diagnostic only: routine CTest never depends on game content.
        if (argc == 3 && std::string(argv[1]) == "--validate") {
            const auto catalog = loadRules(argv[2]);
            std::cout << "Valid rules catalog: " << catalog.monsterCount() << " monsters, "
                      << catalog.attackCount() << " attacks, " << catalog.raceCount() << " races, "
                      << catalog.classCount() << " classes, " << catalog.itemCount() << " items\n";
            if (catalog.preset()) {
                const Party party(catalog);
                constexpr const char* positions[]{"Front left", "Front right", "Back left", "Back right"};
                for (std::size_t i = 0; i < partySize; ++i) {
                    const auto& c = *party.occupant(static_cast<FormationSlot>(i));
                    const auto& b = c.build;
                    std::cout << positions[i] << ": " << b.name << " / " << catalog.race(b.race)->name
                              << " / " << catalog.characterClass(b.characterClass)->name << " level "
                              << b.level << " / XP " << c.experience << "\n  abilities";
                    for (const int score : c.stats.abilities)
                        std::cout << ' ' << score;
                    std::cout << " / HP " << c.hitPoints << '/' << c.stats.maxHitPoints << " / AC "
                              << c.stats.armorClass << " / Hit Dice " << c.availableHitDice << 'd'
                              << catalog.characterClass(b.characterClass)->hitDie << " / weight "
                              << c.stats.carriedWeight / 100.0 << '/' << c.stats.carryingCapacity / 100.0
                              << " lb\n";
                    std::cout << "  initiative " << c.stats.initiative << " / saves";
                    for (const int save : c.stats.saves)
                        std::cout << ' ' << save;
                    std::cout << '\n';
                    const auto gear = party.inventory().loadout(party.snapshot().formation[i]);
                    for (std::size_t n = 0; n < gear.itemCount; ++n) {
                        if (!gear.items[n].quantity)
                            continue;
                        std::cout << "  " << gear.items[n].quantity << " x "
                                  << catalog.item(gear.items[n].item)->name;
                        if (gear.equipment.body.index == n)
                            std::cout << " [body]";
                        if (gear.equipment.mainHand.index == n)
                            std::cout << " [main hand]";
                        if (gear.equipment.offHand.index == n)
                            std::cout << " [off hand]";
                        const auto& attack = c.stats.weapons[n];
                        if (attack.available)
                            std::cout << " / attack " << attack.attackBonus << " / damage "
                                      << attack.damage.count << 'd' << attack.damage.sides << " modifier "
                                      << attack.damage.modifier;
                        std::cout << '\n';
                    }
                }
            } else
                std::cout << "No preset party (rules-only catalog)\n";
            return 0;
        }
        check(argc == 1, "Usage: blobber_rules_content_tests [--validate FILE]");
        loading();
        capacities();
        std::cout << "Rules content checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
