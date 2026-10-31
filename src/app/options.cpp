#include "app/options.hpp"
#include "raylib.h"
#include <charconv>
#include <cstring>
#include <stdexcept>
#include <string>
namespace blobber {
Options parseOptions(int argc, char** argv) {
    Options result;
    bool qualitySpecified = false;
    result.assets = std::filesystem::path(GetApplicationDirectory()) / "assets";
    auto dimension = [](const char* text) {
        std::size_t consumed = 0;
        const int value = std::stoi(text, &consumed);
        if (text[consumed] != '\0')
            throw std::invalid_argument("Invalid window dimension");
        return value;
    };
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--assets" && i + 1 < argc)
            result.assets = argv[++i];
        else if (arg == "--campaign-save" && i + 1 < argc)
            result.campaignSave = std::filesystem::absolute(argv[++i]);
        else if (arg == "--renderer" && i + 1 < argc) {
            const std::string value = argv[++i];
            if (value != "world" && value != "legacy")
                throw std::invalid_argument("Renderer expects legacy or world");
            result.worldRenderer = value == "world";
        } else if (arg == "--capture-doors" && i + 1 < argc) {
            result.captureDoorsOnly = result.worldRenderer = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--world-preview")
            result.worldPreview = true;
        else if (arg == "--capture-world" && i + 1 < argc) {
            result.worldPreview = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--benchmark-world" && i + 1 < argc) {
            result.worldPreview = true;
            result.worldBenchmark = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--supersampling" && i + 1 < argc) {
            const std::string value = argv[++i];
            if (value != "on" && value != "off")
                throw std::invalid_argument("Supersampling expects on or off");
            result.supersampling = value == "on";
            qualitySpecified = true;
        } else if (arg == "--development")
            result.development = true;
        else if (arg == "--capture-campaign-save" && i + 1 < argc) {
            result.captureSaveOnly = result.captureMenuOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture-progression" && i + 1 < argc) {
            result.captureProgressionOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture-arena" && i + 1 < argc) {
            result.captureArenaOnly = result.captureMenuOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture-creator" && i + 1 < argc) {
            result.captureCreatorOnly = result.captureMenuOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture-menu" && i + 1 < argc) {
            result.captureMenuOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--check-prepared")
            result.checkPreparedOnly = true;
        else if (arg == "--capture-builds" && i + 1 < argc) {
            result.captureBuildsOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--recovery-demo")
            result.recoveryDemo = true;
        else if (arg == "--capture-recovery" && i + 1 < argc) {
            result.captureRecoveryOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--inventory-demo")
            result.inventoryDemo = true;
        else if (arg == "--capture-inventory" && i + 1 < argc) {
            result.captureInventoryOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--spell-demo") {
            result.spellDemo = true;
            result.hybridDemo = true;
        } else if (arg == "--capture-spells" && i + 1 < argc) {
            result.captureSpellsOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture-projectiles" && i + 1 < argc) {
            result.captureProjectilesOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture-preparation" && i + 1 < argc) {
            result.capturePreparationOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture-hybrid" && i + 1 < argc) {
            result.captureHybridOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture-ai" && i + 1 < argc) {
            result.captureAiOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture-reactions" && i + 1 < argc) {
            result.captureReactionsOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture-hud" && i + 1 < argc) {
            result.captureHudOnly = true;
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        } else if (arg == "--capture" && i + 1 < argc)
            result.captureDirectory = std::filesystem::absolute(argv[++i]);
        else if (arg == "--size" && i + 2 < argc) {
            result.width = dimension(argv[++i]);
            result.height = dimension(argv[++i]);
        } else if (arg == "--ui-demo")
            result.uiDemo = true;
        else if (arg == "--hybrid-demo")
            result.hybridDemo = true;
        else if (arg == "--ai-demo")
            result.aiDemo = true;
        else if (arg == "--combat-demo")
            result.combatDemo = true;
        else if (arg == "--seed" && i + 1 < argc) {
            const char* value = argv[++i];
            std::uint32_t seed = 0;
            const auto parsed = std::from_chars(value, value + std::strlen(value), seed);
            if (parsed.ec != std::errc{} || parsed.ptr != value + std::strlen(value))
                throw std::runtime_error("Seed must be an unsigned 32-bit integer");
            result.seed = seed;
        } else if (arg == "--help")
            result.help = true;
        else
            throw std::runtime_error("Unknown or incomplete argument: " + arg);
    }
    if (result.captureMenuOnly &&
        (result.uiDemo || result.combatDemo || result.aiDemo || result.hybridDemo || result.inventoryDemo ||
         result.recoveryDemo || result.checkPreparedOnly || result.captureBuildsOnly ||
         result.captureProgressionOnly || result.captureSpellsOnly || result.captureInventoryOnly ||
         result.captureRecoveryOnly || result.captureProjectilesOnly || result.captureReactionsOnly ||
         result.captureHudOnly || result.captureAiOnly || result.captureHybridOnly ||
         result.capturePreparationOnly || result.captureDoorsOnly))
        throw std::runtime_error("Menu capture must run independently");
    if (int(result.uiDemo) + int(result.combatDemo) + int(result.aiDemo) + int(result.hybridDemo) +
                int(result.inventoryDemo) + int(result.recoveryDemo) >
            1 ||
        (result.captureAiOnly && (result.uiDemo || result.combatDemo)))
        throw std::runtime_error("Choose only one of --ui-demo, --combat-demo or --ai-demo");
    if (int(result.captureDoorsOnly) + int(result.captureAiOnly) + int(result.captureHudOnly) +
            int(result.captureReactionsOnly) + int(result.captureHybridOnly) +
            int(result.capturePreparationOnly) + int(result.captureProjectilesOnly) +
            int(result.captureSpellsOnly) + int(result.captureInventoryOnly) +
            int(result.captureRecoveryOnly) + int(result.captureBuildsOnly) +
            int(result.captureProgressionOnly) >
        1)
        throw std::runtime_error("Choose only one focused capture mode");
    if (result.aiDemo && !result.captureDirectory.empty() && !result.captureAiOnly)
        throw std::runtime_error("Use --capture-ai for the AI demo map");
    if (result.hybridDemo && !result.captureDirectory.empty() && !result.captureHybridOnly)
        throw std::runtime_error("Use --capture-hybrid for hybrid demo");
    if (result.captureHybridOnly && (result.uiDemo || result.combatDemo || result.aiDemo))
        throw std::runtime_error("Hybrid capture cannot combine with another demo");
    if (result.capturePreparationOnly &&
        (result.uiDemo || result.combatDemo || result.aiDemo || result.hybridDemo))
        throw std::runtime_error("Preparation capture cannot combine with a demo");
    if (result.captureSpellsOnly &&
        (result.uiDemo || result.combatDemo || result.aiDemo || result.hybridDemo))
        throw std::runtime_error("Spell capture cannot combine with a demo");
    if ((result.captureInventoryOnly &&
         (result.uiDemo || result.combatDemo || result.aiDemo || result.hybridDemo)) ||
        (result.inventoryDemo && !result.captureDirectory.empty() && !result.captureInventoryOnly))
        throw std::runtime_error("Use --capture-inventory alone for inventory captures");
    if ((result.recoveryDemo && !result.captureDirectory.empty()) ||
        (result.captureRecoveryOnly &&
         (result.uiDemo || result.combatDemo || result.aiDemo || result.hybridDemo || result.inventoryDemo)))
        throw std::runtime_error("Use --capture-recovery alone for recovery captures");
    if (result.width < 320 || result.height < 240)
        throw std::runtime_error("Window must be at least 320 x 240");
    if (result.captureDoorsOnly && !result.worldRenderer)
        throw std::runtime_error("Door captures require the world renderer");
    if (qualitySpecified && !result.worldPreview && !result.worldRenderer)
        throw std::invalid_argument(
            "Supersampling selection requires the world renderer or development world preview");
    if (result.worldPreview &&
        (!result.development || result.captureMenuOnly || result.uiDemo || result.combatDemo ||
         result.aiDemo || result.hybridDemo || result.inventoryDemo || result.recoveryDemo ||
         result.spellDemo || result.checkPreparedOnly || result.captureHudOnly ||
         result.captureReactionsOnly || result.captureProjectilesOnly || result.capturePreparationOnly ||
         result.captureBuildsOnly || result.captureProgressionOnly || result.captureSpellsOnly ||
         result.captureInventoryOnly || result.captureRecoveryOnly ||
         (!result.worldBenchmark.empty() && !result.captureDirectory.empty())))
        throw std::invalid_argument(
            "World preview requires --development and cannot combine with other diagnostics");
    return result;
}
} // namespace blobber
