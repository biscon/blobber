#include "probe/legacy.hpp"
#include "assets/image.hpp"
#include <stdexcept>
namespace blobber::probe {
namespace {
rules::Catalog catalog() {
    rules::MonsterDefinition monster;
    monster.id = "probe";
    monster.abilities.fill(10);
    monster.armorClass = 12;
    monster.hitPoints = 7;
    monster.attackCount = 1;
    monster.attacks[0] = {0};
    return {{{"probe.attack", rules::AttackKind::Melee, 2, {1, 4, 0}, rules::DamageType::Piercing}},
            {monster}};
}
} // namespace
Legacy::Legacy(const Scene& scene, const Art& art)
    : catalog_(catalog()),
      placements_(scene.map, scene.spawn.cell, {"probe"}, {{{0}, scene.actor, {0, 0}, Heading::South}}),
      actors_(scene.map, catalog_, placements_), animator_(actors_, {npcTimings(art.npc)}) {
    auto mat = [&](unsigned n) {
        const auto& p = art.surfaces[n];
        return Material(p.width, p.height, p.colors);
    };
    Materials materials{mat(0), mat(1), mat(2)};
    auto prepared = prepareScene(materials);
    bytes = sceneStorageBytes(prepared);
    PreparedNpcs npcs({art.npc});
    bytes += npcs.storageBytes();
    renderer_ = std::make_unique<SceneRenderer>(std::move(prepared), std::move(npcs));
    bytes += 3 * width * height * 4 + 2 * width * height * 2;
    OwnedImage image(GenImageColor(width, height, BLACK));
    texture_ = LoadTextureFromImage(image.value);
    if (!IsTextureValid(texture_)) {
        if (texture_.id)
            UnloadTexture(texture_);
        texture_ = {};
        throw std::runtime_error("Legacy texture allocation failed");
    }
    SetTextureFilter(texture_, TEXTURE_FILTER_POINT);
}
Legacy::~Legacy() {
    if (texture_.id)
        UnloadTexture(texture_);
}
const Texture2D& Legacy::render(const Scene& scene, const MovementState& state, unsigned ms) {
    MovementState coherent = state;
    if (!revision_ || !(previous_.player == state.player) ||
        !(previous_.transition.from == state.transition.from) ||
        !(previous_.transition.to == state.transition.to) || previous_.revision != state.revision)
        ++revision_;
    previous_ = state;
    coherent.revision = revision_;
    animator_.setTime(ms);
    cached = !pipeline_.update(*renderer_, scene.map, coherent, &actors_, &animator_.snapshot());
    if (!cached)
        UpdateTexture(texture_, pipeline_.frame().data());
    return texture_;
}
} // namespace blobber::probe
