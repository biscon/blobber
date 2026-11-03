#include "audio/limiter.hpp"
#include "platform/audio.hpp"
#include "support/check.hpp"
#include "support/hybrid_fixtures.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
using namespace blobber;
namespace {
std::atomic<float> peak{0};
std::atomic<unsigned> audibleFrames{0};
void capture(void* data, unsigned frames) {
    const auto* pcm = static_cast<const float*>(data);
    float p = 0;
    unsigned audible = 0;
    for (unsigned i = 0; i < frames * 2; ++i) {
        p = std::max(p, std::abs(pcm[i]));
        audible += std::abs(pcm[i]) > .00001f;
    }
    peak.store(std::max(peak.load(), p));
    audibleFrames.fetch_add(audible);
}
} // namespace
int main(int argc, char** argv) {
    if (argc != 2)
        return 2;
    SetTraceLogLevel(LOG_WARNING);
    const std::filesystem::path root(argv[1]);
    AudioRuntime runtime;
    runtime.prepare(root, false, root / "settings.json");
    if (!runtime.ready())
        return 77;
    check(runtime.warning().empty(), "Owned MP3 bank loads");
    SetMasterVolume(0); // Measurements are before device master; speakers stay silent.
    runtime.settings.values.fill(1);
    runtime.settings.musicVolume = 1;
    // Decode the owned quiet MP3 as a reference for source-level stereo playback.
    auto reference = LoadWave((root / "audio/music/short.mp3").c_str());
    check(IsWaveValid(reference), "Owned source-level reference decodes");
    WaveFormat(&reference, 48000, 32, 2);
    const auto* referencePcm = static_cast<const float*>(reference.data);
    float sourcePeak = 0;
    for (unsigned i = 0; i < std::min(reference.frameCount, 14400u) * 2; ++i)
        sourcePeak = std::max(sourcePeak, std::abs(referencePcm[i]));
    UnloadWave(reference);
    AttachAudioMixedProcessor(capture);
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog();
    NpcWorld places(map, {5, 8}, {}, {});
    GameSession session(map, catalog, places, 5, {{5, 8}, Heading::North}, false);
    runtime.update(&session, false, true, 0, false);
    auto tick = [&](unsigned ms, bool pause = false, bool focus = true, bool tactical = false) {
        for (unsigned i = 0; i < ms / 10; ++i) {
            runtime.update(&session, pause || tactical, focus, .01f, pause, !tactical);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    };
    CombatOutcome event;
    event.event = CombatEvent::Started;
    session.audioEvents().combat(event);
    tick(300);
    check(audibleFrames > 0 && runtime.musicBanner().visible, "Real MP3 produces mixed samples and banner");
    const float musicPeak = peak.load();
    check(sourcePeak > .001f && musicPeak > sourcePeak * .96f && musicPeak < sourcePeak * 1.04f,
          "100 percent music matches decoded stereo source level, without center attenuation");
    std::cout << "Source peak=" << sourcePeak << " music peak=" << musicPeak << '\n';
    const auto first = runtime.musicBanner().revision;
    tick(80, true);
    audibleFrames = 0;
    tick(100, true);
    check(audibleFrames == 0, "Explicit pause is silent");
    tick(80, false, false);
    audibleFrames = 0;
    tick(100, false, false);
    check(audibleFrames == 0, "Focus loss is silent");
    tick(100, false, true, true);
    check(audibleFrames > 0 && !runtime.musicBanner().visible, "Tactical panels keep music running");
    runtime.settings.musicEnabled = false;
    tick(80);
    audibleFrames = 0;
    tick(100);
    check(audibleFrames == 0, "Music toggle freezes real output");
    runtime.settings.musicEnabled = true;
    tick(100);
    check(runtime.musicBanner().revision == first, "Unmute resumes same track");
    for (unsigned i = 0; i < 250 && runtime.musicBanner().revision == first; ++i)
        tick(10);
    check(runtime.musicBanner().revision > first && runtime.musicBanner().visible,
          "Nonlooping MP3 end advances playlist");
    const auto second = runtime.musicBanner().revision;
    for (unsigned i = 0; i < 8; ++i)
        runtime.play("hot");
    tick(100);
    check(peak > .8f && peak <= audio::PeakLimiter::ceiling + 1e-6f,
          "Music plus eight hot effects respects final ceiling");
    event.event = CombatEvent::Finished;
    session.audioEvents().combat(event);
    tick(2500);
    check(runtime.musicBanner().visible && runtime.musicBanner().revision == second,
          "Five-second fade still on same track halfway");
    tick(2600);
    check(!runtime.musicBanner().visible, "Five-second fade ends without starting next track");
    audibleFrames = 0;
    tick(100);
    check(audibleFrames == 0, "No following song leaks into postcombat silence");
    event.event = CombatEvent::Started;
    session.audioEvents().combat(event);
    tick(1500); // Remaining duration of the short fixture is under five seconds.
    event.event = CombatEvent::Finished;
    session.audioEvents().combat(event);
    tick(900);
    check(!runtime.musicBanner().visible, "Short track uses remaining duration for fade");
    runtime.detach();
    DetachAudioMixedProcessor(capture);
    runtime.shutdown();
    std::cout << "Muted real MP3 integration: advance, pause/focus/mute, tactical continuity, 5s/short fade "
                 "and mix ceiling passed; peak="
              << peak << '\n';
}
