#pragma once
#include "assets/blueprint_loader.hpp"
#include "review_residents.hpp"
#include "ui/dialogue_preparation.hpp"

// Production integration/capture using application-owned input and a hidden window.
inline void reviewDialogueBeats(const std::filesystem::path& root, const std::filesystem::path& output) {
    using namespace blobber;
    const auto rules = loadRules(root / "rules/catalog.json");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    const auto roomBlueprints = blueprints::load(root / "blueprints/underkeep.json");
    CampaignRun run(roomBlueprints, rules, *rules.preset(), definitions.types, definitions.timings, 5);
    Window window(1920, 1080, true);
    if (!IsWindowState(FLAG_WINDOW_HIDDEN))
        throw std::runtime_error("Capture window must stay hidden");
    GpuResources resources;
    Presenter presenter(resources, root);
    WorldRenderer renderer(resources, world_assets::load(root / "prepared/world"));
    auto view = std::make_unique<GameView>(renderer);
    auto& game = run.game();
    auto catalog = loadDialogue(root / "dialogue");
    prepareDialogueUi(catalog, presenter.fonts());
    const auto beforeOverlays = resources.counts();
    presenter.prepareDialogue(catalog.overlayFiles);
    if (resources.counts().textures != beforeOverlays.textures + catalog.overlayFiles.size())
        throw std::runtime_error("Overlay preparation count mismatch");
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
               << " serial=" << commands.dialoguePlayer().beatSerial() << " moving=" << camera.moving()
               << " overlay=" << commands.snapshot().dialogue.overlay.index;
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
    input(UiKey::DialogueAdvance);
    capture("panel-introduction");
    input(UiKey::Two);
    const auto advanceBeat = [&] {
        const auto serial = commands.dialoguePlayer().beatSerial();
        for (unsigned i = 0; i < 128 && commands.dialoguePlayer().beatSerial() == serial; ++i)
            input(UiKey::DialogueAdvance);
        if (commands.dialoguePlayer().beatSerial() == serial)
            throw std::runtime_error("Beat did not advance");
    };
    const auto overlay = [&] { return bool(commands.snapshot().dialogue.overlay); };
    if (overlay())
        throw std::runtime_error("Scroll appeared before document beat");
    input(UiKey::DialogueAdvance);

    advanceBeat();
    const auto panMs = [&] {
        return std::max(dialogueMinimumPanMs,
                        commands.dialoguePlayer().beat()->durationMs.value_or(dialogueMinimumPanMs));
    };
    const auto delay = commands.dialoguePlayer().beat()->overlayDelayMs;
    const auto documentPan = panMs();
    const auto& pull = definitions.timings[game.actors().actor(game.actors().id(1))->npc.type.index]
                                          [unsigned(NpcAnimation::PullDocument)];
    const auto hold = std::max(documentPan, npcClipDuration(pull) - pull.durationMs[pull.count - 1]);
    if (delay <= hold)
        throw std::runtime_error("Script delay leaves no unobstructed hold pose");
    tick((documentPan - 1) / 1000.);
    if (!camera.moving() || overlay())
        throw std::runtime_error("Boric pan/overlay timing failed");
    tick(.001);
    if (camera.moving() || animator.snapshot().frames[1].clip != NpcAnimation::PullDocument)
        throw std::runtime_error("Boric document cue or authored pan endpoint failed");
    tick((hold - documentPan) / 1000.);
    if (overlay())
        throw std::runtime_error("Scroll covered the document pull");
    capture("01-boric-holds-document");
    tick((delay - hold - 1) / 1000.);
    if (overlay())
        throw std::runtime_error("Scroll appeared before authored delay");
    tick(.001);
    if (!overlay() || animator.snapshot().frames[1].active != 3)
        throw std::runtime_error("Document pull did not hold final frame");
    capture("02-boric-scroll");
    const auto documentSerial = commands.dialoguePlayer().beatSerial();
    UiInput click;
    click.pressed[unsigned(UiKey::Click)] = true;
    click.pointer = ArtPoint{20, 265};
    routeHudInput(commands, click);
    refresh();
    if (!overlay() || !commands.snapshot().dialogue.revealing)
        throw std::runtime_error("Speech body click still advances dialogue");
    click.pointer = ArtPoint{240, 345};
    routeHudInput(commands, click);
    refresh();
    if (overlay() || commands.dialoguePlayer().beatSerial() != documentSerial)
        throw std::runtime_error("First Next failed to dismiss while revealing the current line");
    capture("03-boric-dismissed");
    tick(.1);
    if (overlay())
        throw std::runtime_error("Dismissed scroll reappeared");
    advanceBeat();
    const auto balinPan = panMs();
    tick((balinPan - 1) / 1000.);
    if (!camera.moving() || overlay() || animator.snapshot().frames[1].clip != NpcAnimation::Idle)
        throw std::runtime_error("Balin pan or outgoing guard reset failed");
    tick(.001);
    if (camera.moving() || animator.snapshot().frames[2].clip != NpcAnimation::Laugh)
        throw std::runtime_error("Balin Laugh or authored pan endpoint failed");
    input(UiKey::DialogueAdvance);
    capture("04-balin-unobstructed");
    if (overlay() || animator.snapshot().frames[2].clip != NpcAnimation::Idle)
        throw std::runtime_error("Balin completion changed overlay or failed idle reset");
    advanceBeat();
    tick(panMs() / 1000.);
    input(UiKey::DialogueAdvance);
    if (overlay() || animator.snapshot().frames[1].clip != NpcAnimation::Idle)
        throw std::runtime_error("Boric completion changed overlay or failed idle reset");
    advanceBeat();
    if (overlay() || animator.snapshot().frames[0].clip != NpcAnimation::ExplosiveRage)
        throw std::runtime_error("Mayor reaction failed to clear overlay immediately");
    tick(.1);
    if (!camera.moving())
        throw std::runtime_error("Rage pan ended before 200 ms");
    tick(.1);
    if (camera.moving())
        throw std::runtime_error("Rage pan did not end at 200 ms");
    tick(1);
    tick(.5);
    capture("05-mayor-rage");
    for (unsigned i = 0; i < 10 && commands.snapshot().dialogue.revealing; ++i)
        tick(1);
    if (animator.snapshot().frames[0].clip != NpcAnimation::Idle)
        throw std::runtime_error("Rage persisted after natural text reveal");
    capture("panel-final-page");
    // Replay then abandon while the document is visible.
    input(UiKey::Two);
    advanceBeat();
    if (overlay())
        throw std::runtime_error("Replay skipped overlay delay");
    tick(commands.dialoguePlayer().beat()->overlayDelayMs / 1000.);
    if (!overlay())
        throw std::runtime_error("Replay did not show scroll after delay");
    input(UiKey::Three);
    input(UiKey::DialogueAdvance);
    capture("panel-single-page");
    if (overlay())
        throw std::runtime_error("Topic replacement retained scroll");
    input(UiKey::Two);
    advanceBeat();
    input(UiKey::DialogueAdvance); // Cancel pending overlay before its delay.
    tick(.7);
    if (overlay())
        throw std::runtime_error("Early Next failed to cancel pending scroll");
    input(UiKey::Cancel);
    if (overlay())
        throw std::runtime_error("Exit retained scroll");
    tick(.7);
    capture("06-exploration-restored");
    if (effective || game.inConversation())
        throw std::runtime_error("Dialogue failed to release presentation");
    const auto withOverlay = resources.counts();
    presenter.clearDialogue();
    if (resources.counts().textures + catalog.overlayFiles.size() != withOverlay.textures)
        throw std::runtime_error("Overlay teardown leaked texture leases");
    view.reset();
    if (renderer.memory().textureCount || renderer.memory().residentAlpha)
        throw std::runtime_error("Dialogue capture leaked scene leases");
    report << "All portrait beats, replay/interruption, camera forwarding, idle reset and resource release "
              "passed.\n";
}
