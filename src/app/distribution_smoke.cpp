#include "app/distribution_smoke.hpp"
#include "assets/dialogue.hpp"
#include "audio/bank.hpp"
#include "audio/music.hpp"
#include "persistence/campaign_store.hpp"
#include "platform/audio.hpp"
#include "platform/file_io.hpp"
#include "platform/presenter.hpp"
#include "rlgl.h"
#include "ui/interaction.hpp"
#include "ui/menu_screen.hpp"
#include "world/sanctuary_city.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
namespace blobber {
namespace {
using Json = nlohmann::json;
namespace fs = std::filesystem;
void require(bool condition, const std::string& message) {
    if (!condition)
        throw std::runtime_error("Distribution smoke: " + message);
}
void write(const fs::path& path, const Json& value) {
    std::ofstream out(path);
    out << value.dump(2) << '\n';
    out.close();
    require(bool(out), "cannot write " + path.u8string());
}
Json read(const fs::path& path) {
    std::ifstream in(path);
    return Json::parse(in);
}
void bounded(unsigned& actions) {
    require(++actions <= 4096, "scripted action limit exceeded");
}
void validateAudio(const Options& o) {
    const auto root = o.assets / "audio";
    const auto bank = audio::loadBank(root);
    require(!bank.cues.empty() && bank.warning.empty(), "audio bank: " + bank.warning);
    for (const auto& cue : bank.cues)
        for (const auto& variant : cue.variants)
            require(fs::is_regular_file(root / variant.path), "missing audio " + variant.path);
    for (const auto& suffix : {"music", "music/sanctuary", "music/mainmenu"}) {
        std::string warning;
        const auto tracks = audio::loadMusicTracks(root / suffix, warning);
        require(!tracks.empty() && warning.empty(), "music catalog " + std::string(suffix) + ": " + warning);
    }
    const auto dialogue = loadDialogue(o.assets / "dialogue", root, o.assets / "ui");
    for (const auto& file : dialogue.audioFiles)
        require(fs::is_regular_file(file), "missing dialogue audio " + file);
}
} // namespace
Json distributionSmokeState(const CampaignRun& run) {
    const auto pose = run.game().movement().player;
    Json result{{"run_id", run.runId()},
                {"seed", run.location(0).seed},
                {"location", run.activeLocation()},
                {"pose", {pose.cell.x, pose.cell.y, unsigned(pose.heading)}},
                {"gold", run.game().party().gold()},
                {"quest_keys", run.game().party().questKeys()},
                {"characters", Json::array()},
                {"visited", Json::array()},
                {"inventory", Json::array()}};
    for (const auto& c : run.game().party().snapshot().characters)
        result["characters"].push_back({c.hitPoints, c.temporaryHitPoints, int(c.life), c.mana,
                                        c.availableHitDice, int(c.experience), int(c.build.level)});
    const auto inventory = run.game().party().inventory().checkpoint();
    for (const auto& stack : inventory.stacks)
        if (stack.quantity)
            result["inventory"].push_back({stack.item.index, stack.quantity});
    for (unsigned i = 0; i < campaignLocationCount; ++i)
        result["visited"].push_back(run.location(i).visited);
    return result;
}
void requireDistributionSmokeState(const Json& expected, const CampaignRun& run) {
    require(expected == distributionSmokeState(run), "Continue differs from the saved run summary");
}
void distributionSmokeWalk(GameSession& game, Player target, unsigned& actions) {
    const auto& map = game.geometry().base();
    const auto count = std::size_t(map.width()) * map.height();
    require(count <= 65536, "navigation map capacity exceeded");
    const auto index = [&](Cell c) { return c.y * map.width() + c.x; };
    const auto cell = [&](int i) { return Cell{i % map.width(), i / map.width()}; };
    require(target.cell.x >= 0 && target.cell.x < map.width() && target.cell.y >= 0 &&
                target.cell.y < map.height(),
            "navigation target outside map");
    std::vector<int> parent(count, -1), queue;
    queue.reserve(count);
    const auto start = game.movement().player.cell;
    queue.push_back(index(start));
    parent[std::size_t(index(start))] = index(start);
    for (std::size_t i = 0; i < queue.size(); ++i) {
        const auto here = cell(queue[i]);
        for (unsigned h = 0; h < 4; ++h) {
            const auto next = here + forward(Heading(h));
            if (next.x < 0 || next.x >= map.width() || next.y < 0 || next.y >= map.height() ||
                !map.canStep(here, next) || game.actors().occupied(next) ||
                parent[std::size_t(index(next))] >= 0)
                continue;
            const auto f = game.fixtures().at(next);
            if (f >= 0 && stairFixture(game.fixtures().definition().entries[unsigned(f)].kind))
                continue;
            parent[std::size_t(index(next))] = queue[i];
            queue.push_back(index(next));
        }
    }
    require(parent[std::size_t(index(target.cell))] >= 0, "no legal route to checkpoint");
    std::vector<Cell> path;
    for (auto c = target.cell; !(c == start); c = cell(parent[std::size_t(index(c))]))
        path.push_back(c);
    // Existing discrete/no-animation mode; no teleports or authored-state replacement.
    game.setAnimated(false);
    const auto face = [&](Heading heading) {
        for (unsigned i = 0; i < 4 && game.movement().player.heading != heading; ++i) {
            bounded(actions);
            game.move(Action::TurnRight);
        }
        require(game.movement().player.heading == heading, "turn rejected");
    };
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        const auto delta = *it + game.movement().player.cell * -1;
        face(delta.x > 0   ? Heading::East
             : delta.x < 0 ? Heading::West
             : delta.y > 0 ? Heading::South
                           : Heading::North);
        bounded(actions);
        game.move(Action::Forward);
        require(game.movement().player.cell == *it, "movement rejected");
    }
    face(target.heading);
}
DistributionSmoke::DistributionSmoke(const Options& o) : options_(o) {
    const auto root = o.smokeOutput.parent_path();
    require(root.is_absolute() && root != root.root_path() &&
                fs::is_regular_file(root / ".blobber-smoke-owner.json"),
            "dedicated marked user-data root required");
    require(read(root / ".blobber-smoke-owner.json") == Json{{"version", 1}}, "user-data owner mismatch");
#ifdef _WIN32
    const auto* local = _wgetenv(L"LOCALAPPDATA");
    require(local && fs::path(local).is_absolute(), "isolated LOCALAPPDATA required");
    const auto relative = fs::path(local).lexically_normal().lexically_relative(root.lexically_normal());
    require(!relative.empty() && *relative.begin() != "..", "LOCALAPPDATA leaves user-data root");
#else
    for (const auto* key : {"HOME", "XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_CACHE_HOME"}) {
        const auto* value = std::getenv(key);
        require(value && fs::path(value).is_absolute(), std::string("isolated ") + key + " required");
        const auto relative = fs::path(value).lexically_normal().lexically_relative(root.lexically_normal());
        require(!relative.empty() && *relative.begin() != "..", std::string(key) + " leaves user-data root");
    }
#endif
    if (o.smokePhase == SmokePhase::Create) {
        require(!fs::exists(campaignSavePath()) && !fs::exists(characterLibraryPath()) &&
                    !fs::exists(audio::settingsPath()),
                "create requires fresh user data");
        require(fs::create_directory(o.smokeOutput), "create output already exists");
    } else
        require(fs::is_regular_file(o.smokeOutput / "expected.json"), "missing create summary");
    validateAudio(o);
}
void DistributionSmoke::point() {
    bounded(actions_);
    require(std::chrono::steady_clock::now() - started_ < std::chrono::minutes(10),
            "runtime deadline exceeded");
    require(IsWindowState(FLAG_WINDOW_HIDDEN), "window became visible");
    if (graphics_.is_null()) {
        using GlString = const unsigned char* (*)(unsigned);
        const auto get = reinterpret_cast<GlString>(rlGetProcAddress("glGetString"));
        require(get != nullptr, "OpenGL identity unavailable");
        graphics_ = {{"vendor", reinterpret_cast<const char*>(get(0x1F00))},
                     {"renderer", reinterpret_cast<const char*>(get(0x1F01))},
                     {"version", reinterpret_cast<const char*>(get(0x1F02))}};
#ifdef _WIN32
        graphics_["module"] = disk::loadedModulePath("opengl32.dll").u8string();
#endif
    }
}
void DistributionSmoke::capture(Presenter& presenter, const char* name) {
    presenter.capture(options_.smokeOutput / (std::string(name) + ".png"));
}
int DistributionSmoke::menu(MenuView& menu, Presenter& presenter) {
    point();
    require(!menu.error[0] && menu.usable, std::string("menu failure: ") + menu.error.data());
    if (finished_) {
        require(menu.canContinue, "saved campaign is not available to Continue");
        presenter.audio->persist();
        require(presenter.audio->warning().empty(), presenter.audio->warning());
        return 3;
    }
    require(!launched_, "gameplay returned before completion");
    if (options_.smokePhase == SmokePhase::Continue) {
        require(menu.canContinue, "Continue is disabled");
        launched_ = true;
        return 60;
    }
    switch (menus_++) {
    case 0: {
        require(!menu.canContinue, "unexpected existing campaign");
        capture(presenter, "01-menu");
        ArenaScenarioStore store(menu.catalog, characterLibraryPath().parent_path() / "scenarios");
        auto scenario = defaultArena(menu.catalog);
        require(scenario.adjust(menu.catalog.findMonster("goblin"), 1), "scenario monster unavailable");
        require(store.save(scenario).durable, "scenario save failed");
        return 1;
    }
    case 1:
        return 9; // Normal Use Defaults action.
    case 2:
        require(menu.model.ready(menu.library.snapshot()), "default party is not ready");
        launched_ = true;
        return 30;
    default:
        throw std::runtime_error("Distribution smoke: unexpected menu state");
    }
}
InputSample DistributionSmoke::sample() const {
    return {};
}
bool DistributionSmoke::gameplay(CampaignRun& run, CommandController& commands, Presenter& presenter) {
    point();
    if (options_.smokePhase == SmokePhase::Continue) {
        require(!commands.snapshot().welcomeOpen, "Continue opened the new-campaign welcome");
        requireDistributionSmokeState(read(options_.smokeOutput / "expected.json"), run);
        finished_ = true;
        return true;
    }
    auto& game = run.game();
    switch (stage_++) {
    case 0: {
        require(run.activeLocationId() == CampaignLocationId::City && commands.snapshot().welcomeOpen,
                "missing Sanctuary welcome");
        capture(presenter, "02-welcome");
        UiInput input;
        input.pressed[std::size_t(UiKey::Confirm)] = true;
        routeHudInput(commands, input);
        break;
    }
    case 1:
        require(!commands.snapshot().welcomeOpen, "welcome did not close");
        capture(presenter, "03-sanctuary");
        distributionSmokeWalk(game, {{10, 8}, Heading::West}, actions_);
        break;
    case 2:
        require(commands.startDialogue(), "mayor dialogue did not open");
        commands.tickDialogue(.7, true);
        break;
    case 3:
        require(commands.snapshot().dialogue.open, "dialogue panel not open");
        capture(presenter, "04-dialogue");
        commands.finishDialogue();
        break;
    case 4:
        distributionSmokeWalk(game, sanctuaryCity().doorway("inn").approach, actions_);
        break;
    case 5:
        commands.interactFixture();
        require(bool(commands.snapshot().travelRequested), "inn travel not requested");
        break;
    case 6:
        require(run.activeLocationId() == CampaignLocationId::Inn, "inn travel failed");
        capture(presenter, "05-inn");
        distributionSmokeWalk(game, run.location(unsigned(CampaignLocationId::Inn)).exit, actions_);
        break;
    case 7:
        commands.interactFixture();
        require(bool(commands.snapshot().travelRequested), "return travel not requested");
        break;
    case 8: {
        require(run.activeLocationId() == CampaignLocationId::City, "return to Sanctuary failed");
        bool found = false;
        for (unsigned i = 0; i < game.fixtures().definition().count; ++i) {
            const auto& f = game.fixtures().definition().entries[i];
            if (f.kind == FixtureKind::Downstairs) {
                distributionSmokeWalk(game, {stairLanding(f), f.facing}, actions_);
                found = true;
                break;
            }
        }
        require(found, "missing dungeon entrance");
        break;
    }
    case 9:
        commands.interactFixture();
        if (commands.snapshot().travel.open) {
            commands.selectTravel(0);
            commands.confirmTravel();
        }
        require(bool(commands.snapshot().travelRequested), "dungeon travel not requested");
        break;
    case 10:
        require(run.activeLocationId() == CampaignLocationId::FloorOne, "dungeon travel failed");
        capture(presenter, "06-dungeon");
        commands.toggleMenu();
        break;
    case 11: {
        require(commands.snapshot().menuOpen && commands.saveMenu().allowed, "campaign is not save-safe");
        commands.saveMenu().cursor = 2;
        UiInput input;
        input.pressed[std::size_t(UiKey::Confirm)] = true;
        commands.routeSaveMenu(input);
        require(commands.saveMenu().intent == SaveIntent::SaveMenu, "Save and Main Menu not requested");
        break;
    }
    default:
        throw std::runtime_error("Distribution smoke: save did not return to menu");
    }
    return false;
}
void DistributionSmoke::saved(const CampaignRun& run) {
    require(stage_ == 12, "save arrived out of order");
    write(options_.smokeOutput / "expected.json", distributionSmokeState(run));
    finished_ = true;
}
void DistributionSmoke::complete() {
    require(finished_, "application exited before smoke completion");
    require(fs::is_regular_file(audio::settingsPath()) && fs::is_regular_file(characterLibraryPath()) &&
                fs::is_regular_file(campaignSavePath()),
            "default user data was not persisted");
    write(options_.smokeOutput /
              (options_.smokePhase == SmokePhase::Create ? "create.json" : "continue.json"),
          {{"status", "passed"},
           {"actions", actions_},
           {"audio", "references resolved; no device playback"},
           {"assets", options_.assets.u8string()},
           {"prepared", options_.preparedAssets.u8string()},
           {"campaign", campaignSavePath().u8string()},
           {"settings", audio::settingsPath().u8string()},
           {"library", characterLibraryPath().u8string()},
           {"graphics", graphics_}});
    std::cout << "Distribution smoke passed\n";
}
} // namespace blobber
