#include "game/commands.hpp"
#include "game/session.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/early_equipment_fixtures.hpp"
#include <cstring>
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
void equippedAdvancement() {
    const auto cat = blobber::test::withMagicItems(creationCatalog(true));
    const auto map = hybridRoom();
    const NpcWorld none(map, spellSpawn.cell, {"goblin"}, {});
    GameSession s(map, cat, none, 5, spellSpawn, false);
    auto& party = s.party();
    for (const auto* name : {"belt_of_vigor", "amulet_of_reserves", "circlet_of_focus"}) {
        const auto id = cat.findItem(name);
        check(bool(party.acquire({{3}, id, 1, party.inventory().revision()})),
              "Acquire advancement magic fixture");
        for (std::uint16_t cell = 0; cell < bagCells; ++cell) {
            const auto h = party.inventory().at({{3}, cell});
            const auto* stack = party.inventory().stack(h);
            if (stack && stack->item.index == id.index) {
                check(bool(party.inventoryChange({InventoryOp::Move,
                                                  h,
                                                  {{3}, std::uint16_t(itemEquipmentPosition(*cat.item(id)))},
                                                  0,
                                                  party.inventory().revision()})),
                      "Equip advancement fixture");
                break;
            }
        }
    }
    const auto old = *party.character({3});
    party.grantNextLevelExperience();
    auto request = chosen(s, {3});
    const auto result = party.advance(request);
    check(bool(result), "Wizard advances with capacity items equipped");
    const auto& now = *party.character({3});
    check(now.maxMana == maximumMana(Casting::Intelligence, 2) + 1 && now.mana == old.mana + 2 &&
              now.stats.maxHitPoints == old.stats.maxHitPoints + 5 && now.hitPoints == old.hitPoints + 5,
          "Advancement retains item capacity and grants only ordinary level resource increases");
    const auto stats = now.stats;
    auto without = stats;
    without.spellAttackBonus = 0;
    bool tested = false;
    for (std::uint16_t i = 0; i < cat.spellCount(); ++i) {
        const auto before = spellAccess(cat, now.build, without, {i});
        const auto after = party.spellAccess({3}, {i});
        if (before) {
            check(after.attack == before.attack + 1 && after.difficulty == before.difficulty &&
                      after.modifier == before.modifier,
                  "Circlet improves spell attack access without changing DC or ability modifier");
            tested = true;
        }
    }
    check(tested, "Owned caster has usable spells");
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
        s.audioEvents().enabled = true;
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
            check(s.audioEvents().count == 1 &&
                      std::strcmp(s.audioEvents().entries[0].cue, "advancement") == 0,
                  "Shared development threshold grant emits exactly one bell");
            check(!s.grantNextLevelExperience(), "repeated grant before advancement is no-op");
            check(s.audioEvents().count == 1, "Repeated grant cannot replay the earned-level bell");
            for (std::uint16_t i = 0; i < 4; ++i) {
                s.audioEvents().clear();
                const auto before = s.restToken();
                auto r = chosen(s, {i});
                auto bad = r;
                bad.choices.prepared = UINT32_MAX;
                check(!s.advance(bad), "forged choices reject");
                check(p.character({i})->build.level == level - 1 && s.restToken() == before,
                      "failed commit is atomic");
                check(bool(s.previewAdvancement(r)), "Advancement can be previewed without a bell");
                check(s.audioEvents().count == 0, "Preview and rejected advancement are silent");
                const auto result = s.advance(r);
                check(bool(result), advancementMessage(result.error));
                check(s.audioEvents().count == 0,
                      "Completing advancement is silent because the bell played when XP was earned");
                check(!s.advance(r), "repeat request cannot level twice");
                check(s.audioEvents().count == 0, "Rejected repeated request cannot replay the bell");
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
        check(s.audioEvents().count == 0, "Level-cap grants stay silent");
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
    ui.refresh({}, false, false, true);
    check(!ui.snapshot().cards[3].levelEarned, "Completing advancement clears the earned-level reminder");
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
void earnedLevelReminders() {
    const auto catalog = creationCatalog(true);
    const auto map = hybridRoom();
    const auto empty = spellPlacements(map, 0);
    GameSession game(map, catalog, empty, 5, spellSpawn, false);
    CommandController ui(game);
    ui.refresh({}, false, false, true);
    check(!ui.snapshot().cards[0].levelEarned, "Insufficient XP shows no level reminder");
    game.party().grantNextLevelExperience();
    for (const auto action : {Action::Forward, Action::TurnLeft, Action::Back, Action::TurnRight}) {
        game.move(action);
        check(game.movement().transition.active(), "Reminder regression exercises animated movement");
        for (unsigned frame = 0; frame < 4; ++frame) {
            ui.refresh({}, false, false, true);
            for (const auto& card : ui.snapshot().cards)
                check(card.levelEarned && !card.levelEligible,
                      "Earned reminder stays visible throughout movement while immediate action is disabled");
            check(game.advance(chosen(game, {0})).error == AdvancementError::Moving,
                  "Movement reminder cannot bypass actual advancement admission");
            game.update(.03, false);
        }
        game.update(.1, false);
        ui.refresh({}, false, false, true);
        check(ui.snapshot().cards[0].levelEarned && ui.snapshot().cards[0].levelEligible,
              "Stopping preserves the reminder and restores safe advancement");
    }
    game.party().damage({0}, game.party().character({0})->hitPoints);
    ui.refresh({}, false, false, true);
    check(ui.snapshot().cards[0].levelEarned && !ui.snapshot().cards[0].levelEligible,
          "Incapacitation retains earned XP reminder without permitting advancement");
    game.party().heal({0}, 1);
    check(bool(game.advance(chosen(game, {0}))), "Advance once after movement settles");
    game.party().grantNextLevelExperience();
    check(bool(game.advance(chosen(game, {0}))), "Advance to supported level cap");
    ui.refresh({}, false, false, true);
    check(!ui.snapshot().cards[0].levelEarned && !ui.snapshot().cards[0].levelEligible,
          "Level cap clears the reminder even with maximum XP");

    const auto nearby = spellPlacements(map, 1, true);
    auto danger = std::make_unique<GameSession>(map, catalog, nearby, 5, spellSpawn, false);
    danger->party().grantNextLevelExperience();
    danger->update(0, false);
    CommandController fighting(*danger);
    fighting.refresh({}, false, false, true);
    check(danger->combat().snapshot().phase != CombatPhase::Exploration &&
              fighting.snapshot().cards[0].levelEarned && !fighting.snapshot().cards[0].levelEligible &&
              danger->advance(chosen(*danger, {0})).error == AdvancementError::Unsafe,
          "Combat keeps the earned reminder but rejects actual advancement");

    const auto unsupported = creationCatalog(false);
    auto noProgression = std::make_unique<GameSession>(map, unsupported, empty, 5, spellSpawn, false);
    noProgression->party().awardExperience(10000);
    CommandController staticParty(*noProgression);
    staticParty.refresh({}, false, false, true);
    check(!staticParty.snapshot().cards[0].levelEarned,
          "Classes without supported progression do not show a misleading reminder");
}

void earnedLevelAudio() {
    const auto cat = creationCatalog(true);
    const auto map = hybridRoom();
    const auto npcs = spellPlacements(map, 3);
    auto s = std::make_unique<GameSession>(map, cat, npcs, 5, spellSpawn, false);
    s->audioEvents().enabled = true;
    const auto bells = [](GameSession& game) {
        const auto& journal = game.audioEvents();
        unsigned count = 0;
        for (unsigned n = 0; n < journal.count; ++n)
            count += std::strcmp(journal.entries[n].cue, "advancement") == 0;
        return count;
    };
    s->party().awardExperience(1050); // 262.5 each; three 50-XP kills reach exactly 300.
    s->actors().damage(s->actors().id(0), 10000, 0);
    s->update(0, true);
    check(bells(*s) == 0, "XP below next threshold stays silent");
    s->actors().damage(s->actors().id(1), 10000, 0);
    s->actors().damage(s->actors().id(2), 10000, 0);
    s->update(0, true);
    check(bells(*s) == 1, "Multiple deaths and four newly eligible members emit one immediate bell");
    for (std::uint16_t id = 0; id < 4; ++id)
        check(s->party().character({id})->experience == 300, "Reward batch crosses exact XP threshold");
    s->audioEvents().clear();
    for (unsigned n = 0; n < 4; ++n)
        s->update(0, true);
    check(bells(*s) == 0, "Already credited deaths and eligible party never replay the bell");
    check(bool(s->advance(chosen(*s, {0}))) && bells(*s) == 0, "Completing an XP-earned level is silent");
    auto savedFloor = std::make_unique<FloorState>();
    s->exportFloor(*savedFloor);
    auto restoredParty = std::make_unique<Party>(cat, s->party().checkpoint());
    auto restored = std::make_unique<GameSession>(map, std::move(restoredParty), npcs, 5, spellSpawn, false,
                                                  std::vector<NpcTimings>{}, RecoveryLevel{}, FloorFixtures{},
                                                  FloorObjectiveDefinition{});
    restored->importFloor(*savedFloor);
    restored->audioEvents().enabled = true;
    restored->update(0, true);
    check(bells(*restored) == 0, "Loading an already-eligible party and credited deaths stays silent");

    auto danger =
        std::make_unique<GameSession>(map, cat, spellPlacements(map, 2, true), 5, spellSpawn, false);
    danger->audioEvents().enabled = true;
    danger->party().awardExperience(1160); // 290 each before one member dies.
    danger->party().damage({0}, 10000);
    danger->party().damage({1}, danger->party().character({1})->hitPoints);
    danger->update(0, false);
    danger->audioEvents().clear();
    danger->actors().damage(danger->actors().id(0), 10000, 0);
    danger->update(0, true);
    check(bells(*danger) == 1 && danger->party().character({0})->experience == 290 &&
              danger->party().character({1})->experience > 300 &&
              danger->advancementEligibility(chosen(*danger, {2})) != AdvancementError::None,
          "Unsafe combat rewards unconscious recipients, excludes dead members and rings immediately");
    danger->audioEvents().clear();
    danger->actors().damage(danger->actors().id(1), 10000, 0);
    danger->update(0, true);
    check(bells(*danger) == 0, "More XP while already eligible stays silent");

    const auto unsupported = creationCatalog(false);
    const NpcWorld empty(map, spellSpawn.cell, {"goblin"}, {});
    auto unsupportedGame = std::make_unique<GameSession>(map, unsupported, empty, 5, spellSpawn, false);
    unsupportedGame->audioEvents().enabled = true;
    unsupportedGame->enableDevelopment(true);
    unsupportedGame->grantNextLevelExperience();
    check(bells(*unsupportedGame) == 0, "Unsupported classes cannot produce earned-level audio");
}

} // namespace
int main() {
    try {
        identities();
        equippedAdvancement();
        rewards();
        advancementUi();
        earnedLevelReminders();
        earnedLevelAudio();
        std::cout << "Progression checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
