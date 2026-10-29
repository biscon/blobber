#include "game/commands.hpp"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "ui/interaction.hpp"
#include <iostream>
#include <memory>
using namespace blobber;
using rules::SpellKind;
namespace {
struct EffectsFixture {
    Map map = hybridRoom();
    rules::Catalog catalog = spellCatalog();
    NpcWorld places = spellPlacements(map);
    Party party{catalog};
    ActorWorld actors{map, catalog, places};
    rules::DiceRng rng{5};
    CombatEvents events;
    EffectController effects{party, actors, rng};
    EffectsFixture() {
        effects.bindEvents(events, 0);
        effects.partyCell(spellSpawn.cell);
    }
    void apply(EffectKind kind, unsigned caster, EffectTarget target, int dc = 1000) {
        EffectRequest r;
        r.world = actors.snapshot().world;
        r.kind = kind;
        r.caster = {std::uint16_t(caster)};
        r.targets[0] = target;
        r.count = 1;
        r.difficulty = dc;
        check(bool(effects.apply(r, 0, events)), "Fixture effect applies");
    }
};
void allCasts() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog(1000, 1, 1);
    for (unsigned k = 14; k < unsigned(SpellKind::Count); ++k) {
        const auto kind = SpellKind(k);
        const auto& r = *rules::spellRules(kind);
        const bool close = r.area || kind == SpellKind::ShockingGrasp || kind == SpellKind::PoisonSpray;
        const auto places = spellPlacements(map, close ? 12 : 3, close);
        auto s = std::make_unique<GameSession>(map, catalog, places, 5, spellSpawn, false);
        if (kind == SpellKind::ShockingGrasp) {
            s->party().lockFormation(false);
            s->party().swapFormation(FormationSlot::FrontRight, FormationSlot::BackRight);
            s->party().lockFormation(true);
        }
        if (close)
            spellReady(*s, r.casting == rules::Casting::Wisdom ? 1 : 3);
        auto c = spellCommand(*s, kind);
        if (r.ground)
            c.aimedCell = Cell{6, 7};
        if (!r.area && !r.self) {
            c.targetCount = 1;
            c.allies[0] = {0};
            c.enemies[0] = s->actors().id(close ? 2 : 0);
        }
        const int mana = s->party().character(c.character)->mana;
        const auto out = s->submit(c);
        if (out.reason != CommandReason::Resolved)
            throw std::runtime_error(std::string(r.id) + ": " + commandMessage(out.reason));
        check(s->party().character(c.character)->mana == mana - r.level * 2, "Exactly one spell cost");
        check(s->submit(c).reason != CommandReason::Resolved,
              "Accepted command cannot repeat on same cooldown/turn");
        if (kind == SpellKind::FalseLife)
            check(s->party().character({3})->temporaryHitPoints >= 5, "False Life grants temp HP");
        if (kind == SpellKind::Grease)
            check(s->effects().area({6, 7}), "Grease remains after cast");
        s->update(.5, false);
        if (kind == SpellKind::RayOfFrost)
            check(s->effects().has(c.enemies[0], EffectKind::Slow), "Ray slow arrives at impact");
        if (kind == SpellKind::ShockingGrasp || kind == SpellKind::PoisonSpray)
            check(s->actors().actor(c.enemies[0])->hitPoints < 1000, "New contact damage reaches target");
    }
}
void budgetsAndGround() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog(1);
    const auto places = spellPlacements(map, 3);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    auto c = spellCommand(s, SpellKind::Sleep);
    c.aimedCell = Cell{6, 4};
    const auto preview = s.combat().area(c, spellSpawn);
    check(preview.count == 3 && preview.recipientCount == 3, "Nine-cell preview collects stable occupants");
    check(s.submit(c).reason == CommandReason::Resolved, "Sleep cast");
    for (unsigned i = 0; i < 3; ++i)
        check(s.effects().has(s.actors().id(i), EffectKind::Sleep), "Budget sleeps all low-HP targets");
    check(s.actors().occupied({6, 4}), "Sleep preserves occupancy");
    s.update(6, false);
    c = spellCommand(s, SpellKind::Sleep);
    c.aimedCell = Cell{6, 4};
    const int mana = s.party().character({3})->mana;
    check(s.submit(c).reason == CommandReason::MissingTarget && s.party().character({3})->mana == mana,
          "Already sleeping recipients rejected before cost");
    s.update(54, false);
    check(!s.effects().has(s.actors().id(0), EffectKind::Sleep), "Sleep expires at sixty gameplay seconds");
    const auto empty = spellPlacements(map, 0);
    GameSession friendly(map, catalog, empty, 5, spellSpawn, false);
    for (std::uint16_t i = 0; i < 4; ++i)
        friendly.party().damage({i}, friendly.party().character({i})->hitPoints - 1);
    c = spellCommand(friendly, SpellKind::Sleep);
    c.aimedCell = spellSpawn.cell;
    check(friendly.combat().area(c, spellSpawn).recipientCount == 4,
          "Origin ground area previews all allies");
    check(friendly.submit(c).reason == CommandReason::Resolved, "Friendly sleep accepts origin cell");
    for (std::uint16_t i = 0; i < 4; ++i)
        check(friendly.effects().has(rules::CharacterId{i}, EffectKind::Sleep),
              "Friendly sleep keeps independent health");
    friendly.update(1, false);
    check(friendly.combat().snapshot().phase != CombatPhase::Defeat, "All-party magical sleep is not defeat");
}
void immunityRollbackAndAid() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog(1, 1, 10, false, true, true);
    const auto places = spellPlacements(map, 3);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    auto sleep = spellCommand(s, SpellKind::Sleep);
    sleep.aimedCell = Cell{6, 4};
    check(s.submit(sleep).reason == CommandReason::MissingTarget && s.party().character({3})->mana == 4,
          "Immune-only area rejects without cost");
    check(s.effects().sleepImmune(rules::CharacterId{3}) && s.effects().metalArmor(s.actors().id(0)) &&
              s.effects().metalArmor(rules::CharacterId{0}),
          "Explicit race, monster and armor metadata");
    const auto empty = spellPlacements(map, 0);
    GameSession friendly(map, catalog, empty, 5, spellSpawn, false);
    for (std::uint16_t i = 0; i < 3; ++i)
        friendly.party().damage({i}, friendly.party().character({i})->hitPoints - 1);
    sleep = spellCommand(friendly, SpellKind::Sleep);
    sleep.aimedCell = spellSpawn.cell;
    check(friendly.submit(sleep).reason == CommandReason::Resolved &&
              friendly.effects().canAct(rules::CharacterId{3}),
          "Immune caster remains awake through friendly Sleep");
    friendly.update(6, false);
    PartyCommand aid;
    aid.character = {3};
    aid.action = PartyAction::Aid;
    aid.ally = rules::CharacterId{0};
    aid.world = friendly.actors().snapshot().world;
    aid.turn = friendly.combat().snapshot().turn;
    check(friendly.submit(aid).hit && !friendly.effects().has(rules::CharacterId{0}, EffectKind::Sleep),
          "Aid wakes automatically outside turns");
    const auto normal = spellCatalog(1000);
    GameSession a(map, normal, places, 19, spellSpawn, false), b(map, normal, places, 19, spellSpawn, false);
    auto bad = spellCommand(a, SpellKind::Grease);
    bad.aimedCell = Cell{0, 0};
    check(a.submit(bad).reason != CommandReason::Resolved, "Solid ground rejected");
    for (auto* session : {&a, &b}) {
        auto c = spellCommand(*session, SpellKind::RayOfFrost);
        c.targetCount = 1;
        c.enemies[0] = session->actors().id(0);
        session->submit(c);
        session->update(.4, false);
    }
    check(a.actors().snapshot().actors[0].hitPoints == b.actors().snapshot().actors[0].hitPoints,
          "Rejected ground cast preserves RNG");
    GameSession low(map, normal, places, 5, spellSpawn, false);
    sleep = spellCommand(low, SpellKind::Sleep);
    sleep.aimedCell = Cell{6, 4};
    check(low.submit(sleep).reason == CommandReason::Resolved && low.party().character({3})->mana == 2 &&
              !low.effects().has(low.actors().id(0), EffectKind::Sleep),
          "Insufficient HP budget still spends accepted cast");
}
void sanctuaryRetargetAndWake() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog(1000);
    const auto places = spellPlacements(map, 1, true);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    for (std::uint16_t i = 0; i < 4; ++i) {
        EffectRequest r;
        r.world = s.actors().snapshot().world;
        r.caster = {1};
        r.kind = EffectKind::Sanctuary;
        r.count = 1;
        r.targets[0] = rules::CharacterId{i};
        r.difficulty = 1000;
        check(bool(s.applyEffect(r)), "Ward fixture");
    }
    bool blocked = false;
    for (unsigned n = 0; n < 150 && !blocked; ++n) {
        const auto& state = s.combat().snapshot();
        if (state.awaitingAction)
            s.submit(turnCommand(s));
        const auto& events = s.update(.5, false);
        unsigned saves = 0;
        bool spent = false;
        for (const auto& e : events) {
            saves += e.event == CombatEvent::Effect && e.effect.kind == EffectKind::Sanctuary &&
                     e.effect.notice == EffectNotice::InitialSave;
            spent |= e.event == CombatEvent::Spent;
        }
        if (saves) {
            check(saves == 2 && spent,
                  "Melee attacker tests both legal front targets once, then spends action");
            blocked = true;
        }
    }
    check(blocked, "Sanctuary rejection reached through enemy turn");
    EffectsFixture f;
    const auto hp = f.party.character({3})->hitPoints;
    f.apply(EffectKind::Bless, 3, rules::CharacterId{0});
    f.party.grantTemporary({3}, 2000, 60000);
    f.party.damage({3}, 1000);
    f.effects.damaged({3}, 1000, 0, f.events);
    check(f.party.character({3})->hitPoints == hp && !f.effects.concentration({3}).serial,
          "Concentration uses damage before temporary absorption");
}
void fullAreaBudget() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog(1);
    std::vector<NpcInstance> placements;
    for (unsigned i = 0; i < 36; ++i)
        placements.push_back(
            {{0}, {5 + int(i / 4) % 3, 4 + int(i / 12)}, {int(i % 2), int(i / 2) % 2}, Heading::North});
    NpcWorld world(map, spellSpawn.cell, {"goblin"}, placements);
    GameSession s(map, catalog, world, 5, spellSpawn, false);
    auto c = spellCommand(s, SpellKind::Sleep);
    c.aimedCell = Cell{6, 5};
    check(s.combat().area(c, spellSpawn).count == 36, "Full nine-cell footprint includes all 36 NPCs");
    rules::DiceRng expected(5);
    const auto budget = expected.roll({5, 8, 0}).value.total;
    check(s.submit(c).reason == CommandReason::Resolved, "Full footprint cast accepted");
    for (unsigned i = 0; i < 36; ++i)
        check(s.effects().has(s.actors().id(i), EffectKind::Sleep) == (i < budget),
              "Equal HP recipients follow stable identity order");
}
void stress() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    std::vector<NpcInstance> placements;
    for (unsigned i = 0; i < 256; ++i)
        placements.push_back(
            {{0}, {1 + int(i / 4) % 8, 1 + int(i / 32)}, {int(i % 2), int(i / 2) % 2}, Heading::South});
    NpcWorld world(map, {11, 11}, {"goblin"}, placements);
    Party party(catalog);
    ActorWorld actors(map, catalog, world);
    rules::DiceRng rng(5);
    CombatEvents events;
    EffectController effects(party, actors, rng);
    effects.bindEvents(events, 0);
    effects.partyCell({11, 11});
    for (unsigned i = 0; i < 256; ++i)
        for (auto kind : {EffectKind::Sleep, EffectKind::Blind, EffectKind::Slow, EffectKind::Knockdown})
            effects.condition(actors.id(i), kind, {3}, 0, events);
    for (int i = 0; i < 16; ++i)
        effects.createArea({1 + i % 8, 1 + i / 8}, {3}, 1000, 0, events);
    events.clear();
    effects.advance(60000, 60000, true, events);
    check(events.count > 1000 && events.count < CombatEvents::capacity,
          "Maximum conditions and persistent areas drain within event capacity");
    for (unsigned i = 0; i < 256; ++i)
        check(effects.canAct(actors.id(i)), "All max-capacity conditions eventually expire");
    std::cout << "Maximum-capacity event batch: " << events.count << " / " << CombatEvents::capacity << '\n';
}
void tempHealth() {
    rules::HealthState h{7, rules::Life::Conscious, 0, 0, 8};
    auto r = rules::takeDamage(h, 10, 6, false);
    check(r.state.hitPoints == 7 && r.state.temporaryHitPoints == 2, "Buffer absorbs first");
    r = rules::takeDamage(r.state, 10, 5, false);
    check(r.state.hitPoints == 4 && !r.state.temporaryHitPoints, "Excess reaches HP");
    r = rules::takeDamage(h, 10, INT64_MAX, false);
    check(r.died && !r.state.temporaryHitPoints, "Huge damage safe and death clears buffer");
    EffectsFixture f;
    const auto hp = f.party.character({3})->hitPoints;
    check(f.party.grantTemporary({3}, 8, 100) && f.party.grantTemporary({3}, 5, 200),
          "Recast keeps higher pool");
    check(f.party.character({3})->temporaryHitPoints == 8 && f.party.character({3})->hitPoints == hp,
          "Temp HP never heals or stacks");
    f.effects.advance(199, 199, true, f.events);
    check(f.party.character({3})->temporaryHitPoints == 8, "Recast refreshes lifetime");
    f.effects.advance(200, 200, true, f.events);
    check(!f.party.character({3})->temporaryHitPoints, "Temp expiry exact");
    f.party.damage({0}, f.party.character({0})->hitPoints);
    f.party.grantTemporary({0}, 6, 1000);
    check(f.party.character({0})->life == rules::Life::Dying, "Temp HP never revives");
    check(rules::deathSave(*f.party.character({0}), f.party.character({0})->stats.maxHitPoints, 20, 100)
                  .state.hitPoints == 1,
          "Natural twenty still revives");
    check(rules::deathSave(*f.party.character({0}), f.party.character({0})->stats.maxHitPoints, 1, 100)
                  .state.failures == 2,
          "Natural one ignores positive bonuses");
}
void conditionsAndOneShots() {
    EffectsFixture f;
    const rules::CharacterId wizard{3}, fighter{0};
    const auto enemy = f.actors.id(0);
    f.apply(EffectKind::Bless, 3, fighter);
    f.effects.condition(wizard, EffectKind::Knockdown, wizard, 0, f.events);
    check(!f.effects.canAct(wizard) && f.effects.concentration(wizard).serial,
          "Knockdown blocks actions but preserves concentration");
    check(f.effects.stand(wizard, 0, f.events) && !f.effects.stand(wizard, 0, f.events),
          "Standing recovers once");
    f.effects.condition(wizard, EffectKind::Blind, wizard, 0, f.events);
    check(f.effects.canAct(wizard) && !f.effects.sees(wizard) && f.effects.concentration(wizard).serial,
          "Blind preserves acting and concentration");
    check(f.effects.attackMode(wizard, enemy, false) == rules::RollMode::Disadvantage,
          "Blind outgoing disadvantage");
    f.effects.condition(enemy, EffectKind::Sleep, wizard, 0, f.events);
    check(f.effects.attackMode(wizard, enemy, false) == rules::RollMode::Normal,
          "Advantage/disadvantage cancel");
    f.effects.condition(wizard, EffectKind::Sleep, wizard, 0, f.events);
    check(!f.effects.concentration(wizard).serial, "Sleep breaks concentration");
    f.effects.wake(wizard, 0, f.events);
    check(f.effects.canAct(wizard), "Waking has no stand-up penalty");
    f.apply(EffectKind::Resistance, 1, enemy);
    f.effects.save(enemy, rules::Ability::Dexterity, 10);
    check(f.effects.has(enemy, EffectKind::Resistance), "Automatic failure does not consume Resistance");
    f.effects.wake(enemy, 0, f.events);
    f.effects.save(enemy, rules::Ability::Dexterity, 10);
    check(!f.effects.has(enemy, EffectKind::Resistance), "Rolled save consumes Resistance");
    f.apply(EffectKind::Guidance, 1, fighter);
    check(f.effects.oneShot(fighter, EffectKind::Guidance) >= 1 &&
              f.effects.oneShot(fighter, EffectKind::Guidance) == 0,
          "One eligible ability roll gets bonus");
    f.effects.advance(3000, 3000, true, f.events);
    f.effects.clockMode(true);
    f.effects.advance(9000, 4000, false, f.events);
    check(f.effects.has(wizard, EffectKind::Blind), "Short condition waits for scheduled caster turn");
    f.effects.casterTurn(wizard, 4000, f.events);
    check(!f.effects.has(wizard, EffectKind::Blind), "Caster turn expires blind before action");
    f.effects.condition(enemy, EffectKind::Slow, wizard, 4000, f.events);
    f.effects.clockMode(false);
    f.effects.advance(14999, 9999, true, f.events);
    check(f.effects.has(enemy, EffectKind::Slow), "Exit preserves remaining real-time budget");
    f.effects.advance(15000, 10000, true, f.events);
    check(!f.effects.has(enemy, EffectKind::Slow), "Short condition expiry after phase transition");
}
void damageWakeAndBlindAi() {
    EffectsFixture f;
    const rules::CharacterId wizard{3};
    f.party.grantTemporary(wizard, 8, 60000);
    f.effects.condition(wizard, EffectKind::Sleep, wizard, 0, f.events);
    f.effects.damaged(wizard, 0, 0, f.events);
    check(f.effects.has(wizard, EffectKind::Sleep), "Zero damage never wakes");
    const int hp = f.party.character(wizard)->hitPoints;
    f.party.damage(wizard, 2);
    f.effects.damaged(wizard, 2, 0, f.events);
    check(!f.effects.has(wizard, EffectKind::Sleep) && f.party.character(wizard)->hitPoints == hp &&
              f.party.character(wizard)->temporaryHitPoints == 6,
          "Damage wakes even when entirely absorbed by temporary HP");
    const auto enemy = f.actors.id(0);
    NpcAiController ai(f.map, f.actors, 5);
    ai.bindEffects(&f.effects);
    f.effects.condition(enemy, EffectKind::Blind, wizard, 0, f.events);
    ai.update(100, 100, spellSpawn);
    check(!ai.aware(enemy), "Blind NPC cannot newly acquire visible party");
    ai.alert(enemy, {6, 8});
    const auto remembered = ai.snapshot().actors[enemy.slot].lastSeen;
    ai.update(100, 200, {{7, 8}, Heading::North});
    check(ai.snapshot().actors[enemy.slot].lastSeen == remembered, "Blind NPC retains last known location");
    const auto map = hybridRoom();
    const auto catalog = spellCatalog(1000, 30);
    const auto places = spellPlacements(map, 1);
    GameSession miss(map, catalog, places, 5, spellSpawn, false);
    auto c = spellCommand(miss, SpellKind::RayOfFrost);
    c.targetCount = 1;
    c.enemies[0] = miss.actors().id(0);
    miss.submit(c);
    miss.update(.4, false);
    check(!miss.effects().has(c.enemies[0], EffectKind::Slow) &&
              miss.actors().actor(c.enemies[0])->hitPoints == 1000,
          "Missed Ray neither damages nor slows");
}
void hazardsAndMovement() {
    EffectsFixture f;
    const auto enemy = f.actors.id(0);
    const auto a = f.effects.createArea({6, 4}, {3}, 1000, 0, f.events);
    check(a.serial && f.effects.createArea({6, 4}, {3}, 999, 0, f.events).serial,
          "Overlaps retain identities");
    f.events.clear();
    f.effects.groundCheck(enemy, {6, 4}, 0, f.events);
    check(f.effects.has(enemy, EffectKind::Knockdown) && f.events.count == 2,
          "Overlap checks only strongest patch once");
    f.effects.groundCheck(enemy, {6, 4}, 0, f.events);
    check(f.events.count == 3, "Repeated knockdown does not stack recovery");
    f.effects.clockMode(true);
    check(f.effects.stand(enemy, 0, f.events), "Turn begins by standing");
    f.effects.groundCheck(enemy, {6, 4}, 0, f.events);
    check(!f.effects.canAct(enemy), "End-turn Grease can knock standing creature down again");
    f.effects.clockMode(false);
    f.effects.advance(6000, 6000, true, f.events);
    check(f.effects.has(enemy, EffectKind::Knockdown), "Stationary hazard recurs after automatic recovery");
    for (int i = 2; i < 16; ++i)
        check(f.effects.createArea({3 + i % 5, 3 + i / 5}, {3}, 10, 6000, f.events).serial, "Fill area pool");
    check(!f.effects.canCreateArea() && !f.effects.createArea({4, 4}, {3}, 10, 6000, f.events).serial,
          "Full pool rejects without eviction");
    f.effects.advance(66000, 66000, true, f.events);
    check(f.effects.canCreateArea() && !f.effects.area({6, 4}), "Patch expiry releases storage");
    f.effects.condition(enemy, EffectKind::Slow, {3}, 66000, f.events);
    NpcAiController ai(f.map, f.actors, 5);
    ai.bindEffects(&f.effects);
    ai.turnMovement(true);
    const auto before = f.actors.actor(enemy)->npc.cell;
    check(ai.approach(enemy, spellSpawn, 66000) && f.actors.actor(enemy)->npc.cell == before,
          "Slowed turn prepares without movement");
    check(ai.approach(enemy, spellSpawn, 66001) && !(f.actors.actor(enemy)->npc.cell == before),
          "Second movement action commits");
    const auto* actor = f.actors.actor(enemy);
    check(actor->walkDurationMs == 1200 && actorWalking(*actor, 67200),
          "Committed NPC travel visibly takes double time");
    f.effects.advance(72000, 72000, true, f.events);
    check(actor->walkDurationMs == 1200, "Expiry does not retime committed movement");
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto empty = spellPlacements(map, 0);
    GameSession s(map, catalog, empty, 5, spellSpawn, false);
    auto grease = spellCommand(s, SpellKind::Grease);
    grease.aimedCell = Cell{6, 7};
    check(s.submit(grease).reason == CommandReason::Resolved, "Empty ground accepts Grease");
    s.move(Action::Forward);
    check(s.movement().transition.durationSeconds == .320f,
          "Party step into Grease has double animation duration");
    s.update(.160, false);
    check(s.movement().transition.active(), "Half-time movement still in progress");
    s.update(.160, false);
    check(!s.movement().transition.active(), "Slowed step completes");
    RestRequest rest;
    rest.world = s.actors().snapshot().world;
    rest.token = s.restToken();
    rest.kind = RestKind::Short;
    check(s.previewRest(rest).error == RestError::Unsafe, "Grease beneath party prevents resting");
}
void wardsAndUi() {
    EffectsFixture f;
    const auto enemy = f.actors.id(0);
    f.apply(EffectKind::Sanctuary, 1, rules::CharacterId{0});
    check(!f.effects.wardAllows(enemy, rules::CharacterId{0}, 0, f.events),
          "Sanctuary failed save blocks target");
    f.effects.breakWard(rules::CharacterId{0}, 0, f.events);
    check(f.effects.wardAllows(enemy, rules::CharacterId{0}, 0, f.events), "Accepted offense removes ward");
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto empty = spellPlacements(map, 0);
    GameSession s(map, catalog, empty, 5, spellSpawn, false);
    CommandController ui(s);
    ui.refresh({}, false, false, true);
    ui.selectCharacter(3);
    ui.openSpellbook();
    std::size_t index = 0;
    while (ui.snapshot().magic.entries[index].kind != SpellKind::Grease)
        ++index;
    ui.chooseSpellbook(index);
    ui.magicGround(Cell{6, 7}, true);
    check(ui.snapshot().magic.pending.aimedCell == std::optional<Cell>{{6, 7}} &&
              ui.floorPreview().eligible[0],
          "Ground UI selection and preview agree");
    ui.magicKeyboardTarget(1);
    check(!(ui.snapshot().magic.pending.aimedCell == std::optional<Cell>{{6, 7}}),
          "Keyboard cycles ground cells");
    ui.magicGround(Cell{6, 7}, true);
    check(ui.confirm() == CommandReason::Resolved, "Ground UI confirms cast");
    check(ui.floorPreview().ground.revision > 0, "Persistent ground survives targeting completion");
}
} // namespace
int main(int argc, char**) {
    try {
        if (argc > 1) {
            stress();
            return 0;
        }
        allCasts();
        fullAreaBudget();
        immunityRollbackAndAid();
        sanctuaryRetargetAndWake();
        budgetsAndGround();
        tempHealth();
        conditionsAndOneShots();
        damageWakeAndBlindAi();
        hazardsAndMovement();
        wardsAndUi();
        std::cout << "Expanded spells, conditions, timing, ground targeting and resources passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
