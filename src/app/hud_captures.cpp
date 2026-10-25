#include "app/hud_captures.hpp"
#include "ui/interaction.hpp"
#include <limits>
#include <stdexcept>
namespace blobber {
namespace {
// Diagnostic preparation may allocate; scripted input uses the production router.
struct CaptureView {
    GameSession session;
    CommandController commands;
    NpcAnimator animator;
    FramePipeline frames;
    const Map& map;
    const SceneRenderer& renderer;
    CaptureView(const Map& m, const rules::Catalog& c, const NpcWorld& n, const std::vector<NpcTimings>& t,
                const SceneRenderer& r, unsigned seed)
        : session(m, c, n, seed, combatCamera(m, n)), commands(session, t), animator(session.actors(), t),
          map(m), renderer(r) {
        commands.record(session.update(0, false));
        refresh();
    }
    void refresh(bool debug = false) {
        animator.setTime(session.timeMs());
        frames.update(renderer, map, session.movement(), &session.actors(), &animator.snapshot());
        commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, debug,
                         true);
    }
    void input(UiInput input) {
        const auto* picking = frames.picking();
        const auto target = input.pointer && picking
                                ? picking->pick(int(input.pointer->x) - viewX, int(input.pointer->y) - viewY)
                                : std::nullopt;
        routeHudInput(commands, input, target);
        refresh();
    }
    std::optional<ArtPoint> body(ActorId id) const {
        const auto* picking = frames.picking();
        if (!picking)
            return {};
        const auto anchor = picking->targets().anchors[id.slot];
        if (!anchor.valid)
            return {};
        int distance = std::numeric_limits<int>::max();
        std::optional<ArtPoint> best;
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x)
                if (picking->slot(x, y) == id.slot) {
                    const int d = (x - anchor.x) * (x - anchor.x) + (y - anchor.y) * (y - anchor.y);
                    if (d < distance) {
                        distance = d;
                        best = ArtPoint{float(viewX + x), float(viewY + y)};
                    }
                }
        return best;
    }
    void character(unsigned id) {
        for (int step = 0; step < 200; ++step) {
            const auto& state = session.combat().snapshot();
            if (state.awaitingAction && state.activeCharacter->index == id) {
                refresh();
                return;
            }
            if (state.awaitingAction) {
                commands.selectAction(PartyAction::Defend);
                commands.confirm();
            } else
                commands.record(session.update(.45, false));
            refresh();
        }
        throw std::runtime_error("HUD capture cannot reach requested turn");
    }
};
UiInput click(UiRect r) {
    UiInput input;
    input.pointer = ArtPoint{float(r.x + r.width / 2), float(r.y + r.height / 2)};
    input.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
    return input;
}
} // namespace
void captureHud(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                const std::vector<NpcTimings>& timings, const SceneRenderer& renderer, Presenter& presenter,
                const std::filesystem::path& directory) {
    if (placements.instances().empty())
        return;
    const auto idle = gameplayPlacements(map, placements);
    auto save = [&](CaptureView& v, const char* name, bool reduced = false, bool debug = false) {
        v.refresh(debug);
        presenter.present(v.frames.frame(), v.session.movement(), map, debug, true, &v.commands.snapshot(),
                          v.frames.picking());
        presenter.capture(directory / (std::string(name) + ".png"));
        if (reduced) {
            presenter.capture(directory / (std::string(name) + "-1280.png"), 1280, 720);
            presenter.capture(directory / (std::string(name) + "-4x3.png"), 1024, 768);
        }
    };
    {
        CaptureView v(map, catalog, idle, timings, renderer, 5);
        save(v, "hud-overview", true);
        v.character(0);
        for (std::size_t i = 0; i < v.commands.snapshot().targets.count; ++i) {
            const auto id = v.commands.snapshot().targets.ids[i];
            if (v.session.combat().exposed(id)) {
                UiInput hover;
                hover.pointer = v.body(id);
                v.input(hover);
                save(v, "hud-hover-reachable", true);
                break;
            }
        }
        UiInput hover;
        hover.pointer = ArtPoint{float(hudLayout::action(1).x + 10), float(hudLayout::action(1).y + 10)};
        v.input(hover);
        save(v, "hud-hover-defend");
        hover.pointer = ArtPoint{float(hudLayout::action(3).x + 10), float(hudLayout::action(3).y + 10)};
        v.input(hover);
        save(v, "hud-hover-unavailable");
        v.input(click(hudLayout::card(2)));
        save(v, "hud-character-details", true);
        v.input(click(hudLayout::detailsClose));
        v.input(click(hudLayout::gear));
        save(v, "hud-pause");
        v.input(click(hudLayout::resume));
        v.character(2);
        if (v.commands.snapshot().targets.count) {
            hover.pointer = v.body(v.commands.snapshot().targets.ids[0]);
            v.input(hover);
            save(v, "hud-hover-unreachable");
        }
        v.commands.hover(HoverKind::None);
        save(v, "hud-map", false, true);
    }
    {
        CaptureView v(map, catalog, idle, timings, renderer, 5);
        v.character(1);
        v.session.party().damage({0}, v.session.party().character({0})->hitPoints);
        v.refresh();
        v.input(click(hudLayout::action(5)));
        save(v, "hud-aid-targeting");
        UiInput hover;
        hover.pointer = ArtPoint{520, 40};
        v.input(hover);
        save(v, "hud-aid-hover");
        v.input(click(hudLayout::card(0)));
        save(v, "hud-aid-result");
        for (int step = 0; step < 16; ++step) {
            if (v.session.combat().snapshot().awaitingAction)
                v.input(click(hudLayout::action(1)));
            else {
                v.commands.record(v.session.update(.45, false));
                v.refresh();
            }
        }
        v.input(click(hudLayout::logExpand));
        save(v, "hud-history", true);
        UiInput scroll;
        scroll.pointer = ArtPoint{200, 100};
        scroll.scroll = 4;
        v.input(scroll);
        save(v, "hud-history-scrolled");
    }
    {
        CaptureView v(map, catalog, idle, timings, renderer, 5);
        v.character(0);
        ActorId rear;
        for (std::size_t i = 0; i < v.session.combat().snapshot().enemyCount; ++i) {
            const auto id = v.session.combat().snapshot().enemies[i];
            if (!v.session.combat().exposed(id))
                rear = id;
        }
        const auto* actor = v.session.actors().actor(rear);
        if (!actor)
            throw std::runtime_error("Diagonal capture requires a protected rear enemy");
        const auto original = actor->npc.subcell;
        const auto paired = original + forward(v.session.movement().player.heading) * -1;
        for (std::size_t i = 0; i < v.session.combat().snapshot().enemyCount; ++i) {
            const auto id = v.session.combat().snapshot().enemies[i];
            if (!sameActor(id, rear) && !(v.session.actors().actor(id)->npc.subcell == paired))
                v.session.actors().damage(id, std::numeric_limits<int>::max(), v.session.timeMs());
        }
        v.commands.record(v.session.update(1.2, false)); // Held party turn; let diagnostic deaths finish.
        save(v, "hud-diagonal-before", true);
        bool advanced = false;
        for (int step = 0; step < 40 && !advanced; ++step) {
            if (v.session.combat().snapshot().awaitingAction) {
                v.commands.selectAction(PartyAction::Defend);
                v.commands.confirm();
            } else {
                const auto out = v.session.update(.45, false);
                v.commands.record(out);
                advanced = out.enemyAttack && sameActor(out.enemy, rear);
            }
            v.refresh();
        }
        if (!advanced || actor->npc.subcell == original || actor->npc.subcell == paired)
            throw std::runtime_error("Protected rear enemy did not advance diagonally and attack");
        save(v, "hud-diagonal-after", true);
    }
    // Isolate one real enemy outcome to compare guard, wind-up and strike feedback.
    std::array<bool, 2> impactCaptured{};
    for (unsigned seed = 5; seed < 40 && !(impactCaptured[0] && impactCaptured[1]); ++seed) {
        CaptureView v(map, catalog, idle, timings, renderer, seed);
        for (int step = 0; step < 100 && v.session.combat().snapshot().phase == CombatPhase::Turns; ++step) {
            if (v.session.combat().snapshot().awaitingAction) {
                v.commands.selectAction(PartyAction::Defend);
                v.commands.confirm();
                v.refresh();
                continue;
            }
            const auto out = v.session.update(.45, false);
            v.refresh();
            if (out.event != CombatEvent::Attack || !out.enemyAttack || impactCaptured[out.hit ? 0 : 1])
                continue;
            const auto& attack = timings[v.session.actors().actor(out.enemy)->npc.type.index]
                                        [static_cast<std::size_t>(NpcAnimation::Attack)];
            unsigned delay = 0;
            for (std::size_t i = 0; i < attack.count / 2; ++i)
                delay += attack.durationMs[i];
            if (!delay)
                throw std::runtime_error("Impact capture requires a multi-pose attack");
            CommandController impactUi(v.session, timings);
            impactUi.record(out);
            const auto captureImpact = [&](const char* phase) {
                v.refresh();
                impactUi.refresh(v.frames.picking() ? v.frames.picking()->targets() : TargetSnapshot{}, false,
                                 false, true);
                presenter.present(v.frames.frame(), v.session.movement(), map, false, true,
                                  &impactUi.snapshot(), v.frames.picking());
                const auto name = std::string("hud-impact-") + (out.hit ? "hit-" : "miss-") + phase;
                presenter.capture(directory / (name + ".png"));
                presenter.capture(directory / (name + "-1280.png"), 1280, 720);
            };
            captureImpact("guard");
            v.session.update(double(delay - 1) / 1000, false);
            captureImpact("windup");
            v.session.update(.001, false);
            captureImpact("strike");
            impactCaptured[out.hit ? 0 : 1] = true;
            break;
        }
    }
    if (!impactCaptured[0] || !impactCaptured[1])
        throw std::runtime_error("HUD capture did not exercise hit and MISS impact timing");
    std::array<bool, 4> captured{};
    constexpr const char* names[]{"hud-damage-monster", "hud-miss-monster", "hud-damage-party",
                                  "hud-miss-party"};
    for (unsigned seed = 5; seed < 12 && !(captured[0] && captured[1] && captured[2] && captured[3]);
         ++seed) {
        CaptureView v(map, catalog, idle, timings, renderer, seed);
        for (int step = 0; step < 300 && v.session.combat().snapshot().phase == CombatPhase::Turns; ++step) {
            if (v.session.combat().snapshot().awaitingAction) {
                bool attacked = false;
                if (v.session.combat().exposed(*v.session.combat().snapshot().activeCharacter)) {
                    for (std::size_t i = 0; i < v.commands.snapshot().targets.count; ++i) {
                        const auto id = v.commands.snapshot().targets.ids[i];
                        if (v.session.combat().member(id) && v.session.combat().exposed(id)) {
                            UiInput input;
                            input.pointer = v.body(id);
                            input.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
                            v.input(input);
                            attacked = true;
                            break;
                        }
                    }
                }
                if (!attacked)
                    v.input(click(hudLayout::action(1)));
            } else {
                v.commands.record(v.session.update(.45, false));
                v.refresh();
            }
            v.commands.hover(HoverKind::None);
            for (const auto& feedback : v.commands.snapshot().feedback)
                if (feedback.active) {
                    const auto index = (feedback.party ? 2 : 0) + (feedback.hit ? 0 : 1);
                    if (!captured[index]) {
                        save(v, names[index], true);
                        captured[index] = true;
                    }
                }
        }
    }
    for (bool found : captured)
        if (!found)
            throw std::runtime_error("HUD capture did not exercise every hit/miss destination");
}
} // namespace blobber
