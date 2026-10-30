#pragma once
#include "game/commands.hpp"
#include "persistence/campaign_store.hpp"
namespace blobber {
// Explicit application I/O boundary; ordinary HUD refresh never accesses files.
class CampaignSaveFlow {
  public:
    explicit CampaignSaveFlow(CampaignStore* store) : store_(store) {}
    bool closeRequested(CampaignRun&, CommandController&); // Native-close intent, no OS input.
    bool poll(CampaignRun&, CommandController&);           // true: successful Save and Main Menu.
  private:
    CampaignStore* store_;
    CampaignSlot slot_;
    bool menuWasOpen_ = false, terminalAttempted_ = false;
};
} // namespace blobber
