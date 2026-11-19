#include "cave_review.hpp"
#include "game/commands.hpp"
#include "game/perception.hpp"
#include "game/session.hpp"
#include "generation/cave_population.hpp"
#include <iostream>
#include <stdexcept>

namespace blobber::cave_review {
using namespace generation;
namespace {
void require(bool ok, const char* why) {
    if (!ok)
        throw std::runtime_error(why);
}
void equip(Party& party, rules::CharacterId owner, const char* name, EquipmentSlot slot) {
    const auto item = party.catalog().findItem(name);
    const auto& bag = party.inventory();
    require(bool(party.acquire({owner, item, 1, bag.revision()})), "Review equipment did not fit");
    for (unsigned n = 0; n < bagCells; ++n) {
        auto handle = bag.at({owner, std::uint16_t(n)});
        const auto* stack = bag.stack(handle);
        if (stack && stack->item.index == item.index) {
            require(bool(party.inventoryChange(
                        {InventoryOp::Move, handle, {owner, std::uint16_t(slot)}, 0, bag.revision()})),
                    "Review equipment rejected");
            return;
        }
    }
    throw std::runtime_error("Missing review equipment");
}
void advance(Party& party, GameSession* session = nullptr) {
    const auto& catalog = party.catalog();
    for (std::uint16_t n = 0; n < 4; ++n) {
        const auto& c = *party.character({n});
        auto request = session ? session->advancementRequest({n}) : party.advancementRequest({n});
        auto& picks = request.choices;
        if (c.build.level == 3)
            picks.abilityImprovement.increases[unsigned(rules::Ability::Constitution)] = 2;
        for (auto kind :
             {rules::SpellChoice::Cantrips, rules::SpellChoice::Learned, rules::SpellChoice::Prepared}) {
            const auto options = rules::advancementChoices(catalog, c.build.view(), c.build.portrait,
                                                           c.build.spells, picks, kind);
            auto chosen = rules::recommendedChoices(catalog, options, kind);
            const auto preferred =
                rules::spellBit(catalog.findSpell(n == 1 ? "inflict_wounds" : "magic_missile"));
            if (chosen && (options.available & ~options.locked & preferred) && !(chosen & preferred))
                chosen = (chosen & (chosen - 1)) | preferred;
            if (kind == rules::SpellChoice::Cantrips)
                picks.cantrips = chosen;
            if (kind == rules::SpellChoice::Learned)
                picks.learned = chosen;
            if (kind == rules::SpellChoice::Prepared)
                picks.prepared = chosen;
        }
        require(bool(session ? session->advance(request) : party.advance(request)),
                "Review party advancement rejected");
    }
}
void finishGroup(GameSession& game, const Encounter& e) {
    const auto& world = game.actors().snapshot();
    // Advance the route independently of dice and party policy. Bind to real
    // deaths and let the session issue XP, objectives and loot normally.
    for (unsigned j = e.first; j < e.first + e.count; ++j)
        if (world.actors[j].npc.vault == VaultPart::None || world.actors[j].npc.vault == VaultPart::Mother)
            game.actors().damage(game.actors().id(j), 10000, game.timeMs());
    for (unsigned tick = 0; tick < 50; ++tick) {
        game.update(.1, false);
        if (game.combat().snapshot().phase == CombatPhase::Victory)
            game.combat().continueExploring();
        if (game.combat().snapshot().phase == CombatPhase::Exploration)
            return;
    }
    throw std::runtime_error("Route encounter did not settle");
}
// One equipped-party opening after the route fixture supplies the incoming XP.
// Drive attacks through normal session commands and require actual target sight.
void fightGuardians(GameSession& game, const Encounter& group) {
    auto& actors = game.actors();
    const auto& catalog = game.party().catalog();
    unsigned damage = 0, downs = 0, spells = 0;
    for (unsigned tick = 0; tick < 6000; ++tick) {
        const auto& events = game.update(.1, false);
        for (const auto& event : events)
            if (event.event == CombatEvent::Attack && event.enemyAttack) {
                damage += unsigned(event.damage);
                downs += event.downed;
            }
        const auto& turn = game.combat().snapshot();
        if (turn.phase == CombatPhase::Victory || turn.phase == CombatPhase::Defeat) {
            std::cout << "Guardian opening: round " << turn.round << ", " << damage << " damage taken, "
                      << downs << " downs, " << spells << " spell casts, "
                      << (turn.phase == CombatPhase::Victory ? "victory" : "defeat") << '\n';
            require(turn.phase == CombatPhase::Victory && game.combat().continueExploring(),
                    "Guardian opening defeated");
            return;
        }
        if (!turn.awaitingAction || !turn.activeCharacter)
            continue;
        PartyCommand cmd;
        cmd.world = turn.world;
        cmd.turn = turn.turn;
        cmd.character = *turn.activeCharacter;
        auto act = [&](PartyAction action, const char* spell = nullptr) {
            cmd.action = action;
            cmd.spell = spell ? catalog.findSpell(spell) : rules::SpellId{};
            cmd.targetCount = spell ? 1 : 0;
            if (spell) {
                cmd.enemies[0] = *cmd.target;
                cmd.darts[0] = 3;
            }
            const bool ready = game.submit(cmd).reason == CommandReason::Resolved;
            if (ready && spell)
                ++spells;
            return ready;
        };
        bool acted = false;
        const auto pose = game.movement().player;
        for (unsigned n = group.first; n < group.first + group.count && !acted; ++n) {
            const auto& actor = actors.snapshot().actors[n];
            if (actor.life != ActorLife::Alive ||
                !inPartyView(game.geometry(), pose.cell, pose.heading, actor.npc.cell, actor.npc.subcell))
                continue;
            cmd.target = actors.id(n);
            if (game.combat().warnedTarget(*cmd.target).index == cmd.character.index)
                acted = act(PartyAction::Defend);
            if (!acted && cmd.character.index == 3)
                acted = act(PartyAction::Spell, "magic_missile") || act(PartyAction::Spell, "fire_bolt");
            if (!acted && cmd.character.index == 1)
                acted = act(PartyAction::Spell, "inflict_wounds");
            if (!acted)
                acted = act(PartyAction::Melee) || act(PartyAction::Ranged);
        }
        if (!acted) {
            cmd.action = PartyAction::Feature;
            cmd.feature = rules::Feature::EndTurn;
            cmd.target.reset();
            cmd.targetCount = 0;
            require(game.submit(cmd).reason == CommandReason::Resolved, "Unable to finish guardian turn");
        }
    }
    throw std::runtime_error("Guardian opening timed out");
}
unsigned units(const Party& party) {
    const auto& c = *party.character({0});
    return c.experience * rules::experienceScale + c.experienceFraction;
}
} // namespace
Party::Saved populationParty(const rules::Catalog& catalog, unsigned depth, bool fullEntry) {
    Party party(catalog);
    // Representative legal Underkeep rewards for a four-survivor party.
    equip(party, {0}, "cloak_of_protection", EquipmentSlot::Cloak);
    equip(party, {2}, "boots_of_balance", EquipmentSlot::Feet);
    const auto incoming = (fullEntry ? 3175u : 2700u) + (depth >= 5 ? (fullEntry ? 5125u : 4600u) : 0u) +
                          (depth == 6 ? (fullEntry ? 7900u : 7600u) : 0u);
    party.awardExperience(incoming);
    const auto target = depth - 2;
    while (unsigned(party.character({0})->build.level) < target)
        advance(party);
    if (depth > 4)
        require(bool(party.acquire({{0}, catalog.findItem("camp_kit"), 1, party.inventory().revision()})),
                "Previous guardian's camp kit did not fit");
    return party.checkpoint();
}
Party::Saved guardianParty(const rules::Catalog& catalog, const Party::Saved& incoming, unsigned depth) {
    Party party(catalog, incoming);
    constexpr unsigned ordinaryBeforeGuardian[]{2400, 5600, 10400};
    party.awardExperience(ordinaryBeforeGuardian[depth - 4]);
    if (party.advancementEligibility(party.advancementRequest({0})) == AdvancementError::None)
        advance(party);
    return party.checkpoint();
}
void checkPopulation(GameSession& game, const CaveFloor& floor, const RecoveryLevel& recovery, bool full) {
    const auto& f = floor.layout;
    const auto& world = game.actors().snapshot();
    const unsigned expectedCounts[]{19, 22, 30}, ordinary[]{4600, 7600, 10600}, extras[]{525, 300, 300};
    const auto depth = f.depth - 4;
    const auto start = units(game.party());
    const auto gate = f.fixtures.entries[f.guardianGate].cell;
    require(world.count == expectedCounts[depth] && world.count == f.populationCount,
            "Wrong cave roster size");
    require(game.landingSafety(0).safe && game.geometry().solid(gate), "Unsafe entry or unguarded exit");
    game.setAnimated(false);
    CellMask forbidden{};
    std::vector<Cell> steps;
    auto walk = [&](Cell dest, bool contact = false) { walkTo(game, dest, {}, &steps, contact, &forbidden); };
    auto collect = [&](unsigned index, Cell cell) {
        walk(cell);
        const auto& loot = game.loot().snapshot();
        const auto slot = std::uint16_t(NpcWorld::maxInstances * lootPerActor + index);
        const LootId id{loot.world, slot, loot.stacks[slot].generation};
        bool taken = false;
        for (std::uint16_t n = 0; !taken && n < 4; ++n)
            taken = bool(game.pickup({id, {n}, game.party().inventory().revision()}));
        require(taken && !game.loot().find(id), "Finite cave supply could not be collected");
    };
    for (unsigned i = 0; i < recovery.groundCount - 2; ++i)
        collect(i, recovery.ground[i].cell);
    std::array<bool, 8> cleared{};
    unsigned priorSteps = 0;
    for (unsigned i = 0; i < f.encounterCount; ++i) {
        const auto& e = f.encounters[i];
        if (e.role == EncounterRole::Optional && !full)
            continue;
        forbidden.fill(false);
        for (unsigned j = 0; j < f.encounterCount; ++j)
            if (j != i && !cleared[j])
                for (int k = 0; k < f.width * f.height; ++k)
                    forbidden[k] |= f.membership[k] == f.encounters[j].room;
        const auto target = f.population[e.first].cell;
        walk(target, true);
        for (unsigned tick = 0; tick < 30 && realTime(game.combat().snapshot().phase); ++tick)
            game.update(.1, false);
        bool reachedContact = false;
        for (unsigned member = e.first; member < e.first + e.count; ++member)
            reachedContact |= game.combat().member(game.actors().id(member));
        require(reachedContact, "Expected encounter did not enroll on approach");
        for (unsigned j = 0; j < world.count; ++j)
            if (j < e.first || j >= e.first + e.count)
                require(!game.combat().member(game.actors().id(j)) || world.actors[j].hasDeath,
                        "Independent encounter joined the route");
        if (!full && depth < 2 && e.role == EncounterRole::Guardian)
            fightGuardians(game, e);
        else
            finishGroup(game, e);
        cleared[i] = true;
        auto request = game.party().advancementRequest({0});
        if (game.party().advancementEligibility(request) == AdvancementError::None) {
            require(game.advancementEligibility(game.advancementRequest({0})) == AdvancementError::None,
                    "No safe advancement after clear");
            advance(game.party(), &game);
            std::cout << "Manual advancement available at " << encounterRecipe(e.recipe).name << ", level "
                      << game.party().character({0})->build.level << '\n';
        }
        std::cout << (full ? "Full " : "Ordinary ") << encounterRecipe(e.recipe).name << " room "
                  << unsigned(e.room) << " at " << target.x << ',' << target.y << " after "
                  << steps.size() - priorSteps << " steps; " << units(game.party()) / 12.0 << " XP each\n";
        priorSteps = steps.size();
        if (e.role == EncounterRole::Optional) {
            const auto idx = unsigned(recovery.groundCount) - (e.room == 5 ? 2 : 1);
            collect(idx, recovery.ground[idx].cell);
        }
        require(game.geometry().solid(gate) == (e.role != EncounterRole::Guardian),
                "Guardian gate changed early or late");
    }
    require(game.objective().completed && game.objective().rewardIssued, "Guardian completion missing");
    if (depth < 2)
        collect(unsigned(recovery.groundCount), f.rooms[4].anchor);
    walk(floor.sites[CaveSite::Goal]);
    walk(f.spawn.cell);
    require(game.landingSafety(0).safe, "Return stair threatened after backtrack");
    require(units(game.party()) == start + 3 * (ordinary[depth] + (full ? extras[depth] : 0)),
            "Route XP missing or duplicated on return");
    if (depth == 2 && full) {
        CommandController ui(game);
        ui.openInventory(rules::CharacterId{0}, CharacterPage::Character);
        const auto& sheet = ui.snapshot().inventory.sheet;
        require(sheet.experience == 6775 && !sheet.nextLevelExperience &&
                    !ui.snapshot().cards[0].levelEligible,
                "Character sheet lost banked XP or offered level five");
    }
}
void checkShortcut(GameSession& game, const CaveFloor& floor) {
    const auto& f = floor.layout;
    const auto& boss = f.encounters[f.encounterCount - 1];
    const auto start = units(game.party());
    std::vector<Cell> steps;
    CellMask forbidden{};
    for (unsigned i = 0; i < f.encounterCount; ++i)
        if (f.encounters[i].role == EncounterRole::Optional)
            for (int k = 0; k < f.width * f.height; ++k)
                forbidden[k] |= f.membership[k] == f.encounters[i].room;
    game.setAnimated(false);
    std::cout << "Shortest walk to the guardian:";
    for (unsigned pass = 0; !game.objective().completed && pass < f.encounterCount; ++pass) {
        walkTo(game, f.population[boss.first].cell, {}, &steps, true, &forbidden);
        for (unsigned tick = 0; tick < 30 && realTime(game.combat().snapshot().phase); ++tick)
            game.update(.1, false);
        bool met = false;
        for (unsigned i = 0; i < f.encounterCount; ++i) {
            const auto& e = f.encounters[i];
            bool active = false;
            for (unsigned n = e.first; n < e.first + e.count; ++n)
                active |=
                    game.combat().member(game.actors().id(n)) && !game.actors().snapshot().actors[n].hasDeath;
            if (!active)
                continue;
            require(e.role != EncounterRole::Optional && !met, "Shortcut recruited an unrelated group");
            std::cout << ' ' << encounterRecipe(e.recipe).name;
            finishGroup(game, e);
            met = true;
            break;
        }
        require(met, "Shortcut missed a live contact");
    }
    require(game.objective().completed, "Shortcut never reached guardian");
    std::cout << "; " << steps.size() << " steps, " << (units(game.party()) - start) / 3 << " party XP\n";
}
} // namespace blobber::cave_review
