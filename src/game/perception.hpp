#pragma once
#include "world/fixtures.hpp"
#include "world/map.hpp"
namespace blobber {
bool discoveryVisible(GeometryView, Cell, Heading, Cell) noexcept;
bool lineOfSight(GeometryView, Cell, Cell) noexcept;
bool inBowRange(Cell, Cell) noexcept;
bool combatAdjacent(GeometryView, Cell, Cell) noexcept;
bool inFrontBand(Cell, Heading, Cell) noexcept;
bool inMeleeFront(GeometryView, Cell, Heading, Cell) noexcept;
bool inPartyView(GeometryView, Cell, Heading, Cell) noexcept;
bool npcSees(GeometryView, Cell, Heading, Cell) noexcept;
Heading headingToward(Cell, Cell) noexcept;
int cellDistance(Cell, Cell) noexcept;
} // namespace blobber
