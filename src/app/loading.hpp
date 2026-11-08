#pragma once
#include "core/preparation.hpp"
#include "platform/audio.hpp"
#include "platform/presenter.hpp"
namespace blobber {
// Outlives fade-out, threaded loading and fade-in; every exit restores normal policy.
class TravelMusicScope {
  public:
    TravelMusicScope(AudioRuntime* audio, bool sanctuaryDestination) : audio_(audio) {
        if (audio_)
            audio_->beginTravelMusic(sanctuaryDestination);
    }
    ~TravelMusicScope() {
        if (audio_)
            audio_->endTravelMusic();
    }
    TravelMusicScope(const TravelMusicScope&) = delete;
    TravelMusicScope& operator=(const TravelMusicScope&) = delete;

  private:
    AudioRuntime* audio_;
};
// A save is joined even after a native close request: publication is not cancellable.
// Pumping borrows only the cached presenter, never the run, commands or store.
class SavingScreen {
  public:
    SavingScreen(PreparationWorker* worker, Presenter& presenter,
                 bool (*closeRequested)() = WindowShouldClose, bool terminal = false)
        : presenter_(presenter), closeRequested_(closeRequested), terminal_(terminal),
          scope_(
              {[](void* owner, const char*) { static_cast<SavingScreen*>(owner)->point(); }, this, worker}) {
        if (worker)
            worker->reset();
        point(); // Display feedback before inspection or snapshot work starts.
    }
    bool closing() const noexcept { return closing_; }
    unsigned presentations() const noexcept { return presentations_; }

  private:
    void point() {
        const auto now = GetTime();
        if (last_ >= 0 && now - last_ < .004)
            return;
        closing_ |= closeRequested_();
        ++presentations_;
        if (presenter_.audio)
            presenter_.audio->pumpLoadingMusic(IsWindowFocused());
        if (terminal_)
            presenter_.presentCached(0);
        else
            presenter_.presentSaving();
        last_ = GetTime();
    }
    Presenter& presenter_;
    bool (*closeRequested_)(); // Injectable close probe; verification never sends OS input.
    bool terminal_;
    double last_ = -1;
    bool closing_ = false;
    unsigned presentations_ = 0;
    PreparationScope scope_;
};
// Main-thread loading presentation. Scope ends only after every submitted task is joined.
class LoadingScreen {
  public:
    LoadingScreen(PreparationWorker* worker, Presenter* presenter = nullptr, bool travel = false)
        : worker_(worker), presenter_(presenter), travel_(travel), start_(GetTime()), last_(-1),
          scope_({[](void* owner, const char* phase) { static_cast<LoadingScreen*>(owner)->point(phase); },
                  this, worker}) {
        if (worker_)
            worker_->reset();
        point("Loading");
    }
    void attach(Presenter& presenter) noexcept { presenter_ = &presenter; }
    bool closing() const noexcept { return closing_; }
    unsigned presentations() const noexcept { return presentations_; }
    double longestGapSeconds() const noexcept { return longestGap_; }
    void point(const char* phase) {
        const double now = GetTime();
        if (last_ >= 0 && now - last_ < .004)
            return;
        if (lastPresentation_ >= 0)
            longestGap_ = std::max(longestGap_, now - lastPresentation_);
        lastPresentation_ = now;
        ++presentations_;
        if (WindowShouldClose()) {
            closing_ = true;
            if (worker_)
                worker_->cancel();
        }
        if (presenter_ && presenter_->audio)
            presenter_->audio->pumpLoadingMusic(IsWindowFocused());
        // Preparation phases name engine resources; keep those details out of player UI.
        const char* status =
            phase && (!travel_ || now - start_ >= .25) ? (travel_ ? "Travelling..." : "Loading...") : nullptr;
        if (presenter_)
            presenter_->presentCached(1, status);
        else {
            BeginDrawing();
            ClearBackground({12, 18, 18, 255});
            if (status)
                DrawText(status, std::max(8, (GetScreenWidth() - MeasureText(status, 24)) / 2),
                         GetScreenHeight() / 2, 24, {202, 178, 118, 255});
            EndDrawing();
        }
        // Presentation/vsync time is not preparation work; start the next work budget after it.
        last_ = GetTime();
        if (closing_)
            throw PreparationCancelled();
    }

  private:
    PreparationWorker* worker_;
    Presenter* presenter_;
    bool travel_, closing_ = false;
    double start_, last_;
    unsigned presentations_ = 0;
    double longestGap_ = 0, lastPresentation_ = -1;
    PreparationScope scope_;
};
} // namespace blobber
