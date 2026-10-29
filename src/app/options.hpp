#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
namespace blobber {
struct Options {
    int width = 1920, height = 1080;
    std::filesystem::path assets, captureDirectory;
    bool captureMenuOnly = false;
    bool captureBuildsOnly = false;
    bool recoveryDemo = false, captureRecoveryOnly = false;
    bool inventoryDemo = false, captureInventoryOnly = false;
    bool spellDemo = false, captureSpellsOnly = false;
    bool captureProjectilesOnly = false, captureReactionsOnly = false, captureHudOnly = false, help = false,
         uiDemo = false, combatDemo = false, aiDemo = false, captureAiOnly = false, hybridDemo = false,
         captureHybridOnly = false, capturePreparationOnly = false, checkPreparedOnly = false;
    std::optional<std::uint32_t> seed;
};
Options parseOptions(int argc, char** argv);
} // namespace blobber
