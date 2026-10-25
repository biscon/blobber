#include "game/commands.hpp"
#include "support/actor_fixtures.hpp"
#include "support/check.hpp"
#include "support/party_fixtures.hpp"
#include "ui/interaction.hpp"
#include <cstring>
#include <iostream>
#include <limits>
using namespace blobber;
int main() {
    try {
        const auto catalog = partyFixture().catalog();
        Party party(catalog);
        const Map map({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
        auto actors =
            testActors(map, {3, 3}, {"test"},
                       {{{0}, {3, 1}, {0, 0}, Heading::South}, {{0}, {3, 1}, {1, 1}, Heading::South}});
        CommandController commands(party, actors);
        TargetSnapshot targets;
        targets.world = actors.snapshot().world;
        targets.revision = actors.snapshot().revision;
        targets.ids[0] = actors.id(1);
        targets.ids[1] = actors.id(0);
        targets.ids[2] = actors.id(0);
        targets.count = 3;
        commands.refresh(targets, false, false, true);
        check(commands.snapshot().targets.count == 2 && commands.snapshot().targets.ids[0].slot == 0,
              "Stable deduplicated targets");
        commands.refresh(commands.snapshot().targets, false, false, true);
        check(commands.snapshot().targets.count == 2, "Borrowed target snapshot can be refreshed safely");
        const int hp = party.character({0})->hitPoints;
        check(commands.confirm() == CommandReason::MissingTarget, "Target required");
        commands.selectTarget(actors.id(0));
        check(commands.confirm() == CommandReason::PreviewAccepted, "Melee preview accepted");
        commands.selectAction(PartyAction::BackMelee);
        check(commands.confirm() == CommandReason::BackRowOnly,
              "Front member cannot preview back-row action");
        commands.selectCharacter(2);
        check(commands.confirm() == CommandReason::PreviewAccepted, "Back member can preview step forward");
        check(party.swapFormation(FormationSlot::BackLeft, FormationSlot::FrontLeft), "Swap fixture");
        commands.refresh(targets, false, false, true);
        check(commands.confirm() == CommandReason::BackRowOnly,
              "Selected identity follows changed formation");
        for (const auto action : {PartyAction::Ranged, PartyAction::Spell, PartyAction::Item}) {
            commands.selectAction(action);
            check(commands.confirm() == CommandReason::Unavailable,
                  "Deferred actions explain unavailability");
        }
        check(commands.submit({{99}, PartyAction::Defend, {}}) == CommandReason::InvalidCharacter,
              "Invalid member rejected");
        check(commands.submit({{0}, static_cast<PartyAction>(255), {}}) == CommandReason::InvalidAction,
              "Invalid action rejected");
        commands.selectAction(PartyAction::Melee);
        commands.selectTarget(actors.id(0));
        const auto command = commands.snapshot().selection;
        commands.refresh(targets, true, false, true);
        check(!commands.snapshot().selection.target && commands.confirm() == CommandReason::Moving,
              "Movement clears target and suspends confirmation");
        commands.refresh(targets, false, true, true);
        check(commands.confirm() == CommandReason::MapOpen, "Map suspends commands");
        commands.refresh(targets, false, false, true);
        commands.selectTarget(actors.id(0));
        commands.refresh(targets, false, false, false);
        check(!commands.snapshot().selection.target && commands.confirm() == CommandReason::Unfocused,
              "Focus loss cancels");
        commands.refresh(targets, false, false, true);
        auto foreign = testActors(map, {3, 3}, {"test"}, {{{0}, {3, 1}, {0, 0}, Heading::South}});
        check(commands.submit({{0}, PartyAction::Melee, foreign.id(0)}) == CommandReason::InvalidTarget,
              "Foreign actor rejected");
        check(bool(actors.damage(actors.id(0), 7, 700)), "Kill target fixture");
        check(commands.submit(command) == CommandReason::InvalidTarget,
              "Death revalidated before snapshot refresh");
        const auto dead = actors.id(0);
        check(bool(actors.remove(dead)), "Remove dead fixture");
        check(commands.submit(command) == CommandReason::InvalidTarget, "Removed target rejected");
        commands.refresh(targets, false, false, true);
        check(commands.snapshot().targets.count == 0, "Stale visibility snapshot rejected");
        targets.revision = actors.snapshot().revision;
        commands.refresh(targets, false, false, true);
        check(commands.snapshot().targets.count == 1, "Dead target filtered");
        commands.selectTarget(actors.id(1));
        targets.count = 0;
        commands.refresh(targets, false, false, true);
        check(!commands.snapshot().selection.target, "Hidden target cleared without retargeting");
        const auto count = commands.snapshot().log.size();
        commands.refresh(targets, false, false, true);
        check(commands.snapshot().log.size() == count, "Idle refresh does not add log messages");
        commands.selectAction(PartyAction::Defend);
        check(commands.confirm() == CommandReason::PreviewAccepted, "Defend requires no target");
        for (int i = 0; i < 40; ++i)
            commands.confirm();
        check(commands.snapshot().log.size() == CombatLog::capacity, "Command log bounded");
        check(party.character({0})->hitPoints == hp && party.character({0})->availableHitDice == 1 &&
                  actors.actor(actors.id(1))->hitPoints == 7,
              "Preview requests never spend resources or damage targets");
        CombatLog log;
        for (int i = 0; i < 40; ++i) {
            char n[16];
            std::snprintf(n, sizeof(n), "entry %d", i);
            log.append(n);
        }
        check(std::strcmp(log.newest(0), "entry 39") == 0 && std::strcmp(log.newest(31), "entry 8") == 0,
              "Ring retains newest entries chronologically");
        char longText[300];
        std::memset(longText, 'x', 299);
        longText[299] = 0;
        log.append(longText);
        check(std::strlen(log.newest(0)) == 191 && !*log.newest(32),
              "Truncation and out-of-range log reads safe");
        commands.scrollLog(999);
        check(commands.snapshot().logOffset == commands.snapshot().log.lineCount(hudLayout::logColumns) - 1,
              "Scroll bounded oldest");
        commands.scrollLog(-999);
        check(commands.snapshot().logOffset == 0, "Scroll bounded newest");
        HudDemo demo;
        demo.enabled = true;
        for (std::size_t i = 0; i < 4; ++i)
            demo.resources[i] = {1, 2, 6, DisplayStatus::Ready};
        demo.resources[0].status = DisplayStatus::Unconscious;
        commands.refresh(targets, false, false, true, &demo);
        check(commands.submit({{0}, PartyAction::Defend, {}}) == CommandReason::CannotAct,
              "Demo status blocks preview without modifying Party");
        commands.refresh(targets, false, false, true);
        check(commands.submit({{0}, PartyAction::Defend, {}}) == CommandReason::PreviewAccepted &&
                  party.character({0})->hitPoints == hp,
              "Leaving fixture restores authoritative view");

        // Full-capacity pagination and selection must not truncate targets.
        std::vector<std::string> rows(11, std::string(11, '.'));
        for (int i = 0; i < 11; ++i) {
            rows[0][i] = '#';
            rows[10][i] = '#';
            rows[i][0] = '#';
            rows[i][10] = '#';
        }
        const Map largeMap(rows);
        std::vector<NpcInstance> placements;
        // Leave the spawn cell empty and fill exactly 256 subslots.
        for (int y = 1; y < 10; ++y)
            for (int x = 1; x < 10; ++x)
                if (x != 1 || y != 1)
                    for (int sy = 0; sy < 2; ++sy)
                        for (int sx = 0; sx < 2; ++sx)
                            if (placements.size() < NpcWorld::maxInstances)
                                placements.push_back({{0}, {x, y}, {sx, sy}, Heading::South});
        auto many = testActors(largeMap, {1, 1}, {"test"}, placements);
        CommandController paged(party, many);
        TargetSnapshot all;
        all.world = many.snapshot().world;
        all.revision = many.snapshot().revision;
        all.count = many.snapshot().count;
        for (std::size_t i = 0; i < all.count; ++i)
            all.ids[i] = many.id(i);
        paged.refresh(all, false, false, true);
        paged.cycleTarget(-1);
        check(paged.snapshot().selection.target->slot == all.count - 1 && paged.snapshot().targetPage == 63,
              "Last of 256 targets remains selectable and paginated");
        paged.cycleTarget(1);
        check(paged.snapshot().selection.target->slot == 0 && paged.snapshot().targetPage == 0,
              "Target cycling wraps");
        paged.pageTargets(-1);
        check(paged.snapshot().targetPage == 63, "Paging wraps to last page");
        all.count = all.ids.size() + 1;
        paged.refresh(all, false, false, true);
        check(paged.snapshot().targets.count == 0 && !paged.snapshot().selection.target,
              "Oversized visibility input rejected safely");

        UiInputEdges edges;
        std::array<bool, uiKeyCount> down{};
        down[static_cast<std::size_t>(UiKey::Confirm)] = true;
        check(edges.sample(down, true).has(UiKey::Confirm), "First press fires");
        check(!edges.sample(down, true).has(UiKey::Confirm), "Held confirm does not repeat");
        edges.sample(down, false);
        check(!edges.sample(down, true).has(UiKey::Confirm), "Regain focus while held does not fire");
        down.fill(false);
        edges.sample(down, true);
        down[static_cast<std::size_t>(UiKey::Confirm)] = true;
        check(edges.sample(down, true).has(UiKey::Confirm), "Fresh press after release fires");
        edges.sample(down, false);
        down[static_cast<std::size_t>(UiKey::Click)] = true;
        check(!edges.sample(down, true).has(UiKey::Click), "Mouse held on focus regain suppressed");
        UiInputEdges platformEdges;
        std::array<bool, uiKeyCount> physical{};
        physical[static_cast<std::size_t>(UiKey::Confirm)] = true;
        check(platformEdges.sample(physical, true).has(UiKey::Confirm), "Focused confirmation starts once");
        physical[static_cast<std::size_t>(UiKey::Confirm)] =
            false; // Synthetic release at blur, not a physical release.
        platformEdges.sample(physical, false);
        platformEdges.sample(physical, true);
        physical[static_cast<std::size_t>(UiKey::Confirm)] =
            true; // Delayed key-down synthesized by desktop focus restoration.
        check(!platformEdges.sample(physical, true).has(UiKey::Confirm),
              "Refocus synthetic press cannot replay confirmation");
        physical[static_cast<std::size_t>(UiKey::Confirm)] = false;
        platformEdges.sample(physical, true);
        physical[static_cast<std::size_t>(UiKey::Confirm)] = true;
        check(platformEdges.sample(physical, true).has(UiKey::Confirm),
              "Focused release then fresh press rearms confirmation");
        UiInput input;
        input.pressed[static_cast<std::size_t>(UiKey::Confirm)] = true;
        commands.cancel();
        commands.selectAction(PartyAction::Melee);
        routeHudInput(commands, input);
        check(std::strstr(commands.snapshot().log.newest(0), "Select a visible target") != nullptr,
              "Keyboard activation routes rejection");
        input.pressed = {};
        input.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
        const auto portrait = hudLayout::card(2);
        input.pointer = ArtPoint{float(portrait.x + 5), float(portrait.y + 5)};
        routeHudInput(commands, input);
        check(commands.snapshot().inspected &&
                  commands.snapshot().inspected->index == party.snapshot().formation[2].index,
              "Portrait inspection uses formation identity");
        commands.closeOverlay();
        input.pointer = ArtPoint{float(hudLayout::logExpand.x + 2), float(hudLayout::logExpand.y + 2)};
        routeHudInput(commands, input);
        check(commands.snapshot().logExpanded, "Log expands for complete messages");
        for (const auto size : {std::array<int, 4>{1920, 1080, 1920, 1080},
                                {1280, 720, 2560, 1440},
                                {1024, 768, 1024, 768},
                                {2560, 1080, 2560, 1080}}) {
            const auto center = artPoint(size[0] / 2.0f, size[1] / 2.0f, size[0], size[1], size[2], size[3]);
            check(center && std::abs(center->x - 320) < 0.001f && std::abs(center->y - 180) < 0.001f,
                  "Logical/high-DPI/aspect-fit center agrees");
        }
        check(!artPoint(10, 10, 1024, 768, 1024, 768) && !artPoint(10, 500, 2560, 1080, 2560, 1080),
              "Letterbox and pillarbox clicks rejected");
        check(!artPoint(0, 0, 0, 0, 0, 0) &&
                  !artPoint(std::numeric_limits<float>::quiet_NaN(), 0, 640, 360, 640, 360),
              "Invalid coordinate inputs rejected");
        check(!artPoint(640, 0, 640, 360, 640, 360) && !hudLayout::card(0).contains({159, 300}),
              "Half-open edges avoid ambiguous hits");
        std::cout << "Command and HUD input checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
