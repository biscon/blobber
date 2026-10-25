#pragma once
#include "game/session.hpp"
#include "party_fixtures.hpp"
namespace blobber {
inline rules::Catalog combatCatalog(int hp = 7, int bonus = 4, rules::Dice damage = {1, 6, 2}, int ac = 15) {
    auto f = partyFixture();
    rules::MonsterDefinition m;
    m.id = "goblin";
    m.abilities = {8, 14, 10, 10, 8, 8};
    m.armorClass = ac;
    m.hitPoints = hp;
    m.attacks[0] = {0};
    m.attackCount = 1;
    return {{{"melee", rules::AttackKind::Melee, bonus, damage, rules::DamageType::Slashing}},
            {m},
            f.races,
            f.classes,
            f.items,
            f.preset};
}
inline Map combatMap() {
    return Map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
}
inline NpcWorld combatPlacements(const Map& map, Player pose = {{3, 3}, Heading::North}, int count = 4) {
    std::vector<NpcInstance> instances;
    for (int i = 0; i < count; ++i)
        instances.push_back({{0}, pose.cell + forward(pose.heading), {i % 2, i / 2}, Heading::South});
    return {map, pose.cell, {"goblin"}, instances};
}
inline PartyCommand turnCommand(const GameSession& session, PartyAction action = PartyAction::Defend) {
    const auto& state = session.combat().snapshot();
    PartyCommand c;
    c.character = state.activeCharacter.value_or(rules::CharacterId{});
    c.action = action;
    c.world = state.world;
    c.turn = state.turn;
    return c;
}
inline TargetSnapshot allCombatTargets(const GameSession& session) {
    TargetSnapshot t;
    t.world = session.actors().snapshot().world;
    t.revision = session.actors().snapshot().revision;
    for (std::size_t i = 0; i < session.actors().snapshot().count; ++i)
        if (session.actors().snapshot().actors[i].life == ActorLife::Alive)
            t.ids[t.count++] = session.actors().id(i);
    return t;
}
} // namespace blobber
