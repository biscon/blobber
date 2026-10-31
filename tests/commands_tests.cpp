#include "game/commands.hpp"
#include "support/actor_fixtures.hpp"
#include "support/check.hpp"
#include "support/combat_fixtures.hpp"
#include "ui/interaction.hpp"
#include <cstring>
#include <iostream>
#include <limits>
using namespace blobber;
int main() {
    try {
        const auto catalog = combatCatalog();
        const auto map = combatMap();
        const NpcWorld smallPlacements(
            map, {3, 3}, {"goblin"},
            {{{0}, {3, 1}, {0, 0}, Heading::South}, {{0}, {3, 1}, {1, 1}, Heading::South}});
        GameSession session(map, catalog, smallPlacements, 5, {{3, 3}, Heading::North}, false);
        auto& party = session.party();
        auto& actors = session.actors();
        CommandController commands(session);
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
        commands.selectTarget(actors.id(0));
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
        const NpcWorld largePlacements(largeMap, {1, 1}, {"goblin"}, placements);
        GameSession largeSession(largeMap, catalog, largePlacements, 5, {{1, 1}, Heading::North}, false);
        auto& many = largeSession.actors();
        CommandController paged(largeSession);
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
        check(commands.snapshot().log.size() > 0, "Keyboard activation routes through live commands");
        input.pressed = {};
        input.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
        const auto portrait = hudLayout::detail(2);
        input.pointer = ArtPoint{float(portrait.x + 5), float(portrait.y + 5)};
        routeHudInput(commands, input);
        check(commands.snapshot().inventory.open &&
                  commands.snapshot().inventory.page == CharacterPage::Character &&
                  commands.snapshot().inventory.character.index == party.snapshot().formation[2].index,
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
