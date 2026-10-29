#include "assets/image.hpp"
#include "assets/prepared_assets.hpp"
#include "assets/prepared_loot.hpp"
#include "game/commands.hpp"
#include "game/party.hpp"
#include "input/key_repeat.hpp"
#include "persistence/character_library.hpp"
#include "render/frame_pipeline.hpp"
#include "render/transitions.hpp"
#include "rules/content.hpp"
#include "support/actor_fixtures.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "support/creation_fixtures.hpp"
#include "support/hybrid_fixtures.hpp"
#include "support/inventory_fixtures.hpp"
#include "support/npc_fixtures.hpp"
#include "support/party_fixtures.hpp"
#include "support/recovery_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include "support/spell_fixtures.hpp"
#include "ui/interaction.hpp"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>
#include <type_traits>
#ifdef _MSC_VER
#include <malloc.h>
#endif
namespace {
bool tracking = false;
std::size_t allocations = 0;
void* allocate(std::size_t size) {
    if (tracking)
        ++allocations;
    if (void* p = std::malloc(size ? size : 1))
        return p;
    throw std::bad_alloc();
}
} // namespace
void* operator new(std::size_t size) {
    return allocate(size);
}
void* operator new[](std::size_t size) {
    return allocate(size);
}
void operator delete(void* p) noexcept {
    std::free(p);
}
void operator delete[](void* p) noexcept {
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept {
    std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
    std::free(p);
}
void* operator new(std::size_t size, std::align_val_t alignment) {
    if (tracking)
        ++allocations;
    const auto align = static_cast<std::size_t>(alignment);
#ifdef _MSC_VER
    void* p = _aligned_malloc(size ? size : 1, align);
#else
    void* p = std::aligned_alloc(align, ((size ? size : 1) + align - 1) / align * align);
#endif
    if (!p)
        throw std::bad_alloc();
    return p;
}
void* operator new[](std::size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}
void operator delete(void* p, std::align_val_t) noexcept {
#ifdef _MSC_VER
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void operator delete[](void* p, std::align_val_t a) noexcept {
    ::operator delete(p, a);
}
void operator delete(void* p, std::size_t, std::align_val_t a) noexcept {
    ::operator delete(p, a);
}
void operator delete[](void* p, std::size_t, std::align_val_t a) noexcept {
    ::operator delete(p, a);
}
using namespace blobber;
int main() {
    try {
        static_assert(!std::is_copy_constructible_v<Frame>);
        const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
        const auto appearance = testProneAppearance();
        const auto archerAppearance = testArcherAppearance();
        const std::vector<NpcTimings> roleTimings{npcTimings(appearance), npcTimings(archerAppearance)};
        const auto bundleDirectory =
            std::filesystem::temp_directory_path() /
            ("blobber-alloc-bundles-" +
             std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        struct Cleanup {
            std::filesystem::path path;
            ~Cleanup() {
                std::error_code error;
                std::filesystem::remove_all(path, error);
            }
        } cleanup{bundleDirectory};
        {
            savePreparedScene(bundleDirectory / "scene.bin", prepareScene(testGreaseMaterials()), 31);
            savePreparedNpcs(bundleDirectory / "npcs.bin", PreparedNpcs({appearance}),
                             {npcTimings(appearance)}, 19, 31);
            savePreparedNpcs(bundleDirectory / "roles.bin", PreparedNpcs({appearance, archerAppearance}),
                             roleTimings, 19, 31);
        }
        const SceneRenderer renderer(
            loadPreparedScene(bundleDirectory / "scene.bin", 31),
            loadPreparedNpcs(bundleDirectory / "npcs.bin", {npcTimings(appearance)}, 19, 31));
        const SceneRenderer roleRenderer(
            loadPreparedScene(bundleDirectory / "scene.bin", 31),
            loadPreparedNpcs(bundleDirectory / "roles.bin", roleTimings, 19, 31));
        auto roleActors =
            testActors(map, {3, 5}, {"near", "far"},
                       {{{0}, {2, 3}, {1, 1}, Heading::South}, {{1}, {4, 3}, {0, 1}, Heading::South}});
        NpcAnimator roleAnimator(roleActors, roleTimings);
        FramePipeline roleFrames;
        AttackVisualSnapshot visualShots;
        visualShots.world = roleActors.snapshot().world;
        visualShots.revision = 1;
        auto& visualShot = visualShots.attacks[0];
        visualShot.id = {visualShots.world, 1};
        visualShot.stage = AttackStage::Flight;
        visualShot.ranged = visualShot.incoming = visualShot.hit = true;
        visualShot.enemy = roleActors.id(1);
        visualShot.npcOrigin = roleActors.snapshot().actors[1].npc;
        visualShot.sourcePosition = actorPosition(roleActors.snapshot().actors[1], 0);
        visualShot.impactMs = 1000;
        visualShot.partyOrigin = {{3, 5}, Heading::North};
        MovementState roleView;
        roleView.player = {{3, 5}, Heading::North};
        auto npcs = testActors(map, {3, 3}, {"test"},
                               {{{0}, {3, 1}, {0, 0}, Heading::South},
                                {{0}, {3, 1}, {1, 1}, Heading::West, NpcAnimation::Attack},
                                {{0}, {3, 1}, {0, 1}, Heading::East, NpcAnimation::Walk},
                                {{0}, {3, 1}, {1, 0}, Heading::North}});
        NpcAnimator animator(npcs, {npcTimings(appearance)});
        FramePipeline frames;
        Frame expected, source, destination;
        Controller controller({{3, 3}, Heading::North});
        KeyRepeat repeat;
        bool okay = true;
        rules::MonsterDefinition monster;
        monster.id = "allocation-fixture";
        monster.abilities.fill(10);
        monster.armorClass = 12;
        monster.hitPoints = 8;
        monster.attacks[0] = {0};
        monster.attackCount = 1;
        const rules::Catalog catalog(
            {{"fixture-attack", rules::AttackKind::Melee, 3, {1, 6, 2}, rules::DamageType::Slashing}},
            {monster});
        const auto monsterId = catalog.findMonster("allocation-fixture");
        rules::DiceRng dice(42);
        const auto partyCatalog = partyFixture().catalog();
        Party party(partyCatalog);
        auto invalidBuild = partyCatalog.preset()->characters[0];
        invalidBuild.equipment.body = {16};
        CommandController commands(party, npcs);
        UiInputEdges edges;
        const auto fightCatalog = combatCatalog(100000, -100, {1, 4, -100});
        const auto fightPlacements = combatPlacements(map);
        auto fightStorage = std::unique_ptr<GameSession>(new GameSession(map, fightCatalog, fightPlacements,
                                                                         5, {{3, 4}, Heading::North}, false,
                                                                         {npcTimings(appearance)}));
        auto& fight = *fightStorage;
        CommandController fightUi(fight, {npcTimings(appearance)});
        NpcAnimator fightAnimator(fight.actors(), {npcTimings(appearance)});
        FramePipeline fightFrames;
        auto diagonalFightStorage = std::unique_ptr<GameSession>(
            new GameSession(map, fightCatalog, fightPlacements, 5, {{3, 3}, Heading::North}));
        auto& diagonalFight = *diagonalFightStorage;
        diagonalFight.actors().damage(diagonalFight.actors().id(0), 100000, 0);
        diagonalFight.actors().damage(diagonalFight.actors().id(2), 100000, 0);
        int meleeActions = 0;
        bool monsterFeedback = false, partyFeedback = false, attackPlayback = false;
        auto walkingNpc = NpcInstance{{0}, {1, 1}, {0, 0}, Heading::North};
        walkingNpc.ai.routine = NpcRoutine::Patrol;
        walkingNpc.ai.waypointCount = 2;
        walkingNpc.ai.waypoints[0] = {4, 1};
        walkingNpc.ai.waypoints[1] = {1, 1};
        NpcWorld walkingPlacements(map, {3, 5}, {"goblin"}, {walkingNpc});
        auto walkingStorage = std::unique_ptr<GameSession>(
            new GameSession(map, fightCatalog, walkingPlacements, 5, {{3, 5}, Heading::North}));
        auto& walking = *walkingStorage;
        NpcAnimator walkAnimator(walking.actors(), {npcTimings(appearance)});
        FramePipeline walkFrames;
        std::vector<std::string> bigRows(100, std::string(100, '.'));
        bigRows.front() = bigRows.back() = std::string(100, '#');
        for (auto& row : bigRows)
            row.front() = row.back() = '#';
        Map bigMap(bigRows);
        NpcWorld bigPlacements(bigMap, {1, 1}, {"goblin"}, {{{0}, {2, 2}, {0, 0}, Heading::North}});
        ActorWorld bigActors(bigMap, fightCatalog, bigPlacements);
        Navigation navigation(bigMap, bigActors);
        std::vector<NpcInstance> crowd;
        for (int i = 0; i < 4; ++i)
            crowd.push_back({{0}, {3, 1}, {i % 2, i / 2}, Heading::South});
        NpcWorld crowdPlacements(map, {3, 5}, {"goblin"}, crowd);
        auto crowdedStorage = std::unique_ptr<GameSession>(
            new GameSession(map, fightCatalog, crowdPlacements, 5, {{3, 5}, Heading::South}));
        auto& crowded = *crowdedStorage;
        const auto hybridMap = hybridRoom();
        const auto hybridRules = hybridCatalog();
        NpcWorld hybridPlaces(hybridMap, {5, 8}, {"goblin"},
                              {{{0}, {5, 4}, {0, 1}, Heading::North},
                               {{0}, {7, 4}, {0, 0}, Heading::North},
                               {{0}, {8, 8}, {0, 0}, Heading::North}});
        auto hybridStorage = std::unique_ptr<GameSession>(
            new GameSession(hybridMap, hybridRules, hybridPlaces, 5, {{5, 8}, Heading::North}, false));
        auto& hybrid = *hybridStorage;
        hybrid.setAnimated(false);
        CommandController hybridUi(hybrid);
        auto archer = NpcInstance{{0}, {5, 4}, {0, 0}, Heading::South};
        archer.monster = hybridRules.findMonster("archer");
        NpcWorld archerPlaces(hybridMap, {5, 8}, {"goblin"}, {archer});
        auto archerSessionStorage = std::unique_ptr<GameSession>(
            new GameSession(hybridMap, hybridRules, archerPlaces, 5, {{5, 8}, Heading::North}));
        auto& archerSession = *archerSessionStorage;
        archer.cell = {5, 7};
        NpcWorld closeArcherPlaces(hybridMap, {5, 8}, {"goblin"}, {archer});
        auto closeArcherStorage = std::unique_ptr<GameSession>(
            new GameSession(hybridMap, hybridRules, closeArcherPlaces, 5, {{5, 8}, Heading::North}));
        auto& closeArcher = *closeArcherStorage;
        Party effectParty(hybridRules);
        ActorWorld effectActors(hybridMap, hybridRules, hybridPlaces);
        rules::DiceRng effectRng(5);
        EffectController effects(effectParty, effectActors, effectRng);
        CombatEvents effectEvents;
        NpcAnimator effectAnimator(effectActors, {npcTimings(appearance)});
        auto effectSessionStorage = std::unique_ptr<GameSession>(
            new GameSession(hybridMap, hybridRules, hybridPlaces, 5, {{5, 8}, Heading::North}, false));
        auto& effectSession = *effectSessionStorage;
        const auto spellRules = spellCatalog();
        const auto spellFar = spellPlacements(hybridMap);
        const auto spellNear = spellPlacements(hybridMap, 12, true);
        std::array<std::unique_ptr<GameSession>, static_cast<unsigned>(rules::SpellKind::Count)>
            spellSessions;
        std::array<std::unique_ptr<CommandController>, static_cast<unsigned>(rules::SpellKind::Count)>
            spellUis;
        for (unsigned i = 0; i < spellSessions.size(); ++i) {
            const auto kind = static_cast<rules::SpellKind>(i);
            const bool close = rules::spellRules(kind)->area || (kind == rules::SpellKind::InflictWounds ||
                                                                 kind == rules::SpellKind::ShockingGrasp ||
                                                                 kind == rules::SpellKind::PoisonSpray);
            spellSessions[i] = std::make_unique<GameSession>(
                hybridMap, spellRules, close ? spellNear : spellFar, 5, spellSpawn, false);
            if (kind == rules::SpellKind::ShockingGrasp) {
                spellSessions[i]->party().lockFormation(false);
                spellSessions[i]->party().swapFormation(FormationSlot::FrontRight, FormationSlot::BackRight);
                spellSessions[i]->party().lockFormation(true);
            }
            spellUis[i] = std::make_unique<CommandController>(*spellSessions[i]);
        }
        ProjectilePlayer spellVisuals;
        FramePipeline spellFrames;
        const auto featurePlaces = spellPlacements(hybridMap, 0);
        auto featureSessionStorage = std::unique_ptr<GameSession>(
            new GameSession(hybridMap, spellRules, featurePlaces, 5, spellSpawn, false));
        auto& featureSession = *featureSessionStorage;
        CommandController featureUi(featureSession);
        const auto inventoryRules = inventoryCatalog();
        const NpcWorld inventoryPlaces(hybridMap, {6, 8}, {}, {});
        auto inventoryStorage = std::make_unique<GameSession>(hybridMap, inventoryRules, inventoryPlaces, 5,
                                                              Player{{6, 8}, Heading::North}, false);
        auto& inventorySession = *inventoryStorage;
        auto inventoryUiStorage = std::make_unique<CommandController>(inventorySession);
        auto& inventoryUi = *inventoryUiStorage;
        const auto arrow = findInventoryItem(inventorySession.party(), 2, 9);
        const auto arrowCell = inventorySession.party().inventory().locate(arrow).cell;
        const auto recoveryRules = recoveryCatalog();
        RecoveryLevel recoveryLevel;
        recoveryLevel.sanctuaries[0] = {6, 8};
        recoveryLevel.sanctuaryCount = 1;
        for (std::size_t i = 0; i < recoveryLevel.ground.size(); ++i)
            recoveryLevel.ground[recoveryLevel.groundCount++] = {{6, 8}, {12}, 6};
        auto recoverySession = std::make_unique<GameSession>(hybridMap, recoveryRules, inventoryPlaces, 5,
                                                             Player{{6, 8}, Heading::North}, false,
                                                             std::vector<NpcTimings>{}, recoveryLevel);
        auto recoveryUi = std::make_unique<CommandController>(*recoverySession);
        OwnedImage lootImage(GenImageColor(30, 30, WHITE));
        auto lootBank = prepareLoot(lootImage.value);
        auto lootRenderer = std::make_unique<SceneRenderer>(
            loadPreparedScene(bundleDirectory / "scene.bin", 31), PreparedNpcs{},
            std::vector<NpcAttachments>{}, std::move(lootBank));
        auto lootFrames = std::make_unique<FramePipeline>();
        const auto deathPlaces = combatPlacements(map, Player{{3, 3}, Heading::North}, 4);
        ActorWorld deathActors(map, recoveryRules, deathPlaces);
        LootWorld deathLoot(deathActors, {}, 5);
        const auto creationRules = creationCatalog();
        auto draft = rules::recommendedDraft(creationRules, {3}, {1});
        std::snprintf(draft.name.data(), draft.name.size(),
                      "Long custom character name exercising bounded previews");
        auto customParty = *creationRules.preset();
        customParty.characters[1] = rules::finishBuild(creationRules, draft, "custom.cleric");
        NpcWorld creationEmpty(hybridMap, spellSpawn.cell, {}, {});
        auto creationSession = std::make_unique<GameSession>(hybridMap, creationRules, customParty,
                                                             creationEmpty, 5, spellSpawn, false);
        auto creationUi = std::make_unique<CommandController>(*creationSession);
        LibrarySnapshot librarySnapshot;
        librarySnapshot.count = 128;
        librarySnapshot.records[127].id.text[0] = 'x';
        const auto lookupId = librarySnapshot.records[127].id;
        tracking = true;
        for (unsigned iteration = 0; iteration < 128; ++iteration) {
            okay &= bool(rules::previewBuild(creationRules, draft));
            okay &= librarySnapshot.find(lookupId) == &librarySnapshot.records[127];
            for (std::uint16_t i = 0; i < creationRules.spellCount(); ++i)
                creationSession->party().spellAccess({1}, {i});
            creationUi->refresh({}, false, false, true);
            creationUi->selectCharacter(1);
            creationUi->openSpellbook();
            creationUi->browseSpellbook(1);
            creationUi->closeOverlay();
            creationUi->openInventory(rules::CharacterId{1}, CharacterPage::Character);
            creationUi->closeOverlay();
        }

        for (std::size_t i = 0; i < 4; ++i)
            deathActors.damage(deathActors.id(i), 100, 0);
        deathLoot.reconcile(deathActors);
        okay &= deathLoot.find({deathActors.snapshot().world, 0}) != nullptr;

        for (int i = 0; i < 64; ++i) {
            recoverySession->party().damage({0}, 1);
            recoveryUi->refresh({}, false, false, true);
            recoveryUi->openCamp();
            recoveryUi->campKind(RestKind::Long);
            recoveryUi->startRest();
            recoveryUi->tickCamp(.1, true);
            recoveryUi->closeOverlay();
            const LootId lootId{recoverySession->actors().snapshot().world,
                                static_cast<std::uint16_t>(2048 + i)};
            recoveryUi->floorHover(lootId);
            recoveryUi->openLoot(lootId);
            recoveryUi->lootPage(1);
            lootFrames->update(*lootRenderer, hybridMap, recoverySession->movement(), nullptr, nullptr,
                               nullptr, 0, nullptr, &recoverySession->loot().snapshot());
            recoveryUi->lootHover(0);
            recoveryUi->lootRecipient(i % 4);
            recoveryUi->lootTake(0);
            if (i == 63)
                recoveryUi->lootTakeAll();
            recoveryUi->closeOverlay();
        }

        for (unsigned n = 0; n < 64; ++n) {
            const auto handle = findInventoryItem(recoverySession->party(), 0, 13);
            okay &= bool(
                recoverySession->dropItem(handle, recoverySession->party().inventory().revision(), true));
            okay &= bool(recoverySession->dropItem(handle, recoverySession->party().inventory().revision()));
            recoveryUi->openInventory(rules::CharacterId{0});
            recoveryUi->refresh({}, false, false, true);
            recoveryUi->closeOverlay();
            lootFrames->update(*lootRenderer, hybridMap, recoverySession->movement(), nullptr, nullptr,
                               nullptr, 0, nullptr, &recoverySession->loot().snapshot());
            const auto& stack = recoverySession->loot().snapshot().stacks[lootReservedCapacity];
            okay &= bool(recoverySession->pickup(
                {{recoverySession->actors().snapshot().world, lootReservedCapacity, stack.generation},
                 {0},
                 recoverySession->party().inventory().revision()}));
        }

        for (int iteration = 0; iteration < 100; ++iteration) {
            inventoryUi.refresh({}, false, false, true);
            inventoryUi.openInventory(rules::CharacterId{2});
            inventoryUi.inventoryPage(CharacterPage::Character);
            inventoryUi.refresh({}, false, false, true);
            okay &= inventoryUi.snapshot().inventory.sheet.proficiency == 2;
            inventoryUi.inventoryPage(CharacterPage::Inventory);
            inventoryUi.inventorySelect(arrowCell);
            inventoryUi.inventoryDrop({{2}, 47}, true);
            okay &= inventoryUi.snapshot().inventory.mode == InventoryMode::Quantity;
            inventoryUi.inventoryConfirm();
            inventoryUi.inventorySelect(47);
            inventoryUi.inventoryDrop({{2}, arrowCell});
            inventoryUi.inventorySelect(arrowCell);
            inventoryUi.inventoryAction(InventoryAction::Transfer);
            inventoryUi.inventoryCharacter({0});
            inventoryUi.inventoryCharacter({0});
            const auto transferred = inventorySession.party().inventory().locate(arrow).cell;
            inventoryUi.inventorySelect(transferred);
            inventoryUi.inventoryAction(InventoryAction::Transfer);
            inventoryUi.inventoryCharacter({2});
            const auto shield = inventorySession.party().inventory().at({{0}, 50});
            okay &= bool(
                inventorySession.inventoryChange(inventoryRequest(inventorySession.party(), shield, 0, 47)));
            okay &= bool(
                inventorySession.inventoryChange(inventoryRequest(inventorySession.party(), shield, 0, 50)));
            const auto potion = findInventoryItem(inventorySession.party(), 3, 10);
            okay &=
                inventorySession.inventoryChange(inventoryRequest(inventorySession.party(), potion, 3, 48))
                    .error == InventoryError::Equipment;
            inventoryUi.refresh({}, false, false, false);
            inventoryUi.closeOverlay();
        }
        inventorySession.party().damage({1}, 5);
        inventoryUi.refresh({}, false, false, true);
        inventoryUi.openInventory(rules::CharacterId{0});
        inventoryUi.inventorySelect(inventorySession.party()
                                        .inventory()
                                        .locate(findInventoryItem(inventorySession.party(), 0, 10))
                                        .cell);
        inventoryUi.inventoryAction(InventoryAction::Use);
        okay &= inventoryUi.snapshot().inventory.itemTargeting;
        inventoryUi.inventoryAlly(1, true);
        okay &= inventorySession.party().character({1})->hitPoints > 7;

        for (int repeat = 0; repeat < 20; ++repeat) {
            featureUi.refresh({}, false, false, true);
            for (unsigned member : {0u, 2u}) {
                featureUi.selectCharacter(member);
                featureUi.openSpellbook();
                featureUi.browseSpellbook(1);
                featureUi.chooseSpellbook(1); // Defense / rear-row melee are read-only.
                okay &= featureUi.snapshot().magic.open && !featureUi.snapshot().magic.armed;
                featureUi.hover(HoverKind::Action, 3);
                featureUi.closeOverlay();
            }
        }
        for (unsigned i = 0; i < spellSessions.size(); ++i) {
            auto& s = *spellSessions[i];
            auto& ui = *spellUis[i];
            const auto kind = static_cast<rules::SpellKind>(i);
            const auto& rule = *rules::spellRules(kind);
            const unsigned caster = rule.casting == rules::Casting::Wisdom ? 1 : 3;
            if (rule.area ||
                (kind == rules::SpellKind::InflictWounds || kind == rules::SpellKind::ShockingGrasp ||
                 kind == rules::SpellKind::PoisonSpray))
                spellReady(s, caster);
            auto command = spellCommand(s, kind);
            if (rule.ground)
                command.aimedCell = Cell{6, 7};
            if (!rule.area && !rule.self) {
                command.targetCount = 1;
                command.enemies[0] = s.actors().id((kind == rules::SpellKind::InflictWounds ||
                                                    kind == rules::SpellKind::ShockingGrasp ||
                                                    kind == rules::SpellKind::PoisonSpray)
                                                       ? 2
                                                       : 0);
                command.darts[0] = 3;
                command.allies[0] = {3};
            }
            if (kind == rules::SpellKind::CureWounds || kind == rules::SpellKind::SpareTheDying)
                s.party().damage({3}, s.party().character({3})->hitPoints);
            ui.refresh(allCombatTargets(s), false, false, true);
            ui.selectCharacter(kind == rules::SpellKind::ShockingGrasp ? 1 : caster);
            ui.openSpellbook();
            ui.cycleSpellbookTab(1);
            ui.cycleSpellbookTab(1);
            ui.cycleSpellbookTab(-1);
            std::size_t entry = 0;
            while (entry < ui.snapshot().magic.count &&
                   ui.snapshot().magic.entries[entry].spell.index != command.spell.index)
                ++entry;
            ui.chooseSpellbook(entry);
            if (rule.ground)
                ui.magicGround(command.aimedCell, true);
            if (rule.area) {
                const auto area = ui.floorPreview();
                spellFrames.update(renderer, hybridMap, s.movement(), &s.actors(), nullptr, nullptr,
                                   s.timeMs(), &area);
                spellFrames.update(renderer, hybridMap, s.movement(), &s.actors(), nullptr, nullptr,
                                   s.timeMs(), &area);
                spellFrames.update(renderer, hybridMap, s.movement(), &s.actors());
            } else if (rule.hostile) {
                ui.magicEnemy(command.enemies[0]);
                if (kind == rules::SpellKind::MagicMissile) {
                    ui.magicEnemy(command.enemies[0]);
                    ui.magicEnemy(command.enemies[0], true);
                    ui.magicEnemy(command.enemies[0]);
                }
            } else
                ui.magicAlly(3);
            ui.refresh(allCombatTargets(s), false, false, false);
            ui.refresh(allCombatTargets(s), false, false, true);
            ui.backToSpellbook();
            ui.closeOverlay();
            okay &= s.submit(command).reason == CommandReason::Resolved;
            ui.record(s.events());
            if (kind == rules::SpellKind::Grease) {
                const auto ground = ui.floorPreview();
                spellFrames.update(renderer, hybridMap, s.movement(), &s.actors(), nullptr, nullptr,
                                   s.timeMs(), &ground);
                spellFrames.update(renderer, hybridMap, s.movement(), &s.actors(), nullptr, nullptr,
                                   s.timeMs(), &ground);
                auto travel = s.movement();
                travel.transition = {spellSpawn,
                                     {spellSpawn.cell + right(spellSpawn.heading), spellSpawn.heading},
                                     Action::Right,
                                     .08f,
                                     .32f};
                ++travel.revision;
                spellFrames.update(renderer, hybridMap, travel, &s.actors(), nullptr, nullptr, s.timeMs(),
                                   &ground);
            }
            spellVisuals.update(&s.combat().attacks(), s.timeMs());
            ui.record(s.update(.5, false));
            spellVisuals.update(&s.combat().attacks(), s.timeMs());
            ui.refresh(allCombatTargets(s), false, false, true);
            ui.openSpellbook();
            for (unsigned c = 0; c < 4; ++c) {
                UiInput hover;
                const auto r = hudLayout::status(c, 0);
                hover.pointer = ArtPoint{float(r.x + 1), float(r.y + 1)};
                routeHudInput(ui, hover);
                const auto emblem = hudLayout::concentration(c);
                hover.pointer = ArtPoint{float(emblem.x + 1), float(emblem.y + 1)};
                routeHudInput(ui, hover);
            }
            ui.closeOverlay();
        }
        EffectRequest effectRequest;
        effectRequest.world = effectActors.snapshot().world;
        effectRequest.caster = {0};
        effectRequest.targets[0] = rules::CharacterId{0};
        effectRequest.count = 1;
        okay &= bool(effects.apply(effectRequest, 0, effectEvents));
        effects.damaged({0}, 1, 100, effectEvents);
        effectRequest.kind = EffectKind::Bane;
        effectRequest.caster = {1};
        effectRequest.targets[0] = effectActors.id(0);
        effectRequest.difficulty = 1000;
        okay &= bool(effects.apply(effectRequest, 100, effectEvents));
        effects.save(effectActors.id(0), rules::Ability::Wisdom, 10);
        effectRequest.kind = EffectKind::HoldPerson;
        okay &= bool(effects.apply(effectRequest, 100, effectEvents));
        effects.advance(60000, 60100, true, effectEvents);
        effectEvents.clear();
        effectRequest.kind = EffectKind::MageArmor;
        effectRequest.caster = {3};
        effectRequest.targets[0] = rules::CharacterId{3};
        for (std::size_t i = 0; i < EffectController::capacity; ++i)
            okay &= bool(effects.apply(effectRequest, 60100, effectEvents));
        okay &= effects.apply(effectRequest, 60100, effectEvents).error == EffectError::Capacity;
        okay &= effects.armorClass(rules::CharacterId{3}) == effectParty.character({3})->stats.armorClass + 3;
        for (const auto kind :
             {EffectKind::Sleep, EffectKind::Blind, EffectKind::Slow, EffectKind::Knockdown})
            effects.condition(effectActors.id(0), kind, {3}, 60100, effectEvents);
        effectAnimator.setTime(60100, &effects);
        okay &= effectAnimator.snapshot().frames[0].clip == NpcAnimation::Prone;
        const auto statusSummary = effects.statuses(effectActors.id(0));
        char duration[96];
        for (std::size_t i = 0; i < statusSummary.count; ++i)
            statusDuration(statusSummary.entries[i], duration, sizeof(duration));
        effects.wake(effectActors.id(0), 60100, effectEvents);
        effects.clockMode(true);
        const auto turnStatuses = effects.statuses(effectActors.id(0));
        for (std::size_t i = 0; i < turnStatuses.count; ++i)
            statusDuration(turnStatuses.entries[i], duration, sizeof(duration));
        effects.casterTurn({3}, 60100, effectEvents);
        effects.clockMode(false);
        const auto groundCell = effectActors.actor(effectActors.id(0))->npc.cell;
        okay &= effects.createArea(groundCell, {3}, 1000, 60100, effectEvents).serial != 0;
        effects.groundCheck(effectActors.id(0), groundCell, 60100, effectEvents);
        effects.advance(120000, 120100, true, effectEvents);
        effectParty.damage({3}, 999);
        effects.reconcile(60100, effectEvents);
        effectRequest.world = effectSession.actors().snapshot().world;
        effectRequest.kind = EffectKind::HoldPerson;
        effectRequest.targets[0] = effectSession.actors().id(0);
        okay &= bool(effectSession.applyEffect(effectRequest));
        effectSession.update(60, true);
        effectSession.update(60, false);
        okay &= !effectSession.effects().has(effectSession.actors().id(0), EffectKind::HoldPerson);

        roleActors.attack(roleActors.id(0), 0);
        roleActors.attack(roleActors.id(1), 0, true);
        for (const auto time : {0u, 599u, 600u, 999u, 1000u, 1400u, 1550u, 2400u, 3000u}) {
            if (time == 1000)
                okay &= roleActors.walk(roleActors.id(1), {3, 3}, {0, 1}, roleView.player.cell, 1000).changed;
            if (time == 1400)
                okay &= roleActors.damage(roleActors.id(1), 1, time).changed;
            if (time == 2400)
                okay &= roleActors.damage(roleActors.id(1), 100, 1800).died;
            roleAnimator.setTime(time);
            roleFrames.update(roleRenderer, map, roleView, &roleActors, &roleAnimator.snapshot(),
                              &visualShots, time);
            const auto& pose = roleAnimator.snapshot().frames[1];
            okay &= roleFrames.picking() && roleFrames.picking()->targets().count == (time < 2400 ? 2 : 1);
            if (time == 600)
                okay &= pose.clip == NpcAnimation::RangedAttack && pose.active == 1;
            if (time == 1400)
                okay &= pose.clip == NpcAnimation::Hurt && (pose.offsetX != 0 || pose.offsetY != 0);
            if (time == 2400)
                okay &= pose.clip == NpcAnimation::Death && pose.visible;
            if (time == 3000)
                okay &= !pose.visible;
        }
        int archerShots = 0;
        bool archerRetreated = false;
        for (int i = 0; i < 140; ++i) {
            archerShots += archerSession.update(.1, false).step.event == CombatEvent::AttackStarted;
            if (closeArcher.combat().snapshot().awaitingAction)
                closeArcher.submit(turnCommand(closeArcher));
            else
                archerRetreated |= closeArcher.update(.1, false).step.event == CombatEvent::Move;
        }
        okay &= archerShots >= 2 && archerRetreated &&
                archerSession.actors().actor(archerSession.actors().id(0))->npc.cell == Cell{5, 4};
        auto bow = turnCommand(hybrid, PartyAction::Ranged);
        bow.character = {2};
        bow.target = hybrid.actors().id(0);
        hybridUi.record(hybrid.submit(bow));
        for (int i = 0; i < 3; ++i)
            hybrid.move(Action::Forward);
        bool hybridRound = false;
        for (int i = 0; i < 100; ++i) {
            if (i == 6)
                for (const Cell step : {Cell{7, 8}, Cell{6, 8}, Cell{6, 7}, Cell{6, 6}, Cell{6, 5}})
                    okay &= hybrid.actors()
                                .move(hybrid.actors().id(2), step, {0, 0}, hybrid.movement().player.cell)
                                .changed;
            const auto st = hybrid.combat().snapshot();
            hybridRound |= st.round > 1;
            if (st.awaitingAction)
                hybridUi.record(hybrid.submit(turnCommand(hybrid)));
            else
                hybridUi.record(hybrid.update(.5, false));
            hybridUi.refresh(allCombatTargets(hybrid), false, false, true);
        }
        okay &= hybridRound && hybrid.combat().snapshot().enemyCount == 3;
        hybrid.setAnimated(true);
        hybrid.actors().damage(hybrid.actors().id(0), 1000, hybrid.timeMs());
        for (int i = 0; i < 80; ++i) {
            hybridUi.record(hybrid.update(.04, false));
            hybridUi.refresh(allCombatTargets(hybrid), hybrid.movement().transition.active(), false, true);
        }
        okay &= hybrid.movement().player.heading == Heading::East;

        for (int i = 0; i < 25; ++i)
            crowded.update(.1, false);
        okay &= crowded.actors().occupiedSlots({3, 4}) == 15;
        navigation.request(0, {2, 2}, {98, 98}, true, Cell{98, 97});
        for (int i = 0; i < 20; ++i) {
            navigation.update({1, 1});
            okay &= navigation.lastWork() <= Navigation::nodeBudget;
        }
        okay &= navigation.take(0).found;
        bool presentedTravel = false;
        for (int i = 0; i < 150; ++i) {
            walking.update(.016, i % 13 == 0);
            walkAnimator.setTime(walking.timeMs());
            walkFrames.update(renderer, map, walking.movement(), &walking.actors(), &walkAnimator.snapshot());
            const auto& pose = walkAnimator.snapshot().frames[0];
            presentedTravel |= (pose.offsetX != 0 || pose.offsetY != 0) && walkFrames.picking() &&
                               walkFrames.picking()->targets().count > 0;
        }
        okay &= walking.actors().snapshot().actors[0].hasWalk && presentedTravel;

        fight.setAnimated(false);
        fight.move(Action::Forward);
        fightUi.record(fight.update(0, false));
        for (int i = 0; i < 1000; ++i) {
            fightUi.record(fight.update(.45, i % 19 == 0));
            fightAnimator.setTime(fight.timeMs());
            fightFrames.update(renderer, map, fight.movement(), &fight.actors(), &fightAnimator.snapshot(),
                               &fight.combat().attacks(), fight.timeMs());
            const auto* pick = fightFrames.picking();
            fightUi.refresh(pick ? pick->targets() : TargetSnapshot{}, false, false, true);
            if (fight.combat().snapshot().awaitingAction) {
                const auto id = *fight.combat().snapshot().activeCharacter;
                const rules::CharacterId ally{static_cast<std::uint16_t>((id.index + 1) % 4)};
                if (i % 3 == 0) {
                    fight.party().damage(ally, fight.party().character(ally)->hitPoints);
                    fightUi.refresh(pick ? pick->targets() : TargetSnapshot{}, false, false, true);
                    fightUi.selectAction(PartyAction::Aid);
                    fightUi.selectAlly(0);
                } else if (i % 3 == 2) {
                    fightUi.selectAction(PartyAction::Melee);
                    fightUi.selectTarget(fight.actors().id(2));
                } else
                    fightUi.selectAction(PartyAction::Defend);
                const auto result = fightUi.confirm();
                if (i % 3 == 2 && result == CommandReason::Resolved)
                    ++meleeActions;
                fight.party().heal(ally, 100);
            }
            for (const auto& feedback : fightUi.snapshot().feedback) {
                monsterFeedback |= feedback.active && !feedback.party;
                partyFeedback |= feedback.active && feedback.party;
            }
            for (std::size_t slot = 0; slot < fightAnimator.snapshot().count; ++slot)
                attackPlayback |= fightAnimator.snapshot().frames[slot].clip == NpcAnimation::Attack;
            fightUi.hover(HoverKind::Enemy, 0, fight.actors().id(2), 100, 100);
            fightUi.hover(HoverKind::Action, 1, {}, 60, 320);
            fightUi.inspect(2);
            fightUi.closeOverlay();
            fightUi.toggleMenu();
            fightUi.closeOverlay();
            char logLine[CombatLog::messageBytes];
            fightUi.snapshot().log.lineCount(hudLayout::logColumns);
            fightUi.snapshot().log.line(0, hudLayout::logColumns, logLine, sizeof(logLine));
            fightUi.toggleLog();
            fightUi.scrollLog(1);
            fightUi.confirm();
            fightUi.toggleLog();
        }
        for (std::size_t i = 0; i < 4; ++i)
            fight.actors().damage(fight.actors().id(i), 100000, fight.timeMs());
        fightUi.record(fight.update(.45, false));
        okay &= meleeActions > 0 && monsterFeedback && partyFeedback && attackPlayback;
        okay &= fight.combat().snapshot().phase == CombatPhase::Victory;
        okay &= fight.combat().continueExploring();

        diagonalFight.update(0, false);
        bool diagonalAttack = false;
        for (int i = 0; i < 40; ++i) {
            if (diagonalFight.combat().snapshot().awaitingAction)
                diagonalFight.submit(turnCommand(diagonalFight));
            else {
                const auto out = diagonalFight.update(.45, false).step;
                diagonalAttack |= out.enemyAttack && out.enemy.slot == 1;
            }
        }
        okay &= diagonalAttack &&
                diagonalFight.actors().actor(diagonalFight.actors().id(1))->npc.subcell == Cell{0, 1};

        for (int i = 0; i < 1000; ++i) {
            const auto& snapshot = party.snapshot();
            const auto* member = party.character({static_cast<std::uint16_t>(i % rules::partySize)});
            const auto derived = rules::deriveCharacter(
                partyCatalog, member->build,
                party.inventory().loadout({static_cast<std::uint16_t>(i % rules::partySize)}));
            okay &= bool(derived) && derived.value.maxHitPoints == member->hitPoints;
            okay &= party.equipped({0}, EquipmentSlot::MainHand) != nullptr;
            okay &= snapshot.formation[0].index < rules::partySize;
            okay &= !rules::deriveCharacter(partyCatalog, invalidBuild);
            okay &= party.swapFormation(FormationSlot::FrontLeft, FormationSlot::BackRight);
            okay &= !party.swapFormation(FormationSlot::FrontLeft, static_cast<FormationSlot>(255));
            okay &= party.occupant(FormationSlot::FrontLeft) != nullptr;
        }
        for (int i = 0; i < 1000; ++i) {
            const auto* actor = catalog.monster(monsterId);
            const auto* attack = catalog.attack(actor->attacks[0]);
            const auto roll = dice.d20(rules::rollMode(i % 2 == 0, i % 3 == 0));
            const auto hit = rules::resolveAttack(roll.value, attack->bonus, actor->armorClass);
            const auto save = rules::resolveSave(roll.value, 2, 12);
            const auto damage =
                dice.damage(attack->damage, hit.value.outcome == rules::AttackOutcome::Critical);
            const auto bulk = dice.roll({32, 100, -3});
            okay &= bool(roll) && bool(hit) && bool(save) && bool(damage) && bool(bulk);
            okay &= damage.value.total >= 0 &&
                    rules::abilityModifier(actor->abilities, rules::Ability::Strength).value == 0;
            okay &= rules::proficiencyBonus(3).value == 2;
        }
        okay &= frames.update(renderer, map, controller.state(), &npcs);
        okay &= !frames.update(renderer, map, controller.state(), &npcs);
        for (int action = 0; action < 6; ++action) {
            controller = Controller({{3, 3}, Heading::North});
            // Revision belongs to one controller/level lifetime, so use a new pipeline
            // only at startup, and synthetic monotonically increasing states here.
            controller.submit(map, static_cast<Action>(action), true, &npcs);
            for (int step = 0; step < 12; ++step) {
                auto state = controller.state();
                state.revision = action + 1;
                frames.update(renderer, map, state, &npcs);
                renderer.compose(map, state.player, destination, &npcs);
                if (state.transition.active()) {
                    const auto& t = state.transition;
                    if (isStrafe(t.action))
                        renderer.composeStrafe(map, t.from, t.to, t.progress(), expected, &npcs);
                    else {
                        renderer.compose(map, t.from, source, &npcs);
                        composeTransition(source, destination, t.action, t.progress(), expected);
                    }
                } else
                    expected.copyFrom(destination);
                okay &= same(frames.frame(), expected);
                controller.update(map, 0.02f, &npcs);
            }
        }
        std::array<bool, 6> down{}, pressed{};
        down[0] = pressed[0] = true;
        repeat.sample(down, pressed, 0.016f);
        pressed.fill(false);
        for (int i = 0; i < 120; ++i)
            repeat.sample(down, pressed, 0.016f);
        repeat.clear();
        controller.clearPending();
        controller = Controller({{1, 1}, Heading::North});
        controller.submit(map, Action::Forward);
        okay &= controller.state().blocked;
        controller.submit(map, Action::Right);
        controller.submit(map, Action::TurnRight);
        controller.update(map, 1, &npcs);
        controller.setAnimated(false);
        auto state = controller.state();
        state.revision = 100;
        frames.update(renderer, map, state, &npcs);
        renderer.compose(map, state.player, expected, &npcs);
        okay &= same(frames.frame(), expected);
        okay &= !frames.update(renderer, map, state, &npcs);
        controller = Controller({{3, 2}, Heading::North});
        controller.submit(map, Action::Forward, true, &npcs);
        okay &= controller.state().blocked;
        controller.submit(map, Action::Back, true, &npcs);
        controller.submit(map, Action::Forward, true, &npcs);
        controller.update(map, 1, &npcs);
        controller.update(map, 1, &npcs);
        controller.submit(map, Action::Forward, true, &npcs);
        okay &= controller.state().blocked;
        state.player = {{3, 3}, Heading::North};
        state.transition.elapsed = Transition::duration;
        state.revision = 101;
        for (std::uint64_t ms = 0; ms < 5000; ms += 17) {
            animator.setTime(ms);
            frames.update(renderer, map, state, &npcs, &animator.snapshot());
            renderer.compose(map, state.player, expected, &npcs, &animator.snapshot());
            okay &= same(frames.frame(), expected);
            okay &= !frames.update(renderer, map, state, &npcs, &animator.snapshot());
        }
        for (int action = 0; action < 6; ++action) {
            const auto kind = static_cast<Action>(action);
            const Player from{{3, 3}, Heading::North};
            Player to = from;
            okay &= tryAction(map, to, kind, &npcs);
            state.player = to;
            ++state.revision;
            for (std::uint64_t ms : {70u, 95u, 120u, 155u}) {
                animator.setTime(ms);
                state.transition = {from, to, kind, Transition::duration * 0.5f};
                frames.update(renderer, map, state, &npcs, &animator.snapshot());
                if (isStrafe(kind))
                    renderer.composeStrafe(map, from, to, 0.5f, expected, &npcs, &animator.snapshot());
                else {
                    renderer.compose(map, from, source, &npcs, &animator.snapshot());
                    renderer.compose(map, to, destination, &npcs, &animator.snapshot());
                    composeTransition(source, destination, kind, 0.5f, expected);
                }
                okay &= same(frames.frame(), expected);
            }
        }
        // Exercise authoritative mutations and transient presentation inside the guard.
        state.player = {{3, 5}, Heading::North};
        state.transition.elapsed = Transition::duration;
        ++state.revision;
        const auto actor = npcs.id(0);
        for (int i = 0; i < 100; ++i) {
            okay &= npcs.move(actor, {2, 1}, {0, 0}, state.player.cell).changed;
            okay &= npcs.face(actor, Heading::West).changed;
            okay &= npcs.move(actor, {3, 1}, {0, 0}, state.player.cell).changed;
            okay &= npcs.face(actor, Heading::South).changed;
            okay &= npcs.move(actor, {3, 1}, {1, 1}, state.player.cell).error == ActorError::Occupied;
            okay &= npcs.damage({}, 1, 0).error == ActorError::InvalidHandle;
            animator.setTime(1000);
            frames.update(renderer, map, state, &npcs, &animator.snapshot());
        }
        for (std::size_t i = 0; i < npcs.snapshot().count; ++i) {
            const auto id = npcs.id(i);
            okay &= npcs.damage(id, 1, 1100).damage == 1;
            animator.setTime(1100);
            frames.update(renderer, map, state, &npcs, &animator.snapshot());
            okay &= animator.snapshot().frames[i].clip == NpcAnimation::Hurt;
            animator.setTime(1250);
            frames.update(renderer, map, state, &npcs, &animator.snapshot());
            okay &= animator.snapshot().frames[i].active == 1;
            okay &= npcs.attack(id, 1260).changed;
            animator.setTime(1260);
            frames.update(renderer, map, state, &npcs, &animator.snapshot());
            okay &= animator.snapshot().frames[i].clip == NpcAnimation::Attack;
            okay &= npcs.damage(id, 999, 1300).died;
            for (const auto ms : {1300u, 1399u, 1400u, 1500u, 1700u, 1900u, 2499u, 2500u}) {
                animator.setTime(ms);
                frames.update(renderer, map, state, &npcs, &animator.snapshot());
                const auto& selected = animator.snapshot().frames[i];
                okay &= selected.visible == (ms < 2500);
                okay &= !selected.visible || selected.clip == NpcAnimation::Death;
                if (ms == 1900)
                    okay &= selected.active == 3;
            }
            okay &= npcs.remove(id).changed;
            okay &= npcs.remove(id).error == ActorError::InvalidHandle;
            animator.setTime(1300);
            frames.update(renderer, map, state, &npcs, &animator.snapshot());
        }
        okay &= !npcs.occupied({3, 1});
        // First and repeated HUD operations, including ring overwrite and picking refresh.
        for (int i = 0; i < 100; ++i) {
            animator.setTime(20000 + i * 60);
            frames.update(renderer, map, state, &npcs, &animator.snapshot());
            const auto* picking = frames.picking();
            commands.refresh(picking ? picking->targets() : TargetSnapshot{}, state.transition.active(),
                             false, true);
            commands.selectCharacter(i % 4);
            commands.selectAction(PartyAction::Defend);
            if (!state.transition.active())
                okay &= commands.confirm() == CommandReason::PreviewAccepted;
            commands.selectAction(PartyAction::Melee);
            commands.cycleTarget(1);
            commands.confirm();
            commands.scrollLog(1);
            commands.pageTargets(1);
            const auto point = artPoint(100, 100, 1280, 720, 2560, 1440);
            std::array<bool, uiKeyCount> down{};
            down[static_cast<std::size_t>(UiKey::Confirm)] = i % 2 == 0;
            const auto input = edges.sample(down, i % 7 != 0, point, 1);
            routeHudInput(commands, input, picking ? picking->pick(200, 200) : std::nullopt);
            commands.cancel();
        }
        tracking = false;
        check(okay, "Frame pipeline endpoints, idle reuse, and blocked movement");
        check(allocations == 0, "Engine-owned CPU frame work must not allocate");
        std::cout << "Frame processing: zero C++ allocations\n";
    } catch (const std::exception& e) {
        tracking = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
