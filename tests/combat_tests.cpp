#include "game/commands.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "ui/combat_feedback.hpp"
#include "ui/interaction.hpp"
#include <iostream>
#include <limits>
#include <vector>
using namespace blobber;
namespace {
constexpr Player spawn{{3, 3}, Heading::North};
void start(GameSession& s) {
    check(s.update(0, false).event == CombatEvent::Started, "Contact starts once");
}
void awaitCharacter(GameSession& s, rules::CharacterId id) {
    for (int i = 0; i < 200; ++i) {
        const auto& state = s.combat().snapshot();
        if (state.awaitingAction && state.activeCharacter->index == id.index)
            return;
        if (state.awaitingAction)
            s.submit(turnCommand(s));
        else
            s.update(.45, false);
    }
    check(false, "Expected character turn");
}
void rows() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    for (auto heading : {Heading::North, Heading::East, Heading::South, Heading::West}) {
        Player pose{{3, 3}, heading};
        const auto placements = combatPlacements(map, pose);
        GameSession s(map, catalog, placements, 5, pose);
        check(s.combat().snapshot().phase == CombatPhase::Contact, "Cardinal contact");
        check(!s.party().swapFormation(FormationSlot::FrontLeft, FormationSlot::BackLeft),
              "Contact locks formation");
        start(s);
        std::vector<ActorId> front, rear;
        for (std::size_t i = 0; i < 4; ++i) {
            const auto id = s.actors().id(i);
            const auto sub = placements.instances()[i].subcell;
            const bool expected = heading == Heading::North   ? sub.y == 1
                                  : heading == Heading::East  ? sub.x == 0
                                  : heading == Heading::South ? sub.y == 0
                                                              : sub.x == 1;
            check(s.combat().exposed(id) == expected, "Rows measured toward party, not actor facing");
            (expected ? front : rear).push_back(id);
        }
        awaitCharacter(s, {0});
        auto c = turnCommand(s, PartyAction::Melee);
        c.target = rear[0];
        check(s.submit(c).reason == CommandReason::FrontProtected, "Cannot melee opposing rear");
        s.actors().damage(front[0], 1000, s.timeMs());
        check(s.combat().exposed(rear[0]) && !s.combat().exposed(rear[1]),
              "A single front death exposes only its paired rear slot");
        check(s.combat().validate(c, pose) == CommandReason::Ready, "Gap permits immediate target selection");
        s.actors().damage(front[1], 1000, s.timeMs());
        check(s.combat().exposed(rear[0]) && s.combat().exposed(rear[1]), "Enemy row collapse exposes rear");
        check(s.submit(c).reason == CommandReason::Resolved, "Now exposed target can be hit");
        awaitCharacter(s, {2});
        c = turnCommand(s, PartyAction::Melee);
        c.target = rear[0];
        check(s.submit(c).reason == CommandReason::RearBlocked, "Party rear cannot attack across own row");
        s.party().damage({0}, s.party().character({0})->hitPoints);
        check(s.combat().exposed(rules::CharacterId{2}) && !s.combat().exposed(rules::CharacterId{3}),
              "Unconscious front exposes only its paired party member");
        check(s.combat().validate(c, pose) == CommandReason::Ready, "Rear member can attack through own gap");
        s.party().damage({1}, s.party().character({1})->hitPoints);
        check(s.combat().exposed(rules::CharacterId{2}), "Party row collapse exposes rear");
        check(s.submit(c).reason == CommandReason::Resolved, "Rear uses ordinary melee after collapse");
        s.party().heal({0}, 1);
        check(!s.combat().exposed(rules::CharacterId{2}), "Healing restores paired-slot protection");
    }
}
void advancement() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    for (auto heading : {Heading::North, Heading::East, Heading::South, Heading::West})
        for (int lane = 0; lane < 2; ++lane) {
            const Player pose{{3, 3}, heading};
            const auto placements = combatPlacements(map, pose);
            GameSession s(map, catalog, placements, 5, pose);
            start(s);
            ActorId front, rear, blockedRear;
            Cell vacant;
            for (std::size_t i = 0; i < 4; ++i) {
                const auto id = s.actors().id(i);
                const auto sub = s.actors().actor(id)->npc.subcell;
                const int column = heading == Heading::North || heading == Heading::South ? sub.x : sub.y;
                if (column == lane) {
                    if (s.combat().exposed(id)) {
                        front = id;
                        vacant = sub;
                    } else
                        rear = id;
                } else if (!s.combat().exposed(id))
                    blockedRear = id;
            }
            const auto blockedPosition = s.actors().actor(blockedRear)->npc.subcell;
            s.actors().damage(front, 1000, s.timeMs());
            check(s.combat().exposed(rear) && !s.combat().exposed(blockedRear),
                  "Either single lane can open independently from all four approaches");
            check(!(s.actors().actor(rear)->npc.subcell == vacant), "Advance waits for own turn");
            bool attacked = false;
            std::array<std::uint64_t, 4> lastRound{};
            for (int step = 0; step < 40; ++step) {
                const auto round = s.combat().snapshot().round;
                if (s.combat().snapshot().awaitingAction)
                    s.submit(turnCommand(s));
                else {
                    const auto out = s.update(.45, false);
                    if (out.enemyAttack) {
                        check(lastRound[out.enemy.slot] != round, "Advancement grants no extra attack");
                        lastRound[out.enemy.slot] = round;
                        attacked |= sameActor(out.enemy, rear);
                        check(!sameActor(out.enemy, blockedRear), "Protected opposite rear cannot attack");
                    }
                }
            }
            check(attacked && s.actors().actor(rear)->npc.subcell == vacant,
                  "Exposed melee enemy advances to paired vacant front and attacks");
            check(s.actors().actor(blockedRear)->npc.subcell == blockedPosition,
                  "Other lane remains in place");
            check(s.actors().move(blockedRear, s.actors().actor(rear)->npc.cell, vacant, pose.cell).error ==
                      ActorError::Occupied,
                  "Advanced slot remains collision occupied");
        }
}
void feedback() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const auto placements = combatPlacements(map);
    GameSession s(map, catalog, placements, 5, spawn);
    start(s);
    awaitCharacter(s, {0});
    CommandController ui(s);
    auto targets = allCombatTargets(s);
    const auto enemy = s.actors().id(2);
    targets.anchors[enemy.slot] = {100, 90, true};
    ui.refresh(targets, false, false, true);
    CombatOutcome hit;
    hit.event = CombatEvent::Attack;
    hit.enemy = enemy;
    hit.hit = true;
    hit.damage = 12;
    s.actors().damage(enemy, 1000, s.timeMs());
    ui.record(hit);
    ui.refresh(allCombatTargets(s), false, false, true);
    const auto find = [&](bool party) -> const CombatFeedback* {
        for (const auto& entry : ui.snapshot().feedback)
            if (entry.active && entry.party == party)
                return &entry;
        return nullptr;
    };
    check(find(false) && find(false)->damage == 12 && find(false)->anchor.x == 100,
          "Lethal hit retains prior body anchor after target refresh");
    const auto rect = feedbackRect(*find(false), ui.snapshot().cards);
    check(rect && rect->x + rect->width / 2 == viewX + 100 && rect->y + rect->height / 2 == viewY + 90,
          "Monster feedback centered in viewport");
    CombatOutcome miss = hit;
    miss.enemyAttack = true;
    miss.recipient = {3};
    miss.hit = false;
    miss.damage = 0;
    ui.record(miss);
    check(find(true) && !find(true)->hit && find(false), "Party MISS and monster damage coexist");
    const auto partyRect = feedbackRect(*find(true), ui.snapshot().cards);
    const auto card = hudLayout::card(3);
    check(partyRect && partyRect->x >= card.x && partyRect->x + partyRect->width < card.x + card.width,
          "Party feedback belongs to recipient's portrait, not active character");
    s.update(30, true);
    ui.refresh(allCombatTargets(s), false, false, true);
    check(find(true) && find(false), "Pause freezes feedback lifetime");
    s.update(.999, false); // Current party turn holds simulation actions.
    ui.refresh(allCombatTargets(s), false, false, true);
    check(find(true) && find(false), "Feedback survives until duration boundary");
    hit.enemyAttack = true;
    hit.recipient = {3};
    ui.record(hit);
    unsigned partyCount = 0;
    for (const auto& entry : ui.snapshot().feedback)
        partyCount += entry.active && entry.party;
    check(partyCount == 1 && find(true)->hit, "New hit replaces same recipient's MISS");
    s.update(.001, false);
    ui.refresh(allCombatTargets(s), false, false, true);
    check(!find(false) && find(true), "Old feedback expires while replacement persists");
    s.update(1, false);
    ui.refresh(allCombatTargets(s), false, false, true);
    check(!find(true), "Replacement also expires");
    for (const auto point : {TargetAnchor{-20, -40, true}, TargetAnchor{900, 800, true}}) {
        CombatFeedback edge;
        edge.active = true;
        edge.anchor = point;
        const auto r = feedbackRect(edge, ui.snapshot().cards);
        check(r && r->x >= viewX && r->y >= viewY && r->x + r->width <= viewX + viewWidth &&
                  r->y + r->height <= viewY + viewHeight,
              "Whole splash stays inside viewport at edges");
    }
    ui.record(miss);
    ui.refresh(allCombatTargets(s), true, false, true);
    check(!find(true), "Movement clears old scene feedback");
    GameSession restarted(map, catalog, placements, 5, spawn);
    CommandController fresh(restarted);
    fresh.record(miss);
    for (const auto& entry : fresh.snapshot().feedback)
        check(!entry.active, "Restart rejects feedback from old world");
}
void commandsAndRng() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const auto placements = combatPlacements(map);
    GameSession a(map, catalog, placements, 12, spawn), b(map, catalog, placements, 12, spawn);
    start(a);
    start(b);
    awaitCharacter(a, {0});
    awaitCharacter(b, {0});
    auto c = turnCommand(a, PartyAction::Melee);
    c.target = a.actors().id(2);
    const auto turn = c.turn;
    auto invalid = c;
    invalid.turn--;
    check(a.submit(invalid).reason == CommandReason::StaleTurn, "Old token rejected");
    invalid = c;
    invalid.character = {99};
    check(a.submit(invalid).reason == CommandReason::InvalidCharacter, "Invalid character");
    invalid = c;
    invalid.character = {1};
    check(a.submit(invalid).reason == CommandReason::NotYourTurn, "Only active character acts");
    invalid = c;
    invalid.target = b.actors().id(2);
    check(a.submit(invalid).reason == CommandReason::InvalidTarget, "Foreign target rejected");
    invalid = c;
    invalid.action = PartyAction::BackMelee;
    check(a.submit(invalid).reason == CommandReason::FeatureRequired, "Rogue exception deferred");
    invalid = c;
    invalid.action = PartyAction::Aid;
    invalid.ally = rules::CharacterId{1};
    check(a.submit(invalid).reason == CommandReason::InvalidAlly, "Aid conscious rejected");
    check(a.combat().snapshot().turn == turn, "Rejected requests spend no turn");
    auto other = turnCommand(b, PartyAction::Melee);
    other.target = b.actors().id(2);
    const auto x = a.submit(c), y = b.submit(other);
    check(x.roll == y.roll && x.damage == y.damage && x.critical == y.critical,
          "Rejected requests preserve RNG sequence");
    check(a.submit(c).reason == CommandReason::StaleTurn, "Duplicate command cannot execute twice");
    // Production command gating must use the current visibility revision.
    awaitCharacter(a, {0});
    CommandController ui(a);
    auto targets = allCombatTargets(a);
    ui.refresh(targets, false, false, true);
    ui.selectTarget(a.actors().id(2));
    const auto selected = ui.snapshot().selection;
    auto hidden = targets;
    hidden.count = 0;
    ui.refresh(hidden, false, false, true);
    check(ui.submit(selected) == CommandReason::NotVisible, "Hidden bodies cannot be targeted through list");
    ui.refresh(targets, false, false, true);
    a.actors().face(a.actors().id(2), Heading::East);
    check(ui.submit(selected) == CommandReason::NotVisible, "Stale picking revision rejected");
    ui.refresh(allCombatTargets(a), false, false, true);
    ui.selectAction(PartyAction::Defend);
    ui.toggleLog();
    check(ui.confirm() == CommandReason::LogOpen, "Expanded log blocks confirmation");
    ui.toggleLog();
    ui.selectAction(PartyAction::Defend);
    const auto repeated = ui.snapshot().selection;
    check(ui.confirm() == CommandReason::Resolved && ui.submit(repeated) == CommandReason::StaleTurn,
          "UI routes authority once");
    check(a.combat().snapshot().defending[0], "Defend active after action");
    awaitCharacter(a, {0});
    check(!a.combat().snapshot().defending[0], "Defend expires at next turn start");
}
void movementAndPause() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const auto placements = combatPlacements(map);
    for (bool animated : {false, true}) {
        GameSession s(map, catalog, placements, 5, {{3, 5}, Heading::North});
        s.setAnimated(animated);
        s.move(Action::Forward);
        if (animated) {
            s.move(Action::Forward);
            s.update(.16, false);
        } else
            s.move(Action::Forward);
        check(s.movement().player.cell == Cell{3, 3}, "Queued or instant move reaches contact cell");
        check(s.combat().snapshot().phase == CombatPhase::Contact,
              "Contact captured at committed destination");
        s.move(Action::Back);
        s.move(Action::TurnRight);
        s.update(.16, false);
        check(s.movement().player == spawn, "Pending escape and turns cannot bypass contact");
        const auto clock = s.timeMs(), turn = s.combat().snapshot().turn;
        s.update(30, true);
        check(s.timeMs() == clock && s.combat().snapshot().turn == turn, "Pause freezes clock and turns");
        if (s.combat().snapshot().awaitingAction)
            s.submit(turnCommand(s));
        const auto before = s.combat().snapshot().round;
        s.update(30, false);
        check(s.combat().snapshot().round <= before + 1, "Large frame has no automatic round catch-up");
    }
    GameSession side(map, catalog, placements, 5, {{2, 2}, Heading::North});
    check(side.combat().snapshot().phase == CombatPhase::Exploration, "Side contact not in this slice");
    side.move(Action::TurnRight);
    check(side.combat().snapshot().phase == CombatPhase::Contact, "Turning to face contact starts fight");
}
void recoveryAndOutcomes() {
    const auto map = combatMap();
    const auto safe = combatCatalog(1000, -100, {1, 4, -100});
    const auto placements = combatPlacements(map);
    bool aidSuccess = false, aidFailure = false, naturalTwenty = false, death = false;
    for (std::uint32_t seed = 0; seed < 100 && !(aidSuccess && aidFailure && naturalTwenty && death);
         ++seed) {
        GameSession s(map, safe, placements, seed, spawn);
        start(s);
        awaitCharacter(s, {0});
        s.party().damage({3}, s.party().character({3})->hitPoints);
        auto c = turnCommand(s, PartyAction::Aid);
        c.ally = rules::CharacterId{3};
        const auto aid = s.submit(c);
        aidSuccess |= aid.hit;
        aidFailure |= !aid.hit;
        check(s.combat().snapshot().turn != c.turn, "Aid spends turn whether successful or not");
        check((s.party().character({3})->life == rules::Life::Stable) == aid.hit,
              "Aid outcome updates authoritative health");
        if (aid.hit)
            s.party().damage({3}, 1);
        for (int step = 0; step < 120 && s.party().character({3})->life == rules::Life::Dying; ++step) {
            if (s.combat().snapshot().awaitingAction)
                s.submit(turnCommand(s));
            else {
                const auto presented = s.combat().snapshot();
                const auto out = s.update(.45, false);
                if (out.event == CombatEvent::DeathSave && out.character.index == 3 && out.roll == 20) {
                    naturalTwenty = true;
                    check(!confirmationMatches(presented, s.combat().snapshot()),
                          "Natural twenty must be presented before confirming the regained action");
                    check(s.combat().snapshot().awaitingAction &&
                              s.combat().snapshot().activeCharacter->index == 3,
                          "Twenty permits same turn action");
                }
                death |= out.died;
            }
        }
    }
    check(aidSuccess && aidFailure && naturalTwenty && death,
          "Deterministic seed set exercises Aid and death-save outcomes");
    GameSession victory(map, safe, placements, 5, spawn);
    start(victory);
    victory.party().damage({0}, victory.party().character({0})->hitPoints);
    victory.party().damage({1}, 100000);
    for (std::size_t i = 0; i < 4; ++i)
        victory.actors().damage(victory.actors().id(i), 1000, 0);
    check(victory.update(.45, false).phase == CombatPhase::Victory, "All enemies dead gives victory");
    check(victory.party().character({0})->life == rules::Life::Stable &&
              victory.party().character({0})->hitPoints == 0,
          "Victory stabilizes without healing");
    check(victory.party().character({1})->life == rules::Life::Dead, "Victory cannot resurrect");
    check(victory.update(.45, false).event == CombatEvent::None, "Victory resolves once");
    check(victory.combat().continueExploring() && !victory.combat().continueExploring(), "Continue once");
    check(victory.party().swapFormation(FormationSlot::FrontLeft, FormationSlot::BackLeft),
          "Continue unlocks formation");
    GameSession defeat(map, safe, placements, 5, spawn);
    start(defeat);
    for (std::uint16_t i = 0; i < 4; ++i) {
        defeat.party().damage({i}, defeat.party().character({i})->hitPoints);
        defeat.party().stabilize({i});
    }
    check(defeat.update(.45, false).phase == CombatPhase::Defeat, "All stable is still immediate defeat");
    check(defeat.update(.45, false).event == CombatEvent::None, "Defeat once");
    check(!defeat.combat().continueExploring(), "Cannot continue after defeat");
}
void groupedTurnsAndAidInput() {
    const auto map = combatMap();
    auto f = partyFixture();
    rules::MonsterDefinition a;
    a.id = "group-a";
    a.abilities.fill(10);
    a.armorClass = 15;
    a.hitPoints = 1000;
    a.attacks[0] = {0};
    a.attackCount = 1;
    auto b = a;
    b.id = "group-b";
    const rules::Catalog catalog(
        {{"melee", rules::AttackKind::Melee, -100, {1, 4, -100}, rules::DamageType::Slashing}}, {a, b},
        f.races, f.classes, f.items, f.preset);
    const NpcWorld placements(map, spawn.cell, {"group-a", "group-b"},
                              {{{0}, {3, 2}, {0, 0}, Heading::East},
                               {{1}, {3, 2}, {1, 0}, Heading::East},
                               {{0}, {3, 2}, {0, 1}, Heading::East},
                               {{1}, {3, 2}, {1, 1}, Heading::East}});
    GameSession game(map, catalog, placements, 123, spawn);
    start(game);
    std::array<int, 4> characterActions{}, enemyActions{};
    for (int i = 0; i < 100 && game.combat().snapshot().round == 1; ++i) {
        const auto& state = game.combat().snapshot();
        if (state.awaitingAction) {
            ++characterActions[state.activeCharacter->index];
            game.submit(turnCommand(game));
        } else {
            const auto turn = state.turn;
            check(game.update(0, false).event == CombatEvent::None && state.turn == turn,
                  "Same timestamp cannot duplicate automatic turn");
            const auto out = game.update(.45, false);
            if (out.event == CombatEvent::Attack) {
                ++enemyActions[out.enemy.slot];
                check(out.enemyAttack && out.recipient.index < 2, "Enemies target conscious front row only");
            }
        }
    }
    check(characterActions == std::array<int, 4>{1, 1, 1, 1}, "Individual turns once each per round");
    check(enemyActions == std::array<int, 4>{0, 0, 1, 1},
          "Separate enemy groups act once, protected rear never attacks");
    awaitCharacter(game, {0});
    game.party().damage({2}, game.party().character({2})->hitPoints);
    CommandController ui(game);
    ui.refresh(allCombatTargets(game), false, false, true);
    ui.selectAction(PartyAction::Aid);
    UiInput input;
    const auto command = ui.snapshot().selection;
    const auto turn = game.combat().snapshot().turn;
    input.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
    const auto portrait = hudLayout::card(2);
    input.pointer = ArtPoint{float(portrait.x + 5), float(portrait.y + 5)};
    routeHudInput(ui, input);
    check(game.combat().snapshot().turn != turn && ui.snapshot().selection.action == PartyAction::Melee,
          "Aid portrait click commits and resets to basic attack");
    GameSession restart(map, catalog, placements, 123, spawn);
    start(restart);
    awaitCharacter(restart, {0});
    check(restart.submit(command).reason == CommandReason::StaleTurn,
          "Restart rejects prior session turn token");
    check(restart.actors().actor(game.actors().id(0)) == nullptr, "Restart has fresh actor identity");
    check(restart.party().character({2})->hitPoints == restart.party().character({2})->stats.maxHitPoints,
          "Restart restores preset without copying old resources");
}
void contentAndInvalidTargets() {
    const auto map = combatMap();
    const auto placements = combatPlacements(map);
    auto f = partyFixture();
    f.preset.characters[0].equipment.mainHand = {};
    rules::MonsterDefinition m;
    m.id = "goblin";
    m.abilities.fill(10);
    m.armorClass = 15;
    m.hitPoints = 1000;
    m.attacks[0] = {0};
    m.attackCount = 1;
    const rules::Catalog unarmed(
        {{"melee", rules::AttackKind::Melee, -100, {1, 4, -100}, rules::DamageType::Slashing}}, {m}, f.races,
        f.classes, f.items, f.preset);
    GameSession game(map, unarmed, placements, 123, spawn);
    start(game);
    awaitCharacter(game, {0});
    auto c = turnCommand(game, PartyAction::Melee);
    c.target = game.actors().id(2);
    check(game.submit(c).reason == CommandReason::NoWeapon, "Unequipped weapon does not use carried preview");
    check(game.submit(turnCommand(game)).reason == CommandReason::Resolved, "Unarmed character can defend");
    const rules::Catalog rangedOnly(
        {{"bow", rules::AttackKind::Ranged, 4, {1, 6, 2}, rules::DamageType::Piercing}}, {m}, f.races,
        f.classes, f.items, f.preset);
    bool rejected = false;
    try {
        GameSession invalid(map, rangedOnly, placements, 1, spawn);
    } catch (const std::runtime_error&) {
        rejected = true;
    }
    check(rejected, "Unsupported monster attack content rejected before play");
    const auto safe = combatCatalog(1000, -100, {1, 4, -100});
    GameSession targets(map, safe, placements, 5, spawn);
    start(targets);
    awaitCharacter(targets, {0});
    c = turnCommand(targets, PartyAction::Melee);
    c.target = targets.actors().id(2);
    targets.actors().damage(*c.target, 1000, targets.timeMs());
    check(targets.submit(c).reason == CommandReason::InvalidTarget, "Dead target rejected before action");
    targets.actors().remove(*c.target);
    check(targets.submit(c).reason == CommandReason::InvalidTarget, "Removed target rejected before action");
    const auto turn = targets.combat().snapshot().turn;
    c.target.reset();
    check(targets.submit(c).reason == CommandReason::MissingTarget &&
              targets.combat().snapshot().turn == turn,
          "Missing target preserves turn");
    check(!targets.party().heal({99}, 1) && !targets.party().deathSave({99}, 20),
          "Party operations validate identity");
}
void initiativeAndDefend() {
    const auto map = combatMap();
    const auto placements = combatPlacements(map);
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    bool tie = false, defendedAttack = false;
    for (std::uint32_t seed = 0; seed < 100; ++seed) {
        GameSession s(map, catalog, placements, seed, spawn);
        rules::DiceRng expected(seed);
        std::array<int, 5> totals{}, dexterity{};
        for (std::uint16_t i = 0; i < 4; ++i) {
            totals[i] = expected.d20().value.selected + s.party().character({i})->stats.initiative;
            dexterity[i] = s.party().character({i})->stats.abilities[1];
        }
        totals[4] = expected.d20().value.selected + 2;
        dexterity[4] = 14;
        start(s);
        int previous = -1;
        bool firstEnemy = true;
        for (int step = 0; step < 10 && s.combat().snapshot().round == 1; ++step) {
            const auto state = s.combat().snapshot();
            const int current = state.activeCharacter ? state.activeCharacter->index : 4;
            if (previous >= 0) {
                check(totals[previous] >= totals[current], "Observed initiative totals descend");
                if (totals[previous] == totals[current] && previous != current) {
                    tie = true;
                    check(dexterity[previous] >= dexterity[current], "Ties favor higher Dexterity");
                    if (dexterity[previous] == dexterity[current])
                        check(previous < current, "Exact ties favor party then stable identity");
                }
            }
            previous = current;
            if (state.awaitingAction)
                s.submit(turnCommand(s));
            else {
                // Defending is target-specific, and only actual defended attacks consume two d20s.
                if (firstEnemy) {
                    const auto recipient = (expected.roll({1, 4, 0}).value.total - 1) % 2;
                    const bool defending = state.defending[static_cast<std::size_t>(recipient)];
                    const auto roll =
                        expected.d20(defending ? rules::RollMode::Disadvantage : rules::RollMode::Normal)
                            .value;
                    const auto out = s.update(.45, false);
                    check(out.recipient.index == recipient && out.roll == roll.selected,
                          "Enemy choice and Defend use the seeded unbiased rolls");
                    defendedAttack |= defending;
                    firstEnemy = false;
                } else
                    s.update(.45, false);
            }
        }
    }
    check(tie && defendedAttack, "Seed set covers initiative ties and defended attacks");
    GameSession removed(map, catalog, placements, 5, spawn);
    for (std::size_t i = 0; i < 4; ++i)
        removed.actors().remove(removed.actors().id(i));
    check(removed.update(0, false).phase == CombatPhase::Victory,
          "Removed contact members cannot be dereferenced at initiative start");
}
void presentedInput() {
    const auto map = combatMap();
    const auto catalog = combatCatalog(1000, -100, {1, 4, -100});
    const auto placements = combatPlacements(map);
    GameSession s(map, catalog, placements, 5, spawn);
    CommandController ui(s);
    const auto contact = ui.snapshot().combat;
    start(s);
    awaitCharacter(s, {0});
    ui.refresh(allCombatTargets(s), false, false, true);
    ui.selectAction(PartyAction::Defend);
    const auto turn = s.combat().snapshot().turn;
    UiInput input;
    input.pressed[static_cast<std::size_t>(UiKey::Confirm)] = true;
    check(!confirmationMatches(contact, s.combat().snapshot()), "New combat turn was not presented yet");
    routeHudInput(ui, input, {}, confirmationMatches(contact, s.combat().snapshot()));
    check(s.combat().snapshot().turn == turn, "Pre-turn Enter cannot consume newly selected action");
    input.pressed = {};
    input.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
    input.pointer = ArtPoint{float(hudLayout::action(1).x + 3), float(hudLayout::action(1).y + 3)};
    routeHudInput(ui, input, {}, false);
    check(s.combat().snapshot().turn == turn, "Pre-turn mouse confirmation also rejected");
    routeHudInput(ui, input, {}, confirmationMatches(ui.snapshot().combat, s.combat().snapshot()));
    check(s.combat().snapshot().turn != turn, "Fresh confirmation for presented turn acts");
    const auto shown = s.combat().snapshot();
    for (std::uint16_t i = 0; i < 4; ++i)
        s.party().damage({i}, s.party().character({i})->hitPoints);
    s.update(.45, false);
    check(s.combat().snapshot().phase == CombatPhase::Defeat &&
              !confirmationMatches(shown, s.combat().snapshot()),
          "Automatic defeat cannot consume a confirmation from the preceding view");
    check(confirmationMatches(s.combat().snapshot(), s.combat().snapshot()),
          "Presented outcome allows a later fresh confirmation");
}
void completeFights() {
    const auto map = combatMap();
    const auto placements = combatPlacements(map);
    const auto catalog = combatCatalog();
    bool won = false, lost = false, miss = false, critical = false;
    for (std::uint32_t seed = 0; seed < 30 && !(won && lost && miss && critical); ++seed) {
        for (bool attack : {false, true}) {
            GameSession s(map, catalog, placements, seed, spawn);
            start(s);
            for (int step = 0; step < 3000 && s.combat().snapshot().phase == CombatPhase::Turns; ++step) {
                CombatOutcome out;
                if (s.combat().snapshot().awaitingAction) {
                    auto c = turnCommand(s);
                    if (attack && s.combat().exposed(c.character))
                        for (std::size_t i = 0; i < 4; ++i) {
                            const auto id = s.actors().id(i);
                            if (s.actors().actor(id)->life == ActorLife::Alive && s.combat().exposed(id)) {
                                c.action = PartyAction::Melee;
                                c.target = id;
                                break;
                            }
                        }
                    out = s.submit(c);
                    check(out.reason == CommandReason::Resolved, "Complete fight command resolves");
                } else
                    out = s.update(.45, false);
                if (out.event == CombatEvent::Attack) {
                    miss |= !out.hit;
                    critical |= out.critical;
                }
                if (out.enemyAttack) {
                    check(out.recipient.index < 4, "Enemy target valid");
                    const auto* attacker = s.actors().actor(out.enemy);
                    check(attacker->hasAttack && attacker->attackTimeMs == s.timeMs(),
                          "Every enemy attack, hit or miss, publishes an animation cue");
                }
            }
            won |= s.combat().snapshot().phase == CombatPhase::Victory;
            lost |= s.combat().snapshot().phase == CombatPhase::Defeat;
            check(s.combat().snapshot().phase != CombatPhase::Turns, "Complete fight terminates");
        }
    }
    check(won && lost && miss && critical,
          "Unmodified preset mechanics produce wins, losses, misses and crits");
}
} // namespace
int main() {
    try {
        rows();
        advancement();
        feedback();
        commandsAndRng();
        movementAndPause();
        recoveryAndOutcomes();
        groupedTurnsAndAidInput();
        contentAndInvalidTargets();
        initiativeAndDefend();
        presentedInput();
        completeFights();
        std::cout << "Combat checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
