#include "support/automation_fixtures.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::automation;
using namespace blobber::automation::test;
int main(int argc, char** argv) {
    try {
        const auto cat = catalog(false, 50, 1, false, false, true, false);
        const auto defs = definitions();
        if (argc == 2 && std::string(argv[1]) == "--scenario") {
            auto setup = generatedScenario(cat);
            setup["kind"] = "campaign";
            setup.erase("depth");
            std::cout << setup.dump() << '\n';
            return 0;
        }
        std::filesystem::path record;
        for (int i = 1; i + 1 < argc; i += 2)
            if (std::string(argv[i]) == "--record")
                record = argv[i + 1];
        std::unique_ptr<TraceWriter> trace;
        if (!record.empty()) {
            trace = std::make_unique<TraceWriter>(record);
            trace->append("header", {{"fixture", "pc072-owned-route"}});
        }
        RecordedOutput output(std::cout, trace.get());
        Protocol protocol(cat, defs, Mode::Diagnostic);
        std::string line;
        while (!protocol.closed() && readLine(std::cin, line)) {
            const auto request = parseRequest(line);
            if (trace)
                trace->append("request", request);
            output.response(protocol.request(request, output), trace ? protocol.state() : Json(nullptr));
        }
        if (trace)
            trace->finish();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
