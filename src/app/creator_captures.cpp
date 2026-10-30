#include "app/creator_captures.hpp"
#include "app/menu.hpp"
#include <cstdio>
#include <fstream>
#include <memory>
#include <stdexcept>
namespace blobber {
namespace {
int failing = -1;
void require(bool okay, const char* why) {
    if (!okay)
        throw std::runtime_error(why);
}
UiInput click(UiRect r) {
    UiInput in;
    in.pointer = ArtPoint{float(r.x + r.width / 2), float(r.y + r.height / 2)};
    in.pressed[std::size_t(UiKey::Click)] = true;
    return in;
}
void activate(MenuView& v, FlowAction action, int value = 0) {
    auto& m = v.creation.model;
    auto& w = v.creator.controls;
    layoutCreator(w, m, v.catalog);
    for (unsigned i = 0; i < w.count; ++i)
        if (w.entries[i].action == action && w.entries[i].value == value) {
            routeCreator(m, w, v.catalog, click(w.entries[i].rect), {}, 0);
            return;
        }
    throw std::runtime_error("Capture control missing");
}
} // namespace
bool creatorCaptureFailStage(LibraryStage stage) {
    return int(stage) == failing;
}
void captureCreator(MenuView& v, Presenter& presenter, const Options& options) {
    std::filesystem::create_directories(options.captureDirectory);
    auto capture = [&](const std::string& name) {
        presenter.presentMenu(drawMenu, &v);
        presenter.capture(options.captureDirectory / (name + ".png"));
        presenter.capture(options.captureDirectory / (name + "-1280.png"), 1280, 720);
        presenter.capture(options.captureDirectory / (name + "-4x3.png"), 1024, 768);
    };
    capture("01-main");
    v.model.screen = MenuScreen::Library;
    capture("02-library");
    v.model.screen = MenuScreen::Picker;
    v.model.defaults();
    capture("03-picker");
    v.creation.begin(v.catalog, v.model, v.library.snapshot());
    v.creator.controls = {};
    capture("05-empty-identity");
    TextInput text;
    std::snprintf(text.text.data(), 65, "Mira");
    routeCreator(v.creation.model, v.creator.controls, v.catalog, {}, text, 0);
    capture("05-identity");
    const auto shortName = v.creation.model.draft.name;
    v.creation.model.draft.name.fill('W');
    v.creation.model.draft.name[64] = 0;
    v.creation.model.nameEdit.focus(true, v.creation.model.draft.name);
    v.creation.model.refresh(v.catalog);
    capture("05-long-name");
    v.creation.model.draft.name = shortName;
    v.creation.model.refresh(v.catalog);
    activate(v, FlowAction::Next);
    capture("06-class-empty");
    activate(v, FlowAction::Class, 3);
    capture("06-class");
    activate(v, FlowAction::Next);
    capture("07-point-buy");
    activate(v, FlowAction::Reset);
    capture("07-unspent");
    activate(v, FlowAction::Recommend);
    activate(v, FlowAction::Next);
    capture("08-skills");
    activate(v, FlowAction::Next);
    capture("09-cantrips");
    v.creation.model.selection.search = {};
    std::snprintf(v.creation.model.selection.search.data(), 65, "No such spell");
    v.creation.model.selection.filter(v.catalog);
    capture("09-no-results");
    v.creation.model.selection.search = {};
    v.creation.model.selection.filter(v.catalog);
    activate(v, FlowAction::Next);
    capture("10-spellbook");
    UiInput scroll;
    scroll.scroll = -1;
    scroll.pointer = ArtPoint{300, 150};
    routeCreator(v.creation.model, v.creator.controls, v.catalog, scroll, {}, 0);
    capture("10-spellbook-scrolled");
    activate(v, FlowAction::Next);
    capture("11-preparation");
    activate(v, FlowAction::Next);
    capture("12-equipment");
    activate(v, FlowAction::Next);
    capture("13-review");
    for (unsigned i = 0; i < 14; ++i)
        activate(v, FlowAction::Scroll, 1);
    capture("13-review-equipment");
    v.creation.model.reviewOffset = 0;
    activate(v, FlowAction::Cancel);
    capture("16-discard");
    activate(v, FlowAction::Keep);
    const auto before = v.library.snapshot().count;
    failing = int(LibraryStage::Write);
    activate(v, FlowAction::Save);
    const auto failed = v.creation.save(v.library, v.model);
    failing = -1;
    require(!failed.committed && v.creation.active, "Capture failed-write contract");
    capture("13-save-error");
    activate(v, FlowAction::Save);
    const auto saved = v.creation.save(v.library, v.model);
    require(saved.committed && v.library.snapshot().count == before + 1, "Capture save failed");
    capture("03-return-highlight");
    // Full production-model creation + durable save for every supported combination.
    for (std::uint16_t race = 0; race < 4; ++race)
        for (std::uint16_t cls = 0; cls < 4; ++cls) {
            v.model.screen = MenuScreen::Library;
            v.creation.begin(v.catalog, v.model, v.library.snapshot());
            v.creator.controls = {};
            TextInput name;
            std::snprintf(name.text.data(), 65, "Custom %u %u", race, cls);
            routeCreator(v.creation.model, v.creator.controls, v.catalog, {}, name, 0);
            activate(v, FlowAction::Race, race);
            activate(v, FlowAction::Next);
            activate(v, FlowAction::Class, cls);
            // Select a non-default matching portrait while retaining all other choices.
            auto& m = v.creation.model;
            for (std::uint8_t p = 4; p < rules::portraitCount; ++p)
                if (v.catalog.race({race})->id == rules::portrait({p})->race)
                    m.draft.portrait = {p};
            m.refresh(v.catalog);
            unsigned guard = 0;
            while (m.step != CreatorStep::Review && ++guard < 12) {
                activate(v, FlowAction::Next);
                if ((race == 3 && cls == 0) || (race == 1 && cls == 1) || (race == 0 && cls == 0))
                    capture("path-" + std::to_string(race) + "-" + std::to_string(cls) + "-" +
                            creatorStepName(m.step));
            }
            require(m.step == CreatorStep::Review && bool(m.preview), "Capture combination invalid");
            capture("review-" + std::to_string(race) + "-" + std::to_string(cls));
            activate(v, FlowAction::Save);
            const auto result = v.creation.save(v.library, v.model);
            require(result.committed, "Capture combination save");
            const auto index = std::size_t(race) * 4 + cls;
            if (index < 4) {
                v.model.activeSlot = index;
                v.model.assign({result.id, -1});
            }
        }
    v.model.screen = MenuScreen::Library;
    v.model.firstRow = 0;
    capture("02-many-row-library");
    v.model.screen = MenuScreen::Picker;
    capture("03-created-party");
    const auto chosen = v.model.resolve(v.catalog, v.library.snapshot());
    require(bool(v.library.load()), "Capture restart reload");
    launchMenuParty(options, v.catalog, chosen, presenter);
    // Shared rest panel, real transactional API, isolated empty-world session.
    const auto map = Map::load((options.assets / "maze.txt").string());
    const NpcWorld empty(map, Player{}.cell, {}, {});
    auto restBuilds = *v.catalog.preset();
    restBuilds.characters[3].name = std::string(64, 'W');
    auto owner = std::make_unique<GameSession>(map, v.catalog, restBuilds, empty, 5, Player{}, false);
    CommandController commands(*owner);
    commands.refresh({}, false, false, true);
    commands.openCamp();
    commands.campKind(RestKind::Long);
    auto campCapture = [&](const char* name) {
        auto draw = [](void* ctx, const UiFonts& fonts, const UiArt& art) {
            const auto& ui = *static_cast<const CommandController*>(ctx);
            HudPainter painter(&fonts, &art);
            painter.fill({0, 0, 640, 360}, hudInk::background);
            painter.camp(ui.snapshot());
        };
        presenter.presentMenu(draw, &commands);
        for (const auto size : {0, 1, 2})
            presenter.capture(options.captureDirectory / (std::string(name) + (size == 0   ? ".png"
                                                                               : size == 1 ? "-1280.png"
                                                                                           : "-4x3.png")),
                              size == 0   ? 1920
                              : size == 1 ? 1280
                                          : 1024,
                              size == 0   ? 1080
                              : size == 1 ? 720
                                          : 768);
    };
    campCapture("rest-keep-current");
    commands.openPreparation();
    require(commands.snapshot().camp.preparing, "Capture prep unavailable");
    campCapture("rest-cleric");
    auto h = commands.snapshot().camp;
    FlowControls controls;
    layoutPreparation(controls, h);
    for (unsigned i = 0; i < controls.count; ++i)
        if (controls.entries[i].action == FlowAction::Member && controls.entries[i].value == 3)
            commands.routePreparation(click(controls.entries[i].rect));
    campCapture("rest-wizard");
    h = commands.snapshot().camp;
    layoutPreparation(controls, h);
    for (unsigned i = 0; i < controls.count; ++i)
        if (controls.entries[i].action == FlowAction::Toggle && controls.entries[i].enabled) {
            commands.routePreparation(click(controls.entries[i].rect));
            break;
        }
    campCapture("rest-changed");
    h = commands.snapshot().camp;
    layoutPreparation(controls, h);
    for (unsigned i = 0; i < controls.count; ++i)
        if (controls.entries[i].action == FlowAction::Done)
            commands.routePreparation(click(controls.entries[i].rect));
    campCapture("rest-staged");
    commands.startRest();
    require(commands.snapshot().camp.playing, "Capture rest commit");
    campCapture("rest-complete");
    std::ofstream report(options.captureDirectory / "capture-results.txt");
    report << "16 race/class builds created through creator controls, saved and reloaded.\n"
           << "Injected pre-commit write failure retained draft; retry committed once.\n"
           << "Four created characters launched through production menu launch and reconstructed.\n"
           << "Long Rest preparation committed before cosmetic playback. No OS input automation.\n";
}
} // namespace blobber
