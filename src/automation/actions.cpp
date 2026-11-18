#include "automation/codec.hpp"
namespace blobber::automation {
Json describeTravel(const CampaignRun& run) {
    const auto recall = run.previewTravel(run.recallRequest());
    Json out = {{"recall",
                 {{"ready", recall.accepted},
                  {"reason", recall.reason},
                  {"cost", 1},
                  {"kits", run.game().party().campSupplies().kits}}},
                {"waypoints", Json::array()}};
    for (unsigned depth = 1; depth <= 3; ++depth)
        if (run.location(depth).visited) {
            const auto check = run.previewTravel(run.waypointRequest(depth));
            out["waypoints"].push_back(
                {{"depth", depth}, {"ready", check.accepted}, {"reason", check.reason}, {"cost", 0}});
        }
    const auto door = run.doorwayRequest();
    const auto check = run.previewTravel(door);
    out["doorway"] = {{"ready", check.accepted}, {"reason", check.reason}, {"cost", 0}};
    return out;
}
Json describeActions(const GameSession& g, const Observation& o) {
    const auto& cat = g.party().catalog();
    Json result = {
        {"combat", Json::array()},
        {"management",
         {"inventory", "drop", "pickup", "open_container", "rest", "advancement", "continue", "interact"}},
        {"movement", {"forward", "back", "left", "right", "turn_left", "turn_right"}},
        {"note",
         "Descriptors are not complete combinations. Preview a fully specified action before submission."}};
    if (g.campaign()) {
        result["management"].push_back("doorway");
        result["management"].push_back("recall");
        result["management"].push_back("waypoint");
        result["travel"] = describeTravel(*g.campaign());
    }
    const char* featureIds[]{
        "none",         "second_wind",   "arcane_recovery",         "dismiss_concentration",
        "action_surge", "preserve_life", "cunning_action",          "spiritual_weapon",
        "escape_web",   "end_turn",      "dismiss_spiritual_weapon", "lay_on_hands", "sacred_weapon", "dismiss_sacred_weapon", "divine_smite"};
    const char* names[]{"melee", "back_melee", "defend", "ranged", "spell", "item", "aid", "feature", "smite"};
    const auto eligibility = [&](const PartyCommand& c) {
        auto reason = g.actionBusy() ? CommandReason::Moving : g.combat().validate(c, g.movement().player);
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
        for (unsigned a = 0; a < unsigned(PartyAction::Count); ++a) {
            if (a == unsigned(PartyAction::Spell) || a == unsigned(PartyAction::Feature))
                continue;
            c.action = PartyAction(a);
            c.target.reset();
            c.ally.reset();
            Json d = {{"action", names[a]}, {"eligibility", eligibility(c)}};
            if (a == unsigned(PartyAction::Melee) || a == unsigned(PartyAction::BackMelee) ||
                a == unsigned(PartyAction::Ranged) || a == unsigned(PartyAction::Smite)) {
                d["targets"] = Json::array();
                for (std::size_t n = 0; n < o.targets.candidateCount; ++n) {
                    c.target = o.targets.candidates[n];
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
        for (unsigned f = 1; f <= unsigned(rules::Feature::DivineSmite); ++f) {
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
        entry["lay_on_hands"] = {{"remaining", member.layOnHands}, {"maximum", g.party().progressionClass({i}) == rules::ProgressionClass::Paladin ? 5 * member.build.level : 0}, {"amount_field", "healing_amount"}, {"minimum", 1}, {"targets", Json::array()}};
        for (std::uint16_t target = 0; target < 4; ++target) {
            const auto* ally = g.party().character({target});
            const int cap = ally && ally->life != rules::Life::Dead ? std::min(member.layOnHands, ally->stats.maxHitPoints - ally->hitPoints) : 0;
            entry["lay_on_hands"]["targets"].push_back({{"character", target}, {"maximum", cap}});
        }
        const auto request = g.advancementRequest({i});
        const auto learned = rules::advancementChoices(cat, member.build.view(), member.build.portrait,
                                                       member.build.spells, {}, true);
        const auto prepared = rules::advancementChoices(cat, member.build.view(), member.build.portrait,
                                                        member.build.spells, {}, false);
        const auto cantrips =
            rules::advancementChoices(cat, member.build.view(), member.build.portrait, member.build.spells,
                                      {}, rules::SpellChoice::Cantrips);
        entry["advancement"] = {
            {"reason", advancementMessage(g.advancementEligibility(request))},
            {"ability_points", member.build.level == 3 ? 2 : 0},
            {"ability_score_cap", 20},
            {"ability_order",
             {"strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma"}},
            {"cantrip_options", spellList(cat, cantrips.available)},
            {"cantrip_min", cantrips.minimum},
            {"cantrip_max", cantrips.maximum},
            {"learnable", spellList(cat, learned.available)},
            {"learn_min", learned.minimum},
            {"learn_max", learned.maximum},
            {"preparable", spellList(cat, prepared.available)},
            {"prepare_max", prepared.maximum},
            {"note", "Prepared choices are additions. Ability improvements and learned spells "
                     "may unlock further preparation; preview the complete selection."}};
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
    for (std::size_t i = 0; i < o.targets.candidateCount; ++i)
        result["targets"].push_back(o.targets.candidates[i].slot);
    return result;
}
} // namespace blobber::automation
