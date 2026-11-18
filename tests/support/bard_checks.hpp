#pragma once
#include "bard_fixtures.hpp"
#include "game/commands.hpp"
#include "game/creator.hpp"
#include "progression_fixtures.hpp"
#include "support/check.hpp"
namespace blobber::test {
inline void bardProgressionAndRecovery() {
    using namespace rules;
    const auto catalog = bardCatalog();
    const auto bard = catalog.findClass("bard");
    for (std::uint16_t race = 0; race < catalog.raceCount(); ++race) {
        auto draft = recommendedDraft(catalog, {race}, bard);
        std::snprintf(draft.name.data(), draft.name.size(), "Bard");
        CreatorModel model;
        model.draft = draft;
        check(model.validStep(catalog, CreatorStep::Cantrips) &&
                  model.validStep(catalog, CreatorStep::Learned) &&
                  !model.applicable(catalog, CreatorStep::Prepared) && bool(previewBuild(catalog, draft)),
              "Bard creator offers a locked signature cantrip and known spells");
        auto preset = *catalog.preset();
        preset.characters[0] = finishBuild(catalog, draft, "bard.hero");
        Party party(catalog, preset);
        const auto& c = *party.character({0});
        for (int level = 1; level <= 4; ++level) {
            levelBard(party, level);
            const int caps[]{4, 6, 14, 17};
            check(c.maxMana == caps[level - 1] && spellCount(c.build.spells.learned) == unsigned(level + 3) &&
                      spellCount(c.build.spells.cantrips) == unsigned(level == 4 ? 3 : 2) &&
                      !c.build.spells.prepared &&
                      party.spellAccess({0}, catalog.findSpell("vicious_mockery")).casting ==
                          Casting::Charisma,
                  "Bard grants and Charisma mana follow each level");
        }
    }
    const auto map = hybridRoom();
    const auto places = spellPlacements(map, 0);
    GameSession game(map, catalog, places, 2, spellSpawn, false);
    levelBard(game.party(), 2);
    check(game.submit(inspire(game, 0)).reason == CommandReason::InvalidAlly, "Cannot inspire yourself");
    const int uses = game.party().inspirationUses({0});
    check(game.submit(inspire(game, 1)).reason == CommandReason::Resolved &&
              game.party().inspirationUses({0}) == uses - 1 &&
              game.party().character({1})->inspirationSource.index == 0,
          "Grant binds one die and spends one use");
    game.update(6.1, false);
    check(game.submit(inspire(game, 1)).reason == CommandReason::InvalidAlly,
          "Held die cannot be overwritten");
    Party restored(catalog, game.party().checkpoint());
    check(restored.character({1})->inspirationUntil == game.party().character({1})->inspirationUntil &&
              restored.inspirationUses({0}) == uses - 1,
          "Party checkpoint preserves held and unassigned dice");
    game.party().damage({0}, game.party().character({0})->hitPoints - 1);
    RestRequest r;
    r.kind = RestKind::Short;
    r.token = game.restToken();
    r.world = game.actors().snapshot().world;
    r.dice[0] = 1;
    check(game.previewRest(r).song, "An able Bard provides Song of Rest");
    const auto rest = game.rest(r);
    check(rest && rest.songHealing[0] > 0 && !rest.songHealing[1] &&
              !game.party().character({1})->inspirationUntil && game.party().inspirationUses({0}) == uses - 1,
          "One hour expires the die and heals a Hit Die spender");
}
inline void bardAttackDecisions() {
    const auto catalog = bardCatalog();
    const auto map = hybridRoom();
    const auto places = spellPlacements(map, 1);
    bool rescued = false, failed = false, kept = false;
    for (unsigned seed = 1; seed < 80 && !(rescued && failed && kept); ++seed) {
        GameSession game(map, catalog, places, seed, spellSpawn, false);
        check(game.submit(inspire(game, 2)).reason == CommandReason::Resolved, "Give die to archer");
        const auto hp = game.actors().actor(game.actors().id(0))->hitPoints;
        check(game.submit(inspiredShot(game)).reason == CommandReason::Resolved, "Archer accepts shot");
        const auto question = game.pendingRollDecision();
        if (!question.serial)
            continue;
        check(question.purpose == RollPurpose::Attack && question.needed() >= 1 && question.needed() <= 6,
              "Offer precisely a rescuable attack");
        const auto now = game.timeMs(), clock = game.gameTime().elapsedMs;
        game.update(12, false);
        check(game.timeMs() == now && game.gameTime().elapsedMs == clock &&
                  game.submit(inspiredShot(game)).reason == CommandReason::Unavailable && game.saveSafety() &&
                  game.travelSafety(),
              "Prompt freezes sim and blocks new actions");
        const bool spend = kept;
        check(!game.resolveRollDecision(question.world + 1, question.serial, {spend, {}}) &&
                  game.resolveRollDecision(question.world, question.serial, {spend, {}}) &&
                  !game.resolveRollDecision(question.world, question.serial, {spend, {}}),
              "Resolve exactly once");
        check(bool(game.party().character({2})->inspirationUntil) == !spend,
              "Spending always consumes the die");
        const bool hit = spend && game.lastInspirationDie() >= question.needed();
        for (unsigned i = 0; i < 15; ++i)
            game.update(.05, false);
        check((game.actors().actor(game.actors().id(0))->hitPoints < hp) == hit,
              "Impact honors accepted final roll");
        kept |= !spend;
        rescued |= hit;
        failed |= spend && !hit;
    }
    check(kept && rescued && failed, "Deterministic attacks exercise all three choices");
}

inline void competingLoreBards() {
    using namespace rules;
    const auto catalog = bardCatalog();
    const auto map = hybridRoom();
    const NpcWorld close(map, spellSpawn.cell, {"goblin"}, {{{0}, {6, 7}, {0, 1}, Heading::South}});
    auto preset = *catalog.preset();
    preset.characters[1] = preset.characters[0];
    preset.characters[1].id = "second.bard";
    preset.characters[1].name = "Second Bard";
    for (unsigned seed = 1; seed <= 32; ++seed) {
        GameSession game(map, catalog, preset, close, seed, spellSpawn, true);
        levelParty(game, 3);
        for (unsigned tick = 0; tick < 80 && !game.pendingRollDecision().serial; ++tick) {
            const auto& combat = game.combat().snapshot();
            if (combat.awaitingAction && combat.activeCharacter)
                game.submit(featureCommand(game, combat.activeCharacter->index, Feature::EndTurn));
            game.update(.15, false);
        }
        const auto d = game.pendingRollDecision();
        if (!d.serial)
            continue;
        check(d.purpose == RollPurpose::CuttingWords && d.eligible == 3,
              "One incoming attack offers both eligible Bards");
        const auto pool = game.party().inspirationUses({1});
        check(!game.resolveRollDecision(d.world, d.serial, {true, {2}}) &&
                  game.resolveRollDecision(d.world, d.serial, {true, {1}}) &&
                  !game.resolveRollDecision(d.world, d.serial, {true, {0}}),
              "Only one eligible Bard can answer the attack");
        check(game.party().inspirationUses({1}) == pool - 1 && game.party().inspirationUses({0}) == pool,
              "Cutting Words spends only the chosen Bard's unassigned use");
        return;
    }
    check(false, "Reached a potentially preventable enemy hit");
}
inline void bardDeathAndRayDecisions() {
    using namespace rules;
    const auto catalog = bardCatalog();
    const auto map = hybridRoom();
    const auto places = spellPlacements(map, 1);
    bool death = false, rays = false;
    for (unsigned seed = 1; seed <= 64 && !(death && rays); ++seed) {
        if (!death) {
            GameSession game(map, catalog, places, seed, spellSpawn, false);
            game.submit(inspire(game, 1));
            game.party().damage({1}, game.party().character({1})->hitPoints);
            game.submit(inspiredShot(game));
            for (unsigned i = 0; i < 18 && !game.pendingRollDecision().serial; ++i)
                game.update(.5, false);
            const auto d = game.pendingRollDecision();
            if (d.serial) {
                const auto* c = game.party().character({1});
                check(d.purpose == RollPurpose::DeathSave && !c->successes && !c->failures,
                      "Death save counts wait for the player's choice");
                check(game.resolveRollDecision(d.world, d.serial, {true, {}}),
                      "Resolve dying companion's save");
                check(c->successes + c->failures == 1 &&
                          bool(c->successes) == (game.lastInspirationDie() >= d.needed()),
                      "Death save records the final result once");
                death = true;
            }
        }
        if (!rays) {
            GameSession game(map, catalog, places, seed, spellSpawn, false);
            levelParty(game, 3, SpellKind::ScorchingRay);
            game.submit(inspire(game, 3));
            auto cast = spellCommand(game, SpellKind::ScorchingRay);
            cast.targetCount = 1;
            cast.enemies[0] = game.actors().id(0);
            cast.darts[0] = 3;
            const auto mana = game.party().character({3})->mana;
            check(game.submit(cast).reason == CommandReason::Resolved, "Wizard casts all three rays");
            unsigned offered = 0;
            while (game.pendingRollDecision().serial) {
                const auto d = game.pendingRollDecision();
                check(d.purpose == RollPurpose::Attack && ++offered <= 3, "One decision at most per ray");
                check(game.resolveRollDecision(d.world, d.serial, {offered == 2, {}}),
                      "Keep first ray and spend on next");
            }
            if (offered == 2) {
                check(!game.party().character({3})->inspirationUntil &&
                          mana - game.party().character({3})->mana == spellManaCost(2),
                      "A later ray can spend the held die without recasting");
                rays = true;
            }
        }
    }
    check(death && rays, "Reached an inspired death save and two separate rays of one cast");
}
inline void bardSavesAndMockery() {
    using namespace rules;
    const auto catalog = bardCatalog();
    const auto map = hybridRoom();
    const auto places = spellPlacements(map, 0);
    bool saved = false, concentration = false;
    for (unsigned seed = 1; seed <= 64 && !(saved && concentration); ++seed) {
        GameSession game(map, catalog, places, seed, spellSpawn, false);
        levelParty(game, 3, SpellKind::Shatter);
        EffectRequest bless;
        bless.world = game.actors().snapshot().world;
        bless.kind = EffectKind::Bless;
        bless.caster = CharacterId{1};
        bless.targets[0] = CharacterId{1};
        bless.count = 1;
        bless.difficulty = 12;
        check(bool(game.applyEffect(bless)), "Cleric establishes concentration");
        check(game.submit(inspire(game, 1)).reason == CommandReason::Resolved,
              "Inspire the concentrating cleric");
        auto shatter = spellCommand(game, SpellKind::Shatter);
        shatter.character = {0};
        shatter.aimedCell = spellSpawn.cell;
        const int hp = game.party().character({1})->hitPoints;
        check(game.submit(shatter).reason == CommandReason::Resolved,
              "Bard casts Shatter after granting Inspiration");
        auto d = game.pendingRollDecision();
        if (d.serial && d.purpose == RollPurpose::Save) {
            check(game.party().character({1})->hitPoints == hp, "Damage waits for the area save");
            const bool spend = !saved;
            check(game.resolveRollDecision(d.world, d.serial, {spend, {}}), "Resolve area save");
            saved |= spend;
            d = game.pendingRollDecision();
        }
        if (d.serial && d.purpose == RollPurpose::Concentration) {
            const int damaged = game.party().character({1})->hitPoints;
            check(damaged < hp && game.effects().concentration({1}).serial,
                  "Damage landed once, concentration waits");
            check(game.resolveRollDecision(d.world, d.serial, {true, {}}) &&
                      game.party().character({1})->hitPoints == damaged,
                  "Resuming a nested roll never repeats the damage");
            check(bool(game.effects().concentration({1}).serial) == (game.lastInspirationDie() >= d.needed()),
                  "Concentration honors the final roll");
            concentration = true;
        }
    }
    check(saved && concentration, "Reached party and nested concentration saves");
    const auto hostile = spellPlacements(map, 1);
    bool mocked = false;
    for (unsigned seed = 1; seed <= 12 && !mocked; ++seed) {
        GameSession game(map, catalog, hostile, seed, spellSpawn, false);
        auto cast = spellCommand(game, SpellKind::ViciousMockery);
        cast.character = {0};
        cast.targetCount = 1;
        cast.enemies[0] = game.actors().id(0);
        const auto enemy = cast.enemies[0];
        check(game.submit(cast).reason == CommandReason::Resolved, "Bard can taunt a visible enemy");
        const auto status = game.effects().statuses(enemy);
        for (unsigned i = 0; i < status.count; ++i)
            mocked |= status.entries[i].spell == SpellKind::ViciousMockery;
        if (mocked) {
            check(game.effects().attackMode(enemy, CharacterId{0}, false) == RollMode::Disadvantage,
                  "Mockery weakens the enemy's next attack");
            game.update(6.1, false);
            check(game.effects().attackMode(enemy, CharacterId{0}, false) == RollMode::Normal,
                  "Real-time mockery expires");
        }
    }
    check(mocked, "Mockery failed-save and expiry exercised");
}
} // namespace blobber::test
