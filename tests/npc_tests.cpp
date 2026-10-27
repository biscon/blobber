#include "assets/image.hpp"
#include "assets/npc_loader.hpp"
#include "assets/npc_sheet.hpp"
#include "support/actor_fixtures.hpp"
#include "support/check.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <nlohmann/json.hpp>

using namespace blobber;
namespace fs = std::filesystem;
using Json = nlohmann::json;
namespace {
Map openMap() {
    return Map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                "#.......#", "#########"});
}
void testWorldAndCollision() {
    const Map map = openMap();
    for (int sy = 0; sy < 2; ++sy)
        for (int sx = 0; sx < 2; ++sx)
            for (int h = 0; h < 4; ++h)
                for (int a = 0; a < 4; ++a) {
                    const Player origin{{4, 4}, static_cast<Heading>(h)};
                    Player destination = origin;
                    check(tryAction(map, destination, static_cast<Action>(a)), "Fixture is walkable");
                    auto npcs = testActors(map, origin.cell, {"test"},
                                           {{{0}, destination.cell, {sx, sy}, Heading::North}});
                    Player blocked = origin;
                    check(!tryAction(map, blocked, static_cast<Action>(a), &npcs) && blocked == origin,
                          "Every occupied subcell blocks all movement directions/headings");
                    Controller c(origin);
                    c.submit(map, static_cast<Action>(a), true, &npcs);
                    check(c.state().blocked && c.state().revision == 0 && !c.state().transition.active(),
                          "NPC collision preserves pose/revision and reports blocked");
                    check(tryAction(map, blocked, Action::TurnLeft, &npcs), "NPCs do not block turning");
                }
    const std::vector<NpcInstance> four{{{0}, {4, 4}, {0, 0}, Heading::North},
                                        {{0}, {4, 4}, {1, 0}, Heading::East},
                                        {{0}, {4, 4}, {0, 1}, Heading::South},
                                        {{0}, {4, 4}, {1, 1}, Heading::West}};
    auto npcs = testActors(map, {1, 1}, {"friendly-or-hostile"}, four);
    check(npcs.snapshot().count == 4 && npcs.occupied({4, 4}) && !npcs.occupied({3, 4}) &&
              !npcs.occupied({-1, 4}) && !npcs.occupied({9, 9}),
          "Four slots and bounded occupancy queries");
    const NpcWorld placements(map, {1, 1}, {"friendly-or-hostile"}, four);
    check(placements.typeId({0}) == "friendly-or-hostile", "String type IDs remain accessible");
    rejects([&] { placements.typeId({1}); }, "Invalid typed index rejected");
    auto invalid = four;
    invalid.push_back(four[0]);
    rejects([&] { NpcWorld w(map, {1, 1}, {"test"}, invalid); }, "Fifth occupant/duplicate slot rejected");
    for (NpcInstance bad : std::vector<NpcInstance>{{{1}, {4, 4}, {0, 0}, Heading::North},
                                                    {{0}, {4, 4}, {2, 0}, Heading::North},
                                                    {{0}, {0, 0}, {0, 0}, Heading::North},
                                                    {{0}, {1, 1}, {0, 0}, Heading::North},
                                                    {{0}, {4, 4}, {0, 0}, static_cast<Heading>(4)}})
        rejects([&] { NpcWorld w(map, {1, 1}, {"test"}, {bad}); }, "Invalid placement rejected");
    rejects([&] { NpcWorld w(map, {1, 1}, {"same", "same"}, {}); }, "Duplicate type IDs rejected");
    rejects([&] { NpcWorld w(map, {1, 1}, {""}, {}); }, "Empty type ID rejected");
    rejects([&] { NpcWorld w(map, {1, 1}, std::vector<std::string>(33, "test"), {}); }, "Type capacity");
    rejects([&] { NpcWorld w(map, {1, 1}, {"test"}, std::vector<NpcInstance>(257)); }, "Instance capacity");
    Controller c({{4, 6}, Heading::North});
    c.submit(map, Action::Forward, true, &npcs);
    c.submit(map, Action::Forward, true, &npcs);
    c.update(map, 1, &npcs);
    check(c.state().player.cell == Cell{4, 5} && c.state().blocked && c.state().revision == 1,
          "Queued commands check NPC occupancy at execution");
    c.setAnimated(false);
    c.submit(map, Action::Forward, true, &npcs);
    check(c.state().blocked && c.state().player.cell == Cell{4, 5}, "Instant mode also blocks NPC entry");
}
void testLoading() {
    const auto directory =
        fs::temp_directory_path() /
        ("blobber-npc-tests-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(directory / "type");
    struct Cleanup {
        fs::path path;
        ~Cleanup() {
            std::error_code e;
            fs::remove_all(path, e);
        }
    } cleanup{directory};
    SetTraceLogLevel(LOG_WARNING);
    OwnedImage image(GenImageColor(8, 12, Color{20, 80, 160, 255}));
    check(ExportImage(image.value, (directory / "type/sprite.png").string().c_str()),
          "Export own NPC fixture");
    const Map map = openMap();
    const Json definition{
        {"id", "test"},
        {"width", 0.45},
        {"height", 0.65},
        {"views",
         {{"front", "sprite.png"}, {"back", "sprite.png"}, {"left", "sprite.png"}, {"right", "sprite.png"}}}};
    const Json level{
        {"definitions", {"type/test.json"}},
        {"instances",
         Json::array({{{"type", "test"}, {"cell", {4, 4}}, {"subcell", {1, 1}}, {"facing", "south"}}})}};
    auto write = [](const fs::path& path, const Json& value) { std::ofstream(path) << value.dump(); };
    auto load = [&] {
        auto result = loadNpcs(directory / "level.json", map, {1, 1});
        return result;
    };
    write(directory / "type/test.json", definition);
    write(directory / "level.json", level);
    auto loaded = load();
    check(loaded.world.occupied({4, 4}) && loaded.world.instances()[0].facing == Heading::South &&
              loaded.appearances[0].views[0].pixels[0].b == 160,
          "Relative definition/image paths and decoded pixels");
    check(loaded.world.instances()[0].ai.routine == NpcRoutine::Guard, "Legacy placements default to guard");
    auto attached = definition;
    attached["attachments"]["release"] = {
        {"front", {.6, .3}}, {"back", {.4, .3}}, {"left", {.1, .4}}, {"right", {.9, .4}}};
    write(directory / "type/test.json", attached);
    const auto metadata = loadNpcMetadata(directory / "level.json", map, {1, 1});
    check(metadata.attachments[0].release[2].x == .1f &&
              metadata.definitionFingerprint != loaded.definitionFingerprint,
          "Source-free directional attachments are fingerprinted");
    for (const Json& badPoint :
         {Json::array({-1, 0}), Json::array({2, 0}), Json::array({0}), Json::array({"x", 0})}) {
        auto bad = attached;
        bad["attachments"]["release"]["front"] = badPoint;
        write(directory / "type/test.json", bad);
        rejects([&] { loadNpcMetadata(directory / "level.json", map, {1, 1}); },
                "Malformed attachment rejected without decoding");
    }
    write(directory / "type/test.json", definition);
    auto patrolLevel = level;
    patrolLevel["instances"][0]["ai"] = {{"routine", "patrol"}, {"waypoints", {{4, 5}, {6, 5}}}};
    write(directory / "level.json", patrolLevel);
    check(load().world.instances()[0].ai.waypointCount == 2, "Patrol settings survive JSON loading");
    for (const auto& badAi : std::vector<Json>{
             {{"routine", "unknown"}},
             {{"routine", "patrol"}},
             {{"routine", "guard"}, {"waypoints", {{4, 5}}}},
             {{"routine", "patrol"}, {"waypoints", {{0, 0}}}},
             {{"routine", "patrol"},
              {"waypoints",
               Json::array({{4, 5}, {4, 5}, {4, 5}, {4, 5}, {4, 5}, {4, 5}, {4, 5}, {4, 5}, {4, 5}})}},
             Json::array({"wander"})}) {
        auto bad = level;
        bad["instances"][0]["ai"] = badAi;
        write(directory / "level.json", bad);
        rejects(load, "Malformed or oversized AI settings rejected during preparation");
    }
    Json empty{{"definitions", Json::array()}, {"instances", Json::array()}};
    write(directory / "level.json", empty);
    check(load().world.instances().empty(), "Explicit empty NPC level accepted");
    for (int failure = 0; failure < 13; ++failure) {
        auto bad = level;
        auto& p = bad["instances"][0];
        switch (failure) {
        case 0:
            p["type"] = "missing";
            break;
        case 1:
            p["cell"] = {0, 0};
            break;
        case 2:
            p["cell"] = {1, 1};
            break;
        case 3:
            p["cell"] = {4.5, 4};
            break;
        case 4:
            p["subcell"] = {0, 2};
            break;
        case 5:
            p["facing"] = "up";
            break;
        case 6:
            bad["instances"].push_back(p);
            break;
        case 7:
            bad["definitions"].push_back("type/test.json");
            break;
        case 8:
            p["cell"] = {18446744073709551615ull, 4};
            break;
        case 9:
            p["cell"] = {4};
            break;
        case 10:
            bad["instances"] = Json::object();
            break;
        case 11:
            bad["instances"] = Json::array();
            for (int i = 0; i < 257; ++i)
                bad["instances"].push_back(level["instances"][0]);
            break;
        case 12:
            bad["definitions"] = Json::array();
            for (int i = 0; i < 33; ++i)
                bad["definitions"].push_back("type/test.json");
            break;
        }
        write(directory / "level.json", bad);
        rejects(load, "Invalid level JSON rejected");
    }
    write(directory / "level.json", level);
    for (int failure = 0; failure < 10; ++failure) {
        auto bad = definition;
        switch (failure) {
        case 0:
            bad["views"]["front"] = "missing.png";
            break;
        case 1:
            bad["views"].erase("left");
            break;
        case 2:
            bad["width"] = 0;
            break;
        case 3:
            bad["height"] = 1.1;
            break;
        case 4:
            bad["id"] = "";
            break;
        case 5:
            bad["views"]["front"] = {{"image", "sprite.png"}, {"anchor", {0.5, 0.5}}};
            break;
        case 6:
            bad["views"]["front"] = {{"image", "sprite.png"}, {"anchor", {0.5, 1.1}}};
            break;
        case 7:
            bad["alpha_cutoff"] = 255;
            break;
        case 8:
            bad["height"] = "tall";
            break;
        case 9:
            bad["views"]["front"] = "bad.png";
            std::ofstream(directory / "type/bad.png") << "broken";
            break;
        }
        write(directory / "type/test.json", bad);
        rejects(load, "Invalid NPC definition/image rejected");
    }
    auto animated = definition;
    animated.erase("views");
    Json clip{{"durations_ms", {60, 60}}, {"views", Json::object()}};
    for (const char* side : {"front", "back", "left", "right"})
        clip["views"][side] = Json::array({"sprite.png", {{"image", "sprite.png"}, {"anchor", {0.5, 1}}}});
    animated["animations"] = {{"idle", clip}, {"walk", clip}, {"attack", clip}};
    for (const char* side : {"back", "left", "right"})
        animated["animations"]["attack"]["views"].erase(side);
    auto animatedLevel = level;
    animatedLevel["instances"][0]["animation"] = "attack";
    animatedLevel["instances"][0]["phase_ms"] = 60;
    write(directory / "type/test.json", animated);
    write(directory / "level.json", animatedLevel);
    auto clips = load();
    check(clips.world.instances()[0].animation == NpcAnimation::Attack &&
              clips.world.instances()[0].phaseMs == 60 &&
              clips.appearances[0].animations[0].views[0].size() == 2,
          "Animated JSON loads clips, images, anchors and placement timing");
    for (int failure = 0; failure < 16; ++failure) {
        auto bad = animated;
        auto badLevel = animatedLevel;
        auto& idle = bad["animations"]["idle"];
        switch (failure) {
        case 0:
            bad["views"] = definition["views"];
            break;
        case 1:
            bad["animations"].erase("idle");
            break;
        case 2:
            idle["durations_ms"] = Json::array();
            break;
        case 3:
            idle["durations_ms"] = {1, 1, 1, 1, 1};
            break;
        case 4:
            idle["durations_ms"][0] = 0;
            break;
        case 5:
            idle["durations_ms"][0] = -1;
            break;
        case 6:
            idle["durations_ms"][0] = 60001;
            break;
        case 7:
            idle["durations_ms"][0] = 1.5;
            break;
        case 8:
            idle["views"]["left"] = {"sprite.png"};
            break;
        case 9:
            bad["animations"]["walk"]["views"].erase("back");
            break;
        case 10:
            bad["animations"]["attack"]["views"].erase("front");
            break;
        case 11:
            bad["animations"]["dance"] = clip;
            break;
        case 12:
            badLevel["instances"][0]["animation"] = "dance";
            break;
        case 13:
            badLevel["instances"][0]["phase_ms"] = -1;
            break;
        case 14:
            bad["animations"].erase("attack");
            break;
        case 15:
            idle["views"]["front"][1] = "missing.png";
            break;
        }
        write(directory / "type/test.json", bad);
        write(directory / "level.json", badLevel);
        rejects(load, "Invalid animation schema rejected");
    }
    animated["animations"]["hurt"] = clip;
    animated["animations"]["death"] = clip;
    animated["animations"]["death"]["width"] = 0.9;
    write(directory / "type/test.json", animated);
    write(directory / "level.json", level);
    check(load().appearances[0].animations[4].width == 0.9f, "Reaction canvas width loads independently");
    for (int failure = 0; failure < 8; ++failure) {
        auto bad = animated;
        auto badLevel = level;
        if (failure < 2)
            bad["animations"][failure ? "hurt" : "death"]["views"].erase("back");
        if (failure == 2)
            bad["animations"]["hurt"]["width"] = 0.6;
        if (failure == 3)
            bad["animations"]["death"]["width"] = 1.01;
        if (failure == 4)
            bad["animations"]["death"]["width"] = 0;
        if (failure == 5)
            bad["animations"]["death"]["width"] = -1;
        if (failure >= 6)
            badLevel["instances"][0]["animation"] = failure == 6 ? "hurt" : "death";
        write(directory / "type/test.json", bad);
        write(directory / "level.json", badLevel);
        rejects(load, "Invalid reaction dimensions/directions/placement rejected");
    }
    animated["animations"]["ranged_attack"] = clip;
    animated["animations"]["ranged_attack"]["width"] = 0.6;
    write(directory / "type/test.json", animated);
    write(directory / "level.json", level);
    check(
        load().appearances[0].animations[static_cast<std::size_t>(NpcAnimation::RangedAttack)].timing.count ==
            2,
        "Optional four-view bow clip loads separately from melee");
    for (const char* side : {"front", "back", "left", "right"}) {
        auto bad = animated;
        bad["animations"]["ranged_attack"]["views"].erase(side);
        write(directory / "type/test.json", bad);
        rejects(load, "Present bow clips require every view");
    }
    write(directory / "type/test.json", animated);
    auto placedBow = level;
    placedBow["instances"][0]["animation"] = "ranged_attack";
    write(directory / "level.json", placedBow);
    rejects(load, "Bow attack is a runtime cue, not a looping placement clip");
    // The production offline preparer uses generated sheets in an isolated directory.
    fs::create_directories(directory / "sheets");
    OwnedImage sheet(GenImageColor(16, 12, BLANK));
    ImageDrawRectangle(&sheet.value, 3, 6, 2, 4, Color{30, 60, 90, 200});
    ImageDrawRectangle(&sheet.value, 11, 3, 2, 7, Color{30, 60, 90, 200});
    check(ExportImage(sheet.value, (directory / "sheets/test.png").string().c_str()), "Export own sheet");
    Json registration = Json::array({{{"name", "test"},
                                      {"split_x", 8},
                                      {"scale", 1.0},
                                      {"pivot_x", {4, 4}},
                                      {"ground_y", {10, 10}},
                                      {"canvas_width", 16},
                                      {"canvas_height", 12},
                                      {"anchor_x", 8},
                                      {"anchor_y", 10}}});
    auto prepare = [&] { prepareNpcAnimation(directory / "sheets.json"); };
    write(directory / "sheets.json", registration);
    prepare();
    OwnedImage shortPose(LoadImage((directory / "frames/test-0.png").string().c_str()));
    OwnedImage tallPose(LoadImage((directory / "frames/test-1.png").string().c_str()));
    check(shortPose.value.width == 16 && shortPose.value.height == 12 &&
              GetImageColor(shortPose.value, 7, 9).a == 200 && GetImageColor(tallPose.value, 7, 9).a == 200 &&
              GetImageColor(shortPose.value, 7, 3).a == 0 && GetImageColor(tallPose.value, 7, 3).a == 200 &&
              GetImageColor(shortPose.value, 7, 10).a == 0,
          "Registration preserves alpha, ground, pose height differences and horizontal padding");
    for (int failure = 0; failure < 9; ++failure) {
        auto bad = registration;
        auto& entry = bad[0];
        if (failure == 0)
            entry["ground_y"] = {9, 10};
        if (failure == 1)
            entry["ground_y"] = {10};
        if (failure == 2)
            entry["canvas_width"] = 0;
        if (failure == 3)
            entry["canvas_width"] = 2049;
        if (failure == 4)
            entry["anchor_x"] = 0;
        if (failure == 5)
            entry["scale"] = 0;
        if (failure == 6)
            entry["split_x"] = 16;
        if (failure == 7)
            entry["pivot_x"] = {9, 4};
        if (failure == 8)
            entry["rows"] = std::numeric_limits<int>::max();
        write(directory / "sheets.json", bad);
        rejects(prepare, "Malformed registration or visible clipping rejected");
    }
    // Unequal sheet rows preserve complete poses instead of cutting at half-height.
    OwnedImage uneven(GenImageColor(16, 27, BLANK));
    for (const int x : {3, 11}) {
        ImageDrawRectangle(&uneven.value, x, 3, 2, 11, Color{30, 60, 90, 200});
        ImageDrawRectangle(&uneven.value, x, 21, 2, 4, Color{30, 60, 90, 200});
    }
    ExportImage(uneven.value, (directory / "sheets/test.png").string().c_str());
    auto unevenRegistration = registration;
    unevenRegistration[0]["rows"] = 2;
    unevenRegistration[0]["split_y"] = 16;
    unevenRegistration[0]["ground_y"] = {14, 14, 9, 9};
    unevenRegistration[0]["pivot_x"] = {4, 4, 4, 4};
    unevenRegistration[0]["canvas_height"] = 16;
    unevenRegistration[0]["anchor_y"] = 14;
    write(directory / "sheets.json", unevenRegistration);
    prepare();
    OwnedImage upper(LoadImage((directory / "frames/test-0.png").string().c_str()));
    OwnedImage lower(LoadImage((directory / "frames/test-2.png").string().c_str()));
    check(GetImageColor(upper.value, 7, 3).a == 200 && GetImageColor(lower.value, 7, 3).a == 0 &&
              GetImageColor(upper.value, 7, 13).a == 200 && GetImageColor(lower.value, 7, 13).a == 200,
          "Unequal row extraction retains whole silhouettes and common ground");
    unevenRegistration[0]["split_y"] = 27;
    write(directory / "sheets.json", unevenRegistration);
    rejects(prepare, "Invalid row boundary rejected");
    ExportImage(sheet.value, (directory / "sheets/test.png").string().c_str());
    registration[0].erase("ground_y");
    registration[0].erase("canvas_width");
    registration[0].erase("canvas_height");
    registration[0].erase("anchor_x");
    registration[0].erase("anchor_y");
    write(directory / "sheets.json", registration);
    prepare();
    OwnedImage legacy(LoadImage((directory / "frames/test-0.png").string().c_str()));
    check(legacy.value.width == 384 && legacy.value.height == 536 &&
              GetImageColor(legacy.value, 191, 511).a == 200,
          "Existing extraction defaults retain canvas and lowest-ink registration");
    ImageClearBackground(&sheet.value, BLANK);
    ExportImage(sheet.value, (directory / "sheets/test.png").string().c_str());
    rejects(prepare, "Empty sheet frame rejected");
    write(directory / "type/test.json", definition);
    std::ofstream(directory / "level.json") << "{";
    rejects(load, "Malformed JSON rejected");
    std::ofstream(directory / "level.json") << std::string(1024 * 1024 + 1, ' ');
    rejects(load, "Oversize JSON rejected before parsing");
    rejects([&] { loadNpcs(directory / "absent.json", map, {1, 1}); }, "Missing level rejected");
}
std::array<float, viewWidth> noWalls() {
    std::array<float, viewWidth> result;
    result.fill(std::numeric_limits<float>::infinity());
    return result;
}
void testViewsAndPreparation() {
    for (int h = 0; h < 4; ++h) {
        const auto facing = static_cast<Heading>(h);
        const Cell f = forward(facing), r = right(facing);
        check(npcSide(facing, float(f.x), float(f.y)) == NpcSide::Front, "Front selection");
        check(npcSide(facing, float(-f.x), float(-f.y)) == NpcSide::Back, "Back selection");
        check(npcSide(facing, float(r.x), float(r.y)) == NpcSide::Right, "Anatomical right selection");
        check(npcSide(facing, float(-r.x), float(-r.y)) == NpcSide::Left, "Anatomical left selection");
        check(npcSide(facing, float(f.x + r.x), float(f.y + r.y)) == NpcSide::Front,
              "Exact diagonal front tie");
        check(npcSide(facing, float(-f.x + r.x), float(-f.y + r.y)) == NpcSide::Back,
              "Exact diagonal back tie");
        check(npcSide(facing, f.x + r.x * 1.01f, f.y + r.y * 1.01f) == NpcSide::Right,
              "Crossing diagonal changes sector");
    }
    auto invalid = testNpcAppearance();
    invalid.views[0].pixels.pop_back();
    rejects([&] { PreparedNpcs p({invalid}); }, "Invalid decoded image rejected");
    invalid = testNpcAppearance();
    invalid.width = std::numeric_limits<float>::infinity();
    rejects([&] { PreparedNpcs p({invalid}); }, "Non-finite sprite dimensions rejected");
    invalid = testNpcAppearance();
    invalid.views[0].anchorX = -0.1f;
    rejects([&] { PreparedNpcs p({invalid}); }, "Invalid image anchor rejected");
    invalid = testNpcAppearance();
    invalid.views[0].width = 2049;
    rejects([&] { PreparedNpcs p({invalid}); }, "Image axis bound rejected");
    invalid = testNpcAppearance();
    invalid.width = 0.5f;
    invalid.height = 1;
    // Inject a small budget instead of allocating many production-sized banks.
    rejects([&] { PreparedNpcs p({invalid}, {1024, 512u * 1024u * 1024u}); },
            "Prepared pixel budget exhaustion");
    const Map map = openMap();
    const auto appearance = testNpcAppearance();
    const PreparedNpcs prepared({appearance});
    auto npc = testActors(map, {1, 1}, {"test"}, {{{0}, {4, 4}, {0, 0}, Heading::South}});
    const std::array<Player, 4> cameras{{{{4, 6}, Heading::North},
                                         {{4, 2}, Heading::South},
                                         {{6, 4}, Heading::West},
                                         {{2, 4}, Heading::East}}};
    for (int view = 0; view < 4; ++view) {
        Frame frame({0, 0, 0, 255});
        prepared.compose(npc, cameras[view], 0, noWalls(), frame);
        bool found = false;
        for (Color c : frame)
            if (c.r || c.g || c.b) {
                found = true;
                if (view == 0)
                    check(c.r > c.g && c.g > c.b, "Front artwork selected from world position");
                if (view == 1)
                    check(c.g > c.b && c.b > c.r, "Back artwork selected from world position");
                if (view == 2)
                    check(c.b > c.r && c.r > c.g, "Left artwork selected from world position");
                if (view == 3)
                    check(c.r > c.g && c.g > c.b && c.g > c.r / 2,
                          "Right artwork selected from world position");
            }
        check(found, "NPC visible from all four sides");
    }
    Frame near({0, 0, 0, 255}), far({0, 0, 0, 255});
    prepared.compose(npc, {{4, 5}, Heading::North}, 0, noWalls(), near);
    prepared.compose(npc, {{4, 7}, Heading::North}, 0, noWalls(), far);
    auto bounds = [](const Frame& frame) {
        std::array<int, 4> b{viewWidth, viewHeight, -1, -1};
        for (int y = 0; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                const auto c = frame[y * viewWidth + x];
                if (c.r || c.g || c.b) {
                    b[0] = std::min(b[0], x);
                    b[1] = std::min(b[1], y);
                    b[2] = std::max(b[2], x);
                    b[3] = std::max(b[3], y);
                }
            }
        return b;
    };
    const auto n = bounds(near), d = bounds(far);
    check(n[2] - n[0] > 2 * (d[2] - d[0]) && n[3] - n[1] > 2 * (d[3] - d[1]), "NPC grows with inverse depth");
    const float ground = ViewProjection::horizon + ViewProjection::focalLength() * 0.5f / 1.65f;
    check(std::abs((n[3] + 1) - ground) < 1, "Feet project to floor at subcell depth");
    auto empty = testActors(map, {1, 1}, {}, {});
    rejects([&] { prepared.compose(empty, cameras[0], 0, noWalls(), near); },
            "Mismatched world/cache rejected");
}
// Independent ray/block slab intersection; no prepared-wall implementation reused.
float wallDepth(const Map& map, Player p, int column, float phase) {
    const Cell f = forward(p.heading), r = right(p.heading);
    const float slope = (column + 0.5f - viewWidth * 0.5f) / ViewProjection::focalLength();
    const float vx = f.x + r.x * slope, vy = f.y + r.y * slope;
    const float ox = p.cell.x + 0.5f - f.x * 0.4f + r.x * phase,
                oy = p.cell.y + 0.5f - f.y * 0.4f + r.y * phase;
    float nearest = 1000;
    for (int y = 0; y < map.height(); ++y)
        for (int x = 0; x < map.width(); ++x)
            if (map.solid({x, y})) {
                float enter = 0, leave = 1000;
                auto slab = [&](float o, float v, float lo) {
                    if (std::abs(v) < 1e-6f)
                        return o >= lo && o <= lo + 1;
                    float a = (lo - o) / v, b = (lo + 1 - o) / v;
                    if (a > b)
                        std::swap(a, b);
                    enter = std::max(enter, a);
                    leave = std::min(leave, b);
                    return enter < leave;
                };
                if (slab(ox, vx, float(x)) && slab(oy, vy, float(y)))
                    nearest = std::min(nearest, enter);
            }
    return nearest;
}
void testOcclusionAndTransitions() {
    const Map map({"#########", "#.......#", "#.......#", "#.......#", "#...#...#", "#.......#", "#.......#",
                   "#.......#", "#########"});
    auto appearance = testAnimatedNpcAppearance();
    const SceneRenderer renderer(testMaterials(), {appearance});
    const PreparedNpcs prepared({appearance});
    Frame baseline, actual, expected, reverse;
    int hidden = 0, visible = 0, partial = 0;
    for (int h = 0; h < 4; ++h) {
        const Heading heading = static_cast<Heading>(h);
        const Cell f = forward(heading), r = right(heading);
        const Player camera{Cell{4, 4} + f * -2, heading};
        const Cell npcCell = Cell{4, 4} + f;
        for (int sub = 0; sub < 4; ++sub) {
            auto npc =
                testActors(map, {1, 1}, {"test"}, {{{0}, npcCell, {sub % 2, sub / 2}, Heading::South}});
            NpcAnimator animator(npc, {npcTimings(appearance)});
            for (int phase = 0; phase < 8; ++phase) {
                animator.setTime(phase % 2 ? 60 : 0);
                const float offset = phase / 8.0f;
                // composeStrafe takes eased progress; invert it numerically for each exact cache phase.
                float lo = 0, hi = 1;
                for (int i = 0; i < 24; ++i) {
                    const float t = (lo + hi) * 0.5f;
                    if (t * t * (3 - 2 * t) < offset)
                        lo = t;
                    else
                        hi = t;
                }
                const float t = (lo + hi) * 0.5f;
                const Player end{camera.cell + r, heading};
                renderer.composeStrafe(map, camera, end, t, baseline);
                expected.copyFrom(baseline);
                std::array<float, viewWidth> walls;
                for (int x = 0; x < viewWidth; ++x)
                    walls[x] = wallDepth(map, camera, x, offset);
                prepared.compose(npc, camera, phase, walls, expected, &animator.snapshot());
                renderer.composeStrafe(map, camera, end, t, actual, &npc, &animator.snapshot());
                check(same(actual, expected), "NPC wall occlusion agrees with independent slab geometry");
                renderer.composeStrafe(map, end, camera, 1 - t, reverse, &npc, &animator.snapshot());
                check(same(actual, reverse), "NPC strafes retrace identical views");
                int changed = 0;
                for (std::size_t i = 0; i < actual.size(); ++i) {
                    const Color a = actual[i], b = baseline[i];
                    changed += (a.r != b.r || a.g != b.g || a.b != b.b);
                    check(a.a == 255, "NPC composition preserves opaque frame");
                }
                if (changed)
                    ++visible;
                else
                    ++hidden;
                Frame unobstructed;
                unobstructed.copyFrom(baseline);
                prepared.compose(npc, camera, phase, noWalls(), unobstructed, &animator.snapshot());
                if (changed && !same(actual, unobstructed))
                    ++partial;
            }
        }
    }
    check(hidden > 0 && visible > 0 && partial > 0,
          "Fixture exercises full, partial, and visible pillar cases");
    auto npc = testActors(map, {1, 1}, {"test"}, {{{0}, {3, 3}, {1, 1}, Heading::South}});
    const Player from{{3, 5}, Heading::North}, to{{2, 5}, Heading::North};
    renderer.compose(map, from, expected, &npc);
    renderer.composeStrafe(map, from, to, 0, actual, &npc);
    check(same(actual, expected), "NPC strafe source exact");
    renderer.compose(map, to, expected, &npc);
    renderer.composeStrafe(map, from, to, 1, actual, &npc);
    check(same(actual, expected), "NPC strafe destination exact");
}
void testLayersAndClipping() {
    const Map map = openMap();
    auto front = testNpcAppearance(), back = front;
    for (auto& view : front.views)
        for (int y = 0; y < view.height; ++y)
            view.pixels[y * view.width + 7].a = 128;
    for (auto& view : back.views)
        for (auto& p : view.pixels)
            p = {0, 240, 0, 255};
    const PreparedNpcs prepared({front, back});
    const NpcInstance near{{0}, {4, 4}, {0, 1}, Heading::South}, far{{1}, {4, 4}, {0, 0}, Heading::South};
    auto both = testActors(map, {1, 1}, {"near", "far"}, {near, far});
    auto reversed = testActors(map, {1, 1}, {"near", "far"}, {far, near});
    auto onlyNear = testActors(map, {1, 1}, {"near", "far"}, {near});
    Frame result({0, 0, 0, 255}), other({0, 0, 0, 255}), n({0, 0, 0, 255});
    const Player p{{4, 6}, Heading::North};
    prepared.compose(both, p, 0, noWalls(), result);
    prepared.compose(reversed, p, 0, noWalls(), other);
    check(same(result, other), "Depth order does not depend on instance order");
    prepared.compose(onlyNear, p, 0, noWalls(), n);
    int holes = 0, blends = 0, opaque = 0;
    // Middle rows lie in both sprites; green in the foreground slit proves holes expose the rear.
    for (int y = 110; y < 140; ++y)
        for (int x = 190; x < 250; ++x) {
            const auto a = result[y * viewWidth + x], b = n[y * viewWidth + x];
            if (a.g > 0 && a.r == 0)
                ++holes;
            if (a.r > 0 && a.g > b.g)
                ++blends;
            if (a.r > 0 && a.r == b.r && a.g == b.g && a.b == b.b)
                ++opaque;
        }
    check(holes > 0 && blends > 0 && opaque > 0, "Transparent, translucent and opaque NPC overlap");
    // Adjacent lateral sprites cross the screen boundary during a strafe; exercise all clips.
    auto side = testActors(map, {1, 1}, {"near", "far"}, {{{0}, {5, 4}, {0, 0}, Heading::West}});
    int clippedPixels = 0, fullPixels = 0;
    for (int phase = 0; phase < 8; ++phase) {
        std::fill(result.begin(), result.end(), Color{0, 0, 0, 255});
        prepared.compose(side, {{4, 4}, Heading::North}, phase, noWalls(), result);
        int pixels = 0;
        for (Color color : result) {
            pixels += color.r || color.g || color.b;
            check(color.a == 255, "Clipped sprite leaves an opaque frame");
        }
        if (phase == 0)
            clippedPixels = pixels;
        if (phase == 7)
            fullPixels = pixels;
    }
    check(clippedPixels > 0 && fullPixels > clippedPixels * 2,
          "Strafe reveals a sprite clipped at the viewport edge");
    rejects([&] { prepared.compose(side, p, 8, noWalls(), result); }, "Invalid strafe phase rejected");
}
void testFrontBandVisibility() {
    const auto map = openMap();
    const auto appearance = testNpcAppearance();
    PreparedNpcs prepared({appearance});
    for (int d = 0; d < 4; ++d) {
        const Player p{{4, 4}, static_cast<Heading>(d)};
        const auto f = forward(p.heading), r = right(p.heading);
        std::vector<NpcInstance> nearRow;
        for (int side = -1; side <= 1; ++side)
            for (int sub = 0; sub < 4; ++sub) {
                NpcInstance instance{{0}, p.cell + f + r * side, {sub % 2, sub / 2}, Heading::South};
                auto actors = testActors(map, p.cell, {"test"}, {instance});
                Frame frame;
                PickingBuffer picking;
                prepared.compose(actors, p, 0, noWalls(), frame, nullptr, &picking);
                picking.finish(&actors);
                check(picking.targets().count == 1,
                      "Every isolated front-band subslot has a visible clickable body");
                const Cell local{sub % 2 * 2 - 1, sub / 2 * 2 - 1};
                if (local.x * f.x + local.y * f.y < 0)
                    nearRow.push_back(instance);
            }
        auto six = testActors(map, p.cell, {"test"}, nearRow);
        Frame frame;
        PickingBuffer picking;
        prepared.compose(six, p, 0, noWalls(), frame, nullptr, &picking);
        picking.finish(&six);
        check(picking.targets().count == 6,
              "Six front-row bodies remain distinct clickable targets without edge stacking");
        auto walls = noWalls();
        walls.fill(.5f);
        Frame hidden;
        PickingBuffer occluded;
        prepared.compose(six, p, 0, walls, hidden, nullptr, &occluded);
        occluded.finish(&six);
        check(occluded.targets().count == 0, "Edge presentation still respects nearer wall occlusion");
        // A retreat from the outer diagonal slot stays pickable throughout the
        // interpolation and settles back into the unmodified farther projection.
        const auto outer = nearRow.front();
        auto walker = testActors(map, p.cell, {"test"}, {outer});
        NpcAnimator animator(walker, {npcTimings(appearance)});
        walker.walk(walker.id(0), outer.cell + f, outer.subcell, p.cell, 0);
        for (std::uint64_t time : {0u, 100u, 300u, 599u, 600u}) {
            animator.setTime(time);
            Frame walking;
            PickingBuffer hit;
            prepared.compose(walker, p, 0, noWalls(), walking, &animator.snapshot(), &hit);
            hit.finish(&walker);
            check(hit.targets().count == 1,
                  "Outer diagonal retreat remains a visible body throughout travel");
        }
    }
}
void testContactShadows() {
    const Map map = openMap();
    auto invisible = testNpcAppearance();
    for (auto& view : invisible.views)
        for (auto& pixel : view.pixels)
            pixel.a = 0;
    // Transparent fixture isolates floor-shadow behavior from the body artwork.
    const PreparedNpcs prepared({invisible});
    auto npc = testActors(map, {1, 1}, {"test"}, {{{0}, {4, 4}, {0, 0}, Heading::South}});
    const Player player{{4, 6}, Heading::North};
    const Color floor{240, 240, 240, 255};
    Frame shadow(floor), hidden(floor), clipped(floor);
    prepared.compose(npc, player, 0, noWalls(), shadow);
    int darkened = 0;
    std::array<bool, 256> shades{};
    const float focal = ViewProjection::focalLength();
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x) {
            const auto c = shadow[y * viewWidth + x];
            check(c.a == 255 && c.r == c.g && c.g == c.b && c.r >= 163,
                  "Blob is neutral, semitransparent and leaves an opaque frame");
            if (c.r == 240)
                continue;
            ++darkened;
            shades[c.r] = true;
            check(y + 0.5f > ViewProjection::horizon, "Shadow only affects the floor");
            const float depth = focal * ViewProjection::eyeHeight / (y + 0.5f - ViewProjection::horizon);
            const float dx = (x + 0.5f - viewWidth * 0.5f) * depth / focal + 0.25f;
            const float dz = depth - 2.65f;
            const float tolerance = depth / focal; // integer screen placement, at most one pixel
            check(std::sqrt(dx * dx + dz * dz) <= invisible.width * 0.45f + tolerance,
                  "Shadow stays within the projected foot contact patch");
        }
    check(darkened > 10 && std::count(shades.begin(), shades.end(), true) > 5,
          "Soft shadow has a graded falloff, not a solid ellipse");
    auto walls = noWalls();
    walls.fill(0);
    prepared.compose(npc, player, 0, walls, hidden);
    check(same(hidden, Frame(floor)), "Walls fully hide the shadow");
    walls.fill(2.65f);
    prepared.compose(npc, player, 0, walls, clipped);
    int visible = 0;
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x)
            if (clipped[y * viewWidth + x].r < 240) {
                ++visible;
                const float depth = focal * ViewProjection::eyeHeight / (y + 0.5f - ViewProjection::horizon);
                check(depth < 2.65f, "Each shadow row uses floor depth for wall clipping");
            }
    check(visible > 0 && visible < darkened, "Wall edge can partially clip a floor blob");
    auto four = testActors(map, {1, 1}, {"test"},
                           {{{0}, {4, 4}, {0, 0}, Heading::South},
                            {{0}, {4, 4}, {1, 0}, Heading::South},
                            {{0}, {4, 4}, {0, 1}, Heading::South},
                            {{0}, {4, 4}, {1, 1}, Heading::South}});
    Frame group(floor);
    prepared.compose(four, player, 0, noWalls(), group);
    for (int sy = 0; sy < 2; ++sy)
        for (int sx = 0; sx < 2; ++sx) {
            const float depth = 2.65f - sy * 0.5f;
            const int x = static_cast<int>(viewWidth * 0.5f + focal * (sx * 0.5f - 0.25f) / depth);
            const int y =
                static_cast<int>(ViewProjection::horizon + focal * ViewProjection::eyeHeight / depth);
            check(group[y * viewWidth + x].r < 220, "Each occupied subcell receives its own shadow");
        }
}
} // namespace
int main() {
    try {
        testWorldAndCollision();
        testLoading();
        testViewsAndPreparation();
        testOcclusionAndTransitions();
        testLayersAndClipping();
        testFrontBandVisibility();
        testContactShadows();
        std::cout << "NPC checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
