#pragma once
#include "game/session.hpp"
#include "platform/presenter.hpp"
#include "platform/world_renderer.hpp"
#include "ui/map_model.hpp"
#include <stdexcept>
namespace blobber {
// Session view lease. GPU scenes remain owned by the shared active/staged renderer.
class GameView {
  public:
    explicit GameView(WorldRenderer& world, const generation::FloorLayout* rooms = nullptr)
        : world_(world), rooms_(rooms) {}
    ~GameView() {
        if (staged_)
            world_.cancel();
        else if (active_ && world_.visibility() == active_)
            world_.clear();
    }
    bool update(const GameSession& game, const NpcAnimationSnapshot& animation, const SpellArea& area) {
        const world_render::FrameInput input{
            game.geometry(), game.movement(),          &game.actors(), &animation,    &game.loot().snapshot(),
            &game.doors(),   &game.combat().attacks(), &area,          game.timeMs(), game.guardianDoor()};
        const auto previous = image_.revision;
        if (!ready_) {
            map_.prepare(game.geometry().base(), rooms_);
            rooms_ = nullptr; // Membership is copied; no long-lived generation borrow.
            world_.stage(input);
            staged_ = true;
            ready_ = true;
        }
        image_ = staged_ ? world_.renderStaged(input) : world_.render(input);
        return image_.revision != previous;
    }
    void activate() noexcept {
        if (staged_) {
            world_.commit();
            staged_ = false;
            active_ = world_.visibility();
        }
    }
    const PickingBuffer* picking() const noexcept {
        return staged_ ? world_.stagedPicking() : world_.picking();
    }
    std::optional<Cell> groundCell(const GameSession& g, int x, int y) const noexcept {
        if (g.actionBusy())
            return {};
        const auto* v = staged_ ? world_.stagedVisibility() : world_.visibility();
        return v ? v->groundCell({g.geometry(), g.movement(), nullptr, nullptr, nullptr, &g.doors()}, x, y)
                 : std::nullopt;
    }
    void present(Presenter& p, const GameSession& g, const Map& map, bool debug, bool /*changed*/,
                 const HudSnapshot* hud = nullptr, const generation::DiagnosticView* diagnostic = nullptr,
                 bool reveal = false) {
        activate();
        if (debug)
            map_.refresh(&g.fixtures());
        p.presentWorld(image_.target, image_.revision, g.movement(), map, hud, picking(), debug, diagnostic,
                       reveal, &map_);
    }

  private:
    WorldRenderer& world_;
    WorldImage image_{};
    MapOverlayModel map_;
    const generation::FloorLayout* rooms_ = nullptr; // Preparation-only borrow.
    const world_render::VisibilityPipeline* active_ = nullptr;
    bool ready_ = false, staged_ = false;
};
} // namespace blobber
