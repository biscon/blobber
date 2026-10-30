#pragma once
#include "app/options.hpp"
#include "persistence/campaign_store.hpp"
#include "platform/presenter.hpp"
#include "render/scene_renderer.hpp"
namespace blobber {
void captureCampaignSave(CampaignRun&, CampaignStore&, const std::vector<NpcTimings>&, const SceneRenderer&,
                         Presenter&, const Options&);
}
