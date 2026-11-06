#include "assets/dialogue.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace blobber {
namespace {
using Node = YAML::Node;
void fields(const Node& n, std::initializer_list<const char*> allowed) {
    if (!n.IsMap())
        throw std::runtime_error("Expected a mapping");
    std::set<std::string> seen;
    for (const auto& entry : n) {
        const auto name = entry.first.as<std::string>();
        if (!seen.insert(name).second)
            throw std::runtime_error("Duplicate field: " + name);
        if (std::none_of(allowed.begin(), allowed.end(), [&](auto a) { return name == a; }))
            throw std::runtime_error("Unknown field: " + name);
    }
}
void bounded(const Node& n, unsigned depth, unsigned& visits) {
    if (depth > 24 || ++visits > 65536)
        throw std::runtime_error("YAML nesting/structure exceeds limit");
    if (n.IsMap())
        for (const auto& e : n) {
            bounded(e.first, depth + 1, visits);
            bounded(e.second, depth + 1, visits);
        }
    else if (n.IsSequence())
        for (const auto& e : n)
            bounded(e, depth + 1, visits);
}
void sequence(const Node& n, unsigned low, unsigned high, const char* name) {
    if (!n.IsSequence() || n.size() < low || n.size() > high)
        throw std::runtime_error(std::string(name) + " has invalid entry count");
}
std::string text(const Node& n, unsigned maximum, bool multiline = false) {
    if (!n.IsScalar())
        throw std::runtime_error("Expected text");
    const auto s = n.as<std::string>();
    if (s.empty() || s.size() > maximum)
        throw std::runtime_error("Text is empty or exceeds byte limit");
    for (unsigned char c : s)
        if ((c < 32 || c > 126) && !(multiline && c == '\n'))
            throw std::runtime_error("Dialogue text requires printable ASCII (and newlines for speech)");
    return s;
}
DialogueKey key(const Node& n, std::set<DialogueKey>* unique = nullptr) {
    const auto s = text(n, 96);
    for (char c : s)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-'))
            throw std::runtime_error("IDs require lowercase letters, digits, dot, dash or underscore");
    const auto k = dialogueKey(s);
    if (!k || (unique && !unique->insert(k).second))
        throw std::runtime_error("Duplicate/colliding ID: " + s);
    return k;
}
unsigned integer(const Node& n, unsigned low, unsigned high) {
    const auto s = text(n, 10);
    if (s.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error("Expected integer");
    const auto v = std::stoull(s);
    if (v < low || v > high)
        throw std::runtime_error("Integer outside allowed range");
    return unsigned(v);
}
void framing(const Node& n, DialogueFraming& f) {
    if (!n)
        return;
    fields(n, {"distance", "eye_height"});
    if (n["distance"])
        f.distance = n["distance"].as<float>();
    if (n["eye_height"])
        f.eyeHeight = n["eye_height"].as<float>();
    if (!std::isfinite(f.distance) || f.distance < .35f || f.distance > 2 || !std::isfinite(f.eyeHeight) ||
        (n["eye_height"] && (f.eyeHeight < .15f || f.eyeHeight > 1.5f)))
        throw std::runtime_error("Invalid camera framing");
}
unsigned participant(const Node& n, const DialogueCharacter& c) {
    const auto alias = text(n, 96);
    for (unsigned i = 0; i < c.participants.size(); ++i)
        if (c.participants[i].alias == alias)
            return i;
    throw std::runtime_error("Unknown participant: " + alias);
}
unsigned asset(const Node& n, const std::filesystem::path& directory, std::vector<std::string>& files,
               unsigned capacity) {
    const auto name = text(n, 240);
    const std::filesystem::path relative(name);
    if (relative.is_absolute() || relative.has_root_name())
        throw std::runtime_error("Asset path must be relative");
    const auto root = std::filesystem::weakly_canonical(directory);
    const auto path = std::filesystem::weakly_canonical(root / relative);
    auto r = root.begin(), p = path.begin();
    for (; r != root.end() && p != path.end() && *r == *p; ++r, ++p) {
    }
    if (r != root.end() || p == path.end())
        throw std::runtime_error("Asset path escapes its directory");
    const auto resolved = path.string();
    auto found = std::find(files.begin(), files.end(), resolved);
    if (found != files.end())
        return unsigned(found - files.begin());
    if (files.size() == capacity)
        throw std::runtime_error("Dialogue asset capacity exceeded");
    files.push_back(resolved);
    return unsigned(files.size() - 1);
}
DialogueResponse response(const Node& n, const std::set<DialogueKey>& facts, const DialogueCharacter& c,
                          const std::filesystem::path& directory, const std::filesystem::path& uiRoot,
                          DialogueCatalog& out) {
    fields(n, {"beats", "when_fact", "set_fact"});
    DialogueResponse r;
    if (n["when_fact"])
        r.whenFact = key(n["when_fact"]);
    if (n["set_fact"])
        r.setFact = key(n["set_fact"]);
    for (auto k : {r.whenFact, r.setFact})
        if (k && !facts.count(k))
            throw std::runtime_error("Unknown fact");
    const auto beats = n["beats"];
    sequence(beats, 1, 32, "beats");
    for (const auto& b : beats) {
        try {
            fields(b, {"speaker", "text", "camera", "animation", "audio", "overlay_asset", "clear_overlay",
                       "overlay_delay_ms", "overlay_dismiss_on_advance"});
            DialogueBeat beat;
            beat.speaker = participant(b["speaker"], c);
            beat.target = beat.speaker;
            beat.text = text(b["text"], 1024, true);
            if (b["camera"]) {
                const auto camera = b["camera"];
                fields(camera, {"target", "duration_ms"});
                if (camera["target"])
                    beat.target = participant(camera["target"], c);
                if (camera["duration_ms"])
                    beat.durationMs = integer(camera["duration_ms"], 0, 2000);
            }
            if (b["animation"]) {
                const auto clip = text(b["animation"], 16);
                if (clip == "idle")
                    beat.animation = NpcAnimation::Idle;
                else if (clip == "talk")
                    beat.animation = NpcAnimation::Talk;
                else if (clip == "walk")
                    beat.animation = NpcAnimation::Walk;
                else if (clip == "laugh")
                    beat.animation = NpcAnimation::Laugh;
                else if (clip == "pull_document")
                    beat.animation = NpcAnimation::PullDocument;
                else if (clip == "explosive_rage")
                    beat.animation = NpcAnimation::ExplosiveRage;
                else
                    throw std::runtime_error("Unknown dialogue animation: " + clip);
            }
            if (b["audio"])
                beat.audio = asset(b["audio"], directory, out.audioFiles, 64);
            if (b["overlay_asset"] && b["clear_overlay"])
                throw std::runtime_error("overlay_asset and clear_overlay are mutually exclusive");
            if (b["overlay_asset"])
                beat.overlay.index =
                    asset(b["overlay_asset"], uiRoot, out.overlayFiles, dialogueOverlayCapacity);
            if ((b["overlay_delay_ms"] || b["overlay_dismiss_on_advance"]) && !b["overlay_asset"])
                throw std::runtime_error("Overlay timing/dismissal requires overlay_asset");
            if (b["overlay_delay_ms"])
                beat.overlayDelayMs = integer(b["overlay_delay_ms"], 0, 60000);
            if (b["overlay_dismiss_on_advance"]) {
                const auto value = text(b["overlay_dismiss_on_advance"], 5);
                if (value != "true" && value != "false")
                    throw std::runtime_error("overlay_dismiss_on_advance requires true or false");
                beat.overlayDismissOnAdvance = value == "true";
            }
            if (b["clear_overlay"]) {
                const auto value = text(b["clear_overlay"], 5);
                if (value != "true" && value != "false")
                    throw std::runtime_error("clear_overlay requires true or false");
                beat.clearOverlay = value == "true";
            }
            r.pageBeats.push_back(unsigned(r.beats.size()));
            r.pages.push_back(beat.text);
            r.beats.push_back(std::move(beat));
        } catch (const std::exception& e) {
            throw std::runtime_error("beat " + std::to_string(r.beats.size() + 1) + " (line " +
                                     std::to_string(b.Mark().line + 1) + "): " + e.what());
        }
    }
    return r;
}
} // namespace
DialogueCatalog loadDialogue(const std::filesystem::path& directory,
                             const std::filesystem::path& audioDirectory,
                             const std::filesystem::path& uiDirectory) {
    const auto uiRoot = uiDirectory.empty() ? directory.parent_path() / "ui" : uiDirectory;
    const auto audioRoot = audioDirectory.empty() ? directory.parent_path() / "audio" : audioDirectory;
    std::vector<std::filesystem::path> paths;
    std::uintmax_t bytes = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file())
            continue;
        const auto extension = entry.path().extension();
        if (extension != ".yaml" && extension != ".yml")
            continue;
        bytes += entry.file_size();
        if (bytes > 1024 * 1024 || paths.size() == 64)
            throw std::runtime_error("Dialogue catalog capacity exceeded");
        paths.push_back(entry.path());
    }
    if (paths.empty())
        throw std::runtime_error(directory.string() + ": no YAML conversation files");
    std::sort(paths.begin(), paths.end());
    DialogueCatalog out;
    std::set<DialogueKey> identities;
    bool presentationSeen = false;
    for (const auto& path : paths) {
        std::string location = "root";
        try {
            const auto documents = YAML::LoadAllFromFile(path.string());
            if (documents.size() != 1)
                throw std::runtime_error("Expected one YAML document");
            const auto n = documents.front();
            unsigned visits = 0;
            bounded(n, 0, visits);
            fields(n, {"version", "primary", "participants", "presentation", "facts", "greeting",
                       "introduction", "topics"});
            if (integer(n["version"], 2, 2) != 2)
                throw std::runtime_error("Unsupported version");
            if (n["presentation"]) {
                const auto p = n["presentation"];
                fields(p, {"transition_ms", "fade_ms", "characters_per_second"});
                const unsigned transition =
                    p["transition_ms"] ? integer(p["transition_ms"], dialogueMinimumPanMs, 2000) : 700;
                const unsigned fade = p["fade_ms"] ? integer(p["fade_ms"], 1, transition) : 150;
                const unsigned cps = p["characters_per_second"] ? integer(p["characters_per_second"], 1, 240)
                                                                : dialogueDefaultCharactersPerSecond;
                if (fade > transition ||
                    (presentationSeen && (out.transitionMs != transition || out.fadeMs != fade ||
                                          out.charactersPerSecond != cps)))
                    throw std::runtime_error("Inconsistent presentation defaults");
                out.transitionMs = transition;
                out.fadeMs = fade;
                out.charactersPerSecond = cps;
                presentationSeen = true;
            }
            DialogueCharacter c;
            const auto cast = n["participants"];
            if (!cast.IsMap() || !cast.size() || cast.size() > 8)
                throw std::runtime_error("participants requires 1..8 entries");
            std::set<DialogueKey> aliases, residents;
            for (const auto& entry : cast) {
                key(entry.first, &aliases);
                location = "participants." + entry.first.as<std::string>();
                fields(entry.second, {"resident", "name", "text_color", "camera"});
                DialogueParticipant p;
                p.alias = entry.first.as<std::string>();
                p.id = key(entry.second["resident"], &residents);
                p.name = text(entry.second["name"], 48);
                if (entry.second["text_color"]) {
                    const auto color = text(entry.second["text_color"], 7);
                    if (color.size() != 7 || color[0] != '#' ||
                        color.find_first_not_of("0123456789abcdefABCDEF", 1) != std::string::npos)
                        throw std::runtime_error("text_color requires #RRGGBB");
                    p.textColor = (std::uint32_t(std::stoul(color.substr(1), nullptr, 16)) << 8) | 255;
                }
                framing(entry.second["camera"], p);
                c.participants.push_back(std::move(p));
            }
            c.primary = participant(n["primary"], c);
            const auto& host = c.participants[c.primary];
            c.id = host.id;
            if (!identities.insert(c.id).second)
                throw std::runtime_error("Duplicate conversation host");
            c.key = cast[host.alias]["resident"].as<std::string>();
            c.name = host.name;
            c.distance = host.distance;
            c.eyeHeight = host.eyeHeight;
            std::set<DialogueKey> facts;
            if (n["facts"]) {
                sequence(n["facts"], 0, 64, "facts");
                for (const auto& f : n["facts"])
                    key(f, &facts);
            }
            location = "greeting";
            c.greeting = response(n["greeting"], facts, c, audioRoot, uiRoot, out);
            location = "introduction";
            c.introduction = response(n["introduction"], facts, c, audioRoot, uiRoot, out);
            if (c.greeting.whenFact || c.greeting.setFact || c.introduction.whenFact ||
                c.introduction.setFact)
                throw std::runtime_error("Greeting/introduction cannot have fact conditions/effects");
            sequence(n["topics"], 1, 16, "topics");
            std::set<DialogueKey> topics;
            for (const auto& t : n["topics"]) {
                location = "topics[" + std::to_string(c.topics.size()) + "]";
                fields(t, {"id", "label", "kind", "responses"});
                DialogueTopic topic;
                topic.id = key(t["id"], &topics);
                topic.label = text(t["label"], 40);
                const auto kind = t["kind"] ? text(t["kind"], 16) : "response";
                topic.introduction = kind == "introduction";
                topic.farewell = kind == "farewell";
                if (kind == "response") {
                    sequence(t["responses"], 1, 2, "responses");
                    unsigned defaults = 0;
                    for (const auto& r : t["responses"]) {
                        topic.responses.push_back(response(r, facts, c, audioRoot, uiRoot, out));
                        defaults += !topic.responses.back().whenFact;
                    }
                    if (defaults != 1)
                        throw std::runtime_error("Topic needs exactly one default response");
                } else if ((!topic.introduction && !topic.farewell) || t["responses"])
                    throw std::runtime_error("Invalid special topic");
                c.topics.push_back(std::move(topic));
            }
            out.characters.push_back(std::move(c));
        } catch (const std::exception& e) {
            throw std::runtime_error(path.string() + " (" + location + "): " + e.what());
        }
    }
    return out;
}
} // namespace blobber
