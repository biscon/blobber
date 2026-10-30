#include "assets/rules_loader.hpp"
#include "assets/rules_character_loader.hpp"
#include "assets/rules_json.hpp"
#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <limits>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <utility>

namespace blobber {
namespace {
using Json = nlohmann::json;
using namespace rules;
using namespace rules_json;
bool boolean(const Json& value, const std::string& path) {
    if (!value.is_boolean())
        fail(path, "expected boolean");
    return value.get<bool>();
}
Json read(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        fail("file", "cannot open");
    constexpr std::size_t maxBytes = 1024 * 1024;
    std::string text;
    char block[4096];
    while (stream.read(block, sizeof(block)) || stream.gcount()) {
        if (text.size() + static_cast<std::size_t>(stream.gcount()) > maxBytes)
            fail("file", "exceeds 1 MiB");
        text.append(block, static_cast<std::size_t>(stream.gcount()));
    }
    if (stream.bad())
        fail("file", "read failed");
    // Bound parser depth and reject ambiguous duplicate keys before the DOM overwrites them.
    std::array<std::set<std::string>, 17> keys;
    auto callback = [&](int depth, Json::parse_event_t event, Json& value) {
        if (depth > 16)
            fail("json", "nesting exceeds 16");
        if (event == Json::parse_event_t::object_start)
            keys[static_cast<std::size_t>(depth)].clear();
        if (event == Json::parse_event_t::key) {
            auto& seen = keys[static_cast<std::size_t>(depth - 1)];
            const auto key = value.get<std::string>();
            if (!seen.insert(key).second)
                fail("json." + key, "duplicate object key");
        }
        return true;
    };
    return Json::parse(text, callback);
}
} // namespace
rules::Catalog loadRules(const std::filesystem::path& path) {
    using namespace rules;
    try {
        const auto root = read(path);
        if (!root.is_object() || !root.contains("schema_version"))
            fail("schema_version", "missing version");
        const int version = integer(root.at("schema_version"), "schema_version");
        if (version != 1 && version != 2 && version != 3 && version != 4 && version != 5 && version != 6 &&
            version != 7 && version != 8 && version != 9)
            fail("schema_version", "only versions 1 through 9 are supported");
        if (version == 1)
            fields(root, "catalog", {"schema_version", "attacks", "monsters"});
        else if (version >= 8)
            fields(root, "catalog",
                   {"schema_version", "attacks", "monsters", "races", "classes", "items", "preset_party",
                    "spells", "creation"});
        else if (version >= 4)
            fields(root, "catalog",
                   {"schema_version", "attacks", "monsters", "races", "classes", "items", "preset_party",
                    "spells"});
        else
            fields(root, "catalog",
                   {"schema_version", "attacks", "monsters", "races", "classes", "items", "preset_party"});
        const auto& attackJson = root.at("attacks");
        const auto& monsterJson = root.at("monsters");
        array(attackJson, "attacks", Catalog::maxAttacks);
        array(monsterJson, "monsters", Catalog::maxMonsters);
        std::vector<AttackDefinition> attacks;
        attacks.reserve(attackJson.size());
        for (std::size_t i = 0; i < attackJson.size(); ++i) {
            const auto p = "attacks[" + std::to_string(i) + "]";
            const auto& a = attackJson[i];
            fields(a, p, {"id", "kind", "attack_bonus", "damage"});
            AttackDefinition definition;
            definition.id = identifier(a.at("id"), p + ".id");
            const auto kind = string(a.at("kind"), p + ".kind");
            if (kind != "melee" && kind != "ranged")
                fail(p + ".kind", "expected melee or ranged");
            definition.kind = kind == "melee" ? AttackKind::Melee : AttackKind::Ranged;
            definition.bonus = integer(a.at("attack_bonus"), p + ".attack_bonus");
            const auto& damage = a.at("damage");
            fields(damage, p + ".damage", {"count", "sides", "modifier", "type"});
            definition.damage = {integer(damage.at("count"), p + ".damage.count"),
                                 integer(damage.at("sides"), p + ".damage.sides"),
                                 integer(damage.at("modifier"), p + ".damage.modifier")};
            const auto type = string(damage.at("type"), p + ".damage.type");
            if (type == "bludgeoning")
                definition.damageType = DamageType::Bludgeoning;
            else if (type == "piercing")
                definition.damageType = DamageType::Piercing;
            else if (type == "slashing")
                definition.damageType = DamageType::Slashing;
            else
                fail(p + ".damage.type", "expected a physical damage type");
            attacks.push_back(std::move(definition));
        }
        std::vector<MonsterDefinition> monsters;
        monsters.reserve(monsterJson.size());
        constexpr std::array<const char*, abilityCount> abilityNames{
            "strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma"};
        for (std::size_t i = 0; i < monsterJson.size(); ++i) {
            const auto p = "monsters[" + std::to_string(i) + "]";
            const auto& m = monsterJson[i];
            if (version >= 9)
                fields(m, p,
                       {"id", "abilities", "armor_class", "hit_points", "attacks", "traits", "loot",
                        "metal_armor", "sleep_immune", "experience", "humanoid"});
            else if (version >= 7)
                fields(m, p,
                       {"id", "abilities", "armor_class", "hit_points", "attacks", "traits", "loot",
                        "metal_armor", "sleep_immune"});
            else if (version >= 6)
                fields(m, p, {"id", "abilities", "armor_class", "hit_points", "attacks", "traits", "loot"});
            else if (version >= 3)
                fields(m, p, {"id", "abilities", "armor_class", "hit_points", "attacks", "traits"});
            else
                fields(m, p, {"id", "abilities", "armor_class", "hit_points", "attacks"});
            MonsterDefinition definition;
            if (version >= 9) {
                const int xp = integer(m.at("experience"), p + ".experience");
                if (xp < 0)
                    fail(p + ".experience", "must be nonnegative");
                definition.experience = std::uint32_t(xp);
                definition.humanoid = boolean(m.at("humanoid"), p + ".humanoid");
            }
            if (version >= 7) {
                definition.metalArmor = boolean(m.at("metal_armor"), p + ".metal_armor");
                definition.sleepImmune = boolean(m.at("sleep_immune"), p + ".sleep_immune");
            }
            definition.id = identifier(m.at("id"), p + ".id");
            if (version >= 3) {
                array(m.at("traits"), p + ".traits", 1);
                for (const auto& trait : m.at("traits")) {
                    if (string(trait, p + ".traits") != "nimble_escape")
                        fail(p + ".traits", "unknown trait");
                    definition.traits |= static_cast<Traits>(Trait::NimbleEscape);
                }
            }
            definition.armorClass = integer(m.at("armor_class"), p + ".armor_class");
            definition.hitPoints = integer(m.at("hit_points"), p + ".hit_points");
            const auto& scores = m.at("abilities");
            fields(scores, p + ".abilities",
                   {"strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma"});
            for (std::size_t a = 0; a < abilityCount; ++a)
                definition.abilities[a] =
                    integer(scores.at(abilityNames[a]), p + ".abilities." + abilityNames[a]);
            const auto& refs = m.at("attacks");
            array(refs, p + ".attacks", definition.attacks.size());
            definition.attackCount = refs.size();
            for (std::size_t a = 0; a < refs.size(); ++a) {
                const auto field = p + ".attacks[" + std::to_string(a) + "]";
                const auto id = identifier(refs[a], field);
                const auto it = std::find_if(attacks.begin(), attacks.end(),
                                             [&](const AttackDefinition& attack) { return attack.id == id; });
                if (it == attacks.end())
                    fail(field, "unknown attack ID: " + id);
                definition.attacks[a] = {static_cast<std::uint16_t>(it - attacks.begin())};
            }
            monsters.push_back(std::move(definition));
        }
        if (version == 1)
            return Catalog(std::move(attacks), std::move(monsters));
        auto characters = loadCharacterContent(root);
        if (version >= 6) {
            for (std::size_t i = 0; i < monsters.size(); ++i) {
                const auto& entries = monsterJson[i].at("loot");
                array(entries, "monsters[].loot", 8);
                for (const auto& e : entries) {
                    fields(e, "loot[]", {"item", "quantity", "sides", "roll"});
                    const auto name = identifier(e.at("item"), "loot.item");
                    auto it = std::find_if(characters.items.begin(), characters.items.end(),
                                           [&](const ItemDefinition& d) { return d.id == name; });
                    if (it == characters.items.end())
                        fail("loot.item", "unknown item");
                    const int q = integer(e.at("quantity"), "loot.quantity");
                    const int sides = integer(e.at("sides"), "loot.sides");
                    array(e.at("roll"), "loot.roll", 2);
                    if (e.at("roll").size() != 2)
                        fail("loot.roll", "expected two bounds");
                    const int first = integer(e.at("roll")[0], "loot.roll[0]");
                    const int last = integer(e.at("roll")[1], "loot.roll[1]");
                    if (q < 1 || q > 1000 || sides < 0 || sides > 100 || first < 1 || last > 100 ||
                        first > last)
                        fail("loot", "invalid quantity, dice or percentile bounds");
                    auto& monster = monsters[i];
                    monster.loot[monster.lootCount++] = {
                        {static_cast<std::uint16_t>(it - characters.items.begin())},
                        static_cast<std::uint16_t>(q),
                        static_cast<std::uint16_t>(sides),
                        static_cast<std::uint8_t>(first),
                        static_cast<std::uint8_t>(last)};
                }
            }
        }
        return Catalog(std::move(attacks), std::move(monsters), std::move(characters.races),
                       std::move(characters.classes), std::move(characters.items),
                       std::move(characters.preset), std::move(characters.spells),
                       std::move(characters.creation));
    } catch (const std::exception& e) {
        throw std::runtime_error("Loading rules " + path.string() + ": " + e.what());
    }
}
} // namespace blobber
