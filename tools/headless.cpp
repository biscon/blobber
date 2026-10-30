#include "assets/fingerprint.hpp"
#include "assets/rules_loader.hpp"
#include "automation/protocol.hpp"
#include "automation/trace.hpp"
#ifdef BLOBBER_HEADLESS_VISUAL
#include "automation/capture.hpp"
#include "automation/visual.hpp"
#endif
#include <iostream>
#include <vector>
using namespace blobber;
using namespace blobber::automation;
int main(int argc, char** argv) {
    try {
        std::filesystem::path assets = "assets", record, playback, capture, campaignSave;
        std::vector<std::size_t> operations;
        Mode mode = Mode::Diagnostic;
        for (int i = 1; i < argc; ++i) {
            const std::string option = argv[i];
            if (option == "--capabilities") {
#ifdef BLOBBER_HEADLESS_VISUAL
                constexpr bool graphics = true;
#else
                constexpr bool graphics = false;
#endif
                std::cout << Json{{"protocol", 1},
                                  {"visual", graphics},
                                  {"capture", graphics},
                                  {"graphics_runtime", "unchecked"}}
                                 .dump()
                          << '\n';
                return 0;
            }
            if (option == "--help") {
                std::cout << "blobber_headless --assets DIR --mode diagnostic|visual [--record FILE | "
                             "--replay FILE [--capture-dir NEW_DIR --capture-operation N (repeat up to 8)]]\n"
                             "[--campaign-save FILE] enables explicit isolated save/load; replay uses "
                             "recorded I/O.\n"
                             "--capabilities reports optional backend support without a window.\n"
                             "Version 1 JSON lines on stdin/stdout; see docs/headless_protocol.md.\n";
                return 0;
            }
            if (i + 1 >= argc)
                throw std::runtime_error("Missing option value");
            const std::string value = argv[++i];
            if (option == "--assets")
                assets = value;
            else if (option == "--campaign-save")
                campaignSave = std::filesystem::absolute(value);
            else if (option == "--record")
                record = value;
            else if (option == "--replay")
                playback = value;
            else if (option == "--capture-dir")
                capture = value;
            else if (option == "--capture-operation") {
                std::size_t consumed = 0;
                const auto n = std::stoull(value, &consumed);
                if (consumed != value.size() || value.empty() || value[0] == '-' || !n || n > 10000000 ||
                    operations.size() == 8)
                    throw std::runtime_error("Invalid capture operation");
                operations.push_back(static_cast<std::size_t>(n));
            } else if (option == "--mode" && (value == "visual" || value == "diagnostic"))
                mode = value == "visual" ? Mode::Visual : Mode::Diagnostic;
            else
                throw std::runtime_error("Unknown option or mode: " + option);
        }
        if (!record.empty() && !playback.empty())
            throw std::runtime_error("Recording and replay are exclusive");
        if ((!capture.empty() || !operations.empty()) &&
            (playback.empty() || capture.empty() || operations.empty()))
            throw std::runtime_error("Captures require replay, output directory and operations");
#ifndef BLOBBER_HEADLESS_VISUAL
        if (!capture.empty())
            throw std::runtime_error("graphics_unavailable: configure BLOBBER_BUILD_GRAPHICS=ON");
#endif
        const auto catalog = loadRules(assets / "rules/catalog.json");
        const auto definitions = loadNpcDefinitions(assets / "npcs/level.json");
        VisibilityFactory factory = nullptr;
        void* context = nullptr;
        Json content = {{"catalog", fingerprintFile(assets / "rules/catalog.json")},
                        {"npc_definitions", std::to_string(definitions.definitionFingerprint)}};
#ifdef BLOBBER_HEADLESS_VISUAL
        std::unique_ptr<SceneRenderer> renderer;
        std::unique_ptr<VisualContext> visual;
        if (mode == Mode::Visual) {
            try {
                renderer = loadVisualRenderer(assets, definitions);
            } catch (const std::exception& e) {
                throw std::runtime_error(std::string("graphics_unavailable: ") + e.what());
            }
            visual = std::make_unique<VisualContext>(VisualContext{*renderer, definitions.timings});
            context = visual.get();
            factory = makeVisual;
            for (const auto* path : {"preparation.json", "prepared/metadata.json", "prepared/scene.bin",
                                     "prepared/npcs.bin", "prepared/loot.bin"})
                content[path] = fingerprintFile(assets / path);
        }
#else
        if (mode == Mode::Visual)
            throw std::runtime_error("graphics_unavailable: configure BLOBBER_BUILD_GRAPHICS=ON");
#endif
        std::error_code ec;
        const auto executable = std::filesystem::read_symlink("/proc/self/exe", ec);
        const Json header = {{"protocol", 1},
                             {"timing", "fixed-10ms-v1"},
                             {"mode", mode == Mode::Visual ? "visual" : "diagnostic"},
                             {"binary", fingerprintFile(ec ? std::filesystem::path(argv[0]) : executable)},
                             {"content", content}};
        Json runtimeHeader = header;
        if (!campaignSave.empty())
            runtimeHeader["campaign_persistence"] = 1;
        CampaignDisk campaignDisk(campaignSave);
        TraceCampaignFiles campaignFiles(&campaignDisk);
        std::unique_ptr<CampaignStore> saves;
        if (!campaignSave.empty())
            saves = std::make_unique<CampaignStore>(
                catalog, definitions,
                CampaignContent{fingerprintFile(assets / "rules/catalog.json"),
                                definitions.definitionFingerprint},
                campaignFiles);
        Protocol protocol(catalog, definitions, mode, factory, context, saves.get());
        if (!playback.empty()) {
            replay(protocol, playback, runtimeHeader, nullptr, saves ? &campaignFiles : nullptr);
#ifdef BLOBBER_HEADLESS_VISUAL
            if (!capture.empty()) {
                Protocol captureProtocol(catalog, definitions, mode, factory, context, saves.get());
                captureReplay(captureProtocol, playback, runtimeHeader, assets, definitions, capture,
                              operations, saves ? &campaignFiles : nullptr);
            }
#endif
            std::cout << Json{{"replay", "matched"}}.dump() << '\n';
            return 0;
        }
        std::unique_ptr<TraceWriter> trace;
        if (!record.empty()) {
            trace = std::make_unique<TraceWriter>(record);
            trace->append("header", runtimeHeader);
            campaignFiles.record(trace.get());
        }
        RecordedOutput output(std::cout, trace.get());
        std::string line;
        while (!protocol.closed() && readLine(std::cin, line)) {
            Json request;
            try {
                request = parseRequest(line);
            } catch (const std::exception& e) {
                // A malformed line is not a protocol operation and cannot be replayed.
                std::cout << Json{{"version", 1},
                                  {"type", "response"},
                                  {"id", nullptr},
                                  {"ok", false},
                                  {"error", {{"code", "invalid_json"}, {"message", e.what()}}}}
                                 .dump()
                          << '\n'
                          << std::flush;
                continue;
            }
            if (trace)
                trace->append("request", request);
            const auto response = protocol.request(request, output);
            output.response(response, trace ? protocol.state() : Json(nullptr));
        }
        if (trace)
            trace->finish();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Headless runner: " << e.what() << '\n';
        std::cout << Json{{"version", 1}, {"type", "fatal"}, {"error", e.what()}}.dump() << '\n';
        return 1;
    }
}
