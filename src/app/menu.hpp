#pragma once
#include "app/options.hpp"
#include "game/arena.hpp"
#include "platform/presenter.hpp"
namespace blobber {
int runMenus(const Options&);
bool launchArena(const Options&, const rules::Catalog&, const ArenaScenario&, Presenter&);
bool launchMenuParty(const Options&, const rules::Catalog&, const rules::PresetParty&, Presenter&);
} // namespace blobber
