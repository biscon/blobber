#include "platform/environment_ao.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raymath.h"
#pragma GCC diagnostic pop
#include "rlgl.h"
#include <array>
#include <cmath>
#include <string>

namespace blobber {
namespace {
constexpr const char* common = R"(#version 330
uniform sampler2D sceneDepth;
uniform mat4 inverseProjection;
out vec4 finalColor;
const vec2 size=vec2(1000,564);
bool inside(vec2 uv){return all(greaterThanEqual(uv,vec2(0)))&&all(lessThan(uv,vec2(1)));}
float depthAt(vec2 uv){return texture(sceneDepth,uv).r;}
vec3 positionAt(vec2 uv){vec4 p=inverseProjection*vec4(uv*2.0-1.0,depthAt(uv)*2.0-1.0,1);return p.xyz/p.w;}
vec3 normalAt(vec2 uv,vec3 p){
vec2 pixel=1.0/size;
vec2 l=uv-vec2(pixel.x,0),r=uv+vec2(pixel.x,0),d=uv-vec2(0,pixel.y),u=uv+vec2(0,pixel.y);
vec3 dl=p-positionAt(clamp(l,pixel*.5,1.0-pixel*.5));
vec3 dr=positionAt(clamp(r,pixel*.5,1.0-pixel*.5))-p;
vec3 dd=p-positionAt(clamp(d,pixel*.5,1.0-pixel*.5));
vec3 du=positionAt(clamp(u,pixel*.5,1.0-pixel*.5))-p;
vec3 dx=!inside(l)?dr:!inside(r)?dl:abs(dl.z)<abs(dr.z)?dl:dr;
vec3 dy=!inside(d)?du:!inside(u)?dd:abs(dd.z)<abs(du.z)?dd:du;
vec3 n=cross(dx,dy);float len=length(n);
return len>1e-10?n/len:vec3(0,0,1);
}
)";
constexpr const char* evaluate = R"(
uniform mat4 projection;
uniform vec3 samples[32];
const float radius=.30,bias=.01,strength=.60,maxDarkening=.30;
void main(){
vec2 uv=gl_FragCoord.xy/size;
if(depthAt(uv)>=1.0){finalColor=vec4(1);return;}
vec3 p=positionAt(uv),n=normalAt(uv,p);
ivec2 tile=ivec2(gl_FragCoord.xy)%4;
float angle=float((tile.x*7+tile.y*11)%16)*6.28318530718/16.0;
vec3 axis=abs(n.z)<.999?vec3(0,0,1):vec3(0,1,0);
vec3 t=normalize(cross(axis,n)),b=cross(n,t);
vec3 rotated=t*cos(angle)+b*sin(angle);
mat3 basis=mat3(rotated,cross(n,rotated),n);
float blocked=0;
for(int i=0;i<32;++i){
vec3 s=p+basis*samples[i]*radius;
vec4 clip=projection*vec4(s,1);
if(clip.w<=0.0)continue;
vec2 q=clip.xy/clip.w*.5+.5;
if(!inside(q)||depthAt(q)>=1.0)continue;
vec3 hit=positionAt(q);
float falloff=1.0-smoothstep(radius*.5,radius,length(hit-p));
blocked+=(hit.z>=s.z+bias?1.0:0.0)*falloff;
}
float factor=1.0-min(maxDarkening,strength*blocked/32.0);
finalColor=vec4(factor,factor,factor,1);
})";
constexpr const char* blur = R"(
uniform sampler2D sourceAo;uniform vec2 direction;
void main(){
vec2 uv=gl_FragCoord.xy/size;
if(depthAt(uv)>=1.0){finalColor=vec4(1);return;}
vec3 p=positionAt(uv),n=normalAt(uv,p);
float sum=0,weight=0;
for(int i=-2;i<=2;++i){
vec2 q=uv+direction*float(i)/size;
if(!inside(q)||depthAt(q)>=1.0)continue;
vec3 other=positionAt(q),nn=normalAt(q,other);
float edge=abs(dot(other-p,n));
float w=exp(-float(i*i)*.5)*exp(-edge*edge/(2.0*.01*.01))*pow(max(dot(n,nn),0.0),16.0);
sum+=texture(sourceAo,q).r*w;weight+=w;
}
float factor=sum/max(weight,1e-6);finalColor=vec4(factor,factor,factor,1);
})";
} // namespace
EnvironmentAo::EnvironmentAo(GpuResources& r) : resources_(r) {
    static_assert(bytes < 16u * 1024 * 1024);
    depth_ = {r, r.createTarget(width, height, true)};
    output_ = {r, r.createTarget(width, height)};
    scratch_ = {r, r.createTarget(width, height)};
    evaluate_ = {r, r.createShader(nullptr, (std::string(common) + evaluate).c_str())};
    blur_ = {r, r.createShader(nullptr, (std::string(common) + blur).c_str())};
    std::array<Vector3, 32> samples{};
    for (unsigned i = 0; i < samples.size(); ++i) {
        const float z = (float(i) + .5f) / samples.size();
        const float angle = float(i) * 2.39996323f;
        const float radial = std::sqrt(1 - z * z);
        const float scale = .2f + .8f * float((i * 13) % 32) / 31.f;
        samples[i] = {std::cos(angle) * radial * scale, std::sin(angle) * radial * scale, z * scale};
    }
    const auto shader = r.shader(evaluate_.get());
    SetShaderValueV(shader, GetShaderLocation(shader, "samples"), samples.data(), SHADER_UNIFORM_VEC3, 32);
}
void EnvironmentAo::pass(TargetHandle destination, Shader shader, TargetHandle source, Matrix inverse,
                         Vector2 direction) {
    BeginTextureMode(resources_.target(destination));
    ClearBackground(WHITE);
    BeginShaderMode(shader);
    SetShaderValueMatrix(shader, GetShaderLocation(shader, "inverseProjection"), inverse);
    SetShaderValueTexture(shader, GetShaderLocation(shader, "sceneDepth"),
                          resources_.target(depth_.get()).depth);
    if (source) {
        SetShaderValueTexture(shader, GetShaderLocation(shader, "sourceAo"),
                              resources_.target(source).texture);
        SetShaderValue(shader, GetShaderLocation(shader, "direction"), &direction, SHADER_UNIFORM_VEC2);
    }
    DrawRectangle(0, 0, width, height, WHITE);
    EndShaderMode();
    EndTextureMode();
}
void EnvironmentAo::render(Matrix projection) {
    const auto shader = resources_.shader(evaluate_.get());
    SetShaderValueMatrix(shader, GetShaderLocation(shader, "projection"), projection);
    const auto inverse = MatrixInvert(projection);
    pass(output_.get(), shader, {}, inverse, {});
    pass(scratch_.get(), resources_.shader(blur_.get()), output_.get(), inverse, {1, 0});
    pass(output_.get(), resources_.shader(blur_.get()), scratch_.get(), inverse, {0, 1});
}
void EnvironmentAo::bindSurface(Shader shader, bool enabled, int w, int h) {
    const int on = enabled, unit = 1;
    const float size[]{float(w), float(h)};
    rlSetUniform(GetShaderLocation(shader, "aoEnabled"), &on, SHADER_UNIFORM_INT, 1);
    rlSetUniform(GetShaderLocation(shader, "environmentAo"), &unit, SHADER_UNIFORM_INT, 1);
    rlSetUniform(GetShaderLocation(shader, "viewportSize"), size, SHADER_UNIFORM_VEC2, 1);
    rlActiveTextureSlot(1);
    if (enabled)
        rlEnableTexture(resources_.target(output_.get()).texture.id);
    else
        rlDisableTexture();
    rlActiveTextureSlot(0);
}
} // namespace blobber
