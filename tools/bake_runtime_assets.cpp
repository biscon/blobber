#include "assets/fingerprint.hpp"
#include "assets/material_loader.hpp"
#include "assets/npc_derivatives.hpp"
#include "assets/npc_loader.hpp"
#include "assets/preparation_profile.hpp"
#include "assets/prepared_assets.hpp"
#include "assets/prepared_binary.hpp"
#include "game/movement.hpp"
#include <chrono>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace {
namespace fs = std::filesystem;
using namespace blobber;
using Json = nlohmann::json;
Json inputs(const fs::path& root) {
    Json result = Json::object();
    for (const auto& directory : {"npcs", "textures", "effects"})
        for (const auto& entry : fs::recursive_directory_iterator(root / directory))
            if (entry.is_regular_file() &&
                (entry.path().extension() == ".json" || entry.path().extension() == ".png"))
                result[fs::relative(entry.path(), root).generic_string()] = fingerprintFile(entry.path());
    for (const auto& file : {"preparation.json", "maze.txt"})
        result[file] = fingerprintFile(root / file);
    return result;
}
int bake(const fs::path& root, const fs::path& output, bool capacity) {
    const auto start = std::chrono::steady_clock::now();
    if (!fs::exists(root / "effects/grease.png"))
        throw std::runtime_error("Missing Grease ground artwork");
    const auto profile = loadPreparationProfile(root / "preparation.json");
    const auto map = Map::load((root / "maze.txt").string());
    const auto metadata = loadNpcMetadata(root / "npcs/level.json", map, Player{}.cell);
    for (const auto& timing : metadata.timings)
        if (timing[static_cast<std::size_t>(NpcAnimation::Prone)].count != 1)
            throw std::runtime_error(
                "Every gameplay NPC appearance requires a single-frame four-direction prone clip");
    const auto inputHashes = inputs(root);
    fs::create_directories(output);
    // Publish the manifest last; an interrupted bake cannot look complete to CMake.
    fs::remove(output / "metadata.json");
    Json report{{"format", prepared_io::formatVersion},
                {"renderer_revision", prepared_io::rendererRevision},
                {"profile", profile},
                {"definitions", metadata.definitionFingerprint},
                {"inputs", inputHashes}};
    std::size_t sceneBytes = 0;
    ScenePreparationStats sceneStats;
    {
        const auto scene = prepareScene(loadMaterials(root / "textures"), {}, &sceneStats);
        sceneBytes = sceneStorageBytes(scene);
        requirePreparationBytes(0, sceneBytes, PreparationLimits{}.residentBytes);
        savePreparedScene(output / "scene.bin", scene, profile);
    }
    std::size_t derivativeHighWater = 0;
    const auto derivatives =
        prepareNpcDerivatives(root / "npcs/level.json", output / "derivatives", &derivativeHighWater);
    auto sources = loadNpcs(derivatives, map, Player{}.cell);
    if (sources.definitionFingerprint != metadata.definitionFingerprint)
        throw std::runtime_error("Derivative definitions differ from runtime metadata");
    for (auto& appearance : sources.appearances)
        appearance.sampling = ImageSampling::Area;
    {
        const PreparedNpcs npcs(sources.appearances, {PreparationLimits{}.residentBytes - sceneBytes,
                                                      PreparationLimits{}.loadingBytes});
        savePreparedNpcs(output / "npcs.bin", npcs, metadata.timings, metadata.definitionFingerprint,
                         profile);
        report["bytes"] = {{"scene_resident_bake", sceneBytes},
                           {"npc_source_pixels", npcs.sourceBytes()},
                           {"scene_source", sceneStats.sourceBytes},
                           {"scene_loading_high_water", sceneStats.loadingHighWater},
                           {"npc_stationary_pixels", npcs.stationaryPixelBytes()},
                           {"npc_travel_pixels", npcs.travelPixelBytes()},
                           {"npc_shadow_pixels", npcs.preparedPixelBytes() - npcs.stationaryPixelBytes() -
                                                     npcs.travelPixelBytes()},
                           {"npc_metadata_capacity", npcs.storageBytes() - npcs.preparedPixelBytes()},
                           {"npc_resident", npcs.storageBytes()},
                           {"resident_total_bake", sceneBytes + npcs.storageBytes()},
                           {"derivative_loading_high_water", derivativeHighWater},
                           {"tracked_loading_high_water",
                            std::max({derivativeHighWater, sceneStats.loadingHighWater,
                                      sources.sourceDecodeHighWater, npcs.loadingHighWaterBytes()})},
                           {"npc_source_decode_high_water", sources.sourceDecodeHighWater},
                           {"npc_raster_loading_high_water", npcs.loadingHighWaterBytes()}};
    }
    if (capacity) {
        {
            const auto duplicate = sources.appearances;
            sources.appearances.insert(sources.appearances.end(), duplicate.begin(), duplicate.end());
        }
        const PreparedNpcs two(sources.appearances, {PreparationLimits{}.residentBytes - sceneBytes,
                                                     PreparationLimits{}.loadingBytes});
        std::cout << "Two complete appearance sets: resident=" << sceneBytes + two.storageBytes()
                  << " npc_raster_loading=" << two.loadingHighWaterBytes() << '\n';
    }
    for (const auto& file : {"scene.bin", "npcs.bin"})
        report["outputs"][file] = {{"bytes", fs::file_size(output / file)},
                                   {"fingerprint", fingerprintFile(output / file)},
                                   {"checksum", preparedBundleChecksum(output / file)}};
    if (inputs(root) != inputHashes)
        throw std::runtime_error("Source assets changed during bake; retry");
    const auto temporary = output / "metadata.json.tmp";
    {
        std::ofstream out(temporary);
        out << report.dump(2) << '\n';
        if (!out)
            throw std::runtime_error("Cannot write bake manifest");
    }
    fs::rename(temporary, output / "metadata.json");
    std::cout << report.at("bytes").dump(2) << "\nBake milliseconds: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                                       start)
                     .count()
              << '\n';
    return 0;
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc < 3 || argc > 4 || (argc == 4 && std::string(argv[3]) != "--capacity-check"))
            throw std::runtime_error(
                "Usage: blobber_bake_assets SOURCE_ASSETS OUTPUT_PREPARED [--capacity-check]");
        SetTraceLogLevel(LOG_WARNING);
        return bake(fs::absolute(argv[1]), fs::absolute(argv[2]), argc == 4);
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
