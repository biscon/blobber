#include "assets/blueprint_loader.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
namespace blobber::blueprints {
namespace {
using Json = nlohmann::json;
void need(bool ok, const char* message) {
    if (!ok)
        throw Failure(Error::Content, message);
}
void fields(const Json& j, std::initializer_list<const char*> names,
            std::initializer_list<const char*> optional = {}) {
    need(j.is_object(), "Expected blueprint object");
    for (const auto& item : j.items())
        need(std::find(names.begin(), names.end(), item.key()) != names.end() ||
                 std::find(optional.begin(), optional.end(), item.key()) != optional.end(),
             "Unexpected blueprint field");
    for (const auto* name : names)
        need(j.contains(name), "Missing blueprint field");
}
int number(const Json& j, int low, int high) {
    need(j.is_number_integer(), "Expected blueprint integer");
    need(j >= low && j <= high, "Blueprint integer outside bound");
    return j.get<int>();
}
Name name(const Json& j) {
    need(j.is_string(), "Expected blueprint ID");
    const auto s = j.get<std::string>();
    need(!s.empty() && s.size() <= 32 &&
             std::all_of(s.begin(), s.end(),
                         [](char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-'; }),
         "Invalid blueprint ID");
    Name n{};
    std::copy(s.begin(), s.end(), n.begin());
    return n;
}
Cell cell(const Json& j, int low = -1, int high = 9) {
    need(j.is_array() && j.size() == 2, "Expected two cell coordinates");
    return {number(j[0], low, high), number(j[1], low, high)};
}
Heading heading(const Json& j) {
    need(j.is_string(), "Expected cardinal facing");
    const auto s = j.get<std::string>();
    for (unsigned i = 0; i < 4; ++i)
        if (s == std::array<const char*, 4>{"north", "east", "south", "west"}[i])
            return Heading(i);
    throw Failure(Error::Content, "Unknown cardinal facing");
}
Rect rectangle(const Json& j) {
    fields(j, {"origin", "size"});
    return {cell(j.at("origin"), 0, 8), cell(j.at("size"), 1, 9)};
}
template <class T, std::size_t N, class F> unsigned records(const Json& j, std::array<T, N>& out, F parse) {
    need(j.is_array(), "Expected blueprint array");
    if (j.size() > N)
        throw Failure(Error::Capacity, "Blueprint array capacity exceeded");
    unsigned i = 0;
    for (const auto& v : j)
        out[i++] = parse(v);
    return i;
}
} // namespace
BlueprintCatalog decode(std::string_view input) {
    if (input.size() > maxJsonBytes)
        throw Failure(Error::Capacity, "Blueprint JSON exceeds 256 KiB");
    try {
        // Reject duplicate keys before DOM construction can silently discard them.
        std::vector<std::set<std::string>> keys;
        auto callback = [&](int depth, Json::parse_event_t event, Json& parsed) {
            need(depth <= 32, "Blueprint JSON nesting exceeds 32");
            if (event == Json::parse_event_t::object_start)
                keys.emplace_back();
            if (event == Json::parse_event_t::key)
                need(keys.back().insert(parsed.get<std::string>()).second, "Duplicate JSON key");
            if (event == Json::parse_event_t::object_end)
                keys.pop_back();
            return true;
        };
        const auto j = Json::parse(input.begin(), input.end(), callback);
        fields(j, {"schema_version", "kits", "rooms"});
        need(number(j.at("schema_version"), 1, 1) == 1, "Unsupported blueprint schema");
        BlueprintCatalog c;
        c.kitCount = records(j.at("kits"), c.kits, [](const Json& v) {
            fields(v, {"id", "size", "height"}, {"blocking", "fire", "clearance", "closedWall"});
            need(v.at("height").is_number(), "Expected kit height");
            Kit k;
            k.id = name(v.at("id"));
            k.size = cell(v.at("size"), 1, 4);
            k.height = v.at("height").get<float>();
            if (v.contains("clearance")) {
                need(v["clearance"].is_number(), "Expected kit clearance");
                k.clearance = v["clearance"].get<float>();
            }
            if (v.contains("closedWall")) {
                need(v["closedWall"].is_boolean(), "Expected closedWall boolean");
                k.closedWall = v["closedWall"].get<bool>();
            }
            if (v.contains("blocking")) {
                need(v["blocking"].is_boolean(), "Expected blocking boolean");
                k.blocking = v["blocking"].get<bool>();
            }
            if (v.contains("fire")) {
                const auto& f = v["fire"];
                need(f.is_array() && f.size() == 3 && f[0].is_number() && f[1].is_number() &&
                         f[2].is_number(),
                     "Expected local fire anchor");
                k.fire = AmbientFire{f[0].get<float>(), f[1].get<float>(), f[2].get<float>()};
            }
            return k;
        });
        c.roomCount = records(j.at("rooms"), c.rooms, [&](const Json& v) {
            fields(v, {"id", "purpose", "size", "ceiling", "rotations", "solid", "placements", "sockets",
                       "lanes", "encounters", "rewards"});
            RoomBlueprint r;
            r.id = name(v.at("id"));
            r.size = cell(v.at("size"), 1, 9);
            const auto purpose = name(v.at("purpose"));
            const std::array<std::string_view, 4> purposes{"kitchen", "mead-hall", "barracks", "armory"};
            auto it = std::find(purposes.begin(), purposes.end(), purpose.data());
            need(it != purposes.end(), "Unknown room purpose");
            r.purpose = Purpose(it - purposes.begin());
            r.ceiling = unsigned(number(v.at("ceiling"), 4, 8));
            r.rotations = 0;
            const auto& rotations = v.at("rotations");
            need(rotations.is_array() && rotations.size() <= 4, "Invalid rotations");
            for (const auto& value : rotations) {
                const auto angle = number(value, 0, 270);
                need(angle % 90 == 0, "Rotation must be a quarter turn");
                const unsigned bit = 1u << unsigned(angle / 90);
                need(!(r.rotations & bit), "Duplicate rotation");
                r.rotations |= bit;
            }
            const auto& solid = v.at("solid");
            need(solid.is_array() && solid.size() <= maxCells, "Invalid solid mask");
            for (const auto& value : solid) {
                const auto p = cell(value, 0, 8);
                need(!r.solid[index(p)], "Duplicate solid cell");
                r.solid[index(p)] = true;
            }
            r.placementCount = records(v.at("placements"), r.placements, [&](const Json& p) {
                fields(p, {"kit", "anchor", "facing"});
                return Placement{c.kit(name(p.at("kit")).data()), cell(p.at("anchor"), 0, 8),
                                 heading(p.at("facing"))};
            });
            r.socketCount = records(v.at("sockets"), r.sockets, [](const Json& s) {
                fields(s, {"id", "cell", "outward"});
                return Socket{name(s.at("id")), cell(s.at("cell")), heading(s.at("outward"))};
            });
            r.laneCount = records(v.at("lanes"), r.lanes, rectangle);
            r.encounterCount = records(v.at("encounters"), r.encounters, rectangle);
            r.rewardCount = records(v.at("rewards"), r.rewards, [](const Json& a) {
                fields(a, {"cell", "access"});
                return Reward{cell(a.at("cell"), 0, 8), cell(a.at("access"), 0, 8)};
            });
            return r;
        });
        validate(c);
        return c;
    } catch (const Failure&) {
        throw;
    } catch (const std::exception& e) {
        throw Failure(Error::Content, std::string("Invalid blueprint JSON: ") + e.what());
    }
}
BlueprintCatalog load(const std::filesystem::path& path) {
    // Bound the read itself, including streams that grow after stat.
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw Failure(Error::Content, "Cannot open blueprint catalog: " + path.string());
    std::string text(maxJsonBytes + 1, '\0');
    file.read(text.data(), std::streamsize(text.size()));
    if (file.bad())
        throw Failure(Error::Content, "Cannot read blueprint catalog");
    text.resize(std::size_t(file.gcount()));
    return decode(text);
}
} // namespace blobber::blueprints
