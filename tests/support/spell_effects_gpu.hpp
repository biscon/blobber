#pragma once
#include "assets/image.hpp"
#include "platform/spell_lighting_shader.hpp"
#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
namespace blobber::test {
inline std::vector<unsigned char> spellPixels(GpuResources& resources, TargetHandle target) {
    OwnedImage image(LoadImageFromTexture(resources.target(target).texture));
    check(image.value.data, "Spell GPU image readback");
    ImageFlipVertical(&image.value);
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* p = static_cast<const unsigned char*>(image.value.data);
    return {p, p + image.value.width * image.value.height * 4};
}
// Exercise the production light-blocker GLSL directly with tiny owned data and targets.
inline void spellLightGpuParity(GpuResources& resources) {
    using namespace world_render;
    const auto source = withSpellLighting(R"(#version 330
uniform vec3 receiver;out vec4 finalColor;
void main(){finalColor=vec4(lightClear(0,receiver)?1.0:0.0,0,0,1);})");
    GpuLease<ShaderHandle> program(resources, resources.createShader(nullptr, source.c_str()));
    GpuLease<TargetHandle> target(resources, resources.createTarget(4, 4));
    SpellLightData data;
    Image image{data.texels.data(), 32, 32, 1, PIXELFORMAT_UNCOMPRESSED_R32G32B32A32};
    GpuLease<TextureHandle> texture(resources, resources.createTexture(image));
    const auto shader = resources.shader(program.get());
    unsigned visible = 0, blocked = 0;
    auto compare = [&](Vec p) {
        const bool expected = spellLightVisible(data, 0, p);
        resources.updateTexture(texture.get(), data.texels.data(), sizeof(data.texels));
        SetShaderValue(shader, GetShaderLocation(shader, "receiver"), &p, SHADER_UNIFORM_VEC3);
        BeginTextureMode(resources.target(target.get()));
        ClearBackground(BLACK);
        BeginShaderMode(shader);
        SetShaderValueTexture(shader, GetShaderLocation(shader, "spellLightData"),
                              resources.texture(texture.get()));
        DrawRectangle(0, 0, 4, 4, WHITE);
        EndShaderMode();
        EndTextureMode();
        check((spellPixels(resources, target.get())[0] > 127) == expected,
              "GPU light occlusion agrees with owned CPU ray cases");
        expected ? ++visible : ++blocked;
    };
    MovementState movement;
    const Map wall({"#######", "#.....#", "#.....#", "#..#..#", "#.....#", "#.....#", "#######"});
    FrameInput input{wall, movement};
    WorldFrame world;
    EffectFrame effects;
    effects.lightCount = 1;
    effects.lights[0] = {{3.5f, .5f, 4.1f}, {1, .3f, .05f}, 1.5f, 1, 3};
    data = prepareSpellLights(input, world, effects);
    for (Vec p : {Vec{3.5f, .5f, 4.002f}, Vec{3.5f, .5f, 2.9f}, Vec{3.5f, -.2f, 4.2f}, Vec{3.5f, 1.2f, 4.2f},
                  Vec{3.8f, .01f, 4.2f}})
        compare(p);
    const Map room({"#######", "#.....#", "#.....#", "#.....#", "#.....#", "#.....#", "#######"});
    FloorFixtures fixtures;
    fixtures.count = 1;
    fixtures.entries[0] = {{3, 3}, Heading::North, FixtureKind::Door, 0};
    const auto catalog = spellCatalog();
    NpcWorld places(room, {3, 4}, {"goblin"}, {});
    GameSession game(room, catalog, places, 5, {{3, 4}, Heading::North}, false, {}, {}, fixtures);
    DoorSnapshot doors = game.doors();
    FrameInput doorInput{game.geometry(), movement};
    doorInput.doors = &doors;
    for (unsigned phase = 0; phase < 3; ++phase) {
        if (phase == 1) {
            check(game.interact(game.nearbyFixture()).changed, "GPU fixture opens real door");
            doors.until[0] = 300;
        }
        doors.now = phase * 150;
        world.doorCount = doorBoxes(game.geometry(), &doors, world.doors, 0, 0, 0);
        data = prepareSpellLights(doorInput, world, effects);
        for (float x : {3.25f, 3.5f, 3.75f})
            compare({x, .45f, 3.5f});
    }
    check(visible > 0 && blocked > 0, "GPU blocker cases include clear and blocked light paths");
}
inline void spellEffectsGpuChecks() {
    Window window(640, 360, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "SE01 checks remain hidden");
    VisibilityFixture fixture(1, 2, true);
    const auto catalog = spellCatalog();
    const auto map = hybridRoom();
    const auto places = spellPlacements(map, 1);
    GameSession game(map, catalog, places, 5, spellSpawn, false, fixture.timings);
    NpcAnimator animator(game.actors(), fixture.timings);
    animator.setTime(0);
    GpuResources resources;
    const auto baseline = resources.counts();
    spellLightGpuParity(resources);
    {
        WorldRenderer renderer(resources, fixture.publication());
        GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
        for (bool high : {false, true}) {
            renderer.supersampling(high);
            SpellVisualJournal journal;
            journal.clear(game.actors().snapshot().world);
            ActorConditionVisualSnapshot conditions;
            conditions.world = game.actors().snapshot().world;
            world_render::FrameInput input{game.geometry(),
                                           game.movement(),
                                           &game.actors(),
                                           &animator.snapshot(),
                                           &game.loot().snapshot(),
                                           &game.doors(),
                                           &game.combat().attacks(),
                                           nullptr,
                                           0};
            input.spells = &journal.snapshot();
            input.conditions = &conditions;
            renderer.stage(input);
            renderer.commit();
            const auto unlit = spellPixels(resources, renderer.image().target);
            renderer.diagnosticIds(ids.get());
            const auto originalIds = spellPixels(resources, ids.get());
            const auto visibility = renderer.visibility()->frame().revision;
            auto* cast = journal.begin(rules::SpellKind::FireBolt, {3}, spellSpawn, 0);
            cast->stage = AttackStage::Flight;
            cast->impactMs = 1000;
            cast->count = 1;
            cast->targets[0] = {game.actors().id(0), {6.5f, 4.5f}, 0, 4, true, false, false, false};
            input.now = 300;
            renderer.render(input);
            const auto lit = spellPixels(resources, renderer.image().target);
            check(lit != unlit, "Elemental alpha/core/light changes colour");
            check(renderer.stats.aoEvaluations == 0 && renderer.visibility()->frame().revision == visibility,
                  "Effect-only redraw reuses AO and visibility");
            renderer.diagnosticIds(ids.get());
            check(spellPixels(resources, ids.get()) == originalIds, "Spell passes never modify picking IDs");
            const auto uploads = resources.transfers().uploads;
            allocations = 0;
            watching = true;
            for (unsigned i = 0; i < 5; ++i) {
                input.now = 320 + i * 20;
                renderer.render(input);
            }
            watching = false;
            check(!allocations, "GPU spell submission/upload uses no engine C++ allocations");
            check(resources.transfers().uploads > uploads, "Moving light updates reusable data texture");
            input.now = 2000;
            renderer.render(input);
            check(spellPixels(resources, renderer.image().target) == unlit,
                  "Expired light restores exact no-effect colour");
            const auto stoppedUploads = resources.transfers().uploads;
            input.now = 2100;
            renderer.render(input);
            check(renderer.stats.cached && resources.transfers().uploads == stoppedUploads,
                  "Idle resumes cached colour and no uploads");
            conditions.slow[0] = true;
            renderer.render(input);
            const auto frost = spellPixels(resources, renderer.image().target);
            unsigned differences = 0;
            for (int y = 1; y < viewHeight - 1; ++y)
                for (int x = 1; x < viewWidth - 1; ++x) {
                    const auto p = (y * viewWidth + x) * 4;
                    if (frost[p] == unlit[p] && frost[p + 1] == unlit[p + 1] && frost[p + 2] == unlit[p + 2])
                        continue;
                    ++differences;
                    bool body = false;
                    for (int dy = -1; dy <= 1; ++dy)
                        for (int dx = -1; dx <= 1; ++dx) {
                            const auto q = ((y + dy) * viewWidth + x + dx) * 4;
                            body |= originalIds[q] == 0 && originalIds[q + 1] == 0;
                        }
                    check(body, "Frost never paints a rectangle outside the creature alpha mask");
                }
            check(differences > 0, "Authoritative Slow produces visible coating");
            input.now = 2200;
            renderer.render(input);
            check(renderer.stats.cached, "Quiet frost is static");
            conditions.slow.reset();
            renderer.render(input);
            check(spellPixels(resources, renderer.image().target) == unlit,
                  "Condition removal restores exact colour");
            const auto before = resources.counts();
            const auto image = renderer.image();
            resources.failAfter(0);
            rejects([&] { renderer.stage(input); }, "Failed staged resource preserves active view");
            resources.failAfter(-1);
            check(renderer.image().revision == image.revision &&
                      renderer.image().target.slot == image.target.slot &&
                      renderer.image().target.generation == image.target.generation &&
                      resources.counts().bytes == before.bytes,
                  "Staging rollback owns its resource cleanup");
            renderer.stage(input);
            renderer.cancel();
            renderer.render(input);
            check(spellPixels(resources, renderer.image().target) == unlit,
                  "Canceled staged view preserves active presentation");
            using Error = unsigned (*)();
            check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                  "SE01 shader/texture/blend state valid");
            renderer.clear();
        }
    }
    check(resources.counts().bytes == baseline.bytes && resources.counts().textures == baseline.textures,
          "All SE01 GPU resources release before window closes");
}
} // namespace blobber::test
