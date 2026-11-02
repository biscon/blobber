#pragma once
#include "game/actors.hpp"
#include "rules/spells.hpp"
#include <array>
namespace blobber {
struct CombatOutcome;
namespace audio {
struct Event {
    const char* cue = nullptr;
    ActorId actor{};
    float x = 0, z = 0;
    bool positional = false, creature = false, interface = false;
    unsigned priority = 1;
    std::uint64_t sequence = 0;
};
const char* spellFamily(rules::SpellKind) noexcept;
class Journal {
  public:
    static constexpr unsigned capacity = 1024;
    bool enabled = true;
    void emit(Event event) noexcept;
    void combat(const CombatOutcome&) noexcept;
    void clear() noexcept { count = 0; }
    std::array<Event, capacity> entries{};
    unsigned count = 0;
    std::uint64_t sequence = 0, dropped = 0;
};
} // namespace audio
} // namespace blobber
