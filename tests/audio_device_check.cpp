#include "platform/audio.hpp"
#include "support/check.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
using namespace blobber;
namespace {
std::atomic<unsigned> phase{0};
std::array<std::atomic<double>, 5> left{}, rightEnergy{};
void capture(void* buffer, unsigned frames) {
    const auto* samples = static_cast<float*>(buffer);
    double l = 0, r = 0;
    for (unsigned i = 0; i < frames; ++i) {
        l += samples[2 * i] * samples[2 * i];
        r += samples[2 * i + 1] * samples[2 * i + 1];
    }
    auto p = phase.load();
    left[p].store(left[p].load() + l);
    rightEnergy[p].store(rightEnergy[p].load() + r);
}
void tick(AudioRuntime& audio, unsigned ms, bool paused = false) {
    for (unsigned i = 0; i < ms / 10; ++i) {
        audio.update(nullptr, paused, true, .01f);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
} // namespace
int main(int argc, char** argv) {
    SetTraceLogLevel(LOG_WARNING);
    const auto path = std::filesystem::temp_directory_path() /
                      ("blobber-audio-device-" +
                       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(path / "audio");
    struct Cleanup {
        std::filesystem::path p;
        ~Cleanup() { std::filesystem::remove_all(p); }
    } cleanup{path};
    std::array<short, 24000> pcm{};
    for (unsigned i = 0; i < pcm.size(); ++i)
        pcm[i] = short(std::sin(i * 6.283185307179586 * 440 / 48000) * 3000);
    Wave wave{unsigned(pcm.size()), 48000, 16, 1, pcm.data()};
    check(ExportWave(wave, (path / "audio/tone.wav").c_str()), "Owned wave exported");
    {
        std::ofstream f(path / "audio/bank.json");
        f << R"({"cues":{"tone":{"gain":0.1,"cooldown":0,"pitch_min":1,"pitch_max":1,"volume_min":1,"variants":[{"id":"01","path":"tone.wav"}]}}})";
    }
    AudioRuntime audio;
    if (argc > 1) {
        audio.prepare(argv[1], false, path / "settings.json");
        if (!audio.ready())
            return 77;
        check(audio.warning().empty(), "Complete production bank loads without omissions");
        audio.shutdown();
    }
    audio.prepare(path, false, path / "settings.json");
    if (!audio.ready()) {
        std::cout << "SKIP: no real audio device\n";
        return 77;
    }
    // Allow the backend to establish its output callback before measuring first onset.
    tick(audio, 300);
    AttachAudioMixedProcessor(capture);
    auto h = audio.play("tone", -2, 0, true);
    tick(audio, 180);
    audio.stop(h);
    tick(audio, 50);
    phase = 1;
    h = audio.play("tone", 2, 0, true);
    tick(audio, 180);
    audio.stop(h);
    tick(audio, 50);
    h = audio.play("tone");
    audio.update(nullptr, true, true, 0);
    tick(audio, 40, true);
    phase = 2;
    tick(audio, 100, true);
    phase = 3;
    tick(audio, 120);
    audio.stop(h);
    tick(audio, 40);
    Music music = LoadMusicStream((path / "audio/tone.wav").c_str());
    check(IsMusicValid(music), "Stream decoded");
    music.looping = true;
    SetMusicVolume(music, .01f);
    PlayMusicStream(music);
    phase = 4;
    for (unsigned i = 0; i < 80; ++i) {
        UpdateMusicStream(music);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    check(IsMusicStreamPlaying(music), "Stream still playing after wrap");
    StopMusicStream(music);
    UnloadMusicStream(music);
    DetachAudioMixedProcessor(capture);
    for (unsigned i = 0; i < 5; ++i)
        std::cerr << "phase " << i << " L=" << left[i] << " R=" << rightEnergy[i] << "\n";
    check(left[0] > .00001 && rightEnergy[0] < left[0] * .01, "Actual mixer left pan");
    check(rightEnergy[1] > .00001 && left[1] < rightEnergy[1] * .01, "Actual mixer rightEnergy pan");
    check(left[2] + rightEnergy[2] < .000001, "Pause silent");
    check(left[3] + rightEnergy[3] > .00001, "Resume audible");
    check(left[4] + rightEnergy[4] > .00001, "Loop mixed");
    audio.shutdown();
    std::cout << "Real device: signed stereo energy, aliases, pause/resume, stream wrap and cleanup passed\n";
}
