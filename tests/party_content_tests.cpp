#include "assets/recovery_loader.hpp"
#include "assets/rules_loader.hpp"
#include "game/party.hpp"
#include "support/check.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using namespace blobber;
using namespace blobber::rules;
using Json = nlohmann::json;
namespace {
struct Files {
    std::filesystem::path directory =
        std::filesystem::temp_directory_path() /
        ("blobber-party-tests-" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::path path = directory / "catalog.json";
    Files() { check(std::filesystem::create_directory(directory), "Create owned fixture directory"); }
    ~Files() {
        std::error_code ec;
        std::filesystem::remove_all(directory, ec);
    }
    Catalog load(const Json& j) {
        std::ofstream stream(path);
        stream << j.dump();
        stream.close();
        check(bool(stream), "Write fixture");
        return loadRules(path);
    }
    void invalid(const Json& j, const std::string& expected) {
        bool caught = false;
        try {
            (void)load(j);
        } catch (const std::runtime_error& e) {
            const std::string message = e.what();
            check(message.find(path.string()) != std::string::npos, "Failure identifies file");
            if (message.find(expected) == std::string::npos)
                throw std::runtime_error("Expected " + expected + ": " + message);
            caught = true;
        }
        check(caught, "Invalid catalog must fail");
    }
};
Json abilities(int str, int dex, int con, int intelligence, int wis, int cha) {
    return {{"strength", str}, {"dexterity", dex}, {"constitution", con}, {"intelligence", intelligence},
            {"wisdom", wis},   {"charisma", cha}};
}
Json fixture() {
    Json root{{"schema_version", 2}, {"attacks", Json::array()}, {"monsters", Json::array()}};
    root["races"] = Json::array({{{"id", "test.race"},
                                  {"name", "Test race"},
                                  {"ability_increases", abilities(1, 0, 1, 0, 0, 0)},
                                  {"hit_points_per_level", 1},
                                  {"weapons", Json::array({"shortbow"})}}});
    root["classes"] = Json::array({{{"id", "test.class"},
                                    {"name", "Test class"},
                                    {"hit_die", 10},
                                    {"saves", Json::array({"strength", "constitution"})},
                                    {"weapons", Json::array({"dagger"})},
                                    {"armor", Json::array({"medium", "shield"})},
                                    {"allows_defense", true}}});
    root["items"] = Json::array({{{"id", "test.weapon"},
                                  {"name", "Test blade"},
                                  {"description", "A test blade"},
                                  {"weight", 25},
                                  {"kind", "weapon"},
                                  {"weapon", "dagger"},
                                  {"damage", {{"count", 1}, {"sides", 4}, {"type", "piercing"}}},
                                  {"finesse", true},
                                  {"ranged", false},
                                  {"two_handed", false},
                                  {"versatile_sides", 0}},
                                 {{"id", "test.armor"},
                                  {"name", "Test mail"},
                                  {"description", "Test armor"},
                                  {"weight", 100},
                                  {"kind", "armor"},
                                  {"armor", "medium"},
                                  {"base_ac", 12}},
                                 {{"id", "test.shield"},
                                  {"name", "Test shield"},
                                  {"description", "A test shield"},
                                  {"weight", 20},
                                  {"kind", "shield"}}});
    Json party{{"characters", Json::array()}, {"formation", Json::array()}};
    for (int i = 0; i < 4; ++i) {
        const auto id = "test.character" + std::to_string(i);
        party["characters"].push_back({{"id", id},
                                       {"name", "Test " + std::to_string(i)},
                                       {"race", "test.race"},
                                       {"class", "test.class"},
                                       {"level", 1},
                                       {"defense", true},
                                       {"abilities", abilities(10, 16, 15, 10, 10, 10)},
                                       {"items", Json::array({{{"item", "test.weapon"}, {"quantity", 1}},
                                                              {{"item", "test.armor"}, {"quantity", 1}},
                                                              {{"item", "test.shield"}, {"quantity", 1}}})},
                                       {"equipment", {{"body", 1}, {"main_hand", 0}, {"off_hand", 2}}}});
        party["formation"].push_back(id);
    }
    root["preset_party"] = party;
    return root;
}
void inventoryLoading() {
    Files files;
    auto good = fixture();
    good["schema_version"] = 5;
    good["spells"] = Json::array();
    good["classes"][0]["casting"] = "none";
    good["classes"][0]["features"] = Json::array();
    good["classes"][0]["spells"] = Json::array();
    for (auto& item : good["items"])
        item["stack_limit"] = 1;
    good["items"][0]["ammunition"] = "none";
    good["items"].push_back({{"id", "test.arrow"},
                             {"name", "Arrow"},
                             {"description", "Fixture ammunition"},
                             {"kind", "ammunition"},
                             {"weight", 5},
                             {"stack_limit", 100},
                             {"ammunition", "arrow"}});
    good["items"].push_back({{"id", "test.potion"},
                             {"name", "Potion"},
                             {"description", "Fixture healing"},
                             {"kind", "consumable"},
                             {"weight", 50},
                             {"stack_limit", 20},
                             {"effect", "healing"}});
    for (auto& c : good["preset_party"]["characters"]) {
        c["life_domain"] = false;
        c["equipment"]["ranged"] = nullptr;
        c["items"].push_back({{"item", "test.arrow"}, {"quantity", 201}});
        c["items"].push_back({{"item", "test.potion"}, {"quantity", 21}});
    }
    const auto cat = files.load(good);
    const Party p(cat);
    check(p.inventory().count({0}, Ammunition::Arrow, cat) == 201,
          "Schema five preserves and partitions starting quantities");
    check(p.inventory().stack(p.inventory().at({{0}, 0}))->quantity == 100 &&
              p.inventory().stack(p.inventory().at({{0}, 2}))->quantity == 1 &&
              p.inventory().stack(p.inventory().at({{0}, 3}))->quantity == 20 &&
              p.inventory().stack(p.inventory().at({{0}, 4}))->quantity == 1,
          "Ammo and potion stack boundaries");
    for (int fault = 0; fault < 9; ++fault) {
        auto bad = good;
        switch (fault) {
        case 0:
            bad["items"][3]["stack_limit"] = 20;
            break;
        case 1:
            bad["items"][4]["effect"] = "resurrect";
            break;
        case 2:
            bad["items"][3]["ammunition"] = "none";
            break;
        case 3:
            bad["items"][0]["ammunition"] = "arrow";
            break;
        case 4:
            bad["items"][4]["stack_limit"] = 0;
            break;
        case 5:
            bad["items"][4]["effect"] = "mana";
            bad["items"][4]["spell"] = "test.fire";
            break;
        case 6:
            bad["items"][4].erase("effect");
            break;
        case 7:
            bad["items"][0].erase("ammunition");
            break;
        case 8:
            bad["items"][4].erase("stack_limit");
            break;
        }
        files.invalid(bad, "items");
    }
    auto six = good;
    six["schema_version"] = 6;
    six["items"].push_back({{"id", "test.gold"},
                            {"name", "Gold"},
                            {"description", "Test coins"},
                            {"kind", "currency"},
                            {"weight", 2},
                            {"stack_limit", 1000}});
    six["items"].push_back({{"id", "test.kit"},
                            {"name", "Kit"},
                            {"description", "Test kit"},
                            {"kind", "supply"},
                            {"weight", 800},
                            {"stack_limit", 5}});
    six["items"][0]["weapon"] = "scimitar";
    six["classes"][0]["weapons"] = Json::array({"scimitar"});
    six["monsters"].push_back(
        {{"id", "test.monster"},
         {"abilities", abilities(10, 10, 10, 10, 10, 10)},
         {"armor_class", 10},
         {"hit_points", 7},
         {"attacks", Json::array()},
         {"traits", Json::array()},
         {"loot",
          Json::array(
              {{{"item", "test.gold"}, {"quantity", 1}, {"sides", 6}, {"roll", Json::array({1, 100})}}})}});
    // Monsters require an attack even when this fixture only exercises loot loading.
    six["attacks"].push_back(
        {{"id", "test.hit"},
         {"kind", "melee"},
         {"attack_bonus", 0},
         {"damage", {{"count", 1}, {"sides", 4}, {"modifier", 0}, {"type", "slashing"}}}});
    six["monsters"][0]["attacks"].push_back("test.hit");
    const auto newCat = files.load(six);
    check(newCat.monster({0})->lootCount == 1 && stackLimit(*newCat.item({5})) == 1000,
          "Schema six resolves loot and currency");
    check(!newCat.race({0})->sleepImmune && !newCat.monster({0})->metalArmor,
          "Legacy schema defaults capabilities off");
    auto seven = six;
    seven["schema_version"] = 7;
    for (auto& race : seven["races"])
        race["sleep_immune"] = true;
    for (auto& item : seven["items"])
        if (item["kind"] == "armor")
            item["metal_armor"] = true;
    seven["monsters"][0]["sleep_immune"] = true;
    seven["monsters"][0]["metal_armor"] = true;
    const auto capabilities = files.load(seven);
    check(capabilities.race({0})->sleepImmune && capabilities.monster({0})->sleepImmune &&
              capabilities.monster({0})->metalArmor && capabilities.item({1})->metalArmor,
          "Schema seven loads explicit capabilities");
    for (const auto field : {"sleep_immune", "metal_armor"}) {
        auto bad = seven;
        bad["monsters"][0][field] = "yes";
        files.invalid(bad, field);
    }
    auto badArmor = seven;
    badArmor["items"][1]["metal_armor"] = 1;
    files.invalid(badArmor, "metal_armor");
    auto badRace = seven;
    badRace["races"][0]["sleep_immune"] = 1;
    files.invalid(badRace, "sleep_immune");
    for (int fault = 0; fault < 5; ++fault) {
        auto bad = six;
        if (fault == 0)
            bad["monsters"][0]["loot"][0]["item"] = "missing";
        if (fault == 1)
            bad["monsters"][0]["loot"][0]["roll"] = Json::array({20, 10});
        if (fault == 2)
            bad["monsters"][0]["loot"][0]["quantity"] = 1001;
        if (fault == 3)
            bad["monsters"][0]["loot"][0]["sides"] = 1;
        if (fault == 4)
            bad["monsters"][0]["loot"] = Json::array(
                {six["monsters"][0]["loot"][0], six["monsters"][0]["loot"][0], six["monsters"][0]["loot"][0],
                 six["monsters"][0]["loot"][0], six["monsters"][0]["loot"][0], six["monsters"][0]["loot"][0],
                 six["monsters"][0]["loot"][0], six["monsters"][0]["loot"][0],
                 six["monsters"][0]["loot"][0]});
        rejects([&] { files.load(bad); }, "Invalid reward profile rejected");
    }
    const Map recoveryMap({"#####", "#...#", "#...#", "#####"});
    const auto recoveryFile = files.directory / "recovery.json";
    const Json recovery = {
        {"schema_version", 1},
        {"sanctuaries", Json::array({Json::array({1, 1})})},
        {"ground", Json::array({{{"cell", Json::array({2, 1})}, {"item", "test.kit"}, {"quantity", 1}}})}};
    const auto loadLevel = [&](const Json& data) {
        std::ofstream out(recoveryFile);
        out << data;
        out.close();
        return loadRecoveryLevel(recoveryFile, recoveryMap, newCat);
    };
    check(loadLevel(recovery).groundCount == 1, "Authored ground and sanctuary metadata");
    for (int fault = 0; fault < 5; ++fault) {
        auto bad = recovery;
        if (fault == 0)
            bad["ground"][0]["quantity"] = 6;
        if (fault == 1)
            bad["ground"][0]["cell"] = Json::array({0, 0});
        if (fault == 2)
            bad["sanctuaries"].push_back(Json::array({1, 1}));
        if (fault == 3)
            bad["ground"][0]["item"] = "missing";
        if (fault == 4)
            bad["unexpected"] = true;
        rejects([&] { loadLevel(bad); }, "Invalid recovery metadata rejected");
    }
    // Legacy version two keeps its strict fields and still gains bounded live stacks.
    auto legacy = fixture();
    auto arrow = good["items"][3];
    arrow.erase("stack_limit");
    arrow.erase("ammunition");
    legacy["items"].push_back(arrow);
    legacy["preset_party"]["characters"][0]["items"].push_back({{"item", "test.arrow"}, {"quantity", 101}});
    const auto oldCat = files.load(legacy);
    const Party old(oldCat);
    check(old.inventory().count({0}, Ammunition::Arrow, oldCat) == 101 &&
              old.inventory().stack(old.inventory().at({{0}, 1}))->quantity == 1,
          "Legacy ammunition normalizes");
    legacy["items"][3]["stack_limit"] = 100;
    files.invalid(legacy, "stack_limit");
}
void spellsLoading() {
    Files files;
    auto good = fixture();
    good["schema_version"] = 4;
    good["spells"] = Json::array({{{"id", "test.fire"},
                                   {"name", "Test fire"},
                                   {"description", "Fixture spell"},
                                   {"kind", "fire_bolt"}}});
    good["classes"][0]["casting"] = "intelligence";
    good["classes"][0]["features"] = Json::array({"arcane_recovery"});
    good["classes"][0]["spells"] = Json::array({"test.fire"});
    for (auto& c : good["preset_party"]["characters"]) {
        c["life_domain"] = false;
        c["equipment"]["ranged"] = nullptr;
    }
    files.invalid(good, "legacy caster");
    for (auto kind :
         {SpellKind::AcidSplash, SpellKind::RayOfFrost, SpellKind::MageArmor, SpellKind::MagicMissile,
          SpellKind::BurningHands, SpellKind::Thunderwave, SpellKind::Sleep, SpellKind::Grease}) {
        const auto& rule = *rules::spellRules(kind);
        good["spells"].push_back(
            {{"id", rule.id}, {"name", rule.name}, {"description", rule.description}, {"kind", rule.id}});
        good["classes"][0]["spells"].push_back(rule.id);
    }
    const auto catalog = files.load(good);
    const Party party(catalog);
    check(catalog.spellCount() == 9 &&
              catalog.spell(catalog.findSpell("test.fire"))->kind == SpellKind::FireBolt &&
              party.character({0})->mana == 4,
          "Schema four loads typed spells and initializes casting resources");
    for (int fault = 0; fault < 8; ++fault) {
        auto bad = good;
        switch (fault) {
        case 0:
            bad["spells"][0]["kind"] = "fireball";
            files.invalid(bad, "kind");
            break;
        case 1:
            bad["classes"][0]["spells"] = {"missing"};
            files.invalid(bad, "reference");
            break;
        case 2:
            bad["classes"][0]["spells"] = {"test.fire", "test.fire"};
            files.invalid(bad, "duplicate");
            break;
        case 3:
            bad["classes"][0]["casting"] = "wisdom";
            bad["classes"][0]["features"] = Json::array();
            files.invalid(bad, "eligibility");
            break;
        case 4:
            bad["classes"][0]["features"] = {"arcane_recovery", "arcane_recovery"};
            files.invalid(bad, "duplicate");
            break;
        case 5:
            bad["preset_party"]["characters"][0]["life_domain"] = true;
            files.invalid(bad, "feature");
            break;
        case 6:
            bad["spells"].push_back(bad["spells"][0]);
            bad["spells"].back()["id"] = "another";
            files.invalid(bad, "duplicate");
            break;
        case 7:
            for (int i = 0; i < 32; ++i)
                bad["spells"].push_back(bad["spells"][0]);
            files.invalid(bad, "spells");
            break;
        }
    }
}
void loading() {
    Files files;
    const auto good = fixture();
    const auto catalog = files.load(good);
    const Party party(catalog);
    const auto* c = party.character({0});
    check(c && c->hitPoints == 14 && c->stats.armorClass == 17 && c->stats.carriedWeight == 145 &&
              c->stats.weapons[49].attackBonus == 5 && c->stats.weapons[49].damage.modifier == 3,
          "JSON references create observable party statistics");
    for (int fault = 0; fault < 45; ++fault) {
        auto bad = good;
        auto& b = bad["preset_party"]["characters"][0];
        std::string field;
        switch (fault) {
        case 0:
            bad["races"][0]["ability_increases"]["strength"] = 3;
            field = "ability_increases";
            break;
        case 1:
            bad["races"][0]["hit_points_per_level"] = 2;
            field = "races";
            break;
        case 2:
            bad["races"][0]["weapons"] = {"unknown"};
            field = "weapons";
            break;
        case 3:
            bad["races"].push_back(bad["races"][0]);
            field = "duplicate";
            break;
        case 4:
            bad["classes"][0]["saves"] = {"strength", "strength"};
            field = "saves";
            break;
        case 5:
            bad["classes"][0]["saves"] = {"strength"};
            field = "saves";
            break;
        case 6:
            bad["classes"][0]["weapons"] = {"simple", "dagger"};
            field = "weapons";
            break;
        case 7:
            bad["classes"][0]["armor"] = {"heavy", "heavy"};
            field = "armor";
            break;
        case 8:
            bad["classes"][0]["allows_defense"] = 1;
            field = "allows_defense";
            break;
        case 9:
            bad["classes"][0]["hit_die"] = 20;
            field = "classes";
            break;
        case 10:
            bad["items"][0]["weight"] = -1;
            field = "weight";
            break;
        case 11:
            bad["items"][0]["damage"]["modifier"] = 3;
            field = "modifier";
            break;
        case 12:
            bad["items"][0]["finesse"] = 1;
            field = "finesse";
            break;
        case 13:
            bad["items"][0]["damage"]["sides"] = 3;
            field = "weapon properties";
            break;
        case 14:
            bad["items"][1]["armor"] = "shield";
            field = "armor";
            break;
        case 15:
            bad["items"][1]["base_ac"] = 31;
            field = "armor properties";
            break;
        case 16:
            bad["items"][2]["weapon"] = "dagger";
            field = "weapon";
            break;
        case 17:
            bad["items"][0]["kind"] = "spell";
            field = "kind";
            break;
        case 18:
            b["race"] = "missing";
            field = ".race";
            break;
        case 19:
            b["class"] = "missing";
            field = ".class";
            break;
        case 20:
            b["items"][0]["item"] = "missing";
            field = ".item";
            break;
        case 21:
            b["items"][0]["quantity"] = 0;
            field = "quantity";
            break;
        case 22:
            b["items"][0]["quantity"] = 65536;
            field = "quantity";
            break;
        case 23:
            b["items"][0]["quantity"] = 1.5;
            field = "quantity";
            break;
        case 24:
            b["equipment"]["body"] = 16;
            field = "equipment.body";
            break;
        case 25:
            b["equipment"]["main_hand"] = -1;
            field = "main_hand";
            break;
        case 26:
            b["equipment"]["off_hand"] = 0;
            field = "equipment";
            break;
        case 27:
            b["equipment"]["body"] = 4;
            field = "equipment";
            break;
        case 28:
            b["items"][0]["quantity"] = 2;
            field = "equipment";
            break;
        case 29:
            b["level"] = 3;
            field = "level";
            break;
        case 30:
            b["abilities"]["strength"] = 30;
            field = "abilities";
            break;
        case 31:
            b["id"] = " ";
            field = "id";
            break;
        case 32:
            b["name"] = std::string(65, 'n');
            field = "name";
            break;
        case 33:
            bad["preset_party"]["formation"][0] = "missing";
            field = "formation[0]";
            break;
        case 34:
            bad["preset_party"]["formation"][0] = "test.character1";
            field = "formation";
            break;
        case 35:
            bad["preset_party"]["formation"].erase(0);
            field = "formation";
            break;
        case 36:
            bad["preset_party"]["characters"].erase(0);
            field = "characters";
            break;
        case 37:
            bad["preset_party"]["characters"][1]["id"] = "test.character0";
            bad["preset_party"]["formation"][1] = "test.character0";
            field = "formation";
            break;
        case 38:
            bad["items"][0]["weight"] = INT32_MAX;
            field = "weight exceeds";
            break;
        case 39:
            b["mana"] = 10;
            field = "mana";
            break;
        case 40:
            bad.erase("preset_party");
            field = "preset_party";
            break;
        case 41:
            bad["classes"][0]["allows_defense"] = false;
            field = "Defense";
            break;
        case 42:
            bad["classes"][0]["armor"] = Json::array();
            field = "proficiency";
            break;
        case 43:
            bad["items"][0]["two_handed"] = true;
            field = "hand conflict";
            break;
        case 44:
            bad["schema_version"] = 9;
            field = "schema_version";
            break;
        }
        files.invalid(bad, field);
        check(c->hitPoints == 14 && party.character({0}) == c,
              "Failed loading preserves published party/catalog");
        const auto again = files.load(good);
        check(Party(again).character({0})->stats.armorClass == 17, "Valid loading works after failure");
    }
    auto noPreset = good;
    noPreset["preset_party"] = nullptr;
    const auto emptyParty = files.load(noPreset);
    check(!emptyParty.preset(), "Schema 2 supports rules-only catalogs");
    rejects([&] { Party p(emptyParty); }, "Application party construction requires preset");
    Json legacy{{"schema_version", 1}, {"attacks", Json::array()}, {"monsters", Json::array()}};
    check(!files.load(legacy).preset(), "Legacy version 1 remains readable");
    legacy["races"] = Json::array();
    files.invalid(legacy, "unknown field");
}
void hybridSchema() {
    Files files;
    auto j = fixture();
    j["schema_version"] = 3;
    for (auto& c : j["preset_party"]["characters"])
        c["equipment"]["ranged"] = nullptr;
    auto bow = j["items"][0];
    bow["id"] = "test.bow";
    bow["weapon"] = "shortbow";
    bow["ranged"] = true;
    bow["two_handed"] = true;
    bow["finesse"] = false;
    j["items"].push_back(bow);
    auto& first = j["preset_party"]["characters"][0];
    first["items"].push_back({{"item", "test.bow"}, {"quantity", 1}});
    first["equipment"]["off_hand"] = nullptr;
    first["equipment"]["ranged"] = 3;
    j["attacks"].push_back({{"id", "test.shot"},
                            {"kind", "ranged"},
                            {"attack_bonus", 4},
                            {"damage", {{"count", 1}, {"sides", 6}, {"modifier", 2}, {"type", "piercing"}}}});
    j["monsters"].push_back({{"id", "test.goblin"},
                             {"abilities", abilities(8, 14, 10, 10, 8, 8)},
                             {"armor_class", 15},
                             {"hit_points", 7},
                             {"attacks", {"test.shot"}},
                             {"traits", {"nimble_escape"}}});
    auto loaded = files.load(j);
    check(loaded.preset()->characters[0].equipment.ranged.index == 3 &&
              hasTrait(loaded.monster({0})->traits, Trait::NimbleEscape),
          "Schema 3 binds ranged equipment and typed traits");
    auto bad = j;
    bad["monsters"][0]["traits"] = {"unknown"};
    files.invalid(bad, "traits");
    bad = j;
    bad["monsters"][0].erase("traits");
    files.invalid(bad, "traits");
    bad = j;
    bad["preset_party"]["characters"][0]["equipment"]["ranged"] = 0;
    files.invalid(bad, "equipment");
    bad = j;
    bad["preset_party"]["characters"][0]["equipment"]["off_hand"] = 2;
    files.invalid(bad, "equipment");
    const auto legacy = files.load(fixture());
    check(legacy.preset()->characters[0].equipment.ranged.index >= 16, "Schema 2 defaults to no readied bow");
}
void limits() {
    Files files;
    auto j = fixture();
    for (const auto* category : {"races", "classes", "items"}) {
        const std::size_t limit = std::string(category) == "items" ? 64 : 16;
        while (j[category].size() < limit) {
            auto d = j[category][0];
            d["id"] = std::string(category) + "." + std::to_string(j[category].size());
            j[category].push_back(d);
        }
    }
    const auto max = files.load(j);
    check(Party(max).character({0})->hitPoints == 14, "Exact catalog capacities retain usable references");
    for (const auto* category : {"races", "classes", "items"}) {
        auto bad = j;
        bad[category].push_back(j[category][0]);
        files.invalid(bad, "at most");
    }
    auto& b = j["preset_party"]["characters"][0];
    b["equipment"] = {{"body", nullptr}, {"main_hand", nullptr}, {"off_hand", nullptr}};
    while (b["items"].size() < 16)
        b["items"].push_back({{"item", "test.weapon"}, {"quantity", 1}});
    check(files.load(j).preset()->characters[0].itemCount == 16, "Sixteen owned entries accepted");
    b["items"].push_back({{"item", "test.weapon"}, {"quantity", 1}});
    files.invalid(j, "at most 16");
    j = fixture();
    j["items"][0]["weight"] = 0;
    auto& c = j["preset_party"]["characters"][0];
    c["items"][0]["quantity"] = 65535;
    c["equipment"]["main_hand"] = nullptr;
    check(files.load(j).preset()->characters[0].items[0].quantity == 65535,
          "Maximum uint16 quantity survives loading");
}
} // namespace
int main() {
    try {
        loading();
        spellsLoading();
        inventoryLoading();
        hybridSchema();
        limits();
        std::cout << "Party content checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
