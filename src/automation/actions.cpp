#include "automation/codec.hpp"
namespace blobber::automation {
Json describeActions(const GameSession& g, const Observation& o) {
    const auto& cat = g.party().catalog();
    Json result = {
        {"combat", Json::array()},
        {"management", {"inventory", "drop", "pickup", "rest", "advancement", "continue", "interact"}},
        {"movement", {"forward", "back", "left", "right", "turn_left", "turn_right"}},
        {"note",
         "Descriptors are not complete combinations. Preview a fully specified action before submission."}};
    const char* featureIds[]{"none",         "second_wind",   "arcane_recovery", "dismiss_concentration",
                             "action_surge", "preserve_life", "cunning_action",  "spiritual_weapon",
                             "escape_web",   "end_turn"};
    const char* names[]{"melee", "back_melee", "defend", "ranged", "spell", "item", "aid", "feature"};
    const auto eligibility = [&](const PartyCommand& c) {
        auto reason = g.movement().transition.active() ? CommandReason::Moving
                                                       : g.combat().validate(c, g.movement().player);
        if (reason == CommandReason::Ready && !o.diagnostic)
            reason = targetVisibility(c, cat, o.targets, g.actors().snapshot());
        return Json{{"ready", reason == CommandReason::Ready}, {"reason", commandMessage(reason)}};
    };
    for (std::uint16_t i = 0; i < 4; ++i) {
        const auto& member = *g.party().character({i});
        Json entry = {{"character", i},
                      {"actions", Json::array()},
                      {"spells", Json::array()},
                      {"features", Json::array()}};
        PartyCommand c;
        c.character = {i};
        c.world = g.actors().snapshot().world;
        c.turn = g.combat().snapshot().turn;
        for (unsigned a = 0; a < 8; ++a) {
            if (a == unsigned(PartyAction::Spell) || a == unsigned(PartyAction::Feature))
                continue;
            c.action = PartyAction(a);
            c.target.reset();
            c.ally.reset();
            Json d = {{"action", names[a]}, {"eligibility", eligibility(c)}};
            if (a == unsigned(PartyAction::Melee) || a == unsigned(PartyAction::BackMelee) ||
                a == unsigned(PartyAction::Ranged)) {
                d["targets"] = Json::array();
                for (std::size_t n = 0; n < o.targets.count; ++n) {
                    c.target = o.targets.ids[n];
                    auto e = eligibility(c);
                    e["actor"] = c.target->slot;
                    d["targets"].push_back(e);
                }
            }
            entry["actions"].push_back(d);
        }
        c = {};
        c.character = {i};
        c.world = g.actors().snapshot().world;
        c.turn = g.combat().snapshot().turn;
        for (std::uint16_t s = 0; s < cat.spellCount(); ++s) {
            const auto access = g.party().spellAccess({i}, {s});
            if (!access)
                continue;
            const auto& spell = *cat.spell({s});
            const auto& rule = *rules::spellRules(spell.kind);
            c.action = PartyAction::Spell;
            c.spell = {s};
            entry["spells"].push_back({{"id", spell.id},
                                       {"name", spell.name},
                                       {"cost", access.cost},
                                       {"hostile", rule.hostile},
                                       {"area", rule.area},
                                       {"ground", rule.ground},
                                       {"self", rule.self},
                                       {"max_targets", rule.maxTargets},
                                       {"eligibility", eligibility(c)}});
        }
        c.spell = {};
        c.action = PartyAction::Feature;
        for (unsigned f = 1; f < 10; ++f) {
            c.feature = rules::Feature(f);
            c.spell = {};
            if (c.feature == rules::Feature::SpiritualWeapon)
                for (std::uint16_t s = 0; s < cat.spellCount(); ++s)
                    if (cat.spell({s})->kind == rules::SpellKind::SpiritualWeapon)
                        c.spell = {s};
            entry["features"].push_back({{"id", featureIds[f]},
                                         {"name", rules::featureName(c.feature)},
                                         {"eligibility", eligibility(c)}});
        }
        const auto request = g.advancementRequest({i});
        const auto learned = rules::advancementChoices(cat, member.build.view(), member.build.portrait,
                                                       member.build.spells, {}, true);
        const auto prepared = rules::advancementChoices(cat, member.build.view(), member.build.portrait,
                                                        member.build.spells, {}, false);
        entry["advancement"] = {{"reason", advancementMessage(g.advancementEligibility(request))},
                                {"learnable", spellList(cat, learned.available)},
                                {"learn_min", learned.minimum},
                                {"learn_max", learned.maximum},
                                {"preparable", spellList(cat, prepared.available)},
                                {"prepare_max", prepared.maximum},
                                {"note", "Prepared choices are additions. Wizard choices may unlock further "
                                         "preparation; preview the complete selection."}};
        result["combat"].push_back(entry);
    }
    result["fixtures"] = Json::array();
    const auto& fixtures = g.fixtures();
    for (unsigned i = 0; i < fixtures.definition().count; ++i) {
        if (!o.diagnostic && !fixtures.known(i))
            continue;
        const auto& f = fixtures.definition().entries[i];
        if (!fixtureReachable(f, g.movement().player.cell, g.movement().player.heading))
            continue;
        result["fixtures"].push_back({{"fixture", i}, {"name", fixtureName(f)}});
    }
    result["targets"] = Json::array();
    for (std::size_t i = 0; i < o.targets.count; ++i)
        result["targets"].push_back(o.targets.ids[i].slot);
    return result;
}
} // namespace blobber::automation
