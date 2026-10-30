#include "app/save_captures.hpp"
#include "assets/fingerprint.hpp"
#include "game/commands.hpp"
#include "render/frame_pipeline.hpp"
#include <stdexcept>
namespace blobber {
namespace {
bool failRename(CampaignIoStage stage) {
    return stage == CampaignIoStage::Rename;
}
} // namespace
void captureCampaignSave(CampaignRun& original, CampaignStore& saves, const std::vector<NpcTimings>& timings,
                         const SceneRenderer& renderer, Presenter& presenter, const Options& options) {
    const auto capture = [&](CampaignRun& run, const char* name, bool menu = false, const char* message = "",
                             bool overwrite = false, bool warning = false) {
        auto& game = run.game();
        NpcAnimator animator(game.actors(), timings);
        animator.setTime(game.timeMs(), &game.effects());
        CommandController commands(game, timings);
        commands.enableMainMenu();
        commands.saveMenu().enabled = true;
        FramePipeline frames;
        frames.update(renderer, game.geometry(), game.movement(), &game.actors(), &animator.snapshot(),
                      &game.combat().attacks(), game.timeMs(), nullptr, &game.loot().snapshot());
        commands.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false,
                         true);
        if (menu)
            commands.toggleMenu();
        auto& hud = commands.saveMenu();
        std::snprintf(hud.message.data(), hud.message.size(), "%s", message);
        if (overwrite)
            hud.confirmation = SaveConfirmation::Overwrite;
        hud.terminalWarning = warning;
        presenter.present(frames.frame(), game.movement(), run.map(), false, true, &commands.snapshot(),
                          frames.picking());
        presenter.capture(options.captureDirectory / (std::string(name) + ".png"));
        presenter.capture(options.captureDirectory / (std::string(name) + "-1280.png"), 1280, 720);
    };
    capture(original, "save-paused-hub", true);
    auto slot = saves.inspect();
    auto write = saves.save(original, slot.revision, true);
    if (!write.durable)
        throw std::runtime_error("Capture initial save failed");
    capture(original, "save-published", true, "Campaign saved.");
    auto ticket = original.prepareTravel(original.request(original.game().nearbyFixture()));
    if (!original.commitTravel(ticket))
        throw std::runtime_error("Capture stairs failed");
    // Explicit diagnostic injury, not a fought expedition.
    original.party().damage({0}, 2);
    write = saves.save(original, saves.inspect().revision);
    if (!write.durable)
        throw std::runtime_error("Capture floor save failed");
    auto loaded = saves.load();
    capture(*loaded, "save-resumed-floor1");
    auto recall = loaded->prepareTravel(loaded->recallRequest());
    if (!loaded->commitTravel(recall))
        throw std::runtime_error("Capture recall failed");
    write = saves.save(*loaded, saves.inspect().revision);
    if (!write.durable)
        throw std::runtime_error("Capture recalled save failed");
    auto resumed = saves.load();
    capture(*resumed, "save-recall-resumed");
    capture(*resumed, "save-overwrite-dialog", true,
            "Replace the previous campaign? It remains available until this save succeeds.", true);
    const auto definitions = loadNpcDefinitions(options.assets / "npcs/level.json");
    CampaignDisk failedDisk(options.campaignSave, {failRename});
    CampaignStore failed(
        resumed->party().catalog(), definitions,
        {fingerprintFile(options.assets / "rules/catalog.json"), definitions.definitionFingerprint},
        failedDisk);
    auto failure = failed.save(*resumed, failed.inspect().revision);
    if (failure.published || failure.error.empty())
        throw std::runtime_error("Capture expected save failure missing");
    capture(*resumed, "save-error-rename", true, failure.error.c_str());
    for (unsigned i = 0; i < 4; ++i)
        resumed->party().damage({std::uint16_t(i)}, 100000);
    auto terminal = failed.terminal(*resumed);
    if (terminal.durable || !failed.inspect().terminal)
        throw std::runtime_error("Capture terminal failure missing");
    capture(*resumed, "save-terminal-warning", true, terminal.error.c_str(), false, true);
    if (!saves.terminal(*resumed).durable || !saves.inspect().terminal)
        throw std::runtime_error("Capture terminal retry failed");
    capture(*resumed, "save-terminal-closed");
}
} // namespace blobber
