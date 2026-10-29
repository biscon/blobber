#pragma once
#include "game/arena.hpp"
#include "input/ui_input.hpp"
#include "persistence/character_library.hpp"
#include <optional>
namespace blobber {
enum class MenuScreen { Main, Library, Detail, Picker, Loading, Arena, Scenarios };
struct PartyChoice {
    SavedCharacterId saved{};
    int preset = -1;
    int snapshot = -1;
    bool empty() const noexcept { return preset < 0 && snapshot < 0 && !saved.text[0]; }
    friend bool operator==(const PartyChoice& a, const PartyChoice& b) noexcept {
        return a.preset == b.preset && a.saved == b.saved && a.snapshot == b.snapshot;
    }
};
// Fixed menu state; filesystem operations belong to the application transition owner.
struct MenuModel {
    MenuScreen screen = MenuScreen::Main;
    std::array<char, 65> search{};
    rules::RaceId race{};
    rules::ClassId characterClass{};
    bool filterRace = false, filterClass = false;
    std::array<std::uint16_t, LibrarySnapshot::capacity> visible{};
    std::size_t visibleCount = 0, firstRow = 0;
    SavedCharacterId selected{};
    std::array<PartyChoice, 4> choices{};
    std::array<StartingSnapshot, 4> snapshots{};
    bool arenaPicker = false;
    const rules::CharacterDraft* draft(const rules::Catalog&, const LibrarySnapshot&, PartyChoice,
                                       rules::CharacterDraft& scratch) const noexcept;
    std::array<int, 4> snapshotPresets{{-1, -1, -1, -1}};
    bool sameChoice(PartyChoice, PartyChoice) const noexcept;
    void beginArenaPicker(const ArenaScenario&, const rules::Catalog&);
    std::size_t activeSlot = 0;
    std::optional<SavedCharacter> deletion;
    std::uint64_t deletionRevision = 0;
    void filter(const LibrarySnapshot&) noexcept;
    void assign(PartyChoice) noexcept;
    void defaults() noexcept;
    bool ready(const LibrarySnapshot&) const noexcept;
    void confirmDelete(const LibrarySnapshot&, SavedCharacterId) noexcept;
    rules::PresetParty resolve(const rules::Catalog&, const LibrarySnapshot&) const;
};
struct MenuInteraction {
    bool searchFocused = false, pointerDown = false, dragging = false;
    float blink = 0;
    PartyChoice carried{};
    ArtPoint origin{}, pointer{};
    int target = -1;
    void focusSearch(bool focused) noexcept {
        searchFocused = focused;
        blink = 0;
    }
    void edited() noexcept { blink = 0; }
    void tick(float seconds, bool focused) noexcept;
    bool caretVisible() const noexcept { return searchFocused && blink < .5f; }
    void cancelDrag() noexcept {
        pointerDown = dragging = false;
        carried = {};
        target = -1;
    }
    bool routeDrag(MenuModel&, const LibrarySnapshot&, const UiInput&) noexcept;
};
} // namespace blobber
