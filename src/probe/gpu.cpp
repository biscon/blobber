#include "probe/gpu.hpp"
#include "assets/image.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raymath.h"
#pragma GCC diagnostic pop
#include "rlgl.h"
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace blobber::probe {
namespace {
const char* vertex = R"(#version 330
in vec3 vertexPosition;in vec2 vertexTexCoord;in vec3 vertexNormal;
uniform mat4 mvp;uniform mat4 matModel;uniform mat4 matView;
out vec2 uv;out vec3 normal;out float depth;
void main(){vec4 p=matModel*vec4(vertexPosition,1);uv=vertexTexCoord;normal=mat3(matModel)*vertexNormal;depth=-(matView*p).z;gl_Position=mvp*vec4(vertexPosition,1);})";
const char* fragment = R"(#version 330
in vec2 uv;in vec3 normal;in float depth;out vec4 finalColor;
uniform sampler2D texture0;uniform vec3 forward;uniform int kind;
void main(){vec4 c=texture(texture0,uv);if(c.a<=0.001)discard;
float face=kind==1?1.0:(normal.y>0.5?0.85:(normal.y< -0.5?0.58:0.78+0.22*abs(dot(normalize(normal),forward))));
float light=face/(1.0+0.115*depth+0.015*depth*depth);float fog=clamp((depth-9.4)/3.5,0.0,1.0);
finalColor=vec4(c.rgb*light*(1.0-fog)+vec3(15,18,20)/255.0*fog*c.a,c.a);})";
const char* resolveFragment = R"(#version 330
in vec2 fragTexCoord;out vec4 finalColor;uniform sampler2D texture0;
void main(){ivec2 p=ivec2(fragTexCoord*vec2(500,282))*2;finalColor=(texelFetch(texture0,p,0)+texelFetch(texture0,p+ivec2(1,0),0)+texelFetch(texture0,p+ivec2(0,1),0)+texelFetch(texture0,p+ivec2(1,1),0))*0.25;})";
const char* skyFragment = R"(#version 330
in vec2 fragTexCoord;out vec4 finalColor;uniform sampler2D texture0;uniform float yaw;
void main(){vec2 p=fragTexCoord;float x=(p.x-0.5)*500.0;float f=277.653;
float u=(yaw+atan(x/f))/6.2831853;float v=clamp((p.y-0.34)*0.6+0.45,0.0,1.0);finalColor=texture(texture0,vec2(u,v));})";
Shader shader(const char* vs, const char* fs) {
    auto s = LoadShaderFromMemory(vs, fs);
    if (!s.id || s.id == rlGetShaderIdDefault())
        throw std::runtime_error("Probe shader compilation failed");
    return s;
}

} // namespace
unsigned Gpu::upload(const Pixels& p, bool clamp) {
    if (textureCount_ >= textures_.size())
        throw std::runtime_error("Probe texture capacity exceeded");
    Texture2D t = LoadTextureFromImage(p.image());
    if (!IsTextureValid(t)) {
        if (t.id)
            UnloadTexture(t);
        throw std::runtime_error("Probe texture upload failed");
    }
    const unsigned i = textureCount_++;
    textures_[i] = t;
    GenTextureMipmaps(&textures_[i]);
    SetTextureWrap(textures_[i], clamp ? TEXTURE_WRAP_CLAMP : TEXTURE_WRAP_MIRROR_REPEAT);
    for (int w = p.width, h = p.height;; w = std::max(1, w / 2), h = std::max(1, h / 2)) {
        bytes += std::size_t(w) * h * 4;
        if (w == 1 && h == 1)
            break;
    }
    return i;
}
Gpu::Gpu(const Scene& scene, const Art& art) {
    try {
        shader_ = shader(vertex, fragment);
        resolve_ = shader(nullptr, resolveFragment);
        sky_ = shader(nullptr, skyFragment);
        shader_.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(shader_, "matModel");
        shader_.locs[SHADER_LOC_MATRIX_VIEW] = GetShaderLocation(shader_, "matView");
        forwardLocation_ = GetShaderLocation(shader_, "forward");
        kindLocation_ = GetShaderLocation(shader_, "kind");
        skyYawLocation_ = GetShaderLocation(sky_, "yaw");
        material_ = LoadMaterialDefault();
        if (!material_.maps)
            throw std::bad_alloc();
        material_.shader = shader_;
        for (unsigned i = 0; i < 6; ++i)
            upload(art.surfaces[i], i == 4);
        SetTextureWrap(textures_[5], TEXTURE_WRAP_REPEAT);
        for (unsigned side = 0; side < 4; ++side) {
            const auto& frames = art.npc.animations[0].views[side];
            for (unsigned frame = 0; frame < frames.size(); ++frame)
                sprites_[side][frame] = upload(premultiplied(frames[frame], art.npc.alphaCutoff), true);
        }
        for (unsigned mat = 0; mat < 3; ++mat) {
            unsigned count = 0;
            for (const auto& q : scene.faces)
                if (q.material == mat)
                    ++count;
            if (!count)
                continue;
            if (count * 4 > 65535 || scene.faces.size() > maxQuads)
                throw std::runtime_error("Probe mesh index capacity exceeded");
            auto& b = batches_[batchCount_++];
            b.texture = mat;
            Mesh& m = b.mesh;
            m.vertexCount = int(count * 4);
            m.triangleCount = int(count * 2);
            m.vertices = static_cast<float*>(MemAlloc(m.vertexCount * 3 * sizeof(float)));
            m.normals = static_cast<float*>(MemAlloc(m.vertexCount * 3 * sizeof(float)));
            m.texcoords = static_cast<float*>(MemAlloc(m.vertexCount * 2 * sizeof(float)));
            m.indices = static_cast<unsigned short*>(MemAlloc(m.triangleCount * 3 * sizeof(unsigned short)));
            if (!m.vertices || !m.normals || !m.texcoords || !m.indices)
                throw std::bad_alloc();
            unsigned v = 0, k = 0;
            for (const auto& q : scene.faces)
                if (q.material == mat) {
                    for (unsigned index : {0u, 1u, 2u, 0u, 2u, 3u})
                        m.indices[k++] = static_cast<unsigned short>(v + index);
                    for (const auto& vert : q.vertices) {
                        std::memcpy(m.vertices + v * 3, &vert.p, 12);
                        std::memcpy(m.normals + v * 3, &vert.normal, 12);
                        m.texcoords[v * 2] = vert.u;
                        m.texcoords[v * 2 + 1] = vert.v;
                        ++v;
                    }
                }
            UploadMesh(&m, false);
            if (!m.vaoId)
                throw std::runtime_error("Probe mesh upload failed");
            const auto meshBytes = std::size_t(m.vertexCount) * 32 + m.triangleCount * 6;
            bytes += meshBytes;
            cpuBytes += meshBytes;
        }
        quad_ = GenMeshPlane(1, 1, 1, 1);
        cube_ = GenMeshCube(1, 1, 1);
        for (const auto* mesh : {&quad_, &cube_}) {
            const auto meshBytes =
                std::size_t(mesh->vertexCount) * 32 + (mesh->indices ? mesh->triangleCount * 6 : 0);
            bytes += meshBytes;
            cpuBytes += meshBytes;
        }
        if (!quad_.vaoId || !cube_.vaoId)
            throw std::runtime_error("Probe primitive upload failed");
        for (unsigned i = 0; i < 2; ++i) {
            targets_[i] = LoadRenderTexture(width * (i + 1), height * (i + 1));
            if (!IsRenderTextureValid(targets_[i]))
                throw std::runtime_error("Probe target allocation failed");
            SetTextureFilter(targets_[i].texture, TEXTURE_FILTER_POINT);
            bytes += std::size_t(width) * (i + 1) * height * (i + 1) * 8;
        }
        using GetFloat = void (*)(unsigned, float*);
        using GetString = const unsigned char* (*)(unsigned, unsigned);
        using GetInt = void (*)(unsigned, int*);
        auto getString = reinterpret_cast<GetString>(rlGetProcAddress("glGetStringi"));
        auto getInt = reinterpret_cast<GetInt>(rlGetProcAddress("glGetIntegerv"));
        auto getFloat = reinterpret_cast<GetFloat>(rlGetProcAddress("glGetFloatv"));
        int n = 0;
        if (getString && getInt)
            getInt(0x821D, &n);
        for (int i = 0; i < n; ++i) {
            const char* e = reinterpret_cast<const char*>(getString(0x1F03, i));
            if (e && (std::strcmp(e, "GL_EXT_texture_filter_anisotropic") == 0 ||
                      std::strcmp(e, "GL_ARB_texture_filter_anisotropic") == 0)) {
                float max = 1;
                if (getFloat)
                    getFloat(0x84FF, &max);
                anisotropy = int(std::min(4.f, max));
                break;
            }
        }
    } catch (...) {
        cleanup();
        throw;
    }
}
void Gpu::cleanup() noexcept {
    for (auto& b : batches_)
        if (b.mesh.vertices || b.mesh.vaoId) {
            UnloadMesh(b.mesh);
            b.mesh = {};
        }
    if (quad_.vaoId || quad_.vertices) {
        UnloadMesh(quad_);
        quad_ = {};
    }
    if (cube_.vaoId || cube_.vertices) {
        UnloadMesh(cube_);
        cube_ = {};
    }
    for (auto& t : targets_)
        if (t.id) {
            UnloadRenderTexture(t);
            t = {};
        }
    for (auto& t : textures_)
        if (t.id) {
            UnloadTexture(t);
            t = {};
        }
    // Material borrows shader/textures; unload only its own map array.
    if (material_.maps) {
        MemFree(material_.maps);
        material_.maps = nullptr;
    }
    for (auto* s : {&shader_, &resolve_, &sky_})
        if (s->id) {
            UnloadShader(*s);
            *s = {};
        }
}
Gpu::~Gpu() {
    cleanup();
}
void Gpu::filter(int mode) {
    for (unsigned i = 0; i < textureCount_; ++i) {
        rlTextureParameters(textures_[i].id, RL_TEXTURE_MAG_FILTER, RL_TEXTURE_FILTER_NEAREST);
        rlTextureParameters(textures_[i].id, RL_TEXTURE_MIN_FILTER,
                            mode ? RL_TEXTURE_FILTER_NEAREST_MIP_LINEAR : RL_TEXTURE_FILTER_NEAREST);
        if (anisotropy > 1)
            rlTextureParameters(textures_[i].id, RL_TEXTURE_FILTER_ANISOTROPIC,
                                mode && i < 4 ? anisotropy : 1);
    }
}
void Gpu::draw(Mesh& mesh, unsigned texture, Matrix transform, int kind) {
    SetShaderValue(shader_, kindLocation_, &kind, SHADER_UNIFORM_INT);
    material_.maps[MATERIAL_MAP_DIFFUSE].texture = textures_[texture];
    DrawMesh(mesh, material_, transform);
    ++stats.draws;
}
const Texture2D& Gpu::render(const Scene& scene, const Art& art, const MovementState& state, unsigned timeMs,
                             int mode, float door) {
    if (mode < 0 || mode > 2)
        throw std::invalid_argument("Invalid quality profile");
    const auto c = camera(state);
    const auto side = unsigned(npcSide(Heading::South, c.position.x - (scene.actor.x + .25f),
                                       c.position.z - (scene.actor.y + .25f)));
    const auto& clip = art.npc.animations[0];
    const unsigned duration = npcClipDuration(clip.timing);
    unsigned tick = timeMs % duration, frame = 0;
    while (frame + 1 < clip.timing.count && tick >= clip.timing.durationMs[frame])
        tick -= clip.timing.durationMs[frame++];
    const unsigned sprite = sprites_[side][frame];
    stats = {};
    if (ready_ && lastMode_ == mode && lastSprite_ == int(sprite) && lastDoor_ == door &&
        last_.position.x == c.position.x && last_.position.z == c.position.z && last_.yaw == c.yaw) {
        stats.cached = true;
        return result();
    }
    if (lastMode_ != mode)
        filter(mode);
    BeginTextureMode(targets_[mode == 2 ? 1 : 0]);
    ClearBackground({15, 18, 20, 255});
    if (scene.specimen) {
        SetShaderValue(sky_, skyYawLocation_, &c.yaw, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(sky_);
        DrawTexturePro(textures_[5], {0, 0, float(textures_[5].width), float(textures_[5].height)},
                       {0, 0, float(width * (mode == 2 ? 2 : 1)), float(height * (mode == 2 ? 2 : 1))},
                       {0, 0}, 0, WHITE);
        EndShaderMode();
        ++stats.draws;
    }
    Camera3D rc{{c.position.x, c.position.y, c.position.z},
                {c.position.x + c.forward.x, c.position.y, c.position.z + c.forward.z},
                {0, 1, 0},
                60,
                CAMERA_PERSPECTIVE};
    BeginMode3D(rc);
    const double half = nearPlane * std::tan(84 * pi / 360);
    rlSetMatrixProjection(MatrixFrustum(-half, half, -(height - height * .34) * nearPlane / focal(),
                                        height * .34 * nearPlane / focal(), nearPlane, farPlane));
    SetShaderValue(shader_, forwardLocation_, &c.forward, SHADER_UNIFORM_VEC3);
    for (unsigned i = 0; i < batchCount_; ++i)
        draw(batches_[i].mesh, batches_[i].texture, MatrixIdentity());
    auto box = [&](Vector3 scale, Vector3 pos, float yaw, unsigned texture) {
        draw(cube_, texture,
             MatrixMultiply(MatrixMultiply(MatrixScale(scale.x, scale.y, scale.z), MatrixRotateY(yaw)),
                            MatrixTranslate(pos.x, pos.y, pos.z)));
    };
    if (scene.specimen) {
        const float angle = door * pi / 180;
        box({.85f, .9f, .07f}, {5.08f + .425f * std::cos(angle), .45f, 11.5f - .425f * std::sin(angle)},
            angle, 3);
        box({.08f, 1, .12f}, {5.04f, .5f, 11.5f}, 0, 3);
        box({.08f, 1, .12f}, {5.96f, .5f, 11.5f}, 0, 3);
        box({1, .08f, .12f}, {5.5f, .96f, 11.5f}, 0, 3);
        // Wall painting on north room boundary, separate from wall by 0.002 units.
        auto m = MatrixMultiply(MatrixMultiply(MatrixScale(.65f, 1, .65f), MatrixRotateX(pi / 2)),
                                MatrixTranslate(5.5f, .55f, 2.002f));
        draw(quad_, 4, m, 1);
    }
    const auto& im = clip.views[side][frame];
    const float w = clip.width > 0 ? clip.width : art.npc.width, h = art.npc.height;
    const auto center =
        billboardCenter(c, {scene.actor.x + .25f, 0, scene.actor.y + .25f}, w, h, im.anchorX, im.anchorY);
    // Plane +Y normal rotates to camera-facing +Z then around vertical axis.
    Matrix transform = MatrixMultiply(
        MatrixMultiply(MatrixMultiply(MatrixScale(w, 1, h), MatrixRotateX(pi / 2)), MatrixRotateY(-c.yaw)),
        MatrixTranslate(center.x, center.y, center.z));
    rlDisableBackfaceCulling();
    BeginBlendMode(BLEND_ALPHA_PREMULTIPLY);
    draw(quad_, sprite, transform, 1);
    EndBlendMode();
    rlEnableBackfaceCulling();
    EndMode3D();
    EndTextureMode();
    if (mode == 2) {
        BeginTextureMode(targets_[0]);
        ClearBackground(BLANK);
        BeginShaderMode(resolve_);
        DrawTexturePro(targets_[1].texture, {0, 0, 1000, -564}, {0, 0, 500, 282}, {0, 0}, 0, WHITE);
        EndShaderMode();
        EndTextureMode();
        ++stats.draws;
    }
    last_ = c;
    lastMode_ = mode;
    lastSprite_ = int(sprite);
    lastDoor_ = door;
    ready_ = true;
    return result();
}
void finishGpu() {
    using Fn = void (*)();
    static auto fn = reinterpret_cast<Fn>(rlGetProcAddress("glFinish"));
    if (!fn)
        throw std::runtime_error("glFinish unavailable");
    fn();
}
std::string driverDescription() {
    using Fn = const unsigned char* (*)(unsigned);
    auto fn = reinterpret_cast<Fn>(rlGetProcAddress("glGetString"));
    if (!fn)
        return "unavailable";
    std::string s;
    for (unsigned key : {0x1F00u, 0x1F01u, 0x1F02u}) {
        const auto* v = fn(key);
        if (v) {
            if (!s.empty())
                s += " | ";
            s += reinterpret_cast<const char*>(v);
        }
    }
    return s;
}
void exportTexture(Texture2D texture, const std::filesystem::path& path) {
    OwnedImage image(LoadImageFromTexture(texture));
    if (!image.value.data)
        throw std::runtime_error("Probe readback failed");
    ImageFlipVertical(&image.value);
    if (!ExportImage(image.value, path.string().c_str()))
        throw std::runtime_error("Probe PNG export failed");
}
} // namespace blobber::probe
