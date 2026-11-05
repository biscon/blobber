#include "game/target_visibility.hpp"
#include "support/allocations.hpp"
#include "support/check.hpp"
#include "support/spell_fixtures.hpp"
#include "support/world_visibility_fixtures.hpp"
#include "ui/interaction.hpp"
#include <cstring>
#include <iostream>
using namespace blobber;
namespace {
void playerNames() {
    const auto map = hybridRoom();
    const auto catalog = hybridCatalog(false, "goblin.guardian.placeholder");
    const Player pose{{3, 6}, Heading::North};
    NpcWorld places(map, pose.cell, {"goblin.guardian.placeholder"}, {{{0}, {3, 3}, {0, 0}, Heading::South}});
    GameSession game(map, catalog, places, 5, pose, false);
    CommandController ui(game);
    ui.refresh(allCombatTargets(game), false, false, true);
    ui.hover(HoverKind::Enemy, 0, game.actors().id(0));
    check(std::string_view(ui.snapshot().hover.lines[0].data()) == "Goblin Guardian #1",
          "Enemy tooltip hides internal guardian placeholder identity");
    ui.toggleTargetPicker();
    check(ui.snapshot().targetPicker.count == 1 &&
              std::string_view(ui.snapshot().targetPicker.rows[0].name.data()) == "Goblin Guardian #1",
          "Target picker uses the same readable creature identity");
}
void key(CommandController& ui, UiKey k, bool shift = false) {
    UiInput input;
    input.pressed[unsigned(k)] = true;
    input.shift = shift;
    routeHudInput(ui, input);
}
void choose(CommandController& ui, rules::SpellKind kind) {
    ui.selectCharacter(3);
    ui.openSpellbook();
    for (std::size_t i = 0; i < ui.snapshot().magic.count; ++i)
        if (ui.snapshot().magic.entries[i].kind == kind) {
            ui.chooseSpellbook(i);
            return;
        }
    throw std::runtime_error("Missing owned spell fixture");
}
void controls() {
    const auto map = test::worldRoom(9, 9);
    const auto catalog = spellCatalog();
    const Player pose{{3, 6}, Heading::North};
    NpcWorld places(map, pose.cell, {"goblin"},
                    {{{0}, {3, 3}, {0, 0}, Heading::South},
                     {{0}, {3, 3}, {1, 0}, Heading::South},
                     {{0}, {3, 3}, {0, 1}, Heading::South},
                     {{0}, {3, 3}, {1, 1}, Heading::South},
                     {{0}, {3, 2}, {0, 0}, Heading::South},
                     {{0}, {3, 2}, {1, 0}, Heading::South},
                     {{0}, {7, 1}, {0, 0}, Heading::South}});
    GameSession game(map, catalog, places, 5, pose, false);
    CommandController ui(game);
    const auto all = allCombatTargets(game);
    ui.refresh(all, false, false, true);
    ui.selectCharacter(2);
    ui.selectAction(PartyAction::Ranged);
    ui.toggleTargetPicker();
    check(ui.snapshot().targetPicker.count == 4, "Picker first page is bounded to four rows");
    ui.pageTargets(1);
    check(ui.snapshot().targetPage == 1 && ui.snapshot().targetPicker.count == 3,
          "Picker pages remaining enemies");
    ui.pageTargets(1);
    check(ui.snapshot().targetPage == 0, "Picker pages wrap");
    ui.closeTargetPicker();
    key(ui, UiKey::Tab);
    const auto first = *ui.snapshot().selection.target;
    std::array<bool, NpcWorld::maxInstances> visited{};
    for (unsigned i = 0; i < 6; ++i) {
        visited[ui.snapshot().selection.target->slot] = true;
        key(ui, UiKey::Tab);
    }
    check(sameActor(*ui.snapshot().selection.target, first) && !visited[6],
          "Cycle visits every eligible enemy once and skips out-of-range enemy");
    ui.toggleTargetPicker();
    ui.pageTargets(1);
    ui.activateTargetRow(2);
    check(sameActor(*ui.snapshot().selection.target, first) && ui.snapshot().targetPicker.feedback[0],
          "Blocked row explains rejection without replacing target");
    ui.closeTargetPicker();
    ui.selectAction(PartyAction::Melee);
    ui.cancel();
    key(ui, UiKey::Tab);
    check(!ui.snapshot().selection.target && ui.snapshot().targetPicker.feedback[0],
          "No eligible targets gives feedback without submitting");
    ui.toggleTargetPicker();
    ui.targetPickerConfirm();
    check(ui.snapshot().targetPicker.open && ui.snapshot().targetPicker.feedback[0],
          "Rejected confirmation preserves picker and explains rejection");
    ui.selectAction(PartyAction::Ranged);
    ui.selectTarget(first);
    ui.refresh({}, false, false, true);
    check(!ui.snapshot().selection.target && ui.snapshot().targets.candidateCount == 0,
          "Lost candidate list clears selection without retargeting");
    key(ui, UiKey::Tab);
    check(!ui.snapshot().selection.target, "Empty candidate cycle is safe");
    UiInputEdges edges;
    std::array<bool, uiKeyCount> keys{};
    keys[unsigned(UiKey::Tab)] = true;
    check(edges.sample(keys, true).has(UiKey::Tab), "Tab starts on down edge");
    check(!edges.sample(keys, true).has(UiKey::Tab), "Holding Tab never repeats");
    edges.sample(keys, false);
    check(!edges.sample(keys, true).has(UiKey::Tab), "Held Tab on focus return cannot cycle");
}
void targeting() {
    test::VisibilityFixture fixture;
    auto publication = fixture.publication();
    publication.appearances[0].width = .5f;
    publication.appearances[0].height = .9f;
    world_assets::CpuAssets assets(std::move(publication));
    const auto assetsLease = assets.acquire(world_assets::allAssets(assets.publication));
    // Solid owned silhouettes make total occlusion deterministic.
    for (auto& mask : assets.masks)
        std::fill(mask.begin(), mask.end(), 255);
    const auto map = test::worldRoom(9, 9);
    const auto catalog = spellCatalog();
    const Player pose{{3, 6}, Heading::North};
    NpcWorld places(map, pose.cell, {"goblin"},
                    {{{0}, {3, 3}, {1, 1}, Heading::South}, {{0}, {3, 2}, {1, 1}, Heading::South}});
    GameSession game(map, catalog, places, 5, pose, false, fixture.timings);
    const auto scene = world_render::prepare(game.geometry());
    world_render::VisibilityPipeline pipeline(scene, assets);
    pipeline.update({game.geometry(), game.movement(), &game.actors()});
    const auto snapshot = pipeline.frame().picking.targets();
    const auto front = game.actors().id(0), rear = game.actors().id(1);
    check(snapshot.count == 1 && sameActor(snapshot.ids[0], front) && snapshot.candidateCount == 2,
          "Covered rear actor is a geometry candidate without visible body pixels");
    check(!snapshot.anchors[rear.slot].valid, "Covered actor has no presentation anchor");
    for (int y = 0; y < viewHeight; ++y)
        for (int x = 0; x < viewWidth; ++x) {
            const auto hit = pipeline.frame().picking.pick(x, y);
            check(!hit || !sameActor(*hit, rear), "Body picking never reaches covered actor");
        }
    {
        std::vector<std::string> rows(9, std::string(9, '.'));
        rows.front() = rows.back() = std::string(9, '#');
        for (auto& row : rows)
            row.front() = row.back() = '#';
        // A solid cross-corridor wall must remove candidates, not merely their bodies.
        rows[4] = std::string(9, '#');
        const Map blocked(rows);
        NpcWorld blockedPlaces(blocked, pose.cell, {"goblin"}, {{{0}, {3, 3}, {1, 1}, Heading::South}});
        GameSession hidden(blocked, catalog, blockedPlaces, 5, pose, false, fixture.timings);
        const auto wallScene = world_render::prepare(hidden.geometry());
        world_render::VisibilityPipeline walls(wallScene, assets);
        walls.update({hidden.geometry(), hidden.movement(), &hidden.actors()});
        check(walls.frame().picking.targets().candidateCount == 0, "Walls exclude covered candidates");
        FloorFixtures doors;
        doors.count = 1;
        doors.entries[0] = {{3, 4}, Heading::North, FixtureKind::Door};
        GameSession closed(map, catalog, places, 5, pose, false, fixture.timings, {}, doors);
        const auto doorScene = world_render::prepare(closed.geometry());
        world_render::VisibilityPipeline doorView(doorScene, assets);
        doorView.update(
            {closed.geometry(), closed.movement(), &closed.actors(), nullptr, nullptr, &closed.doors()});
        check(doorView.frame().picking.targets().candidateCount == 0, "Closed doors exclude candidates");
    }
    CommandController ui(game);
    ui.refresh(snapshot, false, false, true);
    ui.selectCharacter(2);
    ui.selectAction(PartyAction::Ranged);
    const auto turn = game.combat().snapshot().turn;
    key(ui, UiKey::Tab);
    check(ui.snapshot().selection.target && sameActor(*ui.snapshot().selection.target, front),
          "First Tab selects nearest eligible enemy");
    key(ui, UiKey::Tab);
    check(sameActor(*ui.snapshot().selection.target, rear) &&
              ui.snapshot().availability == CommandReason::Ready,
          "Second Tab selects and validates a fully covered rear target");
    check(game.combat().snapshot().turn == turn && game.combat().snapshot().pendingCount == 0,
          "Cycling does not submit attacks");
    key(ui, UiKey::Tab, true);
    check(sameActor(*ui.snapshot().selection.target, front), "Shift Tab reverses");
    key(ui, UiKey::Tab, true);
    check(sameActor(*ui.snapshot().selection.target, rear), "Reverse wraps");
    ui.toggleTargetPicker();
    check(ui.snapshot().targetPicker.open && ui.snapshot().targetPicker.count == 2,
          "Mouse picker includes both enemies");
    ui.activateTargetRow(1);
    check(sameActor(*ui.snapshot().selection.target, rear) &&
              std::strstr(ui.snapshot().targetPicker.readout.data(), "Obscured"),
          "Picker previews hidden target and explains overlap");
    key(ui, UiKey::Cancel);
    check(!ui.snapshot().targetPicker.open && sameActor(*ui.snapshot().selection.target, rear),
          "First Escape dismisses picker and preserves selection");
    ui.toggleTargetPicker();
    UiInput outside;
    outside.pointer = ArtPoint{300, 80};
    outside.pressed[unsigned(UiKey::Click)] = true;
    routeHudInput(ui, outside, front);
    check(!ui.snapshot().targetPicker.open && sameActor(*ui.snapshot().selection.target, rear) &&
              game.combat().snapshot().pendingCount == 0,
          "Outside dismissal cannot click through to a world attack");
    ui.toggleTargetPicker();
    ui.selectAction(PartyAction::Melee);
    check(!ui.snapshot().targetPicker.open, "Action change closes picker");
    ui.selectAction(PartyAction::Ranged);
    ui.toggleTargetPicker();
    ui.refresh(snapshot, false, false, false);
    check(!ui.snapshot().targetPicker.open && !ui.snapshot().selection.target,
          "Blur closes picker and selection");
    ui.refresh(snapshot, false, false, true);
    key(ui, UiKey::Tab);
    ui.refresh(snapshot, false, false, true);
    check(sameActor(*ui.snapshot().selection.target, front), "Refresh preserves target identity");
    ui.selectTarget(rear);
    ui.toggleTargetPicker();
    UiInputEdges pointer;
    std::array<bool, uiKeyCount> down{};
    const auto row = hudLayout::enemyTargetRow(0, ui.snapshot().targetPicker.count);
    const ArtPoint at{float(row.x + 3), float(row.y + 3)};
    down[unsigned(UiKey::Click)] = true;
    routeHudInput(ui, pointer.sample(down, true, at));
    check(sameActor(*ui.snapshot().selection.target, rear), "Picker press waits for release");
    game.actors().damage(front, 1, 0);
    auto changed = snapshot;
    changed.revision = game.actors().snapshot().revision;
    ui.refresh(changed, false, false, true);
    down.fill(false);
    routeHudInput(ui, pointer.sample(down, true, at));
    check(sameActor(*ui.snapshot().selection.target, rear),
          "Changed actor context cancels captured row click");
    down[unsigned(UiKey::Click)] = true;
    routeHudInput(ui, pointer.sample(down, true, at));
    down.fill(false);
    routeHudInput(ui, pointer.sample(down, true, at));
    check(sameActor(*ui.snapshot().selection.target, front), "Fresh row release selects once without attack");
    ui.closeTargetPicker();
    ui.selectTarget(rear);
    check(ui.confirm() == CommandReason::Resolved, "Covered target receives bow command");

    // Separate session avoids consuming the archer's cooldown/encounter state above.
    GameSession magic(map, catalog, places, 5, pose, false, fixture.timings);
    CommandController spells(magic);
    auto t = snapshot;
    t.world = magic.actors().snapshot().world;
    t.revision = magic.actors().snapshot().revision;
    for (std::size_t i = 0; i < t.count; ++i)
        t.ids[i] = magic.actors().id(t.ids[i].slot);
    for (std::size_t i = 0; i < t.candidateCount; ++i)
        t.candidates[i] = magic.actors().id(t.candidates[i].slot);
    spells.refresh(t, false, false, true);
    choose(spells, rules::SpellKind::FireBolt);
    key(spells, UiKey::Tab);
    key(spells, UiKey::Tab);
    check(spells.snapshot().magic.pending.target &&
              spells.snapshot().magic.pending.target->slot == rear.slot &&
              spells.snapshot().magic.pending.targetCount == 0,
          "Single spell Tab focuses covered target without casting");
    spells.toggleTargetPicker();
    spells.activateTargetRow(1);
    check(spells.snapshot().magic.pending.targetCount == 1,
          "Spell row prepares recipient without submission");
    check(spells.confirm() == CommandReason::Resolved, "Covered target receives single-target spell");

    GameSession multi(map, catalog, places, 5, pose, false, fixture.timings);
    CommandController missiles(multi);
    t.world = multi.actors().snapshot().world;
    t.revision = multi.actors().snapshot().revision;
    for (std::size_t i = 0; i < t.count; ++i)
        t.ids[i] = multi.actors().id(t.ids[i].slot);
    for (std::size_t i = 0; i < t.candidateCount; ++i)
        t.candidates[i] = multi.actors().id(t.candidates[i].slot);
    missiles.refresh(t, false, false, true);
    choose(missiles, rules::SpellKind::MagicMissile);
    key(missiles, UiKey::Tab);
    key(missiles, UiKey::Tab);
    check(missiles.snapshot().magic.assigned == 0 && missiles.snapshot().magic.pending.target->slot == 1,
          "Multi-target Tab only moves focus");
    missiles.toggleTargetPicker();
    for (int i = 0; i < 4; ++i)
        missiles.activateTargetRow(1);
    check(missiles.snapshot().magic.assigned == 3, "Picker respects three-dart cap");
    missiles.activateTargetRow(1, true);
    check(missiles.snapshot().magic.assigned == 2, "Picker removes a dart");
    key(missiles, UiKey::Up);
    check(missiles.snapshot().magic.assigned == 3, "Up adds to focused covered target");
    key(missiles, UiKey::Down);
    check(missiles.snapshot().magic.assigned == 2, "Down removes from focused target");
    watching = true;
    allocations = 0;
    for (unsigned i = 0; i < 8; ++i) {
        missiles.refresh(t, false, false, true);
        key(missiles, UiKey::Tab, i % 2);
        missiles.toggleTargetPicker();
        missiles.pageTargets(1);
    }
    watching = false;
    check(allocations == 0,
          "Candidate refresh, eligibility, cycling and picker use no engine C++ allocations");
    missiles.cancel();
    choose(missiles, rules::SpellKind::FireBolt);
    key(missiles, UiKey::Tab);
    missiles.toggleTargetPicker();
    multi.actors().damage(multi.actors().id(0), 10000, 0);
    t.revision = multi.actors().snapshot().revision;
    t.count = 0;
    t.candidateCount = 1;
    t.candidates[0] = multi.actors().id(1);
    missiles.refresh(t, false, false, true);
    check(!missiles.snapshot().magic.pending.target, "Dead spell focus is cleared without retargeting");
    missiles.closeTargetPicker();
    missiles.toggleTargetPicker();
    missiles.openInventory();
    check(!missiles.snapshot().targetPicker.open, "Opening another overlay closes target picker");
    missiles.closeOverlay();

    const Player contact{{3, 4}, Heading::North};
    const auto contactPlaces = combatPlacements(map, contact, 4);
    GameSession melee(map, catalog, contactPlaces, 5, contact, false);
    spellReady(melee, 0);
    CommandController blades(melee);
    blades.refresh(allCombatTargets(melee), false, false, true);
    auto attack = turnCommand(melee, PartyAction::Melee);
    attack.target = melee.actors().id(0);
    check(blades.validate(attack) == CommandReason::FrontProtected, "Rear formation remains melee protected");
    key(blades, UiKey::Tab);
    check(blades.snapshot().selection.target && blades.snapshot().selection.target->slot >= 2,
          "Melee Tab skips protected rear formation");
    blades.toggleTargetPicker();
    bool blockedRow = false;
    for (const auto& row : blades.snapshot().targetPicker.rows)
        blockedRow |= row.reason == CommandReason::FrontProtected;
    check(blockedRow, "Picker lists protected enemies with rejection reason");

    missiles.cancel();
    t.candidateCount = t.candidates.size() + 1;
    missiles.refresh(t, false, false, true);
    check(missiles.snapshot().targets.candidateCount == 0, "Malformed candidate count fails closed");
}
} // namespace
int main() {
    try {
        playerNames();
        controls();
        targeting();
        std::cout << "Enemy targeting checks passed\n";
    } catch (const std::exception& e) {
        watching = false;
        std::cerr << e.what() << '\n';
        return 1;
    }
}
