#include "game/campaign.hpp"
#include <algorithm>
#include <stdexcept>
namespace blobber {
bool GameSession::runActive() const noexcept {
    return !campaign_ || (campaign_->status() == CampaignStatus::Active &&
                          !(campaign_->activeLocation() == 3 && objective_.completed));
}
namespace {
std::uint64_t rebase(std::uint64_t deadline, std::uint64_t from, std::uint64_t to) {
    if (!deadline)
        return 0;
    const auto remaining = deadline > from ? deadline - from : 0;
    if (to > UINT64_MAX - remaining)
        throw std::invalid_argument("Travel clock exhausted");
    return to + remaining;
}
bool portable(const EffectInstance& e) noexcept {
    if (e.kind == EffectKind::Web || e.kind == EffectKind::SpiritualWeapon)
        return false;
    for (std::size_t i = 0; i < e.count; ++i)
        if (e.targets[i].npc)
            return false;
    return true;
}
} // namespace
CampaignRun::CampaignRun(const rules::Catalog& catalog, const rules::PresetParty& starting,
                         const std::vector<std::string>& types, const std::vector<NpcTimings>& timings,
                         std::uint32_t seed)
    : party_(catalog, starting), timings_(timings) {
    auto& hub = locations_[0];
    hub.authored.emplace(std::vector<std::string>{"#########", "#.......#", "#.......#", "#.......#",
                                                  "#.......#", "#.......#", "#.......#", "#.......#",
                                                  "#########"});
    hub.entry = {{4, 4}, Heading::East};
    hub.exit = hub.entry;
    hub.fixtures.count = 1;
    hub.fixtures.entries[0] = {{5, 4}, Heading::East, FixtureKind::Downstairs};
    hub.fixtures.landingCount = 1;
    hub.fixtures.landings[0] = hub.entry.cell;
    hub.fixtures.npcExcluded.set(4 * 9 + 4);
    hub.fixtures.npcExcluded.set(4 * 9 + 5);
    hub.recovery.sanctuaries[0] = hub.entry.cell;
    hub.recovery.sanctuaryCount = 1;
    hub.placements = NpcWorld(hub.map(), hub.entry.cell, types, {});
    hub.seed = seed;
    hub.visited = true;
    for (unsigned depth = 1; depth <= 3; ++depth) {
        auto result = generation::generate({seed, depth});
        if (!result)
            throw std::runtime_error("Campaign generation failed");
        auto& floor = locations_[depth];
        floor.generated = std::move(result.floor);
        auto population = generation::preparePopulation(*floor.generated, catalog, types);
        floor.placements = std::move(population.placements);
        floor.recovery = population.recovery;
        floor.objective = population.objective;
        floor.fixtures = floor.generated->layout.fixtures;
        floor.entry = floor.generated->layout.spawn;
        floor.exit.cell = floor.fixtures.landings[1];
        // Objective-side arrival faces back into the stairs room.
        for (unsigned i = 0; i < floor.fixtures.count; ++i)
            if (floor.fixtures.entries[i].cell == floor.exit.cell)
                floor.exit.heading = Heading((unsigned(floor.fixtures.entries[i].facing) + 2) % 4);
        floor.seed = floor.generated->runtimeSeed;
    }
    active_ = prepare(0, hub.entry);
    active_->campaign_ = this;
    active_->discover();
}
std::unique_ptr<GameSession> CampaignRun::prepare(unsigned index, Player arrival) {
    const auto& l = locations_[index];
    auto game = std::make_unique<GameSession>(l.map(), party_, *l.placements, l.seed, arrival, timings_,
                                              l.recovery, l.fixtures, l.objective);
    if (l.saved)
        game->importFloor(*l.saved);
    return game;
}
std::uint64_t CampaignRun::gameplayMs() const noexcept {
    const auto delta = game().combat().snapshot().gameplayMs - baseClock_;
    return elapsed_ > UINT64_MAX - delta ? UINT64_MAX : elapsed_ + delta;
}
CampaignStatus CampaignRun::status() const noexcept {
    bool conscious = false;
    for (const auto& member : party_.snapshot().characters)
        conscious |= member.life == rules::Life::Conscious;
    if (!conscious || game().combat().snapshot().phase == CombatPhase::Defeat)
        return CampaignStatus::Defeat;
    if (location_ == 3 && game().objective().completed && game().combat().settledActions(game().timeMs()))
        return CampaignStatus::Victory;
    return CampaignStatus::Active;
}
const char* CampaignRun::name() const noexcept {
    constexpr const char* names[]{"Sanctuary hub", "Dungeon floor 1", "Dungeon floor 2", "Dungeon floor 3"};
    return names[location_];
}
TravelRequest CampaignRun::request(FixtureRequest fixture) const noexcept {
    TravelRequest r;
    r.revision = revision_;
    r.world = game().actors().snapshot().world;
    r.fixture = fixture;
    return r;
}
TravelRequest CampaignRun::recallRequest() const noexcept {
    auto r = request({});
    r.kind = TravelKind::Recall;
    r.supply = party_.campSupplies().supply;
    r.inventoryRevision = party_.inventory().revision();
    return r;
}
TravelRequest CampaignRun::waypointRequest(unsigned depth) const noexcept {
    auto r = request(game().nearbyFixture());
    r.kind = TravelKind::Waypoint;
    r.depth = depth;
    return r;
}
FixtureResult CampaignRun::previewTravel(const TravelRequest& r) const noexcept {
    if (status() != CampaignStatus::Active || !game().runActive())
        return {false, false, "Campaign has ended."};
    if (r.revision != revision_ || revision_ == UINT64_MAX || r.world != game().actors().snapshot().world)
        return {false, false, "Stale campaign travel request."};
    const auto& g = game();
    if (r.kind == TravelKind::Recall) {
        if (!location_)
            return {false, false, "Already in the sanctuary hub."};
        if (const auto* reason = g.travelSafety())
            return {false, false, reason};
        if (r.inventoryRevision != party_.inventory().revision())
            return {false, false, "Inventory changed; reopen Travel."};
        const auto* stack = party_.inventory().stack(r.supply);
        if (!stack || party_.inventory().locate(r.supply).cell >= bagCells ||
            party_.catalog().item(stack->item)->kind != rules::ItemKind::Supply ||
            party_.inventory().revision() == UINT64_MAX)
            return {false, false, "Recall requires one camp kit."};
        return {true, false, "Recall to the sanctuary hub: one camp kit."};
    }
    if (r.kind != TravelKind::Stairs && r.kind != TravelKind::Waypoint)
        return {false, false, "Invalid travel kind."};
    if (r.kind == TravelKind::Waypoint &&
        (location_ != 0 || r.depth < 1 || r.depth > 3 || !locations_[r.depth].visited))
        return {false, false, "Return requires the hub entrance and an unlocked entry."};
    const auto* f = g.fixtures().find(r.fixture.id);
    if (!f || !stairs(f->kind) || r.fixture.revision != g.fixtures().revision() ||
        (g.fixtures().definition().discovery && !g.fixtures().known(r.fixture.id.slot)))
        return {false, false, "Stale or unknown stairs."};
    if (!fixtureReachable(*f, g.movement().player.cell, g.movement().player.heading))
        return {false, false, "Move closer and face the stairs."};
    if (const auto* reason = g.travelSafety())
        return {false, false, reason};
    if (r.kind == TravelKind::Waypoint)
        return {f->kind == FixtureKind::Downstairs, false, "Return to the unlocked floor entry for free."};
    if (f->kind == FixtureKind::Downstairs) {
        if (location_ == 3)
            return {false, false, "No deeper floor."};
        if (location_ && !g.objective().completed)
            return {false, false, "Defeat the designated guardians before descending."};
        return {true, false,
                location_ == 0   ? "Enter floor 1."
                : location_ == 1 ? "Descend to floor 2."
                                 : "Descend to floor 3."};
    }
    if (!location_)
        return {false, false, "No upstairs here."};
    return {true, false,
            location_ == 1   ? "Return to the sanctuary hub."
            : location_ == 2 ? "Ascend to floor 1."
                             : "Ascend to floor 2."};
}
CampaignRun::PreparedTravel CampaignRun::prepareTravel(const TravelRequest& r) {
    const auto check = previewTravel(r);
    if (!check)
        throw std::invalid_argument(check.reason);
    PreparedTravel ticket;
    ticket.request = r;
    ticket.from = location_;
    const bool stair = r.kind == TravelKind::Stairs;
    const bool down = stair && game().fixtures().find(r.fixture.id)->kind == FixtureKind::Downstairs;
    ticket.to = stair ? (down ? location_ + 1 : location_ - 1) : (r.kind == TravelKind::Recall ? 0 : r.depth);
    ticket.landing = stair && !down && ticket.to != 0 ? 1 : 0;
    ticket.arrival = ticket.landing ? locations_[ticket.to].exit : locations_[ticket.to].entry;
    if (r.kind == TravelKind::Recall) {
        ticket.chargedInventory.emplace(party_.inventory());
        if (!party_.stageCampSupply(r.supply, r.inventoryRevision, *ticket.chargedInventory, ticket.supplier,
                                    ticket.supplierStats))
            throw std::invalid_argument("Camp kit could not be staged; current floor retained.");
    }
    ticket.mutation = game().mutation_;
    ticket.actorRevision = game().actors().snapshot().revision;
    ticket.partyStamp = partyStamp();
    ticket.source = std::make_unique<FloorState>();
    game().exportFloor(*ticket.source);
    ticket.session = prepare(ticket.to, ticket.arrival);
    ticket.session->enableDevelopment(game().development());
    ticket.session->setAnimated(game().movement().animated);
    auto destination = std::make_unique<FloorState>();
    ticket.session->exportFloor(*destination);
    const auto from = ticket.source->effects.gameplayMs, to = destination->effects.gameplayMs;
    auto& local = ticket.source->effects;
    auto& incoming = destination->effects;
    for (auto& e : local.instances) {
        if (!e.active || !portable(e))
            continue;
        auto free = std::find_if(incoming.instances.begin(), incoming.instances.end(),
                                 [](const auto& value) { return !value.active; });
        if (free == incoming.instances.end() || incoming.nextSerial == UINT64_MAX)
            throw std::invalid_argument("Destination effect capacity exhausted");
        *free = e;
        free->id.serial = incoming.nextSerial++;
        free->expiresMs = rebase(e.expiresMs, from, to);
        free->retryMs = rebase(e.retryMs, from, to);
        free->readyMs = rebase(e.readyMs, from, to);
        e.active = false;
    }
    incoming.casterTurns = local.casterTurns;
    for (unsigned i = 0; i < 4; ++i) {
        incoming.conditions[i] = local.conditions[i];
        for (auto& c : incoming.conditions[i])
            if (c.active)
                c.untilMs = rebase(c.untilMs, from, to);
        local.conditions[i] = {};
        ticket.temporaryUntil[i] = rebase(party_.state_.characters[i].temporaryUntilMs, from, to);
    }
    const auto transfer = [&](auto& dest, const auto& source) {
        for (unsigned i = 0; i < 4; ++i)
            dest[i] = rebase(source[i], from, to);
    };
    auto& c = destination->combat;
    const auto& old = ticket.source->combat;
    transfer(c.partyReady, old.partyReady);
    transfer(c.saveDue, old.saveDue);
    transfer(c.defendDue, old.defendDue);
    transfer(c.sneakReady, old.sneakReady);
    transfer(c.cunningReady, old.cunningReady);
    transfer(c.extraUntil, old.extraUntil);
    c.extraAction = old.extraAction;
    c.processedCasterRound = old.processedCasterRound;
    c.state.defending = old.state.defending;
    c.state.spent = old.state.spent;
    c.state.cooldownMs = old.state.cooldownMs;
    ticket.session->importFloor(*destination);
    const auto safe = ticket.session->landingSafety(ticket.landing);
    if (!safe.safe)
        throw std::invalid_argument(safe.reason);
    ticket.session->discover();
    return ticket;
}
bool CampaignRun::commitTravel(PreparedTravel& ticket) noexcept {
    if (!ticket.session || !ticket.source || ticket.from != location_ ||
        ticket.mutation != game().mutation_ || ticket.actorRevision != game().actors().snapshot().revision ||
        ticket.partyStamp != partyStamp() || !previewTravel(ticket.request) ||
        !(ticket.session->movement().player == ticket.arrival) ||
        !ticket.session->landingSafety(ticket.landing).safe)
        return false;
    elapsed_ = gameplayMs();
    if (ticket.chargedInventory) {
        party_.inventory_ = *ticket.chargedInventory;
        party_.state_.characters[ticket.supplier.index].stats = ticket.supplierStats;
    }
    for (unsigned i = 0; i < 4; ++i)
        party_.state_.characters[i].temporaryUntilMs = ticket.temporaryUntil[i];
    locations_[location_].saved.swap(ticket.source);
    active_.swap(ticket.session);
    location_ = ticket.to;
    if (ticket.request.kind == TravelKind::Stairs)
        locations_[location_].visited = true;
    baseClock_ = game().combat().snapshot().gameplayMs;
    ++revision_;
    game().campaign_ = this;
    game().clearPending();
    return true;
}
std::array<std::uint64_t, 100> CampaignRun::partyStamp() const noexcept {
    std::array<std::uint64_t, 100> out{};
    std::size_t n = 0;
    out[n++] = party_.inventory().revision();
    out[n++] = party_.preparationRevision();
    out[n++] = party_.progressionRevision();
    out[n++] = party_.questKeys();
    for (unsigned i = 0; i < 4; ++i) {
        const auto& c = *party_.character({std::uint16_t(i)});
        for (auto value :
             {std::uint64_t(c.hitPoints), std::uint64_t(c.life), std::uint64_t(c.temporaryHitPoints),
              std::uint64_t(c.successes), std::uint64_t(c.failures), std::uint64_t(c.mana),
              std::uint64_t(c.availableHitDice), std::uint64_t(c.experience),
              std::uint64_t(c.experienceFraction), std::uint64_t(c.secondWind),
              std::uint64_t(c.arcaneRecovery), std::uint64_t(c.actionSurge), std::uint64_t(c.channelDivinity),
              std::uint64_t(c.aidBonus), c.temporaryUntilMs,
              std::uint64_t(party_.snapshot().formation[i].index)})
            out[n++] = value;
    }
    return out;
}
const CombatEvents& CampaignRun::update(double seconds, bool paused) noexcept {
    return game().update(seconds, paused || status() != CampaignStatus::Active);
}
} // namespace blobber
