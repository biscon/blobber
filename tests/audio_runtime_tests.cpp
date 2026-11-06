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
unsigned lastPlayed = 0;
struct MusicBuffer {
    bool live = false, playing = false, paused = false;
    float position = 0, duration = 10, volume = 0;
};
std::array<MusicBuffer, 256> musicBuffers{};
unsigned musicNext = 1, lastMusic = 0, musicPlays = 0;
unsigned musicId(Music m) {
    return unsigned(reinterpret_cast<std::uintptr_t>(m.ctxData));
}
AudioCallback processor = nullptr;
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
    s.stream.sampleRate = 48000;
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
    check(!fake::processor, "Processor detached before device close");
    for (const auto& b : fake::buffers)
        check(!b.live, "Device closes after buffers");
    check(!fake::streams, "Device closes after streams");
    fake::device = false;
}
void AttachAudioMixedProcessor(AudioCallback processor) {
    check(fake::device && !fake::processor, "Exactly one limiter attached to prepared device");
    fake::processor = processor;
}
void DetachAudioMixedProcessor(AudioCallback processor) {
    check(fake::processor == processor, "Detach only the runtime's own limiter");
    fake::processor = nullptr;
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
    fake::lastPlayed = fake::id(s);
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
    m.frameCount = 480000;
    m.stream.sampleRate = 48000;
    auto n = fake::musicNext++;
    check(n < fake::musicBuffers.size(), "Mock stream bound");
    fake::musicBuffers[n] = {true, false, false, 0, 10, 0};
    m.ctxData = reinterpret_cast<void*>(std::uintptr_t(n));
    return m;
}
bool IsMusicValid(Music m) {
    return m.frameCount != 0;
}
void UnloadMusicStream(Music m) {
    auto& b = fake::musicBuffers[fake::musicId(m)];
    check(b.live && !b.playing, "Music stopped before unload");
    b.live = false;
    --fake::streams;
}
void PlayMusicStream(Music m) {
    auto& b = fake::musicBuffers[fake::musicId(m)];
    check(b.live, "Only prepared streams play");
    b.playing = true;
    b.paused = false;
    fake::lastMusic = fake::musicId(m);
    ++fake::musicPlays;
}
void StopMusicStream(Music m) {
    auto& b = fake::musicBuffers[fake::musicId(m)];
    b.playing = b.paused = false;
    b.position = 0;
}
void PauseMusicStream(Music m) {
    fake::musicBuffers[fake::musicId(m)].paused = true;
}
void ResumeMusicStream(Music m) {
    fake::musicBuffers[fake::musicId(m)].paused = false;
}
void SetMusicVolume(Music m, float v) {
    fake::musicBuffers[fake::musicId(m)].volume = v;
}
void SetMusicPan(Music, float) {}
bool IsMusicStreamPlaying(Music m) {
    auto& b = fake::musicBuffers[fake::musicId(m)];
    return b.playing && !b.paused;
}
float GetMusicTimePlayed(Music m) {
    return fake::musicBuffers[fake::musicId(m)].position;
}
float GetMusicTimeLength(Music m) {
    return fake::musicBuffers[fake::musicId(m)].duration;
}
void UpdateMusicStream(Music m) {
    auto& b = fake::musicBuffers[fake::musicId(m)];
    if (b.playing && !b.paused) {
        b.position += .01f;
        if (!m.looping && b.position >= b.duration)
            StopMusicStream(m);
    }
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
        f << R"({"mix_gain_db":{"effects":12},"cues":{"test":{"gain":0.8,"volume_min":1,"cooldown":0,"concurrency":8,"variants":[{"id":"01","path":"sample.wav"}]},"ambience":{"loop":true,"category":2,"variants":[{"id":"01","path":"sample.wav"}]}}})";
    }
    AudioRuntime runtime;
    runtime.prepare(dir, true, dir / "settings.json");
    check(!runtime.ready(), "Disabled never opens device");
    runtime.prepareDialogue({(dir / "audio/sample.wav").string()});
    check(fake::plays == 0, "Disabled dialogue preparation opens no device or voice");
    check(!fake::processor, "Disabled mode has no processor");
    fake::fail = true;
    runtime.prepare(dir, false, dir / "settings.json");
    check(!runtime.ready(), "Missing device nonfatal");
    runtime.prepareDialogue({(dir / "audio/sample.wav").string()});
    check(!fake::processor, "Missing device has no processor");
    fake::fail = false;
    runtime.prepare(dir, false, dir / "settings.json");
    check(runtime.ready(), "Prepared mock runtime");
    auto first = runtime.play("test");
    check(std::abs(fake::buffers[fake::lastPlayed].volume - .8f * .7f * .8f * 3.9810717f) < 1e-5f,
          "Runtime applies the prepared bus boost without clipping individual voice gain");
    runtime.settings.values[1] = 0;
    check(runtime.play("test").slot == 64, "Boost never defeats the Effects mute");
    runtime.settings.values[1] = .8f;
    float peak[]{12.f, -6.f};
    fake::processor(peak, 1);
    check(peak[0] < .842f && peak[1] > -.422f, "Final callback limits both channels together");
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
        float samples[]{4.f, -2.f, .1f, -.1f};
        fake::processor(samples, 2);
    }
    watching = false;
    check(allocations == 0, "Prepared voice processing allocates no C++ memory");
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog();
    NpcWorld places(map, {5, 8}, {}, {});
    GameSession session(map, catalog, places, 5, {{5, 8}, Heading::North}, false);
    runtime.update(&session, false, true, .01f);
    const auto ambient = fake::lastMusic;
    const auto ambientStarts = fake::musicPlays;
    const auto ambientVolume = fake::musicBuffers[ambient].volume;
    auto ambientPosition = fake::musicBuffers[ambient].position;
    runtime.play("test");
    const auto effect = fake::lastPlayed;
    runtime.update(&session, true, true, .01f, false, false, false);
    check(fake::musicBuffers[ambient].position > ambientPosition && !fake::musicBuffers[ambient].paused &&
              fake::musicPlays == ambientStarts && fake::musicBuffers[ambient].volume == ambientVolume,
          "Dialogue keeps ambience advancing at unchanged volume without restarting");
    check(fake::buffers[effect].paused && runtime.play("test").slot == 64,
          "Ambience override does not resume or admit gameplay effects");
    ambientPosition = fake::musicBuffers[ambient].position;
    runtime.update(&session, true, false, .01f, false, false, false);
    check(fake::musicBuffers[ambient].paused && fake::musicBuffers[ambient].position == ambientPosition,
          "Focus loss overrides dialogue ambience playback");
    runtime.update(&session, true, true, .01f, false, false, false);
    check(!fake::musicBuffers[ambient].paused && fake::musicBuffers[ambient].position > ambientPosition,
          "Refocusing resumes the same ambient stream during dialogue");
    ambientPosition = fake::musicBuffers[ambient].position;
    runtime.update(&session, true, true, .01f, true, false, true);
    check(fake::musicBuffers[ambient].paused && fake::musicBuffers[ambient].position == ambientPosition,
          "Another pause cause takes precedence over dialogue ambience");
    runtime.update(&session, true, true, .01f);
    check(fake::musicBuffers[ambient].paused && fake::musicBuffers[ambient].position == ambientPosition,
          "Existing menu/settings callers retain default ambient pause");
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 100; ++i)
        runtime.update(&session, true, true, .01f, false, false, false);
    watching = false;
    check(!allocations && fake::musicBuffers[ambient].position > ambientPosition,
          "Warmed dialogue ambience servicing advances without engine allocations");
    ambientPosition = fake::musicBuffers[ambient].position;
    runtime.update(&session, false, true, .01f);
    check(fake::musicBuffers[ambient].position > ambientPosition &&
              fake::musicBuffers[ambient].volume == ambientVolume && fake::musicPlays == ambientStarts &&
              !fake::buffers[effect].paused,
          "Dialogue exit continues ambience and resumes gameplay effects without a restart");
    runtime.prepareDialogue({(dir / "audio/sample.wav").string(), (dir / "missing.wav").string()});
    runtime.update(&session, true, true, .01f, false, false, false, false);
    const auto beforeDialogue = fake::plays;
    runtime.presentDialogue(1, 0, true);
    const auto reaction = fake::lastPlayed;
    check(fake::plays == beforeDialogue + 1 && !fake::buffers[reaction].paused,
          "Dialogue sound starts while gameplay effects stay paused");
    runtime.presentDialogue(1, 0, true);
    check(fake::plays == beforeDialogue + 1, "Repeated beat snapshot never replays audio");
    runtime.update(&session, true, false, .01f, false, false, false, false);
    check(fake::buffers[reaction].paused, "Focus loss pauses current dialogue voice");
    runtime.update(&session, true, true, .01f, false, false, false, false);
    check(!fake::buffers[reaction].paused && fake::plays == beforeDialogue + 1,
          "Refocus resumes reaction without restarting it");
    runtime.update(&session, true, true, .01f);
    check(fake::buffers[reaction].paused, "Other modal callers suspend dialogue sound by default");
    runtime.update(&session, true, true, .01f, false, false, false, false);
    runtime.presentDialogue(2, 64, true);
    check(!fake::buffers[reaction].playing && fake::plays == beforeDialogue + 1,
          "Silent next beat stops the outgoing reaction");
    runtime.presentDialogue(3, 1, true);
    check(fake::plays == beforeDialogue + 1, "Missing reaction sample degrades to silence");
    allocations = 0;
    watching = true;
    for (unsigned i = 4; i < 104; ++i) {
        runtime.update(&session, true, true, .01f, false, false, false, false);
        runtime.presentDialogue(i, 0, true);
        runtime.presentDialogue(i, 0, true);
    }
    watching = false;
    check(!allocations && fake::plays == beforeDialogue + 101,
          "Exactly-once beat audio and pause servicing allocate no C++ memory");
    const auto lastReaction = fake::lastPlayed;
    runtime.presentDialogue(104, 64, false);
    check(!fake::buffers[lastReaction].playing, "Dialogue exit stops voice immediately");
    runtime.presentDialogue(105, 0, true);
    runtime.detach();
    check(!fake::buffers[lastReaction].playing, "World detach stops dialogue voice");
    runtime.clearDialogue();
    check(!fake::buffers[lastReaction].live, "Dialogue aliases and sources release at preparation boundary");
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
    float quiet[]{.2f, -.1f};
    fake::processor(quiet, 1);
    check(quiet[0] == .2f && quiet[1] == -.1f, "Reprepare resets limiter history");
    runtime.shutdown();
    runtime.shutdown();
    fake::aliasFail = true;
    runtime.prepare(dir, false, dir / "settings.json");
    check(runtime.play("test").slot == 64, "Alias failure degrades safely");
    runtime.prepareDialogue({(dir / "audio/sample.wav").string()});
    runtime.update(nullptr, true, true, .01f, true, false, {}, false);
    const auto aliasFailurePlays = fake::plays;
    runtime.presentDialogue(1, 0, true);
    check(fake::plays == aliasFailurePlays, "Dialogue alias failure degrades safely and frees its source");
    runtime.shutdown();
    fake::aliasFail = false;
    std::filesystem::create_directories(dir / "audio/music");
    std::ofstream(dir / "audio/music/a.mp3") << "mock-owned fixture";
    std::ofstream(dir / "audio/music/b.mp3") << "mock-owned fixture";
    std::ofstream(dir / "audio/music/playlist.json")
        << R"({"tracks":[{"id":"a","path":"a.mp3","title":"First Track","artist":"Fixture","gain_db":-6},{"id":"b","path":"b.mp3","title":"Second Track","artist":"Fixture","gain_db":-6}]})";
    runtime.prepare(dir, false, dir / "settings.json");
    runtime.update(&session, false, true, 0, false);
    CombatOutcome start;
    start.event = CombatEvent::Started;
    session.audioEvents().combat(start);
    const auto initialPlays = fake::musicPlays;
    runtime.update(&session, false, true, .01f, false);
    check(fake::musicPlays == initialPlays + 1 && runtime.musicBanner().visible,
          "Encounter starts prepared music and banner once");
    const auto song = fake::lastMusic;
    check(std::abs(fake::musicBuffers[song].volume * .6875f -
                   runtime.settings.values[0] * runtime.settings.musicVolume * std::pow(10.f, -6.f / 20.f)) <
              1e-6f,
          "Music compensates raylib center loss while preserving master, slider and manual trim");
    const auto bannerRevision = runtime.musicBanner().revision;
    session.audioEvents().combat(start);
    runtime.update(&session, true, true, .01f, false, false);
    check(fake::musicPlays == initialPlays + 1 && !fake::musicBuffers[song].paused &&
              !runtime.musicBanner().visible,
          "Repeated encounter and tactical modal preserve playback; banner defers");
    runtime.update(&session, true, true, .01f, true);
    check(fake::musicBuffers[song].paused, "Explicit menu pauses music");
    const auto position = fake::musicBuffers[song].position;
    runtime.update(&session, true, true, .01f, true);
    check(fake::musicBuffers[song].position == position, "Paused decoder is not serviced");
    runtime.settings.musicEnabled = false;
    runtime.update(&session, false, true, .01f, false);
    check(fake::musicBuffers[song].paused && !runtime.musicBanner().visible, "Toggle off freezes and hides");
    runtime.settings.musicEnabled = true;
    runtime.update(&session, false, true, .01f, false);
    check(!fake::musicBuffers[song].paused && runtime.musicBanner().revision == bannerRevision,
          "Same encounter resumes without announcement restart");
    fake::musicBuffers[song].position = 10;
    allocations = 0;
    watching = true;
    runtime.update(&session, false, true, .01f, false);
    watching = false;
    check(allocations == 0 && fake::lastMusic != song && runtime.musicBanner().revision > bannerRevision,
          "EOF advances to next prepared stream without C++ allocation");
    const auto second = fake::lastMusic;
    fake::musicBuffers[second].position = 9.8f;
    CombatOutcome finish;
    finish.event = CombatEvent::Finished;
    session.audioEvents().combat(finish);
    runtime.update(&session, false, true, .1f, false);
    check(fake::musicBuffers[second].volume > 0 && fake::musicBuffers[second].volume < .7f * .5f * .5012f,
          "Short remaining track starts attenuating");
    runtime.update(&session, false, true, .1f, false);
    runtime.update(&session, false, true, .1f, false);
    check(!fake::musicBuffers[second].playing && !runtime.musicBanner().visible,
          "Fade stops without starting another song");
    // Attach after a real, owned session has already entered initiative. This is
    // the same presentation adoption path used after restoring an active save.
    NpcWorld contactPlaces(map, {5, 8}, {"goblin"}, {{{0}, {5, 7}, {0, 1}, Heading::South}});
    GameSession active(map, catalog, contactPlaces, 8, {{5, 8}, Heading::North}, false);
    active.setAnimated(false);
    for (unsigned i = 0; i < 20 && active.combat().snapshot().round == 0; ++i)
        active.update(.1, false);
    check(active.combat().snapshot().round > 0, "Owned session reaches real initiative");
    active.audioEvents().encounterActive = false; // Imported simulation has no audio transport history.
    const auto beforeAttach = fake::musicPlays;
    runtime.update(&active, false, true, 0, false);
    runtime.update(&active, false, true, .01f, false);
    check(fake::musicPlays == beforeAttach + 2 && runtime.musicBanner().visible,
          "Attach adopts active combat once (one ambience start and one song)");
    const auto afterAttach = fake::musicPlays;
    runtime.update(&active, false, true, .01f, false);
    check(fake::musicPlays == afterAttach, "Repeated restored session snapshot does not restart music");
    runtime.persistMusic();
    check(std::filesystem::is_regular_file(dir / "music-state.json"), "Explicit boundary persists playlist");
    runtime.shutdown();
    std::filesystem::remove_all(dir);
    std::cout << "Audio runtime ownership, failure and allocation checks passed\n";
}
