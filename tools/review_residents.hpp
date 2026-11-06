#pragma once
#include "assets/dialogue.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "game/campaign.hpp"
#include "game/sanctuary_hub.hpp"
#include "persistence/campaign_codec.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "render/dialogue_camera.hpp"
#include "ui/interaction.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>

// Application-owned hidden review; no OS input or focus manipulation.
inline void reviewResidents(const std::filesystem::path& root, const std::filesystem::path& output,
                            bool idleOnly = false) {
    using namespace blobber;
    const auto catalog = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    CampaignRun run(catalog, *catalog.preset(), definitions.types, definitions.timings, 5);
    Window window(1920, 1080, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Capture window must stay hidden");
    GpuResources resources;
    Presenter presenter(resources, root);
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    std::ofstream report(output / "residents.txt");
    auto view = std::make_unique<GameView>(renderer);
    auto& game = run.game();
    NpcAnimator animation(game.actors(), definitions.timings);
    auto dialogue = loadDialogue(root / "dialogue");
    prepareDialogueLayout(
        dialogue,
        [](const char* t, void* p) { return static_cast<const UiFonts*>(p)->width(t, UiTextStyle::Label); },
        const_cast<UiFonts*>(&presenter.fonts()));
    CommandController commands(game, definitions.timings);
    commands.bindDialogue(&dialogue, &run.dialogue());
    auto shot = [&](const std::string& name, std::optional<world_render::Camera> camera = {}) {
        animation.setTime(game.timeMs(), &game.effects(), commands.dialogueSpeaker(),
                          commands.dialoguePlayer().clockMs());
        view->update(game, animation.snapshot(), {}, camera);
        view->activate();
        commands.residentVisibility(view->picking()->residents());
        commands.refresh(view->picking()->targets(), false, false, true);
        if (commands.snapshot().targets.count)
            throw std::runtime_error("City residents entered offensive picking");
        view->present(presenter, game, run.map(), false, true, &commands.snapshot());
        presenter.capture(output / (name + ".png"));
        const auto memory = renderer.memory();
        report << name << " textures=" << memory.residentTextures << " alpha=" << memory.residentAlpha
               << " gpu=" << memory.worldGpu << " cpu=" << renderer.cpuBytes()
               << " texture_count=" << memory.textureCount << '\n';
    };
    game.restorePose({{11, 8}, Heading::West}, false);
    shot("formation");
    game.restorePose({{10, 8}, Heading::West}, false);
    shot("mayor-before");
    if (!commands.startDialogue(game.actors().id(0)))
        throw std::runtime_error("Mayor dialogue admission failed");
    const auto original = world_render::camera(game.movement(), game.geometry());
    const auto target = world_render::dialogueCamera(*renderer.scene(), renderer.assets(), game.geometry(),
                                                     original, *game.actors().actor(game.actors().id(0)),
                                                     *commands.dialoguePlayer().character());
    commands.tickDialogue(.7, true);
    shot("mayor-dialogue", target);
    commands.tickDialogue(.25, true);
    shot("mayor-talk", target);
    UiInput reveal;
    reveal.pressed[unsigned(UiKey::DialogueAdvance)] = true;
    routeHudInput(commands, reveal);
    shot("mayor-page", target);
    if (idleOnly) {
        const auto time = game.timeMs();
        bool seen[2][2]{};
        bool captured[2]{};
        for (unsigned tick = 0; tick < 100; ++tick) {
            commands.tickDialogue(.02, true);
            game.update(.02, false);
            animation.setTime(game.timeMs(), &game.effects(), commands.dialogueSpeaker(),
                              commands.dialoguePlayer().clockMs());
            view->update(game, animation.snapshot(), {}, target);
            for (unsigned i = 1; i < 3; ++i) {
                const auto& frame = animation.snapshot().frames[i];
                if (frame.clip != NpcAnimation::Idle || frame.active > 1 || game.timeMs() != time)
                    throw std::runtime_error("Dialogue idle changed simulation or guard clip");
                seen[i - 1][frame.active] = true;
                if (frame.active == 1 && !captured[i - 1]) {
                    shot(i == 1 ? "boric-dialogue-idle" : "balin-dialogue-idle", target);
                    captured[i - 1] = true;
                }
            }
        }
        for (const auto& frames : seen)
            if (!frames[0] || !frames[1])
                throw std::runtime_error("Guard idle did not advance during dialogue");
        report << "Both guard idle poses rendered during dialogue; simulation stayed frozen\n";
        commands.finishDialogue();
        view.reset();
        if (renderer.memory().textureCount || renderer.memory().residentAlpha)
            throw std::runtime_error("Dialogue idle review leaked world leases");
        return;
    }
    commands.finishDialogue();
    shot("mayor-restored");
    if (!commands.startDialogue(game.actors().id(0)))
        throw std::runtime_error("Repeat mayor dialogue admission failed");
    commands.tickDialogue(.7, true);
    if (commands.snapshot().dialogue.introduction)
        throw std::runtime_error("Mayor introduction repeated");
    commands.finishDialogue();
    auto checkpoint = run.checkpoint();
    const CampaignContent content{"residents-review", definitions.definitionFingerprint};
    const auto encoded = encodeCampaign(catalog, definitions, content, *checkpoint);
    const auto decoded = decodeCampaign(catalog, definitions, content, encoded);
    const auto* history = decoded->dialogue.get(dialogueKey("sanctuary.mayor"));
    if (!history || !history->introduced)
        throw std::runtime_error("Mayor introduction missing after save round trip");
    report << "Production save round trip retains mayor introduction and canonical residents\n";
    for (unsigned i = 1; i < 3; ++i) {
        const auto& npc = game.actors().snapshot().actors[i].npc;
        game.restorePose({{10, npc.cell.y}, Heading::West}, false);
        shot("guard-" + std::to_string(i));
        if (commands.startDialogue(game.actors().id(i)))
            throw std::runtime_error("Silent guard started dialogue");
    }
    view.reset();
    // Standalone stationary presentation previews use the production city and assets.
    // Each session dies after its view, and never enters campaign persistence.
    const auto city = sanctuaryCity();
    for (unsigned who = 0; who < 3; ++who) {
        for (const auto clip : {NpcAnimation::Walk, NpcAnimation::Talk, NpcAnimation::Laugh}) {
            if (!who && clip == NpcAnimation::Laugh)
                continue;
            auto placements = sanctuaryResidents(definitions.types);
            const auto type = placements[who].type;
            auto npc = placements[who];
            npc.cell = {9, 8};
            npc.subcell = {0, 1};
            npc.animation = clip;
            const Player front{{10, 8}, Heading::West};
            NpcWorld world(city.map, front.cell, definitions.types, {npc});
            GameSession preview(city.map, catalog, world, 5, front, false, definitions.timings);
            NpcAnimator previewAnimation(preview.actors(), definitions.timings);
            GameView previewView(renderer);
            const Player poses[]{
                front, {{8, 8}, Heading::East}, {{9, 7}, Heading::South}, {{9, 10}, Heading::North}};
            for (unsigned side = 0; side < (clip == NpcAnimation::Walk ? 4u : 1u); ++side) {
                preview.restorePose(poses[side], false);
                std::uint64_t time = 0;
                for (unsigned frame = 0; frame < 4; ++frame) {
                    previewAnimation.setTime(time);
                    previewView.update(preview, previewAnimation.snapshot(), {});
                    previewView.activate();
                    previewView.present(presenter, preview, city.map, false, true);
                    const auto name = definitions.types[type.index] + "-" +
                                      (clip == NpcAnimation::Walk   ? "walk"
                                       : clip == NpcAnimation::Talk ? "talk"
                                                                    : "laugh") +
                                      "-" + std::to_string(side) + "-" + std::to_string(frame);
                    presenter.capture(output / (name + ".png"));
                    if (previewAnimation.snapshot().frames[0].active != frame)
                        throw std::runtime_error("Preview frame sampling mismatch");
                    time += definitions.timings[type.index][unsigned(clip)].durationMs[frame];
                }
            }
        }
    }
    const auto memory = renderer.memory();
    report << "released textures=" << memory.textureCount << " alpha=" << memory.residentAlpha << '\n';
    if (memory.textureCount || memory.residentAlpha)
        throw std::runtime_error("Resident review leaked world leases");
}
