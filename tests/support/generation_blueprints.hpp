#pragma once
#include "world/blueprints.hpp"
#include <algorithm>
namespace blobber::test {
// Frozen, owned test contract. No production files or artwork are read.
inline const blueprints::BlueprintCatalog& campaignBlueprints() {
    static const auto value = [] {
        blueprints::BlueprintCatalog c;
        auto name = [](blueprints::Name& n, const char* s) {
            std::copy(s, s + std::char_traits<char>::length(s), n.begin());
        };
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "hearth");
            k.size = {1, 1};
            k.height = 0.5f;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "work-surface");
            k.size = {1, 1};
            k.height = 0.38f;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "table-and-benches");
            k.size = {1, 2};
            k.height = 0.34f;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "sleeping-bay");
            k.size = {1, 1};
            k.height = 0.35f;
            k.blocking = true;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "footlocker");
            k.size = {1, 1};
            k.height = 0.15f;
            k.blocking = false;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "weapon-storage");
            k.size = {2, 1};
            k.height = 0.75f;
            k.blocking = false;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "armor-display");
            k.size = {1, 1};
            k.height = 0.8f;
            k.blocking = false;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "barrel");
            k.size = {1, 1};
            k.height = 0.34f;
            k.blocking = false;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "crate-large");
            k.size = {1, 1};
            k.height = 0.23f;
            k.blocking = false;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "crate-small");
            k.size = {1, 1};
            k.height = 0.14f;
            k.blocking = false;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "grain-sack");
            k.size = {1, 1};
            k.height = 0.17f;
            k.blocking = false;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "tapestry");
            k.size = {1, 1};
            k.height = 1.62f;
            k.blocking = false;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "brazier");
            k.size = {1, 1};
            k.height = 0.38f;
            k.blocking = true;
            k.fire = AmbientFire{0.0f, 0.36f, 0.25f};
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "chandelier");
            k.size = {3, 2};
            k.height = 2.0f;
            k.blocking = false;
            k.clearance = 1.25f;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "pottery-shelves");
            k.size = {1, 1};
            k.height = 0.82f;
            k.blocking = false;
            k.closedWall = true;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "sleeping-bay-red");
            k.size = {1, 1};
            k.height = 0.35f;
            k.blocking = true;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "armory-workbench");
            k.size = {1, 1};
            k.height = 0.78f;
            k.blocking = false;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "coat-pegs");
            k.size = {1, 1};
            k.height = 0.71f;
            k.blocking = false;
            k.closedWall = true;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "bedding-shelf");
            k.size = {1, 1};
            k.height = 0.78f;
            k.blocking = false;
            k.closedWall = true;
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "wall-sconce");
            k.size = {1, 1};
            k.height = 0.67f;
            k.blocking = false;
            k.closedWall = true;
            k.fire = AmbientFire{0.0f, 0.58f, 0.334f, 0.6f};
        }
        {
            auto& k = c.kits[c.kitCount++];
            name(k.id, "wall-sconce-near-table");
            k.size = {1, 1};
            k.height = 0.67f;
            k.blocking = false;
            k.closedWall = true;
            k.fire = AmbientFire{0.35f, 0.58f, 0.334f, 0.6f};
        }
        {
            auto& r = c.rooms[c.roomCount++];
            name(r.id, "kitchen");
            r.purpose = blueprints::Purpose::Kitchen;
            r.size = {5, 5};
            r.ceiling = 6;
            r.placements[r.placementCount++] = {c.kit("hearth"), {0, 0}, Heading::South};
            r.placements[r.placementCount++] = {c.kit("work-surface"), {4, 0}, Heading::West};
            r.placements[r.placementCount++] = {c.kit("crate-large"), {1, 0}, Heading::South};
            r.placements[r.placementCount++] = {c.kit("barrel"), {3, 0}, Heading::South};
            r.placements[r.placementCount++] = {c.kit("crate-small"), {0, 3}, Heading::East};
            r.placements[r.placementCount++] = {c.kit("barrel"), {0, 4}, Heading::East};
            r.placements[r.placementCount++] = {c.kit("grain-sack"), {1, 4}, Heading::North};
            r.placements[r.placementCount++] = {c.kit("pottery-shelves"), {0, 1}, Heading::East};
            r.placements[r.placementCount++] = {c.kit("pottery-shelves"), {0, 2}, Heading::East};
            r.placements[r.placementCount++] = {c.kit("pottery-shelves"), {2, 0}, Heading::South};
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "north");
                s.cell = {2, -1};
                s.outward = Heading::North;
            }
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "east");
                s.cell = {5, 2};
                s.outward = Heading::East;
            }
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "south");
                s.cell = {2, 5};
                s.outward = Heading::South;
            }
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "west");
                s.cell = {-1, 2};
                s.outward = Heading::West;
            }
            r.lanes[r.laneCount++] = {{2, 0}, {1, 5}};
            r.lanes[r.laneCount++] = {{0, 2}, {5, 1}};
            r.encounters[r.encounterCount++] = {{2, 3}, {2, 2}};
            r.rewards[r.rewardCount++] = {{4, 4}, {3, 4}};
        }
        {
            auto& r = c.rooms[c.roomCount++];
            name(r.id, "mead-hall");
            r.purpose = blueprints::Purpose::MeadHall;
            r.size = {7, 7};
            r.ceiling = 8;
            r.solid[blueprints::index({1, 5})] = true;
            r.solid[blueprints::index({5, 5})] = true;
            r.placements[r.placementCount++] = {c.kit("table-and-benches"), {0, 1}, Heading::North};
            r.placements[r.placementCount++] = {c.kit("table-and-benches"), {6, 1}, Heading::North};
            r.placements[r.placementCount++] = {c.kit("tapestry"), {1, 0}, Heading::South};
            r.placements[r.placementCount++] = {c.kit("tapestry"), {5, 0}, Heading::South};
            r.placements[r.placementCount++] = {c.kit("brazier"), {0, 0}, Heading::South};
            r.placements[r.placementCount++] = {c.kit("brazier"), {6, 0}, Heading::South};
            r.placements[r.placementCount++] = {c.kit("table-and-benches"), {3, 1}, Heading::North};
            r.placements[r.placementCount++] = {c.kit("armor-display"), {2, 0}, Heading::South};
            r.placements[r.placementCount++] = {c.kit("armor-display"), {4, 0}, Heading::South};
            r.placements[r.placementCount++] = {c.kit("chandelier"), {2, 1}, Heading::North};
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "north");
                s.cell = {3, -1};
                s.outward = Heading::North;
            }
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "east");
                s.cell = {7, 4};
                s.outward = Heading::East;
            }
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "south");
                s.cell = {3, 7};
                s.outward = Heading::South;
            }
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "west");
                s.cell = {-1, 4};
                s.outward = Heading::West;
            }
            r.lanes[r.laneCount++] = {{2, 0}, {1, 7}};
            r.lanes[r.laneCount++] = {{4, 0}, {1, 7}};
            r.lanes[r.laneCount++] = {{2, 0}, {3, 1}};
            r.lanes[r.laneCount++] = {{0, 3}, {7, 1}};
            r.lanes[r.laneCount++] = {{0, 4}, {7, 1}};
            r.encounters[r.encounterCount++] = {{2, 4}, {3, 2}};
            r.rewards[r.rewardCount++] = {{6, 6}, {5, 6}};
        }
        {
            auto& r = c.rooms[c.roomCount++];
            name(r.id, "barracks");
            r.purpose = blueprints::Purpose::Barracks;
            r.size = {5, 7};
            r.ceiling = 7;
            r.placements[r.placementCount++] = {c.kit("sleeping-bay"), {0, 1}, Heading::East};
            r.placements[r.placementCount++] = {c.kit("sleeping-bay-red"), {4, 1}, Heading::West};
            r.placements[r.placementCount++] = {c.kit("sleeping-bay-red"), {0, 5}, Heading::East};
            r.placements[r.placementCount++] = {c.kit("sleeping-bay"), {4, 5}, Heading::West};
            r.placements[r.placementCount++] = {c.kit("coat-pegs"), {0, 0}, Heading::East};
            r.placements[r.placementCount++] = {c.kit("bedding-shelf"), {4, 0}, Heading::West};
            r.placements[r.placementCount++] = {c.kit("wall-sconce"), {0, 2}, Heading::East};
            r.placements[r.placementCount++] = {c.kit("wall-sconce"), {4, 2}, Heading::West};
            r.placements[r.placementCount++] = {c.kit("sleeping-bay"), {0, 3}, Heading::East};
            r.placements[r.placementCount++] = {c.kit("sleeping-bay-red"), {4, 3}, Heading::West};
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "north");
                s.cell = {2, -1};
                s.outward = Heading::North;
            }
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "south");
                s.cell = {2, 7};
                s.outward = Heading::South;
            }
            r.lanes[r.laneCount++] = {{2, 0}, {1, 7}};
            r.lanes[r.laneCount++] = {{1, 3}, {3, 1}};
            r.encounters[r.encounterCount++] = {{1, 2}, {3, 2}};
            r.rewards[r.rewardCount++] = {{3, 6}, {2, 6}};
        }
        {
            auto& r = c.rooms[c.roomCount++];
            name(r.id, "armory");
            r.purpose = blueprints::Purpose::Armory;
            r.size = {5, 5};
            r.ceiling = 6;
            r.placements[r.placementCount++] = {c.kit("weapon-storage"), {3, 0}, Heading::South};
            r.placements[r.placementCount++] = {c.kit("armory-workbench"), {4, 1}, Heading::West};
            r.placements[r.placementCount++] = {c.kit("armor-display"), {0, 4}, Heading::East};
            r.placements[r.placementCount++] = {c.kit("footlocker"), {4, 0}, Heading::West};
            r.placements[r.placementCount++] = {c.kit("wall-sconce"), {0, 0}, Heading::East};
            r.placements[r.placementCount++] = {c.kit("wall-sconce-near-table"), {4, 2}, Heading::West};
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "south");
                s.cell = {2, 5};
                s.outward = Heading::South;
            }
            {
                auto& s = r.sockets[r.socketCount++];
                name(s.id, "west");
                s.cell = {-1, 2};
                s.outward = Heading::West;
            }
            r.lanes[r.laneCount++] = {{2, 1}, {1, 4}};
            r.lanes[r.laneCount++] = {{0, 2}, {5, 1}};
            r.encounters[r.encounterCount++] = {{1, 3}, {2, 2}};
            r.rewards[r.rewardCount++] = {{4, 4}, {3, 4}};
        }
        blueprints::validate(c);
        return c;
    }();
    return value;
}
} // namespace blobber::test
