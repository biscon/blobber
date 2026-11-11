#include "persistence/campaign_codec.hpp"
#include "assets/fingerprint.hpp"
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
        return decorationKinds - 1;
    else if constexpr (std::is_same_v<T, Heading>)
        return 3;
    else if constexpr (std::is_same_v<T, NpcAnimation>)
        return npcAnimationCount - 1;
    else if constexpr (std::is_same_v<T, NpcRoutine>)
        return unsigned(NpcRoutine::Wander);
    else if constexpr (std::is_same_v<T, NpcRole>)
        return unsigned(NpcRole::Resident);
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
    else if constexpr (std::is_same_v<T, generation::BiomeId> || std::is_same_v<T, generation::AlgorithmId>)
        return 0;
    else if constexpr (std::is_same_v<T, generation::LayoutProfile>)
        return 1;
    else if constexpr (std::is_same_v<T, blueprints::Purpose>)
        return 3;
    else if constexpr (std::is_same_v<T, generation::Role>)
        return unsigned(generation::Role::Optional);
    else if constexpr (std::is_same_v<T, generation::ActorRecipe>)
        return unsigned(generation::ActorRecipe::BugbearCommander);
    else if constexpr (std::is_same_v<T, generation::EncounterRecipe>)
        return unsigned(generation::EncounterRecipe::FinalGuardians);
    else if constexpr (std::is_same_v<T, generation::EncounterRole>)
        return unsigned(generation::EncounterRole::Guardian);
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
        } else if constexpr (std::is_same_v<T, blobber::Mt19937> || std::is_same_v<T, rules::DiceRng>) {
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
                if constexpr (std::is_same_v<T, blobber::Mt19937>) {
                    if (!(in >> v))
                        invalid("Invalid RNG");
                } else if (!v.restore(in))
                    invalid("Invalid dice RNG");
            } else {
                std::ostringstream out;
                out.imbue(std::locale::classic());
                if constexpr (std::is_same_v<T, blobber::Mt19937>)
                    out << v;
                else
                    v.describe(out);
                j = out.str();
            }
        } else {
            static_assert(!sizeof(T), "Composite save values must use the sparse archive");
        }
    }
};
#include "persistence/campaign_delta.hpp"
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
    if (!content.blueprints || !j.is_object() || j.size() != 12 || !j.at("schema").is_number_integer() ||
        !j.at("simulation").is_number_integer() || !j.at("generator").is_number_integer() ||
        !j.at("templates").is_number_integer() || j.at("schema") != 22 || j.at("simulation") != 24 ||
        j.at("generator") != generation::generatorVersion ||
        j.at("templates") != generation::templateVersion || j.at("catalog") != content.catalog ||
        j.at("npcs") != std::to_string(content.npcs) ||
        j.at("blueprints") != std::to_string(blueprints::fingerprint(*content.blueprints)) ||
        !j.contains("campaign") || !j.at("campaign").is_object() || !j.at("seed").is_number_unsigned() ||
        j.at("seed").get<std::uint64_t>() > UINT32_MAX || !j.at("baseline").is_array() ||
        j.at("baseline").size() != campaignLocationCount)
        invalid("Incompatible campaign schema, simulation or content");
    const auto& profile = j.at("reward_profile");
    if (!profile.is_array() || profile.size() != 4 || !profile[0].is_number_unsigned() ||
        profile[0].get<std::uint64_t>() > rules::allWeaponTraining || !profile[1].is_boolean() ||
        !profile[2].is_boolean() || !profile[3].is_boolean())
        invalid("Invalid reward profile");
    for (const auto& signature : j.at("baseline"))
        if (!signature.is_string() || signature.get_ref<const std::string&>().empty() ||
            signature.get_ref<const std::string&>().size() > 20 ||
            signature.get_ref<const std::string&>().find_first_not_of("0123456789") != std::string::npos)
            invalid("Invalid campaign baseline fingerprint");
}
} // namespace
std::string savedCampaignRunId(const std::string& text) {
    const auto j = parse(text);
    const auto& campaign = j.at("campaign");
    const auto id = campaign.at("runId").get<std::string>();
    if (id.size() != 32 || id.find_first_not_of("0123456789abcdef") != std::string::npos)
        invalid("Invalid saved campaign run identity");
    return id;
}
namespace {
std::string definitionSignature(const CampaignLocationState& location) {
    auto h = fingerprintBasis;
    for (auto value : {location.worldFingerprint, location.fingerprint})
        for (unsigned byte = 0; byte < 8; ++byte) {
            const auto part = static_cast<unsigned char>(value >> (byte * 8));
            fingerprintBytes(h, &part, 1);
        }
    for (const auto& row : location.rows)
        fingerprintBytes(h, row.data(), row.size());
    const auto reward = [&](const GroundPlacement& item) {
        for (auto value : {unsigned(item.item.index), unsigned(item.quantity), unsigned(item.container)})
            for (unsigned byte = 0; byte < 4; ++byte) {
                const auto part = static_cast<unsigned char>(value >> (byte * 8));
                fingerprintBytes(h, &part, 1);
            }
    };
    for (unsigned i = 0; i < location.recovery.groundCount; ++i)
        reward(location.recovery.ground[i]);
    for (unsigned i = 0; i < location.objective.rewardCount; ++i)
        reward(location.objective.rewards[i]);
    return std::to_string(h);
}
Json signatures(const CampaignDefinitions& definitions) {
    Json out = Json::array();
    for (const auto& location : definitions.locations)
        out.push_back(definitionSignature(location));
    return out;
}
void cloneDefinition(CampaignLocationState& to, const CampaignLocationState& from) {
    to.generated = from.generated;
    to.generation = from.generation;
    to.layout = from.layout;
    to.diagnostics = from.diagnostics;
    to.fingerprint = from.fingerprint;
    to.world = from.world;
    to.worldFingerprint = from.worldFingerprint;
    to.rows = from.rows;
    to.types = from.types;
    to.placements = from.placements;
    to.initialActors = from.initialActors;
    to.recovery = from.recovery;
    to.fixtures = from.fixtures;
    to.objective = from.objective;
    to.entry = from.entry;
    to.exit = from.exit;
    to.seed = from.seed;
}
template <class T>
std::string encode(const rules::Catalog& catalog, const NpcDefinitions& definitions,
                   const CampaignContent& content, T& value, const T& baseline) {
    if (!value.baseline || value.runId.size() != 32 ||
        value.runId.find_first_not_of("0123456789abcdef") != std::string::npos ||
        value.activeLocation >= campaignLocationCount)
        invalid("Invalid campaign capture");
    Json summary = {{"location", value.activeLocation}, {"party", Json::array()}};
    for (const auto& member : value.party.state.characters)
        summary["party"].push_back(member.build.name);
    if (!content.blueprints)
        invalid("Missing blueprint catalog");
    Json j = {{"schema", 22},
              {"blueprints", std::to_string(blueprints::fingerprint(*content.blueprints))},
              {"simulation", 24},
              {"generator", generation::generatorVersion},
              {"templates", generation::templateVersion},
              {"catalog", content.catalog},
              {"npcs", std::to_string(content.npcs)},
              {"seed", value.baseline->seed},
              {"reward_profile",
               {value.baseline->rewardProfile.weapons, value.baseline->rewardProfile.spellAttack,
                value.baseline->rewardProfile.manaCaster, value.baseline->rewardProfile.unarmored}},
              {"baseline", signatures(*value.baseline)},
              {"summary", std::move(summary)}};
    auto scratch = std::make_unique<FloorState>();
    DeltaArchive archive{j, catalog, definitions, false, 0, scratch.get()};
    archive("campaign", value, baseline);
    auto text = j.dump() + "\n";
    if (text.size() > campaignMaxBytes)
        invalid("Campaign save exceeds 16 MiB");
    return text;
}
} // namespace
namespace {
CampaignSummary campaignSummary(const Json& j) {
    CampaignSummary out;
    out.runId = j.at("campaign").at("runId").get<std::string>();
    const auto& summary = j.at("summary");
    if (out.runId.size() != 32 || out.runId.find_first_not_of("0123456789abcdef") != std::string::npos ||
        !summary.is_object() || summary.size() != 2 || !summary.at("location").is_number_unsigned() ||
        summary.at("location").get<std::uint64_t>() >= campaignLocationCount ||
        !summary.at("party").is_array() || summary.at("party").size() != rules::partySize)
        invalid("Invalid campaign summary");
    out.location = summary.at("location").get<unsigned>();
    for (const auto& name : summary.at("party")) {
        if (!name.is_string() || !rules::validDisplayText(name.get<std::string>(), 64))
            invalid("Invalid saved party summary");
        if (!out.party.empty())
            out.party += ", ";
        out.party += name.get<std::string>();
    }
    return out;
}
} // namespace
CampaignSummary inspectCampaign(const CampaignContent& content, const std::string& text) {
    const auto j = parse(text);
    envelope(j, content);
    return campaignSummary(j);
}

std::string encodeCampaign(const rules::Catalog& catalog, const NpcDefinitions& definitions,
                           const CampaignContent& content, CampaignSaveData& value) {
    CampaignSaveData baseline;
    if (!value.baseline)
        invalid("Missing campaign baseline");
    for (unsigned i = 0; i < campaignLocationCount; ++i)
        baseline.locations[i].definition = &value.baseline->locations[i];
    return encode(catalog, definitions, content, value, baseline);
}
std::string encodeCampaign(const rules::Catalog& catalog, const NpcDefinitions& definitions,
                           const CampaignContent& content, CampaignCheckpoint& value) {
    auto baseline = std::make_unique<CampaignCheckpoint>();
    if (!value.baseline)
        invalid("Missing campaign baseline");
    for (unsigned i = 0; i < campaignLocationCount; ++i)
        cloneDefinition(baseline->locations[i], value.baseline->locations[i]);
    return encode(catalog, definitions, content, value, *baseline);
}
std::unique_ptr<CampaignCheckpoint> decodeCampaign(const rules::Catalog& catalog,
                                                   const NpcDefinitions& definitions,
                                                   const CampaignContent& content, const std::string& text) {
    auto j = parse(text);
    envelope(j, content);
    if (!j.at("seed").is_number_unsigned() || j.at("seed").get<std::uint64_t>() > UINT32_MAX)
        invalid("Invalid campaign seed");
    auto out = std::make_unique<CampaignCheckpoint>();
    const auto& profile = j.at("reward_profile");
    out->baseline = prepareCampaignDefinitions(
        *content.blueprints, catalog, definitions.types, j["seed"].get<std::uint32_t>(),
        {profile[0].get<rules::WeaponTraining>(), profile[1].get<bool>(), profile[2].get<bool>(),
         profile[3].get<bool>()});
    if (j.at("baseline") != signatures(*out->baseline))
        invalid("Campaign definition baseline differs");
    auto baseline = std::make_unique<CampaignCheckpoint>();
    for (unsigned i = 0; i < campaignLocationCount; ++i) {
        cloneDefinition(out->locations[i], out->baseline->locations[i]);
        cloneDefinition(baseline->locations[i], out->baseline->locations[i]);
    }
    DeltaArchive archive{j["campaign"], catalog, definitions, true};
    archive.transfer(j["campaign"], *out, *baseline);
    const auto summary = campaignSummary(j);
    if (summary.runId != out->runId || summary.location != out->activeLocation)
        invalid("Campaign summary differs from state");
    for (unsigned i = 0; i < rules::partySize; ++i)
        if (j["summary"]["party"][i] != out->party.state.characters[i].build.name)
            invalid("Campaign party summary differs from state");
    for (unsigned i = 0; i < campaignLocationCount; ++i) {
        const auto& location = out->locations[i];
        const auto& original = out->baseline->locations[i];
        if (location.generated != original.generated || location.seed != original.seed ||
            location.types != original.types ||
            !CampaignEqual::same(location.generation, original.generation) ||
            !CampaignEqual::same(location.placements, original.placements) ||
            !CampaignEqual::same(location.recovery, original.recovery) ||
            !CampaignEqual::same(location.fixtures, original.fixtures) ||
            !CampaignEqual::same(location.objective, original.objective) ||
            !CampaignEqual::same(location.entry, original.entry) ||
            !CampaignEqual::same(location.exit, original.exit))
            invalid("Saved initial definition differs from its baseline");
        auto expectedLayout = original.layout;
        if (location.generated)
            expectedLayout.dressingFingerprint = location.worldFingerprint;
        if (!CampaignEqual::same(location.layout, expectedLayout))
            invalid("Saved generated layout differs from its baseline");
    }
    // Full logical admission is performed once by the restoring CampaignRun.
    return out;
}
} // namespace blobber
