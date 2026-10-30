#include "game/commands.hpp"
#include "game/session.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::rules;
namespace {
AdvancementRequest chosen(GameSession& s, CharacterId id) {
    auto r = s.advancementRequest(id);
    const auto& c = *s.party().character(id);
    const auto context =
        advancementChoices(s.party().catalog(), c.build.view(), c.build.portrait, c.build.spells, {}, true);
    r.choices.learned = recommendedChoices(s.party().catalog(), context, SpellChoice::Learned);
    return r;
}
void identities() {
    const auto cat = creationCatalog(true);
    const auto map = hybridRoom();
    const NpcWorld empty(map, spellSpawn.cell, {"goblin"}, {});
    for (std::uint16_t race = 0; race < 4; ++race) {
        auto start = *cat.preset();
        for (std::uint16_t cls = 0; cls < 4; ++cls) {
            auto draft = recommendedDraft(cat, {race}, {cls});
            std::snprintf(draft.name.data(), draft.name.size(), "Custom %u", cls);
            start.characters[cls] = finishBuild(cat, draft, "advance." + std::to_string(cls));
        }
        GameSession s(map, cat, start, empty, 5, spellSpawn, false);
        auto& p = s.party();
        std::array<SpellGrants, 4> originals{};
        std::array<int, 4> oldHp{};
        for (std::uint16_t i = 0; i < 4; ++i) {
            originals[i] = p.character({i})->build.spells;
            oldHp[i] = p.character({i})->stats.maxHitPoints;
            p.damage({i}, 2);
            if (p.character({i})->mana)
                p.spendMana({i}, 2);
        }
        const auto invRevision = p.inventory().revision();
        check(!s.grantNextLevelExperience(), "development grant disabled by default");
        s.enableDevelopment(true);
        for (int level = 2; level <= 3; ++level) {
            check(s.grantNextLevelExperience(), "development grant earns threshold");
            check(!s.grantNextLevelExperience(), "repeated grant before advancement is no-op");
            for (std::uint16_t i = 0; i < 4; ++i) {
                const auto before = s.restToken();
                auto r = chosen(s, {i});
                auto bad = r;
                bad.choices.prepared = UINT32_MAX;
                check(!s.advance(bad), "forged choices reject");
                check(p.character({i})->build.level == level - 1 && s.restToken() == before,
                      "failed commit is atomic");
                const auto result = s.advance(r);
                check(bool(result), advancementMessage(result.error));
                check(!s.advance(r), "repeat request cannot level twice");
                const auto& c = *p.character({i});
                check(c.build.id == start.characters[i].id && c.build.name == start.characters[i].name &&
                          c.build.portrait.index == start.characters[i].portrait.index &&
                          c.build.baseAbilities == start.characters[i].baseAbilities,
                      "all race/class identities survive advancement");
                check(c.hitPoints == c.stats.maxHitPoints - 2 && c.availableHitDice == level,
                      "HP deficit preserved and new hit die earned");
                check(c.build.spells.cantrips == originals[i].cantrips &&
                          c.build.spells.racial == originals[i].racial &&
                          c.build.spells.prepared == originals[i].prepared,
                      "existing choices and sources remain");
                if (i == 3)
                    check(spellCount(c.build.spells.learned) == unsigned(4 + 2 * level),
                          "two new wizard spells");
                if (c.maxMana)
                    check(c.mana == c.maxMana - 2, "mana deficit preserved");
                check(p.inventory().revision() == invRevision && s.restToken() > before,
                      "inventory untouched and old rest invalidated");
                check(c.stats.maxHitPoints > oldHp[i], "maximum HP rises");
                oldHp[i] = c.stats.maxHitPoints;
            }
        }
        check(!s.grantNextLevelExperience(), "supported level cap");
        check(!s.advance(chosen(s, {0})), "cannot level beyond three");
        check(cat.preset()->characters[0].level == 1 && start.characters[0].level == 1,
              "launch and catalog never mutate");
    }
}
void rewards() {
    const auto cat = creationCatalog(true);
    Party p(cat);
    p.awardExperience(50);
    check(p.character({0})->experience == 12 && p.character({0})->experienceFraction == 6,
          "quarter shares retain half XP");
    p.damage({0}, 10000);
    p.damage({1}, p.character({1})->hitPoints);
    for (int i = 0; i < 3; ++i)
        p.awardExperience(1);
    check(p.character({0})->experience == 12 && p.character({1})->experience == 13 &&
              p.character({1})->experienceFraction == 6,
          "dead excluded, unconscious included, exact thirds");
    p.awardExperience(UINT32_MAX);
    check(p.character({2})->experience == 900 && p.character({2})->experienceFraction == 0,
          "large XP safely saturates");
    const auto map = hybridRoom();
    const auto npcs = spellPlacements(map, 1);
    GameSession s(map, cat, npcs, 5, spellSpawn, false);
    const auto id = s.actors().id(0);
    s.actors().damage(id, 10000, 0);
    s.actors().remove(id);
    s.update(0, true);
    for (int n = 0; n < 8; ++n)
        s.update(0, true);
    check(s.party().character({0})->experience == 12 && s.party().character({0})->experienceFraction == 6,
          "removed corpse rewards once even while paused");
}
void advancementUi() {
    const auto catalog = creationCatalog(true);
    const auto map = hybridRoom();
    const auto empty = spellPlacements(map, 0);
    GameSession s(map, catalog, empty, 5, spellSpawn, false);
    s.enableDevelopment(true);
    s.grantNextLevelExperience();
    CommandController ui(s);
    ui.refresh({}, false, false, true);
    check(ui.snapshot().cards[3].levelEligible, "HUD exposes safe advancement eligibility");
    auto click = [&](FlowAction action, int value = -1) {
        FlowControls controls;
        layoutAdvancement(controls, ui.snapshot().advancement);
        check(!controls.overflow, "advancement controls fit fixed storage");
        for (unsigned i = 0; i < controls.count; ++i)
            if (controls.entries[i].action == action && (value < 0 || controls.entries[i].value == value)) {
                check(controls.entries[i].enabled, "requested advancement control is enabled");
                UiInput input;
                input.focused = true;
                input.pressed[std::size_t(UiKey::Click)] = true;
                const auto r = controls.entries[i].rect;
                input.pointer = ArtPoint{float(r.x + r.width / 2), float(r.y + r.height / 2)};
                ui.routeAdvancement(input);
                return;
            }
        check(false, "advancement control exists");
    };
    const auto baseline = s.party().character({3})->build.spells;
    ui.openAdvancement({3});
    click(FlowAction::Next);
    click(FlowAction::Next);
    check(ui.snapshot().advancement.step == AdvancementStep::Learned, "wizard enters shared learned picker");
    check(ui.snapshot().advancement.selection.context.available &
              spellBit(ui.snapshot().advancement.selection.visible[0]),
          "only newly available choices are listed");
    check(ui.snapshot().advancement.selection.count == 2, "two new Wizard spells; known cards hidden");
    click(FlowAction::Recommend);
    click(FlowAction::Cancel);
    check(s.party().character({3})->build.spells.learned == baseline.learned &&
              s.party().character({3})->build.level == 1,
          "cancel discards draft choices");
    ui.openAdvancement({3});
    click(FlowAction::Next);
    click(FlowAction::Next);
    click(FlowAction::Recommend);
    click(FlowAction::Next);
    check(ui.snapshot().advancement.step == AdvancementStep::Prepared,
          "new preparation is a separate choice");
    click(FlowAction::Recommend);
    const auto recommended = ui.snapshot().advancement.selection.selected;
    check(recommended && (recommended & spellBit(ui.snapshot().advancement.selection.inspected)),
          "recommendation selects a spell and previews that selection");
    click(FlowAction::Recommend);
    check(ui.snapshot().advancement.selection.selected == recommended, "recommending twice is idempotent");
    SpellId first;
    const auto& picker = ui.snapshot().advancement.selection;
    for (unsigned i = 0; i < picker.count; ++i)
        if (!(recommended & spellBit(picker.visible[i]))) {
            first = picker.visible[i];
            break;
        }
    check(bool(spellBit(first)), "fixture has an alternative to its recommended spell");
    click(FlowAction::Toggle, first.index);
    check(ui.snapshot().advancement.selection.selected == spellBit(first) &&
              ui.snapshot().advancement.selection.inspected.index == first.index,
          "clicking another spell replaces the sole pick and updates its preview");
    click(FlowAction::Toggle, first.index);
    check(!ui.snapshot().advancement.selection.selected, "clicking selected spell clears optional slot");
    click(FlowAction::Toggle, first.index);
    check(ui.snapshot().advancement.selection.selected == spellBit(first), "cleared spell can be reselected");
    click(FlowAction::Toggle, first.index);

    click(FlowAction::Next);
    check(bool(ui.snapshot().advancement.preview), "complete isolated draft previews");
    const auto request = ui.snapshot().advancement.request;
    click(FlowAction::Done);
    check(!ui.snapshot().advancement.open && s.party().character({3})->build.level == 2,
          "confirmation commits once and returns to sheet");
    check(!s.advance(request), "old UI request cannot be replayed");
    ui.openAdvancement({1});
    click(FlowAction::Next);
    click(FlowAction::Next);
    const auto& cleric = ui.snapshot().advancement.selection;
    check(cleric.count == 1 && catalog.spell(cleric.visible[0])->kind == SpellKind::Sanctuary,
          "Cleric sees only the one additional preparation choice");
    check(!cleric.selected && cleric.valid(), "leaving the earned slot empty remains legal");
    click(FlowAction::Next);
    check(ui.snapshot().advancement.preview.preview.spells.prepared ==
              s.party().character({1})->build.spells.prepared,
          "skipping preserves every existing preparation");
    const auto stale = ui.snapshot().advancement.request;
    check(bool(s.advance(s.advancementRequest({0}))), "another advancement invalidates the draft");
    check(s.previewAdvancement(stale).error == AdvancementError::Stale,
          "stale draft is rejected without changes");
}

} // namespace
int main() {
    try {
        identities();
        rewards();
        advancementUi();
        std::cout << "Progression checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
