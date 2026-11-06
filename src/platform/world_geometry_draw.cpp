#include "platform/world_renderer.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "raymath.h"
#pragma GCC diagnostic pop
#include "rlgl.h"
namespace blobber {
bool WorldRenderer::drawStaticGeometry(const View& v, const world_render::WorldFrame* frame) {
    bool visibleWater = false;
    for (unsigned i = 0; i < draws_.count; ++i) {
        const auto& range = v.cpu.ranges[draws_.ranges[i]];
        if (range.variant != noContainer &&
            (frame ? frame->containerOpened[range.id - PickingBuffer::containerBase] : false) !=
                bool(range.variant))
            continue;
        visibleWater |= range.water;
        const int waterFrame = range.water ? int(v.waterFrame) : -1;
        rlSetUniform(waterFrame_, &waterFrame, SHADER_UNIFORM_INT, 1);
        const int rangeCutout = range.cutout ? 1 : 0;
        rlSetUniform(meshCutout_, &rangeCutout, SHADER_UNIFORM_INT, 1);
        const int rangeUv = range.painted ? 4 : 0;
        rlSetUniform(uvMode_, &rangeUv, SHADER_UNIFORM_INT, 1);
        const float color[]{float(range.id & 255) / 255, float(range.id >> 8) / 255, 0, 1};
        rlSetUniform(idColor_, color, SHADER_UNIFORM_VEC4, 1);
        rlEnableTexture(resources_.texture(v.materials[range.material]).id);
        resources_.draw(v.mesh.get(), range.first, range.count);
        ++stats.draws;
    }
    const int noWater = -1;
    rlSetUniform(waterFrame_, &noWater, SHADER_UNIFORM_INT, 1);
    const int noCutout = 0;
    rlSetUniform(meshCutout_, &noCutout, SHADER_UNIFORM_INT, 1);
    return visibleWater;
}
void WorldRenderer::drawDoorGeometry(const View& v, const world_render::WorldFrame& frame,
                                     unsigned doorPhase) {
    for (unsigned i = 0; i < frame.doorCount; ++i) {
        const auto& box = frame.doors[i];
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
} // namespace blobber
