#include "app/loading.hpp"
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
    std::string path;
};
std::array<MusicBuffer, 256> musicBuffers{};
unsigned musicNext = 1, lastMusic = 0, musicPlays = 0;
const auto audioThread = std::this_thread::get_id();
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
Music LoadMusicStream(const char* path) {
    ++fake::streams;
    Music m{};
    m.frameCount = 480000;
    m.stream.sampleRate = 48000;
    auto n = fake::musicNext++;
    check(n < fake::musicBuffers.size(), "Mock stream bound");
    fake::musicBuffers[n] = {true, false, false, 0, 10, 0, path};
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
    check(std::this_thread::get_id() == fake::audioThread, "Music servicing stays on the window thread");
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
    for (const char* bank : {"sanctuary", "mainmenu"}) {
        const auto root = dir / "audio/music" / bank;
        std::filesystem::create_directories(root);
        std::ofstream(root / "a.mp3") << "owned mock song";
        std::ofstream(root / "playlist.json")
            << R"({"tracks":[{"id":"a","path":"a.mp3","title":"Owned Song","artist":"Fixture"}]})";
    }
    runtime.prepare(dir, false, dir / "settings.json");
    runtime.update(&session, false, true, 0, false);
    const auto dungeonAmbient = fake::lastMusic;
    const auto ambientBeforeMusic = fake::musicBuffers[dungeonAmbient].position;
    check(fake::musicBuffers[dungeonAmbient].path == (dir / "audio/sample.wav").string() &&
              !fake::musicBuffers[dungeonAmbient].paused,
          "Dungeon ambience plays before combat music starts");
    CombatOutcome start;
    start.event = CombatEvent::Started;
    session.audioEvents().combat(start);
    const auto initialPlays = fake::musicPlays;
    runtime.update(&session, false, true, .01f, false);
    check(fake::musicPlays == initialPlays + 1 && runtime.musicBanner().visible,
          "Encounter starts prepared music and banner once");
    check(fake::musicBuffers[dungeonAmbient].paused &&
              fake::musicBuffers[dungeonAmbient].position == ambientBeforeMusic,
          "Combat music pauses ambience before it can advance in the same update");
    const auto effectDuringMusic = runtime.play("test");
    check(effectDuringMusic.slot < 64 && fake::buffers[fake::lastPlayed].playing,
          "Combat music excludes ambience while preserving one-shot effects");
    runtime.stop(effectDuringMusic);
    const auto song = fake::lastMusic;
    const auto combatPosition = fake::musicBuffers[song].position;
    runtime.beginTravelMusic(true);
    runtime.pumpLoadingMusic(true);
    runtime.endTravelMusic();
    check(fake::musicBuffers[song].position == combatPosition,
          "Loading pump does not service combat music even with a sanctuary destination");
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
    check(fake::musicBuffers[dungeonAmbient].paused, "Menu pause keeps ambience suspended too");
    const auto position = fake::musicBuffers[song].position;
    runtime.update(&session, true, true, .01f, true);
    check(fake::musicBuffers[song].position == position, "Paused decoder is not serviced");
    runtime.settings.musicEnabled = false;
    runtime.update(&session, false, true, .01f, false);
    check(fake::musicBuffers[song].paused && !runtime.musicBanner().visible, "Toggle off freezes and hides");
    check(!fake::musicBuffers[dungeonAmbient].paused &&
              fake::musicBuffers[dungeonAmbient].position > ambientBeforeMusic,
          "Muted combat music permits the same ambient stream to resume");
    runtime.settings.musicEnabled = true;
    runtime.update(&session, false, true, .01f, false);
    check(!fake::musicBuffers[song].paused && runtime.musicBanner().revision == bannerRevision,
          "Same encounter resumes without announcement restart");
    check(fake::musicBuffers[dungeonAmbient].paused, "Unmuting combat music pauses ambience again");
    const float musicVolume = runtime.settings.musicVolume;
    runtime.settings.musicVolume = 0;
    runtime.update(&session, false, true, .01f, false);
    check(!fake::musicBuffers[dungeonAmbient].paused, "Zero Music volume restores dungeon ambience");
    runtime.settings.musicVolume = musicVolume;
    runtime.update(&session, false, true, .01f, false);
    check(fake::musicBuffers[dungeonAmbient].paused && fake::musicPlays == initialPlays + 1,
          "Restoring Music volume suppresses ambience without restarting either stream");
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 4; ++i) {
        runtime.settings.musicEnabled = false;
        runtime.update(&session, false, true, .01f, false);
        runtime.settings.musicEnabled = true;
        runtime.update(&session, false, true, .01f, false);
    }
    watching = false;
    check(!allocations && fake::musicBuffers[dungeonAmbient].paused && fake::musicPlays == initialPlays + 1,
          "Warmed ambience/music switching allocates no engine C++ memory or restarts streams");
    fake::musicBuffers[song].position = 10;
    allocations = 0;
    watching = true;
    runtime.update(&session, false, true, .01f, false);
    watching = false;
    check(allocations == 0 && fake::lastMusic != song && runtime.musicBanner().revision > bannerRevision,
          "EOF advances to next prepared stream without C++ allocation");
    check(fake::musicBuffers[dungeonAmbient].paused, "Track rollover does not leak ambience");
    const auto second = fake::lastMusic;
    fake::musicBuffers[second].position = 9.8f;
    CombatOutcome finish;
    finish.event = CombatEvent::Finished;
    session.audioEvents().combat(finish);
    const auto ambientBeforeFade = fake::musicBuffers[dungeonAmbient].position;
    runtime.update(&session, false, true, .1f, false);
    check(fake::musicBuffers[second].volume > 0 && fake::musicBuffers[second].volume < .7f * .5f * .5012f,
          "Short remaining track starts attenuating");
    check(fake::musicBuffers[dungeonAmbient].paused &&
              fake::musicBuffers[dungeonAmbient].position == ambientBeforeFade,
          "Ambience stays paused throughout audible combat-music fade");
    runtime.update(&session, false, true, .1f, false);
    runtime.update(&session, false, true, .1f, false);
    check(!fake::musicBuffers[second].playing && !runtime.musicBanner().visible,
          "Fade stops without starting another song");
    check(!fake::musicBuffers[dungeonAmbient].paused &&
              fake::musicBuffers[dungeonAmbient].position > ambientBeforeFade &&
              fake::musicPlays == initialPlays + 2,
          "Fade completion resumes the held ambient stream without rewinding or replaying it");
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
    check(fake::musicPlays == beforeAttach + 1 && runtime.musicBanner().visible,
          "Attach adopts active combat once without briefly starting dungeon ambience");
    const auto afterAttach = fake::musicPlays;
    runtime.update(&active, false, true, .01f, false);
    check(fake::musicPlays == afterAttach, "Repeated restored session snapshot does not restart music");
    runtime.persistMusic();
    check(std::filesystem::is_regular_file(dir / "music-state.json"), "Explicit boundary persists playlist");

    const auto ambientPlaying = [&] {
        for (const auto& stream : fake::musicBuffers)
            if (stream.live && stream.playing && stream.path == (dir / "audio/sample.wav").string())
                return true;
        return false;
    };
    const auto songsPlaying = [&] {
        unsigned count = 0;
        for (const auto& stream : fake::musicBuffers)
            count += stream.live && stream.playing && stream.path.find("/audio/music/") != std::string::npos;
        return count;
    };
    runtime.setGameplayMusic(CampaignLocationId::City);
    runtime.update(&session, false, true, .01f, false);
    const auto townSong = fake::lastMusic;
    const auto townStarts = fake::musicPlays;
    check(fake::musicBuffers[townSong].path == (dir / "audio/music/sanctuary/a.mp3").string() &&
              runtime.musicBanner().visible && !ambientPlaying() && songsPlaying() == 1,
          "Sanctuary starts its own music without ambient hiss");
    auto townPosition = fake::musicBuffers[townSong].position;
    runtime.play("test");
    const auto sourceVoice = fake::lastPlayed;
    for (const auto location : {CampaignLocationId::Inn, CampaignLocationId::Smithy,
                                CampaignLocationId::Emporium, CampaignLocationId::City}) {
        runtime.setGameplayMusic(location);
        // Alternate owned worlds to model committed doorway replacement.
        auto* destination = location == CampaignLocationId::Inn || location == CampaignLocationId::Emporium
                                ? &active
                                : &session;
        runtime.update(destination, true, true, 0, true, false);
        runtime.update(destination, false, true, .01f, false);
        runtime.update(destination, false, true, .01f, false);
        check(fake::musicPlays == townStarts && fake::lastMusic == townSong &&
                  fake::musicBuffers[townSong].position > townPosition && !ambientPlaying(),
              "Every town doorway preserves song position and excludes ambience");
        townPosition = fake::musicBuffers[townSong].position;
    }
    check(!fake::buffers[sourceVoice].playing, "Town doorway replacement still stops old-world voices");
    runtime.update(&session, true, true, .01f, false, false);
    check(!runtime.musicBanner().visible && runtime.musicBanner().age == 6 &&
              fake::musicBuffers[townSong].position > townPosition,
          "Dialogue/inventory discards sanctuary announcement while song advances");
    // A failed travel leaves the source world/location attached.
    runtime.update(&session, true, true, 0, true, false);
    runtime.update(&session, false, true, .01f, false);
    check(fake::lastMusic == townSong && fake::musicPlays == townStarts && !runtime.musicBanner().visible,
          "Travel rollback resumes source song without resurrecting its announcement");
    townPosition = fake::musicBuffers[townSong].position;
    runtime.update(&session, false, false, .01f, false);
    const auto townUnfocusedPosition = fake::musicBuffers[townSong].position;
    runtime.update(&session, false, false, .01f, false);
    check(townUnfocusedPosition == townPosition && fake::musicBuffers[townSong].paused &&
              fake::musicBuffers[townSong].position == townUnfocusedPosition,
          "Focus loss suspends sanctuary music");
    runtime.settings.musicEnabled = false;
    runtime.update(&session, false, true, .01f, false);
    check(fake::musicBuffers[townSong].paused && !ambientPlaying() && !runtime.musicBanner().visible,
          "Muted sanctuary stays free of hiss and announcements");
    runtime.settings.musicEnabled = true;
    runtime.update(&session, false, true, .01f, false);
    fake::musicBuffers[townSong].position = 10;
    allocations = 0;
    watching = true;
    runtime.update(&session, false, true, .01f, false);
    watching = false;
    check(!allocations && fake::musicPlays == townStarts + 1 && runtime.musicBanner().age == 0,
          "Sanctuary advances at EOF without engine allocation, even in a one-song bank");
    const auto normalTownVolume = fake::musicBuffers[townSong].volume;
    runtime.worldGain = 0;
    {
        TravelMusicScope travel(&runtime, true);
        runtime.update(&session, true, true, 0, true, false);
        check(!fake::musicBuffers[townSong].paused && fake::musicBuffers[townSong].volume == normalTownVolume,
              "Town doorway music keeps normal volume through a black travel frame");
        const auto loadingPosition = fake::musicBuffers[townSong].position;
        const auto loadingAge = runtime.musicBanner().age;
        allocations = 0;
        watching = true;
        for (unsigned i = 0; i < 8; ++i)
            runtime.pumpLoadingMusic(true);
        watching = false;
        check(!allocations && fake::musicBuffers[townSong].position > loadingPosition &&
                  fake::musicBuffers[townSong].volume == normalTownVolume && !runtime.musicBanner().visible &&
                  runtime.musicBanner().age == loadingAge,
              "Loading pump advances existing music at normal gain without allocation or announcement");
        {
            PreparationWorker worker;
            std::promise<void> gate;
            auto ready = gate.get_future();
            struct Pump {
                AudioRuntime& audio;
                std::promise<void>& gate;
                unsigned calls = 0;
            } pump{runtime, gate};
            PreparationScope scope({[](void* owner, const char*) {
                                        auto& pump = *static_cast<Pump*>(owner);
                                        // Release before pumping so a failed assertion cannot strand the
                                        // worker.
                                        if (!pump.calls++)
                                            pump.gate.set_value();
                                        pump.audio.pumpLoadingMusic(true);
                                    },
                                    &pump, &worker});
            const auto beforeWorker = fake::musicBuffers[townSong].position;
            const auto workerThread = prepareCpu([&] {
                ready.wait();
                return std::this_thread::get_id();
            });
            check(pump.calls && workerThread != fake::audioThread &&
                      fake::musicBuffers[townSong].position > beforeWorker,
                  "Threaded preparation pumps sanctuary samples through main-thread checkpoints");
        }
        runtime.pumpLoadingMusic(false);
        const auto unfocused = fake::musicBuffers[townSong].position;
        runtime.pumpLoadingMusic(false);
        check(fake::musicBuffers[townSong].paused && fake::musicBuffers[townSong].position == unfocused,
              "Loading music respects focus loss");
        runtime.pumpLoadingMusic(true);
        runtime.pumpLoadingMusic(true);
        runtime.settings.musicEnabled = false;
        runtime.pumpLoadingMusic(true);
        const auto muted = fake::musicBuffers[townSong].position;
        runtime.pumpLoadingMusic(true);
        check(fake::musicBuffers[townSong].paused && fake::musicBuffers[townSong].position == muted,
              "Loading music respects the Music toggle");
        runtime.settings.musicEnabled = true;
        runtime.pumpLoadingMusic(true);
        runtime.pumpLoadingMusic(true);
        const auto loadingVolume = runtime.settings.musicVolume;
        runtime.settings.musicVolume = 0;
        runtime.pumpLoadingMusic(true);
        check(fake::musicBuffers[townSong].paused, "Loading music respects zero Music volume");
        runtime.settings.musicVolume = loadingVolume;
        runtime.pumpLoadingMusic(true);
        runtime.pumpLoadingMusic(true);
        const auto beforeEof = fake::musicPlays;
        fake::musicBuffers[townSong].position = 10;
        runtime.pumpLoadingMusic(true);
        check(fake::musicPlays == beforeEof + 1 && fake::lastMusic == townSong &&
                  fake::musicBuffers[townSong].position < .1f && !runtime.musicBanner().visible,
              "Playlist rollover continues during loading and defers the new announcement");
        runtime.setGameplayMusic(CampaignLocationId::Emporium);
        runtime.update(&active, true, true, 0, true, false);
        runtime.worldGain = .5f;
        runtime.update(&active, true, true, 0, true, false);
        check(!fake::musicBuffers[townSong].paused &&
                  fake::musicBuffers[townSong].volume == normalTownVolume &&
                  fake::musicPlays == beforeEof + 1,
              "Committed shop replacement and fade-in preserve the same song and music gain");
        runtime.worldGain = 1;
    }
    runtime.update(&active, true, true, 0, true, false);
    const auto outsideTravel = fake::musicBuffers[townSong].position;
    runtime.pumpLoadingMusic(true);
    check(fake::musicBuffers[townSong].paused && fake::musicBuffers[townSong].position == outsideTravel,
          "Travel scope exit restores explicit pause and disables loading pumping");
    runtime.update(&active, false, true, .01f, false);
    runtime.update(&active, false, true, .01f, false);
    {
        TravelMusicScope travel(&runtime, false);
        runtime.worldGain = 0;
        runtime.update(&active, true, true, 0, true, false);
        const auto leavingTown = fake::musicBuffers[townSong].position;
        runtime.pumpLoadingMusic(true);
        check(fake::musicBuffers[townSong].paused && fake::musicBuffers[townSong].position == leavingTown,
              "Non-sanctuary travel keeps its existing music fade and suspension");
        runtime.worldGain = 1;
    }
    runtime.setGameplayMusic(CampaignLocationId::FloorOne);
    runtime.update(&session, false, true, .01f, false);
    check(!fake::musicBuffers[townSong].playing && ambientPlaying(),
          "Leaving sanctuary stops town music and restores dungeon ambience");
    const auto beforeReturn = fake::musicPlays;
    runtime.setGameplayMusic(CampaignLocationId::City);
    runtime.update(&session, false, true, .01f, false);
    check(fake::musicPlays == beforeReturn + 1 && !ambientPlaying(),
          "Returning to sanctuary starts a new song and stops dungeon hiss");
    runtime.persistMusic();
    check(std::filesystem::is_regular_file(dir / "sanctuary-music-state.json"),
          "Sanctuary persists independently of combat");

    runtime.detach();
    runtime.beginMenuMusic();
    runtime.update(nullptr, false, true, .01f);
    const auto menuSong = fake::lastMusic;
    const auto menuStarts = fake::musicPlays;
    check(fake::musicBuffers[menuSong].path == (dir / "audio/music/mainmenu/a.mp3").string() &&
              !runtime.musicBanner().visible && !ambientPlaying() && songsPlaying() == 1,
          "Menu starts its own song with no announcement or world ambience");
    const auto menuPosition = fake::musicBuffers[menuSong].position;
    // Menu Settings uses paused=true/musicPaused=true with no GameSession.
    runtime.update(nullptr, true, true, .01f, true, false);
    check(fake::musicBuffers[menuSong].position > menuPosition && fake::musicPlays == menuStarts,
          "Connected menus and Settings continue the existing menu song");
    runtime.update(nullptr, false, false, .01f);
    const auto unfocusedPosition = fake::musicBuffers[menuSong].position;
    runtime.update(nullptr, false, false, .01f);
    check(fake::musicBuffers[menuSong].paused && fake::musicBuffers[menuSong].position == unfocusedPosition,
          "Menu music respects focus suspension");
    runtime.settings.musicEnabled = false;
    runtime.update(nullptr, false, true, .01f);
    check(fake::musicBuffers[menuSong].paused, "Menu music respects shared mute");
    runtime.settings.musicEnabled = true;
    runtime.update(nullptr, false, true, .01f);
    fake::musicBuffers[menuSong].position = 10;
    runtime.settings.musicEnabled = false;
    runtime.update(nullptr, false, true, .01f);
    check(fake::musicBuffers[menuSong].position == 10 && fake::musicBuffers[menuSong].paused,
          "Muting at the menu EOF boundary freezes decoder without losing completion");
    runtime.settings.musicEnabled = true;
    runtime.update(nullptr, false, true, .01f);
    runtime.update(nullptr, false, true, .01f);
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 100; ++i)
        runtime.update(nullptr, true, true, .01f);
    watching = false;
    check(!allocations && !fake::musicBuffers[menuSong].playing && fake::musicPlays == menuStarts,
          "Finished menu stays silent without allocation or repeated playback");
    runtime.endMenuMusic();
    runtime.update(nullptr, false, true, .01f);
    check(fake::musicPlays == menuStarts, "Launch boundary does not restart menu music");
    runtime.setGameplayMusic(std::nullopt);
    runtime.update(&active, false, true, .01f, false);
    runtime.detach();
    runtime.beginMenuMusic();
    runtime.update(nullptr, false, true, .01f);
    check(fake::lastMusic == menuSong && fake::musicBuffers[menuSong].position < .1f &&
              fake::musicPlays > menuStarts,
          "Return from gameplay restarts menu song from the beginning");
    runtime.endMenuMusic();
    check(!fake::musicBuffers[menuSong].playing, "Menu music stops before launch loading");
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 8; ++i) {
        runtime.setGameplayMusic(CampaignLocationId::City);
        runtime.update(&session, false, true, .01f, false);
        runtime.setGameplayMusic(CampaignLocationId::Inn);
        runtime.update(&active, false, true, .01f, false);
        runtime.setGameplayMusic(CampaignLocationId::FloorOne);
        runtime.update(&session, false, true, .01f, false);
        runtime.detach();
        runtime.beginMenuMusic();
        runtime.update(nullptr, false, true, .01f);
        runtime.endMenuMusic();
    }
    watching = false;
    check(!allocations, "Warmed town/world/menu transitions allocate no engine C++ memory");
    runtime.shutdown();
    std::filesystem::remove(dir / "audio/music/playlist.json");
    runtime.prepare(dir, false, dir / "settings.json");
    runtime.setGameplayMusic(CampaignLocationId::FloorOne);
    runtime.update(&active, false, true, .01f, false);
    check(ambientPlaying() && !fake::musicBuffers[fake::lastMusic].paused && songsPlaying() == 0 &&
              !runtime.musicBanner().visible,
          "Missing combat playlist leaves ambience audible even during an active encounter");
    runtime.shutdown();

    // A multi-song sanctuary fixture exercises selection and decoder restarts,
    // unlike the intentional one-song repeat fixture above.
    const auto townRoot = dir / "audio/music/sanctuary";
    std::ofstream(townRoot / "b.mp3") << "owned second song";
    std::ofstream(townRoot / "c.mp3") << "owned third song";
    std::ofstream(townRoot / "playlist.json")
        << R"({"tracks":[{"id":"a","path":"a.mp3","title":"A","artist":"Fixture"},{"id":"b","path":"b.mp3","title":"B","artist":"Fixture"},{"id":"c","path":"c.mp3","title":"C","artist":"Fixture"}]})";
    std::filesystem::create_directories(dir / "multi");
    std::ofstream(dir / "multi/sanctuary-music-state.json")
        << R"({"version":1,"order":["a","b","c"],"cursor":0,"random":42,"last":""})";
    runtime.prepare(dir, false, dir / "multi/settings.json");
    runtime.setGameplayMusic(CampaignLocationId::City);
    runtime.update(&session, false, true, .01f, false);
    check(runtime.musicBanner().visible, "Fresh visible sanctuary song announces normally");
    auto previousSong = fake::lastMusic;
    std::array<bool, 256> cycleSongs{};
    cycleSongs[previousSong] = true;
    for (unsigned selection = 1; selection < 12; ++selection) {
        const auto starts = fake::musicPlays;
        // Dialogue keeps playback running, but discards even a partly shown banner.
        runtime.update(&session, true, true, .01f, false, false);
        check(!runtime.musicBanner().visible, "Dialogue hides the current announcement");
        fake::musicBuffers[previousSong].position = 10;
        const bool hidden = selection % 4 != 0;
        allocations = 0;
        watching = true;
        runtime.update(&session, hidden, true, .01f, false, !hidden);
        watching = false;
        const auto selectedSong = fake::lastMusic;
        check(!allocations && selectedSong != previousSong && fake::musicPlays == starts + 1 &&
                  songsPlaying() == 1 && !fake::musicBuffers[previousSong].playing,
              "Sanctuary EOF starts exactly one different song, including shuffle wrap during dialogue");
        if (selection % 3 == 0)
            cycleSongs.fill(false);
        check(!cycleSongs[selectedSong], "Every sanctuary song plays once within each shuffle cycle");
        cycleSongs[selectedSong] = true;
        check(runtime.musicBanner().visible == !hidden, "Only songs starting on a visible screen announce");
        const auto positionBeforeExit = fake::musicBuffers[selectedSong].position;
        runtime.update(&session, false, true, .01f, false);
        check(runtime.musicBanner().visible == !hidden && fake::musicPlays == starts + 1 &&
                  fake::musicBuffers[selectedSong].position > positionBeforeExit,
              "Dialogue exit neither delays an announcement nor replays the song");
        previousSong = selectedSong;
    }
    const auto lastTownPath = fake::musicBuffers[previousSong].path;
    runtime.persistMusic();
    runtime.shutdown();
    runtime.prepare(dir, false, dir / "multi/settings.json");
    runtime.setGameplayMusic(CampaignLocationId::City);
    runtime.update(&session, false, true, .01f, false);
    check(fake::musicBuffers[fake::lastMusic].path != lastTownPath && runtime.musicBanner().visible,
          "Persisted sanctuary shuffle wraps to a different song after reattachment");
    runtime.shutdown();
    std::filesystem::remove_all(dir);
    std::cout << "Audio runtime ownership, failure and allocation checks passed\n";
}
