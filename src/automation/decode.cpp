#include "automation/codec.hpp"
#include <charconv>
#include <set>
#include <stdexcept>
namespace blobber::automation {
unsigned bounded(const Json& j, unsigned max) {
    if (!j.is_number_integer() ||
        (j.is_number_unsigned() ? j.get<std::uint64_t>() > max
                                : j.get<std::int64_t>() < 0 || std::uint64_t(j.get<std::int64_t>()) > max))
        throw std::invalid_argument("Integer outside allowed range");
    return j.get<unsigned>();
}
std::uint64_t serial(const Json& j) {
    if (!j.is_string())
        throw std::invalid_argument("Identity/revision must be a decimal string");
    const auto& s = j.get_ref<const std::string&>();
    std::uint64_t value = 0;
    const auto result = std::from_chars(s.data(), s.data() + s.size(), value);
    if (s.empty() || result.ec != std::errc{} || result.ptr != s.data() + s.size() ||
        (s.size() > 1 && s[0] == '0'))
        throw std::invalid_argument("Invalid identity/revision");
    return value;
}
void fields(const Json& j, std::initializer_list<const char*> names) {
    if (!j.is_object())
        throw std::invalid_argument("Expected object");
    for (auto it = j.begin(); it != j.end(); ++it) {
        bool found = false;
        for (const auto* name : names)
            found |= it.key() == name;
        if (!found)
            throw std::invalid_argument("Unknown field: " + it.key());
    }
}
namespace {
Json parseDocument(const std::string& line, std::size_t maxBytes, unsigned maxDepth) {
    if (line.size() > maxBytes)
        throw std::invalid_argument("JSON document exceeds byte limit");
    // Check structural depth before asking the JSON DOM parser to allocate recursively.
    unsigned depth = 0;
    bool quoted = false, escape = false;
    for (char c : line) {
        if (quoted) {
            if (escape)
                escape = false;
            else if (c == '\\')
                escape = true;
            else if (c == '"')
                quoted = false;
        } else if (c == '"')
            quoted = true;
        else if (c == '{' || c == '[') {
            if (++depth > maxDepth)
                throw std::invalid_argument("JSON nesting exceeds limit");
        } else if (c == '}' || c == ']') {
            if (!depth)
                throw std::invalid_argument("Unbalanced JSON");
            --depth;
        }
    }
    std::array<std::set<std::string>, 35> keys;
    return Json::parse(line, [&](int depth, Json::parse_event_t event, Json& value) {
        if (event == Json::parse_event_t::object_start)
            keys.at(depth).clear();
        if (event == Json::parse_event_t::key && !keys.at(depth - 1).insert(value.get<std::string>()).second)
            throw std::invalid_argument("Duplicate JSON key");
        return true;
    });
}
} // namespace
Json parseRequest(const std::string& line) {
    return parseDocument(line, maxRequestBytes, 32);
}
Json parseTraceRecord(const std::string& line) {
    return parseDocument(line, maxTraceRecordBytes, 34);
}
namespace {
Cell cell(const Json& j) {
    if (!j.is_array() || j.size() != 2)
        throw std::invalid_argument("Cell must be [x,y]");
    return {int(bounded(j[0], Map::maxDimension - 1)), int(bounded(j[1], Map::maxDimension - 1))};
}
ItemHandle item(const Json& j, const GameSession& g) {
    fields(j, {"slot", "generation"});
    return {g.party().inventory().identity(), serial(j.at("generation")),
            std::uint16_t(bounded(j.at("slot"), inventoryCapacity - 1))};
}
rules::SpellId spell(const Json& j, const rules::Catalog& catalog) {
    const auto name = j.get<std::string>();
    for (std::uint16_t i = 0; i < catalog.spellCount(); ++i)
        if (catalog.spell({i})->id == name)
            return {i};
    throw std::invalid_argument("Unknown spell ID");
}
std::uint32_t spells(const Json& j, const rules::Catalog& c) {
    if (!j.is_array() || j.size() > 32)
        throw std::invalid_argument("Expected at most 32 spell IDs");
    std::uint32_t mask = 0;
    for (const auto& entry : j) {
        const auto bit = rules::spellBit(spell(entry, c));
        if (mask & bit)
            throw std::invalid_argument("Duplicate spell");
        mask |= bit;
    }
    return mask;
}
} // namespace
ActionRequest decodeAction(const Json& j, const GameSession& g) {
    const auto op = j.at("kind").get<std::string>();
    const auto& cat = g.party().catalog();
    ActionRequest r;
    if (op == "move") {
        fields(j, {"kind", "direction"});
        r.operation = Operation::Move;
        const std::array<const char*, 6> names{"forward", "back", "left", "right", "turn_left", "turn_right"};
        for (unsigned i = 0; i < names.size(); ++i)
            if (j.at("direction") == names[i]) {
                r.movement = Action(i);
                return r;
            }
        throw std::invalid_argument("Unknown movement direction");
    }
    if (op == "continue") {
        fields(j, {"kind"});
        r.operation = Operation::Continue;
        return r;
    }
    if (op == "recall" || op == "waypoint") {
        fields(j, op == "recall" ? std::initializer_list<const char*>{"kind"}
                                 : std::initializer_list<const char*>{"kind", "depth"});
        if (!g.campaign())
            throw std::invalid_argument("Travel requires a campaign");
        r.operation = Operation::Travel;
        r.travel = op == "recall" ? g.campaign()->recallRequest()
                                  : g.campaign()->waypointRequest(bounded(j.at("depth"), 3));
        return r;
    }
    if (op == "interact") {
        fields(j, {"kind", "fixture"});
        r.operation = Operation::Interact;
        r.fixture = {{g.fixtures().world(), std::uint16_t(bounded(j.at("fixture"), UINT16_MAX))},
                     g.fixtures().revision()};
        return r;
    }
    if (op == "inventory") {
        fields(j, {"kind", "operation", "item", "character", "cell", "quantity"});
        r.operation = Operation::Inventory;
        const auto operation = j.at("operation").get<std::string>();
        if (operation == "move")
            r.inventory.operation = InventoryOp::Move;
        else if (operation == "split")
            r.inventory.operation = InventoryOp::Split;
        else if (operation == "transfer")
            r.inventory.operation = InventoryOp::Transfer;
        else
            throw std::invalid_argument("Unknown inventory operation");
        r.inventory.source = item(j.at("item"), g);
        r.inventory.destination.character = {std::uint16_t(bounded(j.at("character"), 3))};
        r.inventory.destination.cell =
            j.contains("cell") ? std::uint16_t(bounded(j.at("cell"), 51)) : UINT16_MAX;
        r.inventory.quantity = j.contains("quantity") ? bounded(j.at("quantity"), UINT16_MAX) : 0;
        r.inventory.revision = g.party().inventory().revision();
        return r;
    }
    if (op == "drop") {
        fields(j, {"kind", "item"});
        r.operation = Operation::Drop;
        r.item = item(j.at("item"), g);
        r.inventory.revision = g.party().inventory().revision();
        return r;
    }
    if (op == "pickup") {
        fields(j, {"kind", "loot", "character"});
        r.operation = Operation::Pickup;
        fields(j.at("loot"), {"slot", "generation"});
        r.pickup.loot = {g.loot().snapshot().world,
                         std::uint16_t(bounded(j.at("loot").at("slot"), lootCapacity - 1)),
                         serial(j.at("loot").at("generation"))};
        r.pickup.recipient = {std::uint16_t(bounded(j.at("character"), 3))};
        r.pickup.inventoryRevision = g.party().inventory().revision();
        return r;
    }
    if (op == "rest") {
        fields(j, {"kind", "type", "dice", "prepared"});
        r.operation = Operation::Rest;
        const auto type = j.at("type").get<std::string>();
        if (type != "short" && type != "long")
            throw std::invalid_argument("Unknown rest kind");
        r.rest.kind = type == "short" ? RestKind::Short : RestKind::Long;
        r.rest.world = g.actors().snapshot().world;
        r.rest.token = g.restToken();
        if (j.contains("dice")) {
            const auto& a = j.at("dice");
            if (!a.is_array() || a.size() != 4)
                throw std::invalid_argument("Expected four hit-die counts");
            for (unsigned i = 0; i < 4; ++i)
                r.rest.dice[i] = bounded(a[i], 3);
        }
        if (j.contains("prepared")) {
            const auto& a = j.at("prepared");
            if (!a.is_array() || a.size() != 4)
                throw std::invalid_argument("Expected four preparation lists");
            r.rest.preparation = g.party().preparationRequest();
            r.rest.preparation.enabled = true;
            for (unsigned i = 0; i < 4; ++i)
                r.rest.preparation.prepared[i] = spells(a[i], cat);
        }
        return r;
    }
    if (op == "advancement") {
        fields(j, {"kind", "character", "learned", "prepared"});
        r.operation = Operation::Advancement;
        r.advancement = g.advancementRequest({std::uint16_t(bounded(j.at("character"), 3))});
        if (j.contains("learned"))
            r.advancement.choices.learned = spells(j.at("learned"), cat);
        if (j.contains("prepared"))
            r.advancement.choices.prepared = spells(j.at("prepared"), cat);
        return r;
    }
    if (op != "combat")
        throw std::invalid_argument("Unknown action kind");
    fields(j, {"kind", "character", "action", "target", "ally", "spell", "feature", "item", "enemies",
               "allies", "darts", "cell", "restore_paralysis"});
    auto& c = r.combat;
    c.world = g.actors().snapshot().world;
    c.turn = g.combat().snapshot().turn;
    c.character = {std::uint16_t(bounded(j.at("character"), 3))};
    const std::array<const char*, 9> actions{"melee", "back_melee", "defend",  "ranged", "spell",
                                             "item",  "aid",        "feature", nullptr};
    bool found = false;
    for (unsigned i = 0; i < 8; ++i)
        if (j.at("action") == actions[i]) {
            c.action = PartyAction(i);
            found = true;
        }
    if (!found)
        throw std::invalid_argument("Unknown combat action");
    if (j.contains("target"))
        c.target = g.actors().id(bounded(j.at("target"), NpcWorld::maxInstances - 1));
    if (j.contains("ally"))
        c.ally = rules::CharacterId{std::uint16_t(bounded(j.at("ally"), 3))};
    if (j.contains("spell"))
        c.spell = spell(j.at("spell"), cat);
    if (j.contains("feature")) {
        const std::array<const char*, 11> names{
            "none",         "second_wind",   "arcane_recovery",         "dismiss_concentration",
            "action_surge", "preserve_life", "cunning_action",          "spiritual_weapon",
            "escape_web",   "end_turn",      "dismiss_spiritual_weapon"};
        bool matched = false;
        for (unsigned i = 1; i < 11; ++i)
            if (j.at("feature") == names[i]) {
                c.feature = rules::Feature(i);
                matched = true;
            }
        if (!matched)
            throw std::invalid_argument("Unknown feature");
    }
    if (j.contains("item")) {
        c.item = item(j.at("item"), g);
        c.inventoryRevision = g.party().inventory().revision();
    }
    if (j.contains("enemies") && j.contains("allies"))
        throw std::invalid_argument("Select enemies or allies, not both");
    for (const auto* name : {"enemies", "allies"})
        if (j.contains(name)) {
            const auto& a = j.at(name);
            if (!a.is_array() || a.size() > 3)
                throw std::invalid_argument("Expected at most three recipients");
            c.targetCount = a.size();
            for (std::size_t i = 0; i < a.size(); ++i)
                if (std::string_view(name) == "enemies")
                    c.enemies[i] = g.actors().id(bounded(a[i], NpcWorld::maxInstances - 1));
                else
                    c.allies[i] = {std::uint16_t(bounded(a[i], 3))};
        }
    if (j.contains("darts")) {
        const auto& a = j.at("darts");
        if (!a.is_array() || a.size() != c.targetCount)
            throw std::invalid_argument("Dart counts must match recipients");
        for (std::size_t i = 0; i < a.size(); ++i)
            c.darts[i] = bounded(a[i], 3);
    }
    if (j.contains("cell"))
        c.aimedCell = cell(j.at("cell"));
    if (j.contains("restore_paralysis"))
        c.restoreParalysis = j.at("restore_paralysis").get<bool>();
    return r;
}
} // namespace blobber::automation
