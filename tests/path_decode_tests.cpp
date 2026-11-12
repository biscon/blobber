#include "raylib.h"
#include "support/check.hpp"
#include "support/temp_directory.hpp"
#include <array>
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    const auto root = testDirectory();
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    } cleanup{root};
    SetTraceLogLevel(LOG_WARNING);
    try {
        const auto wav = root / std::filesystem::u8path(u8"tone æ 日本.wav");
        const auto mp3 = root / std::filesystem::u8path(u8"tone æ 日本.mp3");
        std::filesystem::copy_file(std::filesystem::path(BLOBBER_PATH_MP3_FIXTURE), mp3);
        std::array<short, 2205> samples{};
        for (unsigned i = 0; i < samples.size(); ++i)
            samples[i] = i % 50 < 25 ? 1000 : -1000;
        Wave generated{unsigned(samples.size()), 22050, 16, 1, samples.data()};
        check(ExportWave(generated, wav.u8string().c_str()), "Export owned Unicode WAV");
        if (argc == 2 && std::string_view(argv[1]) == "--stream") {
            InitAudioDevice();
            if (!IsAudioDeviceReady()) {
                std::cout << "Audio device unavailable; filename streaming decode unverified\n";
                return 77;
            }
            SetMasterVolume(0);
            for (const auto& path : {wav, mp3}) {
                auto music = LoadMusicStream(path.u8string().c_str());
                const bool valid = IsMusicValid(music) && music.frameCount > 0;
                UnloadMusicStream(music);
                if (!valid) {
                    CloseAudioDevice();
                    throw std::runtime_error("Unicode streaming decode");
                }
            }
            CloseAudioDevice();
        } else {
            for (const auto& path : {wav, mp3}) {
                auto wave = LoadWave(path.u8string().c_str());
                const bool valid = IsWaveValid(wave) && wave.frameCount > 0 && wave.sampleRate == 22050;
                UnloadWave(wave);
                check(valid, "Unicode WAV/MP3 filename decode");
            }
        }
        std::cout << "Owned Unicode audio filename decode passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
