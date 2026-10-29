#include "app/ai_demo.hpp"
#include "app/spell_captures.hpp"
#include "ui/interaction.hpp"
#include <stdexcept>
namespace blobber {
// Frozen presentation scenarios: application-owned offscreen captures, never OS input.
void captureSpellReview(const rules::Catalog& catalog, const NpcWorld& source,
                        const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                        Presenter& presenter, const std::filesystem::path& directory) {
    const auto map = aiDemoMap();
    const Player spawn{{8, 11}, Heading::North};
    std::vector<std::string> names;
    for (std::size_t i = 0; i < source.typeCount(); ++i)
        names.push_back(source.typeId({std::uint16_t(i)}));
    std::vector<NpcInstance> instances;
    for (unsigned i = 0; i < names.size(); ++i)
        instances.push_back({{std::uint16_t(i)}, {8, 8}, {int(i % 2), 0}, Heading::South});
    NpcWorld placements(map, spawn.cell, names, instances);
    GameSession s(map, catalog, placements, 5, spawn, false, timings);
    CommandController ui(s, timings);
    NpcAnimator animator(s.actors(), timings);
    Frame frame;
    PickingBuffer picking;
    PartyCommand sleep;
    sleep.character = {3};
    sleep.action = PartyAction::Spell;
    sleep.spell = catalog.findSpell("sleep");
    sleep.aimedCell = Cell{8, 8};
    sleep.world = s.actors().snapshot().world;
    sleep.turn = s.combat().snapshot().turn;
    if (s.submit(sleep).reason != CommandReason::Resolved)
        throw std::runtime_error("Prone capture cast failed");
    s.update(.5, false);
    for (std::size_t i = 0; i < s.actors().snapshot().count; ++i)
        if (!s.effects().has(s.actors().id(i), EffectKind::Sleep))
            throw std::runtime_error("Prone capture missing sleeper");
    animator.setTime(s.timeMs(), &s.effects());
    const auto area = ui.floorPreview();
    for (unsigned heading = 0; heading < 4; ++heading) {
        MovementState movement;
        movement.player = {Cell{8, 8} + forward(Heading(heading)) * -2, Heading(heading)};
        renderer.compose(map, movement.player, frame, &s.actors(), &animator.snapshot(), &picking, nullptr,
                         &area);
        ui.refresh(picking.targets(), false, false, true);
        presenter.present(frame, movement, map, false, true, &ui.snapshot(), &picking);
        const auto name = "review-prone-" + std::to_string(heading);
        presenter.capture(directory / (name + ".png"));
        presenter.capture(directory / (name + "-1280.png"), 1280, 720);
        presenter.capture(directory / (name + "-1024.png"), 1024, 768);
    }
    // Portrait overflow mixes benefits and a hostile debuff, using real effect instances.
    NpcWorld empty(map, spawn.cell, names, {});
    GameSession buffs(map, catalog, empty, 5, spawn, false, timings);
    CommandController cards(buffs, timings);
    const auto apply = [&](EffectKind kind, unsigned caster) {
        EffectRequest r;
        r.world = buffs.actors().snapshot().world;
        r.kind = kind;
        r.caster = {std::uint16_t(caster)};
        r.targets[0] = rules::CharacterId{3};
        r.count = 1;
        r.difficulty = 1000;
        if (!buffs.applyEffect(r))
            throw std::runtime_error("Status capture effect failed");
    };
    apply(EffectKind::Bless, 1);
    apply(EffectKind::Bane, 2);
    apply(EffectKind::ShieldOfFaith, 0);
    apply(EffectKind::MageArmor, 3);
    apply(EffectKind::Resistance, 3);
    apply(EffectKind::Sanctuary, 3);
    buffs.party().grantTemporary({3}, 5, 3600000);
    renderer.compose(map, spawn, frame);
    cards.refresh({}, false, false, true);
    cards.selectCharacter(3);
    cards.openSpellbook();
    const auto capture = [&](const char* name) {
        presenter.present(frame, buffs.movement(), map, false, true, &cards.snapshot());
        presenter.capture(directory / (std::string(name) + ".png"));
        presenter.capture(directory / (std::string(name) + "-1280.png"), 1280, 720);
        presenter.capture(directory / (std::string(name) + "-1024.png"), 1024, 768);
    };
    capture("review-tabs-cantrips");
    cards.selectSpellbookTab(1);
    capture("review-tabs-level-one");
    cards.selectSpellbookTab(MagicHud::abilityTab);
    capture("review-tabs-abilities");
    for (unsigned n : {0u, 1u, 5u}) {
        UiInput hover;
        const auto r = hudLayout::status(3, int(n));
        hover.pointer = ArtPoint{float(r.x + 1), float(r.y + 1)};
        routeHudInput(cards, hover);
        capture(n == 0   ? "review-status-benefit"
                : n == 1 ? "review-status-hostile"
                         : "review-status-overflow");
    }
}
} // namespace blobber
