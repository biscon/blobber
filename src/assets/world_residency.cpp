#include "assets/world_assets.hpp"
#include "core/preparation.hpp"
#include "game/actors.hpp"
#include <algorithm>
#include <stdexcept>
namespace blobber::world_assets {
AssetRequirements allAssets(const Publication& p) {
    AssetRequirements r;
    if (p.textures.size() > catalogTextureCapacity || p.meshes.size() > meshCapacity)
        throw std::invalid_argument("Asset catalog capacity exceeded");
    for (unsigned i = 0; i < p.textures.size(); ++i)
        r.textures.set(i);
    for (unsigned i = 0; i < p.meshes.size(); ++i)
        r.meshes.set(i);
    return r;
}
AssetRequirements CpuAssets::requirements(const std::string& id, const ActorSnapshot* actors,
                                          bool interactive, bool ambientFire) const {
    AssetRequirements r;
    const auto p = pack(id);
    const auto texture = [&](unsigned i) {
        if (i == UINT16_MAX)
            return;
        if (i >= publication.textures.size())
            throw std::invalid_argument("Invalid location texture reference");
        r.textures.set(i);
    };
    for (unsigned i = 0; i < p.materialCount; ++i)
        texture(p.materials[i]);
    for (auto i : {p.door, p.lockedDoor, p.doorEdge, p.crest, p.vine, p.sky, p.guardianEmblem})
        texture(i);
    for (auto i : p.decorations)
        texture(i);
    for (auto i : p.models)
        if (i != UINT16_MAX) {
            if (i >= publication.meshes.size())
                throw std::invalid_argument("Invalid location model reference");
            r.meshes.set(i);
            texture(publication.meshes[i].texture);
        }
    for (const auto& b : p.buildings)
        for (auto i : b.parts) {
            if (i >= publication.meshes.size())
                throw std::invalid_argument("Invalid building mesh reference");
            r.meshes.set(i);
            texture(publication.meshes[i].texture);
        }
    if (ambientFire || id == "sanctuary-inn" || id == "sanctuary-smithy")
        texture(named("effect/hearth-flame"));
    if (actors) {
        if (actors->count > NpcWorld::maxInstances || actors->typeCount != publication.appearances.size())
            throw std::invalid_argument("Location appearance/actor counts differ");
        std::bitset<32> types;
        for (unsigned i = 0; i < actors->count; ++i) {
            const auto type = actors->actors[i].npc.type.index;
            if (type >= publication.appearances.size())
                throw std::invalid_argument("Invalid location actor appearance");
            types.set(type);
        }
        for (unsigned i = 0; i < publication.appearances.size(); ++i)
            if (types[i])
                for (const auto& clip : publication.appearances[i].clips)
                    for (const auto& side : clip.textures)
                        for (auto t : side)
                            texture(t);
    }
    if (interactive) {
        if (publication.effects[0].body == UINT16_MAX)
            throw std::invalid_argument("Interactive publication is missing elemental effect clips");
        for (const auto& clip : publication.effects) {
            texture(clip.body);
            texture(clip.emission);
        }
        constexpr const char* common[]{"loot/bag",         "fixture/key-bronze", "fixture/key-silver",
                                       "fixture/endpoint", "fixture/threshold",  "effect/arrow",
                                       "effect/tip",       "effect/feathers",    "effect/slash",
                                       "effect/glint",     "effect/grease",      "effect/web",
                                       "effect/weapon",    "effect/preview",     "effect/white"};
        for (auto name : common)
            texture(named(name));
    }
    return r;
}
AssetLease::~AssetLease() {
    reset();
}
AssetLease::AssetLease(AssetLease&& other) noexcept
    : owner_(std::exchange(other.owner_, nullptr)), held_(other.held_) {}
AssetLease& AssetLease::operator=(AssetLease&& other) noexcept {
    if (this != &other) {
        reset();
        owner_ = std::exchange(other.owner_, nullptr);
        held_ = other.held_;
    }
    return *this;
}
void AssetLease::reset() noexcept {
    if (owner_)
        owner_->release(held_);
    owner_ = nullptr;
}
CpuAssets::Residency CpuAssets::residency() const noexcept {
    Residency r;
    r.reads = reads_;
    r.readBytes = readBytes_;
    for (unsigned i = 0; i < textureRefs_.size(); ++i)
        if (textureRefs_[i]) {
            ++r.textures;
            r.alphaBytes += masks[i].capacity();
        }
    for (unsigned i = 0; i < meshRefs_.size(); ++i)
        if (meshRefs_[i]) {
            ++r.meshes;
            r.meshBytes += meshes_[i].vertices.capacity() * sizeof(std::array<float, 8>);
        }
    return r;
}
AssetLease CpuAssets::acquire(const AssetRequirements& required) const {
    auto alpha = residency().alphaBytes;
    unsigned count = residency().textures;
    for (unsigned i = 0; i < catalogTextureCapacity; ++i)
        if (required.textures[i]) {
            if (i >= publication.textures.size())
                throw std::invalid_argument("Missing required world texture");
            if (!textureRefs_[i]) {
                alpha += publication.textures[i].alphaBytes;
                ++count;
            }
        }
    for (unsigned i = 0; i < meshCapacity; ++i)
        if (required.meshes[i]) {
            if (i >= publication.meshes.size())
                throw std::invalid_argument("Missing required world model");
            if (!required.textures[publication.meshes[i].texture])
                throw std::invalid_argument("Required model is missing its atlas dependency");
        }
    if (alpha > alphaBudget || count > textureCapacity)
        throw std::runtime_error("Resident world mask/texture capacity exceeded");
    AssetLease lease;
    lease.owner_ = this;
    for (unsigned i = 0; i < publication.textures.size(); ++i)
        if (required.textures[i]) {
            if (!textureRefs_[i]) {
                const auto& t = publication.textures[i];
                if (t.alphaBytes) {
                    preparationPoint("Loading location alpha masks");
                    auto mask = publication.read(i, true);
                    AlphaBounds b{t.width, t.height, 0, 0};
                    for (unsigned y = 0; y < t.height; ++y)
                        for (unsigned x = 0; x < t.width; ++x)
                            if (mask[std::size_t(y) * t.width + x]) {
                                b.left = std::min(b.left, x);
                                b.top = std::min(b.top, y);
                                b.right = std::max(b.right, x + 1);
                                b.bottom = std::max(b.bottom, y + 1);
                            }
                    masks[i] = std::move(mask);
                    bounds[i] = b;
                    ++reads_;
                    readBytes_ += t.alphaBytes;
                }
            }
            ++textureRefs_[i];
            lease.held_.textures.set(i);
        }
    for (unsigned i = 0; i < publication.meshes.size(); ++i)
        if (required.meshes[i]) {
            if (!meshRefs_[i]) {
                preparationPoint("Loading location models");
                meshes_[i] = publication.readMesh(i);
                ++reads_;
                readBytes_ += meshes_[i].vertices.size() * 32;
            }
            ++meshRefs_[i];
            lease.held_.meshes.set(i);
        }
    if (bytes() > worldCpuBudget)
        throw std::runtime_error("Resident world CPU budget exceeded");
    return lease;
}
void CpuAssets::release(const AssetRequirements& held) const noexcept {
    for (unsigned i = 0; i < textureRefs_.size(); ++i)
        if (held.textures[i] && --textureRefs_[i] == 0) {
            std::vector<unsigned char>{}.swap(masks[i]);
            bounds[i] = {};
        }
    for (unsigned i = 0; i < meshRefs_.size(); ++i)
        if (held.meshes[i] && --meshRefs_[i] == 0)
            meshes_[i] = {};
}
const PropMesh& CpuAssets::mesh(unsigned id) const {
    if (id >= meshRefs_.size() || !meshRefs_[id])
        throw std::logic_error("World model was not acquired at preparation");
    return meshes_[id];
}
} // namespace blobber::world_assets
