#include "generation/report.hpp"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string_view>
using namespace blobber::generation;
namespace {
unsigned number(std::string_view s) {
    unsigned n = 0;
    const auto r = std::from_chars(s.data(), s.data() + s.size(), n);
    if (r.ec != std::errc{} || r.ptr != s.data() + s.size())
        throw std::runtime_error("Expected unsigned integer");
    return n;
}
} // namespace
int main(int argc, char** argv) {
    try {
        GenerationRequest request;
        bool seed = false, fallback = false, json = false;
        unsigned count = 1;
        for (int i = 1; i < argc; ++i) {
            const std::string_view option(argv[i]);
            if (option == "--help") {
                std::cout << "blobber_generate_floor --seed UINT32 [--depth 1..3] [--format ascii|json] "
                             "[--force-fallback] [--count 1..100000]\n"
                             "Multiple seeds emit streaming JSON lines plus a summary. Sweeps are opt-in.\n";
                return 0;
            }
            if (option == "--force-fallback") {
                fallback = true;
                continue;
            }
            if (++i == argc)
                throw std::runtime_error("Missing option value");
            const std::string_view value(argv[i]);
            if (option == "--seed") {
                request.seed = number(value);
                seed = true;
            } else if (option == "--depth")
                request.depth = number(value);
            else if (option == "--count")
                count = number(value);
            else if (option == "--format" && (value == "ascii" || value == "json"))
                json = value == "json";
            else
                throw std::runtime_error("Unknown option/value");
        }
        if (!seed || !count || count > 100000 || std::uint64_t(request.seed) + count - 1 > UINT32_MAX)
            throw std::runtime_error("Require seed, count 1..100000, and a non-overflowing seed range");
        unsigned failures = 0, fallbacks = 0, retries = 0;
        std::uint64_t totalUs = 0, maxUs = 0;
        std::array<unsigned, unsigned(Error::ForcedFallback) + 1> reasons{};
        std::vector<std::uint64_t> fingerprints;
        fingerprints.reserve(count);
        for (unsigned i = 0; i < count; ++i, ++request.seed) {
            const auto start = std::chrono::steady_clock::now();
            const auto result = generate(request, templates(), fallback);
            const auto us = std::uint64_t(std::chrono::duration_cast<std::chrono::microseconds>(
                                              std::chrono::steady_clock::now() - start)
                                              .count());
            totalUs += us;
            maxUs = std::max(maxUs, us);
            failures += !result;
            fallbacks += result.diagnostics.fallback;
            retries += result.diagnostics.attempts ? result.diagnostics.attempts - 1 : 0;
            for (unsigned n = 0; n < result.diagnostics.attempts; ++n)
                if (result.diagnostics.rejected[n])
                    ++reasons[unsigned(result.diagnostics.rejected[n].code)];
            if (result)
                fingerprints.push_back(result.floor->fingerprint);
            auto j = report(result, request);
            if (count > 1)
                for (const char* field : {"map", "rooms", "connections"})
                    j.erase(field);
            j["generation_us"] = us;
            j["retained_bytes"] =
                sizeof(GeneratedFloor) +
                (result ? unsigned(result.floor->layout.width * result.floor->layout.height) : 0);
            j["scratch_bytes"] = scratchBytes();
            if (json || count > 1 || !result)
                std::cout << j.dump() << '\n';
            else {
                j.erase("map");
                std::cout
                    << "DIAGNOSTIC — provisional roles: E entrance, A approach, O objective, B optional\n"
                    << j.dump(2) << '\n'
                    << ascii(result.floor->layout);
            }
            if (!std::cout)
                throw std::runtime_error("Report output failed");
        }
        if (count > 1) {
            std::sort(fingerprints.begin(), fingerprints.end());
            const auto unique = std::unique(fingerprints.begin(), fingerprints.end()) - fingerprints.begin();
            nlohmann::json r = nlohmann::json::object();
            for (unsigned i = 1; i < reasons.size(); ++i)
                if (reasons[i])
                    r[errorName(Error(i))] = reasons[i];
            std::cout << nlohmann::json{{"summary", true},       {"count", count},
                                        {"failures", failures},  {"fallbacks", fallbacks},
                                        {"retries", retries},    {"distinct_fingerprints", unique},
                                        {"rejection_causes", r}, {"total_us", totalUs},
                                        {"max_us", maxUs}}
                             .dump()
                      << '\n';
        }
        return failures ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
