#include "app/menu.hpp"
#include "app/menu_model.hpp"
#include "assets/rules_loader.hpp"
#include "platform/input.hpp"
#include "platform/window.hpp"
#include "ui/hud_painter.hpp"
#include "ui/menu_layout.hpp"
#include "ui/menu_screen.hpp"
#include <cstdio>
#include <cstring>
#include <random>
#include <unistd.h>
namespace blobber {
int runMenus(const Options& options) {
    const auto catalog = loadRules(options.assets / "rules/catalog.json");
    Window window(options.width, options.height, options.captureMenuOnly);
    Presenter presenter(options.assets);
    presenter.prepareMenu();
    std::filesystem::path libraryPath;
    if (options.captureMenuOnly) {
        char pattern[] = "/tmp/blobber-menu-XXXXXX";
        const auto* dir = mkdtemp(pattern);
        if (!dir)
            throw std::runtime_error("Cannot create isolated capture library");
        libraryPath = std::filesystem::path(dir) / "characters.json";
    }
    LibraryIoHooks hooks;
    if (options.captureMenuOnly)
        hooks.entropy = [](std::array<std::uint8_t, 16>& bytes) {
            static unsigned id = 0;
            bytes.fill(0);
            ++id;
            bytes[14] = std::uint8_t(id >> 8);
            bytes[15] = std::uint8_t(id);
            return true;
        };
    CharacterLibrary library(catalog, characterLibraryPath(libraryPath), hooks);
    MenuView v{catalog, library};
    auto reload = [&] {
        const auto r = library.initialize();
        v.usable = bool(r);
        v.message(r ? "" : r.message.data());
        v.model.filter(library.snapshot());
    };
    reload();
    if (options.captureMenuOnly) {
        std::filesystem::create_directories(options.captureDirectory);
        auto capture = [&](const char* name) {
            presenter.presentMenu(drawMenu, &v);
            presenter.capture(options.captureDirectory / (std::string(name) + ".png"));
            presenter.capture(options.captureDirectory / (std::string(name) + "-1280.png"), 1280, 720);
            presenter.capture(options.captureDirectory / (std::string(name) + "-4x3.png"), 1024, 768);
        };
        capture("main");
        v.model.screen = MenuScreen::Library;
        capture("library");
        v.model.confirmDelete(library.snapshot(), library.snapshot().records[0].id);
        capture("delete");
        v.model.deletion.reset();
        v.model.selected = library.snapshot().records[0].id;
        v.model.screen = MenuScreen::Detail;
        capture("detail");
        v.model.screen = MenuScreen::Picker;
        capture("picker-empty");
        UiInput dragPress;
        dragPress.pointer = ArtPoint{45, 100};
        dragPress.pointerHeld = true;
        dragPress.pressed[std::size_t(UiKey::Click)] = true;
        v.interaction.routeDrag(v.model, library.snapshot(), dragPress);
        UiInput dragMove;
        dragMove.pointer = ArtPoint{450, 120};
        dragMove.pointerHeld = true;
        v.interaction.routeDrag(v.model, library.snapshot(), dragMove);
        capture("picker-drag");
        UiInput dragDrop;
        dragDrop.pointer = dragMove.pointer;
        dragDrop.pointerReleased = true;
        v.interaction.routeDrag(v.model, library.snapshot(), dragDrop);
        capture("picker-dropped");
        v.interaction.focusSearch(true);
        std::snprintf(v.model.search.data(), 65, "Alden");
        v.model.filter(library.snapshot());
        capture("search-caret-on");
        v.interaction.tick(.6f, true);
        capture("search-caret-off");
        v.interaction.focusSearch(false);
        v.model.search = {};
        v.model.filter(library.snapshot());
        v.model.defaults();
        capture("picker-ready");
        for (int i = 0; i < 20; ++i) {
            auto draft = library.snapshot().records[i % 4].draft;
            std::snprintf(draft.name.data(), draft.name.size(), "Adventurer %02d", i);
            library.create(draft, library.snapshot().revision);
        }
        v.model.screen = MenuScreen::Library;
        v.model.filter(library.snapshot());
        capture("library-many");
        v.model.firstRow = 3;
        capture("library-scrolled");
        std::snprintf(v.model.search.data(), 65, "No matching record");
        v.model.filter(library.snapshot());
        capture("library-no-results");
        v.model.search = {};
        v.model.filter(library.snapshot());
        v.model.activeSlot = 0;
        v.model.assign({library.snapshot().records[4].id, -1});
        v.model.selected = library.snapshot().records[3].id;
        v.model.screen = MenuScreen::Detail;
        capture("detail-wizard");
        v.message("Launch failed: prepared dungeon assets unavailable. Retry after repair.");
        v.model.screen = MenuScreen::Picker;
        capture("launch-error");
        launchMenuParty(options, catalog, v.model.resolve(catalog, library.snapshot()), presenter);
        return 0;
    }
    Input input;
    bool suppress = true;
    while (!WindowShouldClose()) {
        presenter.presentMenu(drawMenu, &v);
        auto sample = input.poll(GetFrameTime());
        auto& m = v.model;
        v.interaction.tick(GetFrameTime(), sample.ui.focused);
        if (!sample.ui.focused)
            continue;
        if (suppress) {
            while (GetCharPressed()) {
            };
            if (!IsKeyDown(KEY_ENTER) && !IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                suppress = false;
            continue;
        }
        if (v.interaction.routeDrag(m, library.snapshot(), sample.ui)) {
            while (GetCharPressed()) {
            }
            continue;
        }
        if (v.interaction.searchFocused && !m.deletion) {
            for (int ch = GetCharPressed(); ch; ch = GetCharPressed()) {
                auto n = std::strlen(m.search.data());
                if (ch >= 32 && ch < 127 && n < 64) {
                    m.search[n] = char(ch);
                    m.search[n + 1] = 0;
                    v.interaction.edited();
                }
            }
            if (IsKeyPressed(KEY_BACKSPACE)) {
                auto n = std::strlen(m.search.data());
                if (n) {
                    m.search[n - 1] = 0;
                    v.interaction.edited();
                }
            }
            m.filter(library.snapshot());
        } else
            while (GetCharPressed()) {
            }
        if (sample.ui.has(UiKey::Tab) || sample.ui.has(UiKey::Down) || sample.ui.has(UiKey::Right))
            v.focus = (v.focus + (sample.ui.shift ? std::max(1, v.count) - 1 : 1)) % std::max(1, v.count);
        if (sample.ui.has(UiKey::Up) || sample.ui.has(UiKey::Left))
            v.focus = (v.focus + std::max(1, v.count) - 1) % std::max(1, v.count);
        if (sample.ui.has(UiKey::Tab) || sample.ui.has(UiKey::Up) || sample.ui.has(UiKey::Down) ||
            sample.ui.has(UiKey::Left) || sample.ui.has(UiKey::Right))
            v.interaction.focusSearch(v.focus < v.count && v.actions[v.focus] == 5);
        if (sample.ui.has(UiKey::Click) &&
            (!sample.ui.pointer || !menuLayout::search.contains(*sample.ui.pointer)))
            v.interaction.focusSearch(false);
        int action = 0;
        if (sample.ui.has(UiKey::Confirm) && v.focus < v.count)
            action = v.actions[v.focus];
        if (sample.ui.has(UiKey::Click) && sample.ui.pointer)
            for (int i = 0; i < v.count; ++i)
                if (v.buttons[i].contains(*sample.ui.pointer)) {
                    v.focus = i;
                    action = v.actions[i];
                    break;
                }
        if (IsKeyPressed(KEY_ESCAPE))
            action = m.deletion ? 40 : 4;
        if (!m.deletion && sample.ui.scroll)
            action = m.screen == MenuScreen::Detail ? (sample.ui.scroll > 0 ? 33 : 34)
                                                    : (sample.ui.scroll > 0 ? 31 : 32);
        if (!m.deletion && (sample.ui.has(UiKey::PreviousPage) || sample.ui.has(UiKey::NextPage))) {
            const bool previous = sample.ui.has(UiKey::PreviousPage);
            action = m.screen == MenuScreen::Detail ? (previous ? 33 : 34) : (previous ? 31 : 32);
        }
        if (!action)
            continue;
        if (action != 5)
            v.interaction.focusSearch(false);
        if (action == 1)
            m.screen = MenuScreen::Picker;
        else if (action == 2)
            m.screen = MenuScreen::Library;
        else if (action == 3)
            return 0;
        else if (action == 4) {
            m.screen = m.screen == MenuScreen::Detail ? v.detailCaller : MenuScreen::Main;
        } else if (action == 5)
            v.interaction.focusSearch(true);
        else if (action == 6) {
            if (!m.filterRace) {
                m.filterRace = true;
                m.race.index = 0;
            } else if (++m.race.index >= 4)
                m.filterRace = false;
            m.filter(library.snapshot());
        } else if (action == 7) {
            if (!m.filterClass) {
                m.filterClass = true;
                m.characterClass.index = 0;
            } else if (++m.characterClass.index >= 4)
                m.filterClass = false;
            m.filter(library.snapshot());
        } else if (action == 8)
            reload();
        else if (action == 9)
            m.defaults();
        else if (action >= 10 && action < 14)
            m.activeSlot = action - 10;
        else if (action >= 20 && action < 24)
            m.choices[action - 20] = {};
        else if (action == 33) {
            if (v.detailOffset)
                --v.detailOffset;
        } else if (action == 34) {
            if (v.detailOffset + 11 < v.detailRows)
                ++v.detailOffset;
        } else if (action == 31) {
            if (m.firstRow)
                --m.firstRow;
        } else if (action == 32) {
            const std::size_t cols = m.screen == MenuScreen::Picker ? 3 : 4;
            if ((m.firstRow + 2) * cols < m.visibleCount)
                ++m.firstRow;
        } else if (action == 40)
            m.deletion.reset();
        else if (action == 41 && m.deletion) {
            auto r = library.erase(m.deletion->id, m.deletionRevision);
            v.message(r ? "" : r.message.data());
            if (r.committed) {
                m.deletion.reset();
                v.usable = bool(r);
                m.filter(library.snapshot());
            }
        } else if (action >= 3000 && action < 3128 && v.usable)
            m.confirmDelete(library.snapshot(), library.snapshot().records[action - 3000].id);
        else if (action >= 2000 && action < 3000) {
            m.selected = library.snapshot().records[action - 2000].id;
            v.detailCaller = m.screen;
            v.detailOffset = 0;
            m.screen = MenuScreen::Detail;
        } else if (action >= 1000 && action < 2000) {
            const auto& r = library.snapshot().records[action - 1000];
            m.selected = r.id;
            m.assign({r.id, -1});
        } else if (action == 30 && m.ready(library.snapshot())) {
            try {
                bool saved = false;
                for (const auto& c : m.choices)
                    saved |= c.preset < 0;
                if (saved) {
                    const auto before = library.snapshot().revision;
                    const auto r = library.load();
                    if (!r)
                        throw std::runtime_error(r.message.data());
                    if (before != library.snapshot().revision) {
                        m.filter(library.snapshot());
                        throw std::runtime_error("Library changed. Review the party and launch again.");
                    }
                }
                auto starting = m.resolve(catalog, library.snapshot());
                m.screen = MenuScreen::Loading;
                presenter.presentMenu(drawMenu, &v);
                if (!launchMenuParty(options, catalog, starting, presenter))
                    return 0;
                m.screen = MenuScreen::Main;
            } catch (const std::exception& e) {
                m.screen = MenuScreen::Picker;
                v.message(e.what());
            }
        }
        if (action != 5)
            v.focus = 0;
        suppress = true;
    }
    return 0;
}
} // namespace blobber
