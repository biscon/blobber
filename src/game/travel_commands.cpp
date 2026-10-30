#include "game/campaign.hpp"
#include "game/commands.hpp"
namespace blobber {
void CommandController::openTravel() noexcept {
    if (!session_ || !session_->campaign() || state_.suspended || state_.modal() || state_.magic.casting() ||
        state_.magic.armed || state_.inventory.itemTargeting)
        return;
    auto& run = *session_->campaign();
    if (run.status() != CampaignStatus::Active)
        return;
    const bool hub = run.activeLocation() == 0;
    if (hub) {
        const auto check = run.previewTravel(run.request(session_->nearbyFixture()));
        if (!check) {
            state_.log.append(check.reason);
            return;
        }
    }
    closeOverlay();
    cancel();
    state_.hover = {};
    state_.travel = {};
    auto& h = state_.travel;
    h.open = true;
    h.hub = hub;
    if (hub) {
        for (unsigned depth = 1; depth <= 3; ++depth)
            if (run.location(depth).visited || depth == 1)
                h.rows[h.count++] = {depth, run.location(depth).visited
                                                ? run.waypointRequest(depth)
                                                : run.request(session_->nearbyFixture())};
    } else {
        h.count = 1;
        h.rows[0].request = run.recallRequest();
    }
    session_->clearPending();
    refreshTravel();
}
void CommandController::refreshTravel() noexcept {
    auto& h = state_.travel;
    if (!session_ || !session_->campaign() || !h.open || h.cursor >= h.count)
        return;
    h.kits = party_.campSupplies().kits;
    h.preview = session_->campaign()->previewTravel(h.rows[h.cursor].request);
}
void CommandController::selectTravel(unsigned row) noexcept {
    if (!state_.travel.open || row >= state_.travel.count)
        return;
    state_.travel.cursor = row;
    refreshTravel();
}
void CommandController::confirmTravel() noexcept {
    auto& h = state_.travel;
    if (!h.open)
        return;
    refreshTravel();
    if (!h.preview) {
        state_.log.append(h.preview.reason);
        return;
    }
    const auto request = h.rows[h.cursor].request;
    closeOverlay();
    state_.travelRequested = request;
}
} // namespace blobber
