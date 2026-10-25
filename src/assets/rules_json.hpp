#pragma once
// Internal shared helpers for the bounded rules JSON loaders.
#include "rules/content.hpp"
#include <algorithm>
#include <initializer_list>
#include <nlohmann/json.hpp>
#include <stdexcept>
namespace blobber::rules_json {
using Json = nlohmann::json;
using namespace rules;
[[noreturn]] inline void fail(const std::string& field, const std::string& message) {
    throw std::runtime_error(field + ": " + message);
}
inline void fields(const Json& value, const std::string& path, std::initializer_list<const char*> names) {
    if (!value.is_object())
        fail(path, "expected object");
    for (const auto* name : names)
        if (!value.contains(name))
            fail(path + "." + name, "missing field");
    for (auto it = value.begin(); it != value.end(); ++it)
        if (std::none_of(names.begin(), names.end(), [&](const char* name) { return it.key() == name; }))
            fail(path + "." + it.key(), "unknown field");
}
inline void array(const Json& value, const std::string& path, std::size_t limit) {
    if (!value.is_array() || value.size() > limit)
        fail(path, "expected array with at most " + std::to_string(limit) + " entries");
}
inline std::int32_t integer(const Json& value, const std::string& path) {
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
inline std::string string(const Json& value, const std::string& path) {
    if (!value.is_string())
        fail(path, "expected string");
    return value.get<std::string>();
}
inline std::string identifier(const Json& value, const std::string& path) {
    auto id = string(value, path);
    if (!validContentId(id))
        fail(path, "expected 1..128 printable nonspace ASCII bytes");
    return id;
}
} // namespace blobber::rules_json
