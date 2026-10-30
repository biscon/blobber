#include "game/navigation.hpp"
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
    if (slot >= requests_.size() || owner_ == slot || results_[slot].ready)
        return;
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
            const auto heading = headingToward(active_.target, *active_.preferred);
            for (int side = -1; side <= 1; ++side) {
                const auto cell = *active_.preferred + right(heading) * side;
                const bool available = map_.npcAllowed(cell) && combatAdjacent(map_, active_.target, cell) &&
                                       actors_.occupiedSlots(cell) != 15;
                preferAvailable_ |= available;
                if (!side)
                    centerAvailable_ = available;
            }
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
        if (canceled_ || head_ == tail_) {
            finish(frontStep_.has_value() || fallbackStep_.has_value(),
                   frontStep_.value_or(fallbackStep_.value_or(Cell{})));
            continue;
        }
        const auto current = queue_[head_++];
        const auto c = cell(current);
        ++work_;
        if ((active_.adjacent ? combatAdjacent(map_, active_.target, c) : c == active_.target) &&
            !(c == party)) {
            const auto step = first_[current] == 4
                                  ? active_.from
                                  : active_.from + forward(static_cast<Heading>(first_[current]));
            const bool front =
                preferAvailable_ &&
                inMeleeFront(map_, active_.target, headingToward(active_.target, *active_.preferred), c);
            if (!preferAvailable_ || c == *active_.preferred || (front && !centerAvailable_)) {
                finish(true, step);
                continue;
            }
            if (front && !frontStep_)
                frontStep_ = step;
            if (!fallbackStep_)
                fallbackStep_ = step;
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
            if (!map_.npcAllowed(next) || next == party || actors_.occupiedSlots(next) == 15)
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
