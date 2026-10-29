#include "persistence/library_codec.hpp"
#include "assets/rules_json.hpp"
#include "persistence/starting_codec.hpp"
#include <set>

namespace blobber {
namespace {
using namespace rules;
using namespace rules_json;
std::uint64_t revision(const Json& j) {
    if (!j.is_number_unsigned() && (!j.is_number_integer() || j.get<std::int64_t>() < 0))
        fail("revision", "expected positive uint64");
    const auto v = j.get<std::uint64_t>();
    if (!v)
        fail("revision", "zero is reserved");
    return v;
}
Json spells(const Catalog& c, std::uint32_t mask) {
    Json j = Json::array();
    for (std::uint16_t i = 0; i < c.spellCount(); ++i)
        if (mask & spellBit({i}))
            j.push_back(c.spell({i})->id);
    return j;
}
std::uint32_t spells(const Catalog& c, const Json& j) {
    array(j, "spells", 32);
    std::uint32_t out = 0;
    for (const auto& v : j) {
        auto id = c.findSpell(identifier(v, "spell"));
        auto bit = spellBit(id);
        if (!bit || (out & bit))
            fail("spell", "unknown or duplicate spell");
        out |= bit;
    }
    return out;
}
Json gear(const Catalog& c, const CharacterBuild& b) {
    Json items = Json::array();
    for (std::size_t i = 0; i < b.itemCount; ++i)
        items.push_back({{"item", c.item(b.items[i].item)->id}, {"quantity", b.items[i].quantity}});
    auto slot = [](OwnedItemId id) -> Json {
        return id.index == UINT16_MAX ? Json(nullptr) : Json(id.index);
    };
    return {{"items", items},
            {"equipment",
             {{"body", slot(b.equipment.body)},
              {"main_hand", slot(b.equipment.mainHand)},
              {"off_hand", slot(b.equipment.offHand)},
              {"ranged", slot(b.equipment.ranged)}}}};
}
} // namespace
Json encodeStartingDraft(const Catalog& c, const CharacterDraft& d, const std::string& id) {
    const auto b = finishBuild(c, d, id);
    const auto& s = d.spells;
    const auto* options = c.creation(d.characterClass);
    Json choices = Json::array();
    for (std::size_t g = 0; g < options->groupCount; ++g)
        choices.push_back(options->groups[g].options[d.gear[g]].id);
    return {{"id", id},
            {"name", d.name.data()},
            {"race", c.race(d.race)->id},
            {"class", c.characterClass(d.characterClass)->id},
            {"portrait", portrait(d.portrait)->id},
            {"abilities", d.abilities},
            {"level", 1},
            {"defense", d.defense},
            {"life_domain", d.lifeDomain},
            {"spells",
             {{"cantrips", spells(c, s.cantrips)},
              {"learned", spells(c, s.learned)},
              {"prepared", spells(c, s.prepared)},
              {"domain", spells(c, s.domain)},
              {"racial", spells(c, s.racial)}}},
            {"choices", choices},
            {"gear", gear(c, b)}};
}
CharacterDraft decodeStartingDraft(const Catalog& c, const Json& j) {
    fields(j, "character",
           {"id", "name", "race", "class", "portrait", "abilities", "level", "defense", "life_domain",
            "spells", "choices", "gear"});
    const auto id = string(j.at("id"), "id");
    if (id.empty() || id.size() > 128)
        fail("id", "invalid character identity");
    CharacterDraft d;
    const auto name = string(j.at("name"), "name");
    if (!validDisplayText(name, 64))
        fail("name", "invalid printable name");
    std::copy(name.begin(), name.end(), d.name.begin());
    d.race = c.findRace(identifier(j.at("race"), "race"));
    d.characterClass = c.findClass(identifier(j.at("class"), "class"));
    d.portrait = findPortrait(identifier(j.at("portrait"), "portrait"));
    array(j.at("abilities"), "abilities", 6);
    if (j.at("abilities").size() != 6)
        fail("abilities", "expected six values");
    for (unsigned a = 0; a < 6; ++a)
        d.abilities[a] = integer(j.at("abilities")[a], "ability");
    if (integer(j.at("level"), "level") != 1)
        fail("level", "only level-one blueprints");
    if (!j.at("defense").is_boolean() || !j.at("life_domain").is_boolean())
        fail("features", "expected booleans");
    d.defense = j.at("defense");
    d.lifeDomain = j.at("life_domain");
    const auto& s = j.at("spells");
    fields(s, "spells", {"cantrips", "learned", "prepared", "domain", "racial"});
    d.spells = {spells(c, s.at("cantrips")), spells(c, s.at("learned")), spells(c, s.at("prepared")),
                spells(c, s.at("domain")), spells(c, s.at("racial"))};
    const auto* options = c.creation(d.characterClass);
    if (!options)
        fail("class", "missing creation content");
    array(j.at("choices"), "choices", 3);
    if (j.at("choices").size() != options->groupCount)
        fail("choices", "wrong choice count");
    for (std::size_t g = 0; g < options->groupCount; ++g) {
        const auto option = identifier(j.at("choices")[g], "choice");
        bool found = false;
        for (std::uint8_t n = 0; n < options->groups[g].count; ++n)
            if (options->groups[g].options[n].id == option) {
                d.gear[g] = n;
                found = true;
            }
        if (!found)
            fail("choice", "unavailable gear choice");
    }
    if (!previewBuild(c, d))
        fail("build", "invalid starting build");
    const auto b = finishBuild(c, d, id);
    if (j.at("gear") != gear(c, b))
        fail("gear", "saved gear differs from validated starting choices");
    return d;
}
std::string encodeLibrary(const rules::Catalog& c, const LibrarySnapshot& state) {
    using namespace rules;
    Json records = Json::array();
    for (std::size_t i = 0; i < state.count; ++i) {
        const auto& r = state.records[i];
        records.push_back(encodeStartingDraft(c, r.draft, r.id.text.data()));
    }
    std::string text = Json{{"schema_version", 1},
                            {"initialized", true},
                            {"revision", state.revision},
                            {"characters", records}}
                           .dump(2) +
                       "\n";
    if (text.size() > libraryMaxBytes)
        throw LibraryException(LibraryError::Full, "Serialized library exceeds 1 MiB");
    return text;
}
LibrarySnapshot decodeLibrary(const rules::Catalog& c, const std::string& text) {
    using namespace rules;
    if (text.size() > libraryMaxBytes)
        throw LibraryException(LibraryError::Invalid, "Library exceeds 1 MiB");
    std::array<std::set<std::string>, 17> keys;
    auto callback = [&](int depth, Json::parse_event_t event, Json& value) {
        if (depth > 16)
            fail("json", "nesting exceeds 16");
        if (event == Json::parse_event_t::object_start)
            keys[std::size_t(depth)].clear();
        if (event == Json::parse_event_t::key &&
            !keys[std::size_t(depth - 1)].insert(value.get<std::string>()).second)
            fail("json", "duplicate object key");
        return true;
    };
    const auto root = Json::parse(text, callback);
    if (!root.contains("schema_version") || !root.at("schema_version").is_number_integer())
        fail("schema_version", "missing integer version");
    if (root.at("schema_version") != 1)
        throw LibraryException(LibraryError::Unsupported,
                               "Unsupported character library version; original retained");
    fields(root, "library", {"schema_version", "initialized", "revision", "characters"});
    if (root.at("initialized") != true)
        fail("initialized", "expected true");
    LibrarySnapshot out;
    out.revision = revision(root.at("revision"));
    array(root.at("characters"), "characters", LibrarySnapshot::capacity);
    for (const auto& j : root.at("characters")) {
        fields(j, "character",
               {"id", "name", "race", "class", "portrait", "abilities", "level", "defense", "life_domain",
                "spells", "choices", "gear"});
        SavedCharacter r;
        const auto id = string(j.at("id"), "id");
        if (id.size() != 36)
            fail("id", "expected UUIDv4");
        std::copy(id.begin(), id.end(), r.id.text.begin());
        if (!validSavedCharacterId(r.id) || out.find(r.id))
            fail("id", "invalid or duplicate UUIDv4");
        r.draft = decodeStartingDraft(c, j);
        out.records[out.count++] = r;
    }
    return out;
}
} // namespace blobber
