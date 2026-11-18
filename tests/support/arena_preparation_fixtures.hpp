#pragma once
#include "game/arena.hpp"
namespace blobber {
inline void arenaLevel(const rules::Catalog& c, ArenaScenario& s, unsigned level) {
    s.preparation = {level, {}};
    for (unsigned i = 0; i < 4; ++i) {
        auto identity = rules::draftIdentity(s.party[i].draft);
        auto spells = s.party[i].draft.spells;
        const auto gear = rules::previewBuild(c, s.party[i].draft).loadout;
        for (unsigned step = 0; step + 1 < level; ++step) {
            const auto context =
                rules::advancementChoices(c, identity, s.party[i].draft.portrait, spells, {}, true);
            auto& choices = s.preparation.advancements[step][i];
            if (step == 2) {
                choices.abilityImprovement.increases[2] = 2;
                const auto can = rules::advancementChoices(c, identity, s.party[i].draft.portrait, spells,
                                                           choices, rules::SpellChoice::Cantrips);
                choices.cantrips = rules::recommendedChoices(c, can, rules::SpellChoice::Cantrips);
            }
            choices.learned = rules::recommendedChoices(c, context, rules::SpellChoice::Learned);
            const auto prep = rules::advancementChoices(c, identity, s.party[i].draft.portrait, spells, choices, false);
            if (prep.minimum)
                choices.prepared = rules::recommendedChoices(c, prep, rules::SpellChoice::Prepared);
            const auto next = rules::previewAdvancement(c, identity, spells, gear, choices);
            if (!next.valid)
                throw std::runtime_error("Fixture arena advancement invalid");
            spells = next.spells;
            ++identity.level;
            identity.levelFourAsi = next.abilityImprovement;
        }
    }
}
} // namespace blobber
