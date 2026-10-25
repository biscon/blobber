#pragma once

#include "game/actors.hpp"
#include "world/map.hpp"
#include <cstdint>
#include <optional>

namespace blobber {
enum class Action { Forward, Back, Left, Right, TurnLeft, TurnRight };
inline bool isStrafe(Action action) {
    return action == Action::Left || action == Action::Right;
}
struct Player {
    Cell cell{3, 21};
    Heading heading = Heading::North;
};
inline bool operator==(const Player& a, const Player& b) {
    return a.cell == b.cell && a.heading == b.heading;
}
bool tryAction(const Map& map, Player& player, Action action, const ActorWorld* npcs = nullptr);
struct Transition {
    static constexpr float duration = 0.160f;
    Player from{}, to{};
    Action action = Action::Forward;
    float elapsed = duration;
    bool active() const { return elapsed < duration; }
    float progress() const;
};
struct MovementState {
    Player player;
    Transition transition;
    bool animated = true, blocked = false;
    std::uint64_t revision = 0;
};
class Controller {
  public:
    explicit Controller(Player spawn = {}) { state_.player = spawn; }
    const MovementState& state() const noexcept { return state_; }
    void submit(const Map& map, Action action, bool fresh = true, const ActorWorld* npcs = nullptr);
    void update(const Map& map, float seconds, const ActorWorld* npcs = nullptr);
    void clearPending() noexcept { pending_.reset(); }
    void setAnimated(bool enabled);

  private:
    MovementState state_;
    std::optional<Action> pending_;
};
} // namespace blobber
