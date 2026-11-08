#include "assets/dialogue.hpp"
#include "assets/npc_metadata.hpp"
#include "assets/rules_loader.hpp"
#include "game/campaign.hpp"
#include "persistence/campaign_codec.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "render/dialogue_camera.hpp"
#include "ui/dialogue_preparation.hpp"
#include "ui/interaction.hpp"
#include "world/sanctuary_city.hpp"
#include <fstream>
#include <iostream>
using namespace blobber;
int main(int argc, char** argv) try {
    if (argc != 3 && argc != 4)
        throw std::runtime_error("Usage: blobber_review_inn ASSETS NEW_OUTPUT [details|innkeeper|dialogue]");
    const std::filesystem::path root(argv[1]), output(argv[2]);
    if (!std::filesystem::create_directory(output))
        throw std::runtime_error("Use a new output directory");
    const auto catalog = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    auto run =
        std::make_unique<CampaignRun>(catalog, *catalog.preset(), definitions.types, definitions.timings, 5);
    Window window(1920, 1080, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Capture window must stay hidden");
    GpuResources resources;
    Presenter presenter(resources, root);
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    auto view = std::make_unique<GameView>(renderer);
    std::ofstream report(output / "report.txt");
    auto memory = [&](const char* name) {
        const auto m = renderer.memory();
        report << name << " textures=" << m.residentTextures << " alpha=" << m.residentAlpha
               << " world_gpu=" << m.worldGpu << " world_cpu=" << renderer.cpuBytes()
               << " app_peak=" << m.peakApplicationGpu << '\n';
    };
    auto update = [&] {
        auto& game = run->game();
        game.discover();
        NpcAnimator animation(game.actors(), definitions.timings);
        animation.setTime(game.timeMs(), &game.effects());
        view->update(game, animation.snapshot(), {});
        view->activate();
    };
    auto capture = [&](const char* name, bool map = false, bool rest = false) {
        update();
        auto& game = run->game();
        CommandController commands(game, definitions.timings);
        commands.refresh(view->picking() ? view->picking()->targets() : TargetSnapshot{}, false, map, true);
        if (rest) {
            commands.openCamp();
            commands.campKind(RestKind::Long);
        }
        view->present(presenter, game, run->map(), map, true, &commands.snapshot());
        presenter.capture(output / (std::string(name) + ".png"));
        const auto p = game.movement().player;
        report << name << " location=" << run->name() << " cell=" << p.cell.x << ',' << p.cell.y
               << " heading=" << headingName(p.heading) << " clock=" << game.timeMs()
               << " prompt=" << commands.snapshot().fixturePrompt.data() << '\n';
        if (game.actors().snapshot().count == 1 && !game.actors().snapshot().actors[0].hostile()) {
            const auto& npc = game.actors().snapshot().actors[0].npc;
            const auto feet = actorPosition(game.actors().snapshot().actors[0], game.timeMs());
            report << "resident cell=" << npc.cell.x << ',' << npc.cell.y << " subcell=" << npc.subcell.x
                   << ',' << npc.subcell.y << " feet=" << feet.x << ',' << feet.y
                   << " facing=" << headingName(npc.facing) << '\n';
        }
    };
    auto travel = [&](bool cancel) {
        auto ticket = run->prepareTravel(run->doorwayRequest());
        auto next = std::make_unique<GameView>(renderer);
        NpcAnimator animation(ticket.game().actors(), definitions.timings);
        next->update(ticket.game(), animation.snapshot(), {});
        memory(cancel ? "staged-cancel" : "staged-commit");
        if (!cancel) {
            if (!run->commitTravel(ticket))
                throw std::runtime_error("Travel commit failed");
            next->activate();
            view.swap(next);
        }
        next.reset(); // Source view must die before ticket's source session.
    };
    run->game().restorePose(sanctuaryCity().doorway("inn").approach, false);
    capture("city-entry");
    memory("city-only");
    travel(true);
    memory("city-after-cancel");
    travel(false);
    capture("entry");
    memory("inn-only");
    if (argc == 4) {
        if (std::string_view(argv[3]) == "dialogue") {
            auto dialogue = loadDialogue(root / "dialogue");
            prepareDialogueUi(dialogue, presenter.fonts());
            auto& game = run->game();
            game.restorePose({{6, 5}, Heading::North}, false);
            update();
            CommandController commands(game, definitions.timings);
            commands.bindDialogue(&dialogue, &run->dialogue());
            commands.residentVisibility(view->picking()->residents());
            commands.refresh(view->picking()->targets(), false, false, true);
            view->present(presenter, game, run->map(), false, true, &commands.snapshot());
            presenter.capture(output / "dialogue-before.png");
            if (!commands.startDialogue())
                throw std::runtime_error("Dialogue admission failed");
            const auto original = world_render::camera(game.movement(), game.geometry());
            const auto target = world_render::dialogueCamera(
                *renderer.scene(), renderer.assets(), game.geometry(), original,
                *game.actors().actor(game.actors().id(0)), *commands.dialoguePlayer().character());
            report << "dialogue camera source=" << original.position.x << ',' << original.position.y << ','
                   << original.position.z << " target=" << target.position.x << ',' << target.position.y
                   << ',' << target.position.z << " horizon=" << target.horizon << '\n';
            NpcAnimator animation(game.actors(), definitions.timings);
            const auto clock = game.timeMs();
            auto shot = [&](const char* name) {
                const auto& player = commands.dialoguePlayer();
                animation.setTime(game.timeMs(), &game.effects(), commands.dialogueSpeaker(),
                                  player.clockMs(), &commands.dialogueAnimation());
                const auto camera =
                    player.active() ? std::optional<world_render::Camera>(
                                          world_render::interpolateCamera(original, target, player.weight()))
                                    : std::nullopt;
                view->update(game, animation.snapshot(), {}, camera,
                             player.active() && (player.leaving() || player.weight() < 1));
                commands.residentVisibility(view->picking() ? view->picking()->residents()
                                                            : TargetSnapshot{});
                commands.refresh(view->picking() ? view->picking()->targets() : TargetSnapshot{}, false,
                                 false, true);
                view->present(presenter, game, run->map(), false, true, &commands.snapshot());
                presenter.capture(output / (std::string(name) + ".png"));
                if (camera && !world_render::sameCamera(renderer.visibility()->frame().camera, *camera))
                    throw std::runtime_error("Camera mismatch");
                report << name << " weight=" << player.weight() << " revealing=" << player.revealing()
                       << " clip=" << unsigned(animation.snapshot().frames[0].clip)
                       << " game_ms=" << game.timeMs() << " presentation_ms=" << player.clockMs() << '\n';
            };
            commands.tickDialogue(.35, true);
            game.update(.35, false);
            shot("dialogue-transition");
            commands.tickDialogue(.35, true);
            shot("dialogue-greeting");
            UiInput topic;
            topic.pressed[unsigned(UiKey::Two)] = true;
            routeHudInput(commands, topic);
            UiInput reveal;
            reveal.pressed[unsigned(UiKey::DialogueAdvance)] = true;
            routeHudInput(commands, reveal);
            shot("dialogue-topic-idle");
            UiInput leave;
            leave.pressed[unsigned(UiKey::Cancel)] = true;
            routeHudInput(commands, leave);
            commands.tickDialogue(.7, true);
            shot("dialogue-restored");
            if (game.timeMs() != clock || !(game.movement().player == Player{{6, 5}, Heading::North}))
                throw std::runtime_error("Dialogue advanced simulation");
            commands.residentVisibility(view->picking()->residents());
            if (!commands.startDialogue())
                throw std::runtime_error("Repeat conversation admission failed");
            commands.tickDialogue(.7, true);
            shot("dialogue-return");
            if (commands.snapshot().dialogue.introduction)
                throw std::runtime_error("Introduction repeated");
            commands.finishDialogue();
            auto saved = run->checkpoint();
            if (!saved->dialogue.get(dialogueKey("sanctuary.innkeeper"))->introduced)
                throw std::runtime_error("Introduction missing from save");
            view.reset();
            memory("released");
            return 0;
        }
        if (std::string_view(argv[3]) == "innkeeper") {
            auto& game = run->game();
            game.restorePose({{6, 5}, Heading::North}, false);
            capture("innkeeper-front");
            if (!view->picking() || view->picking()->targets().count ||
                view->picking()->targets().candidateCount ||
                game.combat().snapshot().phase != CombatPhase::Exploration)
                throw std::runtime_error("Innkeeper must remain outside offensive picking and combat");
            game.restorePose({{5, 3}, Heading::East}, false);
            capture("innkeeper-right");
            game.restorePose({{7, 3}, Heading::West}, false);
            capture("innkeeper-left");
            game.restorePose({{6, 5}, Heading::North}, false);
            if (!game.actors().talk(game.actors().id(0), true))
                throw std::runtime_error("Resident talk preview rejected");
            game.update(.18, false);
            capture("innkeeper-talk-1");
            game.update(.16, false);
            capture("innkeeper-talk-2");
            game.actors().talk(game.actors().id(0), false);
            view.reset();
            memory("released");
            return 0;
        }
        if (std::string_view(argv[3]) != "details")
            throw std::runtime_error("Unknown capture selection");
        run->game().restorePose({{3, 9}, Heading::West}, false);
        capture("plant-close");
        run->game().restorePose({{8, 3}, Heading::North}, false);
        capture("ale-barrels");
        view.reset();
        memory("released");
        return 0;
    }
    const std::pair<const char*, Player> shots[]{
        {"common-room", {{5, 8}, Heading::North}}, {"bar", {{6, 5}, Heading::North}},
        {"hearth", {{4, 4}, Heading::West}},       {"seating", {{5, 6}, Heading::West}},
        {"stairs", {{7, 5}, Heading::East}},       {"plants-exit", {{5, 7}, Heading::South}},
        {"staff-aisle", {{4, 3}, Heading::East}},  {"plant-close", {{3, 9}, Heading::West}},
        {"ale-barrels", {{8, 3}, Heading::North}}, {"south-wall", {{5, 5}, Heading::South}},
        {"table-north", {{3, 5}, Heading::South}}, {"table-south", {{3, 7}, Heading::North}},
        {"table-west", {{2, 6}, Heading::East}},   {"table-east", {{4, 6}, Heading::West}},
    };
    for (const auto& shot : shots) {
        run->game().restorePose(shot.second, false);
        capture(shot.first);
    }
    run->game().restorePose({{4, 4}, Heading::West}, false);
    capture("fire-0");
    const auto pickingRevision = renderer.visibility()->frame().revision;
    run->update(.2, false);
    capture("fire-1");
    if (renderer.visibility()->frame().revision != pickingRevision)
        throw std::runtime_error("Decorative fire invalidated picking geometry");
    const auto clock = run->game().timeMs();
    const auto image = renderer.image().revision;
    run->update(.2, true);
    update();
    if (run->game().timeMs() != clock || renderer.image().revision != image)
        throw std::runtime_error("Pause did not freeze fire/image cache");
    report << "Fire animation preserves picking revision and paused image cache\n";
    run->party().damage({0}, 1);
    capture("free-rest", false, true);
    capture("inn-map", true);
    auto checkpoint = run->checkpoint();
    auto text =
        encodeCampaign(catalog, definitions, {"inn-review", definitions.definitionFingerprint}, *checkpoint);
    report << "Active inn serialized bytes=" << text.size() << '\n';
    auto saved =
        decodeCampaign(catalog, definitions, {"inn-review", definitions.definitionFingerprint}, text);
    view.reset();
    run = std::make_unique<CampaignRun>(catalog, std::move(*saved), definitions.timings);
    view = std::make_unique<GameView>(renderer);
    capture("continue-inn");
    run->game().restorePose(run->location(4).exit, false);
    capture("exit");
    travel(false);
    capture("city-return");
    memory("city-returned");
    view.reset();
    memory("released");
    if (renderer.memory().residentTextures || renderer.memory().residentAlpha)
        throw std::runtime_error("World assets retained after view release");
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
