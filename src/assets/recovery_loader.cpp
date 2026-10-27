#include "assets/recovery_loader.hpp"
#include "assets/rules_json.hpp"
#include <fstream>
#include <set>
namespace blobber {
RecoveryLevel loadRecoveryLevel(const std::filesystem::path& path, const Map& map,
                                const rules::Catalog& catalog) {
    using namespace rules_json;
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        fail("recovery", "cannot open " + path.string());
    std::string text;
    char block[4096];
    while (stream.read(block, sizeof(block)) || stream.gcount()) {
        if (text.size() + stream.gcount() > 65536)
            fail("recovery", "exceeds 64 KiB");
        text.append(block, static_cast<std::size_t>(stream.gcount()));
    }
    if (stream.bad())
        fail("recovery", "read failure");
    std::array<std::set<std::string>, 9> keys;
    const auto root = Json::parse(text, [&](int depth, Json::parse_event_t event, Json& value) {
        if (depth > 8)
            fail("recovery", "nesting exceeds 8");
        if (event == Json::parse_event_t::object_start)
            keys[depth].clear();
        if (event == Json::parse_event_t::key && !keys[depth - 1].insert(value.get<std::string>()).second)
            fail("recovery", "duplicate key");
        return true;
    });
    fields(root, "recovery", {"schema_version", "sanctuaries", "ground"});
    if (integer(root.at("schema_version"), "recovery.schema_version") != 1)
        fail("recovery", "unsupported version");
    const auto cell = [&](const Json& j) {
        array(j, "cell", 2);
        if (j.size() != 2)
            fail("cell", "expected two coordinates");
        Cell c{integer(j[0], "cell.x"), integer(j[1], "cell.y")};
        if (map.solid(c))
            fail("cell", "must be walkable");
        return c;
    };
    RecoveryLevel out;
    array(root.at("sanctuaries"), "sanctuaries", out.sanctuaries.size());
    for (const auto& j : root.at("sanctuaries")) {
        const auto c = cell(j);
        for (std::size_t i = 0; i < out.sanctuaryCount; ++i)
            if (out.sanctuaries[i] == c)
                fail("sanctuaries", "duplicate cell");
        out.sanctuaries[out.sanctuaryCount++] = c;
    }
    array(root.at("ground"), "ground", out.ground.size());
    for (const auto& j : root.at("ground")) {
        fields(j, "ground[]", {"cell", "item", "quantity"});
        const auto id = catalog.findItem(identifier(j.at("item"), "ground.item"));
        const auto* d = catalog.item(id);
        const int q = integer(j.at("quantity"), "ground.quantity");
        if (!d || q < 1 || unsigned(q) > rules::stackLimit(*d))
            fail("ground", "invalid item or quantity");
        out.ground[out.groundCount++] = {cell(j.at("cell")), id, static_cast<std::uint16_t>(q)};
    }
    return out;
}
} // namespace blobber
