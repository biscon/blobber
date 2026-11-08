#include "platform/audio.hpp"
#include <iostream>
#include <stdexcept>

namespace blobber {
void AudioRuntime::stopDialogue() noexcept {
    if (dialogueSound_ < dialogueSounds_.size() && dialogueSounds_[dialogueSound_].valid)
        StopSound(dialogueSounds_[dialogueSound_].voice);
    dialogueSound_ = 64;
    dialogueVoicePaused_ = false;
}
void AudioRuntime::clearDialogue() noexcept {
    stopDialogue();
    dialogueSerial_ = 0;
    for (auto& sound : dialogueSounds_) {
        if (sound.valid) {
            UnloadSoundAlias(sound.voice);
            UnloadSound(sound.source);
        }
        sound = {};
    }
}
void AudioRuntime::prepareDialogue(const std::vector<std::string>& files) {
    if (files.size() > dialogueSounds_.size())
        throw std::invalid_argument("Dialogue audio exceeds 64 prepared sounds");
    clearDialogue();
    if (!ready_)
        return;
    auto bytes = residentBytes_;
    unsigned loaded = unsigned(samples_.size());
    for (unsigned i = 0; i < files.size() && i < dialogueSounds_.size(); ++i) {
        const auto& path = files[i];
        if (loaded == 512 || !std::filesystem::is_regular_file(path)) {
            warning_ += "Unavailable dialogue sound: " + path + "\n";
            std::cerr << "Unavailable dialogue sound: " << path << '\n';
            continue;
        }
        auto wave = LoadWave(path.c_str());
        const auto cost = std::uint64_t(wave.frameCount) * 8 * 48000 / std::max(1u, wave.sampleRate);
        if (!IsWaveValid(wave) || cost > 64 * 1024 * 1024 - bytes) {
            if (wave.data)
                UnloadWave(wave);
            warning_ += "Invalid/oversized dialogue sound: " + path + "\n";
            std::cerr << "Invalid/oversized dialogue sound: " << path << '\n';
            continue;
        }
        auto source = LoadSoundFromWave(wave);
        UnloadWave(wave);
        if (!IsSoundValid(source)) {
            std::cerr << "Cannot load dialogue sound: " << path << '\n';
            continue;
        }
        auto voice = LoadSoundAlias(source);
        if (!IsSoundValid(voice)) {
            UnloadSound(source);
            std::cerr << "Cannot prepare dialogue voice: " << path << '\n';
            continue;
        }
        dialogueSounds_[i] = {source, voice, true};
        bytes += cost;
        ++loaded;
    }
}
void AudioRuntime::presentDialogue(std::uint64_t serial, unsigned sound, bool active) noexcept {
    if (!active) {
        stopDialogue();
        dialogueSerial_ = 0;
        return;
    }
    if (serial == dialogueSerial_)
        return;
    stopDialogue();
    // A pending first cue is not consumed while presentation is suspended.
    if (!focused_ || dialoguePaused_)
        return;
    dialogueSerial_ = serial;
    if (!ready_ || sound >= dialogueSounds_.size() || !dialogueSounds_[sound].valid)
        return;
    dialogueSound_ = sound;
    auto voice = dialogueSounds_[sound].voice;
    SetSoundVolume(voice, gain(1, true));
    SetSoundPitch(voice, 1);
    SetSoundPan(voice, 0);
    PlaySound(voice);
}
} // namespace blobber
