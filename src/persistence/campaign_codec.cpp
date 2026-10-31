#include "persistence/campaign_codec.hpp"
#include <charconv>
#include <cmath>
#include <locale>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>
#include <type_traits>
namespace blobber {
namespace {
using Json = nlohmann::json;
[[noreturn]] void invalid(const char* what) {
    throw std::invalid_argument(what);
}
#include "persistence/campaign_fields.hpp"
template <class T> struct Array : std::false_type {};
template <class T, std::size_t N> struct Array<std::array<T, N>> : std::true_type {};
template <class T> struct Vector : std::false_type {};
template <class T> struct Vector<std::vector<T>> : std::true_type {};
template <class T> struct Optional : std::false_type {};
template <class T> struct Optional<std::optional<T>> : std::true_type {};
template <class T> struct Pointer : std::false_type {};
template <class T> struct Pointer<std::unique_ptr<T>> : std::true_type {};
template <class T> struct Bits : std::false_type {};
template <std::size_t N> struct Bits<std::bitset<N>> : std::true_type {};
template <class T> constexpr unsigned enumMaximum() {
    if constexpr (std::is_same_v<T, Environment>)
        return 1;
    else if constexpr (std::is_same_v<T, DecorationKind>)
        return 3;
    else if constexpr (std::is_same_v<T, Heading>)
        return 3;
    else if constexpr (std::is_same_v<T, NpcAnimation>)
        return npcAnimationCount - 1;
    else if constexpr (std::is_same_v<T, NpcRoutine>)
        return unsigned(NpcRoutine::Wander);
    else if constexpr (std::is_same_v<T, ActorLife>)
        return unsigned(ActorLife::Removed);
    else if constexpr (std::is_same_v<T, Awareness>)
        return unsigned(Awareness::Returning);
    else if constexpr (std::is_same_v<T, CombatPhase>)
        return unsigned(CombatPhase::RoundEnd);
    else if constexpr (std::is_same_v<T, EffectKind>)
        return unsigned(EffectKind::Count) - 1;
    else if constexpr (std::is_same_v<T, rules::Life>)
        return unsigned(rules::Life::Dead);
    else if constexpr (std::is_same_v<T, FixtureKind>)
        return unsigned(FixtureKind::GuardianGate);
    else if constexpr (std::is_same_v<T, generation::Role>)
        return unsigned(generation::Role::Optional);
    else if constexpr (std::is_same_v<T, generation::ActorRecipe>)
        return unsigned(generation::ActorRecipe::Guardian);
    else if constexpr (std::is_same_v<T, generation::Error>)
        return unsigned(generation::Error::ForcedFallback);
    else {
        static_assert(!sizeof(T), "Missing save enum bound");
    }
}
struct Archive {
    Json& object;
    const rules::Catalog& catalog;
    const NpcDefinitions& definitions;
    bool reading;
    std::size_t fields = 0;
    template <class T> void operator()(const char* name, T& value) {
        ++fields;
        if (reading) {
            if (!object.is_object() || !object.contains(name))
                invalid("Missing campaign field");
            transfer(object.at(name), value);
        } else
            transfer(object[name], value);
    }
    template <class Id, class Find, class Get> void content(Json& j, Id& id, Find find, Get get) {
        if (reading) {
            if (j.is_null()) {
                id = Id{};
                return;
            }
            if (!j.is_string())
                invalid("Expected content identifier");
            const auto text = j.get<std::string>();
            if (!rules::validContentId(text))
                invalid("Invalid content identifier");
            id = (catalog.*find)(text);
            if (!(catalog.*get)(id))
                invalid("Unknown campaign content");
        } else {
            const auto* value = (catalog.*get)(id);
            j = value ? Json(value->id) : Json(nullptr);
        }
    }
    template <class T> void transfer(Json& j, T& v) {
        if constexpr (std::is_same_v<T, bool>) {
            if (reading) {
                if (!j.is_boolean())
                    invalid("Expected boolean");
                v = j.get<bool>();
            } else
                j = v;
        } else if constexpr (std::is_integral_v<T>) {
            if (reading) {
                if constexpr (sizeof(T) == 8 && std::is_unsigned_v<T>) {
                    if (!j.is_string())
                        invalid("Expected lossless uint64 string");
                    const auto& s = j.get_ref<const std::string&>();
                    T candidate{};
                    const auto parsed = std::from_chars(s.data(), s.data() + s.size(), candidate);
                    if (s.empty() || s.size() > 20 || (s.size() > 1 && s[0] == '0') ||
                        parsed.ec != std::errc{} || parsed.ptr != s.data() + s.size())
                        invalid("Invalid uint64");
                    v = candidate;
                } else {
                    if (!j.is_number_integer())
                        invalid("Expected integer");
                    if (j.is_number_unsigned()) {
                        const auto x = j.get<std::uint64_t>();
                        if (x > std::uint64_t(std::numeric_limits<T>::max()))
                            invalid("Integer out of range");
                        v = T(x);
                    } else {
                        const auto x = j.get<std::int64_t>();
                        if (x < std::int64_t(std::numeric_limits<T>::min()) ||
                            x > std::int64_t(std::numeric_limits<T>::max()))
                            invalid("Integer out of range");
                        v = T(x);
                    }
                }
            } else {
                if constexpr (sizeof(T) == 8 && std::is_unsigned_v<T>)
                    j = std::to_string(v);
                else
                    j = v;
            }
        } else if constexpr (std::is_enum_v<T>) {
            unsigned n = unsigned(v);
            transfer(j, n);
            if (n > enumMaximum<T>())
                invalid("Invalid campaign enum");
            if (reading)
                v = T(n);
        } else if constexpr (std::is_floating_point_v<T>) {
            if (reading) {
                if (!j.is_number())
                    invalid("Expected finite number");
                v = j.get<T>();
            }
            if (!std::isfinite(v))
                invalid("Invalid floating point");
            if (!reading)
                j = v;
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (reading) {
                if (!j.is_string())
                    invalid("Expected string");
                v = j.get<std::string>();
            }
            if (v.size() > 256)
                invalid("Campaign string too long");
            if (!reading)
                j = v;
        } else if constexpr (std::is_same_v<T, rules::ItemId>) {
            content(j, v, &rules::Catalog::findItem, &rules::Catalog::item);
        } else if constexpr (std::is_same_v<T, rules::MonsterId>) {
            content(j, v, &rules::Catalog::findMonster, &rules::Catalog::monster);
        } else if constexpr (std::is_same_v<T, rules::RaceId>) {
            content(j, v, &rules::Catalog::findRace, &rules::Catalog::race);
        } else if constexpr (std::is_same_v<T, rules::ClassId>) {
            content(j, v, &rules::Catalog::findClass, &rules::Catalog::characterClass);
        } else if constexpr (std::is_same_v<T, NpcTypeId>) {
            if (reading) {
                if (!j.is_string())
                    invalid("Expected NPC appearance");
                const auto it =
                    std::find(definitions.types.begin(), definitions.types.end(), j.get<std::string>());
                if (it == definitions.types.end())
                    invalid("Unknown NPC appearance");
                v.index = std::uint16_t(it - definitions.types.begin());
            } else {
                if (v.index >= definitions.types.size())
                    invalid("Invalid NPC appearance");
                j = definitions.types[v.index];
            }
        } else if constexpr (std::is_same_v<T, rules::PortraitId>) {
            if (reading) {
                if (j.is_null())
                    v = {};
                else {
                    if (!j.is_string())
                        invalid("Invalid portrait");
                    v = rules::findPortrait(j.get<std::string>());
                    if (!rules::portrait(v))
                        invalid("Unknown portrait");
                }
            } else
                j = rules::portrait(v) ? Json(rules::portrait(v)->id) : Json(nullptr);
        } else if constexpr (std::is_same_v<T, rules::SpellGrants>) {
            Json obj = reading ? j : Json::object();
            unsigned count = 0;
            const auto mask = [&](const char* key, std::uint32_t& value) {
                ++count;
                if (reading) {
                    if (!obj.contains(key) || !obj[key].is_array() || obj[key].size() > 32)
                        invalid("Invalid spell choices");
                    value = 0;
                    for (const auto& name : obj[key]) {
                        if (!name.is_string())
                            invalid("Invalid spell identifier");
                        const auto id = catalog.findSpell(name.get<std::string>());
                        if (!catalog.spell(id) || (value & rules::spellBit(id)))
                            invalid("Unknown or duplicate spell");
                        value |= rules::spellBit(id);
                    }
                } else {
                    obj[key] = Json::array();
                    for (unsigned i = 0; i < 32; ++i)
                        if (value & (1u << i)) {
                            const auto* spell = catalog.spell({std::uint16_t(i)});
                            if (!spell)
                                invalid("Unknown spell choice");
                            obj[key].push_back(spell->id);
                        }
                }
            };
            mask("cantrips", v.cantrips);
            mask("learned", v.learned);
            mask("prepared", v.prepared);
            mask("domain", v.domain);
            mask("racial", v.racial);
            if (obj.size() != count)
                invalid("Unknown spell field");
            if (!reading)
                j = std::move(obj);
        } else if constexpr (std::is_same_v<T, std::mt19937> || std::is_same_v<T, rules::DiceRng>) {
            if (reading) {
                if (!j.is_string() || j.get_ref<const std::string&>().size() > 8192)
                    invalid("Invalid RNG state");
                const auto text = j.get<std::string>();
                // Exactly 624 uint32 state words and one position, never a seed shorthand.
                std::istringstream words(text);
                words.imbue(std::locale::classic());
                std::uint64_t word = 0;
                bool nonzero = false;
                for (unsigned i = 0; i < 625; ++i) {
                    if (!(words >> word) || word > (i == 624 ? 624u : UINT32_MAX))
                        invalid("Invalid RNG words");
                    if (i < 624)
                        nonzero |= word != 0;
                }
                words >> std::ws;
                if (!words.eof() || !nonzero)
                    invalid("Invalid RNG word count");
                std::istringstream in(text);
                in.imbue(std::locale::classic());
                if constexpr (std::is_same_v<T, std::mt19937>) {
                    if (!(in >> v))
                        invalid("Invalid RNG");
                } else if (!v.restore(in))
                    invalid("Invalid dice RNG");
            } else {
                std::ostringstream out;
                out.imbue(std::locale::classic());
                if constexpr (std::is_same_v<T, std::mt19937>)
                    out << v;
                else
                    v.describe(out);
                j = out.str();
            }
        } else if constexpr (Bits<T>::value) {
            if (reading) {
                if (!j.is_string())
                    invalid("Expected bitset");
                const auto s = j.get<std::string>();
                if (s.size() != v.size() || s.find_first_not_of("01") != std::string::npos)
                    invalid("Invalid bitset");
                v = T(s);
            } else
                j = v.to_string();
        } else if constexpr (Array<T>::value || Vector<T>::value) {
            if (reading) {
                if (!j.is_array())
                    invalid("Expected array");
                if constexpr (Array<T>::value) {
                    if (j.size() != v.size())
                        invalid("Wrong fixed array size");
                } else {
                    if (j.size() > generation::maxCells)
                        invalid("Vector capacity exceeded");
                    v.resize(j.size());
                }
                for (std::size_t i = 0; i < v.size(); ++i)
                    transfer(j[i], v[i]);
            } else {
                j = Json::array();
                for (auto& item : v) {
                    Json one;
                    transfer(one, item);
                    j.push_back(std::move(one));
                }
            }
        } else if constexpr (Optional<T>::value || Pointer<T>::value) {
            if (reading) {
                if (j.is_null())
                    v.reset();
                else {
                    if constexpr (Optional<T>::value)
                        v.emplace();
                    else
                        v = std::make_unique<typename T::element_type>();
                    transfer(j, *v);
                }
            } else {
                if (v)
                    transfer(j, *v);
                else
                    j = nullptr;
            }
        } else {
            if (!reading)
                j = Json::object();
            Archive nested{j, catalog, definitions, reading};
            visit(nested, v);
            if (reading && j.size() != nested.fields)
                invalid("Unknown campaign field");
        }
    }
};
Json parse(const std::string& text) {
    if (text.size() > campaignMaxBytes)
        invalid("Campaign save exceeds 16 MiB");
    std::array<std::set<std::string>, 33> keys;
    return Json::parse(text, [&](int depth, Json::parse_event_t event, Json& value) {
        if (depth < 0 || depth > 32)
            invalid("Campaign JSON nesting exceeded");
        if (event == Json::parse_event_t::object_start)
            keys[unsigned(depth)].clear();
        if (event == Json::parse_event_t::key &&
            (depth < 1 || !keys[unsigned(depth - 1)].insert(value.get<std::string>()).second))
            invalid("Duplicate campaign JSON key");
        return true;
    });
}
void envelope(const Json& j, const CampaignContent& content) {
    if (!j.is_object() || j.size() != 7 || !j.at("schema").is_number_integer() ||
        !j.at("simulation").is_number_integer() || !j.at("generator").is_number_integer() ||
        !j.at("templates").is_number_integer() || j.at("schema") != 2 || j.at("simulation") != 2 ||
        j.at("generator") != generation::generatorVersion ||
        j.at("templates") != generation::templateVersion || j.at("catalog") != content.catalog ||
        j.at("npcs") != std::to_string(content.npcs) || !j.contains("campaign"))
        invalid("Incompatible campaign schema, simulation or content");
}
} // namespace
std::string encodeCampaign(const rules::Catalog& catalog, const NpcDefinitions& definitions,
                           const CampaignContent& content, CampaignCheckpoint& value) {
    Json j = {{"schema", 2},
              {"simulation", 2},
              {"generator", generation::generatorVersion},
              {"templates", generation::templateVersion},
              {"catalog", content.catalog},
              {"npcs", std::to_string(content.npcs)}};
    Archive archive{j, catalog, definitions, false};
    archive("campaign", value);
    auto text = j.dump() + "\n";
    if (text.size() > campaignMaxBytes)
        invalid("Campaign save exceeds 16 MiB");
    return text;
}
std::unique_ptr<CampaignCheckpoint> decodeCampaign(const rules::Catalog& catalog,
                                                   const NpcDefinitions& definitions,
                                                   const CampaignContent& content, const std::string& text) {
    auto j = parse(text);
    envelope(j, content);
    auto out = std::make_unique<CampaignCheckpoint>();
    Archive archive{j, catalog, definitions, true};
    archive("campaign", *out);
    for (auto& l : out->locations) {
        if (l.types != definitions.types || l.recovery.sanctuaryCount > 16 || l.recovery.groundCount > 64 ||
            l.fixtures.count > fixtureCapacity || l.fixtures.landingCount > 2 || l.objective.count > 32 ||
            l.objective.rewardCount > 2)
            invalid("Invalid saved location capacities/content");
        if (l.generated) {
            if (l.generation.seed != out->locations[1].generation.seed || l.layout.populationCount > 32 ||
                l.placements.size() != l.layout.populationCount ||
                l.diagnostics.attempts > generation::maxAttempts)
                invalid("Inconsistent campaign generation provenance/population");
            FloorObjectiveDefinition expected;
            expected.approachDoor = l.layout.approachDoor;
            expected.gate = l.layout.guardianGate;
            expected.lootSeed = std::uint32_t(generation::seedFor(l.generation, generation::Domain::Reward));
            constexpr const char* names[]{"goblin", "goblin.archer", "goblin.guardian.placeholder"};
            for (unsigned i = 0; i < l.layout.populationCount; ++i) {
                const auto& p = l.layout.population[i];
                const auto& npc = l.placements[i];
                const auto monster = catalog.findMonster(names[unsigned(p.recipe)]);
                if (!npc.monster || npc.monster->index != monster.index || !(npc.cell == p.cell) ||
                    !(npc.subcell == p.subcell) || npc.facing != p.facing ||
                    npc.animation != NpcAnimation::Idle || npc.phaseMs ||
                    npc.ai.routine != NpcRoutine::Guard || npc.ai.waypointCount ||
                    definitions.types[npc.type.index] != monsterAppearance(names[unsigned(p.recipe)]))
                    invalid("Saved initial population differs from its definition");
                if (p.guardian)
                    expected.members[expected.count++] = std::uint16_t(i);
            }
            RecoveryLevel recovery;
            recovery.ground[recovery.groundCount++] = {l.layout.optionalSupply,
                                                       catalog.findItem("healing_potion"), 1};
            recovery.ground[recovery.groundCount++] = {l.layout.optionalSupply,
                                                       catalog.findItem("mana_potion"), 1};
            expected.rewards[expected.rewardCount++] = {l.layout.guardianReward,
                                                        catalog.findItem("healing_potion"), 1};
            expected.rewards[expected.rewardCount++] = {l.layout.guardianReward, catalog.findItem("camp_kit"),
                                                        1};
            Json want, got;
            Archive verify{want, catalog, definitions, false}, actual{got, catalog, definitions, false};
            verify.transfer(want, expected);
            actual.transfer(got, l.objective);
            if (want != got)
                invalid("Saved guardian definition mismatch");
            verify.transfer(want, recovery);
            actual.transfer(got, l.recovery);
            if (want != got)
                invalid("Saved supply definition mismatch");
            // The layout and shared geometry cannot describe different doors or protected cells.
            Json a, b;
            Archive aa{a, catalog, definitions, false}, bb{b, catalog, definitions, false};
            aa.transfer(a, l.fixtures);
            bb.transfer(b, l.layout.fixtures);
            if (a != b || !(l.entry == l.layout.spawn))
                invalid("Saved fixture/layout mismatch");
        }
    }
    return out;
}
} // namespace blobber
