#include "automation/session.hpp"
#include <stdexcept>
namespace blobber::automation {
Session::Session(const rules::Catalog& catalog, const NpcDefinitions& definitions,
                 const ArenaScenario& scenario, Mode mode, VisibilityFactory factory, void* context)
    : scenario_(scenario), map_(arenaMap()),
      placements_(arenaPlacements(map_, catalog, scenario_, definitions.types)),
      game_(map_, catalog, arenaParty(catalog, scenario_), placements_, scenario_.seed, arenaSpawn(), true,
            definitions.timings),
      mode_(mode) {
    if (mode_ == Mode::Visual) {
        if (!factory)
            throw std::runtime_error("Visual backend unavailable in this build");
        visibility_ = factory(context, map_, game_);
        if (!visibility_)
            throw std::runtime_error("Visual backend construction failed");
    }
}
void Session::observe(Observation& out, bool diagnostic) {
    out = {};
    out.session = &game_;
    out.map = &map_;
    out.diagnostic = diagnostic || mode_ == Mode::Diagnostic;
    if (visibility_)
        out.targets = visibility_->sample(map_, game_);
    else {
        out.targets.world = game_.actors().snapshot().world;
        out.targets.revision = game_.actors().snapshot().revision;
        if (!game_.movement().transition.active())
            for (std::size_t i = 0; i < game_.actors().snapshot().count; ++i)
                if (game_.actors().snapshot().actors[i].life == ActorLife::Alive)
                    out.targets.ids[out.targets.count++] = game_.actors().id(i);
    }
    for (std::size_t i = 0; i < out.targets.count; ++i)
        out.visible[out.targets.ids[i].slot] = true;
    for (std::size_t i = 0; i < lootCapacity; ++i) {
        const auto& loot = game_.loot().snapshot().stacks[i];
        out.lootVisible[i] =
            loot.active && (out.diagnostic || loot.cell == game_.movement().player.cell ||
                            (visibility_ && visibility_->lootVisible({game_.loot().snapshot().world,
                                                                      std::uint16_t(i), loot.generation})));
    }
}
bool Session::result() const noexcept {
    const auto p = game_.combat().snapshot().phase;
    return p == CombatPhase::Victory || p == CombatPhase::Defeat;
}
bool Session::terminal() const noexcept {
    return game_.combat().snapshot().phase == CombatPhase::Defeat ||
           (game_.combat().snapshot().phase == CombatPhase::Victory &&
            arenaCleared(game_.actors().snapshot()));
}
bool Session::decision() const noexcept {
    return result() || game_.combat().snapshot().awaitingAction ||
           (realTime(game_.combat().snapshot().phase) && !game_.movement().transition.active());
}
const CombatEvents& Session::tick() noexcept {
    ++ticks_;
    return game_.update(double(tickMs) / 1000, false);
}
ActionResult Session::action(const ActionRequest& r, bool preview) {
    if (terminal())
        return {false, "Terminal"};
    const auto okay = [](bool b, const char* reason) -> ActionResult { return {b, b ? "Ready" : reason}; };
    switch (r.operation) {
    case Operation::Combat: {
        if (game_.movement().transition.active())
            return {false, "Moving"};
        if (mode_ == Mode::Visual) {
            observe(scratch_);
            const auto visible = targetVisibility(r.combat, game_.party().catalog(), scratch_.targets,
                                                  game_.actors().snapshot());
            if (visible != CommandReason::Ready)
                return {false, commandMessage(visible)};
        }
        auto reason = game_.combat().validate(r.combat, game_.movement().player);
        if (reason != CommandReason::Ready)
            return {false, commandMessage(reason)};
        if (preview)
            return {true, "Ready"};
        reason = game_.submit(r.combat).reason;
        return {reason == CommandReason::Resolved, commandMessage(reason)};
    }
    case Operation::Move: {
        if (game_.movement().transition.active())
            return {false, "Moving"};
        if (!realTime(game_.combat().snapshot().phase) || !game_.effects().mobileParty())
            return {false, "Cannot move in this phase or condition"};
        auto player = game_.movement().player;
        if (!tryAction(map_, player, r.movement, &game_.actors()))
            return {false, "Blocked"};
        if (!preview)
            game_.move(r.movement);
        return {true, "Ready"};
    }
    case Operation::Inventory: {
        auto error = game_.inventoryAccess(r.inventory);
        if (error != InventoryError::None)
            return {false, inventoryMessage(error)};
        const auto res =
            preview ? game_.party().previewInventoryChange(r.inventory) : game_.inventoryChange(r.inventory);
        return okay(bool(res), inventoryMessage(res.error));
    }
    case Operation::Drop: {
        const auto res = game_.dropItem(r.item, r.inventory.revision, preview);
        return okay(bool(res), res.reason);
    }
    case Operation::Pickup: {
        if (mode_ == Mode::Visual) {
            observe(scratch_);
            if (r.pickup.loot.slot >= lootCapacity || !scratch_.lootVisible[r.pickup.loot.slot])
                return {false, "Not visible"};
        }
        const auto res = game_.pickup(r.pickup, preview);
        return okay(bool(res), res.reason);
    }
    case Operation::Rest: {
        const auto p = game_.previewRest(r.rest);
        if (!p || preview)
            return okay(bool(p), restMessage(p.error));
        const auto res = game_.rest(r.rest);
        return okay(bool(res), restMessage(res.error));
    }
    case Operation::Advancement: {
        const auto res = preview ? game_.previewAdvancement(r.advancement) : game_.advance(r.advancement);
        return okay(bool(res), advancementMessage(res.error));
    }
    case Operation::Continue:
        if (game_.combat().snapshot().phase != CombatPhase::Victory)
            return {false, "No partial victory"};
        if (!preview) {
            game_.combat().continueExploring();
            game_.clearPending();
        }
        return {true, "Ready"};
    }
    return {false, "Unsupported operation"};
}
} // namespace blobber::automation
