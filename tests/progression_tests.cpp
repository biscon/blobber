#include "game/commands.hpp"
#include "game/session.hpp"
#include "support/barbarian_checks.hpp"
#include "support/bard_checks.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "support/early_equipment_fixtures.hpp"
#include "support/paladin_checks.hpp"
#include "support/ranger_checks.hpp"
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
    if (c.build.level == 3) {
        r.choices.abilityImprovement.increases[2] = 2;
        const auto can = advancementChoices(s.party().catalog(), c.build.view(), c.build.portrait,
                                            c.build.spells, r.choices, SpellChoice::Cantrips);
        r.choices.cantrips = recommendedChoices(s.party().catalog(), can, SpellChoice::Cantrips);
    }
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
        for (int level = 2; level <= 4; ++level) {
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
                check((c.build.spells.cantrips & originals[i].cantrips) == originals[i].cantrips &&
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
        check(!s.advance(chosen(s, {0})), "cannot level beyond four");
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
    check(p.character({2})->experience == 2700 && p.character({2})->experienceFraction == 0,
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
void levelFourChoices() {
    const auto cat = creationCatalog(true);
    const auto map = hybridRoom();
    const auto empty = spellPlacements(map, 0);
    GameSession game(map, cat, empty, 5, spellSpawn, false);
    auto& party = game.party();
    for (unsigned level = 2; level <= 3; ++level) {
        party.grantNextLevelExperience();
        for (std::uint16_t i = 0; i < 4; ++i)
            check(bool(game.advance(chosen(game, {i}))), "prepare level-three party");
    }
    for (std::uint16_t i : {std::uint16_t(0), std::uint16_t(1), std::uint16_t(2)}) {
        const auto& before = *party.character({i});
        auto r = chosen(game, {i});
        r.choices.abilityImprovement = {};
        const unsigned ability = i == 0 ? 0 : i == 1 ? 4 : 1;
        r.choices.abilityImprovement.increases[ability] = 2;
        const auto stats = previewAdvancement(cat, before.build.view(), before.build.spells,
                                              party.inventory().loadout({i}), r.choices);
        check(stats.valid && stats.stats.saves[ability] == before.stats.saves[ability] + 1,
              "ASI updates affected saving throw");
        if (i == 0)
            check(stats.stats.carryingCapacity == before.stats.carryingCapacity + 3000,
                  "strength ASI updates carrying capacity");
        if (i == 1) {
            const auto prep = advancementChoices(cat, before.build.view(), before.build.portrait,
                                                 before.build.spells, r.choices, false);
            check(prep.maximum == 2, "cleric wisdom ASI grants two preparation slots");
        }
        if (i == 2)
            check(stats.stats.initiative == before.stats.initiative + 1 &&
                      stats.stats.armorClass == before.stats.armorClass + 1,
                  "dexterity ASI updates initiative and light armor AC");
    }
    const auto original = *party.character({3});
    auto request = chosen(game, {3});
    request.choices.abilityImprovement = {};
    check(!previewAdvancement(cat, original.build.view(), original.build.spells,
                              party.inventory().loadout({3}), request.choices)
               .valid,
          "missing mandatory ASI rejects");
    request.choices.abilityImprovement.increases[3] = 2;
    auto next = original.build.view();
    ++next.level;
    next.levelFourAsi = request.choices.abilityImprovement;
    const auto expectedSlots = preparationLimit(cat, next) - preparationLimit(cat, original.build.view());
    check(expectedSlots == 2, "wizard INT increase earns two preparation slots");
    auto prep = advancementChoices(cat, original.build.view(), original.build.portrait, original.build.spells,
                                   request.choices, false);
    check(prep.maximum == 2 && prep.minimum == 0, "two new preparations remain optional");
    request.choices.prepared = recommendedChoices(cat, prep, SpellChoice::Prepared);
    auto preview = previewAdvancement(cat, original.build.view(), original.build.spells,
                                      party.inventory().loadout({3}), request.choices);
    check(preview.valid && spellCount(preview.spells.cantrips) == 4 &&
              spellCount(preview.spells.learned) == 12 &&
              preview.stats.modifiers[3] == original.stats.modifiers[3] + 1,
          "complete wizard level-four choices use improved casting score");
    auto bad = request.choices;
    bad.abilityImprovement.increases[0] = 1;
    check(!previewAdvancement(cat, original.build.view(), original.build.spells,
                              party.inventory().loadout({3}), bad)
               .valid,
          "over-budget allocation rejects");
    bad = request.choices;
    bad.cantrips = original.build.spells.racial;
    check(!previewAdvancement(cat, original.build.view(), original.build.spells,
                              party.inventory().loadout({3}), bad)
               .valid,
          "racial cantrip cannot be learned again as class cantrip");
    auto boundary = original.build.view();
    boundary.level = 4;
    boundary.baseAbilities[0] = 19 - cat.race(boundary.race)->abilityIncreases[0];
    boundary.levelFourAsi.increases[0] = 1;
    boundary.levelFourAsi.increases[1] = 1;
    check(effectiveAbilities(cat, boundary).value[0] == 20, "split allocation can reach score twenty");
    boundary.levelFourAsi.increases[0] = 2;
    boundary.levelFourAsi.increases[1] = 0;
    check(!effectiveAbilities(cat, boundary), "allocation above twenty rejects");
    boundary.level = 3;
    check(!effectiveAbilities(cat, boundary), "premature saved allocation rejects");
    party.awardExperience(4 * (2699 - original.experience));
    check(party.character({3})->experience == 2699 &&
              game.advancementEligibility(game.advancementRequest({3})) == AdvancementError::Experience,
          "2699 XP is below level-four threshold");
    party.awardExperience(4);
    check(party.character({3})->experience == 2700, "exact level-four XP threshold");
    request = chosen(game, {3});
    request.choices.abilityImprovement = {};
    request.choices.abilityImprovement.increases[3] = 2;
    prep = advancementChoices(cat, original.build.view(), original.build.portrait, original.build.spells,
                              request.choices, false);
    request.choices.prepared = recommendedChoices(cat, prep, SpellChoice::Prepared);
    CommandController ui(game);
    ui.openAdvancement({3});
    auto click = [&](FlowAction action, int value = -1) {
        FlowControls controls;
        layoutAdvancement(controls, ui.snapshot().advancement);
        check(!controls.overflow, "level-four controls fit capacity");
        for (unsigned n = 0; n < controls.count; ++n) {
            const auto& c = controls.entries[n];
            if (c.action != action || (value >= 0 && c.value != value))
                continue;
            check(c.enabled, "level-four requested control enabled");
            UiInput in;
            in.pressed[std::size_t(UiKey::Click)] = true;
            in.pointer = ArtPoint{float(c.rect.x + 2), float(c.rect.y + 2)};
            ui.routeAdvancement(in);
            return;
        }
        check(false, "level-four requested control exists");
    };
    click(FlowAction::Next);
    click(FlowAction::Next);
    check(ui.snapshot().advancement.step == AdvancementStep::Abilities, "abilities precede caster choices");
    click(FlowAction::Plus, 3);
    click(FlowAction::Plus, 3);
    click(FlowAction::Next);
    check(ui.snapshot().advancement.step == AdvancementStep::Cantrips, "additional cantrip gets own step");
    click(FlowAction::Recommend);
    click(FlowAction::Next);
    click(FlowAction::Recommend);
    click(FlowAction::Next);
    check(ui.snapshot().advancement.selection.context.maximum == 2, "routed preparation sees improved INT");
    click(FlowAction::Recommend);
    check(spellCount(ui.snapshot().advancement.selection.selected) == 2,
          "two preparations selected through UI");
    click(FlowAction::Back);
    click(FlowAction::Back);
    click(FlowAction::Back);
    click(FlowAction::Minus, 3);
    check(!ui.snapshot().advancement.request.choices.prepared,
          "reduced casting capacity clears new preparations");
    click(FlowAction::Plus, 2); // Split INT/CON instead of +2 INT.
    click(FlowAction::Cancel);
    check(party.character({3})->build.level == 3 && !party.character({3})->build.levelFourAsi.points(),
          "cancelled ASI never mutates live identity");
    party.damage({3}, 2);
    party.spendMana({3}, 2);
    check(party.useFeature({3}, Feature::ArcaneRecovery), "spend recovery charge before advancing");
    party.spendMana({3}, 2);
    const auto manaDeficit = party.character({3})->maxMana - party.character({3})->mana;
    EffectRequest aid;
    aid.world = game.actors().snapshot().world;
    aid.caster = CharacterId{1};
    aid.kind = EffectKind::Aid;
    aid.count = 1;
    aid.targets[0] = CharacterId{3};
    check(bool(game.applyEffect(aid)), "active Aid fixture");
    const auto oldHp = party.character({3})->hitPoints;
    const auto oldMax = party.character({3})->stats.maxHitPoints;
    request = chosen(game, {3}); // CON +2, including retroactive HP.
    const auto complete = game.previewAdvancement(request);
    check(bool(complete) &&
              complete.preview.stats.maxHitPoints == oldMax + 4 + original.stats.modifiers[2] + 4,
          "CON improvement adds retroactive HP while retaining Aid");
    check(bool(game.advance(request)), "complete fourth-level transaction commits");
    const auto& result = *party.character({3});
    check(result.hitPoints == oldHp + result.stats.maxHitPoints - oldMax && result.maxMana == 17 &&
              result.maxMana - result.mana == manaDeficit && !result.arcaneRecovery,
          "advance preserves HP/mana deficits, Aid and spent charges");
    check(result.build.baseAbilities == original.build.baseAbilities &&
              result.build.levelFourAsi.increases[2] == 2,
          "earned ASI is separate from creation scores");
    const auto saved = party.checkpoint();
    Party restored(cat, saved);
    check(restored.character({3})->stats.abilities == result.stats.abilities,
          "party restore derives earned ASI");
    auto corrupt = saved;
    corrupt.state.characters[3].build.levelFourAsi.increases[2] = 255;
    bool rejected = false;
    try {
        Party invalid(cat, corrupt);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    check(rejected, "malformed saved ASI rejected before publication");
    AdvancementHud draft;
    draft.catalog = &cat;
    draft.identity = original.build.view();
    draft.request.choices = request.choices;
    draft.step = AdvancementStep::Abilities;
    check(!draft.canIncrease(0), "fully allocated ASI disables additional points");
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
    game.party().grantNextLevelExperience();
    check(bool(game.advance(chosen(game, {0}))), "Advance to level four cap");
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
        blobber::test::paladinChecks();
        blobber::test::rangerProgression();
        blobber::test::rangerGoodberry();
        blobber::test::bardProgressionAndRecovery();
        blobber::test::barbarianChecks();
        blobber::test::bardAttackDecisions();
        blobber::test::bardSavesAndMockery();
        blobber::test::bardDeathAndRayDecisions();
        blobber::test::competingLoreBards();
        blobber::test::rangerMarkAndDamage();
        identities();
        levelFourChoices();
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
