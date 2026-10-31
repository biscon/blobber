#include "platform/world_renderer.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raymath.h"
#pragma GCC diagnostic pop
#include "rlgl.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace blobber {
namespace {
constexpr const char* vertex = R"(#version 330
in vec3 vertexPosition;in vec2 vertexTexCoord;in vec3 vertexNormal;
uniform mat4 mvp;uniform mat4 matView;
out vec2 uv;out vec3 normal;out float depth;
void main(){uv=vertexTexCoord;normal=vertexNormal;depth=-(matView*vec4(vertexPosition,1)).z;gl_Position=mvp*vec4(vertexPosition,1);})";
constexpr const char* fragment = R"(#version 330
in vec2 uv;in vec3 normal;in float depth;out vec4 finalColor;
uniform sampler2D texture0;uniform vec3 forward;
void main(){vec4 c=texture(texture0,uv);
float face=normal.y>0.5?0.85:(normal.y< -0.5?0.58:0.78+0.22*abs(dot(normal,forward)));
float light=face/(1.0+0.115*depth+0.015*depth*depth);float fog=clamp((depth-9.4)/3.5,0.0,1.0);
finalColor=vec4(c.rgb*light*(1.0-fog)+vec3(15,18,20)/255.0*fog,1);})";
constexpr const char* resolveFragment = R"(#version 330
in vec2 fragTexCoord;out vec4 finalColor;uniform sampler2D texture0;
void main(){ivec2 p=clamp(ivec2(fragTexCoord*vec2(500,282)),ivec2(0),ivec2(499,281))*2;
finalColor=(texelFetch(texture0,p,0)+texelFetch(texture0,p+ivec2(1,0),0)+texelFetch(texture0,p+ivec2(0,1),0)+texelFetch(texture0,p+ivec2(1,1),0))*0.25;})";
int anisotropy() {
    using String = const unsigned char* (*)(unsigned, unsigned);
    using Int = void (*)(unsigned, int*);
    using Float = void (*)(unsigned, float*);
    const auto string = reinterpret_cast<String>(rlGetProcAddress("glGetStringi"));
    const auto integer = reinterpret_cast<Int>(rlGetProcAddress("glGetIntegerv"));
    const auto real = reinterpret_cast<Float>(rlGetProcAddress("glGetFloatv"));
    int n = 0;
    if (string && integer)
        integer(0x821D, &n);
    for (int i = 0; i < n; ++i) {
        const auto* e = reinterpret_cast<const char*>(string(0x1F03, unsigned(i)));
        if (e && (!std::strcmp(e, "GL_EXT_texture_filter_anisotropic") ||
                  !std::strcmp(e, "GL_ARB_texture_filter_anisotropic"))) {
            float v = 1;
            if (real)
                real(0x84FF, &v);
            return int(std::min(4.f, v));
        }
    }
    return 1;
}
} // namespace
WorldRenderer::WorldRenderer(GpuResources& r, world_assets::Publication publication)
    : resources_(r), publication_(std::move(publication)) {
    if (publication_.textures.size() > world_assets::textureCapacity ||
        publication_.textureBytes > world_assets::textureBudget ||
        publication_.alphaBytes > world_assets::alphaBudget)
        throw std::runtime_error("World asset budget exceeded");
    std::uint64_t colorBytes = 0, maskBytes = 0;
    for (const auto& texture : publication_.textures) {
        if (texture.levels != world_assets::mipLevels(texture.width, texture.height) ||
            texture.bytes != world_assets::mipBytes(texture.width, texture.height) ||
            texture.alphaBytes != (texture.clamp ? std::uint64_t(texture.width) * texture.height : 0))
            throw std::invalid_argument("Malformed prepared world texture descriptor");
        if (texture.bytes > world_assets::textureBudget - colorBytes ||
            texture.alphaBytes > world_assets::alphaBudget - maskBytes)
            throw std::runtime_error("World texture payload budget exceeded");
        colorBytes += texture.bytes;
        maskBytes += texture.alphaBytes;
    }
    if (colorBytes != publication_.textureBytes || maskBytes != publication_.alphaBytes)
        throw std::invalid_argument("World publication byte totals differ");
    anisotropy_ = blobber::anisotropy();
    textures_.reserve(publication_.textures.size());
    alpha_.reserve(publication_.textures.size());
    for (unsigned i = 0; i < publication_.textures.size(); ++i) {
        const auto& t = publication_.textures[i];
        auto pixels = publication_.read(i);
        Image im{pixels.data(), int(t.width), int(t.height), int(t.levels),
                 PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
        textures_.emplace_back(r, r.createTexture(im));
        const auto texture = r.texture(textures_.back().get());
        SetTextureWrap(texture, t.clamp ? TEXTURE_WRAP_CLAMP : TEXTURE_WRAP_MIRROR_REPEAT);
        rlTextureParameters(texture.id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_NEAREST);
        rlTextureParameters(texture.id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_NEAREST_MIP_LINEAR);
        if (!t.clamp && anisotropy_ > 1)
            rlTextureParameters(texture.id, RL_TEXTURE_FILTER_ANISOTROPIC, anisotropy_);
        if (!t.clamp) {
            if (materialCount_ == materials_.size())
                throw std::runtime_error("World material capacity exceeded");
            materials_[materialCount_++] = textures_.back().get();
        }
        alpha_.push_back(publication_.read(i, true));
    }
    world_ = {r, r.createShader(vertex, fragment)};
    resolve_ = {r, r.createShader(nullptr, resolveFragment)};
    const auto shader = r.shader(world_.get());
    mvp_ = GetShaderLocation(shader, "mvp");
    view_ = GetShaderLocation(shader, "matView");
    forward_ = GetShaderLocation(shader, "forward");
    if (mvp_ < 0 || view_ < 0 || forward_ < 0)
        throw std::runtime_error("World shader interface unavailable");
    high_ = {r, r.createTarget(1000, 564)};
    if (cpuBytes() > 68u * world_assets::MiB)
        throw std::runtime_error("World shared CPU storage budget exceeded");
}
void WorldRenderer::stage(GeometryView geometry, const MovementState& state, world_render::Surfaces surfaces,
                          const std::vector<world_render::Surfaces>& overrides, world_render::Limits limits) {
    if (staged_)
        throw std::runtime_error("A world scene is already staged; commit or cancel it first");
    using Clock = std::chrono::steady_clock;
    const auto begin = Clock::now();
    auto candidate = std::make_unique<View>();
    candidate->cpu = world_render::prepare(geometry, surfaces, overrides, limits);
    for (const auto& range : candidate->cpu.ranges)
        if (range.material >= materialCount_)
            throw std::runtime_error("Scene references an unavailable world material");
    const auto built = Clock::now();
    candidate->mesh = {resources_, resources_.createMesh(candidate->cpu)};
    candidate->output = {resources_, resources_.createTarget(viewWidth, viewHeight)};
    const auto uploaded = Clock::now();
    render(*candidate, state);
    resources_.finishPreparation();
    const auto drawn = Clock::now();
    preparation = {std::chrono::duration<double, std::milli>(built - begin).count(),
                   std::chrono::duration<double, std::milli>(uploaded - built).count(),
                   std::chrono::duration<double, std::milli>(drawn - uploaded).count()};
    staged_ = std::move(candidate);
}
bool WorldRenderer::commit() noexcept {
    if (!staged_)
        return false;
    active_.swap(staged_);
    staged_.reset();
    return true;
}
WorldImage WorldRenderer::image() const noexcept {
    return active_ ? WorldImage{active_->output.get(), active_->revision} : WorldImage{};
}
WorldImage WorldRenderer::render(const MovementState& state) {
    if (!active_)
        throw std::logic_error("No active world scene");
    return render(*active_, state);
}
WorldImage WorldRenderer::render(View& v, const MovementState& state) {
    const auto camera = world_render::camera(state);
    stats = {};
    if (v.ready && v.quality == supersampling_ && world_render::sameCamera(camera, v.last)) {
        stats.cached = true;
        return {v.output.get(), v.revision};
    }
    world_render::cull(v.cpu, camera, draws_);
    stats.chunks = draws_.tested;
    const auto& target = resources_.target(supersampling_ ? high_.get() : v.output.get());
    const auto shader = resources_.shader(world_.get());
    // All checked resource lookups happen before entering a render pass.
    (void)resources_.mesh(v.mesh.get());
    BeginTextureMode(target);
    ClearBackground({15, 18, 20, 255});
    Camera3D rc{
        {camera.position.x, camera.position.y, camera.position.z},
        {camera.position.x + camera.forward.x, camera.position.y, camera.position.z + camera.forward.z},
        {0, 1, 0},
        60,
        CAMERA_PERSPECTIVE};
    BeginMode3D(rc);
    const double half = world_render::nearPlane * std::tan(84 * world_render::pi / 360);
    const auto projection =
        MatrixFrustum(-half, half, -viewHeight * .66 * world_render::nearPlane / world_render::focal(),
                      viewHeight * .34 * world_render::nearPlane / world_render::focal(),
                      world_render::nearPlane, world_render::farPlane);
    rlSetMatrixProjection(projection);
    rlDrawRenderBatchActive();
    rlEnableShader(shader.id);
    rlSetUniformMatrix(view_, rlGetMatrixModelview());
    rlSetUniformMatrix(mvp_, MatrixMultiply(rlGetMatrixModelview(), projection));
    rlSetUniform(forward_, &camera.forward, SHADER_UNIFORM_VEC3, 1);
    rlActiveTextureSlot(0);
    rlEnableDepthTest();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
    for (unsigned i = 0; i < draws_.count; ++i) {
        const auto& range = v.cpu.ranges[draws_.ranges[i]];
        rlEnableTexture(resources_.texture(materials_[range.material]).id);
        resources_.draw(v.mesh.get(), range.first, range.count);
        ++stats.draws;
    }
    rlDisableTexture();
    rlDisableShader();
    rlDisableVertexArray();
    rlDisableVertexBuffer();
    rlDisableVertexBufferElement();
    EndMode3D();
    EndTextureMode();
    if (supersampling_) {
        BeginTextureMode(resources_.target(v.output.get()));
        ClearBackground(BLANK);
        BeginShaderMode(resources_.shader(resolve_.get()));
        DrawTexturePro(target.texture, {0, 0, 1000, -564}, {0, 0, 500, 282}, {0, 0}, 0, WHITE);
        EndShaderMode();
        EndTextureMode();
        ++stats.draws;
    }
    rlActiveTextureSlot(0);
    rlEnableDepthMask();
    rlDisableDepthTest();
    rlEnableBackfaceCulling();
    rlSetBlendMode(BLEND_ALPHA);
    v.last = camera;
    v.quality = supersampling_;
    v.ready = true;
    v.revision = ++revision_;
    return {v.output.get(), v.revision};
}
std::size_t WorldRenderer::cpuBytes() const noexcept {
    std::size_t n = sizeof(*this) + textures_.capacity() * sizeof(GpuLease<TextureHandle>) +
                    alpha_.capacity() * sizeof(std::vector<unsigned char>);
    for (const auto& a : alpha_)
        n += a.capacity();
    n += publication_.textures.capacity() * sizeof(world_assets::Texture) +
         publication_.anchors.capacity() * sizeof(world_assets::Anchor) +
         publication_.appearances.capacity() * sizeof(world_assets::Appearance);
    for (const auto& t : publication_.textures)
        n += t.name.capacity();
    for (const auto& a : publication_.appearances)
        n += a.id.capacity();
    if (active_)
        n += sizeof(View) + active_->cpu.bytes();
    if (staged_)
        n += sizeof(View) + staged_->cpu.bytes();
    return n;
}
std::size_t WorldRenderer::gpuBytes() const noexcept {
    std::size_t n = publication_.textureBytes + 1000u * 564 * 8;
    if (active_)
        n += active_->cpu.meshBytes() + 500u * 282 * 8;
    if (staged_)
        n += staged_->cpu.meshBytes() + 500u * 282 * 8;
    return n;
}
} // namespace blobber
