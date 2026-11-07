#include "assets/dialogue.hpp"
#include "game/commands.hpp"
#include "render/dialogue_camera.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include "ui/button_targets.hpp"
#include "ui/dialogue_layout.hpp"
#include "ui/interaction.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
using namespace blobber;
namespace {
using Json = nlohmann::json;
Json source() {
    return Json::parse(
        R"({"version": 2, "primary": "host", "participants": {"host": {"resident": "fixture.host", "name": "Host"}}, "facts": ["advice"], "greeting": {"beats": [{"speaker": "host", "text": "Welcome back."}]}, "introduction": {"beats": [{"speaker": "host", "text": "Welcome to the inn."}, {"speaker": "host", "text": "Ask me anything."}]}, "topics": [{"id": "about", "label": "About yourself", "kind": "introduction"}, {"id": "advice", "label": "Advice", "responses": [{"set_fact": "advice", "beats": [{"speaker": "host", "text": "Keep your friends close."}, {"speaker": "host", "text": "Know when to turn back."}]}]}, {"id": "rumours", "label": "Rumours", "responses": [{"beats": [{"speaker": "host", "text": "Ask about advice first."}]}, {"when_fact": "advice", "beats": [{"speaker": "host", "text": "Here is another piece of advice."}]}]}, {"id": "farewell", "label": "Farewell", "kind": "farewell"}]})");
}
struct Directory {
    std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        ("blobber-dialogue-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Directory() { std::filesystem::create_directory(path); }
    ~Directory() {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }
    void write(const Json& j) { std::ofstream(path / "dialogue.yaml") << j.dump(2); }
};
float measure(const char* text, void*) {
    return float(std::strlen(text) * 4);
}
DialogueCatalog catalog() {
    Directory d;
    d.write(source());
    auto out = loadDialogue(d.path);
    prepareDialogueLayout(out, measure, nullptr);
    return out;
}
void parser() {
    Directory d;
    auto j = source();
    d.write(j);
    auto c = loadDialogue(d.path);
    check(c.characters[0].eyeHeight == 0, "Omitted eye height selects automatic framing");
    check(c.characters.size() == 1 && c.find(dialogueKey("fixture.host")),
          "Readable YAML loads stable identity");
    auto distanceOnly = j;
    distanceOnly["participants"]["host"]["camera"] = {{"distance", .7}};
    d.write(distanceOnly);
    check(loadDialogue(d.path).characters[0].eyeHeight == 0,
          "Distance adjustment retains automatic eye line");
    for (unsigned fault = 0; fault < 11; ++fault) {
        auto bad = j;
        if (fault == 0)
            bad["participants"]["duplicate"] = bad["participants"]["host"];
        if (fault == 1)
            bad["topics"][1]["responses"][0]["set_fact"] = "missing";
        if (fault == 2)
            bad["greeting"]["beats"][0]["text"] = std::string(1025, 'a');
        if (fault == 3)
            bad["topics"][1]["responses"][0]["when_fact"] = "advice";
        if (fault == 4)
            bad["participants"]["host"]["camera"] = {{"distance", 0}};
        if (fault == 5)
            bad["greeting"]["beats"][0]["text"] = "Curly \u2019";
        if (fault == 6)
            bad["topics"][0]["kind"] = "trade";
        if (fault == 7)
            bad["greeeting"] = bad["greeting"];
        if (fault == 8)
            bad["presentation"] = {{"transition_ms", 700.5}};
        if (fault == 9)
            bad["presentation"] = {{"fade_ms", -1}};
        if (fault == 10)
            bad["presentation"] = {{"transition_ms", 100}};
        d.write(bad);
        bool rejected = false;
        try {
            loadDialogue(d.path);
        } catch (const std::exception& e) {
            rejected = std::string(e.what()).find("dialogue.yaml") != std::string::npos;
        }
        check(rejected, "Malformed dialogue rejects with filename");
    }
    std::ofstream(d.path / "dialogue.yaml") << "{\"version\":1,\"version\":1}";
    rejects([&] { loadDialogue(d.path); }, "Duplicate YAML keys reject");
    c.characters[0].greeting.beats[0].text = "one two three four five six seven eight nine ten eleven twelve";
    prepareDialogueLayout(c, measure, nullptr, 40, 2);
    check(c.characters[0].greeting.pages.size() > 1,
          "Long authored pages paginate with metrics, not clipping");
}
void flow() {
    auto c = catalog();
    auto second = c.characters[0];
    second.id = dialogueKey("fixture.other");
    second.key = "fixture.other";
    second.participants[second.primary].id = second.id;
    c.characters.push_back(second);
    DialogueHistory history;
    DialoguePlayer player;
    DialogueHud h;
    player.bind(&c, &history);
    check(player.start(c.characters[0].id), "First meeting opens");
    player.snapshot(h);
    check(h.introduction && h.count == 4 && h.text[0] == 0,
          "Introduction starts automatically, topics immediate");
    check(h.read[0] && !h.read[1] && !h.read[2] && !h.read[3],
          "Automatic introduction is read immediately; other topics remain unread");
    player.tick(.15, true);
    player.snapshot(h);
    check(h.text[0] == 0, "Greeting starts after panel fade");
    player.tick(.001, true);
    player.snapshot(h);
    check(std::strlen(h.text.data()) == 1 && player.cuesReady(),
          "First visible character and speech cues start together after fade");
    player.tick(.099, true);
    player.snapshot(h);
    check(std::strlen(h.text.data()) == 6 && h.revealing,
          "Default reveal is about 30 percent faster and uses its own clock");
    const auto clock = player.clockMs();
    player.tick(1, false);
    check(player.clockMs() == clock, "Focus loss freezes dialogue presentation");
    player.advance();
    player.snapshot(h);
    check(!h.revealing && h.page == 1, "One press reveals but does not advance");
    player.advance();
    player.snapshot(h);
    check(h.revealing && h.page == 2 && h.text[0] != 0, "Next press advances exactly one page");
    player.choose(1);
    player.advance();
    player.choose(2);
    check(!history.get(c.characters[0].id)->knows(dialogueKey("advice")),
          "Interrupting before final page awards no fact");
    player.advance();
    player.snapshot(h);
    check(std::string_view(h.text.data()) == "Ask about advice first.", "Default response before fact");
    check(h.read[0] && !h.read[1] && h.read[2],
          "Interrupted introduction stays read while ordinary topics require completion");
    player.choose(1);
    player.advance();
    player.advance();
    player.advance();
    check(history.get(c.characters[0].id)->knows(dialogueKey("advice")), "Revealing final page records fact");
    player.choose(2);
    player.advance();
    player.snapshot(h);
    check(std::string_view(h.text.data()) == "Here is another piece of advice.",
          "Fact selects alternate response");
    player.choose(0);
    player.skipIntroduction();
    player.snapshot(h);
    check(!h.revealing && h.pages == 0 && h.count == 4, "Skip introduction goes straight to topics");
    player.leave();
    const auto weight = player.weight();
    player.tick(.1, true);
    check(player.weight() <= weight, "Exit reverses from current weight");
    player.close();
    check(player.start(c.characters[0].id), "Return opens");
    player.advance();
    player.snapshot(h);
    check(h.text[0] == 0 && !player.cuesReady(), "Early reveal waits for panel fade and its sound cue");
    player.tick(.15, true);
    player.snapshot(h);
    check(!h.introduction && std::string_view(h.text.data()) == "Welcome back.",
          "Return uses short greeting");
    check(h.read[0], "Previously introduced resident stays read without an introduction completion key");
    player.close();
    check(player.start(c.characters[1].id), "Second friendly character opens");
    player.snapshot(h);
    check(h.introduction && !history.get(c.characters[1].id)->knows(dialogueKey("advice")),
          "Same dialogue/art has independent character history");
    check(h.read[0] && !h.read[1] && !h.read[2],
          "Each resident marks their own automatic introduction read without sharing topic history");
    DialogueHistory full;
    for (unsigned i = 0; i < 64; ++i) {
        check(full.get(i + 1, true) != nullptr, "Bounded history accepts capacity");
    }
    check(!full.get(65, true) && full.count == 64, "History exhaustion rejects without growth");
    auto* filled = full.get(1);
    for (unsigned i = 0; i < 64; ++i)
        check(full.complete(1, 0, i + 1), "Bounded facts accept capacity");
    check(!full.complete(1, 999, 999) && filled->completedCount == 0 && filled->factCount == 64,
          "Fact exhaustion is atomic with topic completion");
    history.validate();
    auto corrupt = history;
    corrupt.count = 65;
    rejects([&] { corrupt.validate(); }, "Malformed history rejects");
    player.tick(.3, true);
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 100; ++i) {
        player.tick(.01, true);
        player.snapshot(h);
        player.select(1);
    }
    watching = false;
    check(allocations == 0, "Warmed reveal, history and snapshots allocate no C++ memory");
}
NpcInstance resident() {
    NpcInstance n{{0}, {4, 3}, {1, 1}, Heading::South};
    n.role = NpcRole::Resident;
    n.residentId = dialogueKey("fixture.host");
    return n;
}
void integration() {
    test::VisibilityFixture fixture(1, 0, false, true);
    auto publication = fixture.publication();
    world_assets::CpuAssets assets(publication);
    const auto lease = assets.acquire(world_assets::allAssets(assets.publication));
    const auto map = test::worldRoom(9, 9);
    const auto rules = spellCatalog();
    const Player pose{{4, 5}, Heading::North};
    const NpcWorld world(map, pose.cell, {"goblin"}, {resident()});
    GameSession game(map, rules, world, 5, pose, false, fixture.timings);
    const auto scene = world_render::prepare(game.geometry(), {}, {}, {}, &assets);
    world_render::VisibilityPipeline visibility(scene, assets);
    NpcAnimator animator(game.actors(), fixture.timings);
    auto c = catalog();
    DialogueHistory history;
    CommandController controls(game, fixture.timings);
    controls.bindDialogue(&c, &history);
    animator.setTime(0);
    world_render::FrameInput input{game.geometry(), game.movement(), &game.actors(), &animator.snapshot()};
    visibility.update(input);
    const auto* picking = visibility.picking();
    check(picking && picking->residents().count == 1 && picking->targets().count == 0,
          "Resident visible and independently picked without offensive targeting");
    bool picked = false;
    for (int y = 0; y < viewHeight && !picked; ++y)
        for (int x = 0; x < viewWidth && !picked; ++x)
            if (picking->pickResident(x, y)) {
                check(!picking->pick(x, y), "Friendly body cannot be attacked");
                picked = true;
            }
    check(picked, "Friendly per-pixel body picking");
    controls.residentVisibility(picking->residents());
    controls.refresh(picking->targets(), false, false, true);
    check(controls.nearbyResident() && controls.startDialogue(),
          "Frontal visible resident admits conversation");
    check(controls.snapshot().modal() && !controls.snapshot().modal(false),
          "Dialogue pauses gameplay while allowing a separate ambient pause policy");
    auto overlapping = controls.snapshot();
    overlapping.menuOpen = true;
    check(overlapping.modal(false), "An explicit menu still pauses ambience over dialogue");
    overlapping.menuOpen = false;
    overlapping.inventory.open = true;
    check(overlapping.modal(false), "Other gameplay overlays still pause ambience over dialogue");
    const auto position = game.movement().player;
    const auto time = game.timeMs();
    game.update(.5, false);
    game.move(Action::Forward);
    check(game.timeMs() == time && game.movement().player == position && !game.movement().transition.active(),
          "Conversation freezes authoritative gameplay and movement");
    check(game.submit({}).reason == CommandReason::Unavailable,
          "Conversation rejects direct combat submission");
    check(game.saveSafety() && game.travelSafety(), "Active conversation rejects saving/travel");
    controls.tickDialogue(.3, true);
    check(controls.dialogueAnimation().clip == NpcAnimation::Talk &&
              game.actors().actor(game.actors().id(0))->npc.animation == NpcAnimation::Idle,
          "Revealing drives a presentation-only Talk override");
    animator.setTime(time, nullptr, controls.dialogueSpeaker(), controls.dialoguePlayer().clockMs(),
                     &controls.dialogueAnimation());
    const auto before = animator.snapshot().revision;
    controls.tickDialogue(.13, true);
    animator.setTime(time, nullptr, controls.dialogueSpeaker(), controls.dialoguePlayer().clockMs(),
                     &controls.dialogueAnimation());
    check(animator.snapshot().revision != before,
          "Speaker animates on presentation clock while game time frozen");
    UiInput click;
    click.pressed[unsigned(UiKey::Click)] = true;
    click.pointer = ArtPoint{20, 265};
    const auto textBeforeClick = controls.snapshot().dialogue.text;
    routeHudInput(controls, click);
    check(controls.snapshot().dialogue.text == textBeforeClick && controls.snapshot().dialogue.revealing,
          "Clicking speech body cannot reveal or advance");
    UiInput space;
    space.pressed[unsigned(UiKey::DialogueAdvance)] = true;
    routeHudInput(controls, space);
    check(game.actors().actor(game.actors().id(0))->npc.animation == NpcAnimation::Idle &&
              !controls.snapshot().dialogue.revealing,
          "Skip reveal immediately returns speaker to Idle");
    click.pointer = ArtPoint{240, 345};
    routeHudInput(controls, click);
    check(controls.snapshot().dialogue.page == 2 && controls.snapshot().dialogue.revealing,
          "Next button advances introduction instead of skipping it");
    routeHudInput(controls, click);
    check(!controls.snapshot().dialogue.revealing &&
              !dialogueLayout::advanceEnabled(controls.snapshot().dialogue),
          "Next reveals final page and then disables");
    const auto finalSerial = controls.dialoguePlayer().beatSerial();
    routeHudInput(controls, click);
    check(controls.dialoguePlayer().beatSerial() == finalSerial, "Disabled Next does nothing");
    UiInput topic;
    topic.pressed[unsigned(UiKey::Three)] = true;
    routeHudInput(controls, topic);
    check(controls.snapshot().dialogue.pages == 1 && controls.snapshot().dialogue.revealing,
          "Topic selector replaces introduction without Skip button");
    routeHudInput(controls, click);
    check(controls.snapshot().dialogue.revealing, "Absent Next cannot reveal a single-page topic");
    routeHudInput(controls, space);
    check(!controls.snapshot().dialogue.revealing, "Space still reveals single-page topics");
    const auto base = world_render::camera(game.movement(), game.geometry());
    const auto close = world_render::dialogueCamera(
        scene, assets, game.geometry(), base, *game.actors().actor(game.actors().id(0)), c.characters[0]);
    check(!world_render::sameCamera(base, close) && close.horizon != base.horizon,
          "Close camera changes framing without resizing viewport");
    check(std::abs(close.position.y - .7f * .92f) < .001f,
          "Default dialogue camera meets a tall NPC at its eye line");
    check(world_render::sameCamera(base, world_render::interpolateCamera(base, close, 0)) &&
              world_render::sameCamera(close, world_render::interpolateCamera(base, close, 1)),
          "Exact camera endpoints");
    const auto mid = world_render::interpolateCamera(base, close, .5f);
    check(mid.position.z < base.position.z && mid.position.z > close.position.z,
          "Camera interpolates toward resident");
    animator.setTime(time, nullptr, controls.dialogueSpeaker(), controls.dialoguePlayer().clockMs(),
                     &controls.dialogueAnimation());
    input.cameraOverride = mid;
    input.cameraMoving = true;
    check(visibility.update(input, false) && !visibility.picking(),
          "Moving camera invalidates visibility and disables picking");
    input.cameraOverride = close;
    input.cameraMoving = false;
    visibility.update(input);
    check(world_render::sameCamera(visibility.frame().camera, close), "Visibility consumes effective camera");
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 10; ++i) {
        controls.tickDialogue(.01, true);
        animator.setTime(time, nullptr, controls.dialogueSpeaker(), controls.dialoguePlayer().clockMs(),
                         &controls.dialogueAnimation());
        input.cameraOverride = world_render::interpolateCamera(base, close, float(i) / 10);
        visibility.update(input);
    }
    watching = false;
    check(!allocations, "Warmed conversation integration allocates no C++ memory");
    auto blocked = scene;
    const auto v = unsigned(blocked.vertices.size());
    blocked.vertices.push_back({{0, 0, 4.9f}, {}, 0, 0, 1});
    blocked.vertices.push_back({{9, 0, 4.9f}, {}, 0, 0, 1});
    blocked.vertices.push_back({{9, 2, 4.9f}, {}, 0, 0, 1});
    blocked.vertices.push_back({{0, 2, 4.9f}, {}, 0, 0, 1});
    blocked.indices.insert(blocked.indices.end(), {v, v + 1, v + 2, v, v + 2, v + 3});
    const auto stopped = world_render::dialogueCamera(
        blocked, assets, game.geometry(), base, *game.actors().actor(game.actors().id(0)), c.characters[0]);
    check(stopped.position.z > 4.9f, "Camera stops before prepared geometry");
    controls.finishDialogue();
    check(!game.inConversation() && !controls.snapshot().dialogue.open,
          "Close releases authoritative session");
    controls.residentVisibility({});
    controls.refresh({}, false, false, true);
    check(!controls.startDialogue(), "Missing/stale visibility rejects");
    game.restorePose({{4, 5}, Heading::South}, false);
    controls.residentVisibility(picking->residents());
    check(!controls.startDialogue(), "Wrong facing rejects");
    game.restorePose({{4, 7}, Heading::North}, false);
    check(!controls.startDialogue(), "Out-of-range friendly interaction rejects");
    auto foreign = game.actors().id(0);
    ++foreign.world;
    check(!game.beginConversation(foreign), "Foreign resident handle rejects");

    // A second friendly NPC faces a different axis and uses a shorter body and
    // different eye/distance framing. No inn geometry or artwork is involved.
    auto secondPublication = fixture.publication();
    secondPublication.appearances[0].height = .45f;
    world_assets::CpuAssets secondAssets(std::move(secondPublication));
    const auto secondLease = secondAssets.acquire(world_assets::allAssets(secondAssets.publication));
    auto character = c.characters[0];
    character.id = dialogueKey("fixture.other");
    character.key = "fixture.other";
    character.participants[character.primary].id = character.id;
    character.eyeHeight = .35f;
    character.distance = .75f;
    c.characters.push_back(character);
    auto other = resident();
    other.cell = {4, 4};
    other.subcell = {0, 0};
    other.facing = Heading::East;
    other.residentId = character.id;
    const Player secondPose{{5, 4}, Heading::West};
    const NpcWorld otherWorld(map, secondPose.cell, {"goblin"}, {other});
    GameSession otherGame(map, rules, otherWorld, 5, secondPose, false, fixture.timings);
    const auto otherScene = world_render::prepare(otherGame.geometry(), {}, {}, {}, &secondAssets);
    world_render::VisibilityPipeline otherVisibility(otherScene, secondAssets);
    NpcAnimator otherAnimator(otherGame.actors(), fixture.timings);
    otherVisibility.update(
        {otherGame.geometry(), otherGame.movement(), &otherGame.actors(), &otherAnimator.snapshot()});
    CommandController otherControls(otherGame, fixture.timings);
    otherControls.bindDialogue(&c, &history);
    otherControls.residentVisibility(otherVisibility.picking()->residents());
    otherControls.refresh({}, false, false, true);
    check(otherControls.startDialogue() && otherControls.snapshot().dialogue.introduction,
          "Future friendly NPC on another axis uses shared flow");
    const auto otherSource = world_render::camera(otherGame.movement(), otherGame.geometry());
    const auto otherCamera =
        world_render::dialogueCamera(otherScene, secondAssets, otherGame.geometry(), otherSource,
                                     *otherGame.actors().actor(otherGame.actors().id(0)), character);
    const auto head = world_render::project(otherCamera, {4.25f, .45f, 4.25f});
    check(std::abs(otherCamera.position.x - 5.f) < .001f && std::abs(otherCamera.position.y - .35f) < .001f &&
              std::abs(head.y - 14.f) < .01f,
          "Reusable camera frames a short east-facing friendly NPC above the panel");
    character.eyeHeight = 0;
    const auto automaticCamera =
        world_render::dialogueCamera(otherScene, secondAssets, otherGame.geometry(), otherSource,
                                     *otherGame.actors().actor(otherGame.actors().id(0)), character);
    check(std::abs(automaticCamera.position.y - .45f * .92f) < .001f,
          "Default eye line adapts to a shorter NPC without furniture assumptions");
    otherControls.finishDialogue();
    // Same visible resident, but no dialogue entry: no state or error-message side effects.
    c.characters.pop_back();
    otherControls.residentVisibility(otherVisibility.picking()->residents());
    const auto revision = otherGame.actors().snapshot().revision;
    check(!otherControls.startDialogue(otherGame.actors().id(0)) &&
              !otherControls.dialoguePlayer().active() &&
              otherGame.actors().snapshot().revision == revision && !otherGame.saveSafety(),
          "Visible guard without catalog entry stays inert and save-safe");
}
void multiNpc() {
    Directory directory;
    auto sourceFile = source();
    sourceFile["participants"]["guard"] = {
        {"resident", "fixture.guard"}, {"name", "Guard"}, {"text_color", "#9FD6D3"}};
    auto& beats = sourceFile["topics"][1]["responses"][0]["beats"];
    beats = Json::array({{{"speaker", "host"}, {"text", "A report."}, {"animation", "talk"}},
                         {{"speaker", "guard"},
                          {"text", "One two three four five six seven eight nine ten eleven twelve."},
                          {"animation", "laugh"},
                          {"audio", "laugh.wav"}},
                         {{"speaker", "host"}, {"text", "Enough!"}, {"animation", "talk"}}});
    directory.write(sourceFile);
    auto c = loadDialogue(directory.path);
    prepareDialogueLayout(c, measure, nullptr, 40, 2);
    auto& character = c.characters[0];
    const auto& response = character.topics[1].responses[0];
    check(response.pages.size() > response.beats.size(), "Long beat wraps into multiple display pages");
    check(c.audioFiles.size() == 1, "File reference resolved once during preparation even if missing");
    test::VisibilityFixture fixture(1, 0, false, true);
    world_assets::CpuAssets assets(fixture.publication());
    const auto lease = assets.acquire(world_assets::allAssets(assets.publication));
    const auto map = test::worldRoom(9, 9);
    const auto rules = spellCatalog();
    const Player pose{{4, 5}, Heading::North};
    auto guard = resident();
    guard.cell = {3, 3};
    guard.residentId = dialogueKey("fixture.guard");
    const NpcWorld world(map, pose.cell, {"goblin"}, {resident(), guard});
    GameSession game(map, rules, world, 5, pose, false, fixture.timings);
    const auto scene = world_render::prepare(game.geometry(), {}, {}, {}, &assets);
    NpcAnimator animator(game.actors(), fixture.timings);
    world_render::VisibilityPipeline visibility(scene, assets);
    visibility.update({game.geometry(), game.movement(), &game.actors(), &animator.snapshot()});
    CommandController controls(game, fixture.timings);
    DialogueHistory history;
    controls.bindDialogue(&c, &history);
    controls.residentVisibility(visibility.picking()->residents());
    check(controls.startDialogue(game.actors().id(0)), "Host admits multi-NPC conversation");
    world_render::DialogueCameraController camera;
    const auto exploration = world_render::camera(game.movement(), game.geometry());
    const auto sample = [&] {
        const auto& player = controls.dialoguePlayer();
        const auto primary = controls.dialogueActor(character.primary);
        const auto target = controls.dialogueActor(player.beat() ? player.beat()->target : character.primary);
        animator.setTime(game.timeMs(), nullptr, controls.dialogueSpeaker(), player.clockMs(),
                         &controls.dialogueAnimation());
        return camera.update(scene, assets, game.geometry(), exploration,
                             primary ? game.actors().actor(*primary) : nullptr,
                             target ? game.actors().actor(*target) : nullptr, player);
    };
    check(world_render::sameCamera(*sample(), exploration), "Entry starts at exact exploration camera");
    controls.tickDialogue(.7, true);
    const auto primaryCamera = *sample();
    check(!camera.moving(), "700 ms entry settles");
    UiInput topic;
    topic.pressed[unsigned(UiKey::Two)] = true;
    UiInput next;
    next.pressed[unsigned(UiKey::DialogueAdvance)] = true;
    routeHudInput(controls, topic);
    routeHudInput(controls, next); // reveal host
    check(controls.dialogueAnimation().clip == NpcAnimation::Idle,
          "Click-to-complete ends explicit Talk before advancing the beat");
    routeHudInput(controls, next); // guard
    sample();
    const auto serial = controls.dialoguePlayer().beatSerial();
    check(std::string_view(controls.snapshot().dialogue.name) == "Guard" &&
              controls.snapshot().dialogue.textColor == 0x9fd6d3ff,
          "Beat changes speaker label and color while retaining host topics");
    check(controls.dialogueAnimation().clip == NpcAnimation::Laugh &&
              animator.snapshot().frames[0].clip == NpcAnimation::Idle &&
              animator.snapshot().frames[1].clip == NpcAnimation::Laugh,
          "Guard reaction begins and outgoing speaker returns to idle");
    controls.tickDialogue(.1, true);
    const auto midway = *sample();
    check(camera.moving() && !world_render::sameCamera(midway, primaryCamera),
          "Whip-pan is moving at 100 ms");
    controls.tickDialogue(.1, true);
    const auto guardCamera = *sample();
    check(!camera.moving() && !world_render::sameCamera(guardCamera, primaryCamera),
          "Whip-pan settles at 200 ms");
    const auto frozenClock = controls.dialoguePlayer().clockMs();
    controls.tickDialogue(.5, false);
    check(controls.dialoguePlayer().clockMs() == frozenClock &&
              world_render::sameCamera(*sample(), guardCamera),
          "Focus loss freezes the dialogue camera and reactions");
    controls.tickDialogue(1, true);
    sample();
    check(!controls.dialoguePlayer().revealing() && animator.snapshot().frames[1].clip == NpcAnimation::Idle,
          "Natural reveal completion ends explicit Laugh while its text stays displayed");
    while (controls.dialoguePlayer().beat()->speaker != character.primary) {
        routeHudInput(controls, next);
        if (controls.dialoguePlayer().beat()->speaker != character.primary) {
            sample();
            const auto expected =
                controls.dialoguePlayer().revealing() ? NpcAnimation::Laugh : NpcAnimation::Idle;
            check(controls.dialoguePlayer().beatSerial() == serial &&
                      controls.dialogueAnimation().clip == expected &&
                      animator.snapshot().frames[1].clip == expected,
                  "Wrapped pages reveal with Laugh, pause in Idle and keep cue identity");
        }
    }
    sample();
    check(animator.snapshot().frames[1].clip == NpcAnimation::Idle &&
              controls.dialoguePlayer().beatSerial() != serial,
          "Next authored beat resets guard and advances cue identity");
    controls.tickDialogue(.1, true);
    sample();
    check(camera.moving(), "Return is still moving at 100 ms");
    controls.tickDialogue(.1, true);
    check(world_render::sameCamera(*sample(), primaryCamera) && !camera.moving(),
          "Return restores primary at 200 ms");
    check(!controls.dialoguePlayer().revealing() && controls.dialogueAnimation().clip == NpcAnimation::Idle,
          "Natural reveal completion ends explicit Talk on the final beat");
    routeHudInput(controls, next);
    check(history.get(character.id)->knows(dialogueKey("advice")) && !history.get(guard.residentId),
          "Final beat awards only host history");
    check(game.actors().snapshot().actors[0].npc.animation == NpcAnimation::Idle &&
              game.actors().snapshot().actors[1].npc.animation == NpcAnimation::Idle && game.timeMs() == 0,
          "Presentation never mutates authoritative animation or simulation time");
    allocations = 0;
    watching = true;
    for (unsigned i = 0; i < 10; ++i) {
        routeHudInput(controls, topic);
        routeHudInput(controls, next);
        routeHudInput(controls, next);
        controls.tickDialogue(.02, true);
        sample();
    }
    watching = false;
    check(!allocations,
          "Warmed beat changes, participant binding, camera and animation allocate no C++ memory");
    // Retarget an unfinished whip-pan and then leave from an unfinished return.
    const auto beforeRetarget = *sample();
    routeHudInput(controls, topic);
    check(world_render::sameCamera(*sample(), beforeRetarget), "Retarget begins at the displayed camera");
    controls.tickDialogue(.04, true);
    sample();
    UiInput cancel;
    cancel.pressed[unsigned(UiKey::Cancel)] = true;
    routeHudInput(controls, cancel);
    sample();
    check(!controls.dialogueAnimation().actor, "Exit immediately clears all reaction overrides");
    controls.tickDialogue(.35, true);
    sample();
    controls.tickDialogue(.35, true);
    check(!sample() && !game.inConversation(), "Exit releases camera and session state");
    // Short authored pans are clamped; longer pans and independent targets remain available.
    controls.residentVisibility(visibility.picking()->residents());
    check(controls.startDialogue(game.actors().id(0)), "Reopen for explicit camera cues");
    sample();
    controls.tickDialogue(.7, true);
    sample();
    auto& scripted = character.topics[1].responses[0].beats;
    scripted[1].durationMs = 0;
    routeHudInput(controls, topic);
    routeHudInput(controls, next);
    routeHudInput(controls, next);
    check(world_render::sameCamera(*sample(), primaryCamera) && camera.moving(),
          "Zero authored duration starts a pan instead of a cut");
    controls.tickDialogue(.1, true);
    sample();
    check(camera.moving(), "Short authored duration cannot finish before 200 ms");
    controls.tickDialogue(.1, true);
    check(world_render::sameCamera(*sample(), guardCamera) && !camera.moving(),
          "Short authored duration reaches its target at the 200 ms minimum");
    routeHudInput(controls, topic);
    sample();
    controls.tickDialogue(.2, true);
    sample();
    scripted[1].durationMs = 400;
    routeHudInput(controls, next);
    routeHudInput(controls, next);
    sample();
    controls.tickDialogue(.2, true);
    sample();
    check(camera.moving(), "Authored slower pan is not shortened to the minimum");
    controls.tickDialogue(.2, true);
    check(world_render::sameCamera(*sample(), guardCamera) && !camera.moving(),
          "400 ms authored pan settles on time");
    routeHudInput(controls, topic);
    sample();
    controls.tickDialogue(.2, true);
    sample();
    scripted[1].target = character.primary;
    routeHudInput(controls, next);
    routeHudInput(controls, next);
    check(world_render::sameCamera(*sample(), primaryCamera) &&
              std::string_view(controls.snapshot().dialogue.name) == "Guard",
          "Camera can hold host while a different participant speaks");
    scripted[1].target = scripted[1].speaker;
    scripted[1].durationMs.reset();
    controls.finishDialogue();
    sample();
    // A thin obstacle intersects close-up-to-close-up travel, but not either
    // exploration-to-close-up segment. Safe endpoints remain valid; motion cuts.
    auto obstructed = scene;
    const auto vi = unsigned(obstructed.vertices.size());
    const float x = (primaryCamera.position.x + guardCamera.position.x) / 2;
    const float z = primaryCamera.position.z;
    obstructed.vertices.push_back({{x, 0, z - .15f}, {}, 0, 0, 1});
    obstructed.vertices.push_back({{x, 2, z - .15f}, {}, 0, 0, 1});
    obstructed.vertices.push_back({{x, 2, z + .15f}, {}, 0, 0, 1});
    obstructed.vertices.push_back({{x, 0, z + .15f}, {}, 0, 0, 1});
    obstructed.indices.insert(obstructed.indices.end(), {vi, vi + 1, vi + 2, vi, vi + 2, vi + 3});
    world_render::DialogueCameraController blockedCamera;
    controls.residentVisibility(visibility.picking()->residents());
    check(controls.startDialogue(game.actors().id(0)), "Reopen obstruction fixture");
    const auto blockedSample = [&] {
        const auto& player = controls.dialoguePlayer();
        const auto target = controls.dialogueActor(player.beat()->target);
        return blockedCamera.update(obstructed, assets, game.geometry(), exploration,
                                    game.actors().actor(game.actors().id(0)), game.actors().actor(*target),
                                    player);
    };
    blockedSample();
    controls.tickDialogue(.7, true);
    blockedSample();
    routeHudInput(controls, topic);
    routeHudInput(controls, next);
    routeHudInput(controls, next);
    check(world_render::sameCamera(*blockedSample(), guardCamera) && !blockedCamera.moving(),
          "Obstructed translation cuts between safe endpoints instead of crossing geometry");
    controls.finishDialogue();
    sample();
    // An unavailable required participant rejects before replacing current text/history.
    character.participants[response.beats[1].speaker].id = dialogueKey("fixture.absent");
    controls.residentVisibility(visibility.picking()->residents());
    check(controls.startDialogue(game.actors().id(0)), "Primary-only greeting survives missing guard");
    const auto oldSerial = controls.dialoguePlayer().beatSerial();
    routeHudInput(controls, topic);
    check(controls.dialoguePlayer().beatSerial() == oldSerial, "Unavailable response rejects atomically");
    character.participants[response.beats[1].speaker].id = guard.residentId;
    routeHudInput(controls, topic);
    character.participants[response.beats[1].speaker].id = dialogueKey("fixture.absent");
    controls.tickDialogue(.01, true);
    check(!controls.dialoguePlayer().active() && !game.inConversation(),
          "Invalidated required participant tears down safely");
    character.participants[response.beats[1].speaker].id = guard.residentId;
    auto absentClips = fixture.timings;
    absentClips[0][unsigned(NpcAnimation::Laugh)] = {};
    absentClips[0][unsigned(NpcAnimation::Talk)] = {};
    CommandController limited(game, absentClips);
    limited.bindDialogue(&c, &history);
    limited.residentVisibility(visibility.picking()->residents());
    check(limited.startDialogue(game.actors().id(0)), "Missing optional Talk still admits greeting");
    limited.tickDialogue(.3, true);
    NpcAnimator fallback(game.actors(), absentClips);
    fallback.setTime(0, nullptr, limited.dialogueSpeaker(), limited.dialoguePlayer().clockMs(),
                     &limited.dialogueAnimation());
    check(fallback.snapshot().frames[0].clip == NpcAnimation::Idle, "Missing default Talk uses natural Idle");
    const auto beforeMissingClip = limited.dialoguePlayer().beatSerial();
    routeHudInput(limited, topic);
    check(limited.dialoguePlayer().beatSerial() == beforeMissingClip,
          "Explicit unavailable Laugh rejects the response without changing the greeting");
    limited.finishDialogue();
}
void yamlBeats() {
    Directory d;
    auto good = source();
    for (unsigned fault = 0; fault < 9; ++fault) {
        auto bad = good;
        auto& beat = bad["greeting"]["beats"][0];
        if (fault == 0)
            beat["speaker"] = "nobody";
        if (fault == 1)
            beat["camera"] = {{"target", "nobody"}};
        if (fault == 2)
            beat["camera"] = {{"duration_ms", -1}};
        if (fault == 3)
            beat["animation"] = "angry";
        if (fault == 4)
            beat["audio"] = "../outside.wav";
        if (fault == 5)
            bad["participants"]["host"]["text_color"] = "#12345X";
        if (fault == 6)
            bad["greeting"]["beats"] = Json::array();
        if (fault == 7)
            for (unsigned i = 0; i < 32; ++i)
                bad["greeting"]["beats"].push_back(good["greeting"]["beats"][0]);
        if (fault == 8)
            beat["camera"] = {{"duration_ms", 1.5}};
        d.write(bad);
        rejects([&] { loadDialogue(d.path); }, "Invalid beat reference/capacity/path rejects during loading");
    }
    auto valid = good;
    valid["greeting"]["beats"][0]["camera"] = {{"duration_ms", 0}};
    d.write(valid);
    const auto c = loadDialogue(d.path);
    check(c.characters[0].greeting.beats[0].durationMs == 0,
          "Short duration remains author data; playback enforces the minimum");
    valid["greeting"]["beats"][0]["audio"] = "dialogue/voice.wav";
    d.write(valid);
    const auto sharedAudio = d.path / "shared-audio";
    check(loadDialogue(d.path, sharedAudio).audioFiles[0] ==
              std::filesystem::weakly_canonical(sharedAudio / "dialogue/voice.wav").string(),
          "Script override resolves audio against the explicit shared audio root");
    check(loadDialogue(d.path).audioFiles[0] ==
              std::filesystem::weakly_canonical(d.path.parent_path() / "audio/dialogue/voice.wav").string(),
          "Default dialogue loader uses sibling assets/audio directory");
    // A literal YAML file proves multiline speech and comments, independently of JSON-as-YAML fixtures.
    std::ofstream(d.path / "dialogue.yaml") << R"(version: 2
primary: host
participants:
  host: {resident: fixture.host, name: Host}
greeting:
  beats:
    - speaker: host
      text: |
        Hello there.
        A second line.
introduction:
  beats: [{speaker: host, text: Welcome.}] # content-author comment
topics:
  - {id: farewell, label: Farewell, kind: farewell}
)";
    check(loadDialogue(d.path).characters[0].greeting.beats[0].text == "Hello there.\nA second line.\n",
          "YAML literal blocks retain newlines");
}
void overlays() {
    Directory d;
    auto j = source();
    j["introduction"]["beats"][0]["overlay_asset"] = "dialogue/portrait.png";
    j["topics"][1]["responses"][0]["beats"] = Json::array(
        {{{"speaker", "host"},
          {"text", "One two three four five six seven eight nine ten."},
          {"overlay_asset", "dialogue/portrait.png"},
          {"animation", "pull_document"}},
         {{"speaker", "host"}, {"text", "Still here."}},
         {{"speaker", "host"}, {"text", "Another picture."}, {"overlay_asset", "dialogue/other.png"}},
         {{"speaker", "host"},
          {"text", "Enough!"},
          {"clear_overlay", true},
          {"animation", "explosive_rage"}}});
    d.write(j);
    const auto ui = d.path / "custom-ui";
    auto c = loadDialogue(d.path, {}, ui);
    check(c.overlayFiles.size() == 2 &&
              c.overlayFiles[0] == std::filesystem::weakly_canonical(ui / "dialogue/portrait.png").string(),
          "Overlay references deduplicate against explicit UI root");
    check(loadDialogue(d.path).overlayFiles[0] ==
              std::filesystem::weakly_canonical(d.path.parent_path() / "ui/dialogue/portrait.png").string(),
          "Overlay root defaults to sibling UI directory");
    prepareDialogueLayout(c, measure, nullptr, 40, 2);
    DialoguePlayer p;
    DialogueHistory history;
    DialogueHud h;
    p.bind(&c, &history);
    p.start(c.characters[0].id);
    p.snapshot(h);
    check(!h.overlay, "Overlay stays hidden before first visible glyph");
    p.tick(.15, true);
    p.snapshot(h);
    check(!h.overlay, "Fade boundary does not trigger overlay early");
    p.tick(.001, true);
    p.snapshot(h);
    check(h.overlay.index == 0, "First visible glyph applies overlay");
    p.skipIntroduction();
    p.snapshot(h);
    check(!h.overlay, "Skipping clears overlay");
    p.choose(1);
    p.snapshot(h);
    check(h.overlay.index == 0, "Topic shows prepared overlay immediately");
    const auto serial = p.beatSerial();
    allocations = 0;
    watching = true;
    p.tick(1, false);
    p.snapshot(h);
    check(h.overlay.index == 0 && p.beatSerial() == serial, "Focus loss preserves beat and overlay");
    while (p.beatSerial() == serial) {
        p.advance();
        p.snapshot(h);
        check(h.overlay.index == 0, "Page completion and uncued beat preserve overlay");
    }
    while (p.beat()->text == "Still here.")
        p.advance();
    p.snapshot(h);
    check(h.overlay.index == 1, "A new overlay replaces the previous picture");
    while (p.beat()->text == "Another picture.")
        p.advance();
    p.snapshot(h);
    check(!h.overlay, "Explicit clear applies with reaction line");
    p.choose(1);
    p.choose(2);
    p.snapshot(h);
    check(!h.overlay, "Choosing unrelated topic clears previous overlay");
    p.choose(1);
    p.leave();
    p.snapshot(h);
    check(!h.overlay, "Exit clears overlay before camera return");
    p.close();
    p.start(c.characters[0].id);
    p.tick(.2, true);
    p.choose(1);
    p.bind(&c, &history);
    p.snapshot(h);
    check(!h.open && !h.overlay, "Rebind clears overlay and conversation");
    watching = false;
    check(allocations == 0, "Overlay playback/state transitions allocate nothing");
    for (unsigned fault = 0; fault < 6; ++fault) {
        auto bad = j;
        auto& b = bad["introduction"]["beats"][0];
        if (fault == 0)
            b["clear_overlay"] = true;
        if (fault == 1)
            b["overlay_asset"] = "../outside.png";
        if (fault == 2)
            b["overlay_asset"] = "/absolute.png";
        if (fault == 3)
            b["overlay_asset"] = Json::array({"bad"});
        if (fault == 4) {
            b.erase("overlay_asset");
            b["clear_overlay"] = "perhaps";
        }
        if (fault == 5) {
            auto& beats = bad["introduction"]["beats"];
            beats = Json::array();
            for (unsigned i = 0; i <= dialogueOverlayCapacity; ++i)
                beats.push_back({{"speaker", "host"},
                                 {"text", "Picture."},
                                 {"overlay_asset", std::to_string(i) + ".png"}});
        }
        d.write(bad);
        rejects([&] { loadDialogue(d.path, {}, ui); }, "Invalid overlay cue/path/capacity rejects");
    }
    std::filesystem::create_directories(ui);
    std::filesystem::create_directory_symlink(d.path, ui / "escape");
    j["introduction"]["beats"][0]["overlay_asset"] = "escape/out.png";
    d.write(j);
    rejects([&] { loadDialogue(d.path, {}, ui); }, "Overlay symlink escape rejects");
}
void delayedOverlays() {
    Directory d;
    auto j = source();
    auto& b = j["topics"][1]["responses"][0]["beats"][0];
    b["text"] = "One two three four five six seven eight nine ten eleven twelve.";
    b["overlay_asset"] = "dialogue/picture.png";
    b["overlay_delay_ms"] = 600;
    b["overlay_dismiss_on_advance"] = true;
    d.write(j);
    auto c = loadDialogue(d.path);
    prepareDialogueLayout(c, measure, nullptr, 40, 2);
    c.charactersPerSecond = 1;
    const auto& cue = c.characters[0].topics[1].responses[0].beats[0];
    check(cue.overlayDelayMs == 600 && cue.overlayDismissOnAdvance,
          "YAML retains generic overlay delay and dismissal policy");
    DialoguePlayer p;
    DialogueHistory history;
    DialogueHud h;
    p.bind(&c, &history);
    p.start(c.characters[0].id);
    p.tick(.7, true);
    const auto visible = [&] {
        p.snapshot(h);
        return bool(h.overlay);
    };
    p.choose(1);
    allocations = 0;
    watching = true;
    check(!visible(), "Delayed overlay is hidden when beat begins");
    p.tick(.599, true);
    check(!visible(), "Overlay stays hidden through the animation delay");
    const auto clock = p.clockMs();
    p.tick(1, false);
    check(!visible() && p.clockMs() == clock, "Unfocused time cannot finish overlay delay");
    p.tick(.001, true);
    check(visible(), "Overlay appears at the authored delay boundary");
    p.advance();
    check(!visible() && !p.revealing() && h.page == 1,
          "First advance dismisses overlay and still completes current text");
    p.tick(1, true);
    check(!visible(), "Dismissed overlay cannot reappear later in the same beat");
    p.advance();
    p.tick(1, true);
    check(!visible() && h.page == 2, "Page continuation cannot re-trigger dismissed overlay");
    p.choose(1);
    p.tick(.2, true);
    p.advance();
    p.tick(1, true);
    check(!visible(), "Early Next cancels a pending overlay instead of showing it late");
    const auto serial = p.beatSerial();
    while (p.beatSerial() == serial)
        p.advance();
    p.tick(1, true);
    check(!visible(), "Following uncued beat never restores dismissed overlay");
    p.choose(1);
    p.tick(.6, true);
    check(visible(), "Replaying the response schedules the image afresh");
    p.choose(2);
    p.tick(1, true);
    check(!visible(), "Topic replacement cancels delayed cue");
    p.choose(1);
    p.leave();
    p.tick(1, true);
    check(!visible(), "Exit cancels pending cue");
    watching = false;
    check(!allocations, "Delayed overlay playback and cancellation allocate nothing");
    // Natural reveal completion must not dismiss the picture; the next input does.
    c.charactersPerSecond = 1000;
    p.start(c.characters[0].id);
    p.tick(.7, true);
    p.choose(1);
    p.tick(.6, true);
    check(visible() && !p.revealing(), "Natural completion preserves the picture until Next");
    p.advance();
    check(!visible() && h.page == 2, "Next dismisses while retaining normal page advance");
    for (unsigned fault = 0; fault < 6; ++fault) {
        auto bad = j;
        auto& beat = bad["topics"][1]["responses"][0]["beats"][0];
        if (fault == 0)
            beat["overlay_delay_ms"] = -1;
        if (fault == 1)
            beat["overlay_delay_ms"] = 60001;
        if (fault == 2)
            beat["overlay_delay_ms"] = 1.5;
        if (fault == 3)
            beat["overlay_dismiss_on_advance"] = "sometimes";
        if (fault == 4)
            beat.erase("overlay_asset");
        if (fault == 5) {
            beat.erase("overlay_asset");
            beat.erase("overlay_delay_ms");
        }
        d.write(bad);
        rejects([&] { loadDialogue(d.path); }, "Malformed or unattached overlay timing rejects");
    }
}
void authoredReactions() {
    const auto map = test::worldRoom(9, 9);
    const auto rules = spellCatalog();
    const Player pose{{4, 5}, Heading::North};
    const NpcWorld world(map, pose.cell, {"goblin"}, {resident()});
    NpcTimings timings{};
    timings[0] = {{{100, 100, 0, 0}}, 2};
    timings[unsigned(NpcAnimation::PullDocument)] = {{{150, 150, 150, 150}}, 4};
    timings[unsigned(NpcAnimation::ExplosiveRage)] = {{{120, 100, 120, 160}}, 4};
    GameSession game(map, rules, world, 5, pose, false, {timings});
    CommandController controls(game, {timings});
    NpcAnimator animator(game.actors(), {timings});
    auto c = catalog();
    auto& response = c.characters[0].topics[1].responses[0];
    response.beats.resize(1);
    response.beats[0].text = "One two three four five six seven eight nine ten eleven twelve.";
    prepareDialogueLayout(c, measure, nullptr, 40, 2);
    DialogueHistory history;
    controls.bindDialogue(&c, &history);
    TargetSnapshot visible{};
    visible.world = game.actors().snapshot().world;
    visible.revision = game.actors().snapshot().revision;
    visible.count = 1;
    visible.ids[0] = game.actors().id(0);
    controls.residentVisibility(visible);
    check(controls.startDialogue(visible.ids[0]), "Reaction test starts conversation");
    controls.tickDialogue(.7, true);
    UiInput topic, next;
    topic.pressed[unsigned(UiKey::Two)] = true;
    next.pressed[unsigned(UiKey::DialogueAdvance)] = true;
    const auto sample = [&] {
        animator.setTime(0, nullptr, controls.dialogueSpeaker(), controls.dialoguePlayer().clockMs(),
                         &controls.dialogueAnimation());
    };
    for (auto clip : {NpcAnimation::PullDocument, NpcAnimation::ExplosiveRage}) {
        response.beats[0].animation = clip;
        routeHudInput(controls, topic);
        sample();
        check(animator.snapshot().frames[0].clip == clip, "Authored clip starts with line");
        controls.tickDialogue(.16, true);
        sample();
        check(animator.snapshot().frames[0].active == 1, "Authored clip crosses frame boundary");
        routeHudInput(controls, next);
        sample();
        check(animator.snapshot().frames[0].clip == NpcAnimation::Idle, "Click reveal ends authored clip");
        controls.tickDialogue(.7, true);
        routeHudInput(controls, next); // next page, same beat clock
        sample();
        check(animator.snapshot().frames[0].clip == clip && animator.snapshot().frames[0].active == 3,
              "Pagination resumes reaction clock and document stays on last frame");
        controls.tickDialogue(1, true);
        sample();
        check(animator.snapshot().frames[0].clip == NpcAnimation::Idle, "Natural reveal ends authored clip");
    }
    for (auto clip : {NpcAnimation::PullDocument, NpcAnimation::ExplosiveRage}) {
        NpcDialogueOverride override{game.actors().id(0), clip, 0};
        for (auto ms : {0u, 149u, 150u, 599u, 600u, 1000u, 60000u}) {
            override.elapsedMs = ms;
            animator.setTime(0, nullptr, override.actor, ms, &override);
            const auto f = animator.snapshot().frames[0];
            if (clip == NpcAnimation::PullDocument)
                check(f.active == std::min(ms / 150, 3u), "Document plays once and holds at long times");
            else if (ms == 1000 || ms == 60000)
                check(f.active == 0, "Rage loops at 500 ms");
        }
    }
    check(game.actors().snapshot().actors[0].npc.animation == NpcAnimation::Idle,
          "Authored reactions do not mutate authoritative actor state");
    controls.finishDialogue();
}

void inputEdges() {
    auto c = catalog();
    DialogueHistory history;
    DialoguePlayer p;
    p.bind(&c, &history);
    p.start(c.characters[0].id);
    DialogueHud h;
    p.snapshot(h);
    HudSnapshot hud;
    hud.dialogue = h;
    ButtonPress buttons;
    UiInputEdges edges;
    std::array<bool, uiKeyCount> down{};
    const ArtPoint at{240, 345};
    const auto targetAt = [](const ButtonTargets& targets, ArtPoint point) -> const ButtonTarget* {
        for (unsigned i = 0; i < targets.count; ++i)
            if (targets.entries[i].rect.contains(point))
                return &targets.entries[i];
        return nullptr;
    };
    check(!targetAt(hudButtonTargets(hud), {20, 265}), "Speech body is not a button target");
    const auto targets = hudButtonTargets(hud);
    check(targetAt(targets, at) && targetAt(targets, at)->enabled, "Multipage introduction has enabled Next");
    hud.dialogue.pages = 1;
    check(!targetAt(hudButtonTargets(hud), at), "Single-page response has no Next target");
    hud.dialogue = h;
    buttons.route(edges.sample(down, true, at), hudButtonTargets(hud));
    down[unsigned(UiKey::Click)] = true;
    auto event = buttons.route(edges.sample(down, true, at), hudButtonTargets(hud));
    check(!event.has(UiKey::Click), "Dialogue click captured until release");
    p.advance();
    p.snapshot(hud.dialogue);
    down[unsigned(UiKey::Click)] = false;
    event = buttons.route(edges.sample(down, true, at), hudButtonTargets(hud));
    check(!event.has(UiKey::Click), "Reveal context change cancels pending release");
    down[unsigned(UiKey::DialogueAdvance)] = true;
    check(edges.sample(down, true).has(UiKey::DialogueAdvance), "Space uses edge");
    check(!edges.sample(down, true).has(UiKey::DialogueAdvance), "Held Space cannot skip multiple pages");
    edges.sample(down, false);
    check(!edges.sample(down, true).has(UiKey::DialogueAdvance), "Focus regain cannot reveal/advance");
}
} // namespace
int main() {
    try {
        overlays();
        delayedOverlays();
        authoredReactions();
        parser();
        flow();
        integration();
        inputEdges();
        yamlBeats();
        multiNpc();
        std::cout
            << "Dialogue content, history, interaction, animation, camera and allocation checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
