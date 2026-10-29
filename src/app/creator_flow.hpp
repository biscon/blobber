#pragma once
#include "app/menu_model.hpp"
#include "game/creator.hpp"
namespace blobber {
// Explicit transition owner: only save performs disk I/O; ordinary edits are model-only.
struct CreatorFlow {
    CreatorModel model;
    bool active = false;
    MenuScreen caller = MenuScreen::Library;
    std::size_t activeSlot = 0;
    std::uint64_t revision = 0;
    void begin(const rules::Catalog&, const MenuModel&, const LibrarySnapshot&) noexcept;
    LibraryResult save(CharacterLibrary&, MenuModel&);
    void cancel(MenuModel&) noexcept;
};
} // namespace blobber
