#include "automation/codec.hpp"
namespace blobber::automation {
namespace {
std::string u(std::uint64_t v) {
    return std::to_string(v);
}
Json xy(Cell c) {
    return Json::array({c.x, c.y});
}
Json status(const StatusSummary& s) {
    Json a = Json::array();
    for (std::size_t i = 0; i < s.count; ++i) {
        const auto& e = s.entries[i];
        a.push_back({{"name", e.name},
                     {"effect", unsigned(e.effect)},
                     {"clock", unsigned(e.clock)},
                     {"remaining_ms", u(e.remainingMs)},
                     {"harmful", e.harmful},
                     {"stacks", e.stacks}});
    }
    return a;
}
Json handle(ItemHandle h) {
    return {{"slot", h.slot}, {"generation", u(h.generation)}};
}
} // namespace
const char* phaseName(CombatPhase p) noexcept {
    const char* names[]{"exploration", "contact",  "turns",  "victory",
                        "defeat",      "approach", "facing", "round_end"};
    return names[unsigned(p)];
}
Json spellList(const rules::Catalog& cat, std::uint32_t bits) {
    Json a = Json::array();
    for (std::uint16_t i = 0; i < cat.spellCount(); ++i)
        if (bits & rules::spellBit({i}))
            a.push_back(cat.spell({i})->id);
    return a;
}
Json encodeObservation(const Observation& o) {
    const auto& g = *o.session;
    const auto& cat = g.party().catalog();
    const auto& combat = g.combat().snapshot();
    const auto& movement = g.movement();
    Json j = {{"diagnostic", o.diagnostic},
              {"time_ms", u(g.timeMs())},
              {"gameplay_ms", u(combat.gameplayMs)},
              {"phase", phaseName(combat.phase)},
              {"round", u(combat.round)},
              {"awaiting_action", combat.awaitingAction},
              {"resolving_action", combat.resolvingAction},
              {"can_end_turn", combat.canEndTurn},
              {"position", xy(movement.player.cell)},
              {"heading", unsigned(movement.player.heading)},
              {"moving", movement.transition.active()},
              {"party", Json::array()},
              {"enemies", Json::array()},
              {"loot", Json::array()},
              {"targets", Json::array()},
              {"active_character", nullptr}};
    const auto& objective = g.objective();
    if (objective.count && (o.diagnostic || objective.known)) {
        j["objective"] = {{"known", objective.known},
                          {"completed", objective.completed},
                          {"reward_issued", objective.rewardIssued},
                          {"capacity_failure", objective.capacityFailure}};
        if (o.diagnostic) {
            j["objective"]["revision"] = u(objective.revision);
            j["objective"]["members"] = Json::array();
            for (unsigned i = 0; i < objective.count; ++i)
                j["objective"]["members"].push_back(objective.members[i].slot);
        }
    }
    const auto& fixtures = g.fixtures();
    j["quest_keys"] = g.party().questKeys();
    j["fixtures"] = Json::array();
    for (unsigned i = 0; i < fixtures.definition().count; ++i) {
        if (!o.diagnostic && !fixtures.known(i))
            continue;
        const auto& f = fixtures.definition().entries[i];
        j["fixtures"].push_back(
            {{"fixture", i},
             {"kind", unsigned(f.kind)},
             {"name", fixtureName(f)},
             {"position", xy(f.cell)},
             {"key", f.key},
             {"completed", o.diagnostic ? fixtures.completed(i) : fixtures.rememberedComplete(i)}});
    }
    if (fixtures.definition().discovery) {
        j["discovered"] = Json::array();
        for (int y = 0; y < o.map->height(); ++y)
            for (int x = 0; x < o.map->width(); ++x)
                if (fixtures.discovered({x, y}))
                    j["discovered"].push_back({x, y, o.map->solid({x, y}) ? 1 : 0});
    }
    if (o.diagnostic) {
        j["fixture_revision"] = u(fixtures.revision());
        j["geometry_revision"] = u(fixtures.geometryRevision());
        j["discovery_revision"] = u(fixtures.discoveryRevision());
        j["fixture_knowledge"] = Json::array();
        for (unsigned i = 0; i < fixtures.definition().count; ++i)
            j["fixture_knowledge"].push_back({fixtures.known(i), fixtures.rememberedComplete(i)});
    }
    if (combat.activeCharacter)
        j["active_character"] = combat.activeCharacter->index;
    j["formation"] = Json::array();
    for (auto id : g.party().snapshot().formation)
        j["formation"].push_back(id.index);
    for (std::uint16_t i = 0; i < 4; ++i) {
        const auto& p = *g.party().character({i});
        Json c = {{"character", i},
                  {"name", p.build.name},
                  {"id", p.build.id},
                  {"race", cat.race(p.build.race)->id},
                  {"class", cat.characterClass(p.build.characterClass)->id},
                  {"level", p.build.level},
                  {"hp", p.hitPoints},
                  {"max_hp", p.stats.maxHitPoints},
                  {"temporary_hp", p.temporaryHitPoints},
                  {"life", unsigned(p.life)},
                  {"death_successes", p.successes},
                  {"death_failures", p.failures},
                  {"mana", p.mana},
                  {"max_mana", p.maxMana},
                  {"hit_dice", p.availableHitDice},
                  {"xp", p.experience},
                  {"armor_class", g.effects().armorClass(rules::CharacterId{i})},
                  {"abilities", p.stats.abilities},
                  {"base_abilities", p.build.baseAbilities},
                  {"saves", p.stats.saves},
                  {"proficiency", p.stats.proficiency},
                  {"initiative", p.stats.initiative},
                  {"carried_weight", p.stats.carriedWeight},
                  {"carrying_capacity", p.stats.carryingCapacity},
                  {"statuses", status(g.effects().statuses(rules::CharacterId{i}))},
                  {"cooldown_ms", u(combat.cooldownMs[i])},
                  {"spent", combat.spent[i]},
                  {"defending", combat.defending[i]},
                  {"second_wind", p.secondWind},
                  {"arcane_recovery", p.arcaneRecovery},
                  {"action_surge", p.actionSurge},
                  {"channel_divinity", p.channelDivinity},
                  {"cantrips", spellList(cat, p.build.spells.cantrips)},
                  {"learned", spellList(cat, p.build.spells.learned)},
                  {"prepared", spellList(cat, p.build.spells.prepared)},
                  {"domain", spellList(cat, p.build.spells.domain)},
                  {"racial", spellList(cat, p.build.spells.racial)},
                  {"inventory", Json::array()}};
        if (auto concentration = g.effects().concentrationStatus({i})) {
            StatusSummary s;
            s.entries[s.count++] = *concentration;
            c["concentration"] = status(s);
        } else
            c["concentration"] = Json::array();
        for (std::uint16_t slot = 0; slot < 52; ++slot) {
            const auto h = g.party().inventory().at({{i}, slot});
            const auto* stack = g.party().inventory().stack(h);
            if (stack)
                c["inventory"].push_back({{"cell", slot},
                                          {"handle", handle(h)},
                                          {"item", cat.item(stack->item)->id},
                                          {"quantity", stack->quantity}});
        }
        if (o.diagnostic) {
            c["xp_fraction"] = p.experienceFraction;
            c["temporary_until_ms"] = u(p.temporaryUntilMs);
            c["aid_bonus"] = p.aidBonus;
        }
        j["party"].push_back(std::move(c));
    }
    for (std::size_t i = 0; i < o.targets.count; ++i)
        j["targets"].push_back(o.targets.ids[i].slot);
    const auto& actors = g.actors().snapshot();
    for (std::size_t i = 0; i < actors.count; ++i) {
        if (!o.diagnostic && !o.visible[i])
            continue;
        const auto& a = actors.actors[i];
        Json e = {{"actor", i},
                  {"type", cat.monster(a.monster)->id},
                  {"position", xy(a.npc.cell)},
                  {"subcell", xy(a.npc.subcell)},
                  {"heading", unsigned(a.npc.facing)},
                  {"health_fraction", a.maxHitPoints ? double(a.hitPoints) / a.maxHitPoints : 0},
                  {"statuses", status(g.effects().statuses(g.actors().id(i)))},
                  {"targetable", o.visible[i]},
                  {"guardian", g.guardian(g.actors().id(i))}};
        if (o.diagnostic) {
            const auto& ai = g.ai().actors[i];
            e.update({{"hp", a.hitPoints},
                      {"max_hp", a.maxHitPoints},
                      {"life", unsigned(a.life)},
                      {"has_death", a.hasDeath},
                      {"has_damage", a.hasDamage},
                      {"damage_time", u(a.damageTimeMs)},
                      {"has_attack", a.hasAttack},
                      {"attack_time", u(a.attackTimeMs)},
                      {"ranged_attack", a.rangedAttack},
                      {"attack_paused", u(a.attackPausedMs)},
                      {"attack_pause_from", u(a.attackPauseFromMs)},
                      {"attack_pause_until", u(a.attackPauseUntilMs)},
                      {"resumable", a.resumableAttack},
                      {"has_walk", a.hasWalk},
                      {"walk_time", u(a.walkTimeMs)},
                      {"walk_duration", u(a.walkDurationMs)},
                      {"has_travel", a.hasTravel},
                      {"walk_from", Json::array({a.walkFrom.x, a.walkFrom.y})},
                      {"awareness", unsigned(ai.awareness)},
                      {"last_seen", xy(ai.lastSeen)},
                      {"last_seen_ms", u(ai.lastSeenMs)},
                      {"move_due_ms", u(ai.moveDueMs)},
                      {"search_start", u(ai.searchStartMs)},
                      {"waypoint", ai.waypoint},
                      {"search_turns", ai.searchTurns}});
        }
        j["enemies"].push_back(std::move(e));
    }
    const auto& loot = g.loot().snapshot();
    for (std::size_t i = 0; i < lootCapacity; ++i)
        if (loot.stacks[i].active && o.lootVisible[i]) {
            const auto& s = loot.stacks[i];
            Json e = {{"handle", {{"slot", i}, {"generation", u(s.generation)}}}, {"position", xy(s.cell)}};
            // Contents are accessible only when the ordinary loot interaction can open the bag.
            if (o.diagnostic || g.lootAccess({loot.world, std::uint16_t(i), s.generation}))
                e.update({{"item", cat.item(s.item)->id}, {"quantity", s.quantity}});
            j["loot"].push_back(std::move(e));
        }
    // Current ground spells are all party-created: their placement and duration are known.
    j["ground"] = Json::array();
    for (const auto& a : g.effects().ground().areas)
        if (a.active)
            j["ground"].push_back(
                {{"cell", xy(a.cell)}, {"caster", a.caster.index}, {"web", a.web}, {"until", u(a.untilMs)}});
    if (o.diagnostic) {
        j["turn"] = u(combat.turn);
        j["pending_count"] = combat.pendingCount;
        j["active_enemy"] = combat.activeEnemy ? Json(combat.activeEnemy->slot) : Json(nullptr);
        j["rest_token"] = u(g.restToken());
        j["inventory_revision"] = u(g.party().inventory().revision());
        j["preparation_revision"] = u(g.party().preparationRevision());
        j["progression_revision"] = u(g.party().progressionRevision());
        j["actor_revision"] = u(actors.revision);
        j["loot_revision"] = u(loot.revision);
        j["ai_time"] = u(g.ai().timeMs);
        j["known_directions"] = combat.knownDirections;
        j["facing"] = combat.facing ? Json(unsigned(*combat.facing)) : Json(nullptr);
        j["transition"] = {{"from", xy(movement.transition.from.cell)},
                           {"from_heading", unsigned(movement.transition.from.heading)},
                           {"action", unsigned(movement.transition.action)},
                           {"elapsed", movement.transition.elapsed},
                           {"duration", movement.transition.durationSeconds},
                           {"revision", u(movement.revision)}};
        j["effects"] = Json::array();
        for (const auto& e : g.effects().snapshot())
            if (e.active) {
                Json targets = Json::array();
                for (std::size_t i = 0; i < e.count; ++i)
                    targets.push_back(
                        {{"npc", e.targets[i].npc},
                         {"id", e.targets[i].npc ? e.targets[i].enemy.slot : e.targets[i].character.index}});
                j["effects"].push_back({{"serial", u(e.id.serial)},
                                        {"kind", unsigned(e.kind)},
                                        {"caster", e.caster.index},
                                        {"targets", targets},
                                        {"difficulty", e.difficulty},
                                        {"expires", u(e.expiresMs)},
                                        {"retry", u(e.retryMs)},
                                        {"duplicates", e.duplicates},
                                        {"reserved", e.reserved},
                                        {"cell", xy(e.cell)},
                                        {"ready", u(e.readyMs)}});
            }
        j["ground"] = Json::array();
        for (const auto& a : g.effects().ground().areas)
            if (a.active)
                j["ground"].push_back({{"serial", u(a.id.serial)},
                                       {"cell", xy(a.cell)},
                                       {"caster", a.caster.index},
                                       {"difficulty", a.difficulty},
                                       {"until", u(a.untilMs)},
                                       {"web", a.web},
                                       {"effect", u(a.effect.serial)},
                                       {"burn", u(a.burnMs)}});
        j["attacks"] = Json::array();
        const auto attack = [&](const AttackVisual& a) {
            if (a.stage == AttackStage::Empty)
                return;
            j["attacks"].push_back({{"serial", u(a.id.serial)},
                                    {"stage", unsigned(a.stage)},
                                    {"enemy", a.enemy.world ? Json(a.enemy.slot) : Json(nullptr)},
                                    {"character", a.character.index},
                                    {"incoming", a.incoming},
                                    {"ranged", a.ranged},
                                    {"hit", a.hit},
                                    {"target_gone", a.targetGone},
                                    {"start", u(a.startedMs)},
                                    {"release", u(a.releaseMs)},
                                    {"impact", u(a.impactMs)},
                                    {"end", u(a.endMs)},
                                    {"spell", unsigned(a.spell)},
                                    {"darts", a.darts}});
        };
        for (const auto& a : g.combat().attacks().attacks)
            attack(a);
        for (const auto& a : g.combat().attacks().spellExtras)
            attack(a);
        for (const auto& a : g.combat().attacks().spellPulses)
            attack(a);
    }
    return j;
}
Json encodeEvent(const CombatOutcome& e, const rules::Catalog& cat, bool diagnostic) {
    if (e.event == CombatEvent::None || (!diagnostic && (e.event == CombatEvent::AttackStarted ||
                                                         e.event == CombatEvent::ProjectileReleased)))
        return nullptr;
    const char* names[]{"none",
                        "started",
                        "attack",
                        "attack_started",
                        "projectile_released",
                        "attack_canceled",
                        "defend",
                        "aid",
                        "death_save",
                        "finished",
                        "move",
                        "round",
                        "spent",
                        "disengaged",
                        "effect",
                        "cast",
                        "healing",
                        "recovery",
                        "spell_save",
                        "guiding_mark"};
    Json j = {
        {"event", names[unsigned(e.event)]},
        {"time_ms", u(e.occurredMs)},
        {"phase", phaseName(e.phase)},
        {"character", e.character.index},
        {"recipient", e.recipient.index},
        {"enemy", e.enemy.world ? Json(e.enemy.slot) : Json(nullptr)},
        {"enemy_attack", e.enemyAttack},
        {"hit", e.hit},
        {"critical", e.critical},
        {"died", e.died},
        {"damage", e.damage},
        {"roll", e.roll},
        {"spell", e.spell == rules::SpellKind::Count ? Json(nullptr) : Json(rules::spellRules(e.spell)->id)},
        {"feature", rules::featureName(e.feature)},
        {"target_gone", e.targetGone},
        {"save_total", e.saveTotal},
        {"save_difficulty", e.saveDifficulty},
        {"darts", e.darts}};
    if (const auto* item = cat.item(e.item))
        j["item"] = item->id;
    if (e.event == CombatEvent::Effect)
        j["effect"] = {
            {"notice", unsigned(e.effect.notice)},
            {"kind", effectName(e.effect.kind)},
            {"reason", unsigned(e.effect.reason)},
            {"caster", e.effect.caster.index},
            {"npc", e.effect.target.npc},
            {"target", e.effect.target.npc ? e.effect.target.enemy.slot : e.effect.target.character.index},
            {"roll", e.effect.roll},
            {"total", e.effect.total},
            {"difficulty", e.effect.difficulty},
            {"succeeded", e.effect.succeeded}};
    if (diagnostic) {
        j["reason"] = unsigned(e.reason);
        j["attack_serial"] = u(e.attackId.serial);
        j["impact_ms"] = e.impactMs;
        j["kind"] = unsigned(e.kind);
        j["effect_serial"] = u(e.effect.id.serial);
        j["effect_gameplay_ms"] = u(e.effect.gameplayMs);
        j["mirror"] = {
            {"serial", u(e.mirror.effect.serial)}, {"duplicate", e.mirror.duplicate}, {"hit", e.mirror.hit}};
    }
    return j;
}
} // namespace blobber::automation
