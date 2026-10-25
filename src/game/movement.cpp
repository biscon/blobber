#include "game/movement.hpp"
#include <algorithm>

namespace blobber {
bool tryAction(const Map& map, Player& player, Action action, const ActorWorld* npcs) {
    if (action == Action::TurnLeft || action == Action::TurnRight) {
        player.heading = static_cast<Heading>(
            (static_cast<int>(player.heading) + (action == Action::TurnLeft ? 3 : 1)) % 4);
        return true;
    }
    Cell delta;
    switch (action) {
    case Action::Forward:
        delta = forward(player.heading);
        break;
    case Action::Back:
        delta = forward(player.heading) * -1;
        break;
    case Action::Left:
        delta = right(player.heading) * -1;
        break;
    case Action::Right:
        delta = right(player.heading);
        break;
    default:
        return false;
    }
    if (map.solid(player.cell + delta) || (npcs && npcs->occupied(player.cell + delta)))
        return false;
    player.cell = player.cell + delta;
    return true;
}
float Transition::progress() const {
    return std::clamp(elapsed / duration, 0.0f, 1.0f);
}
void Controller::submit(const Map& map, Action action, bool fresh, const ActorWorld* npcs) {
    if (state_.transition.active()) {
        if (fresh)
            pending_ = action;
        return;
    }
    const Player old = state_.player;
    state_.blocked = !tryAction(map, state_.player, action, npcs);
    if (state_.blocked)
        return;
    state_.transition = {old, state_.player, action, state_.animated ? 0.0f : Transition::duration};
    ++state_.revision;
}
void Controller::update(const Map& map, float seconds, const ActorWorld* npcs) {
    state_.transition.elapsed =
        std::min(Transition::duration, state_.transition.elapsed + std::max(0.0f, seconds));
    if (!state_.transition.active() && pending_) {
        const Action action = *pending_;
        pending_.reset();
        submit(map, action, true, npcs);
    }
}
void Controller::setAnimated(bool enabled) {
    state_.animated = enabled;
    if (!enabled)
        state_.transition.elapsed = Transition::duration;
}
} // namespace blobber
