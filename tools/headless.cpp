#include "assets/fingerprint.hpp"
#include "assets/rules_loader.hpp"
#include "automation/protocol.hpp"
#include "automation/trace.hpp"
#ifdef BLOBBER_HEADLESS_VISUAL
#include "automation/visual.hpp"
#endif
#include <iostream>
using namespace blobber;
using namespace blobber::automation;
int main(int argc, char** argv) {
    try {
        std::filesystem::path assets = "assets", record, playback;
        Mode mode = Mode::Diagnostic;
        for (int i = 1; i < argc; ++i) {
            const std::string option = argv[i];
            if (option == "--help") {
                std::cout << "blobber_headless --assets DIR --mode diagnostic|visual [--record FILE | "
                             "--replay FILE]\n"
                             "Version 1 JSON lines on stdin/stdout; see docs/headless_protocol.md.\n";
                return 0;
            }
            if (i + 1 >= argc)
                throw std::runtime_error("Missing option value");
            const std::string value = argv[++i];
            if (option == "--assets")
                assets = value;
            else if (option == "--record")
                record = value;
            else if (option == "--replay")
                playback = value;
            else if (option == "--mode" && (value == "visual" || value == "diagnostic"))
                mode = value == "visual" ? Mode::Visual : Mode::Diagnostic;
            else
                throw std::runtime_error("Unknown option or mode: " + option);
        }
        if (!record.empty() && !playback.empty())
            throw std::runtime_error("Recording and replay are exclusive");
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
            renderer = loadVisualRenderer(assets, definitions);
            visual = std::make_unique<VisualContext>(VisualContext{*renderer, definitions.timings});
            context = visual.get();
            factory = makeVisual;
            for (const auto* path : {"preparation.json", "prepared/metadata.json", "prepared/scene.bin",
                                     "prepared/npcs.bin", "prepared/loot.bin"})
                content[path] = fingerprintFile(assets / path);
        }
#else
        if (mode == Mode::Visual)
            throw std::runtime_error("Visual backend unavailable: configure BLOBBER_BUILD_GRAPHICS=ON");
#endif
        std::error_code ec;
        const auto executable = std::filesystem::read_symlink("/proc/self/exe", ec);
        const Json header = {{"protocol", 1},
                             {"timing", "fixed-10ms-v1"},
                             {"mode", mode == Mode::Visual ? "visual" : "diagnostic"},
                             {"binary", fingerprintFile(ec ? std::filesystem::path(argv[0]) : executable)},
                             {"content", content}};
        Protocol protocol(catalog, definitions, mode, factory, context);
        if (!playback.empty()) {
            replay(protocol, playback, header);
            std::cout << Json{{"replay", "matched"}}.dump() << '\n';
            return 0;
        }
        std::unique_ptr<TraceWriter> trace;
        if (!record.empty()) {
            trace = std::make_unique<TraceWriter>(record);
            trace->append("header", header);
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
