#pragma once
#include "app/options.hpp"
#include "game/campaign.hpp"
#include "platform/input.hpp"
#include <chrono>
#include <nlohmann/json.hpp>
namespace blobber {
class Presenter;
class CommandController;
struct MenuView;
// Diagnostic operations only. Never constructed or called during ordinary play.
nlohmann::json distributionSmokeState(const CampaignRun&);
void requireDistributionSmokeState(const nlohmann::json& expected, const CampaignRun&);
void distributionSmokeWalk(GameSession&, Player target, unsigned& actions);
class DistributionSmoke {
  public:
    explicit DistributionSmoke(const Options&);
    int menu(MenuView&, Presenter&);
    bool gameplay(CampaignRun&, CommandController&, Presenter&);
    InputSample sample() const;
    void saved(const CampaignRun&);
    void complete();
  private:
    void point();
    void capture(Presenter&, const char*);
    const Options& options_;
    nlohmann::json graphics_;
    unsigned stage_ = 0, menus_ = 0, actions_ = 0;
    bool launched_ = false, finished_ = false;
    std::chrono::steady_clock::time_point started_ = std::chrono::steady_clock::now();
};
} // namespace blobber
