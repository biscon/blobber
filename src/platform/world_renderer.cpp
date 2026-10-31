#include "platform/world_renderer.hpp"
#include "core/preparation.hpp"
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
in vec3 vertexPosition;in vec2 vertexTexCoord;in vec3 vertexNormal;layout(location=3) in float vertexShade;
uniform mat4 mvp;uniform mat4 matView;uniform mat4 matModel;
out vec2 uv;out vec3 normal;out float depth;out vec3 worldPosition;out float shade;
void main(){uv=vertexTexCoord;normal=normalize(mat3(matModel)*vertexNormal);vec4 p=matModel*vec4(vertexPosition,1);worldPosition=p.xyz;shade=vertexShade;depth=-(matView*p).z;gl_Position=mvp*p;})";
constexpr const char* fragment = R"(#version 330
in vec2 uv;in vec3 normal;in float depth;in vec3 worldPosition;in float shade;out vec4 finalColor;
uniform float daylight;uniform sampler2D texture0;uniform vec3 forward;uniform int idPass;uniform vec4 idColor;uniform int uvMode;uniform float worldScale;uniform vec3 horizon;
void main(){if(idPass!=0){finalColor=idColor;return;}vec2 mapped=uv;
if(uvMode==1) mapped=abs(normal.y)>0.5?worldPosition.xz:vec2(abs(normal.x)>0.5?worldPosition.z:worldPosition.x,1.0-worldPosition.y);
if(uvMode==2) mapped=vec2(0.02+uv.x*0.035,0.25+uv.y*0.4);
if(uvMode<2) mapped*=worldScale;
vec4 c=texture(texture0,mapped);
float face=normal.y>0.5?0.85:(normal.y< -0.5?0.58:0.78+0.22*abs(dot(normal,forward)));
float light=mix(face/(1.0+0.115*depth+0.015*depth*depth),mix(0.8,1.0,face)/(1.0+0.018*depth+0.0015*depth*depth),daylight);float fog=clamp((depth-mix(9.4,10.0,daylight))/mix(3.5,4.5,daylight),0.0,1.0);
finalColor=vec4((c.rgb*light*(1.0-fog)+mix(vec3(15,18,20)/255.0,horizon,daylight)*fog)*shade,1);})";
constexpr const char* billboardFragment = R"(#version 330
in vec2 uv;in float depth;out vec4 finalColor;
uniform float daylight;uniform vec3 horizon;uniform sampler2D texture0;uniform int kind;uniform int threshold;uniform int flash;uniform vec4 idColor;
void main(){
float fog=clamp((depth-mix(9.4,10.0,daylight))/mix(3.5,4.5,daylight),0.0,1.0);
if(kind==3){vec2 p=(uv-vec2(0.5))*2.0;float f=max(0.0,1.0-dot(p,p));finalColor=vec4(0,0,0,0.32*f*f*(1.0-fog));return;}
if(kind==5){vec4 c=texture(texture0,uv)*idColor;float light=mix(1.0/(1.0+0.115*depth+0.015*depth*depth),1.0/(1.0+0.018*depth+0.0015*depth*depth),daylight);
finalColor=vec4(c.rgb*idColor.a*light*(1.0-fog)+mix(vec3(15,18,20)/255.0,horizon,daylight)*fog*c.a,c.a);return;}
ivec2 size=textureSize(texture0,0);ivec2 p=clamp(ivec2(floor(uv*vec2(size))),ivec2(0),size-1);
float base=texelFetch(texture0,p,0).a;bool covered=base*255.0+0.01>=float(threshold);
vec4 c=texture(texture0,uv);
if(kind==2){if(covered||c.a<=0.001)discard;}else{if(!covered)discard;}
if(kind==4){finalColor=idColor;return;}
float light=mix(1.0/(1.0+0.115*depth+0.015*depth*depth),1.0/(1.0+0.018*depth+0.0015*depth*depth),daylight);
finalColor=vec4(flash!=0?vec3(c.a):c.rgb*light*(1.0-fog)+mix(vec3(15,18,20)/255.0,horizon,daylight)*fog*c.a,c.a);
})";
constexpr const char* skyFragment = R"(#version 330
out vec4 finalColor;uniform vec2 viewport;uniform vec3 forward;uniform vec3 right;uniform sampler2D panorama;
void main(){vec2 p=gl_FragCoord.xy/viewport;float f=250.0/tan(radians(42.0));
vec3 ray=normalize(forward+right*((p.x-0.5)*500.0/f)+vec3(0,(p.y-0.66)*282.0/f,0));
vec2 st=vec2(fract(atan(ray.x,-ray.z)/6.28318530718+0.5),acos(clamp(ray.y,-1.0,1.0))/3.14159265359);
finalColor=vec4(texture(panorama,st).rgb,1);})";
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
    : WorldRenderer(r, prepareCpu([&] {
                        preparationPoint("Preparing dungeon artwork");
                        return std::make_shared<const world_assets::CpuAssets>(std::move(publication));
                    })) {}
WorldRenderer::WorldRenderer(GpuResources& r, std::shared_ptr<const world_assets::CpuAssets> assets)
    : resources_(r), assets_(std::move(assets)),
      publication_(assets_ ? assets_->publication : throw std::invalid_argument("Missing world CPU assets")) {
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
    for (unsigned i = 0; i < publication_.textures.size(); ++i) {
        const auto& t = publication_.textures[i];
        preparationPoint("Loading dungeon artwork");
        auto pixels = prepareCpu([&] {
            preparationPoint("Loading dungeon artwork");
            return publication_.read(i);
        });
        Image im{pixels.data(), int(t.width), int(t.height), int(t.levels),
                 PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
        textures_.emplace_back(r, r.createTexture(im));
        const auto texture = r.texture(textures_.back().get());
        SetTextureWrap(texture, t.clamp ? TEXTURE_WRAP_CLAMP : TEXTURE_WRAP_MIRROR_REPEAT);
        rlTextureParameters(texture.id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_NEAREST);
        rlTextureParameters(texture.id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_NEAREST_MIP_LINEAR);
        if (!t.clamp && anisotropy_ > 1)
            rlTextureParameters(texture.id, RL_TEXTURE_FILTER_ANISOTROPIC, anisotropy_);
    }
    world_ = {r, r.createShader(vertex, fragment)};
    sky_ = {r, r.createShader(nullptr, skyFragment)};
    resolve_ = {r, r.createShader(nullptr, resolveFragment)};
    const auto shader = r.shader(world_.get());
    mvp_ = GetShaderLocation(shader, "mvp");
    view_ = GetShaderLocation(shader, "matView");
    forward_ = GetShaderLocation(shader, "forward");
    model_ = GetShaderLocation(shader, "matModel");
    uvMode_ = GetShaderLocation(shader, "uvMode");
    idMode_ = GetShaderLocation(shader, "idPass");
    idColor_ = GetShaderLocation(shader, "idColor");
    if (mvp_ < 0 || view_ < 0 || forward_ < 0)
        throw std::runtime_error("World shader interface unavailable");
    high_ = {r, r.createTarget(1000, 564)};
    if (assets_->bytes() > 68u * world_assets::MiB)
        throw std::runtime_error("World shared CPU storage budget exceeded");
}
void WorldRenderer::stage(GeometryView geometry, const MovementState& state, world_render::Surfaces surfaces,
                          const std::vector<world_render::Surfaces>& overrides, world_render::Limits limits) {
    if (staged_)
        throw std::runtime_error("A world scene is already staged; commit or cancel it first");
    using Clock = std::chrono::steady_clock;
    const auto begin = Clock::now();
    auto candidate = std::make_unique<View>();
    candidate->pack = assets_->pack(geometry.base().definition().assetPack);
    for (unsigned i = 0; i < candidate->pack.materialCount; ++i)
        candidate->materials[i] = textures_.at(candidate->pack.materials[i]).get();
    if (!publication_.packs.empty() && candidate->pack.materialCount >= 5) {
        surfaces.stairWall = 3;
        surfaces.stairFloor = 4;
    }
    candidate->cpu = prepareCpu([&] {
        preparationPoint("Preparing dungeon");
        return world_render::prepare(geometry, surfaces, overrides, limits);
    });
    for (const auto& range : candidate->cpu.ranges)
        if (range.material >= candidate->pack.materialCount)
            throw std::runtime_error("Scene references an unavailable world material");
    const auto built = Clock::now();
    preparationPoint("Preparing dungeon");
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
    if (cpuBytes() > 352u * world_assets::MiB || gpuBytes() > 352u * world_assets::MiB) {
        staged_.reset();
        throw std::runtime_error("World active/staged budget exceeded");
    }
}
void WorldRenderer::prepareBillboards() {
    if (billboard_)
        return;
    GpuLease<ShaderHandle> shader(resources_, resources_.createShader(vertex, billboardFragment));
    GpuLease<MeshHandle> quad(resources_, resources_.createMesh(world_render::billboardMesh()));
    GpuLease<MeshHandle> proxy(resources_, resources_.createMesh(world_render::proxyMesh()));
    constexpr const char* names[]{"mvp", "matView", "matModel", "kind", "threshold", "flash", "idColor"};
    std::array<int, 7> locations{};
    for (unsigned i = 0; i < locations.size(); ++i) {
        locations[i] = GetShaderLocation(resources_.shader(shader.get()), names[i]);
        if (locations[i] < 0)
            throw std::runtime_error("Billboard shader interface unavailable");
    }
    billboardLocations_ = locations;
    billboard_ = std::move(shader);
    quad_ = std::move(quad);
    proxy_ = std::move(proxy);
}
void WorldRenderer::stage(const world_render::FrameInput& in) {
    if (staged_)
        throw std::runtime_error("A world scene is already staged; commit or cancel it first");
    const bool hadBillboards = bool(billboard_);
    try {
        prepareBillboards();
        stage(in.geometry, in.movement);
        staged_->visibility = prepareCpu([&] {
            preparationPoint("Preparing dungeon");
            return std::make_unique<world_render::VisibilityPipeline>(staged_->cpu, *assets_);
        });
        prepareCpu([&] { staged_->visibility->update(in); });
        if (in.attacks || in.area) {
            staged_->effects = std::make_unique<world_render::EffectsPipeline>(*assets_);
            staged_->effects->update(in, staged_->visibility->frame());
            if (staged_->visibility->bytes() + sizeof(world_render::EffectsPipeline) >
                world_render::viewScratchBudget)
                throw std::runtime_error("Integrated world scratch budget exceeded");
        }
        render(*staged_, in.movement, &staged_->visibility->frame(), {},
               staged_->effects ? &staged_->effects->frame() : nullptr);
        resources_.finishPreparation();
        staged_->publishedPicking = staged_->visibility->picking();
        if (cpuBytes() > 352u * world_assets::MiB || gpuBytes() > 352u * world_assets::MiB)
            throw std::runtime_error("World active/staged budget exceeded");
    } catch (...) {
        staged_.reset();
        if (!hadBillboards) {
            proxy_.reset();
            quad_.reset();
            billboard_.reset();
        }
        throw;
    }
}
WorldImage WorldRenderer::render(const world_render::FrameInput& in) {
    if (!active_ || !active_->visibility)
        throw std::logic_error("No active interactive world scene");
    return renderInput(*active_, in);
}
WorldImage WorldRenderer::renderStaged(const world_render::FrameInput& in) {
    if (!staged_ || !staged_->visibility)
        throw std::logic_error("No staged interactive world");
    return renderInput(*staged_, in);
}
WorldImage WorldRenderer::renderInput(View& v, const world_render::FrameInput& in) {
    using Clock = std::chrono::steady_clock;
    const auto start = measure_ ? Clock::now() : Clock::time_point{};
    v.visibility->update(in);
    const auto prepared = measure_ ? Clock::now() : Clock::time_point{};
    if ((in.attacks || in.area) && !v.effects)
        throw std::logic_error("Effects require preparation before frame processing");
    if (v.effects)
        v.effects->update(in, v.visibility->frame());
    const auto result =
        render(v, in.movement, &v.visibility->frame(), {}, v.effects ? &v.effects->frame() : nullptr);
    if (measure_) {
        stats.visibilityMs = std::chrono::duration<double, std::milli>(prepared - start).count();
        stats.submissionMs = std::chrono::duration<double, std::milli>(Clock::now() - prepared).count();
    }
    v.publishedPicking = v.visibility->picking();
    return result;
}
void WorldRenderer::diagnosticIds(TargetHandle destination) {
    if (!active_ || !active_->visibility)
        throw std::logic_error("No world visibility snapshot for diagnostic IDs");
    const auto& t = resources_.target(destination);
    if (t.texture.width != viewWidth || t.texture.height != viewHeight ||
        t.id == resources_.target(active_->output.get()).id || t.id == resources_.target(high_.get()).id)
        throw std::invalid_argument("Diagnostic IDs require a separate logical target");
    render(*active_, active_->movement, &active_->visibility->frame(), destination);
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
    if (active_->visibility)
        throw std::logic_error("Interactive world rendering requires matching frame input");
    return render(*active_, state);
}
WorldImage WorldRenderer::render(View& v, const MovementState& state, const world_render::WorldFrame* frame,
                                 TargetHandle ids, const world_render::EffectFrame* effects) {
    const auto camera = v.cpu.map ? world_render::camera(state, *v.cpu.map) : world_render::camera(state);
    stats = {};
    if (!ids && v.ready && v.contentRevision == (frame ? frame->revision : 0) &&
        v.effectRevision == (effects ? effects->revision : 0) && v.quality == supersampling_ &&
        world_render::sameCamera(camera, v.last)) {
        stats.cached = true;
        return {v.output.get(), v.revision};
    }
    world_render::cull(v.cpu, camera, draws_);
    stats.chunks = draws_.tested;
    const auto& target = resources_.target(ids ? ids : supersampling_ ? high_.get() : v.output.get());
    const auto shader = resources_.shader(world_.get());
    // All checked resource lookups happen before entering a render pass.
    (void)resources_.mesh(v.mesh.get());
    BeginTextureMode(target);
    ClearBackground(ids ? Color{255, 255, 0, 255} : Color{15, 18, 20, 255});
    const float ambient = v.cpu.map ? world_render::daylight(state, *v.cpu.map) : 0;
    if (v.cpu.outdoor && !ids) {
        const auto sky = resources_.shader(sky_.get());
        const float viewport[]{float(target.texture.width), float(target.texture.height)};
        SetShaderValue(sky, GetShaderLocation(sky, "viewport"), viewport, SHADER_UNIFORM_VEC2);
        SetShaderValue(sky, GetShaderLocation(sky, "forward"), &camera.forward, SHADER_UNIFORM_VEC3);
        SetShaderValue(sky, GetShaderLocation(sky, "right"), &camera.right, SHADER_UNIFORM_VEC3);
        const auto skyTexture = resources_.texture(textures_[v.pack.sky].get());
        rlTextureParameters(skyTexture.id, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_REPEAT);
        rlTextureParameters(skyTexture.id, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);
        SetTextureFilter(skyTexture, TEXTURE_FILTER_BILINEAR);
        BeginShaderMode(sky);
        SetShaderValueTexture(sky, GetShaderLocation(sky, "panorama"), skyTexture);
        DrawRectangle(0, 0, target.texture.width, target.texture.height, WHITE);
        EndShaderMode();
    }
    SetShaderValue(shader, GetShaderLocation(shader, "daylight"), &ambient, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "worldScale"), &v.pack.worldScale, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "horizon"), v.pack.horizon.data(), SHADER_UNIFORM_VEC3);
    if (billboard_) {
        const auto b = resources_.shader(billboard_.get());
        SetShaderValue(b, GetShaderLocation(b, "daylight"), &ambient, SHADER_UNIFORM_FLOAT);
        SetShaderValue(b, GetShaderLocation(b, "horizon"), v.pack.horizon.data(), SHADER_UNIFORM_VEC3);
    }
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
    const auto identity = MatrixIdentity();
    rlSetUniformMatrix(model_, identity);
    const int idPass = ids ? 1 : 0;
    const float emptyColor[]{1, 1, 0, 1};
    rlSetUniform(idMode_, &idPass, SHADER_UNIFORM_INT, 1);
    rlSetUniform(idColor_, emptyColor, SHADER_UNIFORM_VEC4, 1);
    const int staticUv = 0;
    rlSetUniform(uvMode_, &staticUv, SHADER_UNIFORM_INT, 1);
    rlActiveTextureSlot(0);
    rlEnableDepthTest();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
    for (unsigned i = 0; i < draws_.count; ++i) {
        const auto& range = v.cpu.ranges[draws_.ranges[i]];
        const float color[]{float(range.id & 255) / 255, float(range.id >> 8) / 255, 0, 1};
        rlSetUniform(idColor_, color, SHADER_UNIFORM_VEC4, 1);
        rlEnableTexture(resources_.texture(v.materials[range.material]).id);
        resources_.draw(v.mesh.get(), range.first, range.count);
        ++stats.draws;
    }
    if (frame) {
        for (unsigned i = 0; i < frame->doorCount; ++i) {
            const auto& box = frame->doors[i];
            auto model = MatrixIdentity();
            model.m0 = box.x.x;
            model.m1 = box.x.y;
            model.m2 = box.x.z;
            model.m4 = box.y.x;
            model.m5 = box.y.y;
            model.m6 = box.y.z;
            model.m8 = box.z.x;
            model.m9 = box.z.y;
            model.m10 = box.z.z;
            model.m12 = box.origin.x;
            model.m13 = box.origin.y;
            model.m14 = box.origin.z;
            rlSetUniformMatrix(model_, model);
            const int mode = box.worldMapped ? 1 : 3;
            rlSetUniform(uvMode_, &mode, SHADER_UNIFORM_INT, 1);
            const auto id = box.id;
            const float color[]{float(id & 255) / 255, float(id >> 8) / 255, 0, 1};
            rlSetUniform(idColor_, color, SHADER_UNIFORM_VEC4, 1);
            rlEnableTexture(resources_.texture(textures_[box.texture].get()).id);
            rlDisableBackfaceCulling();
            resources_.draw(proxy_.get(), 0, box.worldMapped ? 36 : 12);
            ++stats.draws;
            if (!box.worldMapped) {
                const int edgeMode = 2;
                rlSetUniform(uvMode_, &edgeMode, SHADER_UNIFORM_INT, 1);
                rlEnableTexture(resources_.texture(textures_[box.edgeTexture].get()).id);
                resources_.draw(proxy_.get(), 12, 24);
                ++stats.draws;
            }
        }
        const auto vp = MatrixMultiply(rlGetMatrixModelview(), projection);
        const auto view = rlGetMatrixModelview();
        const auto a = MatrixToFloatV(vp), b = MatrixToFloatV(view);
        if (effects && !ids)
            drawEffects(*effects, a.v, b.v, true);
        drawBillboards(*frame, a.v, b.v, bool(ids), &v.cpu);
        if (effects && !ids)
            drawEffects(*effects, a.v, b.v, false);
    }
    rlDisableTexture();
    rlDisableShader();
    rlDisableVertexArray();
    rlDisableVertexBuffer();
    rlDisableVertexBufferElement();
    EndMode3D();
    EndTextureMode();
    if (!ids && supersampling_) {
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
    if (ids)
        return {ids, frame ? frame->revision : 1};
    v.effectRevision = effects ? effects->revision : 0;
    v.contentRevision = frame ? frame->revision : 0;
    v.movement = state;
    v.last = camera;
    v.quality = supersampling_;
    v.ready = true;
    v.revision = ++revision_;
    return {v.output.get(), v.revision};
}
void WorldRenderer::drawBillboards(const world_render::WorldFrame& frame, const float* vp, const float* view,
                                   bool ids, const world_render::Scene* scene) {
    Matrix projection{}, cameraView{};
    // raymath's field declaration order differs from its column-major float representation.
    auto matrix = [](const float* v) {
        return Matrix{v[0], v[4], v[8],  v[12], v[1], v[5], v[9],  v[13],
                      v[2], v[6], v[10], v[14], v[3], v[7], v[11], v[15]};
    };
    projection = matrix(vp);
    cameraView = matrix(view);
    const auto shader = resources_.shader(billboard_.get());
    rlSetBlendMode(ids ? BLEND_ALPHA : BLEND_ALPHA_PREMULTIPLY);
    rlEnableShader(shader.id);
    rlSetUniformMatrix(billboardLocations_[0], projection);
    rlSetUniformMatrix(billboardLocations_[1], cameraView);
    const int modelLoc = billboardLocations_[2], kindLoc = billboardLocations_[3],
              thresholdLoc = billboardLocations_[4], flashLoc = billboardLocations_[5],
              colorLoc = billboardLocations_[6];
    rlDisableBackfaceCulling();
    auto draw = [&](const Matrix& model, int kind, int threshold, bool flash, std::uint16_t id) {
        const int f = flash ? 1 : 0;
        const float color[]{float(id & 255) / 255, float(id >> 8) / 255, 0, 1};
        rlSetUniformMatrix(modelLoc, model);
        rlSetUniform(kindLoc, &kind, SHADER_UNIFORM_INT, 1);
        rlSetUniform(thresholdLoc, &threshold, SHADER_UNIFORM_INT, 1);
        rlSetUniform(flashLoc, &f, SHADER_UNIFORM_INT, 1);
        rlSetUniform(colorLoc, color, SHADER_UNIFORM_VEC4, 1);
        resources_.draw(quad_.get(), 0, 6);
        ++stats.draws;
    };
    if (scene)
        for (const auto& d : scene->decals) {
            const auto texture = d.motif ? frame.vineTexture : frame.crestTexture;
            rlEnableTexture(resources_.texture(textures_[texture].get()).id);
            const auto& a = d.vertices[0].position;
            const auto& b = d.vertices[1].position;
            const auto& c = d.vertices[3].position;
            auto m = MatrixIdentity();
            m.m0 = b.x - a.x;
            m.m1 = b.y - a.y;
            m.m2 = b.z - a.z;
            m.m4 = c.x - a.x;
            m.m5 = c.y - a.y;
            m.m6 = c.z - a.z;
            m.m12 = (a.x + b.x) * .5f;
            m.m13 = (a.y + b.y) * .5f;
            m.m14 = (a.z + b.z) * .5f;
            rlEnableDepthMask();
            draw(m, ids ? 4 : 1, 16, false, PickingBuffer::empty);
            if (!ids) {
                rlDisableDepthMask();
                draw(m, 2, 16, false, PickingBuffer::empty);
            }
        }
    if (!ids) {
        rlDisableDepthMask();
        for (unsigned i = 0; i < frame.count; ++i) {
            const auto& b = frame.billboards[i];
            if (!b.shadowRadius)
                continue;
            float floor = b.feet.y;
            const Cell under{int(std::floor(b.feet.x)), int(std::floor(b.feet.z))};
            if (scene && scene->map && !scene->map->solid(under))
                floor = elevation(scene->map->surface(under).floor);
            const auto model =
                MatrixMultiply(MatrixMultiply(MatrixScale(2 * b.shadowRadius, 2 * b.shadowRadius, 1),
                                              MatrixRotateX(world_render::pi / 2)),
                               MatrixTranslate(b.feet.x, floor + .001f, b.feet.z - b.shadowRadius));
            draw(model, 3, 1, false, 0);
        }
    }
    for (unsigned i = 0; i < frame.count; ++i) {
        const auto& b = frame.billboards[i];
        const auto foot = world_render::project(frame.camera, b.feet);
        const float scale = world_render::focal() / b.depth;
        if (foot.x + (1 - b.anchorX) * b.width * scale < 0 ||
            foot.x - b.anchorX * b.width * scale > viewWidth ||
            foot.y - b.anchorY * b.height * scale > viewHeight ||
            foot.y + (1 - b.anchorY) * b.height * scale < 0)
            continue;
        rlEnableTexture(resources_.texture(textures_[b.texture].get()).id);
        const float lateral = (.5f - b.anchorX) * b.width;
        const auto model = MatrixMultiply(
            MatrixMultiply(MatrixScale(b.width, b.height, 1), MatrixRotateY(-frame.camera.yaw)),
            MatrixTranslate(b.feet.x + frame.camera.right.x * lateral, b.feet.y + (b.anchorY - 1) * b.height,
                            b.feet.z + frame.camera.right.z * lateral));
        rlEnableDepthMask();
        draw(model, ids ? 4 : 1, b.id < NpcWorld::maxInstances ? 1 : 16, b.flash, b.id);
        if (!ids) {
            rlDisableDepthMask();
            draw(model, 2, b.id < NpcWorld::maxInstances ? 1 : 16, b.flash, b.id);
        }
    }
    rlEnableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);
}
void WorldRenderer::drawEffects(const world_render::EffectFrame& frame, const float* vp, const float* view,
                                bool ground) {
    auto matrix = [](const float* v) {
        return Matrix{v[0], v[4], v[8],  v[12], v[1], v[5], v[9],  v[13],
                      v[2], v[6], v[10], v[14], v[3], v[7], v[11], v[15]};
    };
    rlSetBlendMode(BLEND_ALPHA_PREMULTIPLY);
    rlEnableShader(resources_.shader(billboard_.get()).id);
    rlSetUniformMatrix(billboardLocations_[0], matrix(vp));
    rlSetUniformMatrix(billboardLocations_[1], matrix(view));
    const int kind = 5;
    rlSetUniform(billboardLocations_[3], &kind, SHADER_UNIFORM_INT, 1);
    rlEnableDepthTest();
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    for (unsigned i = 0; i < frame.count; ++i) {
        const auto& p = frame.packets[i];
        if (p.ground != ground)
            continue;
        const auto& a = p.points[0];
        const auto& b = p.points[1];
        const auto& d = p.points[3];
        auto m = MatrixIdentity();
        m.m0 = b.x - a.x;
        m.m1 = b.y - a.y;
        m.m2 = b.z - a.z;
        m.m4 = d.x - a.x;
        m.m5 = d.y - a.y;
        m.m6 = d.z - a.z;
        // The shader normal is irrelevant to the effect branch; keep a nonzero basis.
        m.m8 = 0;
        m.m9 = 0;
        m.m10 = 1;
        m.m12 = (a.x + b.x) * .5f;
        m.m13 = (a.y + b.y) * .5f;
        m.m14 = (a.z + b.z) * .5f;
        rlSetUniformMatrix(billboardLocations_[2], m);
        rlSetUniform(billboardLocations_[6], p.tint.data(), SHADER_UNIFORM_VEC4, 1);
        rlEnableTexture(resources_.texture(textures_[p.texture].get()).id);
        resources_.draw(quad_.get(), 0, 6);
        ++stats.draws;
    }
    rlEnableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);
}
std::size_t WorldRenderer::cpuBytes() const noexcept {
    std::size_t n = sizeof(*this) + assets_->bytes() + textures_.capacity() * sizeof(GpuLease<TextureHandle>);
    if (active_ && active_->cpu.map)
        n += active_->cpu.map->definition().bytes();
    if (staged_ && staged_->cpu.map && (!active_ || staged_->cpu.map != active_->cpu.map))
        n += staged_->cpu.map->definition().bytes();
    for (const auto* v : {active_.get(), staged_.get()})
        if (v) {
            n += sizeof(View) + v->cpu.bytes();
            if (v->visibility)
                n += v->visibility->bytes();
            if (v->effects)
                n += sizeof(world_render::EffectsPipeline);
        }
    return n;
}
std::size_t WorldRenderer::gpuBytes() const noexcept {
    std::size_t n =
        publication_.textureBytes + 1000u * 564 * 8 + (quad_ ? 4u * 32 + 6u * 4 + 24u * 32 + 36u * 4 : 0);
    if (active_)
        n += active_->cpu.meshBytes() + 500u * 282 * 8;
    if (staged_)
        n += staged_->cpu.meshBytes() + 500u * 282 * 8;
    return n;
}
} // namespace blobber
