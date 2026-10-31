#include "automation/capture.hpp"
#include "assets/fingerprint.hpp"
#include "generation/diagnostic_view.hpp"
#include "platform/game_view.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <set>
namespace blobber::automation {
namespace {
void logToStderr(int, const char* text, va_list args) {
    std::vfprintf(stderr, text, args);
    std::fputc('\n', stderr);
}
class Capture final : public ReplayObserver {
  public:
    Capture(WorldRenderer& w, const NpcDefinitions& d, Presenter& p, const std::filesystem::path& o,
            const std::vector<std::size_t>& operations)
        : world(w), definitions(d), presenter(p), output(o), pending(operations.begin(), operations.end()) {}
    void boundary(Session* session, std::size_t operation, const Json& state) override {
        if (!pending.count(operation))
            return;
        if (!session)
            throw std::runtime_error("capture_invalid_point: no session at requested operation");
        auto& game = session->replayGame();
        NpcAnimator animator(game.actors(), definitions.timings);
        animator.setTime(game.timeMs(), &game.effects());
        GameView frames(world);
        SpellArea area;
        area.ground = game.effects().ground();
        frames.update(game, animator.snapshot(), area);
        CommandController hud(game, definitions.timings);
        if (session->arena())
            hud.arenaContext(arenaCleared(game.actors().snapshot()));
        hud.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, game.actionBusy(),
                    false, true);
        frames.present(presenter, game, session->map(), false, true, &hud.snapshot());
        const auto save = [&](const char* suffix) {
            const auto name = "operation-" + std::to_string(operation) + suffix + ".png";
            presenter.capture(output / name);
            bytes += std::filesystem::file_size(output / name);
            if (bytes > 64u * 1024u * 1024u) {
                std::filesystem::remove(output / name);
                throw std::runtime_error("capture_output_limit");
            }
            images.push_back({{"file", name},
                              {"operation", operation},
                              {"ticks", state.at("ticks")},
                              {"session", state.at("session")},
                              {"revision", state.at("revision")},
                              {"width", gameWidth},
                              {"height", gameHeight}});
        };
        save("");
        if (const auto* floor = session->generated()) {
            const auto diagnostic = generation::diagnosticView(*floor);
            hud.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, game.actionBusy(),
                        true, true);
            frames.present(presenter, game, session->map(), true, false, &hud.snapshot(), &diagnostic, true);
            save("-diagnostic-map");
            frames.present(presenter, game, session->map(), true, false, &hud.snapshot(), &diagnostic, false);
            save("-discovered-map");
        }
        world.clear();
        pending.erase(operation);
    }
    WorldRenderer& world;
    const NpcDefinitions& definitions;
    Presenter& presenter;
    std::filesystem::path output;
    std::set<std::size_t> pending;
    Json images = Json::array();
    std::uintmax_t bytes = 0;
};
} // namespace
void captureReplay(Protocol& protocol, const std::filesystem::path& trace, const Json& header,
                   const std::filesystem::path& assets, const NpcDefinitions& definitions,
                   const std::filesystem::path& output, const std::vector<std::size_t>& operations,
                   TraceCampaignFiles* files) {
    if (operations.empty() || operations.size() > 8 ||
        std::set<std::size_t>(operations.begin(), operations.end()).size() != operations.size() ||
        *std::min_element(operations.begin(), operations.end()) == 0)
        throw std::runtime_error("capture_invalid_points: choose 1..8 distinct positive operations");
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("capture_output_exists");
    SetTraceLogCallback(logToStderr);
    std::unique_ptr<Window> window;
    std::unique_ptr<GpuResources> resources;
    std::unique_ptr<Presenter> presenter;
    std::unique_ptr<WorldRenderer> world;
    try {
        auto publication = world_assets::load(assets / "prepared/world");
        if (publication.definitions != definitions.definitionFingerprint)
            throw std::runtime_error("World NPC definitions differ; rebuild world assets");
        window = std::make_unique<Window>(gameWidth, gameHeight, true);
        resources = std::make_unique<GpuResources>();
        presenter = std::make_unique<Presenter>(*resources, assets);
        world = std::make_unique<WorldRenderer>(*resources, std::move(publication));
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("graphics_unavailable: ") + e.what());
    }
    Capture capture(*world, definitions, *presenter, output, operations);
    replay(protocol, trace, header, &capture, files);
    if (!capture.pending.empty())
        throw std::runtime_error("capture_invalid_point: operation exceeds recording");
    Json prepared = Json::object();
    for (const auto* path : {"prepared/world/metadata.json", "prepared/world/textures.bin"})
        prepared[path] = fingerprintFile(assets / path);
    const Json metadata = {{"replay", "matched"},
                           {"trace", trace.string()},
                           {"trace_fingerprint", fingerprintFile(trace)},
                           {"header", header},
                           {"assets", assets.string()},
                           {"prepared", prepared},
                           {"images", capture.images},
                           {"presentation", "Current scene and HUD; no historical UI interactions"}};
    std::ofstream stream(output / "capture.json", std::ios::out);
    stream << metadata.dump(2) << '\n';
    stream.close();
    if (!stream)
        throw std::runtime_error("capture_metadata_write_failed");
}
} // namespace blobber::automation
