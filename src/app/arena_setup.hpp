#pragma once
#include "input/text_edit.hpp"
#include "persistence/arena_store.hpp"
namespace blobber {
struct ArenaSetup {
    ArenaScenario scenario;
    ScenarioListing listing;
    ScenarioEntry currentFile;
    std::array<bool, rules::Catalog::maxMonsters> available{};
    std::array<std::uint16_t, rules::Catalog::maxMonsters> visible{};
    std::size_t visibleCount = 0, first = 0, firstFile = 0;
    std::array<char, 65> search{}, seedText{};
    std::array<char, 1024> folder{};
    TextEdit nameEdit, searchEdit, seedEdit;
    bool initialized = false;
    int pendingLoad = -1;
    void filter(const rules::Catalog&) noexcept;
    void syncSeed() noexcept;
    bool readSeed() noexcept;
    const char* error(const rules::Catalog&) noexcept;
    void blur() noexcept { nameEdit.focused = searchEdit.focused = seedEdit.focused = false; }
};
struct MenuView;
struct Options;
class Presenter;
void captureArena(MenuView&, Presenter&, const Options&, ArenaScenarioStore&);
void openArena(MenuView&, const Options&, ArenaScenarioStore&);
// All disk operations and launch transitions live here, outside rendering/input frames.
bool arenaAction(MenuView&, int action, const Options&, ArenaScenarioStore&, Presenter&);
} // namespace blobber
