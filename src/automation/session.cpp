#include "automation/session.hpp"
#include "world/environment_maps.hpp"
#include <stdexcept>
namespace blobber::automation {
namespace {
std::optional<generation::GeneratedFloor> prepare(const Scenario& s, Mode mode) {
    if (const auto* generated = std::get_if<GeneratedScenario>(&s)) {
        if (generated->forceFallback && mode != Mode::Diagnostic)
            throw std::runtime_error("Forced fallback requires diagnostic mode");
        auto result =
            generation::generate(generated->generation, generation::templates(), generated->forceFallback);
        if (!result)
            throw std::runtime_error(std::string("Generation failed: ") +
                                     generation::errorName(result.diagnostics.failure.code));
        return std::move(result.floor);
    }
    return std::nullopt;
}
} // namespace
Session::Session(const rules::Catalog& catalog, const NpcDefinitions& definitions, const Scenario& scenario,
                 Mode mode, VisibilityFactory factory, void* context)
    : scenario_(scenario), factory_(factory), factoryContext_(context), mode_(mode) {
    if (const auto* campaign = std::get_if<CampaignScenario>(&scenario_)) {
        campaign_ = std::make_unique<CampaignRun>(catalog, startingParty(catalog, campaign->party),
                                                  definitions.types, definitions.timings, campaign->seed);
    } else if (const auto* review = std::get_if<RendererFixtureScenario>(&scenario_)) {
        arenaMap_.emplace(rendererHeightMap());
        map_ = &*arenaMap_;
        placements_.emplace(*map_, Cell{3, 5}, definitions.types,
                            std::vector<NpcInstance>{{{0}, {3, 2}, {0, 0}, Heading::South}});
        const auto fixtures = rendererHeightFixtures();
        RecoveryLevel recovery;
        recovery.groundCount = 1;
        recovery.ground[0] = {{3, 3},
                              catalog.item(catalog.findItem("dagger")) ? catalog.findItem("dagger")
                                                                       : rules::ItemId{0},
                              1};
        game_ = std::make_unique<GameSession>(*map_, catalog, startingParty(catalog, review->party),
                                              *placements_, review->seed, Player{{3, 5}, Heading::North},
                                              false, definitions.timings, recovery, fixtures);
    } else {
        generated_ = prepare(scenario_, mode);
        if (!generated_)
            arenaMap_.emplace(arenaMap());
        map_ = generated_ ? &generated_->map : &*arenaMap_;
        if (generated_)
            population_ = generation::preparePopulation(*generated_, catalog, definitions.types);
        placements_ = generated_ ? std::move(population_->placements)
                                 : arenaPlacements(*map_, catalog, std::get<ArenaScenario>(scenario_),
                                                   definitions.types);
        game_ = std::make_unique<GameSession>(
            *map_, catalog,
            generated_ ? startingParty(catalog, std::get<GeneratedScenario>(scenario_).party)
                       : arenaParty(catalog, std::get<ArenaScenario>(scenario_)),
            *placements_, generated_ ? generated_->runtimeSeed : std::get<ArenaScenario>(scenario_).seed,
            generated_ ? generated_->layout.spawn : arenaSpawn(), true, definitions.timings,
            population_ ? population_->recovery : RecoveryLevel{},
            generated_ ? generated_->layout.fixtures : FloorFixtures{},
            population_ ? population_->objective : FloorObjectiveDefinition{});
    }
    if (mode_ == Mode::Visual) {
        if (!factory)
            throw std::runtime_error("Visual backend unavailable in this build");
        visibility_ = factory(context, map(), game());
        if (!visibility_)
            throw std::runtime_error("Visual backend construction failed");
    }
}
Session::Session(const rules::Catalog&, const NpcDefinitions&, std::unique_ptr<CampaignRun> run, Mode mode,
                 VisibilityFactory factory, void* context)
    : resumed_(true), scenario_(CampaignScenario{}), campaign_(std::move(run)), factory_(factory),
      factoryContext_(context), mode_(mode) {
    if (!campaign_)
        throw std::invalid_argument("Missing restored campaign");
    if (mode_ == Mode::Visual) {
        if (!factory_)
            throw std::runtime_error("Visual backend unavailable in this build");
        visibility_ = factory_(factoryContext_, map(), game());
        if (!visibility_)
            throw std::runtime_error("Visual backend construction failed");
    }
}
void Session::observe(Observation& out, bool diagnostic) {
    out = {};
    out.session = &game();
    out.map = &map();
    out.diagnostic = diagnostic || mode_ == Mode::Diagnostic;
    if (visibility_)
        out.targets = visibility_->sample(map(), game());
    else {
        out.targets.world = game().actors().snapshot().world;
        out.targets.revision = game().actors().snapshot().revision;
        if (!game().actionBusy())
            for (std::size_t i = 0; i < game().actors().snapshot().count; ++i)
                if (game().actors().snapshot().actors[i].life == ActorLife::Alive)
                    out.targets.ids[out.targets.count++] = game().actors().id(i);
    }
    for (std::size_t i = 0; i < out.targets.count; ++i)
        out.visible[out.targets.ids[i].slot] = true;
    for (std::size_t i = 0; i < lootCapacity; ++i) {
        const auto& loot = game().loot().snapshot().stacks[i];
        out.lootVisible[i] =
            loot.active && (out.diagnostic || loot.cell == game().movement().player.cell ||
                            (visibility_ && visibility_->lootVisible({game().loot().snapshot().world,
                                                                      std::uint16_t(i), loot.generation})));
    }
}
bool Session::result() const noexcept {
    const auto p = game().combat().snapshot().phase;
    return terminal() || p == CombatPhase::Victory || p == CombatPhase::Defeat;
}
bool Session::terminal() const noexcept {
    return (campaign_ && campaign_->status() != CampaignStatus::Active) ||
           game().combat().snapshot().phase == CombatPhase::Defeat ||
           (arena() && game().combat().snapshot().phase == CombatPhase::Victory &&
            arenaCleared(game().actors().snapshot()));
}
bool Session::decision() const noexcept {
    return result() || (!game().opening() && game().combat().snapshot().awaitingAction) ||
           (realTime(game().combat().snapshot().phase) && !game().actionBusy());
}
const CombatEvents& Session::tick() noexcept {
    ++ticks_;
    return campaign_ ? campaign_->update(double(tickMs) / 1000, false)
                     : game().update(double(tickMs) / 1000, false);
}
ActionResult Session::action(const ActionRequest& r, bool preview) {
    if (terminal())
        return {false, "Terminal"};
    const auto okay = [](bool b, const char* reason) -> ActionResult { return {b, b ? "Ready" : reason}; };
    switch (r.operation) {
    case Operation::Combat: {
        if (game().actionBusy())
            return {false, game().opening() ? "Wait until the door opens." : "Moving"};
        if (mode_ == Mode::Visual) {
            observe(scratch_);
            const auto visible = targetVisibility(r.combat, game().party().catalog(), scratch_.targets,
                                                  game().actors().snapshot());
            if (visible != CommandReason::Ready)
                return {false, commandMessage(visible)};
        }
        auto reason = game().combat().validate(r.combat, game().movement().player);
        if (reason != CommandReason::Ready)
            return {false, commandMessage(reason)};
        if (preview)
            return {true, "Ready"};
        reason = game().submit(r.combat).reason;
        return {reason == CommandReason::Resolved, commandMessage(reason)};
    }
    case Operation::Move: {
        if (game().actionBusy())
            return {false, game().opening() ? "Wait until the door opens." : "Moving"};
        if (!realTime(game().combat().snapshot().phase) || !game().effects().mobileParty())
            return {false, "Cannot move in this phase or condition"};
        auto player = game().movement().player;
        if (!tryAction(game().geometry(), player, r.movement, &game().actors()))
            return {false, "Blocked"};
        if (!preview)
            game().move(r.movement);
        return {true, "Ready"};
    }
    case Operation::Inventory: {
        auto error = game().inventoryAccess(r.inventory);
        if (error != InventoryError::None)
            return {false, inventoryMessage(error)};
        const auto res = preview ? game().party().previewInventoryChange(r.inventory)
                                 : game().inventoryChange(r.inventory);
        return okay(bool(res), inventoryMessage(res.error));
    }
    case Operation::Drop: {
        const auto res = game().dropItem(r.item, r.inventory.revision, preview);
        return okay(bool(res), res.reason);
    }
    case Operation::Pickup: {
        if (mode_ == Mode::Visual) {
            observe(scratch_);
            if (r.pickup.loot.slot >= lootCapacity || !scratch_.lootVisible[r.pickup.loot.slot])
                return {false, "Not visible"};
        }
        const auto res = game().pickup(r.pickup, preview);
        return okay(bool(res), res.reason);
    }
    case Operation::Rest: {
        const auto p = game().previewRest(r.rest);
        if (!p || preview)
            return okay(bool(p), restMessage(p.error));
        const auto res = game().rest(r.rest);
        return okay(bool(res), restMessage(res.error));
    }
    case Operation::Advancement: {
        const auto res = preview ? game().previewAdvancement(r.advancement) : game().advance(r.advancement);
        return okay(bool(res), advancementMessage(res.error));
    }
    case Operation::Interact: {
        const auto* fixture = game().fixtures().find(r.fixture.id);
        if (campaign_ && fixture && CampaignRun::stairs(fixture->kind)) {
            return travel(campaign_->request(r.fixture), preview);
        }
        const auto result = game().interact(r.fixture, preview);
        return {result.accepted, result.reason};
    }
    case Operation::Travel:
        if (!campaign_)
            return {false, "Travel requires a campaign"};
        return travel(r.travel, preview);
    case Operation::Continue:
        if (game().combat().snapshot().phase != CombatPhase::Victory)
            return {false, "No partial victory"};
        if (!preview) {
            game().combat().continueExploring();
            game().clearPending();
        }
        return {true, "Ready"};
    }
    return {false, "Unsupported operation"};
}
ActionResult Session::travel(const TravelRequest& request, bool preview) {
    const auto check = campaign_->previewTravel(request);
    if (!check || preview)
        return {check.accepted, check.reason};
    try {
        auto ticket = campaign_->prepareTravel(request);
        auto view = visibility_ ? factory_(factoryContext_, campaign_->location(ticket.destination()).map(),
                                           ticket.game())
                                : nullptr;
        if (visibility_ && !view)
            return {false, "Destination view preparation failed"};
        if (!campaign_->commitTravel(ticket))
            return {false, "Stale travel"};
        visibility_.swap(view); // old view dies before ticket's old session
        return {true, "Travel completed"};
    } catch (const std::exception&) {
        return {false, "Destination preparation failed; current floor and supplies retained"};
    }
}
} // namespace blobber::automation
