#include "generation/floor.hpp"
namespace blobber::generation {
Cell rotate(Cell c, int extent, unsigned n) noexcept {
    for (unsigned i = 0; i < n % 4; ++i)
        c = {extent - 1 - c.y, c.x};
    return c;
}
const TemplateCatalog& templates() noexcept {
    static const TemplateCatalog catalog = [] {
        TemplateCatalog t{};
        for (auto& room : t)
            for (int y = 1; y < 6; ++y)
                for (int x = 1; x < 6; ++x)
                    room.floor[y * 7 + x] = 1;
        t[1].floor[3 * 7 + 3] = 0; // Central pillar, walkable on every side.
        t[1].anchor = {2, 3};
        for (int y = 1; y < 3; ++y)
            for (int x = 4; x < 6; ++x)
                t[2].floor[y * 7 + x] = 0; // Three-cell-wide bent interior.
        t[3].floor[1 * 7 + 1] = t[3].floor[1 * 7 + 5] = t[3].floor[5 * 7 + 1] = 0;
        t[4].floor.fill(0);
        for (int y = 1; y <= 3; ++y)
            for (int x = 1; x <= 3; ++x)
                t[4].floor[y * 7 + x] = 1;
        t[4].floor[4 * 7 + 1] = 1;
        for (int x = 1; x <= 5; ++x)
            t[4].floor[5 * 7 + x] = 1;
        t[4].anchor = {2, 2}; // Bent southern exit; rotate toward the sole port.
        return t;
    }();
    return catalog;
}
Failure validateTemplates(const TemplateCatalog& catalog) noexcept {
    for (unsigned n = 0; n < catalog.size(); ++n) {
        const auto& t = catalog[n];
        if (t.anchor.x < 1 || t.anchor.x > 5 || t.anchor.y < 1 || t.anchor.y > 5 ||
            !t.floor[t.anchor.y * 7 + t.anchor.x])
            return {Error::Template, int(n)};
        for (int y = 0; y < 7; ++y)
            for (int x = 0; x < 7; ++x)
                if (t.floor[y * 7 + x] > 1 || ((x == 0 || y == 0 || x == 6 || y == 6) && t.floor[y * 7 + x]))
                    return {Error::Template, int(n)};
    }
    return {};
}
const char* roleName(Role r) noexcept {
    switch (r) {
    case Role::Junction:
        return "junction";
    case Role::Entrance:
        return "entrance";
    case Role::Approach:
        return "approach";
    case Role::Objective:
        return "objective";
    case Role::Optional:
        return "optional";
    }
    return "invalid";
}
const char* errorName(Error e) noexcept {
    switch (e) {
    case Error::None:
        return "none";
    case Error::Request:
        return "invalid_request";
    case Error::Capacity:
        return "capacity";
    case Error::Template:
        return "invalid_template";
    case Error::Bounds:
        return "bounds";
    case Error::Room:
        return "invalid_room";
    case Error::Port:
        return "unmatched_port";
    case Error::Connection:
        return "invalid_connection";
    case Error::Anchor:
        return "invalid_anchor";
    case Error::Disconnected:
        return "disconnected";
    case Error::Loop:
        return "unhelpful_loop";
    case Error::Branch:
        return "invalid_optional_branch";
    case Error::Fixture:
        return "invalid_fixture";
    case Error::Progression:
        return "invalid_progression";
    case Error::Population:
        return "invalid_population";
    case Error::Landing:
        return "unsafe_landing";
    case Error::ForcedFallback:
        return "forced_fallback";
    }
    return "invalid";
}
} // namespace blobber::generation
