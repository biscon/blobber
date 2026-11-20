#include "core/preparation.hpp"
#include "game/campaign.hpp"
#include <algorithm>
#include <stdexcept>

namespace blobber {
namespace {
void advanceStartingParty(Party& party, unsigned target) {
    for (std::uint16_t i = 0; i < rules::partySize; ++i) {
        const auto& member = *party.character({i});
        while (unsigned(member.build.level) < target) {
            auto request = party.advancementRequest({i});
            auto& picks = request.choices;
            if (request.level == 3) {
                unsigned points = 2;
                for (const auto ability :
                     {rules::Ability::Constitution, rules::Ability::Strength, rules::Ability::Dexterity,
                      rules::Ability::Wisdom, rules::Ability::Intelligence, rules::Ability::Charisma}) {
                    const auto slot = unsigned(ability);
                    const auto add = std::min(points, unsigned(20 - member.stats.abilities[slot]));
                    picks.abilityImprovement.increases[slot] = add;
                    points -= add;
                }
            }
            for (auto kind :
                 {rules::SpellChoice::Cantrips, rules::SpellChoice::Learned, rules::SpellChoice::Prepared}) {
                auto options =
                    rules::advancementChoices(party.catalog(), member.build.view(), member.build.portrait,
                                              member.build.spells, picks, kind);
                auto selected = rules::recommendedChoices(party.catalog(), options, kind);
                if (kind == rules::SpellChoice::Cantrips)
                    picks.cantrips = selected;
                else if (kind == rules::SpellChoice::Learned)
                    picks.learned = selected;
                else
                    picks.prepared = selected;
            }
            if (!party.advance(request))
                throw std::invalid_argument("Cannot advance the selected starting party");
        }
    }
}
} // namespace
void CampaignRun::startAtDepth(unsigned depth) {
    if (floorLocation(depth) >= campaignLocationCount)
        throw std::invalid_argument("Choose a starting floor from 1 to 6");
    auto saveLocation = [&](unsigned id, GameSession& session) {
        auto& location = locations_[id];
        location.saved = std::make_unique<FloorState>();
        session.exportFloor(*location.saved);
        location.visited = true;
    };
    saveLocation(0, game());
    for (unsigned skipped = 1; skipped < depth; ++skipped) {
        preparationPoint("Preparing earlier dungeon floors");
        const auto id = floorLocation(skipped);
        const auto& l = locations_[id];
        auto session = prepare(id, l.entry);
        const auto& layout = l.generated->layout;
        for (unsigned i = 0; i < layout.encounterCount; ++i) {
            const auto& group = layout.encounters[i];
            if (group.role == generation::EncounterRole::Optional)
                continue;
            for (unsigned n = group.first; n < group.first + group.count; ++n)
                session->actors().damage(session->actors().id(n),
                                         session->actors().snapshot().actors[n].hitPoints, 0);
        }
        session->loot_.reconcile(session->actors());
        session->reconcileRewards();
        if (!session->objective().completed)
            throw std::invalid_argument("Earlier guardian objective did not complete");
        for (unsigned i = 0; i < l.fixtures.count; ++i) {
            const auto& f = l.fixtures.entries[i];
            if (f.kind == FixtureKind::Key) {
                session->completeFixture(i);
                party_.questKeys_ |= std::uint8_t(1u << f.key);
            }
        }
        // The ordinary approach and guardian use chests 0 and 2 in the Underkeep.
        // The cave camp supplies share the first prepared ground placement’s cell.
        for (unsigned i = 0; i < l.recovery.groundCount + l.objective.rewardCount; ++i) {
            const auto slot = std::uint16_t(NpcWorld::maxInstances * lootPerActor + i);
            const auto& loot = session->loot().snapshot();
            const auto& stack = loot.stacks[slot];
            if (stack.container == 1 ||
                (skipped >= 4 && i < l.recovery.groundCount && !(stack.cell == l.recovery.ground[0].cell)))
                continue;
            bool carried = false;
            for (std::uint16_t owner = 0; owner < rules::partySize && !carried; ++owner)
                carried = bool(
                    party_.acquire({{owner}, stack.item, stack.quantity, party_.inventory().revision()}));
            if (!carried || !session->loot_.collect({loot.world, slot, stack.generation}))
                throw std::invalid_argument("Cannot carry the earlier floor rewards");
        }
        saveLocation(id, *session);
        auto& snapshot = *l.saved;
        for (const unsigned chest : {0u, 2u})
            if (chest < snapshot.loot.state.containerCount)
                snapshot.loot.state.containers[chest].opened = true;
        prepare(id, l.entry, true);
    }
    const auto& destination = dungeonFloors[depth - 1];
    advanceStartingParty(party_, destination.entryLevel);
    location_ = floorLocation(depth);
    auto& location = locations_[location_];
    active_ = prepare(location_, location.entry);
    if (!active_->landingSafety(0).safe)
        throw std::invalid_argument("Starting entrance is unsafe");
    location.visited = true;
    active_->campaign_ = this;
    active_->discover();
}
} // namespace blobber
