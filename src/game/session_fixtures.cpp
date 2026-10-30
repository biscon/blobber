#include "game/session.hpp"
namespace blobber {
void GameSession::discover() noexcept {
    const auto pose = movement().player;
    if (movement().transition.active() ||
        (discoveredReady_ && pose == discoveredPose_ && discoveredFixtures_ == floor_.revision()))
        return;
    floor_.discover(pose.cell, pose.heading);
    discoveredReady_ = true;
    discoveredPose_ = pose;
    discoveredFixtures_ = floor_.revision();
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
    if (!preview)
        events_.clear();
    const auto* f = floor_.find(r.id);
    // Unknown IDs reveal neither existence nor kind, even to a probing caller.
    if (!f || (floor_.definition().discovery && !floor_.known(r.id.slot)))
        return {};
    if (r.revision != floor_.revision())
        return {false, false, "Stale fixture request."};
    if (movement().transition.active())
        return {false, false, "Wait until movement finishes."};
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
    if (f->kind != FixtureKind::Door && f->kind != FixtureKind::Key)
        return {false, false, "Travel and floor objectives are not available yet."};
    if (floor_.completed(r.id.slot))
        return {true, false, "Already completed."};
    if (f->kind == FixtureKind::Door && f->key && !(party_.questKeys_ & (1u << f->key)))
        return {false, false,
                f->key == 2 ? "Requires the Bronze seal key." : "Requires the Silver seal key."};
    if (floor_.revision() == UINT64_MAX || floor_.geometryRevision() == UINT64_MAX)
        return {false, false, "Fixture revision exhausted."};
    if (!preview) {
        events_.clear();
        if (!floor_.complete(r.id.slot))
            return {false, false, "Fixture unavailable."};
        if (f->kind == FixtureKind::Key)
            party_.questKeys_ |= std::uint8_t(1u << f->key);
        movement_.clearPending();
        detectContact();
        discover();
    }
    return {true, !preview,
            f->kind == FixtureKind::Door ? (preview ? "Open permanently." : "Door opened permanently.")
                                         : (preview ? "Collect quest key." : "Quest key collected.")};
}
} // namespace blobber
