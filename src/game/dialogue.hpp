#pragma once
#include "world/npc.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace blobber {
using DialogueKey = std::uint64_t;
inline constexpr unsigned dialogueMinimumPanMs = 200;
constexpr DialogueKey dialogueKey(std::string_view value) noexcept {
    DialogueKey h = 14695981039346656037ull;
    for (const auto c : value)
        h = (h ^ static_cast<unsigned char>(c)) * 1099511628211ull;
    return value.empty() ? 0 : h;
}
struct DialogueFraming {
    float distance = .6f, eyeHeight = 0;
};
struct DialogueParticipant : DialogueFraming {
    DialogueKey id = 0;
    std::string alias, name;
    std::uint32_t textColor = 0xdcd8c7ff;
};
struct DialogueBeat {
    std::string text;
    unsigned speaker = 0, target = 0;
    std::optional<unsigned> durationMs;
    std::optional<NpcAnimation> animation;
    unsigned audio = 64; // Catalog audio index; 64 means silence.
};
struct DialogueResponse {
    DialogueKey whenFact = 0, setFact = 0;
    std::vector<DialogueBeat> beats;
    std::vector<std::string> pages; // Prepared display pages, never authored events.
    std::vector<unsigned> pageBeats;
};
struct DialogueTopic {
    DialogueKey id = 0;
    std::string label;
    std::vector<DialogueResponse> responses;
    bool introduction = false, farewell = false;
};
struct DialogueCharacter : DialogueFraming {
    DialogueKey id = 0;
    std::string key, name;
    DialogueResponse greeting, introduction;
    std::vector<DialogueTopic> topics;
    std::vector<DialogueParticipant> participants;
    unsigned primary = 0;
};
struct DialogueCatalog {
    std::vector<DialogueCharacter> characters;
    std::vector<std::string> audioFiles; // Canonical paths resolved during loading.
    unsigned transitionMs = 700, fadeMs = 150, charactersPerSecond = 45;
    const DialogueCharacter* find(DialogueKey id) const noexcept;
};
// Preparation-only. Actual UI metrics are injected; headless fixtures own theirs.
using DialogueMeasure = float (*)(const char*, void*);
void prepareDialogueLayout(DialogueCatalog&, DialogueMeasure, void*, float width = 286, unsigned rows = 8);
struct DialogueHistoryRecord {
    DialogueKey id = 0;
    bool introduced = false;
    std::array<DialogueKey, 64> completed{}, facts{};
    unsigned completedCount = 0, factCount = 0;
    bool knows(DialogueKey) const noexcept;
};
struct DialogueHistory {
    std::array<DialogueHistoryRecord, 64> records{};
    unsigned count = 0;
    DialogueHistoryRecord* get(DialogueKey, bool create = false) noexcept;
    const DialogueHistoryRecord* get(DialogueKey) const noexcept;
    bool complete(DialogueKey character, DialogueKey topic, DialogueKey fact) noexcept;
    void validate() const;
};
enum class DialoguePhase : std::uint8_t { Closed, Entering, Open, Leaving };
struct DialogueHud {
    bool open = false, leaving = false, introduction = false, revealing = false;
    const char* name = "";
    std::uint32_t textColor = 0xdcd8c7ff;
    std::array<char, 1025> text{};
    std::array<const char*, 7> topics{};
    std::array<bool, 7> read{};
    unsigned count = 0, cursor = 0, offset = 0, totalTopics = 0, page = 0, pages = 0;
    std::uint64_t revision = 0;
    float alpha = 0, cameraWeight = 0;
};
using DialogueAdmission = bool (*)(const DialogueCharacter&, const DialogueResponse&, void*) noexcept;
class DialoguePlayer {
  public:
    void bind(const DialogueCatalog* catalog, DialogueHistory* history, DialogueAdmission admission = nullptr,
              void* context = nullptr) noexcept;
    bool start(DialogueKey) noexcept;
    void tick(double seconds, bool focused) noexcept;
    void advance() noexcept;
    void choose(unsigned visibleRow) noexcept;
    void select(int delta) noexcept;
    void scroll(int delta) noexcept;
    void skipIntroduction() noexcept;
    void leave() noexcept;
    void close() noexcept;
    bool active() const noexcept { return phase_ != DialoguePhase::Closed; }
    bool leaving() const noexcept { return phase_ == DialoguePhase::Leaving; }
    bool revealing() const noexcept;
    float weight() const noexcept;
    std::uint64_t clockMs() const noexcept { return std::uint64_t(clockMs_); }
    const DialogueCharacter* character() const noexcept { return character_; }
    const DialogueBeat* beat() const noexcept;
    const DialogueResponse* currentResponse() const noexcept { return response_; }
    std::uint64_t beatSerial() const noexcept { return beatSerial_; }
    std::uint64_t beatElapsedMs() const noexcept;
    bool cuesReady() const noexcept;
    unsigned transitionMs() const noexcept { return catalog_ ? catalog_->transitionMs : 700; }
    unsigned cursor() const noexcept { return cursor_ - offset_; }
    void snapshot(DialogueHud&) const noexcept;

  private:
    void response(const DialogueResponse*, DialogueKey topic = 0, bool intro = false) noexcept;
    void completed() noexcept;
    bool admissible(const DialogueCharacter&, const DialogueResponse&) const noexcept;
    void beginBeat() noexcept;
    const DialogueCatalog* catalog_ = nullptr;
    DialogueHistory* history_ = nullptr;
    const DialogueCharacter* character_ = nullptr;
    const DialogueResponse* response_ = nullptr;
    DialogueKey topic_ = 0;
    DialoguePhase phase_ = DialoguePhase::Closed;
    unsigned page_ = 0, cursor_ = 0, offset_ = 0;
    double clockMs_ = 0, transition_ = 0, revealed_ = 0;
    float exitFrom_ = 0;
    bool introduction_ = false, awarded_ = false;
    std::uint64_t revision_ = 0, beatSerial_ = 0;
    double beatStartMs_ = 0;
    DialogueAdmission admission_ = nullptr;
    void* admissionContext_ = nullptr;
};
} // namespace blobber
