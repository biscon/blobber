#include "platform/audio.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/hybrid_fixtures.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
using namespace blobber;
namespace fake {
struct Buffer {
    bool live = false, alias = false, playing = false, paused = false;
    unsigned parent = 0;
    float pan = 0, volume = 0;
};
std::array<Buffer, 4096> buffers{};
unsigned next = 1, plays = 0, streams = 0, updates = 0;
bool device = false, fail = false, aliasFail = false;
unsigned id(Sound s) {
    return unsigned(reinterpret_cast<std::uintptr_t>(s.stream.buffer));
}
Sound make(bool alias = false, unsigned parent = 0) {
    auto n = next++;
    check(n < buffers.size(), "Mock bound");
    buffers[n] = {true, alias, false, false, parent, 0, 0};
    Sound s{};
    s.frameCount = 4800;
    s.stream.buffer = reinterpret_cast<rAudioBuffer*>(std::uintptr_t(n));
    return s;
}
} // namespace fake
extern "C" {
void InitAudioDevice() {
    fake::device = !fake::fail;
}
bool IsAudioDeviceReady() {
    return fake::device;
}
void CloseAudioDevice() {
    for (const auto& b : fake::buffers)
        check(!b.live, "Device closes after buffers");
    check(!fake::streams, "Device closes after streams");
    fake::device = false;
}
Wave LoadWave(const char*) {
    static short data[4800]{};
    return {4800, 48000, 16, 1, data};
}
bool IsWaveValid(Wave w) {
    return w.data != nullptr;
}
void UnloadWave(Wave) {}
Sound LoadSoundFromWave(Wave) {
    return fake::make();
}
Sound LoadSoundAlias(Sound s) {
    return fake::aliasFail ? Sound{} : fake::make(true, fake::id(s));
}
bool IsSoundValid(Sound s) {
    return s.stream.buffer != nullptr;
}
void UnloadSoundAlias(Sound s) {
    auto& b = fake::buffers[fake::id(s)];
    check(b.alias && b.live, "Alias lifetime");
    b.live = false;
}
void UnloadSound(Sound s) {
    const auto n = fake::id(s);
    for (const auto& b : fake::buffers)
        check(!b.live || !b.alias || b.parent != n, "Aliases unload before source");
    fake::buffers[n].live = false;
}
void PlaySound(Sound s) {
    auto& b = fake::buffers[fake::id(s)];
    b.playing = true;
    b.paused = false;
    ++fake::plays;
}
void StopSound(Sound s) {
    fake::buffers[fake::id(s)].playing = false;
}
void PauseSound(Sound s) {
    fake::buffers[fake::id(s)].paused = true;
}
void ResumeSound(Sound s) {
    fake::buffers[fake::id(s)].paused = false;
}
bool IsSoundPlaying(Sound s) {
    auto& b = fake::buffers[fake::id(s)];
    return b.playing && !b.paused;
}
void SetSoundVolume(Sound s, float v) {
    fake::buffers[fake::id(s)].volume = v;
}
void SetSoundPitch(Sound, float) {}
void SetSoundPan(Sound s, float v) {
    fake::buffers[fake::id(s)].pan = v;
}
Music LoadMusicStream(const char*) {
    ++fake::streams;
    Music m{};
    m.frameCount = 48000;
    return m;
}
bool IsMusicValid(Music m) {
    return m.frameCount != 0;
}
void UnloadMusicStream(Music) {
    --fake::streams;
}
void PlayMusicStream(Music) {}
void StopMusicStream(Music) {}
void PauseMusicStream(Music) {}
void ResumeMusicStream(Music) {}
void SetMusicVolume(Music, float) {}
void SetMusicPan(Music, float) {}
void UpdateMusicStream(Music) {
    ++fake::updates;
}
}
int main() {
    const auto dir = std::filesystem::temp_directory_path() /
                     ("blobber-audio-runtime-" +
                      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir / "audio");
    {
        std::ofstream f(dir / "audio/sample.wav");
        f << "owned mock fixture";
    }
    {
        std::ofstream f(dir / "audio/bank.json");
        f << R"({"cues":{"test":{"cooldown":0,"concurrency":8,"variants":[{"id":"01","path":"sample.wav"}]},"ambience":{"loop":true,"category":2,"variants":[{"id":"01","path":"sample.wav"}]}}})";
    }
    AudioRuntime runtime;
    runtime.prepare(dir, true, dir / "settings.json");
    check(!runtime.ready(), "Disabled never opens device");
    fake::fail = true;
    runtime.prepare(dir, false, dir / "settings.json");
    check(!runtime.ready(), "Missing device nonfatal");
    fake::fail = false;
    runtime.prepare(dir, false, dir / "settings.json");
    check(runtime.ready(), "Prepared mock runtime");
    auto first = runtime.play("test");
    for (unsigned i = 0; i < 4; ++i)
        runtime.play("test");
    check(!runtime.stop(first), "Alias replacement invalidates handle");
    auto h = runtime.play("test");
    check(runtime.stop(h) && !runtime.stop(h), "Stop invalidates generation");
    const auto before = fake::plays;
    runtime.update(nullptr, true, true, .1f);
    check(runtime.play("test").slot == 64, "Paused world admission");
    runtime.play("test", 0, 0, false, true);
    check(fake::plays == before + 1, "Focused interface remains available");
    runtime.update(nullptr, true, false, .1f);
    check(runtime.play("test", 0, 0, false, true).slot == 64, "Focus loss blocks interface");
    runtime.update(nullptr, false, true, .1f);
    watching = true;
    allocations = 0;
    for (unsigned i = 0; i < 100; ++i) {
        runtime.update(nullptr, false, true, .01f);
        runtime.play("test");
    }
    watching = false;
    check(allocations == 0, "Prepared voice processing allocates no C++ memory");
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog();
    NpcWorld places(map, {5, 8}, {}, {});
    GameSession session(map, catalog, places, 5, {{5, 8}, Heading::North}, false);
    runtime.update(&session, false, true, .01f);
    session.audioEvents().emit({"test"});
    const auto old = fake::plays;
    runtime.update(&session, false, true, .01f);
    runtime.update(&session, false, true, .01f);
    check(fake::plays == old + 1, "Journal consumed once");
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 100; ++i) {
        session.update(.01, false);
        session.audioEvents().emit({"test"});
        runtime.update(&session, false, true, .01f);
    }
    watching = false;
    check(allocations == 0, "Observed session, journal, mix and ambience allocate no C++ memory");
    auto oldHandle = runtime.play("test");
    runtime.prepare(dir, false, dir / "settings.json");
    check(!runtime.stop(oldHandle), "Cross-runtime generation rejected");
    runtime.shutdown();
    runtime.shutdown();
    fake::aliasFail = true;
    runtime.prepare(dir, false, dir / "settings.json");
    check(runtime.play("test").slot == 64, "Alias failure degrades safely");
    runtime.shutdown();
    std::filesystem::remove_all(dir);
    std::cout << "Audio runtime ownership, failure and allocation checks passed\n";
}
