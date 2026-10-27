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
        const bool close = rule.area || kind == rules::SpellKind::InflictWounds;
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
        ui.selectCharacter(caster);
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
        if (!rule.area) {
            if (rule.hostile) {
                // Pick visible bodies, as production UI does. Acid Splash shares their cell.
                const auto targets = ui.snapshot().targets;
                const auto count = std::min<std::size_t>(rule.maxTargets, targets.count);
                if (kind == rules::SpellKind::InflictWounds) {
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
    for (unsigned member : {0u, 2u, 3u}) {
        NpcWorld places(map, spawn.cell, names, {});
        GameSession s(map, catalog, places, 5, spawn, false, timings);
        s.party().damage({0}, 6);
        s.party().spendMana({3}, 3);
        NpcAnimator animator(s.actors(), timings);
        FramePipeline frames;
        CommandController ui(s, timings);
        animator.setTime(0);
        frames.update(renderer, map, s.movement(), &s.actors(), &animator.snapshot(), &s.combat().attacks(),
                      0);
        ui.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false, true);
        ui.selectCharacter(member);
        ui.openSpellbook();
        presenter.present(frames.frame(), s.movement(), map, false, true, &ui.snapshot(), frames.picking());
        presenter.capture(directory / ("spell-feature-" + std::to_string(member) + ".png"));
        if (member != 2) {
            const auto feature = member == 0 ? rules::Feature::SecondWind : rules::Feature::ArcaneRecovery;
            std::size_t index = 0;
            while (index < ui.snapshot().magic.count && ui.snapshot().magic.entries[index].feature != feature)
                ++index;
            ui.chooseSpellbook(index);
            if (ui.confirm() != CommandReason::Resolved)
                throw std::runtime_error("Feature capture action rejected");
            ui.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false, true);
            ui.openSpellbook();
            ui.browseSpellbook(int(index) - int(ui.snapshot().magic.cursor));
            presenter.present(frames.frame(), s.movement(), map, false, true, &ui.snapshot(),
                              frames.picking());
            presenter.capture(directory / ("spell-feature-" + std::to_string(member) + "-used.png"));
        }
    }
}
} // namespace blobber
