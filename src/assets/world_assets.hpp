#pragma once
#include "assets/effect_clips.hpp"
#include "assets/npc_metadata.hpp"
#include "world/surfaces.hpp"
#include <bitset>
#include <fstream>
#include <string>
#include <utility>
#include <vector>
namespace blobber {
struct ActorSnapshot;
}
namespace blobber::world_assets {
constexpr std::size_t MiB = 1024 * 1024, textureBudget = 512 * MiB, alphaBudget = 64 * MiB;
constexpr std::size_t worldCpuBudget = 352 * MiB, worldGpuBudget = 608 * MiB;
constexpr unsigned version = 13, appearanceRevision = 8, textureCapacity = 640;
constexpr unsigned catalogTextureCapacity = 4096, packCapacity = 64, meshCapacity = 512;
constexpr std::size_t metadataBudget = 8 * MiB;
constexpr std::uint64_t installedBudget = 8ull * 1024 * MiB;
constexpr std::size_t steadyGpuReference = 384 * MiB, peakGpuReference = 448 * MiB;
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
// Narrow prepared triangle list, not a general GLTF runtime. Coordinates/UVs are authored.
struct PropMesh {
    std::string name;
    std::uint16_t texture = 0;
    std::vector<std::array<float, 8>> vertices; // Only populated in authoring or an acquired mesh.
    std::uint64_t offset = 0, checksum = 0;
    unsigned vertexCount = 0;
    bool architecture = false;
    bool water = false;  // Opaque architecture; eight fixed 4x2 atlas frames at 8 FPS.
    bool cutout = false; // Named mesh parts only; fixed alpha threshold shared with CPU/GPU.
};
constexpr unsigned meshAlphaThreshold = 128;
constexpr unsigned staticModelCapacity = 32, staticPartCapacity = 8, staticVertexCapacity = 24000;
struct StaticModelId {
    std::uint16_t index = UINT16_MAX;
};
struct StaticModel {
    std::string id;
    std::vector<std::uint16_t> parts;
};
struct Pack {
    std::string id = "dungeon";
    std::array<std::uint16_t, 32> materials{};
    unsigned materialCount = 0;
    std::uint16_t door = 0, lockedDoor = 0, doorEdge = 0, crest = 0, vine = 0, sky = 0;
    std::uint16_t guardianEmblem = UINT16_MAX; // Optional transparent art, reusable across packs.
    std::array<std::uint16_t, decorationKinds> decorations = [] {
        std::array<std::uint16_t, decorationKinds> a{};
        a.fill(UINT16_MAX);
        return a;
    }();
    std::array<std::uint16_t, 4> models{UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX};
    std::vector<StaticModel> buildings;
    StaticModelId building(const std::string& name) const;
    std::size_t dynamicBytes() const noexcept {
        std::size_t n = id.capacity() + buildings.capacity() * sizeof(StaticModel);
        for (const auto& b : buildings)
            n += b.id.capacity() + b.parts.capacity() * sizeof(std::uint16_t);
        return n;
    }
    float worldScale = 1;
    float viewDistance = 16; // Prepared camera clipping; bounded to 16–40 cells.
    std::array<float, 3> horizon{.60f, .67f, .72f};
    // Start distance, transition span, maximum opacity; daylight only.
    std::array<float, 3> daylightFog{10.f, 4.5f, 1.f};
};
struct Publication {
    EffectClips effects{};
    std::filesystem::path payload;
    std::uint64_t fingerprint = 0, definitions = 0, textureBytes = 0, alphaBytes = 0;
    std::vector<Texture> textures;
    std::vector<Anchor> anchors;
    std::vector<Appearance> appearances;
    std::vector<Pack> packs;
    std::vector<PropMesh> meshes;
    std::vector<unsigned char> read(unsigned index, bool alpha = false) const;
    PropMesh readMesh(unsigned index) const;
    void verify() const; // Offline/full diagnostic verification, not startup.
    std::uint64_t payloadChecksum = 0;
};
struct AssetRequirements {
    std::bitset<catalogTextureCapacity> textures;
    std::bitset<meshCapacity> meshes;
};
// Explicit whole-publication selection for small owned fixtures/offline inspection.
AssetRequirements allAssets(const Publication&);
class CpuAssets;
class AssetLease {
  public:
    AssetLease() = default;
    ~AssetLease();
    AssetLease(const AssetLease&) = delete;
    AssetLease& operator=(const AssetLease&) = delete;
    AssetLease(AssetLease&&) noexcept;
    AssetLease& operator=(AssetLease&&) noexcept;

  private:
    friend class CpuAssets;
    const CpuAssets* owner_ = nullptr; // Preparation owner outlives leases.
    AssetRequirements held_{};
    void reset() noexcept;
};
// Metadata is immutable; payloads change only at serialized preparation boundaries.
struct AlphaBounds {
    unsigned left = 0, top = 0, right = 0, bottom = 0;
};
class CpuAssets {
  public:
    explicit CpuAssets(Publication);
    CpuAssets(const CpuAssets&) = delete;
    CpuAssets& operator=(const CpuAssets&) = delete;
    const Publication publication;
    mutable std::vector<std::vector<unsigned char>> masks;
    mutable std::vector<AlphaBounds> bounds;
    std::array<float, 32> edgeHalfWidths{};
    unsigned named(const char*) const;
    Pack pack(const std::string&) const;
    std::size_t bytes() const noexcept;
    AssetRequirements requirements(const std::string& pack, const ActorSnapshot* actors = nullptr,
                                   bool interactive = false) const;
    AssetLease acquire(const AssetRequirements&) const;
    const PropMesh& mesh(unsigned) const;
    struct Residency {
        std::size_t alphaBytes = 0, meshBytes = 0;
        unsigned textures = 0, meshes = 0;
        std::uint64_t reads = 0, readBytes = 0;
    };
    Residency residency() const noexcept;

  private:
    friend class AssetLease;
    mutable std::vector<unsigned> textureRefs_, meshRefs_;
    mutable std::vector<PropMesh> meshes_;
    mutable std::uint64_t reads_ = 0, readBytes_ = 0;
    void release(const AssetRequirements&) const noexcept;
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
                 bool material = true, bool crop = false, bool cutoutMaterial = false);
    void effects(const EffectClips& clips) { publication_.effects = clips; }
    void finish(const std::vector<Appearance>&, std::uint64_t definitions = 0,
                const std::vector<Pack>& packs = {}, const std::vector<PropMesh>& meshes = {});

  private:
    std::filesystem::path directory_, temporary_;
    std::ofstream out_;
    Publication publication_;
    std::uint64_t hash_ = 14695981039346656037ull, offset_ = 0;
    void bytes(const std::vector<unsigned char>&);
};
} // namespace blobber::world_assets
