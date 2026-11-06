#pragma once
#include "game/npc_animation.hpp"
#include <filesystem>
namespace blobber {
struct NpcAttachment {
    float x = .5f, y = .45f;
};
struct NpcBodyBounds {
    float left = 0, top = 0, right = 1, bottom = 1;
};
inline bool validBodyBounds(NpcBodyBounds b) noexcept {
    return b.left >= 0 && b.top >= 0 && b.right <= 1 && b.bottom <= 1 && b.left < b.right && b.top < b.bottom;
}
struct NpcAttachments {
    std::array<NpcAttachment, 4> release{}, torso{};
    std::array<NpcBodyBounds, 4> bodyBounds{};
};
struct NpcImageReference {
    std::filesystem::path path;
    float anchorX = .5f, anchorY = 1;
};
struct NpcClipDefinition {
    NpcClipTiming timing;
    float width = 0;
    std::array<std::vector<NpcImageReference>, 4> views;
};
struct NpcDefinition {
    NpcRole role = NpcRole::Hostile;
    NpcAttachments attachments;
    float width = .45f, height = .65f;
    std::array<NpcImageReference, 4> views;
    std::array<NpcClipDefinition, npcAnimationCount> animations;
    unsigned char alphaCutoff = 0;
    bool areaSampling = false;
};
NpcTimings npcTimings(const NpcDefinition&);
struct ParsedNpcs {
    NpcWorld world;
    std::vector<NpcDefinition> definitions;
    std::uint64_t definitionFingerprint = 0;
};
struct LoadedNpcMetadata {
    NpcWorld world;
    std::vector<NpcTimings> timings;
    std::vector<NpcAttachments> attachments;
    std::uint64_t definitionFingerprint = 0;
};
struct NpcDefinitions {
    std::vector<std::string> types;
    std::vector<NpcTimings> timings;
    std::vector<NpcAttachments> attachments;
    std::uint64_t definitionFingerprint = 0;
};
ParsedNpcs parseNpcMetadata(const std::filesystem::path&, const Map&, Cell, bool placements = true);
NpcDefinitions loadNpcDefinitions(const std::filesystem::path&);
LoadedNpcMetadata loadNpcMetadata(const std::filesystem::path&, const Map&, Cell);
} // namespace blobber
