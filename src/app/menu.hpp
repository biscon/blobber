#pragma once
#include "app/options.hpp"
#include "platform/presenter.hpp"
namespace blobber {
int runMenus(const Options&);
bool launchMenuParty(const Options&, const rules::Catalog&, const rules::PresetParty&, Presenter&);
} // namespace blobber
