#include "platform/cave_atmosphere.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace blobber {
GpuLease<TextureHandle> prepareCaveShore(GpuResources& resources, world_render::Scene& scene) {
    constexpr int samples = 4, reach = 2;
    const Map& map = *scene.map;
    const int w = map.width() * samples, h = map.height() * samples;
    if (w > 8192 || h > 8192)
        throw std::runtime_error("Cave shore exceeds texture capacity");
    auto wet = [&](int x, int z) { return map.surface({x, z}).floor < 0; };
    std::vector<unsigned char> field(std::size_t(w) * h, 255);
    for (int z = 0; z < map.height(); ++z)
        for (int x = 0; x < map.width(); ++x) {
            const bool inside = wet(x, z);
            // Only sample cells in the two-cell band around a pool.
            bool nearPool = inside;
            for (int j = -reach; j <= reach && !nearPool; ++j)
                for (int i = -reach; i <= reach && !nearPool; ++i)
                    nearPool = wet(x + i, z + j);
            if (!nearPool)
                continue;
            scene.chunks[(z / 8) * scene.columns + x / 8].shore = true;
            for (int v = 0; v < samples; ++v)
                for (int u = 0; u < samples; ++u) {
                    const float px = x + (u + .5f) / samples, pz = z + (v + .5f) / samples;
                    float squared = reach * reach;
                    for (int j = z - reach; j <= z + reach; ++j)
                        for (int i = x - reach; i <= x + reach; ++i) {
                            if (wet(i, j) == inside)
                                continue;
                            const float dx = px - std::clamp(px, float(i), float(i + 1));
                            const float dz = pz - std::clamp(pz, float(j), float(j + 1));
                            squared = std::min(squared, dx * dx + dz * dz);
                        }
                    const float distance = (inside ? -1.f : 1.f) * std::sqrt(squared);
                    field[(z * samples + v) * w + x * samples + u] =
                        static_cast<unsigned char>(std::lround(127.5f * (1.f + distance / reach)));
                }
        }
    Image image{field.data(), w, h, 1, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE};
    GpuLease<TextureHandle> shore(resources, resources.createTexture(image));
    const auto texture = resources.texture(shore.get());
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
    return shore;
}

std::string withCaveAtmosphere(std::string shader) {
    // World-space pigment and mist share the physical bank. The painting still
    // supplies the moving surface detail and the slate's native material grain.
    constexpr const char* functions = R"(
uniform sampler2D caveShore;
uniform vec3 caveEye;
float shoreDistance(vec2 p) {
    vec2 extent = vec2(textureSize(caveShore, 0));
    return 4.0 * texture(caveShore, p * 4.0 / extent).r - 2.0;
}
float caveHash(vec2 p) {
    vec3 q = fract(vec3(p.xyx) * vec3(.1031,.1030,.0973));
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}
float caveNoise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f*f*(3.0-2.0*f);
    return mix(mix(caveHash(i), caveHash(i+vec2(1,0)), f.x),
               mix(caveHash(i+vec2(0,1)), caveHash(i+vec2(1)), f.x), f.y);
}
vec3 caveWater(vec3 paint, vec2 p, float bank, float phase) {
    float detail = dot(paint, vec3(.23,.61,.16));
    float shallows = smoothstep(-.50, -.025, bank + .09*(caveNoise(p*4.0)-.5));
    vec3 deep = vec3(.133,.153,.106), rim = vec3(.25,.222,.135);
    vec3 water = mix(deep, rim, shallows) * (.64 + detail) + (detail-.37)*.33;
    float glint = smoothstep(.40,.50,detail) * smoothstep(.46,.70,caveNoise(p*2.7));
    water += vec3(.17,.18,.14)*glint;

    vec2 tile = floor(p);
    vec2 center = .27 + .46 * vec2(caveHash(tile+13.0), caveHash(tile+57.0));
    float age = fract(phase * 2.0 + caveHash(tile+31.0)) * 16.0;
    float r = length(fract(p)-center);
    float width = max(.010, fwidth(r));
    float ring = 1.0 - smoothstep(.003,width+.004,abs(r - .21*age));
    float wake = 1.0 - smoothstep(.003,width+.004,abs(r - .21*age+.033));
    float envelope = smoothstep(.04,.17,age)*(1.0-smoothstep(.58,1.10,age));
    return water + vec3(.25,.26,.21)*(ring + .38*wake)*envelope;
}
vec3 caveFloor(vec3 paint, vec2 p, float bank) {
    vec2 skew = vec2(dot(p,vec2(1.78,.62)), dot(p,vec2(-.80,2.20)));
    float broad = .68*caveNoise(skew+vec2(4.8,17.3)) + .32*caveNoise(p*.93+vec2(4.8,17.3));
    float grain = caveNoise(p*8.0) + .5*caveNoise(p*21.0);
    float bleed = 1.0-smoothstep(0.0,1.8,bank);
    float damp = smoothstep(.56,.62,broad + .13*(grain-.75) + .12*bleed);
    float stain = bleed * smoothstep(.20,.65,broad+.16*grain);
    return paint * mix(vec3(1),vec3(.58,.61,.53),damp)
                 * mix(vec3(1),vec3(.89,.85,.62),stain);
}
float caveMist(vec3 point, float phase) {
    // Integrate two samples of the camera ray inside the ankle-height layer.
    // The opaque receiver terminates the path, including at the pillar base.
    const float top = .15;
    if (point.y >= top) return 0.0;
    vec3 ray = caveEye-point;
    float fraction = clamp((top-point.y)/max(ray.y,.001),0.0,1.0);
    vec3 segment = ray*fraction;
    vec2 drift = vec2(sin(6.283185*phase),cos(6.283185*phase))*.32;
    float density = 0.0;
    for (int i=0; i<2; ++i) {
        vec3 samplePoint = point + segment * (.20 + .45*float(i));
        vec2 p = samplePoint.xz;
        float bank = shoreDistance(p);
        float weight = 1.0-smoothstep(.30,1.25,bank);
        vec2 wind = vec2(p.x*.74+p.y*.39, p.y*1.72-p.x*.48);
        float plume = .72*caveNoise(wind+drift) + .28*caveNoise(2.65*wind-drift);
        density += weight*smoothstep(.40,.72,plume)*(1.0-smoothstep(.045,top,samplePoint.y));
    }
    return min(.31,density*min(length(segment),1.55)*.32);
}
)";
    shader.insert(shader.find('\n') + 1, functions);
    return shader;
}
} // namespace blobber
