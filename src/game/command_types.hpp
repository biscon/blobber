#pragma once
#include "game/actors.hpp"
#include "game/party.hpp"
#include <optional>

namespace blobber {
inline bool sameActor(ActorId a, ActorId b) noexcept {
    return a.slot == b.slot && a.world == b.world;
}
enum class PartyAction : std::uint8_t { Melee, BackMelee, Defend, Ranged, Spell, Item, Aid, Count };
enum class CommandReason : std::uint8_t {
    PreviewAccepted,
    Ready,
    Resolved,
    NoEncounter,
    NotYourTurn,
    StaleTurn,
    FrontProtected,
    RearBlocked,
    NoWeapon,
    InvalidAlly,
    LogOpen,
    FeatureRequired,

    InvalidCharacter,
    InvalidAction,
    Unavailable,
    BackRowOnly,
    MissingTarget,
    InvalidTarget,
    NotVisible,
    Moving,
    MapOpen,
    Unfocused,
    CannotAct,
    Cooldown,
    OutOfRange,
    Blocked,
};
const char* actionName(PartyAction action) noexcept;
const char* commandMessage(CommandReason reason) noexcept;
struct PartyCommand {
    rules::CharacterId character;
    PartyAction action = PartyAction::Melee;
    std::optional<ActorId> target;
    std::optional<rules::CharacterId> ally{};
    std::uint64_t world = 0, turn = 0;
};
// Presentation visibility only. Combat independently validates range, rank, life and turn.
struct TargetAnchor {
    int x = 0, y = 0; // Viewport-local visible-body center, not a world coordinate.
    bool valid = false;
    int top = 0;
};
struct TargetSnapshot {
    std::array<ActorId, NpcWorld::maxInstances> ids{};
    std::size_t count = 0;
    std::uint64_t world = 0, revision = 0;
    std::array<TargetAnchor, NpcWorld::maxInstances> anchors{}; // Indexed by actor slot.
};
} // namespace blobber
