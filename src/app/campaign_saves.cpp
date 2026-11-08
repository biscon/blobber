#include "app/campaign_saves.hpp"
#include "core/preparation.hpp"
#include <cstdio>
namespace blobber {
bool CampaignSaveFlow::closeRequested(CampaignRun& run, CommandController& commands) {
    if (!store_)
        return true;
    if (terminalPending(run, commands))
        return false; // The application must present the result before starting terminal I/O.
    if (run.status() != CampaignStatus::Active && !commands.saveMenu().terminalWarning)
        return true;
    if (!commands.snapshot().menuOpen)
        commands.toggleMenu();
    commands.saveMenu().confirmation = SaveConfirmation::Quit;
    commands.saveMenu().cursor = 0;
    return false;
}
bool CampaignSaveFlow::poll(CampaignRun& run, CommandController& commands) {
    if (!store_)
        return false;
    auto& h = commands.saveMenu();
    h.enabled = true;
    const auto message = [&](const char* text) {
        std::snprintf(h.message.data(), h.message.size(), "%s", text);
    };
    const auto intent = h.intent;
    h.intent = SaveIntent::None;
    if ((run.status() != CampaignStatus::Active && !terminalAttempted_) ||
        intent == SaveIntent::RetryTerminal) {
        terminalAttempted_ = true;
        const CampaignTerminalRequest request{run.runId(), run.status()};
        store_->denyContinue(request.runId);
        const auto result = prepareCpu([&] { return store_->terminal(request); });
        h.terminalWarning = !result.durable;
        if (h.terminalWarning) {
            if (!commands.snapshot().menuOpen)
                commands.toggleMenu();
            std::fprintf(stderr, "Closing save failed: %s\n", result.error.c_str());
            message("Couldn't record the end of this expedition. Please retry.");
        } else {
            message("Expedition closed. Continue is unavailable.");
            if (commands.snapshot().menuOpen)
                commands.closeOverlay();
        }
        return false;
    }
    if (intent != SaveIntent::Save && intent != SaveIntent::SaveMenu)
        return false;
    // Metadata only; a confirmed overwrite keeps its previously reviewed revision.
    if (!h.replace)
        slot_ = prepareCpu([&] { return store_->inspect(); });
    h.leaveAfter = intent == SaveIntent::SaveMenu;
    if (slot_.exists && (!slot_.active || slot_.runId != run.runId()) && !h.replace) {
        h.confirmation = SaveConfirmation::Overwrite;
        h.cursor = 0;
        message("Replace the previous campaign? It remains available until this save succeeds.");
        return false;
    }
    try {
        const auto result = prepareCpu([&] { return store_->save(run, slot_.revision, h.replace); });
        h.replace = false;
        if (result.published)
            slot_.revision = result.revision;
        if (result.durable) {
            slot_.exists = slot_.active = true;
            slot_.runId = run.runId();
            message("Campaign saved.");
            return h.leaveAfter;
        }
        std::fprintf(stderr, "Campaign save failed: %s\n", result.error.c_str());
        message(result.published ? "Couldn't confirm the save. Retry before leaving."
                                 : "Couldn't save. Please try again.");
        slot_ = prepareCpu([&] { return store_->inspect(); });
    } catch (const std::exception& e) {
        h.replace = false;
        std::fprintf(stderr, "Campaign save failed: %s\n", e.what());
        message("Couldn't save. Please try again.");
        slot_ = prepareCpu([&] { return store_->inspect(); });
    }
    return false;
}
} // namespace blobber
