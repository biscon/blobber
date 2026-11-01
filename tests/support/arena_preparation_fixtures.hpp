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
            choices.learned = rules::recommendedChoices(c, context, rules::SpellChoice::Learned);
            const auto next = rules::previewAdvancement(c, identity, spells, gear, choices);
            if (!next.valid)
                throw std::runtime_error("Fixture arena advancement invalid");
            spells = next.spells;
            ++identity.level;
        }
    }
}
} // namespace blobber
