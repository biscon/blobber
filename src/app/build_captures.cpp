#include "app/build_captures.hpp"
#include "rules/creation.hpp"
#include <fstream>
#include <stdexcept>
namespace blobber {
void captureBuilds(const Map& map, const rules::Catalog& catalog, const NpcWorld& source,
                   const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                   Presenter& presenter, const std::filesystem::path& directory) {
    std::vector<std::string> types;
    for (std::uint16_t n = 0; n < source.typeCount(); ++n)
        types.push_back(source.typeId({n}));
    const NpcWorld empty(map, Player{}.cell, types, {});
    std::ofstream trace(directory / "builds.tsv");
    trace << "portrait\tcharacter\tclass\tformation_slot\tart_id\n";
    for (unsigned batch = 0; batch < 5; ++batch) {
        rules::PresetParty builds;
        for (std::uint16_t i = 0; i < 4; ++i) {
            const auto index = std::uint8_t(batch * 4 + i);
            const auto& portrait = *rules::portrait({index});
            auto draft = rules::recommendedDraft(catalog, catalog.findRace(portrait.race), {i});
            draft.portrait = {index};
            std::snprintf(draft.name.data(), draft.name.size(), "%s", portrait.id);
            if (i == 3 || i == 1)
                draft.gear[0] = 1; // quarterstaff production art
            builds.characters[i] = rules::finishBuild(catalog, draft, "capture." + std::to_string(index));
        }
        builds.formation = {{{3}, {1}, {0}, {2}}};
        RecoveryLevel recovery;
        recovery.ground[recovery.groundCount++] = {Player{}.cell, catalog.findItem("healing_potion"), 1};
        GameSession session(map, catalog, builds, empty, 5, Player{}, false, timings, recovery);
        CommandController ui(session, timings);
        NpcAnimator animator(session.actors(), timings);
        FramePipeline frames;
        auto refresh = [&] {
            animator.setTime(0, &session.effects());
            frames.update(renderer, map, session.movement(), &session.actors(), &animator.snapshot(),
                          &session.combat().attacks(), 0);
            ui.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false, true);
        };
        auto save = [&](const std::string& name) {
            refresh();
            presenter.present(frames.frame(), session.movement(), map, false, true, &ui.snapshot(),
                              frames.picking());
            presenter.capture(directory / (name + ".png"));
            presenter.capture(directory / (name + "-1280.png"), 1280, 720);
            presenter.capture(directory / (name + "-4x3.png"), 1024, 768);
        };
        refresh();
        save("builds-" + std::to_string(batch) + "-hud");
        for (std::uint16_t slot = 0; slot < 4; ++slot) {
            const auto id = builds.formation[slot];
            const auto& b = builds.characters[id.index];
            if (ui.snapshot().cards[slot].portrait != 64u + b.portrait.index)
                throw std::runtime_error("Portrait identity changed with formation");
            trace << rules::portrait(b.portrait)->id << '\t' << b.id << '\t'
                  << catalog.characterClass(b.characterClass)->id << '\t' << slot << '\t'
                  << ui.snapshot().cards[slot].portrait << '\n';
            ui.selectCharacter(slot);
            ui.openInventory(id, CharacterPage::Character);
            save("build-" + std::to_string(b.portrait.index) + "-sheet");
            ui.inventoryPage(CharacterPage::Inventory);
            save("build-" + std::to_string(b.portrait.index) + "-inventory");
            ui.inventoryCancel();
            ui.openSpellbook();
            save("build-" + std::to_string(b.portrait.index) + "-spells");
            ui.cancel();
        }
        ui.selectCharacter(1); // cleric retains this slot in the nontrivial formation
        ui.openSpellbook();
        for (std::size_t n = 0; n < ui.snapshot().magic.count; ++n)
            if (ui.snapshot().magic.entries[n].spell.index == catalog.findSpell("bless").index) {
                ui.chooseSpellbook(n);
                break;
            }
        ui.magicAlly(0);
        ui.magicAlly(2);
        save("builds-" + std::to_string(batch) + "-recipients");
        ui.cancel();
        ui.openLoot({session.actors().snapshot().world, 2048});
        save("builds-" + std::to_string(batch) + "-loot");
    }
}
} // namespace blobber
