#include "app/world_preview.hpp"
#include "app/world_interaction_preview.hpp"
#include "assets/image.hpp"
#include "generation/floor.hpp"
#include "platform/presenter.hpp"
#include "platform/window.hpp"
#include "platform/world_renderer.hpp"
#include "rlgl.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#if defined(__unix__)
#include <sys/resource.h>
#endif
namespace blobber {
namespace {
using Clock = std::chrono::steady_clock;
using Json = nlohmann::json;
double ms(Clock::time_point begin) {
    return std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
}
void finish() {
    using Fn = void (*)();
    static auto fn = reinterpret_cast<Fn>(rlGetProcAddress("glFinish"));
    if (!fn)
        throw std::runtime_error("glFinish unavailable");
    fn();
}
std::vector<MovementState> motion(GeometryView map) {
    std::vector<MovementState> out;
    out.reserve(6 * 16);
    for (auto action :
         {Action::Forward, Action::Back, Action::Left, Action::Right, Action::TurnLeft, Action::TurnRight}) {
        bool found = false;
        for (int z = 1; z < map.height() - 1 && !found; ++z)
            for (int x = 1; x < map.width() - 1 && !found; ++x)
                for (auto heading : {Heading::North, Heading::East, Heading::South, Heading::West}) {
                    Player start{{x, z}, heading}, to = start;
                    if (map.solid(start.cell) || !tryAction(map, to, action))
                        continue;
                    Controller controller(start);
                    controller.submit(map, action, true);
                    for (int i = 0; i < 16; ++i) {
                        out.push_back(controller.state());
                        controller.update(map, .01f);
                    }
                    found = true;
                    break;
                }
        if (!found)
            throw std::runtime_error("Preview map has no representative movement path");
    }
    return out;
}
void logicalCapture(GpuResources& r, WorldImage output, const std::filesystem::path& p) {
    OwnedImage im(LoadImageFromTexture(r.target(output.target).texture));
    if (!im.value.data)
        throw std::runtime_error("World readback failed");
    ImageFlipVertical(&im.value);
    if (!ExportImage(im.value, p.string().c_str()))
        throw std::runtime_error("World capture failed");
}
std::string driver() {
    using Fn = const unsigned char* (*)(unsigned);
    const auto fn = reinterpret_cast<Fn>(rlGetProcAddress("glGetString"));
    std::string s;
    for (unsigned key : {0x1F00u, 0x1F01u, 0x1F02u}) {
        if (!s.empty())
            s += " | ";
        const auto* v = fn ? fn(key) : nullptr;
        s += v ? reinterpret_cast<const char*>(v) : "unavailable";
    }
    return s;
}
} // namespace
int runWorldPreview(const Options& o) {
    const bool diagnostic = !o.captureDirectory.empty() || !o.worldBenchmark.empty();
    if (!diagnostic)
        return runWorldInteractionPreview(o);
    const auto output = o.worldBenchmark.empty() ? o.captureDirectory : o.worldBenchmark;
    if (diagnostic) {
        if (std::filesystem::exists(output) && !std::filesystem::is_empty(output))
            throw std::runtime_error("World output directory must be empty");
        std::filesystem::create_directories(output);
    }
    const auto begin = Clock::now();
    auto publication = world_assets::load(o.assets / "prepared/world");
    const auto assetMs = ms(begin);
    const auto generation = generation::generate({o.seed.value_or(5), 1});
    if (!generation)
        throw std::runtime_error("Preview generation failed");
    const auto authored = Map::load((o.assets / "maze.txt").string());
    FloorGeometry generatedGeometry(generation.floor->map, generation.floor->layout.fixtures);
    const GeometryView geometries[]{generatedGeometry.view(), GeometryView(authored)};
    const Player starts[]{generation.floor->layout.spawn, Player{}};
    Window window(o.width, o.height, diagnostic);
    GpuResources resources;
    const auto upload = Clock::now();
    WorldRenderer renderer(resources, std::move(publication));
    renderer.supersampling(o.supersampling);
    const auto uploadMs = ms(upload);
    Controller controller(starts[0]);
    const auto prep = Clock::now();
    renderer.stage(geometries[0], controller.state());
    renderer.commit();
    const auto sceneMs = ms(prep);
    Json report{{"driver", driver()},
                {"supersampling", o.supersampling},
                {"asset_read_ms", assetMs},
                {"shared_upload_ms", uploadMs},
                {"scene_prepare_upload_first_draw_ms", sceneMs},
                {"cpu_bytes", renderer.cpuBytes()},
                {"gpu_estimated_bytes", renderer.gpuBytes()}};
    report["scene_phases_ms"] = {{"mesh", renderer.preparation.meshMs},
                                 {"upload", renderer.preparation.uploadMs},
                                 {"first_draw", renderer.preparation.firstDrawMs}};
    report["resolution"] = {500, 282};
    report["warmup"] = 120;
    report["samples"] = 600;
#ifdef NDEBUG
    report["build"] = "Release";
#else
    report["build"] = "Debug";
#endif
    renderer.stage(geometries[0], controller.state());
    report["active_staged_cpu_bytes"] = renderer.cpuBytes();
    report["active_staged_gpu_estimated_bytes"] = renderer.gpuBytes();
    report["preparation_upload_bytes"] = resources.transfers().bytes;
    renderer.cancel();
    if (!o.worldBenchmark.empty()) {
        ClearWindowState(FLAG_VSYNC_HINT);
        SetTargetFPS(0);
        const auto samples = motion(geometries[0]);
        std::array<double, 600> times{};
        for (bool sync : {false, true})
            for (bool moving : {false, true})
                for (unsigned trial = 0; trial < 3; ++trial) {
                    for (unsigned i = 0; i < 120; ++i) {
                        renderer.render(moving ? samples[i % samples.size()] : controller.state());
                        if (sync)
                            finish();
                    }
                    const auto beforeTransfers = resources.transfers();
                    unsigned draws = 0, cached = 0;
                    for (unsigned i = 0; i < 600; ++i) {
                        const auto t = Clock::now();
                        renderer.render(moving ? samples[i % samples.size()] : controller.state());
                        if (sync)
                            finish();
                        times[i] = ms(t);
                        draws += renderer.stats.draws;
                        cached += renderer.stats.cached;
                    }
                    std::sort(times.begin(), times.end());
                    report["trials"].push_back(
                        {{"synchronized", sync},
                         {"moving", moving},
                         {"trial", trial},
                         {"p50_ms", times[299]},
                         {"p95_ms", times[569]},
                         {"p99_ms", times[593]},
                         {"draws", draws},
                         {"cache_hits", cached},
                         {"mesh_texture_uploads", resources.transfers().uploads - beforeTransfers.uploads},
                         {"uploaded_bytes", resources.transfers().bytes - beforeTransfers.bytes}});
                }
        benchmarkWorldInteractions(o, resources, renderer, output);
    } else {
        Presenter presenter(resources, o.assets);
        if (!o.captureDirectory.empty()) {
            for (unsigned scene = 0; scene < 2; ++scene) {
                Controller start(starts[scene]);
                renderer.stage(geometries[scene], start.state());
                renderer.commit();
                const auto samples = motion(geometries[scene]);
                for (bool quality : {false, true}) {
                    renderer.supersampling(quality);
                    for (unsigned i = 0; i < samples.size(); ++i) {
                        const auto image = renderer.render(samples[i]);
                        if (i % 16 != 0 && i % 16 != 8 && i % 16 != 15)
                            continue;
                        const auto stem =
                            std::to_string(scene) + (quality ? "-ss-" : "-native-") + std::to_string(i);
                        logicalCapture(resources, image, output / (stem + ".png"));
                        if (i == 0 || i == 40) {
                            presenter.presentWorld(image.target, image.revision, samples[i],
                                                   geometries[scene].base());
                            presenter.capture(output / (stem + "-hud.png"));
                        }
                    }
                }
            }
            renderer.render(controller.state());
            const auto retained = renderer.image();
            resources.failAfter(2);
            bool failed = false;
            try {
                renderer.stage(geometries[0], controller.state());
            } catch (const std::exception&) {
                failed = true;
            }
            resources.failAfter(-1);
            if (!failed)
                throw std::runtime_error("Failure injection did not trigger");
            const auto after = renderer.image();
            if (after.target.generation != retained.target.generation || after.revision != retained.revision)
                throw std::runtime_error("Staging changed active image");
            logicalCapture(resources, after, output / "retained-after-failure.png");
            report["failed_stage_retained_active"] = true;
            captureWorldInteractions(o, resources, renderer, presenter, output);
        }
    }
#if defined(__unix__)
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
    report["peak_rss_kib"] = usage.ru_maxrss;
#endif
    if (diagnostic) {
        std::ofstream out(output / "report.json");
        out << report.dump(2) << '\n';
        if (!out)
            throw std::runtime_error("Cannot write world report");
    }
    return 0;
}
} // namespace blobber
