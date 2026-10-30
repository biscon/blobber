#include "game/navigation.hpp"
#include <stdexcept>
namespace blobber {
Navigation::Saved Navigation::checkpoint() const {
    return {queue_,
            seen_,
            first_,
            requests_,
            results_,
            active_,
            geometryRevision_,
            owner_,
            cursor_,
            head_,
            tail_,
            cleanup_,
            work_,
            completed_,
            cleaning_,
            canceled_,
            preferAvailable_,
            centerAvailable_,
            fallbackStep_,
            frontStep_};
}
void Navigation::restore(const Saved& s) {
    const auto n = queue_.size();
    const auto valid = [&](Cell c) {
        return c.x >= 0 && c.y >= 0 && c.x < map_.width() && c.y < map_.height();
    };
    const auto optional = [&](std::optional<Cell> c) { return !c || valid(*c); };
    if (s.queue.size() != n || s.seen.size() != n || s.first.size() != n || s.owner > requests_.size() ||
        s.cursor >= requests_.size() || s.head > s.tail || s.tail > n || s.cleanup > s.tail ||
        s.work > nodeBudget || s.completed > requestBudget || !optional(s.fallbackStep) ||
        !optional(s.frontStep))
        throw std::invalid_argument("Invalid navigation checkpoint");
    std::vector<bool> queued(n);
    for (std::size_t i = 0; i < n; ++i) {
        if (s.seen[i] > 1 || s.first[i] > 4)
            throw std::invalid_argument("Invalid navigation cell");
        if (i < s.tail) {
            if (s.queue[i] >= n || queued[s.queue[i]])
                throw std::invalid_argument("Invalid navigation queue");
            queued[s.queue[i]] = true;
        }
    }
    std::vector<bool> expectedSeen(n);
    if (s.owner < requests_.size())
        for (std::size_t i = s.cleaning ? s.cleanup : 0; i < s.tail; ++i)
            expectedSeen[s.queue[i]] = true;
    for (std::size_t i = 0; i < n; ++i)
        if (bool(s.seen[i]) != expectedSeen[i])
            throw std::invalid_argument("Inconsistent navigation visited cells");
    for (const auto& r : s.requests)
        if (r.pending && (!valid(r.from) || !valid(r.target) || !optional(r.preferred)))
            throw std::invalid_argument("Invalid navigation request");
    if (s.owner < requests_.size() &&
        (!valid(s.active.from) || !valid(s.active.target) || !optional(s.active.preferred)))
        throw std::invalid_argument("Invalid active navigation request");
    for (const auto& r : s.results)
        if (r.ready &&
            (!valid(r.from) || !valid(r.target) || (r.found && !valid(r.step)) || !optional(r.preferred)))
            throw std::invalid_argument("Invalid navigation result");
    queue_ = s.queue;
    seen_ = s.seen;
    first_ = s.first;
    requests_ = s.requests;
    results_ = s.results;
    active_ = s.active;
    geometryRevision_ = s.geometryRevision;
    owner_ = s.owner;
    cursor_ = s.cursor;
    head_ = s.head;
    tail_ = s.tail;
    cleanup_ = s.cleanup;
    work_ = s.work;
    completed_ = s.completed;
    cleaning_ = s.cleaning;
    canceled_ = s.canceled;
    preferAvailable_ = s.preferAvailable;
    centerAvailable_ = s.centerAvailable;
    fallbackStep_ = s.fallbackStep;
    frontStep_ = s.frontStep;
}
} // namespace blobber
