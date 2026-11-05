#pragma once
#include "assets/rules_loader.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "support/spell_parties.hpp"
#include <fstream>
#include <iostream>
namespace blobber::test {
inline void spellEffectsReview(const std::filesystem::path& assets, const std::filesystem::path& output,
                               const char* selected = nullptr, bool se02 = false, unsigned frameStep = 60,
                               bool se03 = false) {
    if (std::filesystem::exists(output) && !std::filesystem::is_empty(output))
        throw std::runtime_error("SE01 review output must be new or empty");
    std::filesystem::create_directories(output);
    const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
    const auto catalog = loadRules(assets / "rules/catalog.json");
    Window window(1920, 1080, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "SE01 review remains hidden");
    GpuResources resources;
    Presenter presenter(resources, assets);
    WorldRenderer renderer(resources, world_assets::load(assets / "prepared/world"));
    struct Case {
        const char* name;
        rules::SpellKind spell;
        int range, count;
        bool motion, bright;
    };
    const Case foundation[]{{"fire-bolt", rules::SpellKind::FireBolt, 3, 1, true, false},
                            {"ray-of-frost", rules::SpellKind::RayOfFrost, 2, 1, true, false},
                            {"burning-hands", rules::SpellKind::BurningHands, 1, 3, true, false},
                            {"fire-near", rules::SpellKind::FireBolt, 1, 1, false, false},
                            {"frost-far", rules::SpellKind::RayOfFrost, 4, 1, false, false},
                            {"cone-sparse", rules::SpellKind::BurningHands, 1, 1, false, false},
                            {"fire-bright", rules::SpellKind::FireBolt, 2, 1, false, true}};
    const Case offensive[]{{"magic-missile", rules::SpellKind::MagicMissile, 3, 3, true, false},
                           {"missile-single", rules::SpellKind::MagicMissile, 3, 1, true, false},
                           {"scorching-ray", rules::SpellKind::ScorchingRay, 3, 3, true, false},
                           {"guiding-bolt", rules::SpellKind::GuidingBolt, 3, 1, true, false},
                           {"sacred-flame", rules::SpellKind::SacredFlame, 3, 1, true, false},
                           {"shocking-grasp", rules::SpellKind::ShockingGrasp, 1, 1, true, false},
                           {"inflict-wounds", rules::SpellKind::InflictWounds, 1, 1, true, false},
                           {"acid-splash", rules::SpellKind::AcidSplash, 3, 2, true, false},
                           {"poison-spray", rules::SpellKind::PoisonSpray, 1, 1, true, false},
                           {"thunderwave", rules::SpellKind::Thunderwave, 1, 1, true, false},
                           {"shatter", rules::SpellKind::Shatter, 3, 3, true, false},
                           {"radiant-bright", rules::SpellKind::GuidingBolt, 3, 1, true, true},
                           {"portrait-healing", rules::SpellKind::CureWounds, 3, 1, true, false},
                           {"portrait-buff", rules::SpellKind::Bless, 3, 1, true, false},
                           {"portrait-damage", rules::SpellKind::SacredFlame, 1, 1, true, false},
                           {"portrait-weapon", rules::SpellKind::SacredFlame, 1, 1, true, false}};
    const Case support[]{{"stabilize", rules::SpellKind::SpareTheDying, 3, 1, true, false},
                         {"cleanse", rules::SpellKind::LesserRestoration, 3, 1, true, false},
                         {"aid", rules::SpellKind::Aid, 3, 1, true, false},
                         {"mirror", rules::SpellKind::MirrorImage, 3, 1, true, false},
                         {"faith", rules::SpellKind::ShieldOfFaith, 3, 1, true, false},
                         {"sanctuary", rules::SpellKind::Sanctuary, 3, 1, true, true},
                         {"sleep", rules::SpellKind::Sleep, 3, 3, true, false},
                         {"prism", rules::SpellKind::ColorSpray, 1, 3, true, false},
                         {"hold", rules::SpellKind::HoldPerson, 3, 1, true, false},
                         {"bane", rules::SpellKind::Bane, 3, 3, true, false},
                         {"grease", rules::SpellKind::Grease, 2, 1, true, false},
                         {"web", rules::SpellKind::Web, 2, 1, true, false},
                         {"web-burn", rules::SpellKind::Web, 2, 1, true, false},
                         {"weapon", rules::SpellKind::SpiritualWeapon, 2, 1, true, false},
                         {"weapon-idle", rules::SpellKind::SpiritualWeapon, 1, 3, true, false},
                         {"weapon-move", rules::SpellKind::SpiritualWeapon, 2, 1, true, false},
                         {"npc-bless", rules::SpellKind::Bless, 1, 3, true, false}};
    const auto* cases = se03 ? support : se02 ? offensive : foundation;
    const unsigned caseCount = se03   ? unsigned(std::size(support))
                               : se02 ? unsigned(std::size(offensive))
                                      : unsigned(std::size(foundation));
    std::ofstream report(output / "checks.txt");
    for (unsigned exampleIndex = 0; exampleIndex < caseCount; ++exampleIndex) {
        const auto& example = cases[exampleIndex];
        const bool weaponIncoming = std::string_view(example.name) == "portrait-weapon";
        const bool npcBless = std::string_view(example.name) == "npc-bless";
        const bool incoming =
            npcBless || weaponIncoming || std::string_view(example.name) == "portrait-damage";
        if (selected && std::string_view(selected) != example.name)
            continue;
        WorldDefinition definition;
        definition.assetPack = example.bright ? "sanctuary-city" : "underkeep";
        if (example.bright) {
            definition.surfaces.resize(13 * 13);
            for (auto& s : definition.surfaces) {
                s.environment = Environment::Daylight;
                s.ceiling = noCeiling;
            }
        }
        std::vector<std::string> rows(13, "#...........#");
        rows.front() = rows.back() = "#############";
        if (std::string_view(example.name) == "fire-bolt")
            for (unsigned y = 2; y <= 9; ++y)
                rows[y][5] = rows[y][7] = '#';
        const Map map(rows, definition);
        const Player pose{{6, 8}, Heading::North};
        const auto name = weaponIncoming                                            ? "goblin"
                          : incoming                                                ? "goblin_shaman"
                          : se03 || example.spell == rules::SpellKind::BurningHands ? "goblin"
                                                                                    : "bugbear";
        const auto found = std::find(definitions.types.begin(), definitions.types.end(), name);
        check(found != definitions.types.end(), "Review species has production art");
        const NpcTypeId type{std::uint16_t(found - definitions.types.begin())};
        std::vector<NpcInstance> instances;
        for (int i = 0; i < example.count; ++i)
            instances.push_back(
                {npcBless && i > 0 ? NpcTypeId{std::uint16_t(std::find(definitions.types.begin(),
                                                                       definitions.types.end(), "goblin") -
                                                             definitions.types.begin())}
                                   : type,
                 {6 + (example.spell != rules::SpellKind::AcidSplash && example.count > 1 ? i - 1 : 0),
                  8 - example.range},
                 {example.spell == rules::SpellKind::AcidSplash ? i : 0, weaponIncoming ? 0 : 1},
                 Heading::South});
        NpcWorld places(map, pose.cell, definitions.types, instances);
        const auto party = leveledSpellParty(catalog, example.spell);
        const unsigned seed = std::string_view(example.name) == "web-burn" ? 3 : 5;
        GameSession game(map, catalog, party, places, seed, pose, false, definitions.timings);
        prepareSpellParty(game.party(), example.spell);
        if (example.spell == rules::SpellKind::ShockingGrasp) {
            game.party().lockFormation(false);
            game.party().swapFormation(FormationSlot::FrontRight, FormationSlot::BackRight);
            game.party().lockFormation(true);
        }
        CommandController commands(game, definitions.timings);
        NpcAnimator animator(game.actors(), definitions.timings);
        GameView view(renderer);
        auto present = [&] {
            animator.setTime(game.timeMs(), &game.effects());
            SpellArea visibleArea;
            visibleArea.ground = game.effects().ground();
            view.update(game, animator.snapshot(), visibleArea);
            commands.refresh(view.picking() ? view.picking()->targets() : TargetSnapshot{}, false, false,
                             true);
            view.present(presenter, game, map, false, true, &commands.snapshot());
        };
        if (example.range == 1 && !incoming)
            spellReady(game, rules::spellRules(example.spell)->casting == rules::Casting::Wisdom ? 1 : 3);
        present();
        if (incoming) {
            bool resolved = false;
            for (unsigned step = 0; step < (weaponIncoming ? 4500u : 1500u) && !resolved; ++step) {
                if (game.combat().snapshot().awaitingAction) {
                    auto turn =
                        turnCommand(game, weaponIncoming ? PartyAction::Feature : PartyAction::Defend);
                    if (weaponIncoming)
                        turn.feature = rules::Feature::EndTurn;
                    commands.record(game.submit(turn));
                }
                const auto& events = game.update(.02, false);
                if (weaponIncoming)
                    for (const auto& event : events)
                        if (event.event == CombatEvent::Attack)
                            report << "weapon outcome incoming=" << event.enemyAttack << " hit=" << event.hit
                                   << " damage=" << event.damage << " at=" << game.timeMs() << '\n';
                for (const auto& event : events)
                    if (!npcBless && event.event == CombatEvent::Attack && event.enemyAttack &&
                        event.spell ==
                            (weaponIncoming ? rules::SpellKind::Count : rules::SpellKind::SacredFlame) &&
                        event.hit && event.damage > 0)
                        resolved = true;
                if (npcBless)
                    for (const auto& event : events)
                        resolved |= event.event == CombatEvent::Effect &&
                                    event.effect.kind == EffectKind::Bless &&
                                    event.effect.notice == EffectNotice::Applied && event.effect.caster.npc;
                commands.record(events);
            }
            check(resolved, "Production enemy attack resolves onto portrait");
        } else {
            auto command = spellCommand(game, example.spell);
            if (se03) {
                const auto& rule = *rules::spellRules(example.spell);
                if (rule.ground)
                    command.aimedCell = Cell{6, 8 - example.range};
                if (!rule.area && !rule.self) {
                    command.targetCount =
                        example.spell == rules::SpellKind::Aid || example.spell == rules::SpellKind::Bane ? 3
                                                                                                          : 1;
                    command.allies = {{{0}, {1}, {2}}};
                    for (unsigned i = 0; i < command.targetCount; ++i)
                        command.enemies[i] = game.actors().id(i);
                }
                if (example.spell == rules::SpellKind::SpareTheDying)
                    game.party().damage({0}, game.party().character({0})->hitPoints);
                if (example.spell == rules::SpellKind::LesserRestoration) {
                    EffectRequest effect;
                    effect.world = game.actors().snapshot().world;
                    effect.caster = rules::CharacterId{3};
                    effect.kind = EffectKind::HoldPerson;
                    effect.targets[0] = rules::CharacterId{0};
                    effect.count = 1;
                    effect.difficulty = 100;
                    check(bool(game.applyEffect(effect)), "Cleanse review applies actual paralysis");
                    command.restoreParalysis = true;
                }
            } else if (example.spell == rules::SpellKind::Shatter)
                command.aimedCell = Cell{6, 8 - example.range};
            else if (example.spell == rules::SpellKind::CureWounds ||
                     example.spell == rules::SpellKind::Bless) {
                command.targetCount = example.spell == rules::SpellKind::Bless ? 3 : 1;
                command.allies = {{{0}, {1}, {2}}};
                if (example.spell == rules::SpellKind::CureWounds)
                    game.party().damage({0}, 5);
            } else if (example.spell != rules::SpellKind::BurningHands &&
                       example.spell != rules::SpellKind::Thunderwave) {
                const bool multiple = example.spell == rules::SpellKind::MagicMissile ||
                                      example.spell == rules::SpellKind::ScorchingRay ||
                                      example.spell == rules::SpellKind::AcidSplash;
                command.targetCount = multiple ? example.count : 1;
                for (unsigned i = 0; i < command.targetCount; ++i) {
                    command.enemies[i] = game.actors().id(i);
                    command.darts[i] = example.count == 1 ? 3 : 1;
                }
            }
            const auto accepted = game.submit(command);
            commands.record(game.events());
            check(accepted.reason == CommandReason::Resolved,
                  (std::string(example.name) + ": " + commandMessage(accepted.reason)).c_str());
        }
        if (example.spell == rules::SpellKind::CureWounds)
            check(commands.snapshot().portraitFeedback[0][0].kind == PortraitFeedbackKind::Healing,
                  "Healing review consumes resolved events");
        if (example.spell == rules::SpellKind::Bless && !npcBless)
            for (unsigned i = 0; i < 3; ++i)
                check(commands.snapshot().portraitFeedback[i][0].kind == PortraitFeedbackKind::Protection,
                      "Buff review affects each selected portrait");
        if (incoming && !weaponIncoming && !npcBless) {
            bool damagePulse = false;
            for (const auto& channels : commands.snapshot().portraitFeedback)
                damagePulse |= channels[1].kind == PortraitFeedbackKind::Damage;
            check(damagePulse, "Enemy hit creates an actual damage pulse");
        }
        if (std::string_view(example.name) == "web-burn") {
            commands.record(game.update(6, false));
            if (!realTime(game.combat().snapshot().phase))
                spellReady(game, 3);
            auto fire = spellCommand(game, rules::SpellKind::FireBolt);
            fire.targetCount = 1;
            fire.enemies[0] = game.actors().id(0);
            check(game.submit(fire).reason == CommandReason::Resolved,
                  "Web ignition uses ordinary Fire Bolt");
            commands.record(game.events());
            commands.record(game.update(.5, false));
            bool burning = false;
            for (const auto& patch : game.effects().ground().areas)
                burning |= patch.active && patch.burnMs;
            check(burning, "Fire Bolt ignites an actual Web patch");
        }
        if (std::string_view(example.name) == "weapon-move") {
            commands.record(game.update(6, false));
            if (game.combat().snapshot().phase == CombatPhase::Victory)
                check(game.combat().continueExploring(),
                      "Weapon review resumes ordinary exploration after victory");
            if (!realTime(game.combat().snapshot().phase))
                spellReady(game, 1);
            auto move = featureCommand(game, 1, rules::Feature::SpiritualWeapon);
            move.spell = catalog.findSpell("spiritual_weapon");
            const auto* weapon = game.effects().spiritualWeapon({1});
            check(weapon, "Movement review retains summoned weapon");
            move.aimedCell = weapon->cell + right(pose.heading);
            check(game.submit(move).reason == CommandReason::Resolved,
                  "Weapon movement uses ordinary feature command");
            commands.record(game.events());
        }
        const bool weaponIdle = std::string_view(example.name) == "weapon-idle";
        if (weaponIdle)
            commands.record(game.update(1.1, false));
        const auto start = game.timeMs();
        const auto& cue = game.combat().spellVisuals().casts[0];
        report << example.name << " cast=" << cue.id.serial;
        if (cue.id.serial)
            report << " release=" << std::int64_t(cue.releaseMs) - std::int64_t(start)
                   << " impact=" << std::int64_t(cue.impactMs) - std::int64_t(start);
        for (unsigned i = 0; i < cue.count; ++i)
            report << " target=" << cue.targets[i].actor.slot << ":hit=" << cue.targets[i].hit
                   << ":damage=" << cue.targets[i].damage;
        report << '\n';
        if (example.motion)
            std::filesystem::create_directory(output / example.name);
        // Captures are deterministic application-owned targets, never OS screenshots/input.
        const unsigned captureStep =
            se03 && example.spell == rules::SpellKind::SpiritualWeapon ? 40 : frameStep;
        for (unsigned frame = 0; frame * captureStep <= (weaponIdle ? 2400u : 1320u); ++frame) {
            const auto offset = frame * captureStep;
            if (offset)
                commands.record(game.update(double(start + offset - game.timeMs()) / 1000, false));
            present();
            if (frame == 2) {
                allocations = 0;
                watching = true;
                present();
                watching = false;
                check(!allocations, "Warmed production feedback presentation allocates nothing");
            }
            if (example.motion) {
                const auto filename = std::string(frame < 10 ? "0" : "") + std::to_string(frame) + ".png";
                presenter.capture(output / example.name / filename);
            }
            if (offset == 120 || offset == 480) {
                if (example.motion || offset == 120)
                    presenter.capture(output /
                                      (std::string(example.name) + "-" + std::to_string(offset) + ".png"));
            }
        }
        if (example.spell == rules::SpellKind::RayOfFrost && example.motion) {
            commands.record(game.update(double(start + 1700 - game.timeMs()) / 1000, false));
            present();
            presenter.capture(output / "ray-of-frost-slow.png");
        }
        const auto memory = renderer.memory();
        report << "textures=" << memory.textureCount << " mip=" << memory.residentTextures
               << " alpha=" << memory.residentAlpha << " worldGpu=" << memory.worldGpu
               << " applicationGpu=" << memory.applicationGpu << " peak=" << memory.peakApplicationGpu
               << '\n';
        if (example.bright) {
            WorldDefinition destinationDefinition;
            destinationDefinition.assetPack = "underkeep";
            const Map destination(rows, destinationDefinition);
            NpcWorld destinationActors(destination, pose.cell, definitions.types, instances);
            GameSession destinationGame(destination, catalog, party, destinationActors, 5, pose, false,
                                        definitions.timings);
            NpcAnimator destinationAnimator(destinationGame.actors(), definitions.timings);
            destinationAnimator.setTime(0);
            GameView staged(renderer);
            staged.update(destinationGame, destinationAnimator.snapshot(), {});
            const auto overlap = renderer.memory();
            report << "city-dungeon staged textures=" << overlap.textureCount
                   << " mip=" << overlap.residentTextures << " alpha=" << overlap.residentAlpha
                   << " worldGpu=" << overlap.worldGpu << " applicationGpu=" << overlap.applicationGpu
                   << " peak=" << overlap.peakApplicationGpu << '\n';
        }
        using Error = unsigned (*)();
        check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
              "Production spell GL integration valid");
    }
}
} // namespace blobber::test
