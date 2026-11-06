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
        R"({"version":1,"characters":[{"id":"fixture.host","name":"Host","facts":["advice"],"greeting":{"pages":["Welcome back."]},"introduction":{"pages":["Welcome to the inn.","Ask me anything."]},"topics":[{"id":"about","label":"About yourself","kind":"introduction"},{"id":"advice","label":"Advice","responses":[{"pages":["Keep your friends close.","Know when to turn back."],"set_fact":"advice"}]},{"id":"rumours","label":"Rumours","responses":[{"pages":["Ask about advice first."]},{"when_fact":"advice","pages":["Here is another piece of advice."]}]},{"id":"farewell","label":"Farewell","kind":"farewell"}]}]})");
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
    void write(const Json& j) { std::ofstream(path / "dialogue.json") << j.dump(2); }
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
          "Readable JSON loads stable identity");
    auto distanceOnly = j;
    distanceOnly["characters"][0]["camera"] = {{"distance", .7}};
    d.write(distanceOnly);
    check(loadDialogue(d.path).characters[0].eyeHeight == 0,
          "Distance adjustment retains automatic eye line");
    for (unsigned fault = 0; fault < 11; ++fault) {
        auto bad = j;
        if (fault == 0)
            bad["characters"].push_back(bad["characters"][0]);
        if (fault == 1)
            bad["characters"][0]["topics"][1]["responses"][0]["set_fact"] = "missing";
        if (fault == 2)
            bad["characters"][0]["greeting"]["pages"][0] = std::string(1025, 'a');
        if (fault == 3)
            bad["characters"][0]["topics"][1]["responses"][0]["when_fact"] = "advice";
        if (fault == 4)
            bad["characters"][0]["camera"] = {{"distance", 0}};
        if (fault == 5)
            bad["characters"][0]["greeting"]["pages"][0] = "Curly \u2019";
        if (fault == 6)
            bad["characters"][0]["topics"][0]["kind"] = "trade";
        if (fault == 7)
            bad["characters"][0]["greeeting"] = bad["characters"][0]["greeting"];
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
            rejected = std::string(e.what()).find("dialogue.json") != std::string::npos;
        }
        check(rejected, "Malformed dialogue rejects with filename");
    }
    std::ofstream(d.path / "dialogue.json") << "{\"version\":1,\"version\":1}";
    rejects([&] { loadDialogue(d.path); }, "Duplicate JSON keys reject");
    c.characters[0].greeting.pages = {"one two three four five six seven eight nine ten eleven twelve"};
    prepareDialogueLayout(c, measure, nullptr, 40, 2);
    check(c.characters[0].greeting.pages.size() > 1,
          "Long authored pages paginate with metrics, not clipping");
}
void flow() {
    auto c = catalog();
    auto second = c.characters[0];
    second.id = dialogueKey("fixture.other");
    second.key = "fixture.other";
    c.characters.push_back(second);
    DialogueHistory history;
    DialoguePlayer player;
    DialogueHud h;
    player.bind(&c, &history);
    check(player.start(c.characters[0].id), "First meeting opens");
    player.snapshot(h);
    check(h.introduction && h.count == 4 && h.text[0] == 0,
          "Introduction starts automatically, topics immediate");
    player.tick(.15, true);
    player.snapshot(h);
    check(h.text[0] == 0, "Greeting starts after panel fade");
    player.tick(.1, true);
    player.snapshot(h);
    check(std::strlen(h.text.data()) == 4 && h.revealing, "Gradual reveal uses own clock");
    const auto clock = player.clockMs();
    player.tick(1, false);
    check(player.clockMs() == clock, "Focus loss freezes dialogue presentation");
    player.advance();
    player.snapshot(h);
    check(!h.revealing && h.page == 1, "One press reveals but does not advance");
    player.advance();
    player.snapshot(h);
    check(h.revealing && h.page == 2 && h.text[0] == 0, "Next press advances exactly one page");
    player.choose(1);
    player.advance();
    player.choose(2);
    check(!history.get(c.characters[0].id)->knows(dialogueKey("advice")),
          "Interrupting before final page awards no fact");
    player.advance();
    player.snapshot(h);
    check(std::string_view(h.text.data()) == "Ask about advice first.", "Default response before fact");
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
    check(!h.introduction && std::string_view(h.text.data()) == "Welcome back.",
          "Return uses short greeting");
    player.close();
    check(player.start(c.characters[1].id), "Second friendly character opens");
    player.snapshot(h);
    check(h.introduction && !history.get(c.characters[1].id)->knows(dialogueKey("advice")),
          "Same dialogue/art has independent character history");
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
    check(game.actors().actor(game.actors().id(0))->npc.animation == NpcAnimation::Talk,
          "Revealing greeting drives Talk");
    animator.setTime(time, nullptr, controls.dialogueSpeaker(), controls.dialoguePlayer().clockMs());
    const auto before = animator.snapshot().revision;
    controls.tickDialogue(.13, true);
    animator.setTime(time, nullptr, controls.dialogueSpeaker(), controls.dialoguePlayer().clockMs());
    check(animator.snapshot().revision != before,
          "Speaker animates on presentation clock while game time frozen");
    UiInput space;
    space.pressed[unsigned(UiKey::DialogueAdvance)] = true;
    routeHudInput(controls, space);
    check(game.actors().actor(game.actors().id(0))->npc.animation == NpcAnimation::Idle &&
              !controls.snapshot().dialogue.revealing,
          "Skip reveal immediately returns speaker to Idle");
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
    animator.setTime(time, nullptr, controls.dialogueSpeaker(), controls.dialoguePlayer().clockMs());
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
        animator.setTime(time, nullptr, controls.dialogueSpeaker(), controls.dialoguePlayer().clockMs());
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
    const ArtPoint at{20, 265};
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
        parser();
        flow();
        integration();
        inputEdges();
        std::cout
            << "Dialogue content, history, interaction, animation, camera and allocation checks passed\n";
        return 0;
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
