#pragma once
#include "creation_fixtures.hpp"
#include "game/session.hpp"
namespace blobber {
inline void levelParty(GameSession& s, int level = 3, rules::SpellKind chosen = rules::SpellKind::Count) {
    auto& party = s.party();
    for (int next = 2; next <= level; ++next) {
        party.grantNextLevelExperience();
        for (std::uint16_t i = 0; i < 4; ++i) {
            auto request = party.advancementRequest({i});
            const auto& c = *party.character({i});
            auto context = rules::advancementChoices(party.catalog(), c.build.view(), c.build.portrait,
                                                     c.build.spells, {}, true);
            const auto* rule = rules::spellRules(chosen);
            const auto bit = rule ? rules::spellBit(party.catalog().findSpell(rule->id)) : 0;
            if (next == 3 && (context.available & bit))
                request.choices.learned = bit;
            for (unsigned j = 0; j < 32 && rules::spellCount(request.choices.learned) < context.minimum; ++j)
                if (context.available & (1u << j))
                    request.choices.learned |= 1u << j;
            context = rules::advancementChoices(party.catalog(), c.build.view(), c.build.portrait,
                                                c.build.spells, request.choices, false);
            if (next == 3 && (context.available & bit))
                request.choices.prepared = bit;
            if (!party.advance(request))
                throw std::runtime_error("Fixture advancement failed");
        }
    }
}
inline void levelReady(GameSession& s, unsigned character) {
    for (int i = 0; i < 300; ++i) {
        const auto& state = s.combat().snapshot();
        if (state.awaitingAction && state.activeCharacter && state.activeCharacter->index == character)
            return;
        if (state.awaitingAction && state.activeCharacter)
            s.submit(featureCommand(s, state.activeCharacter->index, rules::Feature::EndTurn));
        s.update(.5, false);
    }
    throw std::runtime_error("Level fixture could not reach requested turn");
}
} // namespace blobber
