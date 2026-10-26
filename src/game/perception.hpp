#pragma once
#include "world/map.hpp"
namespace blobber {
bool lineOfSight(const Map&, Cell, Cell) noexcept;
bool inBowRange(Cell, Cell) noexcept;
bool inPartyView(const Map&, Cell, Heading, Cell) noexcept;
bool npcSees(const Map&, Cell, Heading, Cell) noexcept;
Heading headingToward(Cell, Cell) noexcept;
int cellDistance(Cell, Cell) noexcept;
} // namespace blobber
