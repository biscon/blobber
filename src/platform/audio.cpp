#include "platform/audio.hpp"
#include "audio/limiter.hpp"
#include "render/world_scene.hpp"
#include <atomic>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
namespace blobber {
namespace {
std::atomic<std::uint64_t> audioOwner{1};
audio::PeakLimiter outputLimiter;
void limitOutput(void* samples, unsigned frames) {
    outputLimiter.process(static_cast<float*>(samples), frames);
}
} // namespace
AudioRuntime::~AudioRuntime() {
    shutdown();
}
unsigned AudioRuntime::cue(const char* id) const noexcept {
    for (unsigned i = 0; i < bank_.cues.size(); ++i)
        if (bank_.cues[i].id == id)
            return i;
    return unsigned(bank_.cues.size());
}
void AudioRuntime::prepare(const std::filesystem::path& assets, bool disabled,
                           const std::filesystem::path& config) {
    shutdown();
    warning_.clear();
    owner_ = audioOwner.fetch_add(1);
    root_ = assets / "audio";
    settingsPath_ = audio::settingsPath(config);
    settings = audio::loadSettings(settingsPath_, warning_);
    if (disabled)
        return;
    InitAudioDevice();
    ready_ = IsAudioDeviceReady();
    if (!ready_) {
        warning_ = "Audio device unavailable; continuing silently.";
        std::cerr << warning_ << '\n';
        return;
    }
    bank_ = audio::loadBank(root_);
    warning_ += bank_.warning;
    samples_.reserve(512);
    cues_.resize(bank_.cues.size());
    std::uint64_t bytes = 0;
    for (unsigned i = 0; i < bank_.cues.size(); ++i) {
        const auto& c = bank_.cues[i];
        auto& p = cues_[i];
        p.samples.fill(512);
        p.variation.random += i * 7919;
        if (c.loop)
            continue;
        for (unsigned j = 0; j < c.variants.size(); ++j) {
            const auto& v = c.variants[j];
            unsigned n = 0;
            for (; n < samples_.size(); ++n)
                if (samples_[n].path == v.path)
                    break;
            if (n == samples_.size()) {
                if (n == 512) {
                    warning_ += "Sample capacity exceeded\n";
                    continue;
                }
                Sample sample;
                sample.path = v.path;
                const auto path = (root_ / v.path).u8string();
                if (!std::filesystem::is_regular_file(path)) {
                    warning_ += "Missing " + path + "\n";
                    continue;
                }
                Wave w = LoadWave(path.c_str());
                // raylib converts resident samples to device-rate stereo float.
                const auto cost = std::uint64_t(w.frameCount) * 8 * 48000 / std::max(1u, w.sampleRate);
                if (!IsWaveValid(w) || cost > 64 * 1024 * 1024 - bytes) {
                    if (w.data)
                        UnloadWave(w);
                    warning_ += "Invalid/oversized " + path + "\n";
                    continue;
                }
                sample.source = LoadSoundFromWave(w);
                UnloadWave(w);
                if (!IsSoundValid(sample.source)) {
                    warning_ += "Cannot load " + path + "\n";
                    continue;
                }
                bytes += cost;
                for (unsigned k = 0; k < c.aliases; ++k) {
                    const auto alias = LoadSoundAlias(sample.source);
                    if (!IsSoundValid(alias))
                        break;
                    sample.voices[sample.count++] = alias;
                }
                samples_.push_back(std::move(sample));
            }
            // Shared paths honor the largest requested alias count at preparation.
            while (samples_[n].count < c.aliases) {
                auto alias = LoadSoundAlias(samples_[n].source);
                if (!IsSoundValid(alias))
                    break;
                samples_[n].voices[samples_[n].count++] = alias;
            }
            p.samples[j] = n;
        }
    }
    residentBytes_ = bytes;
    for (unsigned i = 0; i < streams_.size(); ++i) {
        auto& s = streams_[i];
        s.cue = cue(i < 2 ? "ambience" : i < 6 ? "web.loop" : "spiritual.loop");
        if (s.cue == bank_.cues.size())
            continue;
        const auto path = (root_ / bank_.cues[s.cue].variants.front().path).u8string();
        s.music = LoadMusicStream(path.c_str());
        s.valid = IsMusicValid(s.music);
        if (s.valid)
            s.music.looping = true;
    }
    music_.prepare(root_ / "music",
                   settingsPath_.empty() ? std::filesystem::path{}
                                         : settingsPath_.parent_path() / "music-state.json",
                   warning_);
    sanctuaryMusic_.prepare(root_ / "music/sanctuary",
                            settingsPath_.empty()
                                ? std::filesystem::path{}
                                : settingsPath_.parent_path() / "sanctuary-music-state.json",
                            warning_);
    menuMusic_.prepare(root_ / "music/mainmenu", {}, warning_, audio::MusicPolicy::Once);
    unsigned sampleRate = samples_.empty() ? 0 : samples_.front().source.stream.sampleRate;
    if (!sampleRate) {
        float silence = 0;
        Wave probe{1, 48000, 32, 1, &silence};
        auto sound = LoadSoundFromWave(probe);
        if (IsSoundValid(sound)) {
            sampleRate = sound.stream.sampleRate;
            UnloadSound(sound);
        }
    }
    outputLimiter.reset(sampleRate ? sampleRate : 48000);
    AttachAudioMixedProcessor(limitOutput);
    limiterAttached_ = true;
    if (!warning_.empty())
        std::cerr << warning_ << '\n';
}
float AudioRuntime::gain(unsigned category, bool interface) const noexcept {
    float g = settings.values[0] * settings.values[category] * bank_.categoryGain[category] *
              (interface ? 1.f : worldGain);
    if (category == 4)
        g *= settings.values[1];
    return g;
}
void AudioRuntime::release(unsigned i) noexcept {
    auto& v = voices_[i];
    if (v.active && v.sample < samples_.size())
        StopSound(samples_[v.sample].voices[v.alias]);
    v.active = false;
    v.paused = false;
    if (v.generation != UINT64_MAX)
        ++v.generation;
    else
        v.generation = 0;
}
bool AudioRuntime::stop(audio::Handle h) noexcept {
    if (h.owner != owner_ || h.slot >= 64 || !voices_[h.slot].active ||
        voices_[h.slot].generation != h.generation)
        return false;
    release(h.slot);
    return true;
}
audio::Handle AudioRuntime::play(const char* id, float x, float z, bool positional, bool interface,
                                 unsigned priority) {
    const auto c = cue(id);
    if (!ready_ || !focused_ || c == bank_.cues.size())
        return {};
    const auto& def = bank_.cues[c];
    auto& prepared = cues_[c];
    interface |= def.category == 3;
    if (def.loop || (!interface && worldPaused_) || prepared.since < def.cooldown)
        return {};
    unsigned active = 0;
    for (const auto& v : voices_)
        active += v.active && v.cue == c;
    if (active >= def.concurrency)
        return {};
    auto mix = audio::Mix{1, 0};
    if (positional) {
        mix = audio::spatial(x - x_, z - z_, rx_, rz_, def.range);
        if (session_ && !audio::unobstructed(session_->geometry(), x_, z_, x, z))
            mix.gain = 0;
    }
    if (def.gain * mix.gain * gain(def.category, interface) <= .00001f)
        return {};
    const auto variation = prepared.variation.select(unsigned(def.variants.size()));
    const unsigned sample = prepared.samples[variation];
    if (sample >= samples_.size())
        return {};
    float level = def.gain * def.variants[variation].gain *
                  (def.volumeMin + (1 - def.volumeMin) * prepared.variation.unit());
    const unsigned p = std::max(priority, def.priority);
    unsigned slot = audio::chooseVoice(voices_, p);
    if (slot == 64)
        return {};
    auto& source = samples_[sample];
    unsigned alias = source.count;
    for (unsigned a = 0; a < source.count; ++a) {
        bool used = false;
        for (const auto& v : voices_)
            used |= v.active && v.sample == sample && v.alias == a;
        if (!used) {
            alias = a;
            break;
        }
    }
    if (alias == source.count) {
        unsigned victim = 64;
        for (unsigned i = 0; i < 64; ++i)
            if (voices_[i].active && voices_[i].sample == sample && voices_[i].priority <= p &&
                voices_[i].priority < 4 && voices_[i].generation != UINT64_MAX)
                if (victim == 64 || voices_[i].sequence < voices_[victim].sequence)
                    victim = i;
        if (victim == 64)
            return {};
        alias = voices_[victim].alias;
        slot = victim; // Reuse the alias owner; do not evict a second unrelated voice.
        release(victim);
    }
    if (voices_[slot].active)
        release(slot);
    auto& v = voices_[slot];
    v.active = true;
    v.paused = false;
    v.interface = interface;
    v.priority = p;
    v.sample = sample;
    v.alias = alias;
    v.cue = c;
    v.gain = level;
    v.pitch = def.pitchMin + (def.pitchMax - def.pitchMin) * prepared.variation.unit();
    v.x = x;
    v.z = z;
    v.positional = positional;
    v.range = def.range;
    v.sequence = ++sequence_;
    auto sound = source.voices[alias];
    SetSoundPitch(sound, v.pitch);
    SetSoundVolume(sound, level * mix.gain * gain(def.category, interface));
    SetSoundPan(sound, mix.pan);
    PlaySound(sound);
    prepared.since = 0;
    return {slot, v.generation, owner_};
}
void AudioRuntime::detach() noexcept {
    detachWorld();
    music_.detach();
    sanctuaryMusic_.detach();
    endMenuMusic();
    musicContext_ = MusicContext::None;
    sanctuary_ = false;
    preserveSanctuaryTravel_ = false;
}
void AudioRuntime::beginMenuMusic() noexcept {
    menuMusic_.detach();
    menuMusicActive_ = true;
}
void AudioRuntime::endMenuMusic() noexcept {
    menuMusic_.detach();
    menuMusicActive_ = false;
    if (musicContext_ == MusicContext::Menu)
        musicContext_ = MusicContext::None;
}
void AudioRuntime::setGameplayMusic(std::optional<CampaignLocationId> location) noexcept {
    sanctuary_ = location && (*location == CampaignLocationId::City || interiorLocation(*location));
    if (!sanctuary_)
        preserveSanctuaryTravel_ = false;
}
void AudioRuntime::beginTravelMusic(bool sanctuaryDestination) noexcept {
    preserveSanctuaryTravel_ = sanctuaryDestination && sanctuary_ && musicContext_ == MusicContext::Sanctuary;
}
void AudioRuntime::pumpLoadingMusic(bool focused) {
    if (!ready_ || !preserveSanctuaryTravel_ || musicContext_ != MusicContext::Sanctuary)
        return;
    audio::MusicInput input;
    input.active = true;
    input.audible = settings.musicEnabled;
    input.paused = !focused;
    input.bannerVisible = false;
    sanctuaryMusic_.update(input, settings.values[0] * settings.musicVolume);
}
MusicBanner AudioRuntime::musicBanner() const noexcept {
    if (musicContext_ == MusicContext::Sanctuary)
        return sanctuaryMusic_.banner();
    if (musicContext_ == MusicContext::Combat)
        return music_.banner();
    return {};
}
void AudioRuntime::detachWorld() noexcept {
    stopDialogue();
    dialogueSerial_ = 0;
    for (unsigned i = 0; i < 64; ++i)
        if (voices_[i].active && !voices_[i].interface)
            release(i);
    for (auto& s : streams_)
        if (s.valid && s.active) {
            StopMusicStream(s.music);
            s.active = false;
            s.paused = false;
            s.identity = 0;
        }
    session_ = nullptr;
    world_ = 0;
}
void AudioRuntime::update(GameSession* game, bool paused, bool focused, float dt, bool musicPaused,
                          bool bannerVisible, std::optional<bool> ambiencePaused, bool dialoguePaused) {
    dt = std::isfinite(dt) ? std::clamp(dt, 0.f, .1f) : 0;
    const auto world = game ? game->actors().snapshot().world : 0;
    if (world != world_) {
        detachWorld();
        // World-local encounter IDs may coincide; only sanctuary survives replacement.
        music_.detach();
        session_ = game;
        world_ = world;
        if (game) {
            game->audioEvents().enabled = ready_;
            game->audioEvents().clear();
            const auto& combat = game->combat().snapshot();
            if (!game->audioEvents().encounterActive && combat.round > 0 &&
                combat.phase != CombatPhase::Exploration && combat.phase != CombatPhase::Victory &&
                combat.phase != CombatPhase::Defeat) {
                game->audioEvents().encounterActive = true;
                ++game->audioEvents().encounter;
            }
        }
    }
    session_ = game;
    worldPaused_ = paused;
    ambiencePaused_ = ambiencePaused.value_or(paused);
    focused_ = focused;
    dialoguePaused_ = dialoguePaused;
    if (dialogueSound_ < dialogueSounds_.size()) {
        auto sound = dialogueSounds_[dialogueSound_].voice;
        const bool suspend = !focused || dialoguePaused;
        if (suspend != dialogueVoicePaused_) {
            if (suspend)
                PauseSound(sound);
            else
                ResumeSound(sound);
            dialogueVoicePaused_ = suspend;
        }
        SetSoundVolume(sound, gain(1, true));
    }
    if (game) {
        const auto camera = world_render::camera(game->movement());
        x_ = camera.position.x;
        z_ = camera.position.z;
        rx_ = camera.right.x;
        rz_ = camera.right.z;
    }
    for (auto& c : cues_)
        c.since = std::min(100.f, c.since + dt);
    if (ready_)
        for (unsigned i = 0; i < 64; ++i) {
            auto& v = voices_[i];
            if (!v.active)
                continue;
            const auto sound = samples_[v.sample].voices[v.alias];
            const bool suspend = !focused || (!v.interface && paused);
            if (suspend != v.paused) {
                if (suspend)
                    PauseSound(sound);
                else
                    ResumeSound(sound);
                v.paused = suspend;
            }
            if (!v.paused && !IsSoundPlaying(sound)) {
                release(i);
                continue;
            }
            auto mix = audio::Mix{1, 0};
            if (v.positional) {
                mix = audio::spatial(v.x - x_, v.z - z_, rx_, rz_, v.range);
                if (game && !audio::unobstructed(game->geometry(), x_, z_, v.x, v.z))
                    mix.gain = 0;
            }
            SetSoundVolume(sound, v.gain * mix.gain * gain(bank_.cues[v.cue].category, v.interface));
            SetSoundPan(sound, mix.pan);
        }
    if (game) {
        auto& journal = game->audioEvents();
        for (unsigned i = 0; i < journal.count; ++i) {
            const auto& e = journal.entries[i];
            float x = e.x, z = e.z;
            const char* name = e.cue;
            char buffer[96];
            if (const auto* a = game->actors().actor(e.actor)) {
                const auto position = actorPosition(*a, game->timeMs());
                x = position.x;
                z = position.y;
                if (e.creature) {
                    const auto* m = game->party().catalog().monster(a->monster);
                    const auto& id = m->id;
                    const char* species = id == "giant.rat"        ? "rat"
                                          : id == "goblin_shaman"  ? "shaman"
                                          : id.find("goblin") == 0 ? "goblin"
                                                                   : id.c_str();
                    std::snprintf(buffer, sizeof(buffer), "%s.%s", species, name);
                    name = buffer;
                }
            } else if (e.creature)
                continue;
            play(name, x, z, e.positional, e.interface, e.priority);
        }
        journal.clear();
    }
    if (ready_) {
        const auto context = game ? (sanctuary_ ? MusicContext::Sanctuary : MusicContext::Combat)
                                  : (menuMusicActive_ ? MusicContext::Menu : MusicContext::None);
        if (context != musicContext_) {
            music_.detach();
            sanctuaryMusic_.detach();
            menuMusic_.detach();
            musicContext_ = context;
        }
        audio::MusicInput input;
        if (game) {
            input.encounter = game->audioEvents().encounter;
            input.active = game->audioEvents().encounterActive;
        }
        input.audible = settings.musicEnabled && settings.musicVolume > 0 && settings.values[0] > 0;
        const bool continuousSanctuary = preserveSanctuaryTravel_ && musicContext_ == MusicContext::Sanctuary;
        input.paused = (musicPaused && !continuousSanctuary) || !focused;
        input.bannerVisible = bannerVisible && !continuousSanctuary;
        input.dt = dt;
        const auto volume = settings.values[0] * settings.musicVolume;
        switch (musicContext_) {
        case MusicContext::Combat:
            music_.update(input, volume * worldGain);
            break;
        case MusicContext::Sanctuary:
            input.encounter = 0;
            input.active = true;
            sanctuaryMusic_.update(input, volume * (continuousSanctuary ? 1.f : worldGain));
            break;
        case MusicContext::Menu:
            input.active = true;
            input.paused = !focused;
            input.bannerVisible = false;
            menuMusic_.update(input, volume);
            break;
        case MusicContext::None:
            break;
        }
        streamUpdate(musicContext_ == MusicContext::Combat && music_.audible());
    }
}
void AudioRuntime::streamUpdate(bool suppressAmbience) {
    struct Target {
        std::uint64_t id = 0;
        float x = 0, z = 0, d = 1e9f;
    };
    std::array<Target, 6> targets{};
    auto insert = [&](unsigned start, unsigned count, std::uint64_t id, Cell cell) {
        Target t{id, float(cell.x) + .5f, float(cell.y) + .5f, 0};
        if (start == 0)
            for (unsigned i = 0; i < count; ++i)
                if (targets[i].id && targets[i].x == t.x && targets[i].z == t.z)
                    return;
        t.d = (t.x - x_) * (t.x - x_) + (t.z - z_) * (t.z - z_);
        for (unsigned i = start; i < start + count; ++i)
            if (t.d < targets[i].d)
                std::swap(t, targets[i]);
    };
    if (session_) {
        for (const auto& a : session_->effects().ground().areas)
            if (a.active && a.web && a.burnMs)
                insert(0, 4, a.id.serial, a.cell);
        for (const auto& e : session_->effects().snapshot())
            if (e.active && e.kind == EffectKind::SpiritualWeapon)
                insert(4, 2, e.id.serial, e.cell);
    }
    // Preserve stream assignments when nearest ordering changes.
    std::array<bool, 6> claimed{};
    for (unsigned i = 2; i < 8; ++i)
        if (streams_[i].identity)
            for (unsigned j = i < 6 ? 0 : 4; j < (i < 6 ? 4 : 6); ++j)
                if (targets[j].id == streams_[i].identity) {
                    claimed[j] = true;
                    streams_[i].x = targets[j].x;
                    streams_[i].z = targets[j].z;
                    break;
                }
    for (unsigned i = 0; i < 8; ++i) {
        auto& s = streams_[i];
        if (!s.valid)
            continue;
        bool wanted = i == 0 && session_ && !sanctuary_;
        if (i >= 2) {
            wanted = false;
            for (unsigned j = i < 6 ? 0 : 4; j < (i < 6 ? 4 : 6); ++j)
                if (targets[j].id && s.identity == targets[j].id)
                    wanted = true;
            if (!wanted) {
                if (s.active)
                    StopMusicStream(s.music);
                s.active = false;
                s.identity = 0;
                for (unsigned j = i < 6 ? 0 : 4; j < (i < 6 ? 4 : 6); ++j)
                    if (!claimed[j] && targets[j].id) {
                        claimed[j] = true;
                        s.identity = targets[j].id;
                        s.x = targets[j].x;
                        s.z = targets[j].z;
                        wanted = true;
                        break;
                    }
            }
            s.positional = true;
        }
        if (!wanted) {
            if (s.active)
                StopMusicStream(s.music);
            s.active = false;
            s.paused = false;
            continue;
        }
        if (!s.active) {
            if (i < 2 && suppressAmbience)
                continue; // Restored/ongoing combat must not briefly start ambient playback.
            PlayMusicStream(s.music);
            s.active = true;
            s.paused = false;
        }
        const bool pause = (i < 2 ? ambiencePaused_ || suppressAmbience : worldPaused_) || !focused_;
        if (pause != s.paused) {
            if (pause)
                PauseMusicStream(s.music);
            else
                ResumeMusicStream(s.music);
            s.paused = pause;
        }
        auto mix = audio::Mix{1, 0};
        const auto& c = bank_.cues[s.cue];
        if (s.positional) {
            mix = audio::spatial(s.x - x_, s.z - z_, rx_, rz_, c.range);
            if (session_ && !audio::unobstructed(session_->geometry(), x_, z_, s.x, s.z))
                mix.gain = 0;
        }
        SetMusicVolume(s.music, c.gain * gain(c.category, false) * mix.gain);
        SetMusicPan(s.music, mix.pan);
        if (!s.paused)
            UpdateMusicStream(s.music);
    }
}
void AudioRuntime::persistMusic() {
    music_.persist(warning_);
    sanctuaryMusic_.persist(warning_);
}
void AudioRuntime::persist() {
    warning_.clear();
    audio::saveSettings(settingsPath_, settings, warning_);
    persistMusic();
    if (!warning_.empty())
        std::cerr << warning_ << '\n';
}
void AudioRuntime::shutdown() noexcept {
    if (ready_) {
        // Shutdown is an allocation/I/O boundary. Keep destructors nonthrowing.
        try {
            persistMusic();
        } catch (...) {
        }
        detach();
        clearDialogue();
        for (unsigned i = 0; i < 64; ++i)
            if (voices_[i].active)
                release(i);
        if (limiterAttached_) {
            DetachAudioMixedProcessor(limitOutput);
            limiterAttached_ = false;
        }
        music_.shutdown();
        sanctuaryMusic_.shutdown();
        menuMusic_.shutdown();
        for (auto& s : streams_)
            if (s.valid)
                UnloadMusicStream(s.music);
        for (auto& s : samples_) {
            for (unsigned a = 0; a < s.count; ++a)
                UnloadSoundAlias(s.voices[a]);
            UnloadSound(s.source);
        }
        CloseAudioDevice();
    }
    residentBytes_ = 0;
    ready_ = false;
    streams_ = {};
    samples_.clear();
    cues_.clear();
    bank_ = {};
    session_ = nullptr;
    world_ = 0;
}
} // namespace blobber
