#include "app/menu.hpp"
#include "app/audio_settings.hpp"
#include "app/loading.hpp"
#include "app/menu_model.hpp"
#include "assets/blueprint_loader.hpp"
#include "assets/fingerprint.hpp"
#include "assets/rules_loader.hpp"
#include "persistence/campaign_store.hpp"
#include "platform/audio.hpp"
#include "platform/input.hpp"
#include "platform/window.hpp"
#include "ui/hud_painter.hpp"
#include "ui/menu_layout.hpp"
#include "ui/menu_screen.hpp"
#include <cstdio>
#include <cstring>
#include <random>
namespace blobber {
int runMenus(const Options& options) try {
    Window window(options.width, options.height, false, options.uncapped);
    PreparationWorker worker;
    auto loading = std::make_unique<LoadingScreen>(&worker);
    const auto catalog = prepareCpu([&] {
        preparationPoint("Reading rules");
        return loadRules(options.assets / "rules/catalog.json");
    });
    GpuResources resources;
    AudioRuntime audio;
    audio.prepare(options.assets, options.noAudio, options.settings);
    setBorderlessFullscreen(audio.settings.borderlessFullscreen);
    Presenter presenter(resources, options.assets, &worker, options.preparedAssets);
    presenter.audio = &audio;
    loading->attach(presenter);
    presenter.prepareMenu();
    CharacterLibrary library(catalog, characterLibraryPath());
    MenuView v{catalog, library};
    const auto saveDefinitions = prepareCpu([&] {
        preparationPoint("Reading creature definitions");
        return loadNpcDefinitions(options.assets / "npcs/level.json");
    });
    const auto blueprints =
        prepareCpu([&] { return blueprints::load(options.assets / "blueprints/underkeep.json"); });
    CampaignDisk saveDisk(campaignSavePath(options.campaignSave));
    CampaignStore saves(catalog, saveDefinitions,
                        {fingerprintFile(options.assets / "rules/catalog.json"),
                         saveDefinitions.definitionFingerprint, &blueprints},
                        saveDisk);
    const auto refreshContinue = [&] {
        const auto slot = prepareCpu([&] {
            preparationPoint("Reading saved campaign");
            return saves.inspect();
        });
        v.canContinue = slot.active;
        if (!slot.error.empty())
            std::fprintf(stderr, "Continue unavailable: %s\n", slot.error.c_str());
    };
    refreshContinue();
    ArenaScenarioStore scenarioStore(catalog, characterLibraryPath().parent_path() / "scenarios");
    auto reload = [&] {
        const auto r = library.initialize();
        v.usable = bool(r);
        v.message(libraryFeedback(r));
        v.model.filter(library.snapshot());
    };
    prepareCpu([&] {
        preparationPoint("Reading character library");
        reload();
    });
    if (loading->closing())
        return 0;
    loading.reset();
    presenter.fade(0);
    Input input;
    audio.beginMenuMusic();
    const double menuStarted = GetTime();
    bool suppress = true;
    while (!WindowShouldClose()) {
        v.titleSeconds = GetTime() - menuStarted;
        presenter.presentMenu(drawMenu, &v);
        auto sample = input.poll(GetFrameTime());
        audio.update(nullptr, false, sample.ui.focused, GetFrameTime());
        auto& m = v.model;
        v.interaction.tick(GetFrameTime(), sample.ui.focused);
        if (!sample.ui.focused) {
            v.pointerButtons.cancel();
            v.creator.controls.buttons.cancel();
            v.arena.preparation.controls.buttons.cancel();
            v.arena.preparation.advancement.controls.buttons.cancel();
            v.arena.preparation.route(catalog, sample.ui, {}, 0);
            v.arena.blur();
            v.creation.model.nameEdit.tick(0, false);
            v.creation.model.searchEdit.tick(0, false);
            continue;
        }
        if (suppress) {
            while (GetCharPressed()) {
            };
            if (!IsKeyDown(KEY_ENTER) && !IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                suppress = false;
            continue;
        }
        if (v.creation.active) {
            const auto textInput = pollTextInput();
            routeCreator(v.creation.model, v.creator.controls, catalog, sample.ui, textInput, GetFrameTime());
            if (v.creation.model.canceled) {
                v.creation.cancel(m);
                suppress = true;
            } else if (v.creation.model.saveRequested) {
                const auto result = v.creation.save(library, m);
                if (result.committed) {
                    v.usable = bool(result);
                    v.message(result ? "Character saved." : libraryFeedback(result));
                    suppress = true;
                }
            }
            continue;
        }
        if (v.arena.preparation.active) {
            auto& flow = v.arena.preparation;
            flow.route(catalog, sample.ui, pollTextInput(), GetFrameTime());
            if (flow.commitRequested) {
                if (!flow.commit(catalog, v.arena.scenario))
                    v.message("Arena preparation is incomplete.");
            }
            if (!flow.active) {
                suppress = true;
                v.focus = 0;
            }
            continue;
        }
        if (v.interaction.routeDrag(m, library.snapshot(), sample.ui)) {
            v.pointerButtons.cancel();
            while (GetCharPressed()) {
            }
            continue;
        }
        sample.ui = v.pointerInput(sample.ui);
        const bool arenaScreen = m.screen == MenuScreen::Arena || m.screen == MenuScreen::Scenarios;
        if (arenaScreen) {
            auto& a = v.arena;
            const auto text = pollTextInput();
            a.nameEdit.tick(GetFrameTime(), sample.ui.focused);
            a.seedEdit.tick(GetFrameTime(), sample.ui.focused);
            a.searchEdit.tick(GetFrameTime(), sample.ui.focused);
            a.nameEdit.edit(a.scenario.name, text);
            a.seedEdit.edit(a.seedText, text);
            if (a.searchEdit.edit(a.search, text)) {
                a.first = 0;
                a.filter(catalog);
            }
        } else if (v.interaction.searchFocused && !m.deletion) {
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
        const bool arenaEditing = arenaScreen && (v.arena.nameEdit.focused || v.arena.searchEdit.focused ||
                                                  v.arena.seedEdit.focused);
        if (sample.ui.has(UiKey::Tab) || sample.ui.has(UiKey::Down) ||
            (!arenaEditing && sample.ui.has(UiKey::Right)))
            v.focus = (v.focus + (sample.ui.shift ? std::max(1, v.count) - 1 : 1)) % std::max(1, v.count);
        if (sample.ui.has(UiKey::Up) || (!arenaEditing && sample.ui.has(UiKey::Left)))
            v.focus = (v.focus + std::max(1, v.count) - 1) % std::max(1, v.count);
        if (sample.ui.has(UiKey::Tab) || sample.ui.has(UiKey::Up) || sample.ui.has(UiKey::Down) ||
            sample.ui.has(UiKey::Left) || sample.ui.has(UiKey::Right))
            v.interaction.focusSearch(v.focus < v.count && v.actions[v.focus] == 5);
        if (sample.ui.has(UiKey::Click) &&
            (!sample.ui.pointer || !menuLayout::search.contains(*sample.ui.pointer)))
            v.interaction.focusSearch(false);
        int action = 0;
        if (sample.ui.has(UiKey::Confirm) && !v.controlOverflow && v.focus < v.count && v.enabled[v.focus])
            action = v.actions[v.focus];
        if (sample.ui.has(UiKey::Click) && sample.ui.pointer)
            for (int i = 0; i < v.count; ++i)
                if (v.buttons[i].contains(*sample.ui.pointer)) {
                    v.focus = i;
                    action = v.actions[i];
                    break;
                }
        if (IsKeyPressed(KEY_ESCAPE))
            action = v.arena.pendingLoad >= 0            ? 4501
                     : m.deletion                        ? 40
                     : m.screen == MenuScreen::Scenarios ? 4010
                                                         : 4;
        if (!arenaScreen && !m.deletion && sample.ui.scroll)
            action = m.screen == MenuScreen::Detail ? (sample.ui.scroll > 0 ? 33 : 34)
                                                    : (sample.ui.scroll > 0 ? 31 : 32);
        if (!arenaScreen && !m.deletion &&
            (sample.ui.has(UiKey::PreviousPage) || sample.ui.has(UiKey::NextPage))) {
            const bool previous = sample.ui.has(UiKey::PreviousPage);
            action = m.screen == MenuScreen::Detail ? (previous ? 33 : 34) : (previous ? 31 : 32);
        }
        if (arenaScreen && v.arena.pendingLoad < 0 && sample.ui.scroll)
            action = m.screen == MenuScreen::Scenarios ? (sample.ui.scroll > 0 ? 4014 : 4015)
                                                       : (sample.ui.scroll > 0 ? 4007 : 4008);
        if (!action)
            continue;
        audio.play(action == 4 ? "ui.cancel" : "ui.confirm", 0, 0, false, true);
        if (action == 70) {
            runAudioSettings(presenter);
            suppress = true;
            continue;
        }
        if (arenaScreen)
            v.arena.blur();
        if (action >= 4000) {
            if (!arenaAction(v, action, options, scenarioStore, presenter))
                return 0;
            if (action < 4011 || action > 4013)
                v.focus = 0;
            suppress = true;
            continue;
        }
        if (action != 5)
            v.interaction.focusSearch(false);
        if (action == 60) {
            m.screen = MenuScreen::Loading;
            presenter.presentMenu(drawMenu, &v);
            try {
                if (!launchSavedCampaign(options, catalog, presenter, saves))
                    return 0;
                v.message("");
            } catch (const std::exception& e) {
                std::fprintf(stderr, "Continue failed: %s\n", e.what());
                v.message("Couldn't open this save. Your saved expedition has been kept.");
            }
            m.screen = MenuScreen::Main;
            refreshContinue();
            suppress = true;
            v.focus = 0;
            continue;
        }
        if (action == 50 && v.usable && library.snapshot().count < LibrarySnapshot::capacity) {
            v.creation.begin(catalog, m, library.snapshot());
            v.creator.controls = {};
            v.interaction.cancelDrag();
            v.interaction.focusSearch(false);
        } else if (action == 42) {
            m.search = {};
            m.filterClass = m.filterRace = false;
            m.filter(library.snapshot());
        } else if (action == 1) {
            m.arenaPicker = false;
            m.screen = MenuScreen::Picker;
        } else if (action == 2)
            m.screen = MenuScreen::Library;
        else if (action == 3)
            return 0;
        else if (action == 4) {
            if (m.screen == MenuScreen::Picker && m.arenaPicker) {
                m.arenaPicker = false;
                m.choices = v.dungeonChoices;
                m.screen = MenuScreen::Arena;
            } else
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
            if (v.detailOffset + 13 < v.detailRows)
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
            v.message(libraryFeedback(r));
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
                    saved |= c.preset < 0 && c.snapshot < 0;
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
                if (m.arenaPicker) {
                    StartingParty selected;
                    for (unsigned i = 0; i < 4; ++i) {
                        auto& member = selected[i];
                        rules::draftFromBuild(catalog, starting.characters[i], member.draft);
                        std::snprintf(member.id.data(), member.id.size(), "%s",
                                      starting.characters[i].id.c_str());
                    }
                    m.arenaPicker = false;
                    m.choices = v.dungeonChoices;
                    m.screen = MenuScreen::Arena;
                    const bool changed = v.arena.replaceParty(selected);
                    v.message(changed ? "Party selected. Arena preparation reset to level 1." : "");
                    v.focus = 0;
                    suppress = true;
                    continue;
                }
                m.screen = MenuScreen::Loading;
                presenter.presentMenu(drawMenu, &v);
                if (!launchMenuParty(options, catalog, starting, presenter, &saves))
                    return 0;
                m.screen = MenuScreen::Main;
                refreshContinue();
            } catch (const std::exception& e) {
                m.screen = MenuScreen::Picker;
                std::fprintf(stderr, "Party launch failed: %s\n", e.what());
                v.message("Couldn't start the expedition. Review your party and try again.");
            }
        }
        if (action != 5)
            v.focus = 0;
        suppress = true;
    }
    return 0;
} catch (const PreparationCancelled&) {
    return 0;
}
} // namespace blobber
