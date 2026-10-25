#include "rules/health.hpp"
#include "support/check.hpp"
#include <iostream>
#include <limits>
using namespace blobber;
using namespace blobber::rules;
int main() {
    try {
        const HealthState full{12, Life::Conscious};
        check(validHealth(full, 12), "Valid health");
        check(!takeDamage(full, 12, -1) && !receiveHealing(full, 12, -1), "Negative amounts fail");
        check(!takeDamage({13, Life::Conscious}, 12, 2), "Invalid input never repaired implicitly");
        check(!takeDamage(full, 12, 0).changed, "Zero damage does nothing");
        const auto down = takeDamage(full, 12, 12);
        check(down && down.state.life == Life::Dying && down.state.hitPoints == 0 && !down.died,
              "Exact HP damage downs");
        check(takeDamage(full, 12, 23).state.life == Life::Dying, "Below massive damage threshold");
        check(takeDamage(full, 12, 24).died, "Massive damage at exact threshold");
        check(takeDamage({1, Life::Conscious}, 12, 13).died, "Threshold uses current plus maximum HP");
        check(takeDamage(full, 12, std::numeric_limits<std::int64_t>::max()).died, "Huge damage safe");
        check(deathSave(down.state, 12, 1).state.failures == 2, "Natural one counts twice");
        check(deathSave(down.state, 12, 9).state.failures == 1, "Nine fails");
        check(deathSave(down.state, 12, 10).state.successes == 1, "Ten succeeds");
        auto h = deathSave(down.state, 12, 1).state;
        h = deathSave(h, 12, 10).state;
        const auto dead = deathSave(h, 12, 2);
        check(dead.died && dead.state.life == Life::Dead, "Third nonconsecutive failure kills");
        check(!receiveHealing(dead.state, 12, 10) && !stabilize(dead.state, 12) &&
                  !deathSave(dead.state, 12, 20),
              "Death terminal");
        check(!takeDamage(dead.state, 12, 1).died, "No repeated death event");
        h = down.state;
        for (int i = 0; i < 3; ++i)
            h = deathSave(h, 12, 10).state;
        check(h.life == Life::Stable && !h.successes && !h.failures, "Three successes stabilize and reset");
        check(!stabilize(h, 12).changed && !deathSave(h, 12, 1), "Stable is idempotent and stops saves");
        auto damaged = takeDamage(h, 12, 1);
        check(damaged.state.life == Life::Dying && damaged.state.failures == 1, "Damage breaks stability");
        check(takeDamage(h, 12, 1, true).state.failures == 2, "Critical at zero adds two failures");
        check(takeDamage(h, 12, 12).died, "Damage at zero has massive threshold");
        const auto healed = receiveHealing(damaged.state, 12, 3);
        check(healed.state.hitPoints == 3 && healed.state.life == Life::Conscious && !healed.state.failures,
              "Healing wakes and resets");
        check(receiveHealing(healed.state, 12, std::numeric_limits<std::int64_t>::max()).state.hitPoints ==
                  12,
              "Huge healing capped safely");
        check(deathSave(damaged.state, 12, 20).state.hitPoints == 1, "Natural twenty wakes");
        check(!deathSave(down.state, 12, 0) && !deathSave(down.state, 12, 21), "Invalid rolls rejected");
        check(!stabilize(full, 12) && !deathSave(full, 12, 1), "Conscious not eligible");
        check(receiveHealing(down.state, 12, 0).state.life == Life::Dying, "Zero healing does not wake");
        std::cout << "Health checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
