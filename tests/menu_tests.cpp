#include "app/menu_model.hpp"
#include "game/party.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "ui/menu_background.hpp"
#include "ui/menu_layout.hpp"
#include <iostream>
#include <limits>
#include <string_view>
using namespace blobber;
int main() {
    try {
        LibraryResult failed;
        failed.error = LibraryError::Io;
        std::snprintf(failed.message.data(), failed.message.size(), "Internal fixture revision exhausted");
        check(std::string_view(libraryFeedback(failed)) ==
                  "Couldn't update your character library. Please try again.",
              "Raw storage diagnostics never become library UI feedback");
        failed.error = LibraryError::DurabilityUncertain;
        check(std::string_view(libraryFeedback(failed)).find("before trying again") != std::string_view::npos,
              "Uncertain writes retain readable retry guidance");
        const auto catalog = creationCatalog();
        LibrarySnapshot library;
        library.count = 128;
        for (std::size_t i = 0; i < 128; ++i) {
            auto& r = library.records[i];
            std::snprintf(r.id.text.data(), 37, "00000000-0000-4000-8000-%012zu", i);
            r.draft = rules::recommendedDraft(catalog, {std::uint16_t(i % 4)}, {0});
            std::snprintf(r.draft.name.data(), 65, "Adventurer %zu", i);
        }
        MenuModel m;
        m.filter(library);
        check(m.visibleCount == 128, "full bounded list");
        std::snprintf(m.search.data(), 65, "ADVENTURER 12");
        m.filter(library);
        check(m.visibleCount == 9, "case insensitive bounded search");
        m.search = {};
        m.filter(library);
        for (int i = 0; i < 4; ++i) {
            m.activeSlot = i;
            m.assign({library.records[i].id, -1});
        }
        check(m.ready(library), "same-class party allowed");
        auto build = m.resolve(catalog, library);
        Party party(catalog, build);
        check(party.character({0})->build.name == "Adventurer 0", "live identity");
        m.activeSlot = 0;
        m.assign({library.records[3].id, -1});
        check(m.choices[3].saved == library.records[0].id, "assigned record swaps");
        m.confirmDelete(library, library.records[3].id);
        m.filterRace = true;
        m.race = {1};
        m.filter(library);
        check(m.deletion->id == library.records[3].id, "confirmation identity survives filtering");
        library.records[3].id = {};
        check(!m.ready(library), "stale identity rejected");
        m.defaults();
        LibrarySnapshot empty;
        check(m.ready(empty), "defaults independent of library");
        auto original = m.resolve(catalog, empty);
        Party first(catalog, original);
        first.damage({0}, 1);
        Party restarted(catalog, original);
        check(first.character({0})->hitPoints + 1 == restarted.character({0})->hitPoints, "fresh resources");
        auto kitCount = [&](const Party& p) {
            unsigned count = 0;
            for (std::uint16_t i = 0; i < 4; ++i) {
                const auto load = p.inventory().loadout({i});
                for (const auto& item : load.items)
                    if (item.item.index == catalog.findItem("camp_kit").index)
                        count += item.quantity;
            }
            return count;
        };
        check(kitCount(first) == 1 && kitCount(restarted) == 1,
              "each independent launch has exactly one kit");
        m.activeSlot = 1;
        m.assign({});
        check(!m.ready(empty), "empty slot blocks launch");
        MenuModel dragModel;
        dragModel.screen = MenuScreen::Picker;
        dragModel.filter(library);
        MenuInteraction interaction;
        auto press = [&](ArtPoint point) {
            UiInput in;
            in.pointer = point;
            in.pointerHeld = true;
            in.pressed[std::size_t(UiKey::Click)] = true;
            check(interaction.routeDrag(dragModel, library, in), "drag press consumed");
        };
        auto move = [&](ArtPoint point) {
            UiInput in;
            in.pointer = point;
            in.pointerHeld = true;
            interaction.routeDrag(dragModel, library, in);
        };
        auto release = [&](std::optional<ArtPoint> point) {
            UiInput in;
            in.pointer = point;
            in.pointerReleased = true;
            interaction.routeDrag(dragModel, library, in);
        };
        press({40, 100});
        move({450, 120});
        check(interaction.dragging && interaction.target == 0 && dragModel.choices[0].empty(),
              "drag previews without mutation");
        release(ArtPoint{450, 120});
        check(dragModel.choices[0].saved == library.records[0].id, "drop assigns saved identity");
        press({450, 120});
        move({550, 230});
        release(ArtPoint{550, 230});
        check(dragModel.choices[0].empty() && dragModel.choices[3].saved == library.records[0].id,
              "formation drag moves to empty slot");
        press({170, 100});
        move({450, 120});
        release(ArtPoint{450, 120});
        press({550, 230});
        move({450, 120});
        release(ArtPoint{450, 120});
        check(dragModel.choices[0].saved == library.records[0].id &&
                  dragModel.choices[3].saved == library.records[1].id,
              "occupied drop swaps characters");
        const auto before = dragModel.choices;
        press({40, 100});
        release(ArtPoint{41, 100});
        check(dragModel.choices == before, "click below threshold cannot assign");
        press({40, 100});
        move({320, 340});
        release(std::nullopt);
        check(dragModel.choices == before, "outside release cancels");
        press({40, 100});
        move({450, 120});
        UiInput cancel;
        cancel.pressed[std::size_t(UiKey::Cancel)] = true;
        interaction.routeDrag(dragModel, library, cancel);
        check(!interaction.pointerDown && dragModel.choices == before, "escape cancels drag");
        press({40, 100});
        interaction.tick(.1f, false);
        check(!interaction.pointerDown && dragModel.choices == before, "blur cancels drag");
        press({40, 100});
        move({450, 120});
        auto saved = library.records[0].id;
        library.records[0].id = {};
        release(ArtPoint{450, 120});
        check(dragModel.choices == before, "stale source cannot commit");
        library.records[0].id = saved;
        interaction.focusSearch(true);
        check(interaction.caretVisible(), "focus starts visible caret");
        interaction.tick(.6f, true);
        check(!interaction.caretVisible(), "caret blinks off");
        interaction.edited();
        check(interaction.caretVisible(), "edit resets caret");
        interaction.tick(.6f, true);
        interaction.tick(.5f, true);
        check(interaction.caretVisible(), "caret repeats");
        interaction.tick(.1f, false);
        check(!interaction.caretVisible() && !interaction.searchFocused, "blur removes text focus");
        MenuModel clicks;
        clicks.screen = MenuScreen::Picker;
        clicks.filter(library);
        MenuInteraction clicking;
        const auto row = menuLayout::available(0);
        UiInput downClick;
        downClick.pressed[std::size_t(UiKey::Click)] = true;
        downClick.pointerHeld = true;
        downClick.pointer = ArtPoint{row.x + .5f, row.y + .5f};
        clicking.routeDrag(clicks, library, downClick);
        auto held = downClick;
        held.pressed.fill(false);
        held.pointer = ArtPoint{row.x - 1.f, row.y + .5f};
        clicking.routeDrag(clicks, library, held);
        held.pointer = downClick.pointer;
        clicking.routeDrag(clicks, library, held);
        held.pointerHeld = false;
        held.pointerReleased = true;
        clicking.routeDrag(clicks, library, held);
        check(clicks.choices[0].empty(), "party-row leave/reenter cancels even below the drag threshold");
        const auto start = menuBackgroundCrop(0), midpoint = menuBackgroundCrop(24);
        const auto sameCrop = [](MenuBackgroundCrop a, MenuBackgroundCrop b) {
            return std::abs(a.x - b.x) < 1e-6f && std::abs(a.y - b.y) < 1e-6f &&
                   std::abs(a.width - b.width) < 1e-6f && std::abs(a.height - b.height) < 1e-6f;
        };
        check(!sameCrop(start, midpoint), "Background motion changes the crop over time");
        check(sameCrop(start, menuBackgroundCrop(48)) && sameCrop(start, menuBackgroundCrop(48 - 1e-5)),
              "Background returns smoothly to its starting crop");
        check(sameCrop(start, menuBackgroundCrop(-1)) &&
                  sameCrop(start, menuBackgroundCrop(std::numeric_limits<double>::quiet_NaN())) &&
                  sameCrop(start, menuBackgroundCrop(std::numeric_limits<double>::infinity())),
              "Invalid background times safely use the initial crop");
        for (unsigned sample = 0; sample <= 96; ++sample) {
            const auto crop = menuBackgroundCrop(sample * .5);
            check(crop.width > 0 && crop.height > 0 && crop.x >= 0 && crop.y >= 0 &&
                      crop.x + crop.width <= 1 && crop.y + crop.height <= 1,
                  "Background stays inside its source throughout the pan and zoom");
        }
        const auto late = menuBackgroundCrop(1e12);
        check(late.x >= 0 && late.y >= 0 && late.x + late.width <= 1 && late.y + late.height <= 1,
              "Very long decorative runtimes retain valid source bounds");
        std::cout << "Menu tests passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
