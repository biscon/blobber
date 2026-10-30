#include "generation/diagnostic_view.hpp"
#include <cstdio>
namespace blobber::generation {
DiagnosticView diagnosticView(const GeneratedFloor& f) noexcept {
    DiagnosticView v;
    for (unsigned i = 0; i < f.layout.roomCount; ++i)
        if (f.layout.rooms[i].role != Role::Junction)
            v.markers[v.count++] = {f.layout.rooms[i].anchor, f.layout.rooms[i].role};
    auto line = [&](unsigned i, const char* format, auto... args) {
        std::snprintf(v.lines[i].data(), v.lines[i].size(), format, args...);
    };
    line(0, "Seed %u / depth %u", f.request.seed, f.request.depth);
    line(1, "Generator %u / templates %u", f.request.generator, f.request.templates);
    line(2, "Fingerprint %llu", static_cast<unsigned long long>(f.fingerprint));
    line(3, "Topology %u / rotation %u / rooms %u", f.layout.family, f.layout.rotation, f.layout.roomCount);
    line(4, "Attempts %u / fallback %u", f.diagnostics.attempts, f.diagnostics.fallbackId);
    unsigned errors = 0;
    for (unsigned i = 0; i < f.diagnostics.attempts; ++i)
        if (f.diagnostics.rejected[i])
            ++errors;
    line(5, "Rejections %u / last: %s", errors,
         errors ? errorName(f.diagnostics.rejected[errors - 1].code) : "none");
    line(6, "K key / U up / D down / O objective / + door");
    line(7, "Doors and keys active; travel/objectives unavailable.");
    line(8, "Full map is diagnostic, not discovered knowledge.");
    return v;
}
} // namespace blobber::generation
