#pragma once
#include "game/actors.hpp"
#include <memory>

namespace blobber {
enum class RollPurpose { Attack, Save, Concentration, DeathSave, CuttingWords };
struct RollAnswer {
    bool spend = false;
    rules::CharacterId bard{};
};
struct RollDecision {
    std::uint64_t world = 0, serial = 0;
    RollPurpose purpose = RollPurpose::Attack;
    rules::CharacterId character{};
    ActorId enemy{};
    std::int64_t total = 0, difficulty = 0;
    unsigned eligible = 0;
    bool accepts(std::uint64_t expectedWorld, std::uint64_t expectedSerial,
                 RollAnswer answer) const noexcept {
        if (!serial || serial != expectedSerial || world != expectedWorld)
            return false;
        return !answer.spend || purpose != RollPurpose::CuttingWords ||
               (answer.bard.index < rules::partySize && (eligible & (1u << answer.bard.index)));
    }
    int needed() const noexcept {
        return int(purpose == RollPurpose::CuttingWords ? total - difficulty + 1 : difficulty - total);
    }
};
// A Bard session has one cooperative simulation stack. Its caller and simulation
// alternate on the same thread, including across nested saves in multi-target actions.
class RollResolver {
  public:
    RollResolver();
    ~RollResolver();
    RollResolver(const RollResolver&) = delete;
    RollResolver& operator=(const RollResolver&) = delete;
    void run(void (*operation)(void*) noexcept, void* owner) noexcept;
    RollAnswer ask(RollDecision) noexcept;
    bool answer(std::uint64_t world, std::uint64_t serial, RollAnswer) noexcept;
    const RollDecision& pending() const noexcept { return decision_; }
    void close() noexcept;
    void record(const RollDecision& roll, int die) noexcept {
        last_ = roll;
        last_.serial = serial_;
        die_ = die;
    }
    const RollDecision& last() const noexcept { return last_; }
    int die() const noexcept { return die_; }

  private:
    struct Stack;
    std::unique_ptr<Stack> stack_;
    RollDecision decision_, last_;
    int die_ = 0;
    RollAnswer answer_;
    std::uint64_t serial_ = 0;
    bool executing_ = false, closing_ = false;
};
} // namespace blobber
