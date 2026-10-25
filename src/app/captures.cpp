#include "app/captures.hpp"
#include <array>
namespace blobber {
std::vector<Capture> makeCaptures() {
    const Player corridor{{3, 21}, Heading::North}, room{{5, 8}, Heading::North};
    std::vector<Capture> result;
    auto still = [&](std::string name, Player p) {
        result.push_back({std::move(name), p, p, Action::Forward, 1});
    };
    still("01-corridor", corridor);
    still("02-pillar-room", room);
    still("03-great-hall", {{18, 20}, Heading::North});
    still("04-long-sightline", {{3, 11}, Heading::East});
    still("05-junction", {{11, 11}, Heading::West});
    still("06-near-wall", {{2, 2}, Heading::West});
    still("07-room-corner", {{8, 8}, Heading::North});
    still("08-dead-end", {{5, 19}, Heading::West});
    still("09-approaching-right-passage", {{3, 16}, Heading::North});
    still("10-beside-right-passage", {{3, 15}, Heading::North});
    still("11-facing-adjacent-wall", {{3, 14}, Heading::West});
    const Player center{{6, 7}, Heading::North};
    for (int i = 0; i < 6; ++i) {
        Player p = center;
        if (i == 0)
            p.cell.y--;
        if (i == 1)
            p.cell.y++;
        if (i == 2)
            p.cell.x--;
        if (i == 3)
            p.cell.x++;
        if (i == 4)
            p.heading = Heading::West;
        if (i == 5)
            p.heading = Heading::East;
        result.push_back({"transition-" + std::to_string(i), center, p, static_cast<Action>(i), 0.5f});
    }
    const Player strafeStart{{8, 4}, Heading::West};
    for (Action action : {Action::Left, Action::Right}) {
        Player end = strafeStart;
        end.cell = end.cell + right(end.heading) * (action == Action::Right ? 1 : -1);
        for (int step = 0; step <= 8; ++step)
            result.push_back({std::string("room-strafe-") + (action == Action::Right ? "right-" : "left-") +
                                  std::to_string(step),
                              strafeStart, end, action, step / 8.0f});
    }
    still("npc-front", {{18, 18}, Heading::North});
    still("npc-back", {{18, 16}, Heading::South});
    still("npc-left", {{19, 17}, Heading::West});
    still("npc-right", {{17, 17}, Heading::East});
    still("npc-far", {{18, 20}, Heading::North});
    still("npc-behind-pillar", {{16, 15}, Heading::South});
    const Player npcStrafeStart{{17, 18}, Heading::North};
    const Player npcStrafeEnd{{18, 18}, Heading::North};
    for (int step = 0; step <= 8; ++step)
        result.push_back(
            {"npc-strafe-" + std::to_string(step), npcStrafeStart, npcStrafeEnd, Action::Right, step / 8.0f});
    const Player pillarStart{{17, 15}, Heading::South};
    const Player pillarEnd{{18, 15}, Heading::South};
    for (int step = 0; step <= 8; ++step)
        result.push_back(
            {"npc-pillar-reveal-" + std::to_string(step), pillarStart, pillarEnd, Action::Left, step / 8.0f});
    const std::array<Player, 4> npcViews{{{{18, 18}, Heading::North},
                                          {{18, 16}, Heading::South},
                                          {{19, 17}, Heading::West},
                                          {{17, 17}, Heading::East}}};
    const std::array<const char*, 4> sides{"front", "back", "left", "right"};
    for (std::size_t side = 0; side < npcViews.size(); ++side)
        for (std::uint64_t ms : {0u, 300u, 600u, 700u, 900u, 950u, 1200u, 1500u, 1550u})
            result.push_back({std::string("npc-animation-") + sides[side] + "-" + std::to_string(ms),
                              npcViews[side], npcViews[side], Action::Forward, 1, ms});
    for (std::uint64_t ms : {300u, 700u, 950u})
        result.push_back({"npc-animation-strafe-" + std::to_string(ms), npcStrafeStart, npcStrafeEnd,
                          Action::Right, 0.5f, ms});
    return result;
}
} // namespace blobber
