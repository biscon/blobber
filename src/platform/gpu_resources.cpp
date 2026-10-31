#include "platform/gpu_resources.hpp"
#include "rlgl.h"
#include <algorithm>
#include <stdexcept>
namespace blobber {
namespace {
bool glOkay() noexcept {
    using Fn = unsigned (*)();
    const auto get = reinterpret_cast<Fn>(rlGetProcAddress("glGetError"));
    return get && get() == 0;
}
std::size_t textureBytes(Texture2D t) {
    std::size_t n = 0;
    for (int i = 0; i < t.mipmaps; ++i) {
        n += GetPixelDataSize(t.width, t.height, t.format);
        t.width = std::max(1, t.width / 2);
        t.height = std::max(1, t.height / 2);
    }
    return n;
}
std::size_t targetBytes(RenderTexture2D t) {
    return std::size_t(t.texture.width) * t.texture.height * 8;
}
void unload(GpuMesh m) noexcept {
    if (m.vao)
        rlUnloadVertexArray(m.vao);
    if (m.vertices)
        rlUnloadVertexBuffer(m.vertices);
    if (m.indices)
        rlUnloadVertexBuffer(m.indices);
}
} // namespace
GpuResources::~GpuResources() {
    meshes_.clear([](GpuMesh m) { unload(m); });
    shaders_.clear([](Shader s) { UnloadShader(s); });
    targets_.clear([](RenderTexture2D t) { UnloadRenderTexture(t); });
    textures_.clear([](Texture2D t) { UnloadTexture(t); });
}
void GpuResources::checkpoint() {
    if (failAfter_ == 0)
        throw std::runtime_error("Injected GPU preparation failure");
    if (failAfter_ > 0)
        --failAfter_;
}
void GpuResources::finishPreparation() {
    checkpoint();
    if (!glOkay())
        throw std::runtime_error("GPU preparation or initial rendering failed");
}
TextureHandle GpuResources::createTexture(const Image& image) {
    if (!textures_.available())
        throw std::runtime_error("Texture capacity exhausted (512)");
    if (!image.data || image.width <= 0 || image.height <= 0 || image.width > 8192 || image.height > 8192 ||
        image.mipmaps <= 0)
        throw std::invalid_argument("Invalid texture image");
    checkpoint();
    auto t = LoadTextureFromImage(image);
    if (!IsTextureValid(t) || !glOkay()) {
        if (t.id)
            UnloadTexture(t);
        throw std::runtime_error("Cannot create texture");
    }
    SetTextureFilter(t, TEXTURE_FILTER_POINT);
    ++counts_.textures;
    counts_.bytes += textureBytes(t);
    ++transfers_.uploads;
    transfers_.bytes += textureBytes(t);
    return textures_.insert(t);
}
TargetHandle GpuResources::createTarget(int w, int h) {
    if (!targets_.available())
        throw std::runtime_error("Render target capacity exhausted (8)");
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192)
        throw std::invalid_argument("Invalid render target dimensions");
    checkpoint();
    auto t = LoadRenderTexture(w, h);
    if (!IsRenderTextureValid(t) || !glOkay()) {
        UnloadRenderTexture(t);
        throw std::runtime_error("Cannot create render target");
    }
    SetTextureFilter(t.texture, TEXTURE_FILTER_POINT);
    ++counts_.targets;
    counts_.bytes += targetBytes(t);
    return targets_.insert(t);
}
MeshHandle GpuResources::createMesh(const world_render::Scene& s) {
    if (!meshes_.available())
        throw std::runtime_error("Mesh capacity exhausted (8)");
    if (s.meshBytes() > 40u * 1024 * 1024 || s.vertices.size() > 1048576 || s.indices.size() > 1572864)
        throw std::invalid_argument("World GPU mesh budget exceeded");
    for (auto i : s.indices)
        if (i >= s.vertices.size())
            throw std::invalid_argument("World mesh index outside vertices");
    if (!rlGetProcAddress("glDrawElements") || s.indices.size() % 3)
        throw std::invalid_argument("World indexed drawing unavailable or malformed");
    GpuMesh m{};
    m.count = std::uint32_t(s.indices.size());
    m.bytes = s.meshBytes();
    try {
        checkpoint();
        m.vao = rlLoadVertexArray();
        if (!m.vao)
            throw std::runtime_error("Cannot create world VAO");
        rlEnableVertexArray(m.vao);
        if (!s.vertices.empty()) {
            checkpoint();
            m.vertices = rlLoadVertexBuffer(s.vertices.data(),
                                            int(s.vertices.size() * sizeof(world_render::Vertex)), false);
            if (!m.vertices)
                throw std::runtime_error("Cannot create world vertex buffer");
            rlSetVertexAttribute(0, 3, RL_FLOAT, false, 32, 0);
            rlEnableVertexAttribute(0);
            rlSetVertexAttribute(1, 2, RL_FLOAT, false, 32, 24);
            rlEnableVertexAttribute(1);
            rlSetVertexAttribute(2, 3, RL_FLOAT, false, 32, 12);
            rlEnableVertexAttribute(2);
            checkpoint();
            m.indices = rlLoadVertexBufferElement(s.indices.data(), int(s.indices.size() * 4), false);
            if (!m.indices)
                throw std::runtime_error("Cannot create world index buffer");
        }
        rlDisableVertexArray();
        rlDisableVertexBuffer();
        rlDisableVertexBufferElement();
        if (!glOkay())
            throw std::runtime_error("World GPU buffer upload failed");
    } catch (...) {
        rlDisableVertexArray();
        rlDisableVertexBuffer();
        rlDisableVertexBufferElement();
        unload(m);
        throw;
    }
    ++counts_.meshes;
    counts_.bytes += m.bytes;
    ++transfers_.uploads;
    transfers_.bytes += m.bytes;
    return meshes_.insert(m);
}
ShaderHandle GpuResources::createShader(const char* vs, const char* fs) {
    if (!shaders_.available())
        throw std::runtime_error("Shader capacity exhausted (8)");
    checkpoint();
    auto s = LoadShaderFromMemory(vs, fs);
    if (!s.id || s.id == rlGetShaderIdDefault())
        throw std::runtime_error("World shader compilation failed");
    ++counts_.shaders;
    return shaders_.insert(s);
}
bool GpuResources::destroy(TextureHandle h) noexcept {
    if (auto t = textures_.release(h)) {
        --counts_.textures;
        counts_.bytes -= textureBytes(*t);
        UnloadTexture(*t);
        return true;
    }
    return false;
}
bool GpuResources::destroy(TargetHandle h) noexcept {
    if (auto t = targets_.release(h)) {
        --counts_.targets;
        counts_.bytes -= targetBytes(*t);
        UnloadRenderTexture(*t);
        return true;
    }
    return false;
}
bool GpuResources::destroy(MeshHandle h) noexcept {
    if (auto m = meshes_.release(h)) {
        --counts_.meshes;
        counts_.bytes -= m->bytes;
        unload(*m);
        return true;
    }
    return false;
}
bool GpuResources::destroy(ShaderHandle h) noexcept {
    if (auto s = shaders_.release(h)) {
        --counts_.shaders;
        UnloadShader(*s);
        return true;
    }
    return false;
}
const Texture2D& GpuResources::texture(TextureHandle h) const {
    if (auto p = textures_.get(h))
        return *p;
    throw std::invalid_argument("Invalid or stale texture handle");
}
const RenderTexture2D& GpuResources::target(TargetHandle h) const {
    if (auto p = targets_.get(h))
        return *p;
    throw std::invalid_argument("Invalid or stale render target handle");
}
const GpuMesh& GpuResources::mesh(MeshHandle h) const {
    if (auto p = meshes_.get(h))
        return *p;
    throw std::invalid_argument("Invalid or stale mesh handle");
}
const Shader& GpuResources::shader(ShaderHandle h) const {
    if (auto p = shaders_.get(h))
        return *p;
    throw std::invalid_argument("Invalid or stale shader handle");
}
void GpuResources::draw(MeshHandle h, std::uint32_t first, std::uint32_t count) const {
    const auto& m = mesh(h);
    if (first > m.count || count > m.count - first)
        throw std::invalid_argument("World draw range outside mesh");
    using Fn = void (*)(unsigned, int, unsigned, const void*);
    static auto drawElements = reinterpret_cast<Fn>(rlGetProcAddress("glDrawElements"));
    if (!drawElements)
        throw std::runtime_error("OpenGL indexed drawing unavailable");
    rlEnableVertexArray(m.vao);
    drawElements(0x0004, int(count), 0x1405, reinterpret_cast<const void*>(std::uintptr_t(first) * 4));
    rlDisableVertexArray();
}
} // namespace blobber
