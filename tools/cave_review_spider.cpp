#include "cave_review.hpp"
#include "game/perception.hpp"
#include "platform/game_view.hpp"
#include "rlgl.h"
#include <iostream>
#include <stdexcept>

namespace blobber::cave_review {
namespace {
void require(bool success, const char* message) {
    if (!success)
        throw std::runtime_error(message);
}
} // namespace
void checkSpider(GameSession& game, const generation::CaveFloor& floor,
                 const std::vector<NpcTimings>& timings, const std::filesystem::path& output,
                 WorldRenderer* renderer, Presenter* presenter) {
    using generation::CaveSite;
    CommandController controls(game, timings);
    NpcAnimator animator(game.actors(), timings);
    std::optional<GameView> view;
    if (renderer)
        view.emplace(*renderer, &floor.layout);
    const auto spider = game.actors().id(0);
    TargetSnapshot targets;
    targets.world = spider.world;
    auto refresh = [&] {
        if (!view && game.combat().snapshot().phase == CombatPhase::Turns) {
            targets.revision = game.actors().snapshot().revision;
            targets.ids[0] = targets.candidates[0] = spider;
            const auto& actor = game.actors().snapshot().actors[0];
            const auto pose = game.movement().player;
            targets.count = targets.candidateCount =
                actor.life == ActorLife::Alive &&
                inPartyView(game.geometry(), pose.cell, pose.heading, actor.npc.cell, actor.npc.subcell);
        }
        controls.refresh(targets, game.actionBusy(), false, true);
    };
    auto capture = [&](const char* name, bool pickable = false, bool checkLegs = false) {
        if (!view)
            return;
        animator.setTime(game.timeMs(), &game.effects());
        view->update(game, animator.snapshot(), {});
        targets = view->picking()->targets();
        refresh();
        if (pickable)
            require(targets.count == 1 && sameActor(targets.ids[0], spider), "Spider unavailable to picker");
        if (checkLegs) {
            const auto& picking = *view->picking();
            std::array<std::uint8_t, NpcWorld::maxInstances> marked{};
            marked[spider.slot] = 1;
            const auto b = picking.markedBounds(marked);
            bool gap = false;
            for (int y = b.top; y < b.bottom && !gap; ++y) {
                bool body = false, open = false;
                for (int x = b.left; x < b.right; ++x) {
                    if (picking.slot(x, y) == spider.slot) {
                        gap |= open;
                        body = true;
                    } else if (body) {
                        open = true;
                    }
                }
            }
            require(gap, "Spider picker filled the space between its legs");
        }
        view->present(*presenter, game, floor.map, false, true, &controls.snapshot());
        presenter->capture(output / name);
        using GetError = unsigned (*)();
        const auto getError = reinterpret_cast<GetError>(rlGetProcAddress("glGetError"));
        require(getError && !getError(), "OpenGL error during spider capture");
    };
    game.setAnimated(false);
    const Cell arch = floor.sites[CaveSite::SpiderArch];
    if (view) {
        // Static scale reference before simulation starts. Restore the threshold for the walkthrough.
        for (auto heading : {Heading::North, Heading::South}) {
            const auto side = arch + forward(heading) * -1;
            if (!floor.map.movementBlocked(side)) {
                game.restorePose({side, heading}, false);
                capture("side.png", true, true);
                break;
            }
        }
        game.restorePose(floor.layout.spawn, false);
    }
    walkTo(game, floor.sites[CaveSite::Camp]);
    game.update(6, false);
    require(game.combat().snapshot().phase == CombatPhase::Exploration &&
                game.ai().actors[0].awareness == Awareness::Routine,
            "Camp cannot be inspected safely before the spider");
    refresh();
    const auto& loot = game.loot().snapshot();
    const auto vial = game.party().catalog().findItem("antitoxin");
    LootId pickup;
    for (unsigned i = 0; i < loot.stacks.size(); ++i)
        if (loot.stacks[i].active && loot.stacks[i].item.index == vial.index)
            pickup = {loot.world, std::uint16_t(i), loot.stacks[i].generation};
    require(bool(pickup), "Missing vial at camp");
    controls.openLoot(pickup);
    controls.lootRecipient(2);
    controls.lootHover(0);
    capture("camp.png");
    controls.lootTake(0);
    require(!game.loot().find(pickup), "Vial pickup failed");
    controls.closeOverlay();
    auto holder = game.party().snapshot().formation[2];
    std::uint16_t cell = 0;
    auto& inventory = game.party().inventory();
    for (; cell < bagCells; ++cell) {
        const auto* stack = inventory.stack(inventory.at({holder, cell}));
        if (stack && stack->item.index == vial.index)
            break;
    }
    require(cell < bagCells, "Collected vial missing from holder's bag");
    const auto handle = inventory.at({holder, cell});
    walkTo(game, arch + Cell{-3, 0}, {}, nullptr, true);
    if (realTime(game.combat().snapshot().phase)) {
        for (int i = 0; i < 3 && game.movement().player.heading != Heading::East; ++i) {
            game.move(Action::TurnRight);
            game.update(.2, false);
        }
    }
    capture("arch.png", true, true);
    if (realTime(game.combat().snapshot().phase))
        walkTo(game, arch + Cell{-1, 0}, {}, nullptr, true);
    bool applied = false, cured = false, biteCaptured = false;
    for (unsigned tick = 0; tick < 2400; ++tick) {
        const auto& events = game.update(.05, false);
        controls.record(events);
        for (const auto& event : events) {
            if (event.event == CombatEvent::Attack && event.enemyAttack && !biteCaptured) {
                capture("bite.png", true);
                biteCaptured = true;
            }
            if (event.event == CombatEvent::Effect && event.effect.kind == EffectKind::Poisoned &&
                event.effect.notice == EffectNotice::Applied && !applied) {
                applied = true;
                std::cout << "First failed venom save in round " << game.combat().snapshot().round << '\n';
                refresh();
                for (unsigned i = 0; i < rules::partySize; ++i) {
                    const auto& card = controls.snapshot().cards[i];
                    if (card.id.index == event.effect.target.character.index)
                        for (unsigned j = 0; j < card.effects.count; ++j)
                            if (card.effects.entries[j].effect == EffectKind::Poisoned)
                                controls.hover(HoverKind::Status, int(i * 6 + j), {}, 515, 125);
                }
                capture("poison.png", true);
                controls.hover(HoverKind::None);
            }
        }
        refresh();
        const auto& state = game.combat().snapshot();
        if (state.phase == CombatPhase::Victory) {
            require(biteCaptured && applied && cured, "Fight ended before the bite and cure");
            capture("victory.png");
            std::cout << "Safe camp pickup, bite, poison, antitoxin and weapon-only victory.\n";
            return;
        }
        if (state.phase == CombatPhase::Defeat) {
            for (auto i = controls.snapshot().log.size(); i > 0; --i)
                std::cerr << controls.snapshot().log.newest(i - 1) << '\n';
            throw std::runtime_error("Party lost the teaching fight");
        }
        if (!state.awaitingAction || !state.activeCharacter)
            continue;
        PartyCommand c;
        c.world = state.world;
        c.turn = state.turn;
        c.character = *state.activeCharacter;
        if (!cured && c.character.index == holder.index) {
            const auto& formation = game.party().snapshot().formation;
            for (unsigned i = 0; i < formation.size(); ++i)
                if (game.effects().has(formation[i], EffectKind::Poisoned)) {
                    controls.openInventory(holder);
                    controls.inventorySelect(cell);
                    controls.inventoryAction(InventoryAction::Use);
                    controls.inventoryAlly(i, true);
                    require(!inventory.stack(handle) &&
                                !game.effects().has(formation[i], EffectKind::Poisoned),
                            "Antitoxin did not resolve through inventory targeting");
                    cured = true;
                    capture("cure.png");
                    break;
                }
            if (cured)
                continue;
        }
        if (cured) {
            if (view) {
                animator.setTime(game.timeMs(), &game.effects());
                view->update(game, animator.snapshot(), {});
                targets = view->picking()->targets();
                refresh();
            }
            c.target = spider;
            bool attacked = false;
            for (auto action : {PartyAction::Melee, PartyAction::Ranged, PartyAction::BackMelee}) {
                c.action = action;
                if (game.combat().validate(c, game.movement().player) == CommandReason::Ready) {
                    require(controls.submit(c) == CommandReason::Resolved, "Weapon attack was rejected");
                    attacked = true;
                    break;
                }
            }
            if (attacked)
                continue;
        }
        controls.endTurn();
    }
    throw std::runtime_error("Spider walkthrough timed out");
}
} // namespace blobber::cave_review
