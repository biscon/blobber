#include "app/creator_flow.hpp"
#include <algorithm>
namespace blobber {
void CreatorFlow::begin(const rules::Catalog& c, const MenuModel& menu,
                        const LibrarySnapshot& library) noexcept {
    model.begin(c);
    active = true;
    caller = menu.screen;
    activeSlot = menu.activeSlot;
    revision = library.revision;
}
void CreatorFlow::cancel(MenuModel& menu) noexcept {
    active = false;
    menu.screen = caller;
    menu.activeSlot = activeSlot;
}
LibraryResult CreatorFlow::save(CharacterLibrary& library, MenuModel& menu) {
    model.saveRequested = false;
    if (!active || model.saved)
        return {};
    const auto result = library.create(model.draft, revision);
    if (result.committed) {
        model.saved = true;
        active = false;
        menu.screen = caller;
        menu.activeSlot = activeSlot;
        menu.selected = result.id;
        menu.search = {};
        menu.filterRace = menu.filterClass = false;
        menu.filter(library.snapshot());
        const auto cols = caller == MenuScreen::Picker ? 3u : 4u;
        for (std::size_t i = 0; i < menu.visibleCount; ++i)
            if (library.snapshot().records[menu.visible[i]].id == result.id) {
                menu.firstRow = i / cols;
                break;
            }
    } else {
        model.notify(result.message.data());
        if (result.error == LibraryError::Stale) {
            const auto loaded = library.load();
            if (loaded) {
                revision = library.snapshot().revision;
                model.notify("Library changed and was reloaded. Review your draft, then Save again.");
            } else
                model.notify(loaded.message.data());
        } else if (result.error == LibraryError::RecoveryRequired || result.error == LibraryError::Io) {
            // Retry after a repair must explicitly reload the store. Never retry the write here.
            const auto loaded = library.load();
            if (loaded)
                revision = library.snapshot().revision;
        }
    }
    return result;
}
} // namespace blobber
