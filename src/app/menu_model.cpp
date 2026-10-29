#include "app/menu_model.hpp"
#include "ui/menu_layout.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace blobber {
namespace {
bool contains(const char* name, const char* query) noexcept {
    const auto n = std::strlen(name), q = std::strlen(query);
    if (q > n)
        return false;
    for (std::size_t i = 0; i <= n - q; ++i) {
        std::size_t j = 0;
        while (j < q && std::tolower(static_cast<unsigned char>(name[i + j])) ==
                            std::tolower(static_cast<unsigned char>(query[j])))
            ++j;
        if (j == q)
            return true;
    }
    return false;
}
} // namespace
const rules::CharacterDraft* MenuModel::draft(const rules::Catalog& c, const LibrarySnapshot& library,
                                              PartyChoice choice,
                                              rules::CharacterDraft& scratch) const noexcept {
    if (choice.snapshot >= 0 && choice.snapshot < 4)
        return &snapshots[choice.snapshot].draft;
    if (choice.preset >= 0 && choice.preset < 4 && c.preset()) {
        if (rules::draftFromBuild(c, c.preset()->characters[choice.preset], scratch))
            return &scratch;
    } else if (const auto* r = library.find(choice.saved))
        return &r->draft;
    return nullptr;
}
bool MenuModel::sameChoice(PartyChoice a, PartyChoice b) const noexcept {
    if (a == b)
        return true;
    if (a.snapshot >= 0 && a.snapshot < 4) {
        const char* id = snapshots[a.snapshot].id.data();
        if (b.snapshot >= 0 && b.snapshot < 4)
            return !std::strcmp(id, snapshots[b.snapshot].id.data());
        if (b.preset >= 0)
            return snapshotPresets[a.snapshot] == b.preset;
        return b.saved.text[0] && !std::strcmp(id, b.saved.text.data());
    }
    if (b.snapshot >= 0 && b.snapshot < 4)
        return sameChoice(b, a);
    return false;
}
void MenuModel::beginArenaPicker(const ArenaScenario& arena, const rules::Catalog& catalog) {
    snapshots = arena.party;
    snapshotPresets.fill(-1);
    if (catalog.preset())
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                if (catalog.preset()->characters[j].id == snapshots[i].id.data())
                    snapshotPresets[i] = j;
    for (int i = 0; i < 4; ++i)
        choices[i] = {{}, -1, i};
    arenaPicker = true;
    screen = MenuScreen::Picker;
}
void MenuModel::filter(const LibrarySnapshot& library) noexcept {
    visibleCount = 0;
    for (std::size_t i = 0; i < library.count; ++i) {
        const auto& d = library.records[i].draft;
        if ((!filterRace || d.race.index == race.index) &&
            (!filterClass || d.characterClass.index == characterClass.index) &&
            contains(d.name.data(), search.data()))
            visible[visibleCount++] = static_cast<std::uint16_t>(i);
    }
    const auto rows = (visibleCount + 3) / 4;
    firstRow = std::min(firstRow, rows > 2 ? rows - 2 : 0);
}
void MenuModel::assign(PartyChoice choice) noexcept {
    if (activeSlot >= choices.size())
        return;
    if (!choice.empty())
        for (std::size_t i = 0; i < choices.size(); ++i)
            if (sameChoice(choices[i], choice)) {
                std::swap(choices[i], choices[activeSlot]);
                return;
            }
    choices[activeSlot] = choice;
}
void MenuModel::defaults() noexcept {
    for (int i = 0; i < 4; ++i)
        choices[static_cast<std::size_t>(i)] = {{}, i};
}
bool MenuModel::ready(const LibrarySnapshot& library) const noexcept {
    for (std::size_t i = 0; i < choices.size(); ++i) {
        const auto& c = choices[i];
        if (c.empty() || c.preset >= 4 || (c.snapshot >= 4) ||
            (c.snapshot < 0 && c.preset < 0 && !library.find(c.saved)))
            return false;
        for (std::size_t j = 0; j < i; ++j)
            if (sameChoice(choices[j], c))
                return false;
    }
    return true;
}
void MenuModel::confirmDelete(const LibrarySnapshot& library, SavedCharacterId id) noexcept {
    if (const auto* record = library.find(id)) {
        deletion = *record;
        deletionRevision = library.revision;
    }
}
rules::PresetParty MenuModel::resolve(const rules::Catalog& catalog, const LibrarySnapshot& library) const {
    if (!ready(library))
        throw std::invalid_argument("Choose four distinct available characters");
    rules::PresetParty result;
    for (std::size_t i = 0; i < choices.size(); ++i) {
        const auto& c = choices[i];
        if (c.snapshot >= 0) {
            const auto& member = snapshots[c.snapshot];
            result.characters[i] = rules::finishBuild(catalog, member.draft, member.id.data());
        } else if (c.preset >= 0) {
            if (!catalog.preset())
                throw std::invalid_argument("Default party unavailable");
            result.characters[i] = catalog.preset()->characters[static_cast<std::size_t>(c.preset)];
            rules::CharacterDraft draft;
            if (!rules::draftFromBuild(catalog, result.characters[i], draft))
                throw std::invalid_argument("Invalid default character");
        } else {
            const auto& record = *library.find(c.saved);
            result.characters[i] = rules::finishBuild(catalog, record.draft, record.id.text.data());
        }
        for (std::size_t j = 0; j < i; ++j)
            if (result.characters[i].id == result.characters[j].id)
                throw std::invalid_argument("Choose four distinct characters");
        result.formation[i] = rules::CharacterId{static_cast<std::uint16_t>(i)};
    }
    return result;
}
} // namespace blobber

namespace blobber {
void MenuInteraction::tick(float seconds, bool focused) noexcept {
    if (!focused) {
        focusSearch(false);
        cancelDrag();
        return;
    }
    if (searchFocused)
        blink = std::fmod(blink + std::max(0.f, seconds), 1.f);
}
bool MenuInteraction::routeDrag(MenuModel& m, const LibrarySnapshot& library, const UiInput& input) noexcept {
    const bool pending = pointerDown;
    if (!input.focused || m.screen != MenuScreen::Picker || m.deletion) {
        cancelDrag();
        return pending;
    }
    if (pointerDown && (input.has(UiKey::Cancel) || input.has(UiKey::RightClick))) {
        cancelDrag();
        return true;
    }
    if (input.pointer)
        pointer = *input.pointer;
    if (!pointerDown && input.has(UiKey::Click) && input.pointer) {
        PartyChoice choice;
        for (int i = 0; i < 6; ++i) {
            const auto n = m.firstRow * 3 + std::size_t(i);
            if (n < m.visibleCount && menuLayout::available(i).contains(pointer))
                choice = {library.records[m.visible[n]].id, -1};
        }
        for (int i = 0; i < 4; ++i)
            if (menuLayout::formation(i).contains(pointer) && !menuLayout::remove(i).contains(pointer))
                choice = m.choices[i];
        if (!choice.empty()) {
            carried = choice;
            origin = pointer;
            pointerDown = true;
            dragging = false;
            focusSearch(false);
            return true;
        }
    }
    if (!pointerDown)
        return false;
    const auto dx = pointer.x - origin.x, dy = pointer.y - origin.y;
    if (input.pointer && dx * dx + dy * dy >= 9)
        dragging = true;
    target = dragging && input.pointer ? menuLayout::dropSlot(pointer) : -1;
    if (input.pointerReleased) {
        if (dragging && target >= 0 &&
            (carried.snapshot >= 0 || carried.preset >= 0 || library.find(carried.saved))) {
            m.activeSlot = std::size_t(target);
            m.assign(carried);
            if (carried.preset < 0)
                m.selected = carried.saved;
        }
        cancelDrag();
    } else if (!input.pointerHeld && !input.has(UiKey::Click))
        cancelDrag();
    return true;
}
} // namespace blobber
