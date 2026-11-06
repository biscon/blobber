#include "game/dialogue.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace blobber {
const DialogueCharacter* DialogueCatalog::find(DialogueKey id) const noexcept {
    for (const auto& c : characters)
        if (c.id == id)
            return &c;
    return nullptr;
}
namespace {
bool contains(const std::array<DialogueKey, 64>& values, unsigned count, DialogueKey id) noexcept {
    return std::find(values.begin(), values.begin() + std::min(count, 64u), id) !=
           values.begin() + std::min(count, 64u);
}
void layout(DialogueResponse& response, DialogueMeasure measure, void* context, float width, unsigned rows) {
    std::vector<std::string> pages;
    for (const auto& original : response.pages) {
        std::string page, line;
        unsigned lines = 0;
        auto emit = [&] {
            if (lines == rows) {
                pages.push_back(std::move(page));
                page.clear();
                lines = 0;
            }
            if (lines++)
                page += '\n';
            page += line;
            line.clear();
        };
        std::size_t at = 0;
        while (at < original.size()) {
            if (original[at] == '\n') {
                emit();
                ++at;
                continue;
            }
            if (original[at] == ' ') {
                ++at;
                continue;
            }
            auto end = original.find_first_of(" \n", at);
            if (end == std::string::npos)
                end = original.size();
            const auto word = original.substr(at, end - at);
            if (measure(word.c_str(), context) > width)
                throw std::runtime_error("Dialogue word exceeds response width");
            const auto candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && measure(candidate.c_str(), context) > width)
                emit();
            if (!line.empty())
                line += ' ';
            line += word;
            at = end;
        }
        if (!line.empty() || !lines)
            emit();
        if (!page.empty())
            pages.push_back(std::move(page));
    }
    if (pages.empty() || pages.size() > 64)
        throw std::runtime_error("Dialogue exceeds 64 laid-out pages");
    for (const auto& p : pages)
        if (p.size() > 1024)
            throw std::runtime_error("Dialogue laid-out page exceeds capacity");
    response.pages = std::move(pages);
}
} // namespace
void prepareDialogueLayout(DialogueCatalog& catalog, DialogueMeasure measure, void* context, float width,
                           unsigned rows) {
    if (!measure || !rows || width <= 0)
        throw std::invalid_argument("Invalid dialogue layout metrics");
    for (auto& c : catalog.characters) {
        layout(c.greeting, measure, context, width, rows);
        layout(c.introduction, measure, context, width, rows);
        for (auto& t : c.topics)
            for (auto& r : t.responses)
                layout(r, measure, context, width, rows);
    }
}
bool DialogueHistoryRecord::knows(DialogueKey key) const noexcept {
    return key && contains(facts, factCount, key);
}
DialogueHistoryRecord* DialogueHistory::get(DialogueKey key, bool create) noexcept {
    for (unsigned i = 0; i < count && i < records.size(); ++i)
        if (records[i].id == key)
            return &records[i];
    if (!create || !key || count >= records.size())
        return nullptr;
    auto& r = records[count++];
    r = {};
    r.id = key;
    return &r;
}
const DialogueHistoryRecord* DialogueHistory::get(DialogueKey key) const noexcept {
    for (unsigned i = 0; i < count && i < records.size(); ++i)
        if (records[i].id == key)
            return &records[i];
    return nullptr;
}
bool DialogueHistory::complete(DialogueKey character, DialogueKey topic, DialogueKey fact) noexcept {
    auto* r = get(character, true);
    if (!r)
        return false;
    const bool addTopic = topic && !contains(r->completed, r->completedCount, topic);
    const bool addFact = fact && !r->knows(fact);
    if ((addTopic && r->completedCount == 64) || (addFact && r->factCount == 64))
        return false;
    if (addTopic)
        r->completed[r->completedCount++] = topic;
    if (addFact)
        r->facts[r->factCount++] = fact;
    return true;
}
void DialogueHistory::validate() const {
    if (count > records.size())
        throw std::invalid_argument("Invalid dialogue history count");
    for (unsigned i = 0; i < count; ++i) {
        const auto& r = records[i];
        if (!r.id || r.completedCount > 64 || r.factCount > 64)
            throw std::invalid_argument("Invalid dialogue history record");
        for (unsigned j = 0; j < i; ++j)
            if (records[j].id == r.id)
                throw std::invalid_argument("Duplicate dialogue history character");
        const auto validateKeys = [](const auto& values, unsigned size) {
            for (unsigned j = 0; j < size; ++j)
                if (!values[j] || contains(values, j, values[j]))
                    throw std::invalid_argument("Invalid/duplicate dialogue history key");
        };
        validateKeys(r.completed, r.completedCount);
        validateKeys(r.facts, r.factCount);
    }
}
void DialoguePlayer::bind(const DialogueCatalog* catalog, DialogueHistory* history) noexcept {
    close();
    catalog_ = catalog;
    history_ = history;
}
bool DialoguePlayer::start(DialogueKey key) noexcept {
    if (active() || !catalog_ || !history_)
        return false;
    const auto* c = catalog_->find(key);
    if (!c || c->introduction.pages.empty() || c->greeting.pages.empty() || c->topics.empty())
        return false;
    auto* h = history_->get(key, true);
    if (!h)
        return false;
    const bool first = !h->introduced;
    h->introduced = true;
    character_ = c;
    phase_ = DialoguePhase::Entering;
    clockMs_ = transition_ = 0;
    cursor_ = offset_ = 0;
    response(first ? &c->introduction : &c->greeting, 0, first);
    return true;
}
void DialoguePlayer::response(const DialogueResponse* value, DialogueKey topic, bool intro) noexcept {
    ++revision_;
    response_ = value;
    topic_ = topic;
    introduction_ = intro;
    page_ = 0;
    revealed_ = 0;
    awarded_ = false;
}
bool DialoguePlayer::revealing() const noexcept {
    return active() && !leaving() && response_ && page_ < response_->pages.size() &&
           revealed_ < response_->pages[page_].size();
}
float DialoguePlayer::weight() const noexcept {
    if (phase_ == DialoguePhase::Open)
        return 1;
    if (phase_ == DialoguePhase::Closed)
        return 0;
    const float t = std::clamp(float(transition_ / catalog_->transitionMs), 0.f, 1.f);
    const float eased = t * t * (3 - 2 * t);
    return leaving() ? exitFrom_ * (1 - eased) : eased;
}
void DialoguePlayer::completed() noexcept {
    if (!awarded_ && response_ && page_ + 1 == response_->pages.size() && !revealing()) {
        awarded_ = history_->complete(character_->id, topic_, response_->setFact);
    }
}
void DialoguePlayer::tick(double seconds, bool focused) noexcept {
    if (!active() || !focused || !std::isfinite(seconds) || seconds <= 0)
        return;
    const double ms = std::min(seconds, 1.0) * 1000;
    const double previousClock = clockMs_;
    clockMs_ += ms;
    transition_ += ms;
    if (leaving()) {
        if (transition_ >= catalog_->transitionMs)
            close();
        return;
    }
    if (transition_ >= catalog_->transitionMs)
        phase_ = DialoguePhase::Open;
    const double revealMs =
        std::max(0., clockMs_ - catalog_->fadeMs) - std::max(0., previousClock - catalog_->fadeMs);
    if (revealing())
        revealed_ = std::min(double(response_->pages[page_].size()),
                             revealed_ + revealMs * catalog_->charactersPerSecond / 1000);
    completed();
}
void DialoguePlayer::advance() noexcept {
    if (!active() || leaving() || !response_)
        return;
    if (revealing()) {
        revealed_ = double(response_->pages[page_].size());
        completed();
    } else if (page_ + 1 < response_->pages.size()) {
        ++page_;
        ++revision_;
        revealed_ = 0;
    }
}
void DialoguePlayer::choose(unsigned row) noexcept {
    if (!active() || leaving() || row >= 7 || offset_ + row >= character_->topics.size())
        return;
    cursor_ = offset_ + row;
    const auto& t = character_->topics[cursor_];
    if (t.farewell) {
        leave();
        return;
    }
    if (t.introduction) {
        response(&character_->introduction, t.id, true);
        return;
    }
    const auto* h = history_->get(character_->id);
    const DialogueResponse* selected = nullptr;
    for (const auto& r : t.responses)
        if (!r.whenFact)
            selected = &r;
    for (const auto& r : t.responses)
        if (h && h->knows(r.whenFact)) {
            selected = &r;
            break;
        }
    response(selected, t.id);
}
void DialoguePlayer::select(int delta) noexcept {
    if (!active() || leaving() || character_->topics.empty())
        return;
    const int n = int(character_->topics.size());
    cursor_ = unsigned((int(cursor_) + delta % n + n) % n);
    if (cursor_ < offset_)
        offset_ = cursor_;
    if (cursor_ >= offset_ + 7)
        offset_ = cursor_ - 6;
}
void DialoguePlayer::scroll(int delta) noexcept {
    if (!active() || leaving())
        return;
    const int maximum = std::max(0, int(character_->topics.size()) - 7);
    offset_ = unsigned(std::clamp(int(offset_) + delta, 0, maximum));
    cursor_ = std::clamp(cursor_, offset_,
                         offset_ + std::min(7u, unsigned(character_->topics.size()) - offset_) - 1);
}
void DialoguePlayer::skipIntroduction() noexcept {
    if (active() && !leaving() && introduction_)
        response(nullptr);
}
void DialoguePlayer::leave() noexcept {
    if (active() && !leaving()) {
        exitFrom_ = weight();
        phase_ = DialoguePhase::Leaving;
        transition_ = 0;
    }
}
void DialoguePlayer::close() noexcept {
    phase_ = DialoguePhase::Closed;
    character_ = nullptr;
    response_ = nullptr;
}
void DialoguePlayer::snapshot(DialogueHud& out) const noexcept {
    out = {};
    if (!active())
        return;
    out.revision = revision_;
    out.open = true;
    out.leaving = leaving();
    out.introduction = introduction_;
    out.revealing = revealing();
    out.name = character_->name.c_str();
    out.alpha = leaving() ? std::max(0.f, 1 - float(transition_ / catalog_->fadeMs))
                          : std::min(1.f, float(clockMs_ / catalog_->fadeMs));
    out.cameraWeight = weight();
    out.cursor = cursor_ - offset_;
    out.offset = offset_;
    out.totalTopics = unsigned(character_->topics.size());
    out.count = std::min(7u, out.totalTopics - offset_);
    const auto* h = history_->get(character_->id);
    for (unsigned i = 0; i < out.count; ++i) {
        const auto& t = character_->topics[offset_ + i];
        out.topics[i] = t.label.c_str();
        out.read[i] = h && contains(h->completed, h->completedCount, t.id);
    }
    if (response_) {
        out.page = page_ + 1;
        out.pages = unsigned(response_->pages.size());
        const auto& p = response_->pages[page_];
        const auto count = std::min(std::size_t(revealed_), p.size());
        std::memcpy(out.text.data(), p.data(), std::min(count, out.text.size() - 1));
    }
}
} // namespace blobber
