#include "app/spell_captures.hpp"
#include "app/ai_demo.hpp"
#include "ui/interaction.hpp"
#include <fstream>
#include <stdexcept>
namespace blobber {
void captureSpells(const rules::Catalog& catalog, const NpcWorld& source,
                   const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                   Presenter& presenter, const std::filesystem::path& directory) {
    const auto map = aiDemoMap();
    const Player spawn{{8, 11}, Heading::North};
    std::vector<std::string> names;
    NpcTypeId goblin;
    for (std::size_t i = 0; i < source.typeCount(); ++i) {
        names.push_back(source.typeId({static_cast<std::uint16_t>(i)}));
        if (names.back() == "goblin")
            goblin = {static_cast<std::uint16_t>(i)};
    }
    std::ofstream trace(directory / "spells.tsv");
    if (!trace)
        throw std::runtime_error("Cannot write spell trace");
    trace << "case\tsession_ms\tgameplay_ms\tmana_cleric\tmana_wizard\tphase\n";
    for (unsigned k = 0; k < static_cast<unsigned>(rules::SpellKind::Count) + 2; ++k) {
        const bool sparse = k >= static_cast<unsigned>(rules::SpellKind::Count);
        const auto kind =
            sparse ? (k == static_cast<unsigned>(rules::SpellKind::Count) ? rules::SpellKind::BurningHands
                                                                          : rules::SpellKind::Thunderwave)
                   : static_cast<rules::SpellKind>(k);
        const auto& rule = *rules::spellRules(kind);
        const bool close =
            rule.area || (kind == rules::SpellKind::InflictWounds ||
                          kind == rules::SpellKind::ShockingGrasp || kind == rules::SpellKind::PoisonSpray);
        std::vector<NpcInstance> instances;
        if (rule.hostile)
            for (int i = 0; i < (sparse ? 3 : rule.area ? 12 : 4); ++i)
                instances.push_back({goblin,
                                     {8 + (sparse      ? (i == 0   ? 0
                                                          : i == 1 ? -1
                                                                   : 1)
                                           : rule.area ? i / 4 - 1
                                                       : 0),
                                      sparse && i ? 9
                                      : close     ? 10
                                                  : 9},
                                     {i % 2, (i / 2) % 2},
                                     Heading::North});
        NpcWorld placements(map, spawn.cell, names, instances);
        GameSession s(map, catalog, placements, 5, spawn, false, timings);
        if (kind == rules::SpellKind::ShockingGrasp) {
            s.party().lockFormation(false);
            s.party().swapFormation(FormationSlot::FrontRight, FormationSlot::BackRight);
            s.party().lockFormation(true);
        }
        NpcAnimator animator(s.actors(), timings);
        FramePipeline frames;
        CommandController ui(s, timings);
        const unsigned caster = rule.casting == rules::Casting::Wisdom ? 1 : 3;
        bool focused = true;
        auto refresh = [&](std::uint64_t animationOffset = 0) {
            animator.setTime(s.timeMs() + animationOffset);
            const auto area = ui.floorPreview();
            frames.update(renderer, map, s.movement(), &s.actors(), &animator.snapshot(),
                          &s.combat().attacks(), s.timeMs(), &area);
            ui.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{},
                       s.movement().transition.active(), false, focused);
        };
        auto capture = [&](const std::string& suffix, std::uint64_t animationOffset = 0) {
            refresh(animationOffset);
            presenter.present(frames.frame(), s.movement(), map, false, true, &ui.snapshot(),
                              frames.picking());
            const auto name = std::string("spell-") + rule.id + (sparse ? "-sparse-" : "-") + suffix;
            presenter.capture(directory / (name + ".png"));
            if (suffix == "book" || suffix == "target" || suffix == "partial" || suffix == "unfocused" ||
                suffix == "limit" || suffix == "unavailable" || suffix == "concentration")
                presenter.capture(directory / (name + "-1280.png"), 1280, 720);
            if (k >= 14 && (suffix == "target" || suffix == "cast" || suffix == "book"))
                presenter.capture(directory / (name + "-1024.png"), 1024, 768);
            trace << name << '\t' << s.timeMs() << '\t' << s.combat().snapshot().gameplayMs << '\t'
                  << s.party().character({1})->mana << '\t' << s.party().character({3})->mana << '\t'
                  << int(s.combat().snapshot().phase) << '\n';
        };
        if (close) {
            for (int i = 0; i < 500; ++i) {
                const auto& st = s.combat().snapshot();
                if (st.awaitingAction && st.activeCharacter && st.activeCharacter->index == caster)
                    break;
                if (st.awaitingAction) {
                    PartyCommand defend;
                    defend.character = *st.activeCharacter;
                    defend.action = PartyAction::Defend;
                    defend.world = st.world;
                    defend.turn = st.turn;
                    s.submit(defend);
                    ui.record(s.events());
                }
                ui.record(s.update(.1, false));
            }
        }
        if (kind == rules::SpellKind::CureWounds || kind == rules::SpellKind::SpareTheDying)
            s.party().damage({0}, s.party().character({0})->hitPoints);
        refresh();
        for (unsigned slot = 0; slot < rules::partySize; ++slot)
            if (s.party().snapshot().formation[slot].index == caster)
                ui.selectCharacter(slot);
        UiInput open;
        open.pressed[static_cast<std::size_t>(UiKey::Four)] = true;
        routeHudInput(ui, open);
        std::size_t entry = ui.snapshot().magic.count;
        const auto spell = catalog.findSpell(rule.id);
        for (std::size_t i = 0; i < ui.snapshot().magic.count; ++i)
            if (ui.snapshot().magic.entries[i].spell.index == spell.index)
                entry = i;
        if (entry == ui.snapshot().magic.count)
            throw std::runtime_error("Capture spellbook missing entry");
        ui.browseSpellbook(int(entry) - int(ui.snapshot().magic.cursor));
        capture("book");
        UiInput choose;
        choose.pressed[static_cast<std::size_t>(UiKey::Confirm)] = true;
        routeHudInput(ui, choose);
        if (kind == rules::SpellKind::MagicMissile)
            capture("empty");
        if (rule.ground)
            ui.magicGround(Cell{8, 10}, true);
        if (!rule.area && !rule.self) {
            if (rule.hostile) {
                // Pick visible bodies, as production UI does. Acid Splash shares their cell.
                const auto targets = ui.snapshot().targets;
                const auto count = std::min<std::size_t>(rule.maxTargets, targets.count);
                if ((kind == rules::SpellKind::InflictWounds || kind == rules::SpellKind::ShockingGrasp ||
                     kind == rules::SpellKind::PoisonSpray)) {
                    for (std::size_t i = 0; i < targets.count; ++i)
                        if (s.combat().exposed(targets.ids[i])) {
                            ui.magicEnemy(targets.ids[i]);
                            break;
                        }
                } else
                    for (std::size_t i = 0; i < (kind == rules::SpellKind::MagicMissile ? 3 : count) && count;
                         ++i)
                        ui.magicEnemy(targets.ids[i % count]);
            } else {
                const unsigned ally = kind == rules::SpellKind::MageArmor ? 3 : 0;
                ui.magicAlly(ally);
                if (kind == rules::SpellKind::Bless) {
                    ui.magicAlly(1);
                    ui.magicAlly(2);
                }
            }
        }
        capture("target");
        if (kind == rules::SpellKind::MagicMissile) {
            // Keep simulation still while exercising every idle pose in the rendered UI.
            for (unsigned offset : {200u, 400u, 600u})
                capture("idle-" + std::to_string(offset), offset);
            const auto first = ui.snapshot().magic.pending.enemies[0];
            ui.magicEnemy(first);
            capture("limit");
            ui.magicEnemy(first, true);
            capture("partial");
            ui.magicEnemy(first);
            focused = false;
            capture("unfocused");
            focused = true;
            refresh();
        }
        if (kind == rules::SpellKind::Bless) {
            ui.magicAlly(3);
            capture("limit");
            ui.removeMagicRecipient(1);
            capture("partial");
            ui.magicAlly(1);
        }
        const auto reason = ui.confirm();
        if (reason != CommandReason::Resolved)
            throw std::runtime_error(std::string("Spell capture ") + rule.id + ": " + commandMessage(reason));
        capture("cast");
        if (kind == rules::SpellKind::Bless) {
            ui.openSpellbook();
            ui.browseSpellbook(int(ui.snapshot().magic.count) - 1 - int(ui.snapshot().magic.cursor));
            capture("concentration");
            ui.closeOverlay();
        }
        const auto start = s.timeMs();
        for (unsigned delta : {100u, 199u, 200u, 341u, 342u, 400u, 650u, 1400u}) {
            ui.record(s.update(double(start + delta - s.timeMs()) / 1000, false));
            capture(std::to_string(delta));
        }
        if (kind == rules::SpellKind::MagicMissile) {
            ui.record(s.update(6, false));
            refresh();
            s.party().spendMana({3}, s.party().character({3})->mana);
            ui.selectCharacter(3);
            ui.openSpellbook();
            ui.browseSpellbook(3 - int(ui.snapshot().magic.cursor));
            capture("unavailable");
        }
    }
    // Feature picker and actual self-recovery through the same command UI.
    for (unsigned scenario = 0; scenario < 5; ++scenario) {
        const unsigned member = scenario == 2 ? 3 : scenario % 3 == 0 ? 0 : 2;
        const bool combat = scenario >= 3;
        std::vector<NpcInstance> instances;
        if (combat)
            instances.push_back({goblin, {8, 10}, {0, 1}, Heading::South});
        NpcWorld places(map, spawn.cell, names, instances);
        GameSession s(map, catalog, places, 5, spawn, false, timings);
        s.party().spendMana({3}, 3);
        NpcAnimator animator(s.actors(), timings);
        FramePipeline frames;
        CommandController ui(s, timings);
        const auto refresh = [&] {
            animator.setTime(s.timeMs());
            frames.update(renderer, map, s.movement(), &s.actors(), &animator.snapshot(),
                          &s.combat().attacks(), s.timeMs());
            ui.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false, true);
        };
        const auto capture = [&](const char* suffix, bool empty = false) {
            refresh();
            auto hud = ui.snapshot();
            if (empty) // Presentation-only diagnostic of a class without any features.
                hud.magic.count = hud.magic.cursor = hud.magic.offset = 0;
            presenter.present(frames.frame(), s.movement(), map, false, true, &hud, frames.picking());
            const auto name =
                "feature-" + std::to_string(member) + (combat ? "-combat-" : "-explore-") + suffix;
            presenter.capture(directory / (name + ".png"));
            presenter.capture(directory / (name + "-1280.png"), 1280, 720);
        };
        if (combat) {
            for (int i = 0; i < 300; ++i) {
                const auto& st = s.combat().snapshot();
                if (st.awaitingAction && st.activeCharacter && st.activeCharacter->index == member)
                    break;
                if (st.awaitingAction) {
                    PartyCommand defend;
                    defend.character = *st.activeCharacter;
                    defend.action = PartyAction::Defend;
                    defend.world = st.world;
                    defend.turn = st.turn;
                    s.submit(defend);
                    ui.record(s.events());
                }
                ui.record(s.update(.1, false));
            }
            if (!s.combat().snapshot().awaitingAction ||
                s.combat().snapshot().activeCharacter->index != member)
                throw std::runtime_error("Feature capture did not reach character turn");
        }
        refresh();
        ui.selectCharacter(member);
        capture("actions");
        ui.openSpellbook();
        capture("book");
        if (member == 2) {
            ui.chooseSpellbook(1);
            capture("rear-row");
            if (!ui.snapshot().magic.open)
                throw std::runtime_error("Automatic feature must remain in the picker");
            capture("empty", true);
        }
        if (member != 2) {
            if (member == 0) {
                ui.chooseSpellbook(1);
                capture("defense");
                s.party().damage({0}, 6);
                refresh();
                ui.browseSpellbook(-1);
                capture("injured");
            }
            const auto feature = member == 0 ? rules::Feature::SecondWind : rules::Feature::ArcaneRecovery;
            std::size_t index = 0;
            while (index < ui.snapshot().magic.count && ui.snapshot().magic.entries[index].feature != feature)
                ++index;
            ui.chooseSpellbook(index);
            capture("confirm");
            if (ui.confirm() != CommandReason::Resolved)
                throw std::runtime_error("Feature capture action rejected");
            // Revisit the fighter after readiness/turn handling to show the actual exhausted reason.
            if (!combat)
                ui.record(s.update(6, false));
            else {
                for (int i = 0; i < 300; ++i) {
                    const auto& st = s.combat().snapshot();
                    if (st.awaitingAction && st.activeCharacter && st.activeCharacter->index == member)
                        break;
                    if (st.awaitingAction) {
                        PartyCommand defend;
                        defend.character = *st.activeCharacter;
                        defend.action = PartyAction::Defend;
                        defend.world = st.world;
                        defend.turn = st.turn;
                        s.submit(defend);
                        ui.record(s.events());
                    }
                    ui.record(s.update(.1, false));
                }
            }
            refresh();
            ui.selectCharacter(member);
            ui.openSpellbook();
            ui.browseSpellbook(int(index) - int(ui.snapshot().magic.cursor));
            capture("used");
        }
    }
}
} // namespace blobber
