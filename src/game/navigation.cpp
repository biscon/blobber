#include "game/navigation.hpp"
#include "game/approach_slots.hpp"
#include "game/perception.hpp"
#include <cstdlib>
#include <utility>
namespace blobber {
Navigation::Navigation(GeometryView map, const ActorWorld& actors)
    : map_(map), actors_(actors), queue_(std::size_t(map.width()) * map.height()), seen_(queue_.size()),
      first_(queue_.size()) {}
std::uint32_t Navigation::index(Cell c) const noexcept {
    return c.y * map_.width() + c.x;
}
Cell Navigation::cell(std::uint32_t i) const noexcept {
    return {int(i % map_.width()), int(i / map_.width())};
}
void Navigation::invalidateGeometry() noexcept {
    if (geometryRevision_ == map_.revision())
        return;
    geometryRevision_ = map_.revision();
    requests_ = {};
    results_ = {};
    if (owner_ != requests_.size()) {
        canceled_ = true;
        cleaning_ = true;
    }
}
void Navigation::request(std::size_t slot, Cell from, Cell target, bool adjacent,
                         std::optional<Cell> preferred) noexcept {
    invalidateGeometry();
    if (slot >= requests_.size())
        return;
    const auto matches = [&](const auto& r) {
        return r.from == from && r.target == target && r.adjacent == adjacent && r.preferred == preferred;
    };
    if (results_[slot].ready) {
        if (matches(results_[slot]))
            return;
        results_[slot] = {};
    }
    if (owner_ == slot) {
        if (!canceled_ && !cleaning_ && matches(active_))
            return;
        canceled_ = true;
    }
    requests_[slot] = {true, from, target, adjacent, preferred};
}
void Navigation::cancel(std::size_t slot) noexcept {
    if (slot >= requests_.size())
        return;
    requests_[slot].pending = false;
    results_[slot] = {};
    if (owner_ == slot)
        canceled_ = true;
}
Navigation::Result Navigation::take(std::size_t slot) noexcept {
    invalidateGeometry();
    if (slot >= results_.size())
        return {};
    const auto r = results_[slot];
    results_[slot] = {};
    return r;
}
bool Navigation::start() noexcept {
    for (std::size_t n = 0; n < requests_.size(); ++n) {
        const auto i = (cursor_ + n) % requests_.size();
        if (!requests_[i].pending)
            continue;
        owner_ = i;
        cursor_ = (i + 1) % requests_.size();
        active_ = requests_[i];
        requests_[i].pending = false;
        head_ = tail_ = cleanup_ = 0;
        canceled_ = cleaning_ = false;
        fallbackStep_.reset();
        frontStep_.reset();
        preferAvailable_ = centerAvailable_ = false;
        if (active_.adjacent && active_.preferred && cellDistance(*active_.preferred, active_.target) == 1) {
            preferAvailable_ = true;
            const auto goal = approachGoal(false);
            centerAvailable_ = goal && *goal == *active_.preferred;
        }
        if (map_.solid(active_.from)) {
            finish(false, {});
            return true;
        }
        const auto root = index(active_.from);
        queue_[tail_++] = root;
        seen_[root] = 1;
        first_[root] = 4;
        return true;
    }
    return false;
}
std::optional<Cell> Navigation::approachGoal(bool reachedOnly) const noexcept {
    const Player party{active_.target, headingToward(active_.target, *active_.preferred)};
    const auto id = actors_.id(owner_);
    std::optional<Cell> best;
    unsigned score = 0;
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x) {
            const auto destination = party.cell + Cell{x, y};
            if (!combatAdjacent(map_, party.cell, destination) || (reachedOnly && !seen_[index(destination)]))
                continue;
            const auto choice = approachSlot(map_, actors_, id, party, destination, true);
            if (!choice)
                continue;
            const unsigned rank = 4 * unsigned(choice->rank) + (destination == active_.from ? 2 : 0) +
                                  (destination == *active_.preferred ? 1 : 0);
            if (rank > score) {
                best = destination;
                score = rank;
            }
        }
    return best;
}
void Navigation::finish(bool found, Cell step) noexcept {
    if (!canceled_)
        results_[owner_] = {
            true, found, active_.from, active_.target, step, active_.adjacent, active_.preferred};
    cleaning_ = true;
    ++completed_;
}
void Navigation::update(Cell party) noexcept {
    invalidateGeometry();
    work_ = completed_ = 0;
    while (work_ < nodeBudget) {
        if (cleaning_) {
            while (cleanup_ < tail_ && work_ < nodeBudget) {
                seen_[queue_[cleanup_++]] = 0;
                ++work_;
            }
            if (cleanup_ != tail_)
                break;
            cleaning_ = false;
            owner_ = requests_.size();
        }
        if (completed_ >= requestBudget)
            break;
        if (owner_ == requests_.size() && !start())
            break;
        if (cleaning_)
            continue;
        const bool approaching =
            active_.adjacent && active_.preferred && cellDistance(*active_.preferred, active_.target) == 1;
        if (approaching && !canceled_) {
            const auto goal = approachGoal(head_ == tail_);
            if (goal && seen_[index(*goal)]) {
                const auto first = first_[index(*goal)];
                finish(true, first == 4 ? active_.from : active_.from + forward(Heading(first)));
                continue;
            }
        }
        if (canceled_ || head_ == tail_) {
            finish(false, {});
            continue;
        }
        const auto current = queue_[head_++];
        const auto c = cell(current);
        ++work_;
        if (!approaching &&
            (active_.adjacent ? combatAdjacent(map_, active_.target, c) : c == active_.target) &&
            !(c == party)) {
            const auto step = first_[current] == 4
                                  ? active_.from
                                  : active_.from + forward(static_cast<Heading>(first_[current]));
            finish(true, step);
            continue;
        }
        std::array<int, 4> directions{0, 1, 2, 3};
        if (preferAvailable_ && first_[current] == 4) {
            // On tied routes, first close the lateral offset to the preferred
            // approach axis rather than walking out onto the flank.
            int first = 0;
            if (active_.preferred->x == active_.target.x && c.x != active_.target.x)
                first = c.x > active_.target.x ? 3 : 1;
            else if (active_.preferred->y == active_.target.y && c.y != active_.target.y)
                first = c.y > active_.target.y ? 0 : 2;
            std::swap(directions[0], directions[first]);
        }
        for (const int d : directions) {
            const auto next = c + forward(static_cast<Heading>(d));
            if (!map_.canStep(c, next) || !map_.npcAllowed(next) || next == party ||
                actors_.occupiedSlots(next) == 15)
                continue;
            const auto nextIndex = index(next);
            if (seen_[nextIndex])
                continue;
            seen_[nextIndex] = 1;
            first_[nextIndex] = first_[current] == 4 ? static_cast<std::uint8_t>(d) : first_[current];
            queue_[tail_++] = nextIndex;
        }
    }
}
} // namespace blobber
