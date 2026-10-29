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
    const auto apply = [&](GameSession& session, EffectKind kind, unsigned caster) {
        EffectRequest r;
        r.world = session.actors().snapshot().world;
        r.kind = kind;
        r.caster = {std::uint16_t(caster)};
        r.targets[0] = rules::CharacterId{3};
        r.count = 1;
        r.difficulty = 1000;
        if (!session.applyEffect(r))
            throw std::runtime_error("Status capture effect failed");
    };
    apply(buffs, EffectKind::Bless, 1);
    apply(buffs, EffectKind::Bane, 2);
    apply(buffs, EffectKind::ShieldOfFaith, 0);
    apply(buffs, EffectKind::MageArmor, 3);
    apply(buffs, EffectKind::Resistance, 3);
    apply(buffs, EffectKind::Sanctuary, 3);
    buffs.party().grantTemporary({3}, 5, 3600000);
    renderer.compose(map, spawn, frame);
    cards.refresh({}, false, false, true);
    cards.selectCharacter(3);
    cards.openSpellbook();
    const auto save = [&](const char* name, const GameSession& session, const CommandController& controls,
                          const PickingBuffer* pick = nullptr) {
        presenter.present(frame, session.movement(), map, false, true, &controls.snapshot(), pick);
        presenter.capture(directory / (std::string(name) + ".png"));
        presenter.capture(directory / (std::string(name) + "-1280.png"), 1280, 720);
        presenter.capture(directory / (std::string(name) + "-1024.png"), 1024, 768);
    };
    const auto capture = [&](const char* name) { save(name, buffs, cards); };
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
    const auto hoverAt = [](CommandController& controls, UiRect r) {
        UiInput input;
        input.pointer = ArtPoint{float(r.x + 1), float(r.y + 1)};
        routeHudInput(controls, input);
    };
    hoverAt(cards, hudLayout::concentration(1));
    capture("review-concentration-realtime");

    NpcWorld close(map, spawn.cell, names, {{{0}, {8, 10}, {0, 0}, Heading::South}});
    GameSession fight(map, catalog, close, 5, spawn, false, timings);
    for (unsigned n = 0; n < 100 && !fight.combat().snapshot().awaitingAction; ++n)
        fight.update(.1, false);
    if (realTime(fight.combat().snapshot().phase) || !fight.combat().snapshot().awaitingAction)
        throw std::runtime_error("Portrait capture did not enter turn-based combat");
    apply(fight, EffectKind::Bless, 1);
    apply(fight, EffectKind::Bane, 2);
    apply(fight, EffectKind::ShieldOfFaith, 0);
    apply(fight, EffectKind::MageArmor, 3);
    apply(fight, EffectKind::Resistance, 3);
    apply(fight, EffectKind::Sanctuary, 3);
    fight.party().grantTemporary({3}, 5, fight.effects().gameplayMs() + 3600000);
    CommandController turns(fight, timings);
    NpcAnimator combatAnimator(fight.actors(), timings);
    combatAnimator.setTime(fight.timeMs(), &fight.effects());
    renderer.compose(map, spawn, frame, &fight.actors(), &combatAnimator.snapshot(), &picking);
    turns.refresh(picking.targets(), false, false, true);
    hoverAt(turns, hudLayout::status(3, 0));
    save("review-status-turns", fight, turns, &picking);
    hoverAt(turns, hudLayout::status(3, 5));
    save("review-status-overflow-turns", fight, turns, &picking);
    hoverAt(turns, hudLayout::concentration(1));
    save("review-concentration-turns", fight, turns, &picking);
}
} // namespace blobber
