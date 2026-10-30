#pragma once
#include "probe/assets.hpp"
#include <array>
namespace blobber::probe {
struct DrawStats {
    unsigned draws = 0, uploads = 0;
    bool cached = false;
};
// No recycling: resource values belong exclusively to this immovable owner.
class Gpu {
  public:
    Gpu(const Scene&, const Art&);
    ~Gpu();
    Gpu(const Gpu&) = delete;
    Gpu& operator=(const Gpu&) = delete;
    const Texture2D& render(const Scene&, const Art&, const MovementState&, unsigned timeMs, int mode,
                            float door);
    const Texture2D& result() const { return targets_[0].texture; }
    DrawStats stats{};
    std::size_t bytes = 0, cpuBytes = 0;
    int anisotropy = 1;

  private:
    struct Batch {
        Mesh mesh{};
        unsigned texture = 0;
    };
    std::array<Batch, 8> batches_{};
    unsigned batchCount_ = 0;
    std::array<Texture2D, 32> textures_{};
    unsigned textureCount_ = 0;
    std::array<RenderTexture2D, 2> targets_{};
    std::array<std::array<unsigned, 4>, 4> sprites_{};
    Shader shader_{}, resolve_{}, sky_{};
    int forwardLocation_ = -1, kindLocation_ = -1, skyYawLocation_ = -1;
    Mesh quad_{}, cube_{};
    ::Material material_{};
    bool ready_ = false;
    Camera last_{};
    int lastMode_ = -1, lastSprite_ = -1;
    float lastDoor_ = -1;
    unsigned upload(const Pixels&, bool clamp);
    void filter(int);
    void cleanup() noexcept;
    void draw(Mesh&, unsigned, Matrix, int kind = 0);
};
void finishGpu();
std::string driverDescription();
void exportTexture(Texture2D, const std::filesystem::path&);
} // namespace blobber::probe
