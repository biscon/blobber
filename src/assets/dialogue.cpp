#include "assets/dialogue.hpp"
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
namespace blobber {
namespace {
using Json = nlohmann::json;
void fields(const Json& j, std::initializer_list<const char*> names) {
    if (!j.is_object())
        throw std::runtime_error("Expected an object");
    for (auto it = j.begin(); it != j.end(); ++it) {
        bool found = false;
        for (const auto* name : names)
            found |= it.key() == name;
        if (!found)
            throw std::runtime_error("Unknown field: " + it.key());
    }
}
std::string text(const Json& j, unsigned maximum, bool multiline = false) {
    const auto s = j.get<std::string>();
    if (s.empty() || s.size() > maximum)
        throw std::runtime_error("Text is empty or exceeds byte limit");
    for (const unsigned char c : s)
        if ((c < 32 || c > 126) && !(multiline && c == '\n'))
            throw std::runtime_error("Dialogue text must use printable ASCII (and newlines for pages)");
    return s;
}
DialogueKey key(const Json& j, std::set<DialogueKey>* unique = nullptr) {
    const auto s = text(j, 96);
    for (const auto c : s)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-'))
            throw std::runtime_error("IDs require lowercase letters, digits, dot, dash or underscore");
    const auto h = dialogueKey(s);
    if (!h || (unique && !unique->insert(h).second))
        throw std::runtime_error("Duplicate/colliding ID: " + s);
    return h;
}
DialogueResponse response(const Json& j, const std::set<DialogueKey>& facts) {
    fields(j, {"pages", "when_fact", "set_fact"});
    DialogueResponse r;
    const auto& pages = j.at("pages");
    if (!pages.is_array() || pages.empty() || pages.size() > 8)
        throw std::runtime_error("pages requires 1..8 strings");
    for (const auto& p : pages)
        r.pages.push_back(text(p, 1024, true));
    if (j.contains("when_fact"))
        r.whenFact = key(j.at("when_fact"));
    if (j.contains("set_fact"))
        r.setFact = key(j.at("set_fact"));
    for (const auto k : {r.whenFact, r.setFact})
        if (k && !facts.count(k))
            throw std::runtime_error("Unknown dialogue fact");
    return r;
}
} // namespace
DialogueCatalog loadDialogue(const std::filesystem::path& directory) {
    const auto path = directory / "dialogue.json";
    std::string location = "root";
    try {
        if (std::filesystem::file_size(path) > 1024 * 1024)
            throw std::runtime_error("Catalog exceeds 1 MiB");
        std::ifstream input(path);
        if (!input)
            throw std::runtime_error("Cannot open catalog");
        std::array<std::set<std::string>, 32> keys;
        auto j = Json::parse(input, [&](int depth, Json::parse_event_t event, Json& value) {
            if (depth < 0 || depth >= 32)
                throw std::runtime_error("Dialogue nesting exceeds limit");
            if (event == Json::parse_event_t::object_start)
                keys[unsigned(depth)].clear();
            if (event == Json::parse_event_t::key &&
                (depth < 1 || !keys[unsigned(depth - 1)].insert(value.get<std::string>()).second))
                throw std::runtime_error("Duplicate JSON key");
            return true;
        });
        fields(j, {"version", "presentation", "characters"});
        if (!j.at("version").is_number_integer() || j.at("version") != 1)
            throw std::runtime_error("Unsupported dialogue version");
        DialogueCatalog out;
        if (j.contains("presentation")) {
            const auto& p = j.at("presentation");
            fields(p, {"transition_ms", "fade_ms", "characters_per_second"});
            const auto integer = [&](const char* name, unsigned fallback, unsigned low, unsigned high) {
                if (!p.contains(name))
                    return fallback;
                const auto& value = p.at(name);
                if (!value.is_number_integer() || value < low || value > high)
                    throw std::runtime_error(std::string("Invalid presentation field: ") + name);
                return value.get<unsigned>();
            };
            out.transitionMs = integer("transition_ms", 700, 100, 2000);
            out.fadeMs = integer("fade_ms", 150, 1, out.transitionMs);
            out.charactersPerSecond = integer("characters_per_second", 45, 1, 240);
            if (out.fadeMs > out.transitionMs)
                throw std::runtime_error("presentation.fade_ms exceeds transition_ms");
        }
        const auto& characters = j.at("characters");
        if (!characters.is_array() || characters.empty() || characters.size() > 64)
            throw std::runtime_error("characters requires 1..64 entries");
        std::set<DialogueKey> identities;
        for (unsigned ci = 0; ci < characters.size(); ++ci) {
            location = "characters[" + std::to_string(ci) + "]";
            const auto& c = characters[ci];
            fields(c, {"id", "name", "facts", "greeting", "introduction", "topics", "camera"});
            DialogueCharacter character;
            character.id = key(c.at("id"), &identities);
            character.key = c.at("id").get<std::string>();
            character.name = text(c.at("name"), 48);
            std::set<DialogueKey> facts;
            if (c.contains("facts")) {
                if (!c.at("facts").is_array() || c.at("facts").size() > 64)
                    throw std::runtime_error("facts exceeds 64 entries");
                for (const auto& f : c.at("facts"))
                    key(f, &facts);
            }
            location = character.key + ".greeting";
            character.greeting = response(c.at("greeting"), facts);
            location = character.key + ".introduction";
            character.introduction = response(c.at("introduction"), facts);
            if (character.greeting.whenFact || character.greeting.setFact ||
                character.introduction.whenFact || character.introduction.setFact)
                throw std::runtime_error("Greeting/introduction cannot have fact conditions/effects");
            const auto& topics = c.at("topics");
            if (!topics.is_array() || topics.empty() || topics.size() > 16)
                throw std::runtime_error("topics requires 1..16 entries");
            std::set<DialogueKey> topicIds;
            for (unsigned ti = 0; ti < topics.size(); ++ti) {
                location = "characters[" + std::to_string(ci) + "].topics[" + std::to_string(ti) + "]";
                const auto& t = topics[ti];
                fields(t, {"id", "label", "kind", "responses"});
                DialogueTopic topic;
                topic.id = key(t.at("id"), &topicIds);
                topic.label = text(t.at("label"), 40);
                const auto kind = t.value("kind", std::string("response"));
                topic.introduction = kind == "introduction";
                topic.farewell = kind == "farewell";
                if (kind != "response" && !topic.introduction && !topic.farewell)
                    throw std::runtime_error("Unknown topic kind");
                if (kind == "response") {
                    const auto& responses = t.at("responses");
                    if (!responses.is_array() || responses.empty() || responses.size() > 2)
                        throw std::runtime_error("responses requires a default and at most one fact variant");
                    unsigned defaults = 0;
                    for (const auto& r : responses) {
                        topic.responses.push_back(response(r, facts));
                        defaults += !topic.responses.back().whenFact;
                    }
                    if (defaults != 1)
                        throw std::runtime_error("Topic requires exactly one default response");
                } else if (t.contains("responses"))
                    throw std::runtime_error("Special topic cannot have responses");
                character.topics.push_back(std::move(topic));
            }
            if (c.contains("camera")) {
                location = character.key + ".camera";
                const auto& camera = c.at("camera");
                fields(camera, {"distance", "eye_height"});
                character.distance = camera.value("distance", .6f);
                character.eyeHeight = camera.value("eye_height", 0.f);
                if (!std::isfinite(character.distance) || !std::isfinite(character.eyeHeight) ||
                    character.distance < .35f || character.distance > 2 ||
                    (camera.contains("eye_height") &&
                     (character.eyeHeight < .15f || character.eyeHeight > 1.5f)))
                    throw std::runtime_error("Invalid camera framing");
            }
            out.characters.push_back(std::move(character));
        }
        return out;
    } catch (const std::exception& e) {
        throw std::runtime_error(path.string() + " (" + location + "): " + e.what());
    }
}
} // namespace blobber
