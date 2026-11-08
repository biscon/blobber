#include "ui/art.hpp"
#include "assets/creator_art.hpp"
#include "assets/image.hpp"
#include "assets/map_icons.hpp"
#include "assets/portrait_atlas.hpp"
#include "assets/prepared_image.hpp"
#include "core/preparation.hpp"
#include "game/effects.hpp"
#include "ui/map_model.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
namespace blobber {
UiArt::UiArt(GpuResources& resources, const std::filesystem::path& directory,
             const std::filesystem::path& preparedDirectory) : resources_(resources) {
    constexpr const char* names[]{"alden",
                                  "brunna",
                                  "merrin",
                                  "lethiel",
                                  "goblin",
                                  "melee",
                                  "defend",
                                  "ranged",
                                  "spell",
                                  "backpack",
                                  "aid",
                                  "spells/sacred_flame",
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
                                  "spells/second_wind",
                                  "spells/arcane_recovery",
                                  "spells/dismiss_concentration",
                                  "class_features",
                                  "items/longsword",
                                  "items/chain_mail",
                                  "items/shield",
                                  "items/mace",
                                  "items/scale_mail",
                                  "items/holy_symbol",
                                  "items/shortsword",
                                  "items/leather",
                                  "items/shortbow",
                                  "items/arrow",
                                  "items/dagger",
                                  "items/spellbook",
                                  "items/arcane_focus",
                                  "items/healing_potion",
                                  "items/mana_potion",
                                  "items/silhouette",
                                  "rest",
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
                                  "spells/hold_person",
                                  "items/quarterstaff",
                                  "travel",
                                  "skeleton",
                                  "bugbear",
                                  "giant-rat",
                                  "zombie",
                                  "goblin-shaman"};
    const auto prepared = preparedDirectory.empty() ? directory.parent_path() / "prepared" : preparedDirectory;
    auto loadPreparedAtlas = [&](const char* name, int height) {
        preparationPoint("Loading interface atlases");
        const auto path = prepared / name;
        std::ifstream input(path, std::ios::binary);
        std::array<unsigned char, 14> header{};
        input.read(reinterpret_cast<char*>(header.data()), header.size());
        auto dimension = [&](int n) {
            return (std::uint32_t(header[n]) << 24) | (std::uint32_t(header[n + 1]) << 16) |
                   (std::uint32_t(header[n + 2]) << 8) | header[n + 3];
        };
        if (!input || dimension(0) != 0x716f6966 || dimension(4) != portraitAtlasWidth ||
            dimension(8) != unsigned(height) || header[12] != 4 ||
            std::filesystem::file_size(path) > std::uint64_t(portraitAtlasWidth) * height * 5 + 22)
            throw std::runtime_error("Missing or invalid prepared UI atlas header");
        OwnedImage atlas(preparedImage(path));
        if (!atlas.value.data || atlas.value.width != portraitAtlasWidth || atlas.value.height != height)
            throw std::runtime_error("Missing or invalid prepared UI atlas");
        auto handle = resources_.createTexture(atlas.value);
        SetTextureFilter(resources_.texture(handle), TEXTURE_FILTER_BILINEAR);
        return handle;
    };
    creator_ = loadPreparedAtlas("creator.qoi", creatorAtlasSize);
    portraitSmall_ = loadPreparedAtlas("portraits-small.qoi", smallPortraitHeight);
    portraitLarge_ = loadPreparedAtlas("portraits-large.qoi", largePortraitHeight);
    OwnedImage portraitEffects(preparedImage(directory.parent_path() / "effects/se02/portraits.png"));
    if (!portraitEffects.value.data || portraitEffects.value.width != 1024 ||
        portraitEffects.value.height != 512)
        throw std::runtime_error("Missing or invalid portrait effects atlas");
    portraitEffects_ = resources_.createTexture(portraitEffects.value);
    SetTextureFilter(resources_.texture(portraitEffects_), TEXTURE_FILTER_BILINEAR);
    OwnedImage supportEffects(preparedImage(directory.parent_path() / "effects/se03/portraits.png"));
    if (!supportEffects.value.data || supportEffects.value.width != 1024 ||
        supportEffects.value.height != 1024)
        throw std::runtime_error("Missing or invalid support portrait effects atlas");
    supportEffects_ = resources_.createTexture(supportEffects.value);
    SetTextureFilter(resources_.texture(supportEffects_), TEXTURE_FILTER_BILINEAR);
    OwnedImage feedback(preparedImage(directory.parent_path() / "effects/combat-feedback/backdrops.png"));
    if (!feedback.value.data || feedback.value.width != 768 || feedback.value.height != 128)
        throw std::runtime_error("Missing or invalid combat feedback atlas");
    feedback_ = resources_.createTexture(feedback.value);
    SetTextureFilter(resources_.texture(feedback_), TEXTURE_FILTER_BILINEAR);
    OwnedImage campImage(preparedImage(directory / "camp/bonfire-sheet.png"));
    OwnedImage lootImage(preparedImage(directory / "items/loot-sheet.png"));
    if (!campImage.value.data || !lootImage.value.data)
        throw std::runtime_error("Missing camp/loot art");
    camp_ = resources_.createTexture(campImage.value);
    loot_ = resources_.createTexture(lootImage.value);
    SetTextureFilter(resources_.texture(camp_), TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(resources_.texture(loot_), TEXTURE_FILTER_BILINEAR);
    {
        preparationPoint("Preparing map artwork");
        OwnedImage sheet(preparedImage(directory / "map/markers-v1.png"));
        if (!sheet.value.data)
            throw std::runtime_error("Missing map icon artwork");
        ImageFormat(&sheet.value, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        OwnedImage mapAtlas(prepareMapIconAtlas(sheet.value));
        map_ = resources_.createTexture(mapAtlas.value);
        SetTextureFilter(resources_.texture(map_), TEXTURE_FILTER_BILINEAR);
    }
    OwnedImage atlas(GenImageColor(2048, 2048, BLANK));
    if (!atlas.value.data)
        throw std::runtime_error("Cannot allocate UI atlas");
    for (unsigned i = 0; i < regions_.size(); ++i) {
        preparationPoint("Preparing interface artwork");
        if (i < 4)
            continue; // Portraits are prepared offline in their own bounded atlases.
        const auto path = directory / (std::string(names[i]) + ".png");
        OwnedImage image(preparedImage(path));
        if (!image.value.data)
            throw std::runtime_error("Missing UI art: " + path.string());
        const int maximumWidth = i >= 46               ? 108
                                 : i == silhouetteIcon ? 384
                                 : i >= 29             ? 168
                                 : i < 4               ? 216
                                                       : (i == 4 ? 72 : 108);
        const int maximumHeight = i == silhouetteIcon ? 600 : i < 4 ? 252 : maximumWidth;
        const float factor =
            std::min(float(maximumWidth) / image.value.width, float(maximumHeight) / image.value.height);
        const int w = std::max(1, int(std::round(image.value.width * factor)));
        const int h = std::max(1, int(std::round(image.value.height * factor)));
        ImageResize(&image.value, w, h);
        const int x = i >= 46               ? 2 + int(i - 46) * 112
                      : i == 45             ? 1794
                      : i == silhouetteIcon ? 1026
                                            : int(i % 8) * 256 + 2;
        const int y = i >= 46 ? 1900 : i == silhouetteIcon ? 1282 : int(i / 8) * 256 + 2;
        regions_[i] = {float(x), float(y), float(w), float(h)};
        ImageDrawImage(&atlas.value, image.value, x, y, WHITE);
        // Extrude two pixels so bilinear sampling never reaches adjacent/blank cells.
        for (int py = -2; py < h + 2; ++py)
            for (int px = -2; px < w + 2; ++px)
                if (px < 0 || px >= w || py < 0 || py >= h)
                    ImageDrawPixel(
                        &atlas.value, x + px, y + py,
                        GetImageColor(image.value, std::clamp(px, 0, w - 1), std::clamp(py, 0, h - 1)));
    }
    atlas_ = resources_.createTexture(atlas.value);
    SetTextureFilter(resources_.texture(atlas_), TEXTURE_FILTER_BILINEAR);
}
static_assert(static_cast<unsigned>(rules::SpellKind::Count) == 32,
              "Extend the spell atlas mapping for new spells");
unsigned UiArt::itemIcon(const rules::ItemDefinition& item) noexcept {
    if (item.id == "quarterstaff")
        return quarterstaffIcon;
    if (item.id == "scimitar")
        return 45;
    if (item.kind == rules::ItemKind::Currency)
        return 46;
    if (item.kind == rules::ItemKind::Supply)
        return 47;
    constexpr const char* ids[]{"longsword",   "chain_mail", "shield",       "mace",           "scale_mail",
                                "holy_symbol", "shortsword", "leather",      "shortbow",       "arrow",
                                "dagger",      "spellbook",  "arcane_focus", "healing_potion", "mana_potion"};
    for (unsigned i = 0; i < 15; ++i)
        if (item.id == ids[i])
            return 29 + i;
    return 9;
}
unsigned UiArt::spellIcon(rules::SpellKind kind, rules::Feature feature) noexcept {
    if (feature == rules::Feature::SecondWind)
        return 25;
    if (feature == rules::Feature::ArcaneRecovery)
        return 26;
    if (feature == rules::Feature::DismissConcentration)
        return 27;
    const auto k = static_cast<unsigned>(kind);
    if (k >= 24 && k < 32)
        return 128 + k;
    return kind < rules::SpellKind::Count ? (k < 14 ? 11 + k : 49 + k - 14) : 8;
}
unsigned UiArt::effectIcon(EffectKind kind) noexcept {
    return kind == EffectKind::HoldPerson ? holdPersonIcon : spellIcon(effectSpell(kind));
}
void UiArt::mapMarker(MapMarker kind, Rectangle destination, Heading facing) const {
    const auto n = unsigned(kind);
    if (!n || n > mapIconCount)
        return;
    const int index = int(n - 1);
    const Rectangle source{float(index % mapIconColumns * mapIconStride + mapIconPadding),
                           float(index / mapIconColumns * mapIconStride + mapIconPadding), float(mapIconSize),
                           float(mapIconSize)};
    constexpr float scale = float(gameWidth) / artWidth;
    const float w = destination.width * scale, h = destination.height * scale;
    const Rectangle target{destination.x * scale + w * .5f, destination.y * scale + h * .5f, w, h};
    const float rotation = kind == MapMarker::Party ? float(unsigned(facing) % 4) * 90.f : 0.f;
    DrawTexturePro(resources_.texture(map_), source, target, {w * .5f, h * .5f}, rotation, WHITE);
}
void UiArt::draw(unsigned index, UiRect r, Color tint) const {
    if (index >= 128 && index < 160) {
        creator(index - 128, r, tint);
        return;
    }
    constexpr float scale = float(gameWidth) / artWidth;
    Rectangle destination{r.x * scale, r.y * scale, r.width * scale, r.height * scale};
    if (index < 4)
        index += 64; // Retain diagnostic compatibility for the original four art IDs.
    if (index >= 64 && index < 64 + rules::portraitCount) {
        const bool large = destination.width > 216 || destination.height > 252;
        const auto region = portraitRegion(index - 64, large);
        Rectangle source{float(region.x), float(region.y), float(region.width), float(region.height)};
        const auto ratio = destination.width / destination.height;
        if (source.width / source.height > ratio) {
            auto w = source.height * ratio;
            source.x += (source.width - w) * .5f;
            source.width = w;
        } else {
            auto h = source.width / ratio;
            source.y += (source.height - h) * .5f;
            source.height = h;
        }
        DrawTexturePro(resources_.texture(large ? portraitLarge_ : portraitSmall_), source, destination,
                       {0, 0}, 0, tint);
        return;
    }
    if (index >= 45 && index <= 47) {
        const auto& texture = resources_.texture(loot_);
        const int tile = index == 45 ? 0 : index == 46 ? 5 : 8;
        const float w = texture.width / 3.f, h = texture.height / 3.f;
        DrawTexturePro(texture, {tile % 3 * w, tile / 3 * h, w, h}, destination, {0, 0}, 0, tint);
        return;
    }
    if (index == shamanPortraitIcon)
        index = 63;
    else if (index == zombiePortraitIcon)
        index = 62;
    else if (index == giantRatPortraitIcon)
        index = 61;
    else if (index == bugbearPortraitIcon)
        index = 60;
    else if (index == skeletonPortraitIcon)
        index = 59;
    else if (index >= 49 && index <= travelIcon)
        index -= 3;
    else if (index == restIcon)
        index = 45;
    if (index >= regions_.size()) {
        DrawRectangleRec(destination, {19, 26, 25, 255});
        DrawCircleV({destination.x + destination.width / 2, destination.y + destination.height / 3},
                    destination.width / 5, {101, 115, 108, 255});
        DrawRectangleRec({destination.x + destination.width / 5, destination.y + destination.height / 2,
                          destination.width * 3 / 5, destination.height * 2 / 5},
                         {101, 115, 108, 255});
        return;
    }
    auto source = regions_[index];
    if (index < 5 || index == 59 || index == 60 || index == 61 || index == 62) {
        // Portraits cover their card without stretching faces.
        const float ratio = destination.width / destination.height;
        if (source.width / source.height > ratio) {
            const float width = source.height * ratio;
            source.x += (source.width - width) * .5f;
            source.width = width;
        } else {
            const float height = source.width / ratio;
            source.y += (source.height - height) * .5f;
            source.height = height;
        }
    } else {
        const float fit = std::min(destination.width / source.width, destination.height / source.height);
        const float width = source.width * fit, height = source.height * fit;
        destination.x += (destination.width - width) * .5f;
        destination.y += (destination.height - height) * .5f;
        destination.width = width;
        destination.height = height;
    }
    DrawTexturePro(resources_.texture(atlas_), source, destination, {0, 0}, 0, tint);
}
void UiArt::creator(unsigned index, UiRect r, Color tint) const {
    if (index >= creatorArtNames.size())
        return;
    const Rectangle source{float(2 + (index % 6) * creatorTileStride),
                           float(2 + (index / 6) * creatorTileStride), float(creatorTileSize),
                           float(creatorTileSize)};
    DrawTexturePro(resources_.texture(creator_), source,
                   {float(r.x * 3), float(r.y * 3), float(r.width * 3), float(r.height * 3)}, {0, 0}, 0,
                   tint);
}
void UiArt::camp(unsigned frame, UiRect r, Color tint) const {
    constexpr float scale = float(gameWidth) / artWidth;
    const Rectangle destination{r.x * scale, r.y * scale, r.width * scale, r.height * scale};
    const auto& texture = resources_.texture(camp_);
    const float w = texture.width / 2.f, h = texture.height / 2.f;
    // Keep the camera/characters perfectly still; animate only the fire region.
    DrawTexturePro(texture, {0, 0, w, h}, destination, {0, 0}, 0, tint);
    frame %= 4;
    if (frame) {
        constexpr float x = .465f, y = .24f, width = .19f, height = .73f;
        DrawTexturePro(texture, {(frame % 2 + x) * w, (frame / 2 + y) * h, width * w, height * h},
                       {destination.x + x * destination.width, destination.y + y * destination.height,
                        width * destination.width, height * destination.height},
                       {0, 0}, 0, tint);
    }
}
void UiArt::portraitEffect(unsigned row, unsigned frame, UiRect r, Color tint) const {
    if (row >= 4 || frame >= 8)
        return;
    constexpr float scale = float(gameWidth) / artWidth;
    DrawTexturePro(resources_.texture(portraitEffects_),
                   {float(frame * 128 + 8), float(row * 128 + 8), 112, 112},
                   {r.x * scale, r.y * scale, r.width * scale, r.height * scale}, {0, 0}, 0, tint);
}
void UiArt::supportEffect(unsigned row, unsigned frame, UiRect r, Color tint) const {
    if (row >= 8 || frame >= 8)
        return;
    constexpr float scale = float(gameWidth) / artWidth;
    DrawTexturePro(resources_.texture(supportEffects_),
                   {float(frame * 128 + 8), float(row * 128 + 8), 112, 112},
                   {r.x * scale, r.y * scale, r.width * scale, r.height * scale}, {0, 0}, 0, tint);
}
void UiArt::feedbackBackdrop(FeedbackBackdrop kind, UiRect r, Color tint) const {
    if (unsigned(kind) >= 3)
        return;
    constexpr float scale = float(gameWidth) / artWidth;
    DrawTexturePro(resources_.texture(feedback_), {float(unsigned(kind) * 256 + 16), 8, 224, 112},
                   {r.x * scale, r.y * scale, r.width * scale, r.height * scale}, {0, 0}, 0, tint);
}
} // namespace blobber
