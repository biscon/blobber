#include "assets/shops.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace blobber {
namespace {
using Node = YAML::Node;
void bounded(const Node& n, unsigned depth, unsigned& nodes) {
    if (depth > 12 || ++nodes > 8192)
        throw std::runtime_error("Shop YAML structure exceeds limit");
    if (n.IsMap())
        for (const auto& entry : n) {
            bounded(entry.first, depth + 1, nodes);
            bounded(entry.second, depth + 1, nodes);
        }
    else if (n.IsSequence())
        for (const auto& entry : n)
            bounded(entry, depth + 1, nodes);
}
void fields(const Node& n, std::initializer_list<const char*> allowed) {
    if (!n.IsMap())
        throw std::runtime_error("Expected shop mapping");
    std::set<std::string> seen;
    for (const auto& entry : n) {
        const auto name = entry.first.as<std::string>();
        if (!seen.insert(name).second ||
            std::none_of(allowed.begin(), allowed.end(), [&](auto key) { return name == key; }))
            throw std::runtime_error("Unknown or duplicate shop field: " + name);
    }
}
unsigned integer(const Node& n, unsigned low, unsigned high) {
    if (!n.IsScalar())
        throw std::runtime_error("Expected shop integer");
    const auto s = n.as<std::string>();
    if (s.empty() || s.size() > 9 || s.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error("Expected shop integer");
    const auto value = std::stoul(s);
    if (value < low || value > high)
        throw std::runtime_error("Shop integer outside bounds");
    return unsigned(value);
}
rules::ItemId itemId(const std::string& name, const rules::Catalog& catalog) {
    const auto id = catalog.findItem(name);
    const auto* item = catalog.item(id);
    if (!item || item->kind == rules::ItemKind::Currency)
        throw std::runtime_error("Unknown or currency merchandise: " + name);
    return id;
}
} // namespace
ShopCatalog loadShops(const std::filesystem::path& directory, const rules::Catalog& catalog) {
    ShopCatalog out;
    if (!std::filesystem::is_directory(directory))
        throw std::runtime_error("Missing shops directory: " + directory.string());
    std::vector<std::filesystem::path> paths;
    std::uintmax_t bytes = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        const auto extension = entry.path().extension();
        if (!entry.is_regular_file() || (extension != ".yaml" && extension != ".yml"))
            continue;
        bytes += entry.file_size();
        if (bytes > 1024 * 1024 || paths.size() == merchantCapacity)
            throw std::runtime_error("Shop catalog capacity exceeded");
        paths.push_back(entry.path());
    }
    std::sort(paths.begin(), paths.end());
    for (const auto& path : paths) {
        try {
            const auto documents = YAML::LoadAllFromFile(path.string());
            if (documents.size() != 1)
                throw std::runtime_error("Expected one shop YAML document");
            const auto n = documents.front();
            unsigned nodes = 0;
            bounded(n, 0, nodes);
            fields(n, {"version", "id", "restock_days", "buyback_hours", "buyback_markup_percent", "prices",
                       "stock"});
            integer(n["version"], 1, 1);
            ShopDefinition shop;
            shop.key = n["id"].as<std::string>();
            if (!rules::validContentId(shop.key))
                throw std::runtime_error("Invalid shop ID");
            shop.id = dialogueKey(shop.key);
            if (out.find(shop.id))
                throw std::runtime_error("Duplicate shop ID");
            shop.restockDays = integer(n["restock_days"], 1, 365);
            shop.buybackHours = integer(n["buyback_hours"], 1, 24);
            shop.markupPercent = integer(n["buyback_markup_percent"], 1, 100);
            const auto prices = n["prices"];
            if (!prices.IsMap() || !prices.size() || prices.size() > rules::Catalog::maxItems)
                throw std::runtime_error("Expected bounded item prices");
            for (const auto& entry : prices) {
                const auto id = itemId(entry.first.as<std::string>(), catalog);
                auto& price = shop.prices[id.index];
                if (price.buy)
                    throw std::runtime_error("Duplicate item price");
                fields(entry.second, {"buy", "sell"});
                price.buy = integer(entry.second["buy"], 1, 100000);
                price.sell = integer(entry.second["sell"], 1, price.buy - 1);
            }
            const auto stock = n["stock"];
            if (!stock.IsSequence() || !stock.size() || stock.size() > shopStockCapacity)
                throw std::runtime_error("Expected 1..48 stock entries");
            std::array<bool, rules::Catalog::maxItems> seen{};
            for (const auto& entry : stock) {
                fields(entry, {"item", "quantity"});
                const auto id = itemId(entry["item"].as<std::string>(), catalog);
                if (seen[id.index] || !shop.prices[id.index].buy)
                    throw std::runtime_error("Duplicate stock or missing price");
                seen[id.index] = true;
                shop.stock.push_back({id, integer(entry["quantity"], 1, UINT16_MAX)});
            }
            out.shops.push_back(std::move(shop));
        } catch (const std::exception& e) {
            throw std::runtime_error(path.string() + ": " + e.what());
        }
    }
    return out;
}
void validateShops(const DialogueCatalog& dialogue, const ShopCatalog& shops) {
    for (const auto& c : dialogue.characters)
        if (c.shop && !shops.find(c.shop))
            throw std::runtime_error("Unknown shop reference for dialogue host: " + c.key);
}
} // namespace blobber
