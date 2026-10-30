#pragma once

#include "game/actors.hpp"
#include "world/map.hpp"
#include <cstdint>
#include <optional>

namespace blobber {
class EffectController;
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
bool tryAction(GeometryView map, Player& player, Action action, const ActorWorld* npcs = nullptr);
struct Transition {
    static constexpr float duration = 0.160f;
    Player from{}, to{};
    Action action = Action::Forward;
    float elapsed = duration, durationSeconds = duration;
    bool active() const { return elapsed < durationSeconds; }
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
    void submit(GeometryView map, Action action, bool fresh = true, const ActorWorld* npcs = nullptr);
    void update(GeometryView map, float seconds, const ActorWorld* npcs = nullptr);
    void clearPending() noexcept { pending_.reset(); }
    void setAnimated(bool enabled);
    void bindEffects(const EffectController* e) noexcept { effects_ = e; }

  private:
    MovementState state_;
    const EffectController* effects_ = nullptr;
    std::optional<Action> pending_;
};
} // namespace blobber
