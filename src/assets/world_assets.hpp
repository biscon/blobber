#pragma once
#include "assets/npc_metadata.hpp"
#include <fstream>
#include <string>
#include <vector>
namespace blobber::world_assets {
constexpr std::size_t MiB = 1024 * 1024, textureBudget = 512 * MiB, alphaBudget = 64 * MiB;
constexpr std::size_t worldCpuBudget = 352 * MiB, worldGpuBudget = 608 * MiB;
constexpr unsigned version = 3, appearanceRevision = 4, textureCapacity = 480;
// Stored texels occupy [x,x+width) in the original registered canvas. Anchors
// and attachments always refer to that canvas, including after offline cropping.
struct SpriteCanvas {
    unsigned width = 0, height = 0, x = 0, y = 0;
};
struct Texture {
    std::string name;
    unsigned width = 0, height = 0, levels = 0;
    bool clamp = false;
    std::uint64_t offset = 0, bytes = 0, alphaOffset = 0, alphaBytes = 0, checksum = 0, alphaChecksum = 0;
    bool material = true;
    SpriteCanvas canvas{};
};
struct Clip {
    NpcClipTiming timing{};
    float width = 0;
    std::array<std::array<std::uint16_t, 4>, 4> textures{};
    Clip() {
        for (auto& v : textures)
            v.fill(UINT16_MAX);
    }
};
struct Appearance {
    std::string id;
    float width = .45f, height = .65f;
    unsigned cutoff = 0;
    NpcAttachments attachments;
    std::array<Clip, npcAnimationCount> clips;
};
struct Anchor {
    float x = .5f, y = 1;
};
struct Pack {
    std::string id = "dungeon";
    std::array<std::uint16_t, 32> materials{};
    unsigned materialCount = 0;
    std::uint16_t door = 0, lockedDoor = 0, doorEdge = 0, crest = 0, vine = 0, sky = 0;
    std::uint16_t guardianEmblem = UINT16_MAX; // Optional transparent art, reusable across packs.
    float worldScale = 1;
    std::array<float, 3> horizon{.60f, .67f, .72f};
};
struct Publication {
    std::filesystem::path payload;
    std::uint64_t fingerprint = 0, definitions = 0, textureBytes = 0, alphaBytes = 0;
    std::vector<Texture> textures;
    std::vector<Anchor> anchors;
    std::vector<Appearance> appearances;
    std::vector<Pack> packs;
    std::vector<unsigned char> read(unsigned index, bool alpha = false) const;
};
// Immutable preparation owner; safe to share between live and windowless views.
struct AlphaBounds {
    unsigned left = 0, top = 0, right = 0, bottom = 0;
};
class CpuAssets {
  public:
    explicit CpuAssets(Publication);
    const Publication publication;
    std::vector<std::vector<unsigned char>> masks;
    std::vector<AlphaBounds> bounds;
    std::array<float, 32> edgeHalfWidths{};
    unsigned named(const char*) const;
    Pack pack(const std::string&) const;
    std::size_t bytes() const noexcept;
};
std::size_t mipBytes(unsigned width, unsigned height);
unsigned mipLevels(unsigned width, unsigned height);
Publication load(const std::filesystem::path& directory);
// Offline helpers have no graphics dependency; input pixels are tightly packed RGBA8.
std::vector<unsigned char> preparePixels(const std::vector<unsigned char>& rgba, unsigned width,
                                         unsigned height, unsigned cutoff, bool sprite,
                                         std::vector<unsigned char>& alpha);
class Writer {
  public:
    explicit Writer(const std::filesystem::path& directory);
    ~Writer();
    unsigned add(const std::string& name, unsigned width, unsigned height, bool sprite,
                 const std::vector<unsigned char>& rgba, unsigned cutoff = 0, Anchor anchor = {},
                 bool material = true, bool crop = false);
    void finish(const std::vector<Appearance>&, std::uint64_t definitions = 0,
                const std::vector<Pack>& packs = {});

  private:
    std::filesystem::path directory_, temporary_;
    std::ofstream out_;
    Publication publication_;
    std::uint64_t hash_ = 14695981039346656037ull, offset_ = 0;
    void bytes(const std::vector<unsigned char>&);
};
} // namespace blobber::world_assets
