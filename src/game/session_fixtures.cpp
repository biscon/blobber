#include "game/campaign.hpp"
#include "game/session.hpp"
namespace blobber {
bool GameSession::completeFixture(unsigned slot) noexcept {
    if (!floor_.complete(slot))
        return false;
    const auto kind = floor_.definition().entries[slot].kind;
    if (kind == FixtureKind::Door || kind == FixtureKind::GuardianGate) {
        doors_.now = timeMs_;
        clearPending();
        if (movement().animated && timeMs_ <= UINT64_MAX - DoorSnapshot::durationMs) {
            doors_.until[slot] = timeMs_ + DoorSnapshot::durationMs;
            ++doors_.revision;
        }
    }
    return true;
}
void GameSession::discover() noexcept {
    const auto pose = movement().player;
    if (movement().transition.active() ||
        (discoveredReady_ && pose == discoveredPose_ && discoveredFixtures_ == floor_.revision()))
        return;
    floor_.discover(pose.cell, pose.heading);
    discoveredReady_ = true;
    discoveredPose_ = pose;
    discoveredFixtures_ = floor_.revision();
    if (objective_.count && !objective_.known &&
        (floor_.known(objectiveDefinition_.approachDoor) || floor_.known(objectiveDefinition_.gate))) {
        objective_.known = true;
        ++objective_.revision;
    }
}
FixtureRequest GameSession::nearbyFixture() const noexcept {
    const auto pose = movement().player;
    // At-feet keys take precedence over the faced fixture.
    const auto here = floor_.at(pose.cell);
    if (here >= 0 && floor_.definition().entries[here].kind == FixtureKind::Key && !floor_.completed(here))
        return {floor_.id(here), floor_.revision()};
    auto i = floor_.at(pose.cell + forward(pose.heading));
    if (i < 0)
        i = here;
    if (i >= 0 && !fixtureReachable(floor_.definition().entries[i], pose.cell, pose.heading))
        i = -1;
    if (i >= 0 && floor_.definition().entries[i].kind == FixtureKind::Key && floor_.completed(i))
        i = -1;
    return {i < 0 ? FixtureId{} : floor_.id(unsigned(i)), floor_.revision()};
}
FixtureResult GameSession::interact(const FixtureRequest& r, bool preview) noexcept {
    if (!runActive())
        return {false, false, "Campaign has ended."};
    if (!preview)
        if (mutation_ != UINT64_MAX)
            ++mutation_;
    if (!preview)
        events_.clear();
    const auto* f = floor_.find(r.id);
    // Unknown IDs reveal neither existence nor kind, even to a probing caller.
    if (!f || (floor_.definition().discovery && !floor_.known(r.id.slot)))
        return {};
    if (r.revision != floor_.revision())
        return {false, false, "Stale fixture request."};
    if (actionBusy())
        return {false, false, opening() ? "Wait until the door opens." : "Wait until movement finishes."};
    const auto pose = movement().player;
    if (!fixtureReachable(*f, pose.cell, pose.heading))
        return {false, false, "Move closer and face the fixture."};
    if (!combat_.settledRecovery(pose, timeMs_) || effects_.area(pose.cell))
        return {false, false, "Fixtures require settled safe exploration."};
    bool able = false;
    for (std::uint16_t i = 0; i < 4; ++i)
        able |= effects_.canAct(rules::CharacterId{i});
    if (!able)
        return {false, false, "An able companion is required."};
    if (campaign_ && CampaignRun::stairs(f->kind))
        return campaign_->previewTravel(campaign_->request(r));
    if (f->kind == FixtureKind::GuardianGate)
        return {objective_.completed, false,
                objective_.completed ? "Guardians defeated. Gate open; rewards beyond."
                                     : "Defeat all designated guardians to open this gate."};
    if (f->kind != FixtureKind::Door && f->kind != FixtureKind::Key)
        return {false, false,
                objective_.completed ? "Objective secured. Travel is not available yet."
                                     : "Travel is not available yet."};
    if (floor_.completed(r.id.slot))
        return {true, false, "Already completed."};
    if (f->kind == FixtureKind::Door && f->key && !(party_.questKeys_ & (1u << f->key)))
        return {false, false,
                f->key == 2 ? "Requires the Bronze seal key." : "Requires the Silver seal key."};
    if (floor_.revision() == UINT64_MAX || floor_.geometryRevision() == UINT64_MAX)
        return {false, false, "Fixture revision exhausted."};
    if (!preview) {
        events_.clear();
        if (!completeFixture(r.id.slot))
            return {false, false, "Fixture unavailable."};
        if (f->kind == FixtureKind::Key)
            party_.questKeys_ |= std::uint8_t(1u << f->key);
        movement_.clearPending();
        detectContact();
        discover();
    }
    if (objective_.count && r.id.slot == objectiveDefinition_.approachDoor)
        return {true, !preview,
                floor_.definition().depth == 3 ? "Final guardians beyond! Combat prevents movement."
                                               : "Guardians beyond! Combat prevents movement."};
    return {true, !preview,
            f->kind == FixtureKind::Door ? (preview ? "Open permanently." : "Door opened permanently.")
                                         : (preview ? "Collect quest key." : "Quest key collected.")};
}
} // namespace blobber
