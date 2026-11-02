#pragma once
#include "platform/presenter.hpp"
namespace blobber {
void runAudioSettings(Presenter&, GameSession* game = nullptr);
void captureAudioSettings(Presenter&, const std::filesystem::path&);
} // namespace blobber
