#include "assets/image.hpp"
#include "assets/rules_loader.hpp"
#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "render/dialogue_camera.hpp"
#include "rlgl.h"
#include <fstream>
#include <iomanip>
#include <iostream>

using namespace blobber;
namespace {
void require(bool ok, const char* message) {
    if (!ok)
        throw std::runtime_error(message);
}
void capture(GpuResources& resources, TargetHandle target, const std::filesystem::path& path) {
    OwnedImage image(LoadImageFromTexture(resources.target(target).texture));
    ImageFlipVertical(&image.value);
    require(ExportImage(image.value, path.string().c_str()), "Cannot export resolution review");
}
} // namespace
int main(int argc, char** argv) try {
    if (argc != 4 && !(argc == 5 && (std::string_view(argv[4]) == "--sprites-only" ||
                                     std::string_view(argv[4]) == "--rat-only")))
        throw std::runtime_error("Usage: blobber_review_actor_resolution ASSETS PUBLICATION NEW_OUTPUT "
                                 "[--sprites-only|--rat-only]");
    const bool spritesOnly = argc == 5 && std::string_view(argv[4]) == "--sprites-only";
    const bool ratOnly = argc == 5 && std::string_view(argv[4]) == "--rat-only";
    const std::filesystem::path root(argv[1]), output(argv[3]);
    require(std::filesystem::create_directory(output), "Use a new review output directory");
    const auto definitions = loadNpcDefinitions(root / "npcs/level.json");
    const auto rules = loadRules(root / "rules/catalog.json");
    auto assets = std::make_shared<const world_assets::CpuAssets>(world_assets::load(argv[2]));
    Window window(1920, 1080, true, true);
    require(IsWindowState(FLAG_WINDOW_HIDDEN), "Resolution review must remain hidden");
    GpuResources resources;
    std::ofstream report(output / "coverage.csv");
    report << "actor,nearest_depth,max_ss_width,max_ss_height,max_ss_pixels_per_source_texel,mip_bytes\n";
    report << std::fixed << std::setprecision(3);
    {
        WorldRenderer renderer(resources, assets);
        WorldDefinition definition;
        definition.assetPack = "underkeep";
        const Map map({"#########", "#.......#", "#.......#", "#.......#", "#.......#", "#.......#",
                       "#.......#", "#.......#", "#########"},
                      definition);
        MovementState movement;
        movement.player = {{4, 5}, Heading::North};
        const auto exploration = world_render::camera(movement, map);
        for (unsigned type = 0; !spritesOnly && type < definitions.types.size(); ++type) {
            const auto& appearance = assets->publication.appearances.at(type);
            if (ratOnly && appearance.id != "giant.rat")
                continue;
            const bool resident = !rules.monster(rules.findMonster(appearance.id));
            NpcInstance npc{{std::uint16_t(type)}, {4, 4}, {0, 1}, Heading::South};
            if (resident) {
                npc.role = NpcRole::Resident;
                npc.residentId = 1;
            }
            const NpcWorld placements(map, movement.player.cell, definitions.types, {npc});
            ActorWorld actors(map, rules, placements);
            NpcAnimationSnapshot animation;
            animation.count = 1;
            animation.world = actors.snapshot().world;
            animation.actorRevision = actors.snapshot().revision;
            world_render::FrameInput input{map, movement, &actors, &animation};
            renderer.stage(input);
            renderer.commit();
            const auto dialog =
                resident ? world_render::dialogueCamera(*renderer.scene(), *assets, map, exploration,
                                                        *actors.actor(actors.id(0)), {})
                         : exploration;
            const auto nearCamera = resident ? dialog : exploration;
            const world_render::Vec feet{4.25f, 0, 4.75f + (resident ? 0.f : .28f)};
            const auto projected = world_render::project(nearCamera, feet);
            float width = 0, height = 0, ratio = 0;
            std::bitset<world_assets::catalogTextureCapacity> textures;
            // Measure all authored poses, including side tails, wide attacks and fallen frames.
            for (const auto& clip : appearance.clips)
                for (const auto& side : clip.textures)
                    for (const auto id : side) {
                        if (id == UINT16_MAX)
                            continue;
                        textures.set(id);
                        const auto& t = assets->publication.textures[id];
                        const float sx = 2 * world_render::focal() *
                                         (clip.width ? clip.width : appearance.width) /
                                         (projected.depth * t.canvas.width);
                        const float sy = 2 * world_render::focal() * appearance.height /
                                         (projected.depth * t.canvas.height);
                        width = std::max(width, sx * t.width);
                        height = std::max(height, sy * t.height);
                        ratio = std::max({ratio, sx, sy});
                    }
            std::size_t bytes = 0;
            for (unsigned i = 0; i < assets->publication.textures.size(); ++i)
                if (textures[i])
                    bytes += assets->publication.textures[i].bytes;
            report << appearance.id << ',' << projected.depth << ',' << width << ',' << height << ',' << ratio
                   << ',' << bytes << '\n';
            const auto sample = [&](const std::string& label, NpcAnimation clip, unsigned frame, float x,
                                    float z, bool closeup) {
                auto& selection = animation.frames[0];
                selection.clip = clip;
                selection.active = std::uint8_t(frame);
                selection.offsetX = x;
                selection.offsetY = z;
                ++animation.revision;
                input.cameraOverride = closeup ? std::optional<world_render::Camera>(dialog) : std::nullopt;
                renderer.render(input);
                const auto* picking = renderer.picking();
                require(picking && (resident ? picking->residents().count : picking->targets().count) == 1,
                        "Reviewed actor remains pickable");
                capture(resources, renderer.image().target, output / (appearance.id + "-" + label + ".png"));
            };
            sample("near", NpcAnimation::Idle, 0, 0, 0, resident);
            if (!resident) {
                sample("far", NpcAnimation::Idle, 0, 0, -2, false);
                const auto attack = appearance.clips[unsigned(NpcAnimation::Attack)].timing.count
                                        ? NpcAnimation::Attack
                                        : NpcAnimation::RangedAttack;
                const auto frames = appearance.clips[unsigned(attack)].timing.count;
                sample("attack", attack, std::min(2u, unsigned(frames) - 1), 0,
                       attack == NpcAnimation::Attack ? .28f : 0, false);
            }
            if (appearance.id == "giant.rat") {
                actors.face(actors.id(0), Heading::East);
                animation.actorRevision = actors.snapshot().revision;
                sample("walk", NpcAnimation::Walk, 0, 0, 0, false);
                actors.face(actors.id(0), Heading::South);
                animation.actorRevision = actors.snapshot().revision;
                sample("death", NpcAnimation::Death, 3, 0, 0, false);
            }
            renderer.clear();
        }
        for (unsigned kind = 0; !ratOnly && kind < 4; ++kind) {
            constexpr const char* names[]{"loot", "grease", "web", "weapon"};
            LootSnapshot loot;
            loot.world = 1;
            SpellArea area;
            const Cell cell{4, 4};
            if (kind == 0) {
                loot.stacks[0].cell = cell;
                loot.stacks[0].active = true;
                loot.stacks[0].quantity = 1;
            } else if (kind == 3) {
                area.ground.weapons[0] = {cell, true};
            } else {
                area.ground.areas[0].cell = cell;
                area.ground.areas[0].active = true;
                area.ground.areas[0].web = kind == 2;
            }
            const world_render::FrameInput input{map,   movement, nullptr, nullptr,
                                                 &loot, nullptr,  nullptr, &area};
            renderer.stage(input);
            renderer.commit();
            if (kind == 0)
                require(renderer.visibility()->lootVisible({1, 0, 1}), "Prepared loot remains pickable");
            capture(resources, renderer.image().target, output / (std::string(names[kind]) + ".png"));
            renderer.clear();
        }
        using Error = unsigned (*)();
        require(reinterpret_cast<Error>(rlGetProcAddress("glGetError"))() == 0,
                "Resolution review has OpenGL errors");
    }
    require(!resources.counts().bytes, "Resolution review leaked GPU resources");
    std::cout << "Actor coverage, captures, picking and GPU lifetime checks passed\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
