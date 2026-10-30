#include "app/campaign_saves.hpp"
#include <cstdio>
namespace blobber {
bool CampaignSaveFlow::closeRequested(CampaignRun& run, CommandController& commands) {
    if (!store_)
        return true;
    (void)poll(run, commands);
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
    if (commands.snapshot().menuOpen && !menuWasOpen_ && !h.terminalWarning)
        slot_ = store_->inspect();
    menuWasOpen_ = commands.snapshot().menuOpen;
    const auto intent = h.intent;
    h.intent = SaveIntent::None;
    if ((run.status() != CampaignStatus::Active && !terminalAttempted_) ||
        intent == SaveIntent::RetryTerminal) {
        terminalAttempted_ = true;
        const auto result = store_->terminal(run);
        h.terminalWarning = !result.durable;
        if (h.terminalWarning) {
            if (!commands.snapshot().menuOpen)
                commands.toggleMenu();
            message(result.error.c_str());
        } else {
            message("Expedition closed. Continue is unavailable.");
            if (commands.snapshot().menuOpen)
                commands.closeOverlay();
        }
        return false;
    }
    if (intent != SaveIntent::Save && intent != SaveIntent::SaveMenu)
        return false;
    h.leaveAfter = intent == SaveIntent::SaveMenu;
    if (slot_.exists && (!slot_.active || slot_.runId != run.runId()) && !h.replace) {
        h.confirmation = SaveConfirmation::Overwrite;
        h.cursor = 0;
        message("Replace the previous campaign? It remains available until this save succeeds.");
        return false;
    }
    try {
        const auto result = store_->save(run, slot_.revision, h.replace);
        h.replace = false;
        if (result.published)
            slot_.revision = result.revision;
        if (result.durable) {
            slot_.exists = slot_.active = true;
            slot_.runId = run.runId();
            message("Campaign saved.");
            return h.leaveAfter;
        }
        message(result.published ? "Save published, but durability is unconfirmed. Retry before leaving."
                                 : result.error.c_str());
        slot_ = store_->inspect();
    } catch (const std::exception& e) {
        h.replace = false;
        message(e.what());
        slot_ = store_->inspect();
    }
    return false;
}
} // namespace blobber
