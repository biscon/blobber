#include "assets/image.hpp"
#include "platform/game_view.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include <cmath>
#include <iostream>
using namespace blobber;
namespace {
std::vector<unsigned char> pixels(GpuResources& resources, TargetHandle target) {
    OwnedImage image(LoadImageFromTexture(resources.target(target).texture));
    check(image.value.data, "Read owned lighting fixture");
    ImageFlipVertical(&image.value);
    ImageFormat(&image.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const auto* p = static_cast<const unsigned char*>(image.value.data);
    return {p, p + viewWidth * viewHeight * 4};
}
Map room(bool outside, const char* pack = "sanctuary-city") {
    WorldDefinition definition;
    definition.assetPack = pack;
    definition.surfaces.resize(90);
    for (auto& surface : definition.surfaces) {
        surface.ceiling = outside ? noCeiling : 8;
        surface.environment = outside ? Environment::Daylight : Environment::Dungeon;
    }
    return Map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                "#.......#", "#.......#", "#########"}, definition);
}
void checkLighting(GpuResources& resources, const test::VisibilityFixture& fixture) {
    const auto cat = spellCatalog();
    const auto map = room(true);
    const Player pose{{4, 7}, Heading::North};
    const NpcWorld placements(map, pose.cell, {"goblin"}, {{{0}, {4, 3}, {0, 0}, Heading::South}});
    GameSession game(map, cat, placements, 5, pose, false, fixture.timings);
    NpcAnimator animator(game.actors(), fixture.timings);
    animator.setTime(0);
    WorldRenderer renderer(resources, fixture.publication(), true, true);
    GpuLease<TargetHandle> ids(resources, resources.createTarget(viewWidth, viewHeight));
    for (bool quality : {false, true}) {
        renderer.supersampling(quality);
        SpellVisualJournal journal;
        journal.clear(game.actors().snapshot().world);
        world_render::FrameInput input{game.geometry(), game.movement(), &game.actors(), &animator.snapshot()};
        input.spells = &journal.snapshot();
        renderer.stage(input);
        renderer.commit();
        const auto day = pixels(resources, renderer.image().target);
        const auto visibility = renderer.visibility()->frame().revision;
        renderer.diagnosticIds(ids.get());
        const auto dayIds = pixels(resources, ids.get());
        input.outdoorLighting = world_render::outdoorLighting({16 * gameDuration::hour});
        renderer.render(input);
        check(!renderer.stats.cached && !renderer.stats.aoEvaluations &&
                  renderer.visibility()->frame().revision == visibility,
              "Time changes color without rebuilding AO or visibility");
        const auto night = pixels(resources, renderer.image().target);
        check(day != night, "Night changes outdoor color");
        unsigned body = 0, ground = 0;
        for (std::size_t i = 0; i < day.size(); i += 4) {
            for (unsigned c = 0; c < 3; ++c)
                check(std::abs(float(night[i + c]) - day[i + c] * input.outdoorLighting.ambient[c]) <= 2,
                      "Sky, haze, surfaces and sprites follow the same night profile");
            body += dayIds[i] == 0 && dayIds[i + 1] == 0;
            ground += dayIds[i] == 255 && dayIds[i + 1] == 255;
        }
        check(body > 20 && ground > 200, "Comparison includes visible creature and scenery");
        renderer.diagnosticIds(ids.get());
        check(pixels(resources, ids.get()) == dayIds, "Lighting does not change GPU IDs");
        renderer.render(input);
        check(renderer.stats.cached, "Repeated night profile reuses prepared image");
        const auto transfers = resources.transfers();
        const auto counts = resources.counts();
        allocations = 0;
        watching = true;
        for (GameTime clock : {GameTime{}, GameTime{10 * gameDuration::hour},
                               GameTime{16 * gameDuration::hour}, GameTime{20 * gameDuration::hour}}) {
            input.outdoorLighting = world_render::outdoorLighting(clock);
            renderer.render(input);
        }
        watching = false;
        check(!allocations && resources.transfers().uploads == transfers.uploads &&
                  resources.counts().bytes == counts.bytes && !renderer.stats.aoEvaluations,
              "Time-only frames allocate/upload nothing and reuse AO");
        input.outdoorLighting = {};
        renderer.render(input);
        const auto active = renderer.image();
        auto stagedInput = input;
        stagedInput.outdoorLighting = world_render::outdoorLighting({16 * gameDuration::hour});
        renderer.stage(stagedInput);
        const auto staged = renderer.renderStaged(stagedInput);
        check(pixels(resources, staged.target) == night && renderer.image().revision == active.revision &&
                  pixels(resources, active.target) == day,
              "Destination is prepared at campaign night while active view stays intact");
        renderer.cancel();
        renderer.render(input);
        check(renderer.stats.cached && pixels(resources, renderer.image().target) == day,
              "Cancelled staging preserves active day profile");

        auto* cast = journal.begin(rules::SpellKind::FireBolt, {3}, pose, 0);
        check(cast, "Owned cast journal has capacity");
        cast->stage = AttackStage::Flight;
        cast->impactMs = 1000;
        cast->count = 1;
        cast->targets[0] = {game.actors().id(0), {4.5f, 3.5f}, 0, 4, true, false, false, false};
        input.now = 300;
        renderer.render(input);
        const auto litDay = pixels(resources, renderer.image().target);
        input.outdoorLighting = stagedInput.outdoorLighting;
        renderer.render(input);
        const auto litNight = pixels(resources, renderer.image().target);
        unsigned illumination = 0;
        for (int y = viewHeight / 2; y < viewHeight; ++y)
            for (int x = 0; x < viewWidth; ++x) {
                const auto i = std::size_t(y * viewWidth + x) * 4;
                // Ground samples, away from the actor: local light stays independent of night tint.
                if (dayIds[i] == 255 && dayIds[i + 1] == 255 &&
                    litDay[i] > day[i] + 4 && litDay[i] < 250 &&
                    litNight[i] > litDay[i] * .3f + 4)
                    ++illumination;
            }
        check(illumination > 5, "Spell illumination remains visible and is not multiplied by night ambient");
        renderer.diagnosticIds(ids.get());
        check(pixels(resources, ids.get()) == dayIds, "Night spell light preserves IDs");
    }
}
void interiors(GpuResources& resources, const test::VisibilityFixture& fixture) {
    auto publication = fixture.publication();
    auto inn = publication.packs[0];
    inn.id = "sanctuary-inn";
    publication.packs.push_back(inn);
    for (const char* pack : {"underkeep", "sanctuary-inn"}) {
        const auto map = room(false, pack);
        MovementState movement;
        movement.player = {{4, 7}, Heading::North};
        WorldRenderer renderer(resources, publication, false, true);
        world_render::FrameInput input{map, movement};
        renderer.stage(input);
        renderer.commit();
        const auto day = pixels(resources, renderer.image().target);
        input.outdoorLighting = world_render::outdoorLighting({16 * gameDuration::hour});
        renderer.render(input);
        check(renderer.stats.cached && pixels(resources, renderer.image().target) == day,
              "Inn and dungeon interiors remain pixel-identical and cached at night");
    }
}
} // namespace
int main() try {
    test::VisibilityFixture fixture(1, 0, false, true, true);
    Window window(640, 360, true, true);
    check(IsWindowState(FLAG_WINDOW_HIDDEN), "Clock GPU tests use hidden windows");
    GpuResources resources;
    const auto baseline = resources.counts();
    checkLighting(resources, fixture);
    interiors(resources, fixture);
    const auto after = resources.counts();
    check(after.bytes == baseline.bytes && after.targets == baseline.targets &&
              after.textures == baseline.textures && after.meshes == baseline.meshes &&
              after.shaders == baseline.shaders, "Lighting resource lifetimes are balanced");
    using Error = unsigned (*)();
    check(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0, "Valid final OpenGL state");
    std::cout << "Game time GPU checks passed\n";
} catch (const std::exception& e) {
    watching = false;
    std::cerr << e.what() << '\n';
    return 1;
}
