#include "probe/review.hpp"
#include "assets/image.hpp"
#include "input/key_repeat.hpp"
#include "platform/window.hpp"
#include "rlgl.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#endif
namespace blobber::probe {
using Json = nlohmann::json;
using Clock = std::chrono::steady_clock;
namespace {
constexpr const char* modes[]{"nearest", "native", "supersampled"};
double milliseconds(Clock::time_point begin) {
    return std::chrono::duration<double, std::milli>(Clock::now() - begin).count();
}
void write(const std::filesystem::path& path, const std::string& data) {
    std::ofstream out(path);
    out << data;
    if (!out)
        throw std::runtime_error("Cannot write " + path.string());
}
void outputDirectory(const std::filesystem::path& path) {
    if (std::filesystem::exists(path) && !std::filesystem::is_empty(path))
        throw std::runtime_error("Output directory must be empty: " + path.string());
    std::filesystem::create_directories(path);
}
void drawTexture(Texture2D texture, Rectangle destination, bool legacy) {
    DrawTexturePro(texture, {0, 0, float(texture.width), float(texture.height) * (legacy ? 1 : -1)},
                   destination, {0, 0}, 0, WHITE);
}
void surround(Texture2D texture, bool legacy, const char* title) {
    ClearBackground({12, 18, 18, 255});
    drawTexture(texture, {12, 54, 1500, 846}, legacy);
    DrawText(title, 24, 12, 24, RAYWHITE);
    DrawText("R01 / visual probe", 1540, 70, 22, GOLD);
    DrawText("W S   forward / back\nA D   strafe\nQ E   turn\n1 2 3 quality\nTab   scene\nF     legacy "
             "(flat)\nSpace script/pause\nR     reset\nO     door\nT     animation on/off\nEsc   exit",
             1540, 130, 20, LIGHTGRAY);
    DrawText("500 x 282 world / 1920 x 1080 presentation / native UI remains unchanged", 24, 950, 22, GRAY);
}
struct Presentation {
    RenderTexture2D target{};
    Presentation() {
        target = LoadRenderTexture(1920, 1080);
        if (!IsRenderTextureValid(target)) {
            if (target.id)
                UnloadRenderTexture(target);
            throw std::runtime_error("Presentation target failed");
        }
        SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);
    }
    ~Presentation() { UnloadRenderTexture(target); }
    void draw(Texture2D texture, bool legacy, const char* title) {
        BeginTextureMode(target);
        surround(texture, legacy, title);
        EndTextureMode();
    }
    void screen() {
        BeginDrawing();
        ClearBackground(BLACK);
        auto fit = aspectFit(GetScreenWidth(), GetScreenHeight());
        drawTexture(target.texture, {fit.x, fit.y, fit.width, fit.height}, false);
        EndDrawing();
    }
};
Json provenance(const Settings& s, const Art& art) {
    return {{"driver", driverDescription()},
            {"art", s.shipped ? "shipped" : "generated"},
            {"assetFingerprint", std::to_string(art.fingerprint)},
            {"sourceBytes", art.bytes},
            {"logicalSize", {width, height}},
            {"presentationSize", {1920, 1080}},
            {"simulationTickMs", 10},
            {"near", nearPlane},
            {"far", farPlane},
            {"build",
#ifdef NDEBUG
             "Release"
#else
             "Debug"
#endif
            }};
}
void exportLogical(Texture2D texture, bool legacy, const std::filesystem::path& path) {
    if (!legacy) {
        exportTexture(texture, path);
        return;
    }
    OwnedImage image(LoadImageFromTexture(texture));
    if (!image.value.data || !ExportImage(image.value, path.string().c_str()))
        throw std::runtime_error("Legacy export failed");
}
} // namespace
Settings options(int argc, char** argv) {
    Settings s;
    s.assets = BLOBBER_PROBE_ASSETS;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        auto value = [&]() {
            if (++i >= argc)
                throw std::invalid_argument("Missing value for " + arg);
            return std::string(argv[i]);
        };
        if (arg == "--help")
            s.help = true;
        else if (arg == "--capture")
            s.capture = value();
        else if (arg == "--benchmark")
            s.benchmark = value();
        else if (arg == "--assets")
            s.assets = value();
        else if (arg == "--art") {
            auto v = value();
            if (v != "generated" && v != "shipped")
                throw std::invalid_argument("Art must be generated or shipped");
            s.shipped = v == "shipped";
        } else if (arg == "--profile") {
            auto v = value();
            s.legacy = v == "legacy";
            if (!s.legacy) {
                s.mode = -1;
                for (int m = 0; m < 3; ++m)
                    if (v == modes[m])
                        s.mode = m;
                if (s.mode < 0)
                    throw std::invalid_argument("Unknown profile");
            }
        } else
            throw std::invalid_argument("Unknown probe option: " + arg);
    }
    if (!s.capture.empty() && !s.benchmark.empty())
        throw std::invalid_argument("Capture and benchmark are exclusive");
    return s;
}
void captures(const Settings& settings) {
    outputDirectory(settings.capture);
    Window window(1920, 1080, true);
    ClearWindowState(FLAG_VSYNC_HINT);
    SetTargetFPS(0);
    Json manifest;
    manifest["series"] = Json::array();
    Presentation presentation;
    for (bool specimen : {false, true}) {
        const auto scene = makeScene(specimen);
        const auto art = loadArt(settings.shipped, settings.assets, scene);
        manifest["environment"] = provenance(settings, art);
        Gpu gpu(scene, art);
        manifest["environment"]["anisotropy"] = gpu.anisotropy;
        std::unique_ptr<Legacy> legacy;
        if (!specimen)
            legacy = std::make_unique<Legacy>(scene, art);
        for (int profile = specimen ? 0 : -1; profile < 3; ++profile) {
            const bool old = profile < 0;
            const std::string name =
                std::string(specimen ? "specimen-" : "flat-") + (old ? "legacy" : modes[profile]);
            const auto dir = settings.capture / name;
            std::filesystem::create_directory(dir);
            Json frames = Json::array();
            // Every 10 ms over each actual transition, plus settled idle samples.
            for (unsigned shot = 0; shot < shotCount; ++shot) {
                OwnedImage contact(GenImageColor(width * 5, height, BLACK));
                unsigned tile = 0;
                for (unsigned local = 0; local < shotMs; local += 10) {
                    if (local > 400 && local != 600 && local != 790)
                        continue;
                    const unsigned ms = shot * shotMs + local;
                    const auto state = scripted(scene, ms);
                    if (!clearance(scene.map, camera(state)))
                        throw std::runtime_error("Script camera clips geometry");
                    const unsigned animation = shot == 8 ? std::min(ms, shot * shotMs + 280) : ms;
                    const float door = shot == 10 ? doorAngle(ms) : 90;
                    const auto& texture = old ? legacy->render(scene, state, animation)
                                              : gpu.render(scene, art, state, animation, profile, door);
                    const std::string file = std::to_string(ms) + ".png";
                    exportLogical(texture, old, dir / file);
                    frames.push_back({{"ms", ms}, {"shot", shotName(ms)}, {"file", name + "/" + file}});
                    if (local == 0 || local == 200 || local == 240 || local == 280 || local == 360) {
                        OwnedImage image(LoadImageFromTexture(texture));
                        if (!old)
                            ImageFlipVertical(&image.value);
                        ImageDrawImage(&contact.value, image.value, int(tile++ * width), 0, WHITE);
                    }
                    if (local == 280) {
                        presentation.draw(texture, old, name.c_str());
                        exportTexture(presentation.target.texture,
                                      dir / ("native-" + std::to_string(shot) + ".png"));
                    }
                }
                if (!ExportImage(contact.value,
                                 (dir / ("contact-" + std::to_string(shot) + ".png")).string().c_str()))
                    throw std::runtime_error("Contact export failed");
            }
            manifest["series"].push_back({{"name", name}, {"frames", frames}});
        }
    }
    write(settings.capture / "manifest.json", manifest.dump(2));
    const std::string html = R"(<!doctype html><meta charset="utf-8"><title>R01 motion comparison</title>
<style>body{background:#101818;color:#ddd;font:16px system-ui;margin:20px}button,select,input{margin:8px}section{display:inline-block;margin:5px;vertical-align:top}img{width:750px;height:423px;image-rendering:pixelated}h2{font-size:18px}a{color:#cda}p{max-width:1100px}</style>
<h1>R01 · low-resolution 3D motion proof</h1><p>Flat legacy and GPU views share movement snapshots. Specimen doors/sky have no legacy equivalent. Images retain the logical pixel grid. Select a frame to compare all modes at the same timestamp. Native captures and contact sheets are linked beneath each view.</p>
<button id="play">Play / pause</button><button id="prev">Previous</button><button id="next">Next</button><select id="speed"><option value="1">Real time</option><option value="0.25">Quarter speed</option></select><select id="scene"><option>flat</option><option>specimen</option></select><input id="frame" type="range" min="0" value="0"><span id="stamp"></span><div id="views"></div><pre id="info"></pre>
<script>const data=__DATA__;let index=0,running=false,time=0,last=0;const slider=document.querySelector('#frame');
const series=()=>data.series.filter(s=>s.name.startsWith(document.querySelector('#scene').value));
function show(){const ss=series();slider.max=ss[0].frames.length-1;index=Math.max(0,Math.min(index,+slider.max));slider.value=index;const f=ss[0].frames[index];document.querySelector('#stamp').textContent=f.ms+' ms · '+f.shot;document.querySelector('#views').innerHTML=ss.map(s=>'<section><h2>'+s.name+'</h2><img src="'+s.frames[index].file+'"><p><a href="'+s.name+'/native-'+Math.floor(f.ms/800)+'.png">Native presentation</a> · <a href="'+s.name+'/contact-'+Math.floor(f.ms/800)+'.png">Contact sheet</a></p></section>').join('');}
slider.oninput=()=>{index=+slider.value;time=series()[0].frames[index].ms;show()};document.querySelector('#scene').onchange=show;document.querySelector('#prev').onclick=()=>{running=false;index--;show()};document.querySelector('#next').onclick=()=>{running=false;index++;show()};document.querySelector('#play').onclick=()=>{running=!running;time=series()[0].frames[index].ms;last=performance.now()};
function tick(now){if(running){time=(time+(now-last)*+document.querySelector('#speed').value)%9600;const fs=series()[0].frames;let j=0;while(j+1<fs.length&&fs[j+1].ms<=time)j++;if(j!==index){index=j;show()}}last=now;requestAnimationFrame(tick)}
document.querySelector('#info').textContent=JSON.stringify(data.environment,null,2);show();requestAnimationFrame(tick);</script>)";
    std::string document = html;
    document.replace(document.find("__DATA__"), 8, manifest.dump());
    write(settings.capture / "index.html", document);
}
void benchmark(const Settings& settings) {
    outputDirectory(settings.benchmark);
    Window window(1920, 1080, true);
    ClearWindowState(FLAG_VSYNC_HINT);
    SetTargetFPS(0);
    const auto start = Clock::now();
    auto scene = makeScene(false);
    const double meshMs = milliseconds(start);
    const auto load = Clock::now();
    auto art = loadArt(settings.shipped, settings.assets, scene);
    const double loadMs = milliseconds(load);
    const auto prepare = Clock::now();
    std::unique_ptr<Gpu> gpu;
    std::unique_ptr<Legacy> legacy;
    if (settings.legacy)
        legacy = std::make_unique<Legacy>(scene, art);
    else
        gpu = std::make_unique<Gpu>(scene, art);
    finishGpu();
    const double prepareMs = milliseconds(prepare);
    Json report = provenance(settings, art);
    report["profile"] = settings.legacy ? "legacy" : modes[settings.mode];
    report["meshConstructionMs"] = meshMs;
    report["sourceLoadingMs"] = loadMs;
    report["rendererPreparationAndUploadMs"] = prepareMs;
    report["managedCpuBytesEstimate"] =
        art.bytes + scene.faces.capacity() * sizeof(Quad) + (legacy ? legacy->bytes : gpu->cpuBytes);
    report["gpuBytesEstimate"] = gpu ? gpu->bytes : std::size_t(width) * height * 4;
    report["notes"] = "CPU submission excludes swap/export/readback; synchronized completion includes "
                      "glFinish overhead; moving samples active forward/back/strafe/turn transitions without "
                      "settled holds. These are small-fixture measurements, not GPU timer queries. "
                      "Legacy preparation includes perspective baking, unlike production pre-baked loading.";
    std::array<double, 600> samples{};
    for (const char* work : {"cached-idle", "moving", "animated-idle"})
        for (bool sync : {false, true})
            for (unsigned trial = 0; trial < 3; ++trial) {
                std::uint64_t draws = 0, uploads = 0, hits = 0;
                for (unsigned i = 0; i < 720; ++i) {
                    const bool moving = std::string_view(work) == "moving";
                    auto state = moving ? scripted(scene, ((i / 16) % 6) * shotMs + 200 + (i % 16) * 10)
                                        : scripted(scene, 6 * shotMs);
                    unsigned anim = std::string_view(work) == "cached-idle" ? 0 : i * 10;
                    const auto begin = Clock::now();
                    if (legacy)
                        legacy->render(scene, state, anim);
                    else
                        gpu->render(scene, art, state, anim, settings.mode, 90);
                    rlDrawRenderBatchActive();
                    if (sync)
                        finishGpu();
                    const double elapsed = milliseconds(begin);
                    if (i >= 120) {
                        samples[i - 120] = elapsed;
                        hits += legacy ? legacy->cached : gpu->stats.cached;
                        draws += gpu ? gpu->stats.draws : 0;
                        uploads += legacy && !legacy->cached ? 1 : 0;
                    }
                }
                finishGpu();
                std::sort(samples.begin(), samples.end());
                report["runs"].push_back({{"workload", work},
                                          {"synchronized", sync},
                                          {"trial", trial},
                                          {"samples", 600},
                                          {"p50Ms", samples[299]},
                                          {"p95Ms", samples[569]},
                                          {"p99Ms", samples[593]},
                                          {"draws", draws},
                                          {"uploads", uploads},
                                          {"cacheHits", hits}});
            }
#if defined(__unix__) || defined(__APPLE__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0)
#ifdef __APPLE__
        report["peakRssKiB"] = usage.ru_maxrss / 1024;
#else
        report["peakRssKiB"] = usage.ru_maxrss;
#endif
    else
        report["peakRssKiB"] = nullptr;
#else
    report["peakRssKiB"] = nullptr;
#endif
    write(settings.benchmark / "report.json", report.dump(2));
}
void interactive(const Settings& settings) {
    Window window(1920, 1080);
    Presentation presentation;
    std::array<Scene, 2> scenes{makeScene(false), makeScene(true)};
    Art art = loadArt(settings.shipped, settings.assets, scenes[0]);
    auto gpu = std::make_unique<Gpu>(scenes[0], art);
    std::unique_ptr<Legacy> legacy;
    int sceneIndex = 0, mode = settings.mode;
    bool old = settings.legacy, script = false, paused = false;
    Controller controller(scenes[0].spawn);
    KeyRepeat keys;
    unsigned time = 0, manualTime = 0;
    float accumulator = 0, door = 90;
    bool opening = true;
    while (!WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE)) {
        const bool focused = IsWindowFocused();
        const float dt = focused ? std::min(GetFrameTime(), .1f) : 0;
        if (focused) {
            if (IsKeyPressed(KEY_TAB)) {
                sceneIndex = 1 - sceneIndex;
                old = false;
                controller = Controller(scenes[sceneIndex].spawn);
                // Explicit prototype scene-preparation boundary: keep total targets below four.
                // Production transactional scene staging belongs to R02/R04.
                gpu.reset();
                gpu = std::make_unique<Gpu>(scenes[sceneIndex], art);
            }
            if (IsKeyPressed(KEY_ONE))
                mode = 0;
            if (IsKeyPressed(KEY_TWO))
                mode = 1;
            if (IsKeyPressed(KEY_THREE))
                mode = 2;
            if (IsKeyPressed(KEY_F) && !sceneIndex)
                old = !old;
            if (IsKeyPressed(KEY_R)) {
                controller = Controller(scenes[sceneIndex].spawn);
                time = 0;
                paused = false;
            }
            if (IsKeyPressed(KEY_SPACE)) {
                if (!script) {
                    script = true;
                    paused = false;
                } else
                    paused = !paused;
            }
            if (IsKeyPressed(KEY_O)) {
                opening = !opening;
            }
            if (IsKeyPressed(KEY_T))
                controller.setAnimated(!controller.state().animated);
        } else {
            controller.clearPending();
            keys.clear();
        }
        if (old && !legacy)
            legacy = std::make_unique<Legacy>(scenes[0], art);
        constexpr int codes[]{KEY_W, KEY_S, KEY_A, KEY_D, KEY_Q, KEY_E};
        std::array<bool, 6> down{}, pressed{};
        if (focused)
            for (int k = 0; k < 6; ++k) {
                down[k] = IsKeyDown(codes[k]);
                pressed[k] = IsKeyPressed(codes[k]);
            }
        if (auto action = keys.sample(down, pressed, dt)) {
            if (script)
                controller = Controller(scripted(scenes[sceneIndex], time).player);
            script = false;
            controller.submit(scenes[sceneIndex].map, *action, pressed[unsigned(*action)]);
        }
        accumulator += dt;
        while (accumulator >= .01f) {
            accumulator -= .01f;
            if (script) {
                if (!paused)
                    time = (time + 10) % sequenceMs;
            } else {
                manualTime += 10;
                controller.update(scenes[sceneIndex].map, .01f);
            }
        }
        const auto state = script ? scripted(scenes[sceneIndex], time) : controller.state();
        if (!script)
            door = std::clamp(door + (opening ? 1 : -1) * dt * 225, 0.f, 90.f);
        const unsigned anim =
            script ? ((time / shotMs) == 8 ? std::min(time, 8 * shotMs + 280) : time) : manualTime;
        const auto& texture =
            old ? legacy->render(scenes[0], state, anim)
                : gpu->render(scenes[sceneIndex], art, state, anim, mode, script ? doorAngle(time) : door);
        char title[160];
        std::snprintf(title, sizeof(title), "%s / %s / %s", sceneIndex ? "Specimen" : "Flat comparison",
                      old ? "legacy" : modes[mode], script ? shotName(time) : "manual");
        presentation.draw(texture, old, title);
        presentation.screen();
    }
}
} // namespace blobber::probe
