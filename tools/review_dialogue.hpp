#pragma once
#include "review_residents.hpp"

// Production integration/capture using application-owned input and a hidden window.
inline void reviewDialogueBeats(const std::filesystem::path& root, const std::filesystem::path& output) {
    using namespace blobber;
    const auto rules = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    CampaignRun run(rules, *rules.preset(), definitions.types, definitions.timings, 5);
    Window window(1920, 1080, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Capture window must stay hidden");
    GpuResources resources;
    Presenter presenter(resources, root);
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    auto view = std::make_unique<GameView>(renderer);
    auto& game = run.game();
    auto catalog = loadDialogue(root / "dialogue");
    prepareDialogueLayout(
        catalog,
        [](const char* t, void* p) { return static_cast<const UiFonts*>(p)->width(t, UiTextStyle::Label); },
        const_cast<UiFonts*>(&presenter.fonts()));
    CommandController commands(game, definitions.timings);
    commands.bindDialogue(&catalog, &run.dialogue());
    NpcAnimator animator(game.actors(), definitions.timings);
    world_render::DialogueCameraController camera;
    std::ofstream report(output / "dialogue-beats.txt");
    game.restorePose({{10, 8}, Heading::West}, false);
    const auto original = world_render::camera(game.movement(), game.geometry());
    std::optional<world_render::Camera> effective;
    const auto refresh = [&] {
        const auto& player = commands.dialoguePlayer();
        animator.setTime(game.timeMs(), &game.effects(), commands.dialogueSpeaker(), player.clockMs(),
                         &commands.dialogueAnimation());
        if (renderer.scene()) {
            const auto* host = player.character();
            const auto primary = host ? commands.dialogueActor(host->primary) : std::optional<ActorId>{};
            const auto target = player.beat() ? commands.dialogueActor(player.beat()->target) : primary;
            effective = camera.update(*renderer.scene(), renderer.assets(), game.geometry(), original,
                                      primary ? game.actors().actor(*primary) : nullptr,
                                      target ? game.actors().actor(*target) : nullptr, player);
        }
        view->update(game, animator.snapshot(), {}, effective, camera.moving());
        view->activate();
        const auto* picking = view->picking();
        commands.residentVisibility(picking ? picking->residents() : TargetSnapshot{});
        commands.refresh(picking ? picking->targets() : TargetSnapshot{}, false, false, true);
        if (effective && !world_render::sameCamera(renderer.visibility()->frame().camera, *effective))
            throw std::runtime_error("Effective camera mismatch");
        if (game.timeMs())
            throw std::runtime_error("Dialogue advanced simulation");
    };
    const auto capture = [&](const char* name) {
        refresh();
        view->present(presenter, game, run.map(), false, true, &commands.snapshot());
        presenter.capture(output / (std::string(name) + ".png"));
        report << name << " speaker=" << commands.snapshot().dialogue.name
               << " serial=" << commands.dialoguePlayer().beatSerial() << " moving=" << camera.moving();
        if (effective)
            report << " camera=" << effective->position.x << ',' << effective->position.y << ','
                   << effective->position.z;
        for (unsigned i = 0; i < 3; ++i)
            report << " clip" << i << '=' << unsigned(animator.snapshot().frames[i].clip);
        report << '\n';
    };
    const auto input = [&](UiKey key) {
        UiInput i;
        i.pressed[unsigned(key)] = true;
        routeHudInput(commands, i);
        refresh();
    };
    const auto tick = [&](double dt) {
        commands.tickDialogue(dt, true);
        refresh();
    };
    refresh();
    if (!commands.startDialogue(game.actors().id(0)))
        throw std::runtime_error("Mayor admission failed");
    refresh();
    tick(.7);
    input(UiKey::Five);
    input(UiKey::DialogueAdvance);
    capture("01-mayor-report");
    if (animator.snapshot().frames[0].clip != NpcAnimation::Idle)
        throw std::runtime_error("Mayor Talk persisted after click reveal");
    input(UiKey::DialogueAdvance);
    tick(.1);
    if (!camera.moving())
        throw std::runtime_error("Pan finished before 200 ms");
    capture("02-boric-whip");
    tick(.1);
    capture("03-boric-laugh");
    if (camera.moving() || animator.snapshot().frames[1].clip != NpcAnimation::Laugh)
        throw std::runtime_error("Boric reveal or 200 ms pan failed");
    tick(1);
    capture("04-boric-revealed-idle");
    if (animator.snapshot().frames[1].clip != NpcAnimation::Idle || commands.snapshot().dialogue.revealing)
        throw std::runtime_error("Boric Laugh persisted after natural reveal");
    input(UiKey::DialogueAdvance);
    tick(.1);
    if (!camera.moving())
        throw std::runtime_error("Return finished before 200 ms");
    tick(.1);
    input(UiKey::DialogueAdvance);
    if (animator.snapshot().frames[0].clip != NpcAnimation::Idle)
        throw std::runtime_error("Mayor Talk persisted after reveal");
    input(UiKey::DialogueAdvance);
    tick(.2);
    if (animator.snapshot().frames[2].clip != NpcAnimation::Laugh)
        throw std::runtime_error("Balin did not laugh during reveal");
    input(UiKey::DialogueAdvance);
    capture("05-balin-revealed-idle");
    if (animator.snapshot().frames[2].clip != NpcAnimation::Idle)
        throw std::runtime_error("Balin Laugh persisted after click reveal");
    input(UiKey::DialogueAdvance);
    tick(.2);
    input(UiKey::DialogueAdvance);
    if (!run.dialogue().get(dialogueKey("sanctuary.mayor"))->knows(dialogueKey("heard_guards_lunch")))
        throw std::runtime_error("Completed response did not record running gag fact");
    input(UiKey::Five);
    input(UiKey::DialogueAdvance);
    if (std::string_view(commands.snapshot().dialogue.text.data()).find("forbidden") ==
        std::string_view::npos)
        throw std::runtime_error("Repeat response was not selected");
    input(UiKey::Cancel);
    tick(.7);
    capture("06-exploration-restored");
    if (effective || game.inConversation())
        throw std::runtime_error("Dialogue failed to release presentation");
    view.reset();
    if (renderer.memory().textureCount || renderer.memory().residentAlpha)
        throw std::runtime_error("Dialogue capture leaked scene leases");
    report << "All beats, host fact continuity, camera forwarding, idle reset and resource release passed.\n";
}
