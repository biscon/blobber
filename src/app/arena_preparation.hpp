#pragma once
#include "game/advancement_ui.hpp"
#include "game/arena.hpp"
namespace blobber {
enum class ArenaPreparationStep { Level, Choices, Review };
// Fixed draft; identity views borrow candidate and catalog until cancel/commit.
struct ArenaPreparationFlow {
    bool active = false, commitRequested = false;
    ArenaPreparationStep step = ArenaPreparationStep::Level;
    ArenaScenario candidate;
    AdvancementHud advancement;
    FlowControls controls;
    unsigned transition = 0, member = 0;
    void begin(const ArenaScenario&) noexcept;
    void selectLevel(const rules::Catalog&, unsigned) noexcept;
    bool commit(const rules::Catalog&, ArenaScenario&) noexcept;
    void route(const rules::Catalog&, const UiInput&, const TextInput&, float) noexcept;
    void layout() noexcept;

  private:
    void nextMember(const rules::Catalog&) noexcept;
    void refresh() noexcept;
};
} // namespace blobber
