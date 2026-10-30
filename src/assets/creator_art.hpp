#pragma once
#include <array>
#include <cstddef>
#include <filesystem>
namespace blobber {
inline constexpr int creatorAtlasSize = 2048, creatorTileSize = 324, creatorTileStride = 328;
inline constexpr std::size_t creatorWorkspaceLimit = 64u * 1024 * 1024;
inline constexpr std::array<const char*, 36> creatorArtNames{{"spells/sacred_flame",
                                                              "spells/spare_the_dying",
                                                              "spells/bless",
                                                              "spells/bane",
                                                              "spells/shield_of_faith",
                                                              "spells/cure_wounds",
                                                              "spells/guiding_bolt",
                                                              "spells/inflict_wounds",
                                                              "spells/fire_bolt",
                                                              "spells/acid_splash",
                                                              "spells/mage_armor",
                                                              "spells/magic_missile",
                                                              "spells/burning_hands",
                                                              "spells/thunderwave",
                                                              "spells/ray_of_frost",
                                                              "spells/shocking_grasp",
                                                              "spells/poison_spray",
                                                              "spells/sleep",
                                                              "spells/false_life",
                                                              "spells/grease",
                                                              "spells/color_spray",
                                                              "spells/guidance",
                                                              "spells/resistance",
                                                              "spells/sanctuary",
                                                              "spells/aid",
                                                              "spells/hold_person",
                                                              "spells/lesser_restoration",
                                                              "spells/spiritual_weapon",
                                                              "spells/scorching_ray",
                                                              "spells/shatter",
                                                              "spells/mirror_image",
                                                              "spells/web",
                                                              "melee",
                                                              "items/holy_symbol",
                                                              "items/dagger",
                                                              "items/spellbook"}};
void prepareCreatorArt(const std::filesystem::path&, const std::filesystem::path&);
} // namespace blobber
