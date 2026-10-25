#include "assets/rules_loader.hpp"
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
[[noreturn]] void fail(const std::string& field, const std::string& message) {
    throw std::runtime_error(field + ": " + message);
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
void fields(const Json& value, const std::string& path, std::initializer_list<const char*> names) {
    if (!value.is_object())
        fail(path, "expected object");
    for (const auto* name : names)
        if (!value.contains(name))
            fail(path + "." + name, "missing field");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (std::none_of(names.begin(), names.end(), [&](const char* name) { return it.key() == name; }))
            fail(path + "." + it.key(), "unknown field");
}
void array(const Json& value, const std::string& path, std::size_t limit) {
    if (!value.is_array() || value.size() > limit)
        fail(path, "expected array with at most " + std::to_string(limit) + " entries");
}
std::int32_t integer(const Json& value, const std::string& path) {
    if (!value.is_number_integer())
        fail(path, "expected int32 integer");
    if (value.is_number_unsigned()) {
        if (value.get<std::uint64_t>() > static_cast<std::uint64_t>(INT32_MAX))
            fail(path, "integer exceeds int32");
    } else {
        const auto n = value.get<std::int64_t>();
        if (n < INT32_MIN || n > INT32_MAX)
            fail(path, "integer exceeds int32");
    }
    return value.get<std::int32_t>();
}
std::string string(const Json& value, const std::string& path) {
    if (!value.is_string())
        fail(path, "expected string");
    return value.get<std::string>();
}
std::string identifier(const Json& value, const std::string& path) {
    auto id = string(value, path);
    if (!validContentId(id))
        fail(path, "expected 1..128 printable nonspace ASCII bytes");
    return id;
}
} // namespace
rules::Catalog loadRules(const std::filesystem::path& path) {
    using namespace rules;
    try {
        const auto root = read(path);
        fields(root, "catalog", {"schema_version", "attacks", "monsters"});
        if (integer(root.at("schema_version"), "schema_version") != 1)
            fail("schema_version", "only version 1 is supported");
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
            fields(m, p, {"id", "abilities", "armor_class", "hit_points", "attacks"});
            MonsterDefinition definition;
            definition.id = identifier(m.at("id"), p + ".id");
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
        return Catalog(std::move(attacks), std::move(monsters));
    } catch (const std::exception& e) {
        throw std::runtime_error("Loading rules " + path.string() + ": " + e.what());
    }
}
} // namespace blobber
