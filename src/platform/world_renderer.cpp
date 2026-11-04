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
uniform float daylight;uniform sampler2D texture0;uniform vec3 forward;uniform int idPass;uniform vec4 idColor;uniform int uvMode;uniform float worldScale;uniform vec3 horizon;uniform vec2 doorUv;
void main(){if(idPass!=0){finalColor=idColor;return;}vec2 mapped=uv;
if(uvMode==1) mapped=abs(normal.y)>0.5?worldPosition.xz:vec2(abs(normal.x)>0.5?worldPosition.z:worldPosition.x,1.0-worldPosition.y);
if(uvMode==2) mapped=vec2(0.02+uv.x*0.035,0.25+uv.y*0.4);
if(uvMode==3) mapped=vec2(1.0-(1.0-uv.x)*doorUv.x,0.5+(uv.y-0.5)*doorUv.y);
if(uvMode<2) mapped*=worldScale;
vec4 c=texture(texture0,mapped);
float face=normal.y>0.5?0.85:(normal.y< -0.5?0.58:0.78+0.22*abs(dot(normal,forward)));
if(uvMode==4) face=1.0;
float light=mix(face/(1.0+0.115*depth+0.015*depth*depth),mix(0.8,1.0,face)/(1.0+0.018*depth+0.0015*depth*depth),daylight);float fog=clamp((depth-mix(9.4,10.0,daylight))/mix(3.5,4.5,daylight),0.0,1.0);
finalColor=vec4((c.rgb*light*(1.0-fog)+mix(vec3(15,18,20)/255.0,horizon,daylight)*fog)*shade,1);})";
constexpr const char* billboardFragment = R"(#version 330
in vec2 uv;in float depth;in vec3 worldPosition;out vec4 finalColor;
uniform float daylight;uniform vec3 horizon;uniform sampler2D texture0;uniform int kind;uniform int threshold;uniform int flash;uniform vec4 idColor;
uniform int clipCount;uniform vec4 clipRects[136];
uniform int leafCount;uniform vec4 leafData[96];uniform vec3 eye;
bool slab(float at,float delta,float low,float high,inout float enter,inout float leave){
if(abs(delta)<1e-6)return at>=low&&at<=high;
float a=(low-at)/delta,b=(high-at)/delta;enter=max(enter,min(a,b));leave=min(leave,max(a,b));return leave-enter>1e-6;}
bool leafHidden(){vec3 ray=(worldPosition-eye)/depth;
for(int i=0;i<leafCount;++i){vec4 o=leafData[i*3],x=leafData[i*3+1],z=leafData[i*3+2];
if(x.w==1.0)continue;vec3 d=eye-o.xyz;float enter=0.025,leave=16.0;
if(slab(dot(d,x.xyz),dot(ray,x.xyz),0,1,enter,leave)&&slab(d.y,ray.y,0,o.w,enter,leave)&&
slab(dot(d,z.xyz),dot(ray,z.xyz),0,1,enter,leave)&&(x.w==2.0||enter<=depth+1e-5))return true;}return false;}

void main(){
float fog=clamp((depth-mix(9.4,10.0,daylight))/mix(3.5,4.5,daylight),0.0,1.0);
if(kind==3){vec2 p=(uv-vec2(0.5))*2.0;float f=max(0.0,1.0-dot(p,p));finalColor=vec4(0,0,0,0.32*f*f*(1.0-fog));return;}
if(kind==5){if(leafHidden())discard;vec4 c=texture(texture0,uv)*idColor;float light=mix(1.0/(1.0+0.115*depth+0.015*depth*depth),1.0/(1.0+0.018*depth+0.0015*depth*depth),daylight);
finalColor=vec4(c.rgb*idColor.a*light*(1.0-fog)+mix(vec3(15,18,20)/255.0,horizon,daylight)*fog*c.a,c.a);return;}
if(leafHidden())discard;
for(int i=0;i<clipCount;++i){vec4 r=clipRects[i];if(uv.x>=r.x&&uv.y>=r.y&&uv.x<=r.z&&uv.y<=r.w)discard;}
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
constexpr const char* compositeVertex = R"(#version 330
in vec3 vertexPosition;
void main(){gl_Position=vec4(vertexPosition.x*2.0,vertexPosition.y*2.0-1.0,0,1);})";
constexpr const char* compositeFragment = R"(#version 330
uniform sampler2D bodies;uniform sampler2D bodyDepth;out vec4 finalColor;
void main(){ivec2 p=ivec2(gl_FragCoord.xy);vec4 c=texelFetch(bodies,p,0);
if(c.a<=0.0)discard;finalColor=c;gl_FragDepth=texelFetch(bodyDepth,p,0).r;})";
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
WorldRenderer::WorldRenderer(GpuResources& r, world_assets::Publication publication, bool supersampling)
    : WorldRenderer(r, prepareCpu([&] {
                        preparationPoint("Preparing dungeon artwork");
                        return std::make_shared<const world_assets::CpuAssets>(std::move(publication));
                    }),
                    supersampling) {}
WorldRenderer::WorldRenderer(GpuResources& r, std::shared_ptr<const world_assets::CpuAssets> assets,
                             bool supersampling)
    : resources_(r), assets_(std::move(assets)),
      publication_(assets_ ? assets_->publication : throw std::invalid_argument("Missing world CPU assets")) {
    anisotropy_ = blobber::anisotropy();
    textures_.resize(publication_.textures.size());
    supersampling_ = supersampling;
    world_ = {r, r.createShader(vertex, fragment)};
    sky_ = {r, r.createShader(nullptr, skyFragment)};
    resolve_ = {r, r.createShader(nullptr, resolveFragment)};
    const auto shader = r.shader(world_.get());
    mvp_ = GetShaderLocation(shader, "mvp");
    view_ = GetShaderLocation(shader, "matView");
    forward_ = GetShaderLocation(shader, "forward");
    model_ = GetShaderLocation(shader, "matModel");
    uvMode_ = GetShaderLocation(shader, "uvMode");
    doorUv_ = GetShaderLocation(shader, "doorUv");
    idMode_ = GetShaderLocation(shader, "idPass");
    idColor_ = GetShaderLocation(shader, "idColor");
    if (mvp_ < 0 || view_ < 0 || forward_ < 0)
        throw std::runtime_error("World shader interface unavailable");
    creaturesLow_ = {r, r.createTarget(500, 282, true)};
    creatureComposite_ = {r, r.createShader(compositeVertex, compositeFragment)};
    this->supersampling(supersampling_);
    if (assets_->bytes() > world_assets::worldCpuBudget)
        throw std::runtime_error("World shared CPU storage budget exceeded");
}
void WorldRenderer::stage(GeometryView geometry, const MovementState& state, world_render::Surfaces surfaces,
                          const std::vector<world_render::Surfaces>& overrides, world_render::Limits limits) {
    stagePrepared(geometry, state, surfaces, overrides, limits,
                  assets_->requirements(geometry.base().definition().assetPack));
}
void WorldRenderer::stagePrepared(GeometryView geometry, const MovementState& state,
                                  world_render::Surfaces surfaces,
                                  const std::vector<world_render::Surfaces>& overrides,
                                  world_render::Limits limits,
                                  const world_assets::AssetRequirements& required) {
    if (staged_)
        throw std::runtime_error("A world scene is already staged; commit or cancel it first");
    using Clock = std::chrono::steady_clock;
    const auto begin = Clock::now();
    auto candidate = std::make_unique<View>();
    candidate->assets = prepareCpu([&] { return assets_->acquire(required); });
    candidate->pack = assets_->pack(geometry.base().definition().assetPack);
    if (!publication_.packs.empty() && candidate->pack.materialCount >= 5) {
        surfaces.stairWall = 3;
        surfaces.stairFloor = 4;
    }
    candidate->cpu = prepareCpu([&] {
        preparationPoint("Preparing dungeon");
        return world_render::prepare(geometry, surfaces, overrides, limits, assets_.get());
    });
    for (const auto& range : candidate->cpu.ranges)
        if (range.material >= candidate->pack.materialCount)
            throw std::runtime_error("Scene references an unavailable world material");
    const auto built = Clock::now();
    acquireTextures(*candidate, required);
    for (unsigned i = 0; i < candidate->pack.materialCount; ++i)
        candidate->materials[i] = candidate->textures.at(candidate->pack.materials[i]);
    preparationPoint("Preparing dungeon");
    candidate->mesh = {resources_, resources_.createMesh(candidate->cpu)};
    candidate->output = {resources_, resources_.createTarget(viewWidth, viewHeight)};
    const auto uploaded = Clock::now();
    render(*candidate, state);
    resources_.finishPreparation();
    peakApplicationGpu_ = std::max(peakApplicationGpu_, resources_.counts().bytes);
    const auto drawn = Clock::now();
    preparation = {std::chrono::duration<double, std::milli>(built - begin).count(),
                   std::chrono::duration<double, std::milli>(uploaded - built).count(),
                   std::chrono::duration<double, std::milli>(drawn - uploaded).count()};
    staged_ = std::move(candidate);
    if (cpuBytes() > world_assets::worldCpuBudget || gpuBytes() > world_assets::worldGpuBudget) {
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
    static_assert(world_render::billboardClipCapacity == 136 && fixtureCapacity == 32,
                  "Keep shader clipping/leaf capacities in sync");
    static_assert(4 * world_render::billboardClipCapacity + 12 * fixtureCapacity + 32 <= 1024,
                  "Fragment uniforms must fit OpenGL 3.3 minimum capacity");
    constexpr const char* names[]{"mvp",     "matView",   "matModel",     "kind",      "threshold",   "flash",
                                  "idColor", "clipCount", "clipRects[0]", "leafCount", "leafData[0]", "eye"};
    std::array<int, 12> locations{};
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
        stagePrepared(in.geometry, in.movement, {}, {}, {},
                      assets_->requirements(in.geometry.base().definition().assetPack,
                                            in.actors ? &in.actors->snapshot() : nullptr, true));
        staged_->visibility = prepareCpu([&] {
            preparationPoint("Preparing dungeon");
            return std::make_unique<world_render::VisibilityPipeline>(staged_->cpu, *assets_);
        });
        prepareCpu([&] { staged_->visibility->update(in, false); });
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
        if (cpuBytes() > world_assets::worldCpuBudget || gpuBytes() > world_assets::worldGpuBudget)
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
    v.visibility->update(in, false);
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
    active_->visibility->rasterForDiagnostics();
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
    const auto targetHandle = ids ? ids : supersampling_ ? high_.get() : v.output.get();
    const auto& target = resources_.target(targetHandle);
    const auto creatureHandle = !ids && supersampling_ ? creaturesHigh_.get() : creaturesLow_.get();
    const auto& creatureTarget = resources_.target(creatureHandle);
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
        const auto skyTexture = resources_.texture(v.textures[v.pack.sky]);
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
        if (range.variant != noContainer &&
            (frame ? frame->containerOpened[range.id - PickingBuffer::containerBase] : false) !=
                bool(range.variant))
            continue;
        const int rangeUv = range.painted ? 4 : 0;
        rlSetUniform(uvMode_, &rangeUv, SHADER_UNIFORM_INT, 1);
        const float color[]{float(range.id & 255) / 255, float(range.id >> 8) / 255, 0, 1};
        rlSetUniform(idColor_, color, SHADER_UNIFORM_VEC4, 1);
        rlEnableTexture(resources_.texture(v.materials[range.material]).id);
        resources_.draw(v.mesh.get(), range.first, range.count);
        ++stats.draws;
    }
    if (frame) {
        const auto vp = MatrixMultiply(rlGetMatrixModelview(), projection);
        const auto view = rlGetMatrixModelview();
        const auto a = MatrixToFloatV(vp), b = MatrixToFloatV(view);
        for (unsigned doorPhase = 0; doorPhase < 2; ++doorPhase) {
            if (doorPhase == 1) {
                drawBillboards(v, *frame, a.v, b.v, bool(ids), &v.cpu, BillboardPass::Scenery);
                // Preserve leaf-free scenery/nonbody depth for the transparent body layer.
                resources_.copyDepth(targetHandle, creatureHandle);
                rlEnableShader(shader.id);
                rlSetBlendMode(BLEND_ALPHA);
                rlEnableDepthMask();
            }
            for (unsigned i = 0; i < frame->doorCount; ++i) {
                const auto& box = frame->doors[i];
                if (box.leaf != bool(doorPhase))
                    continue;
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
                if (!box.worldMapped) {
                    const auto& texture = assets_->publication.textures[box.texture];
                    const auto span = world_render::doorUvSpan(box, texture.width, texture.height);
                    rlSetUniform(doorUv_, span.data(), SHADER_UNIFORM_VEC2, 1);
                }
                const auto id = box.id;
                const float color[]{float(id & 255) / 255, float(id >> 8) / 255, 0, 1};
                rlSetUniform(idColor_, color, SHADER_UNIFORM_VEC4, 1);
                rlEnableTexture(resources_.texture(v.textures[box.texture]).id);
                rlDisableBackfaceCulling();
                resources_.draw(proxy_.get(), 0, box.worldMapped ? 36 : 12);
                ++stats.draws;
                if (!box.worldMapped) {
                    const int edgeMode = 2;
                    rlSetUniform(uvMode_, &edgeMode, SHADER_UNIFORM_INT, 1);
                    rlEnableTexture(resources_.texture(v.textures[box.edgeTexture]).id);
                    resources_.draw(proxy_.get(), 12, 24);
                    ++stats.draws;
                }
            }
        }
        drawBillboards(v, *frame, a.v, b.v, bool(ids), &v.cpu, BillboardPass::LeafArt);
        if (effects && !ids)
            drawEffects(v, *effects, a.v, b.v, true, *frame);
        rlEnableFramebuffer(creatureTarget.id);
        // Clear colour only: the copied leaf-free depth belongs to this frame.
        rlClearColor(0, 0, 0, 0);
        using Clear = void (*)(unsigned);
        reinterpret_cast<Clear>(rlGetProcAddress("glClear"))(0x00004000);
        drawBillboards(v, *frame, a.v, b.v, bool(ids), &v.cpu, BillboardPass::Creatures);
        rlEnableFramebuffer(target.id);
        // Blend changes flush raylib's batch and can replace the current shader.
        rlSetBlendMode(ids ? BLEND_ALPHA : BLEND_ALPHA_PREMULTIPLY);
        const auto composite = resources_.shader(creatureComposite_.get());
        rlEnableShader(composite.id);
        const int colorUnit = 0, depthUnit = 1;
        rlSetUniform(GetShaderLocation(composite, "bodies"), &colorUnit, SHADER_UNIFORM_INT, 1);
        rlSetUniform(GetShaderLocation(composite, "bodyDepth"), &depthUnit, SHADER_UNIFORM_INT, 1);
        rlActiveTextureSlot(0);
        rlEnableTexture(creatureTarget.texture.id);
        rlActiveTextureSlot(1);
        rlEnableTexture(creatureTarget.depth.id);
        rlActiveTextureSlot(0);
        rlEnableDepthMask();
        rlEnableDepthTest();
        using DepthFunction = void (*)(unsigned);
        const auto depthFunction = reinterpret_cast<DepthFunction>(rlGetProcAddress("glDepthFunc"));
        depthFunction(0x0207); // GL_ALWAYS: body fragments already passed scenery and leaf tests.
        resources_.draw(quad_.get(), 0, 6);
        ++stats.draws;
        depthFunction(0x0203); // GL_LEQUAL, raylib's standard comparison.
        rlActiveTextureSlot(1);
        rlDisableTexture();
        rlActiveTextureSlot(0);
        if (effects && !ids)
            drawEffects(v, *effects, a.v, b.v, false, *frame);
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
void WorldRenderer::drawBillboards(const View& v, const world_render::WorldFrame& frame, const float* vp,
                                   const float* view, bool ids, const world_render::Scene* scene,
                                   BillboardPass phase) {
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
    const int noClips = 0;
    rlSetUniform(billboardLocations_[7], &noClips, SHADER_UNIFORM_INT, 1);
    rlSetUniform(billboardLocations_[9], &noClips, SHADER_UNIFORM_INT, 1);
    rlSetUniform(billboardLocations_[11], &frame.camera.position, SHADER_UNIFORM_VEC3, 1);
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
    if (scene && phase == BillboardPass::Scenery)
        for (const auto& d : scene->decals) {
            const auto texture = d.texture != UINT16_MAX ? d.texture
                                 : d.motif               ? frame.vineTexture
                                                         : frame.crestTexture;
            rlEnableTexture(resources_.texture(v.textures[texture]).id);
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
        // Guardian art rides on both leaf faces. The opaque leaf owns all depth/IDs.
        rlDisableDepthMask();
        if (phase == BillboardPass::LeafArt)
            for (unsigned i = 0; i < frame.doorCount; ++i) {
                const auto& door = frame.doors[i];
                if (door.emblemTexture == UINT16_MAX)
                    continue;
                rlEnableTexture(resources_.texture(v.textures[door.emblemTexture]).id);
                for (bool back : {false, true}) {
                    const auto q = world_render::doorEmblemQuad(door, back);
                    auto m = MatrixIdentity();
                    m.m0 = q[1].x - q[0].x;
                    m.m1 = q[1].y - q[0].y;
                    m.m2 = q[1].z - q[0].z;
                    m.m4 = q[3].x - q[0].x;
                    m.m5 = q[3].y - q[0].y;
                    m.m6 = q[3].z - q[0].z;
                    m.m12 = (q[0].x + q[1].x) * .5f;
                    m.m13 = (q[0].y + q[1].y) * .5f;
                    m.m14 = (q[0].z + q[1].z) * .5f;
                    draw(m, 1, 16, false, door.id);
                    draw(m, 2, 16, false, door.id);
                }
            }
        if (phase == BillboardPass::Scenery)
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
    world_render::BillboardClips clips;
    world_render::LeafQueries leaves;
    for (unsigned i = 0; i < frame.count; ++i) {
        const auto& b = frame.billboards[i];
        const bool body = b.id < NpcWorld::maxInstances;
        if (phase == BillboardPass::LeafArt || body != (phase == BillboardPass::Creatures))
            continue;
        world_render::prepareLeafQueries(frame, body ? &b : nullptr, leaves, true);
        const int leafCount = int(leaves.count);
        rlSetUniform(billboardLocations_[9], &leafCount, SHADER_UNIFORM_INT, 1);
        if (leafCount)
            rlSetUniform(billboardLocations_[10], leaves.leaves.data(), SHADER_UNIFORM_VEC4, leafCount * 3);
        const auto foot = world_render::project(frame.camera, b.feet);
        const float scale = world_render::focal() / b.depth;
        if (foot.x + (1 - b.anchorX) * b.width * scale < 0 ||
            foot.x - b.anchorX * b.width * scale > viewWidth ||
            foot.y - b.anchorY * b.height * scale > viewHeight ||
            foot.y + (1 - b.anchorY) * b.height * scale < 0)
            continue;
        world_render::billboardClips(frame, b, clips);
        const int clipCount = int(clips.count);
        rlSetUniform(billboardLocations_[7], &clipCount, SHADER_UNIFORM_INT, 1);
        if (clipCount)
            rlSetUniform(billboardLocations_[8], clips.rectangles.data(), SHADER_UNIFORM_VEC4, clipCount);
        rlEnableTexture(resources_.texture(v.textures[b.texture]).id);
        // CPU picking and GPU drawing share the registered crop geometry.
        const auto q = world_render::corners(frame.camera, b);
        auto model = MatrixIdentity();
        model.m0 = q[1].x - q[0].x;
        model.m1 = q[1].y - q[0].y;
        model.m2 = q[1].z - q[0].z;
        model.m4 = q[3].x - q[0].x;
        model.m5 = q[3].y - q[0].y;
        model.m6 = q[3].z - q[0].z;
        model.m12 = (q[0].x + q[1].x) * .5f;
        model.m13 = (q[0].y + q[1].y) * .5f;
        model.m14 = (q[0].z + q[1].z) * .5f;
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
void WorldRenderer::drawEffects(const View& v, const world_render::EffectFrame& frame, const float* vp,
                                const float* view, bool ground, const world_render::WorldFrame& worldFrame) {
    auto matrix = [](const float* v) {
        return Matrix{v[0], v[4], v[8],  v[12], v[1], v[5], v[9],  v[13],
                      v[2], v[6], v[10], v[14], v[3], v[7], v[11], v[15]};
    };
    rlSetBlendMode(BLEND_ALPHA_PREMULTIPLY);
    rlEnableShader(resources_.shader(billboard_.get()).id);
    rlSetUniformMatrix(billboardLocations_[0], matrix(vp));
    rlSetUniformMatrix(billboardLocations_[1], matrix(view));
    const int noClips = 0;
    rlSetUniform(billboardLocations_[7], &noClips, SHADER_UNIFORM_INT, 1);
    world_render::LeafQueries leaves;
    world_render::prepareLeafQueries(worldFrame, nullptr, leaves);
    const int leafCount = int(leaves.count);
    rlSetUniform(billboardLocations_[9], &leafCount, SHADER_UNIFORM_INT, 1);
    if (leafCount)
        rlSetUniform(billboardLocations_[10], leaves.leaves.data(), SHADER_UNIFORM_VEC4, leafCount * 3);
    rlSetUniform(billboardLocations_[11], &worldFrame.camera.position, SHADER_UNIFORM_VEC3, 1);
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
        rlEnableTexture(resources_.texture(v.textures[p.texture]).id);
        resources_.draw(quad_.get(), 0, 6);
        ++stats.draws;
    }
    rlEnableDepthMask();
    rlSetBlendMode(BLEND_ALPHA);
}
std::size_t WorldRenderer::cpuBytes() const noexcept {
    std::size_t n = sizeof(*this) + assets_->bytes() + textures_.capacity() * sizeof(textures_[0]);
    if (active_ && active_->cpu.map)
        n += active_->cpu.map->storageBytes();
    if (staged_ && staged_->cpu.map && (!active_ || staged_->cpu.map != active_->cpu.map))
        n += staged_->cpu.map->storageBytes();
    for (const auto* v : {active_.get(), staged_.get()})
        if (v) {
            n += sizeof(View) + v->cpu.bytes() + v->textureLeases.capacity() * sizeof(SharedTexture) +
                 v->textures.capacity() * sizeof(TextureHandle);
            if (v->visibility)
                n += v->visibility->bytes();
            if (v->effects)
                n += sizeof(world_render::EffectsPipeline);
        }
    return n;
}
std::size_t WorldRenderer::gpuBytes() const noexcept {
    std::size_t n = ((high_ ? 1000u * 564 : 0) + (creaturesHigh_ ? 1000u * 564 : 0) + 500u * 282) * 8 +
                    (quad_ ? 4u * 32 + 6u * 4 + 24u * 32 + 36u * 4 : 0);
    for (unsigned i = 0; i < textures_.size(); ++i)
        if (!textures_[i].expired())
            n += publication_.textures[i].bytes;
    if (active_)
        n += active_->cpu.meshBytes() + 500u * 282 * 8;
    if (staged_)
        n += staged_->cpu.meshBytes() + 500u * 282 * 8;
    return n;
}
WorldRenderer::MemoryReport WorldRenderer::memory() const noexcept {
    MemoryReport r;
    r.installedTextures = publication_.textureBytes;
    for (unsigned i = 0; i < textures_.size(); ++i)
        if (!textures_[i].expired()) {
            ++r.textureCount;
            r.residentTextures += publication_.textures[i].bytes;
        }
    const auto cpu = assets_->residency();
    r.residentAlpha = cpu.alphaBytes;
    r.residentMeshes = cpu.meshBytes;
    r.worldGpu = gpuBytes();
    r.applicationGpu = resources_.counts().bytes;
    r.peakApplicationGpu = std::max(peakApplicationGpu_, resources_.counts().peakBytes);
    return r;
}
void WorldRenderer::supersampling(bool enabled) {
    if (enabled && !high_) {
        GpuLease<TargetHandle> high{resources_, resources_.createTarget(1000, 564)};
        GpuLease<TargetHandle> creatures{resources_, resources_.createTarget(1000, 564, true)};
        high_ = std::move(high);
        creaturesHigh_ = std::move(creatures);
    }
    // Once prepared, keep this bounded scratch for allocation-free quality switches.
    supersampling_ = enabled;
}
void WorldRenderer::acquireTextures(View& v, const world_assets::AssetRequirements& required) {
    std::size_t extra = 0, unionBytes = 0;
    unsigned count = 0, extraCount = 0;
    for (unsigned i = 0; i < textures_.size(); ++i) {
        const bool resident = !textures_[i].expired();
        if (resident || required.textures[i]) {
            unionBytes += publication_.textures[i].bytes;
            ++count;
        }
        if (!resident && required.textures[i]) {
            extra += publication_.textures[i].bytes;
            ++extraCount;
        }
    }
    if (count > world_assets::textureCapacity || resources_.counts().textures + extraCount > 512 ||
        unionBytes > world_assets::textureBudget ||
        gpuBytes() + extra + v.cpu.meshBytes() + viewWidth * viewHeight * 8 > world_assets::worldGpuBudget)
        throw std::runtime_error("Resident world active/staged GPU capacity exceeded");
    v.textures.resize(textures_.size());
    v.textureLeases.reserve(required.textures.count());
    for (unsigned i = 0; i < textures_.size(); ++i)
        if (required.textures[i]) {
            auto lease = textures_[i].lock();
            if (!lease) {
                const auto& t = publication_.textures[i];
                auto pixels = prepareCpu([&] {
                    preparationPoint("Loading location artwork");
                    return publication_.read(i);
                });
                Image im{pixels.data(), int(t.width), int(t.height), int(t.levels),
                         PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
                lease = std::make_shared<GpuLease<TextureHandle>>();
                *lease = {resources_, resources_.createTexture(im)};
                const auto texture = resources_.texture(lease->get());
                SetTextureWrap(texture, t.clamp ? TEXTURE_WRAP_CLAMP : TEXTURE_WRAP_MIRROR_REPEAT);
                rlTextureParameters(texture.id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_NEAREST);
                rlTextureParameters(texture.id, RL_TEXTURE_MIN_FILTER, RL_TEXTURE_FILTER_NEAREST_MIP_LINEAR);
                if (!t.clamp && anisotropy_ > 1)
                    rlTextureParameters(texture.id, RL_TEXTURE_FILTER_ANISOTROPIC, anisotropy_);
                textures_[i] = lease;
            }
            v.textures[i] = lease->get();
            v.textureLeases.push_back(std::move(lease));
            peakApplicationGpu_ = std::max(peakApplicationGpu_, resources_.counts().bytes);
        }
}
} // namespace blobber
