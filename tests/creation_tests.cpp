#include "game/commands.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include <iostream>
using namespace blobber;
int main() {
    try {
        using namespace rules;
        const auto cat = creationCatalog();
        constexpr int costs[]{0, 1, 2, 3, 4, 5, 7, 9};
        for (int n = 8; n <= 15; ++n) {
            check(pointCost(n) == costs[n - 8], "point cost");
            if (n < 15)
                check(pointCost(n + 1) - pointCost(n) == (n < 13 ? 1 : 2), "point transition");
        }
        check(pointCost(7) < 0 && pointCost(16) < 0, "point bounds");
        for (std::uint16_t r = 0; r < 4; ++r)
            for (std::uint16_t c = 0; c < 4; ++c) {
                auto d = recommendedDraft(cat, {r}, {c});
                std::snprintf(d.name.data(), d.name.size(), "Same name");
                check(bool(previewBuild(cat, d)), "all race/class recommendations");
                const auto* cfg = cat.creation({c});
                for (unsigned bits = 0; bits < 8; ++bits) {
                    bool legal = true;
                    for (unsigned g = 0; g < 3; ++g) {
                        d.gear[g] = (bits >> g) & 1;
                        legal &= g < cfg->groupCount ? d.gear[g] < cfg->groups[g].count : d.gear[g] == 0;
                    }
                    check(bool(previewBuild(cat, d)) == legal, "all gear choices");
                }
                d.gear = {};
                auto b = finishBuild(cat, d, "custom");
                CharacterDraft restored;
                check(draftFromBuild(cat, b, restored), "recover draft");
                auto p = *cat.preset();
                p.characters[0] = b;
                Party live(cat, p);
                check(live.character({0})->build.portrait.index == d.portrait.index, "party copies portrait");
                if (r == 3) {
                    const auto racial = SpellId{std::uint16_t(__builtin_ctz(d.spells.racial))};
                    auto access = live.spellAccess({0}, racial);
                    check(access && access.casting == Casting::Intelligence, "racial INT");
                    check(access.modifier == live.character({0})->stats.modifiers[3], "racial modifier");
                    if (c == 0 || c == 2)
                        check(live.character({0})->maxMana == 0, "racial grants no mana");
                }
                d.portrait = {std::uint8_t((r + 1) % 4)};
                check(previewBuild(cat, d).has(BuildError::Portrait), "race portrait mismatch");
            }
        auto d = recommendedDraft(cat, {3}, {3});
        std::snprintf(d.name.data(), d.name.size(), "Wizard");
        d.spells.racial = d.spells.cantrips & (~d.spells.cantrips + 1);
        check(previewBuild(cat, d).has(BuildError::Racial), "duplicate grants reject");
        d = recommendedDraft(cat, {0}, {3});
        std::snprintf(d.name.data(), d.name.size(), "Wizard");
        d.spells.prepared = 0;
        check(previewBuild(cat, d).has(BuildError::Prepared), "minimum preparation");
        d.spells.prepared = d.spells.learned & (~d.spells.learned + 1);
        check(bool(previewBuild(cat, d)), "fewer prepared is legal");
        d.abilities[0] = 9;
        check(previewBuild(cat, d).has(BuildError::Budget), "overspent budget");
        d.abilities[0] = 7;
        check(previewBuild(cat, d).has(BuildError::Abilities), "base bound");
        const auto p = *cat.preset();
        Party a(cat, p), b(cat, p);
        check(a.inventory().identity() != b.inventory().identity(), "independent inventory handles");
        a.damage({0}, 1);
        check(a.character({0})->hitPoints + 1 == b.character({0})->hitPoints, "independent health");
        unsigned kits = 0;
        for (unsigned i = 0; i < 4; ++i)
            for (unsigned j = 0; j < bagCells; ++j)
                if (const auto* stack =
                        a.inventory().stack(a.inventory().at({{std::uint16_t(i)}, std::uint16_t(j)})))
                    if (cat.item(stack->item)->id == "camp_kit")
                        kits += stack->quantity;
        check(kits == 3, "three launch kits");
        const auto wizard = CharacterId{3};
        const auto absent = cat.findSpell("false_life");
        check(!a.spellAccess(wizard, absent), "unlearned spell rejected");
        const auto learned = cat.findSpell("sleep");
        check(!a.spellAccess(wizard, learned), "learned unprepared rejected");
        // Public command boundary must reject forged access without spending resources.
        const auto map = hybridRoom();
        const auto placements = spellPlacements(map, 1);
        GameSession session(map, cat, p, placements, 19, spellSpawn, false);
        for (auto kind : {SpellKind::FalseLife, SpellKind::Sleep}) {
            auto command = spellCommand(session, kind);
            command.targetCount = 1;
            command.enemies[0] = session.actors().id(0);
            const auto before = session.party().character({3})->mana;
            check(session.submit(command).reason == CommandReason::InvalidSpell,
                  "forged unlearned/unprepared direct command rejected");
            check(session.party().character({3})->mana == before, "rejected cast spends nothing");
        }
        for (std::uint16_t cls : {0, 1, 2}) {
            auto custom = *cat.preset();
            auto elf = recommendedDraft(cat, {3}, {cls});
            std::snprintf(elf.name.data(), elf.name.size(), "Elf source check");
            custom.characters[cls] = finishBuild(cat, elf, "elf.source");
            GameSession mixed(map, cat, custom, placements, 19, spellSpawn, false);
            CommandController ui(mixed);
            ui.refresh({}, false, false, true);
            ui.selectCharacter(cls);
            ui.openSpellbook();
            bool racial = false, feature = false, wisdom = false;
            for (std::size_t n = 0; n < ui.snapshot().magic.count; ++n) {
                const auto& entry = ui.snapshot().magic.entries[n];
                if (entry.source == SpellSource::Racial) {
                    racial = true;
                    check(entry.casting == Casting::Intelligence &&
                              entry.attack == 2 + mixed.party().character({cls})->stats.modifiers[3],
                          "racial UI uses INT");
                }
                wisdom |= entry.source == SpellSource::Class && entry.casting == Casting::Wisdom;
                feature |= entry.category != PickerEntryKind::Spell;
            }
            check(racial && (cls == 1 ? wisdom : feature),
                  "racial magic coexists with class spells/features");
            auto command = spellCommand(mixed, SpellKind::FireBolt);
            command.character = {cls};
            command.targetCount = 1;
            command.enemies[0] = mixed.actors().id(0);
            const auto mana = mixed.party().character({cls})->mana;
            check(mixed.submit(command).reason == CommandReason::Resolved,
                  "martial/cleric racial cast legal");
            check(mixed.party().character({cls})->mana == mana, "racial cantrip spends no mana");
        }
        auto invalid = recommendedDraft(cat, {0}, {3});
        std::snprintf(invalid.name.data(), invalid.name.size(), "Bounds");
        invalid.spells.learned |= spellBit(cat.findSpell("false_life"));
        check(previewBuild(cat, invalid).has(BuildError::Learned), "seven learned rejected");
        invalid = recommendedDraft(cat, {0}, {1});
        std::snprintf(invalid.name.data(), invalid.name.size(), "Bounds");
        invalid.spells.domain = 0;
        check(previewBuild(cat, invalid).has(BuildError::Domain), "required domain grants");
        invalid = recommendedDraft(cat, {0}, {0});
        std::snprintf(invalid.name.data(), invalid.name.size(), "Bounds");
        invalid.gear[0] = 255;
        check(previewBuild(cat, invalid).has(BuildError::Gear), "invalid gear rejected");
        invalid.gear = {};
        invalid.portrait = {255};
        check(previewBuild(cat, invalid).has(BuildError::Portrait), "unknown portrait rejected");
        invalid = recommendedDraft(cat, {0}, {0});
        std::snprintf(invalid.name.data(), invalid.name.size(), "Budget");
        invalid.abilities.fill(8);
        check(previewBuild(cat, invalid).has(BuildError::Budget), "unspent budget rejected");
        invalid.abilities.fill(15);
        check(previewBuild(cat, invalid).has(BuildError::Budget), "maximum overspend rejected");
        // A changed content weight must reach the same derivation rejection as live inventory.
        std::vector<RaceDefinition> races;
        std::vector<ClassDefinition> classes;
        std::vector<ItemDefinition> items;
        std::vector<SpellDefinition> spells;
        std::vector<CreationClass> options;
        for (std::uint16_t i = 0; i < 4; ++i) {
            races.push_back(*cat.race({i}));
            classes.push_back(*cat.characterClass({i}));
            options.push_back(*cat.creation({i}));
        }
        for (std::uint16_t i = 0; i < cat.itemCount(); ++i)
            items.push_back(*cat.item({i}));
        for (std::uint16_t i = 0; i < cat.spellCount(); ++i)
            spells.push_back(*cat.spell({i}));
        items[0].weight = 100000;
        const Catalog heavy({}, {}, races, classes, items, std::nullopt, spells, options);
        invalid = recommendedDraft(heavy, {0}, {0});
        std::snprintf(invalid.name.data(), invalid.name.size(), "Heavy");
        check(previewBuild(heavy, invalid).derivation == CharacterError::Overweight,
              "preview rejects overweight content");
        invalid = recommendedDraft(cat, {0}, {3});
        std::snprintf(invalid.name.data(), invalid.name.size(), "Subset");
        invalid.spells.prepared |= spellBit(cat.findSpell("false_life"));
        check(previewBuild(cat, invalid).has(BuildError::Prepared), "unlearned preparation rejected");
        invalid.spells.prepared = invalid.spells.learned;
        check(previewBuild(cat, invalid).has(BuildError::Prepared), "preparation allowance enforced");
        invalid = recommendedDraft(cat, {0}, {0});
        std::snprintf(invalid.name.data(), invalid.name.size(), "Feature");
        invalid.defense = false;
        check(previewBuild(cat, invalid).has(BuildError::Features), "supported Defense choice required");
        invalid.defense = true;
        bool invalidId = false;
        try {
            finishBuild(cat, invalid, "");
        } catch (const std::invalid_argument&) {
            invalidId = true;
        }
        check(invalidId, "empty owning build identity rejected");
        auto duplicate = p;
        duplicate.formation[0] = duplicate.formation[1];
        bool rejected = false;
        try {
            Party bad(cat, duplicate);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        check(rejected, "duplicate formation rejected");
        std::cout << "Creation rules and independent party checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
