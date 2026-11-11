#include "persistence/campaign_codec.hpp"
#include "persistence/library_codec.hpp"
#include "platform/file_io.hpp"
#include "support/automation_fixtures.hpp"
#include "support/generation_blueprints.hpp"
#include <iostream>
using namespace blobber;
using namespace blobber::automation::test;
namespace {
bool entropy(std::array<std::uint8_t, 16>& bytes) {
    static unsigned id = 0;
    bytes.fill(std::uint8_t(++id));
    return true;
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc < 3 || argc > 4)
            throw std::runtime_error("usage: compatibility export|import DIRECTORY [OUTPUT]");
        const auto root = std::filesystem::u8path(argv[2]);
        const auto c = catalog();
        const auto d = definitions();
        const CampaignContent content{"fixture", 0, &blobber::test::campaignBlueprints()};
        if (std::string(argv[1]) == "export") {
            std::filesystem::create_directories(root);
            CharacterLibrary library(c, root / "characters.json", {nullptr, entropy});
            check(bool(library.initialize()), "export library");
            disk::writeDocument(root / "scenario.json", encodeArena(c, scenario(c)));
            CampaignRun run(blobber::test::campaignBlueprints(), c, *c.preset(), d.types, d.timings, 5);
            run.preparePersistenceIdentity("0123456789abcdef0123456789abcdef");
            disk::writeDocument(root / "campaign.json", encodeCampaign(c, d, content, *run.captureSave()));
        } else {
            auto campaign = disk::read(root / "campaign.json", campaignMaxBytes).bytes;
            auto restored = decodeCampaign(c, d, content, campaign);
            check(inspectCampaign(content, campaign).runId == restored->runId,
                  "campaign interchange identity");
            auto arena = decodeArena(c, disk::read(root / "scenario.json", arenaMaxBytes).bytes);
            auto library = decodeLibrary(c, disk::read(root / "characters.json", libraryMaxBytes).bytes);
            check(library.count == 4 && arena.seed == 5, "library/scenario interchange");
            if (argc == 4) {
                const auto out = std::filesystem::u8path(argv[3]);
                std::filesystem::create_directories(out);
                disk::writeDocument(out / "campaign.json", encodeCampaign(c, d, content, *restored));
                disk::writeDocument(out / "characters.json", encodeLibrary(c, library));
                disk::writeDocument(out / "scenario.json", encodeArena(c, arena));
            }
        }
        std::cout << "Cross-platform formats passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
