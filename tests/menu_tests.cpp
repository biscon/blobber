#include "app/menu_model.hpp"
#include "game/party.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "ui/menu_layout.hpp"
#include <iostream>
using namespace blobber;
int main() {
    try {
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
        check(kitCount(first) == 3 && kitCount(restarted) == 3,
              "each independent launch has exactly three kits");
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
        std::cout << "Menu tests passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
