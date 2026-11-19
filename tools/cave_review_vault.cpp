#include "cave_review.hpp"
#include "game/perception.hpp"
#include "platform/game_view.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace blobber::cave_review {
using Site = generation::CaveSite;
namespace {
void require(bool valid, const char* message) {
    if (!valid)
        throw std::runtime_error(message);
}
} // namespace
ArenaScenario prepareVault(const generation::CaveSites& sites, const std::vector<std::string>& types,
                           const rules::Catalog& catalog, std::vector<NpcInstance>& occupants) {
    generation::placeBroodVault(sites, types, catalog, occupants);

    auto scenario = defaultArena(catalog);
    scenario.adjust(catalog.findMonster("broodmother"), 1);
    scenario.preparation.targetLevel = 2;
    for (unsigned i = 0; i < scenario.party.size(); ++i) {
        const auto& draft = scenario.party[i].draft;
        auto& choices = scenario.preparation.advancements[0][i];
        for (auto kind : {rules::SpellChoice::Learned, rules::SpellChoice::Prepared}) {
            const auto ctx = rules::advancementChoices(catalog, rules::draftIdentity(draft), {}, draft.spells,
                                                       choices, kind);
            auto picks = rules::recommendedChoices(catalog, ctx, kind);
            // Prepare the standard party's offensive options for the burst counterexample.
            const auto preferred = catalog.findSpell(i == 1 ? "inflict_wounds" : "magic_missile");
            const auto bit = rules::spellBit(preferred);
            if (ctx.maximum && (ctx.available & bit) && !(picks & bit)) {
                picks &= picks - 1;
                picks |= bit;
            }
            (kind == rules::SpellChoice::Learned ? choices.learned : choices.prepared) = picks;
        }
    }
    return scenario;
}

void checkVault(GameSession& game, const generation::CaveFloor& floor, const std::vector<NpcTimings>& timings,
                VaultRoute route, const std::filesystem::path& output, WorldRenderer* renderer,
                Presenter* presenter) {
    const bool doorway = route == VaultRoute::DoorwayVolley;
    const bool volley = doorway || route == VaultRoute::OpeningVolley;
    const bool suppress = route == VaultRoute::Suppress, burst = route == VaultRoute::Burst || volley;
    const char* name = suppress  ? "Suppress"
                       : doorway ? "Doorway volley"
                       : volley  ? "Opening volley"
                       : burst   ? "Burst"
                                 : "Replacement";
    auto& actors = game.actors();
    const auto& catalog = game.party().catalog();
    CommandController controls(game, timings);
    NpcAnimator animator(actors, timings, &game.combat());
    std::optional<GameView> view;
    if (renderer)
        view.emplace(*renderer, &floor.layout);
    TargetSnapshot targets;
    const auto refresh = [&] {
        if (view) {
            animator.setTime(game.timeMs(), &game.effects());
            view->update(game, animator.snapshot(), {});
            targets = view->picking()->targets();
        } else {
            targets = {};
            targets.world = actors.snapshot().world;
            targets.revision = actors.snapshot().revision;
            const auto pose = game.movement().player;
            for (unsigned i = 0; i < actors.snapshot().count; ++i) {
                const auto& a = actors.snapshot().actors[i];
                if (a.targetable() && a.life == ActorLife::Alive &&
                    inPartyView(game.geometry(), pose.cell, pose.heading, a.npc.cell, a.npc.subcell))
                    targets.ids[targets.count++] = actors.id(i);
            }
        }
        controls.refresh(targets, game.actionBusy(), false, true);
    };
    const auto photo = [&](const char* file) {
        if (view) {
            view->present(*presenter, game, floor.map, false, true, &controls.snapshot());
            presenter->capture(output / file);
        }
    };
    for (const auto& character : game.party().snapshot().characters)
        require(character.build.level == 2 && character.experience == 300, "Vault party must be level two");
    unsigned firstWarning = 0;
    const auto record = [&](const CombatEvents& events) {
        controls.record(events);
        for (const auto& e : events)
            if (!firstWarning && e.event == CombatEvent::ClutchWarning)
                firstWarning = unsigned(game.combat().snapshot().round);
    };
    // The doorway volley stops two cells farther back to test the choke point.
    walkTo(game, floor.sites[Site::Mother] + Cell{0, doorway ? -5 : -3}, {}, nullptr, true);
    record(game.events());
    unsigned shots = 0, openingCasts = 0, openingCleric = 0;
    for (unsigned tick = 0; tick < 120 && realTime(game.combat().snapshot().phase); ++tick) {
        if (volley) {
            for (std::uint16_t member : {1, 2, 3}) {
                refresh();
                const auto& turn = game.combat().snapshot();
                PartyCommand c;
                c.character = {member};
                c.world = turn.world;
                c.turn = turn.turn;
                c.target = actors.id(0);
                if (member == 2)
                    c.action = PartyAction::Ranged;
                else {
                    c.action = PartyAction::Spell;
                    c.spell = catalog.findSpell(member == 3 ? "magic_missile" : "guiding_bolt");
                    c.targetCount = 1;
                    c.enemies[0] = *c.target;
                    if (member == 3)
                        c.darts[0] = 3;
                }
                if (controls.validate(c) == CommandReason::Ready) {
                    require(controls.submit(c) == CommandReason::Resolved, "Opening shot rejected");
                    ++shots;
                    openingCasts += member == 3;
                    openingCleric += member == 1;
                }
            }
        }
        record(game.update(.1, false));
    }
    require(!volley || (shots >= 2 && openingCasts), "Burst check must include legal pre-contact shots");
    require(!realTime(game.combat().snapshot().phase), "Mother did not reach the northern aisle");
    const auto opening = game.movement().player;
    std::cout << name << " opens at (" << opening.cell.x << ',' << opening.cell.y << "), heading "
              << unsigned(opening.heading) << '\n';
    unsigned hatches = 0, firstHatch = 0, firstAttack = 0, motherDeath = 0, childDamage = 0,
             downs = 0;
    unsigned damageBeforeMotherDeath = 0, broodDowns = 0, bristleShots = 0;
    bool broodImage = false, nestImage = false;
    bool savedImage = view && std::filesystem::exists(output / "stirring-sac.png");
    bool surge = false, bow = false, bolt = false;
    unsigned clericSpells = openingCleric, missiles = openingCasts;
    ActorId firstYoung;
    for (unsigned tick = 0; tick < 12000; ++tick) {
        const auto& events = game.update(.1, false);
        record(events);
        const auto& turn = game.combat().snapshot();
        for (const auto& e : events) {
            if (e.event == CombatEvent::ClutchHatch) {
                ++hatches;
                if (!firstHatch)
                    firstHatch = unsigned(turn.round);
                if (!firstYoung.world)
                    firstYoung = actors.id(3);
            }
            if (e.event == CombatEvent::Attack && e.enemyAttack) {
                downs += e.downed;
                if (e.enemy.slot == 3 || e.enemy.slot == 4) {
                    if (!firstAttack)
                        firstAttack = unsigned(turn.round);
                    childDamage += unsigned(e.damage);
                    bristleShots += e.kind == rules::AttackKind::Ranged;
                    if (!motherDeath && !actors.snapshot().actors[0].hasDeath) {
                        damageBeforeMotherDeath += unsigned(e.damage);
                        broodDowns += e.downed;
                    }
                }
            }
        }
        if (!motherDeath && actors.snapshot().actors[0].hasDeath)
            motherDeath = unsigned(turn.round);
        if (turn.phase == CombatPhase::Defeat || turn.phase == CombatPhase::Victory) {
            require(firstWarning == 1 && actors.snapshot().count == 5,
                    "Vault must warn in the first round and keep five slots");
            std::cout << name << ": first warning R" << firstWarning << ", first hatch R" << firstHatch
                      << ", first young attack R" << firstAttack << ", mother death R" << motherDeath
                      << ", young damage " << childDamage << " (" << damageBeforeMotherDeath
                      << " before mother death), brood downs " << broodDowns << ", bristle shots "
                      << bristleShots << ", opening shots " << shots << ", downs " << downs << ", hatches "
                      << hatches << ", surge " << surge << ", cleric casts " << clericSpells
                      << ", missile casts " << missiles << ", phase "
                      << (turn.phase == CombatPhase::Victory ? "victory" : "defeat") << '\n';
            if (burst) {
                require(!doorway || bristleShots, "Doorway opening must exercise ranged brood pressure");
                require(surge && clericSpells && missiles,
                        "Burst route did not spend standard offensive resources");
                require(firstAttack && (!motherDeath || firstAttack <= motherDeath) &&
                            (broodDowns || damageBeforeMotherDeath >= 20),
                        "Standard burst bypassed brood pressure");
                if (turn.phase == CombatPhase::Defeat)
                    return;
            }
            require(turn.phase == CombatPhase::Victory && game.objective().rewardIssued,
                    "Vault did not clear");
            if (suppress)
                require(bow && bolt && actors.snapshot().actors[1].hasDeath &&
                            actors.snapshot().actors[2].hasDeath,
                        "Failed to destroy both sacs with ranged actions");
            if (!suppress && !burst)
                require(actors.snapshot().actors[3].generation >= 2 && firstYoung.world &&
                            !actors.actor(firstYoung),
                        "Missing recurring hatch or stale identity rejection");
            game.update(.1, false);
            unsigned xpUnits = 0;
            for (const auto& ch : game.party().snapshot().characters)
                xpUnits += (ch.experience - 300) * rules::experienceScale + ch.experienceFraction;
            require(xpUnits == 200 * rules::experienceScale, "Vault reward missing or duplicated");
            return;
        }
        if (!turn.activeCharacter || !turn.awaitingAction)
            continue;
        refresh();
        const auto caster = *turn.activeCharacter;
        if (view && suppress && !nestImage && actors.snapshot().actors[1].life == ActorLife::Alive &&
            actors.snapshot().actors[2].life == ActorLife::Alive) {
            photo("vault-nest.png");
            nestImage = true;
        }
        auto act = [&](PartyAction action, ActorId target, const char* spell = nullptr,
                       rules::Feature feature = rules::Feature::None) {
            PartyCommand c;
            c.character = caster;
            c.world = turn.world;
            c.turn = turn.turn;
            c.action = action;
            c.feature = feature;
            if (target.world)
                c.target = target;
            if (spell) {
                c.spell = catalog.findSpell(spell);
                c.targetCount = 1;
                c.enemies[0] = target;
                if (std::string_view(spell) == "magic_missile")
                    c.darts[0] = 3;
            }
            if (controls.validate(c) != CommandReason::Ready)
                return false;
            require(controls.submit(c) == CommandReason::Resolved, "Validated vault action failed");
            return true;
        };
        if (view && burst && !volley && !broodImage && hatches >= 2 && !motherDeath) {
            bool mother = false, young = false;
            for (std::size_t i = 0; i < targets.count; ++i) {
                mother |= targets.ids[i].slot == 0;
                young |= actors.actor(targets.ids[i])->npc.young();
            }
            if (mother && young) {
                photo("mother-and-young.png");
                broodImage = true;
            }
        }
        if (view && caster.index == 2 && !savedImage && !motherDeath) {
            for (unsigned s : {1u, 2u}) {
                if (!game.combat().stirring(actors.id(s)))
                    continue;
                auto last = targets.ids.begin() + targets.count;
                if (std::find_if(targets.ids.begin(), last, [&](ActorId id) { return id.slot == s; }) == last)
                    continue;
                auto picked = actors.id(s);
                {
                    const auto* buffer = view->picking();
                    std::array<std::uint8_t, NpcWorld::maxInstances> mark{};
                    mark[s] = 1;
                    const auto bounds = buffer->markedBounds(mark);
                    std::optional<ActorId> hit;
                    for (int y = bounds.top; y < bounds.bottom && !hit; ++y)
                        for (int x = bounds.left; x < bounds.right && !hit; ++x)
                            if (buffer->slot(x, y) == s)
                                hit = buffer->pick(x, y);
                    require(hit && sameActor(*hit, picked), "No matching visible sac pixel");
                    picked = *hit;
                }
                controls.selectAction(PartyAction::Ranged);
                controls.selectTarget(picked);
                if (controls.enemyEligibility(actors.id(s)) != CommandReason::Ready)
                    continue;
                controls.toggleTargetPicker();
                controls.hoverEnemyTarget(actors.id(s));
                photo("stirring-sac.png");
                if (controls.snapshot().targetPicker.open)
                    controls.toggleTargetPicker();
                savedImage = true;
                break;
            }
        }
        const bool replacement = !suppress && !burst;
        if ((suppress || replacement) && caster.index >= 2) {
            bool fired = false;
            for (unsigned i : {1u, 2u}) {
                if (replacement && i == 1 && actors.snapshot().actors[3].generation < 2)
                    continue;
                if (caster.index == 2 && act(PartyAction::Ranged, actors.id(i))) {
                    bow = fired = true;
                    break;
                }
                if (caster.index == 3 && act(PartyAction::Spell, actors.id(i), "fire_bolt")) {
                    bolt = fired = true;
                    break;
                }
            }
            if (fired)
                continue;
        }
        if (caster.index == 0 && !surge &&
            act(PartyAction::Feature, {}, nullptr, rules::Feature::ActionSurge)) {
            surge = true;
            continue;
        }
        const bool sealAlive = actors.snapshot().actors[1].life == ActorLife::Alive ||
                               actors.snapshot().actors[2].life == ActorLife::Alive;
        std::array<unsigned, 3> order{0, 3, 4};
        if (suppress && sealAlive)
            order = {3, 4, 0};
        if (replacement)
            order = {3, 4, 0};
        bool acted = false;
        for (auto i : order) {
            if (actors.snapshot().actors[i].life != ActorLife::Alive)
                continue;
            const auto target = actors.id(i);
            if (replacement && actors.snapshot().actors[3].generation < 2 && i == 0 &&
                actors.snapshot().actors[0].hitPoints < 35)
                continue;
            if (replacement && actors.snapshot().actors[3].generation < 2 && caster.index < 2 && i == 0) {
                acted = act(PartyAction::Melee, target);
                if (acted)
                    break;
                continue;
            }
            if (caster.index == 1 && (act(PartyAction::Spell, target, "inflict_wounds") ||
                                      act(PartyAction::Spell, target, "guiding_bolt"))) {
                ++clericSpells;
                acted = true;
            } else if (caster.index == 3 && act(PartyAction::Spell, target, "magic_missile")) {
                ++missiles;
                acted = true;
            } else
                acted = act(PartyAction::Melee, target) || act(PartyAction::Ranged, target) ||
                        act(PartyAction::Spell, target, "fire_bolt") ||
                        act(PartyAction::Spell, target, "sacred_flame");
            if (acted)
                break;
        }
        if (!acted)
            controls.endTurn();
    }
    const auto& stuck = game.combat().snapshot();
    std::cerr << name << " timed out, phase " << unsigned(stuck.phase) << " round " << stuck.round << " turn "
              << stuck.turn << " hatches " << hatches << " hits " << firstAttack << " damage " << childDamage
              << " mother death " << motherDeath << '\n';
    for (unsigned i = 0; i < actors.snapshot().count; ++i) {
        const auto& a = actors.snapshot().actors[i];
        std::cerr << i << " hp " << a.hitPoints << " life " << unsigned(a.life) << " at " << a.npc.cell.x
                  << "," << a.npc.cell.y << '\n';
    }
    throw std::runtime_error("Vault walkthrough timed out");
}
} // namespace blobber::cave_review
