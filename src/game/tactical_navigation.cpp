#include "game/tactical_navigation.hpp"
#include "game/effects.hpp"
#include "game/navigation.hpp"
#include "game/perception.hpp"
#include <algorithm>

namespace blobber {
TacticalNavigation::TacticalNavigation(GeometryView map, const ActorWorld& actors)
    : map_(map), actors_(actors), distance_(std::size_t(map.width()) * map.height() * 4, missing),
      parent_(distance_.size()), heap_(distance_.size()), heapPosition_(distance_.size(), missing),
      route_(distance_.size()), stamp_(distance_.size(), 0) {}
std::uint32_t TacticalNavigation::index(Cell c, Cell s) const noexcept {
    return std::uint32_t((c.y * 2 + s.y) * (map_.width() * 2) + c.x * 2 + s.x);
}
Cell TacticalNavigation::cell(std::uint32_t n) const noexcept {
    return {int(n % (map_.width() * 2)) / 2, int(n / (map_.width() * 2)) / 2};
}
Cell TacticalNavigation::subcell(std::uint32_t n) const noexcept {
    return {int(n % (map_.width() * 2)) % 2, int(n / (map_.width() * 2)) % 2};
}
bool TacticalNavigation::legal(std::uint32_t from, std::uint32_t to) const noexcept {
    const auto a = cell(from), b = cell(to), s = subcell(to);
    if (!map_.npcAllowed(b) || b == party_.cell || (!(a == b) && !map_.canStep(a, b)))
        return false;
    const auto* actor = actors_.actor(actor_);
    return actor && ((actor->npc.cell == b && actor->npc.subcell == s) ||
                     !(actors_.occupiedSlots(b) & (1u << (s.x + s.y * 2))));
}
unsigned TacticalNavigation::cost(std::uint32_t to, const EffectController* effects) const noexcept {
    return effects && effects->areaAt(globalSubcell(cell(to), subcell(to))) ? 10 : 5;
}
void TacticalNavigation::goals() noexcept {
    goalCount_ = 0;
    wantedRank_ = 3;
    const auto* actor = actors_.actor(actor_);
    if (waking_) {
        wantedRank_ = 0;
        for (unsigned i = 0; i < wake_.count; ++i) {
            const auto* ally = actors_.actor(wake_.allies[i]);
            if (!ally)
                continue;
            const auto tile = globalSubcell(ally->npc.cell, ally->npc.subcell);
            for (unsigned h = 0; h < 4; ++h) {
                const auto t = tile + forward(Heading(h));
                const auto c = dungeonCell(t), sub = t + c * -2;
                if (!map_.npcAllowed(c) || c == party_.cell || !lineOfSight(map_, ally->npc.cell, c) ||
                    ((actors_.occupiedSlots(c) & (1u << (sub.x + sub.y * 2))) &&
                     !(actor->npc.cell == c && actor->npc.subcell == sub)))
                    continue;
                goals_[goalCount_++] = {index(c, sub), wake_.rank[i], wake_.allies[i]};
            }
        }
        return;
    }
    const auto f = forward(party_.heading);
    for (int dy = -2; dy <= 2; ++dy)
        for (int dx = -2; dx <= 2; ++dx) {
            const auto c = target_ + Cell{dx, dy};
            if (retreat_ ? std::max(std::abs(dx), std::abs(dy)) != 2 : !combatAdjacent(map_, c, target_))
                continue;
            for (int s = 0; s < 4; ++s) {
                const Cell sub{s % 2, s / 2};
                if (!map_.npcAllowed(c) || c == party_.cell ||
                    ((actors_.occupiedSlots(c) & (1u << s)) &&
                     !(actor->npc.cell == c && actor->npc.subcell == sub)))
                    continue;
                // The eight contact cells each have four slots; retreat's ring
                // has sixteen cells, hence the fixed 64-goal capacity.
                const bool near = f.x ? sub.x == (f.x > 0 ? 0 : 1) : sub.y == (f.y > 0 ? 0 : 1);
                const unsigned rank = retreat_
                                          ? (inPartyView(map_, party_.cell, party_.heading, c, sub) ? 0 : 1)
                                      : !inPartyViewport(party_.cell, party_.heading, c, sub) ? 2
                                      : near                                                  ? 0
                                                                                              : 1;
                goals_[goalCount_++] = {index(c, sub), rank};
                wantedRank_ = std::min(wantedRank_, rank);
            }
        }
}
bool TacticalNavigation::less(std::uint32_t a, std::uint32_t b) const noexcept {
    return distance_[a] < distance_[b] || (distance_[a] == distance_[b] && a < b);
}
void TacticalNavigation::push(std::uint32_t node) noexcept {
    auto pos = heapPosition_[node];
    if (pos == missing) {
        pos = std::uint32_t(heapCount_++);
        heap_[pos] = node;
        heapPosition_[node] = pos;
    }
    while (pos) {
        const auto parent = (pos - 1) / 2;
        if (!less(heap_[pos], heap_[parent]))
            break;
        std::swap(heap_[pos], heap_[parent]);
        heapPosition_[heap_[pos]] = pos;
        heapPosition_[heap_[parent]] = parent;
        pos = parent;
    }
}
std::uint32_t TacticalNavigation::pop() noexcept {
    const auto result = heap_[0];
    heapPosition_[result] = missing;
    if (--heapCount_) {
        heap_[0] = heap_[heapCount_];
        heapPosition_[heap_[0]] = 0;
        std::size_t pos = 0;
        while (pos * 2 + 1 < heapCount_) {
            auto child = pos * 2 + 1;
            if (child + 1 < heapCount_ && less(heap_[child + 1], heap_[child]))
                ++child;
            if (!less(heap_[child], heap_[pos]))
                break;
            std::swap(heap_[child], heap_[pos]);
            heapPosition_[heap_[child]] = std::uint32_t(child);
            heapPosition_[heap_[pos]] = std::uint32_t(pos);
            pos = child;
        }
    }
    return result;
}
TacticalNavigation::Result TacticalNavigation::next(ActorId id, Player party, Cell target, bool retreat,
                                                    const EffectController* effects, std::size_t& work,
                                                    const WakeGoals* wake) noexcept {
    const auto* actor = actors_.actor(id);
    if (!actor)
        return {};
    const auto ground = effects ? effects->ground().revision : 0;
    bool wakeChanged = waking_ != bool(wake);
    if (wake && waking_) {
        wakeChanged |= wake->count != wake_.count || wake->budgetFeet != wake_.budgetFeet;
        for (unsigned i = 0; i < wake->count; ++i)
            wakeChanged |= wake->allies[i].world != wake_.allies[i].world ||
                           wake->allies[i].slot != wake_.allies[i].slot || wake->rank[i] != wake_.rank[i];
    }
    if (!valid_ || wakeChanged || actor_.world != id.world || actor_.slot != id.slot || !(party_ == party) ||
        !(target_ == target) || retreat_ != retreat || geometry_ != map_.revision() ||
        occupancy_ != actors_.snapshot().revision || ground_ != ground ||
        root_ != index(actor->npc.cell, actor->npc.subcell)) {
        waking_ = bool(wake);
        if (wake)
            wake_ = *wake;
        wakeTarget_ = {};
        actor_ = id;
        party_ = party;
        target_ = target;
        retreat_ = retreat;
        geometry_ = map_.revision();
        occupancy_ = actors_.snapshot().revision;
        ground_ = ground;
        root_ = index(actor->npc.cell, actor->npc.subcell);
        reset_ = heapCount_ = routeCount_ = 0;
        if (epoch_ < UINT32_MAX - 1) {
            ++epoch_;
            reset_ = distance_.size();
        } else {
            epoch_ = UINT32_MAX;
        }
        best_ = building_ = missing;
        bestRank_ = UINT32_MAX;
        phase_ = Phase::Reset;
        valid_ = true;
        goals();
    }
    while (work < Navigation::nodeBudget) {
        if (phase_ == Phase::Reset) {
            if (reset_ < distance_.size()) {
                stamp_[reset_] = 0;
                ++reset_;
                ++work;
                continue;
            }
            if (epoch_ == UINT32_MAX)
                epoch_ = 1;
            stamp_[root_] = epoch_;
            distance_[root_] = 0;
            heapPosition_[root_] = missing;
            parent_[root_] = missing;
            push(root_);
            phase_ = goalCount_ ? Phase::Search : Phase::Blocked;
        }
        if (phase_ == Phase::Search) {
            if (!heapCount_) {
                phase_ = best_ == missing ? Phase::Blocked : Phase::Build;
                building_ = best_;
                continue;
            }
            const auto node = pop();
            ++work;
            if (waking_ && best_ != missing && distance_[node] > distance_[best_]) {
                phase_ = Phase::Build;
                building_ = best_;
                continue;
            }
            for (std::size_t g = 0; g < goalCount_; ++g)
                if (goals_[g].node == node &&
                    (waking_ ? (best_ == missing || distance_[node] < distance_[best_] ||
                                (distance_[node] == distance_[best_] && goals_[g].rank < bestRank_))
                             : goals_[g].rank < bestRank_)) {
                    best_ = node;
                    bestRank_ = goals_[g].rank;
                    wakeTarget_ = goals_[g].target;
                    if (!waking_ && bestRank_ == wantedRank_) {
                        phase_ = Phase::Build;
                        building_ = node;
                    }
                }
            if (phase_ == Phase::Build)
                continue;
            const int x = int(node % (map_.width() * 2)), y = int(node / (map_.width() * 2));
            for (unsigned h = 0; h < 4; ++h) {
                const auto delta = forward(Heading(h));
                const int nx = x + delta.x, ny = y + delta.y;
                if (nx < 0 || ny < 0 || nx >= map_.width() * 2 || ny >= map_.height() * 2)
                    continue;
                const auto next = std::uint32_t(ny * map_.width() * 2 + nx);
                if (!legal(node, next))
                    continue;
                if (stamp_[next] != epoch_) {
                    stamp_[next] = epoch_;
                    distance_[next] = heapPosition_[next] = missing;
                }
                const auto edge = cost(next, effects);
                if ((waking_ && distance_[node] + edge > wake_.budgetFeet) ||
                    distance_[node] > UINT32_MAX - edge || distance_[node] + edge >= distance_[next])
                    continue;
                distance_[next] = distance_[node] + edge;
                parent_[next] = node;
                push(next);
            }
        } else if (phase_ == Phase::Build) {
            if (building_ == root_) {
                phase_ = Phase::Ready;
                continue;
            }
            route_[routeCount_++] = building_;
            building_ = parent_[building_];
            ++work;
        } else if (phase_ == Phase::Ready) {
            if (!routeCount_)
                return {Status::AtGoal, {}, {}, 0, 0, wakeTarget_};
            const auto next = route_[routeCount_ - 1];
            if (!legal(root_, next)) {
                valid_ = false;
                return {Status::Pending};
            }
            return {Status::Step,        cell(next),       subcell(next),
                    cost(next, effects), distance_[best_], wakeTarget_};
        } else
            return {};
    }
    return {Status::Pending};
}
void TacticalNavigation::committed() noexcept {
    if (phase_ != Phase::Ready || !routeCount_)
        return;
    root_ = route_[--routeCount_];
    occupancy_ = actors_.snapshot().revision;
}
} // namespace blobber
