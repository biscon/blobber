#pragma once
#include "audio/bank.hpp"
#include "audio/events.hpp"
#include "game/session.hpp"
#include "game/travel.hpp"
#include "platform/music_player.hpp"
#include <optional>
#include <raylib.h>
namespace blobber {
class AudioRuntime {
  public:
    AudioRuntime() = default;
    ~AudioRuntime();
    AudioRuntime(const AudioRuntime&) = delete;
    AudioRuntime& operator=(const AudioRuntime&) = delete;
    void prepare(const std::filesystem::path&, bool disabled = false,
                 const std::filesystem::path& settings = {});
    void update(GameSession*, bool paused, bool focused, float dt, bool musicPaused = true,
                bool bannerVisible = true, std::optional<bool> ambiencePaused = {},
                bool dialoguePaused = true);
    void prepareDialogue(const std::vector<std::string>& files);
    void clearDialogue() noexcept;
    void presentDialogue(std::uint64_t serial, unsigned sound, bool active) noexcept;
    audio::Handle play(const char*, float x = 0, float z = 0, bool positional = false, bool interface = false,
                       unsigned priority = 0);
    bool stop(audio::Handle) noexcept;
    void detach() noexcept;
    void beginMenuMusic() noexcept;
    void endMenuMusic() noexcept;
    // Call at gameplay entry and after travel commits, never during preparation.
    void setGameplayMusic(std::optional<CampaignLocationId> location) noexcept;
    // One application-owned travel scope; pumping never reads a GameSession.
    void beginTravelMusic(bool sanctuaryDestination) noexcept;
    void endTravelMusic() noexcept { preserveSanctuaryTravel_ = false; }
    void pumpLoadingMusic(bool focused);
    void shutdown() noexcept;
    audio::Settings settings;
    float worldGain = 1; // Travel fade, never persisted.
    bool ready() const noexcept { return ready_; }
    void persist();
    void persistMusic(); // Explicit I/O boundary, never called by update.
    MusicBanner musicBanner() const noexcept;
    const std::string& warning() const noexcept { return warning_; }

  private:
    enum class MusicContext { None, Combat, Sanctuary, Menu };
    MusicPlayer music_, sanctuaryMusic_, menuMusic_;
    MusicContext musicContext_ = MusicContext::None;
    bool menuMusicActive_ = false, sanctuary_ = false;
    bool preserveSanctuaryTravel_ = false;
    void detachWorld() noexcept;
    struct DialogueSound {
        Sound source{}, voice{};
        bool valid = false;
    };
    std::array<DialogueSound, 64> dialogueSounds_{};
    unsigned dialogueSound_ = 64;
    std::uint64_t dialogueSerial_ = 0, residentBytes_ = 0;
    bool dialoguePaused_ = true, dialogueVoicePaused_ = false;
    void stopDialogue() noexcept;
    struct Sample {
        Sound source{};
        std::array<Sound, 8> voices{};
        unsigned count = 0;
        std::string path;
    };
    struct Prepared {
        std::array<unsigned, 16> samples{};
        audio::Variation variation;
        float since = 100;
    };
    struct Stream {
        Music music{};
        bool valid = false, active = false, paused = false;
        unsigned cue = 0;
        float x = 0, z = 0;
        bool positional = false;
        std::uint64_t identity = 0;
    };
    std::array<audio::Voice, 64> voices_{};
    std::vector<Sample> samples_;
    std::vector<Prepared> cues_;
    std::array<Stream, 8> streams_{};
    audio::Bank bank_;
    std::filesystem::path root_, settingsPath_;
    std::string warning_;
    bool ready_ = false, worldPaused_ = false, ambiencePaused_ = false, focused_ = true;
    bool limiterAttached_ = false;
    std::uint64_t owner_ = 0, sequence_ = 0, world_ = 0;
    float x_ = 0, z_ = 0, rx_ = 1, rz_ = 0;
    GameSession* session_ = nullptr;
    unsigned cue(const char*) const noexcept;
    float gain(unsigned category, bool interface) const noexcept;
    void release(unsigned) noexcept;
    void streamUpdate(bool suppressAmbience);
};
} // namespace blobber
