#include "app/inventory_captures.hpp"
#include "ui/interaction.hpp"
#include "ui/inventory_layout.hpp"
#include <stdexcept>
namespace blobber {
namespace {
UiInput click(UiRect r) {
    UiInput in;
    in.pointer = ArtPoint{float(r.x + r.width / 2), float(r.y + r.height / 2)};
    in.pressed[static_cast<std::size_t>(UiKey::Click)] = true;
    return in;
}
void require(bool value) {
    if (!value)
        throw std::runtime_error("Inventory capture scenario failed");
}
} // namespace
void captureInventory(const Map& map, const rules::Catalog& catalog, const NpcWorld& placements,
                      const std::vector<NpcTimings>& timings, const SceneRenderer& renderer,
                      Presenter& presenter, const std::filesystem::path& directory) {
    // Diagnostic preparation can allocate. Keep the real catalog, rules and input router.
    // Empty placements make the safe-management captures independent of demo enemy distances.
    std::vector<std::string> types;
    for (std::size_t n = 0; n < placements.typeCount(); ++n)
        types.push_back(placements.typeId({static_cast<std::uint16_t>(n)}));
    const NpcWorld empty(map, Player{}.cell, std::move(types), {});
    GameSession session(map, catalog, empty, 5, Player{}, false, timings);
    CommandController ui(session, timings);
    NpcAnimator animator(session.actors(), timings);
    FramePipeline frames;
    auto refresh = [&](bool focused = true) {
        animator.setTime(session.timeMs(), &session.effects());
        frames.update(renderer, map, session.movement(), &session.actors(), &animator.snapshot(),
                      &session.combat().attacks(), session.timeMs());
        ui.refresh(frames.picking() ? frames.picking()->targets() : TargetSnapshot{}, false, false, focused);
    };
    auto input = [&](UiInput in) {
        routeHudInput(ui, in, {});
        refresh(in.focused);
    };
    auto save = [&](const char* name, bool reduced = false) {
        refresh();
        presenter.present(frames.frame(), session.movement(), map, false, true, &ui.snapshot(),
                          frames.picking());
        presenter.capture(directory / (std::string(name) + ".png"));
        if (reduced) {
            presenter.capture(directory / (std::string(name) + "-1280.png"), 1280, 720);
            presenter.capture(directory / (std::string(name) + "-4x3.png"), 1024, 768);
        }
    };
    auto find = [&](rules::CharacterId c, const char* id) {
        const auto definition = catalog.findItem(id);
        for (std::uint16_t cell = 0; cell < 52; ++cell) {
            const auto* stack = session.party().inventory().stack(session.party().inventory().at({c, cell}));
            if (stack && stack->item.index == definition.index)
                return cell;
        }
        throw std::runtime_error("Missing inventory capture item");
    };
    ui.record(session.update(0, false));
    refresh();
    save("inventory-backpack-dock", true);
    input(click(hudLayout::detail(2)));
    require(ui.snapshot().inventory.open && ui.snapshot().inventory.page == CharacterPage::Character &&
            ui.snapshot().inventory.character.index == 2);
    save("character-portrait-entry", true);
    input(click(inventoryLayout::inventoryPage));
    save("inventory-from-character", true);
    input(click(inventoryLayout::close));
    UiInput open;
    open.pressed[static_cast<std::size_t>(UiKey::Inventory)] = true;
    input(open);
    constexpr const char* names[]{"inventory-fighter", "inventory-cleric", "inventory-rogue",
                                  "inventory-wizard"};
    constexpr const char* sheets[]{"character-fighter", "character-cleric", "character-rogue",
                                   "character-wizard"};
    for (std::uint16_t n = 0; n < 4; ++n) {
        input(click(inventoryLayout::tab(n)));
        input(click(inventoryLayout::equipment(n == 3 ? 1 : 0)));
        save(names[n], true);
        input(click(inventoryLayout::characterPage));
        save(sheets[n], true);
        input(click(inventoryLayout::inventoryPage));
        require(ui.snapshot().inventory.character.index == n);
    }
    input(click(inventoryLayout::tab(1)));
    input(click(inventoryLayout::cell(find({1}, "holy_symbol"))));
    save("inventory-holy-symbol", true);
    input(click(inventoryLayout::tab(3)));
    input(click(inventoryLayout::cell(find({3}, "spellbook"))));
    save("inventory-spellbook", true);
    input(click(inventoryLayout::cell(find({3}, "arcane_focus"))));
    save("inventory-focus", true);
    input(click(inventoryLayout::tab(2)));
    const auto arrows = find({2}, "arrow");
    auto press = click(inventoryLayout::cell(arrows));
    press.shift = true;
    press.pointerHeld = true;
    input(press);
    auto drag = click(inventoryLayout::cell(47));
    drag.pressed.fill(false);
    drag.pointerHeld = true;
    drag.shift = true;
    input(drag);
    save("inventory-drag", true);
    drag.pointerHeld = false;
    drag.pointerReleased = true;
    input(drag);
    require(ui.snapshot().inventory.mode == InventoryMode::Quantity && ui.snapshot().inventory.quantity == 1);
    save("inventory-split-default", true);
    input(click(inventoryLayout::plus));
    input(click(inventoryLayout::accept));
    require(ui.snapshot().inventory.cells[47].quantity == 2);
    save("inventory-split-result");
    input(click(inventoryLayout::cell(47)));
    input(click(inventoryLayout::action(ui.snapshot().inventory, InventoryAction::Transfer)));
    save("inventory-transfer-choice");
    input(click(inventoryLayout::tab(0)));
    require(session.party().inventory().count({0}, rules::Ammunition::Arrow, catalog) == 2);
    input(click(inventoryLayout::tab(0)));
    input(click(inventoryLayout::equipment(2)));
    input(click(inventoryLayout::action(ui.snapshot().inventory, InventoryAction::Equip)));
    require(!session.party().equipped({0}, EquipmentSlot::OffHand));
    save("inventory-shield-removed", true);
    input(click(inventoryLayout::characterPage));
    save("character-shield-removed", true);
    input(click(inventoryLayout::inventoryPage));
    input(click(inventoryLayout::action(ui.snapshot().inventory, InventoryAction::Equip)));
    require(session.party().equipped({0}, EquipmentSlot::OffHand));
    input(click(inventoryLayout::cell(find({0}, "healing_potion"))));
    require(!ui.snapshot().inventory.actions[static_cast<unsigned>(InventoryAction::Equip)].visible);
    save("inventory-potion-disabled", true);
    session.party().damage({1}, 8);
    refresh();
    input(click(inventoryLayout::action(ui.snapshot().inventory, InventoryAction::Use)));
    require(ui.snapshot().inventory.itemTargeting);
    save("inventory-potion-targeting", true);
    input(click(hudLayout::card(1)));
    require(!ui.snapshot().inventory.itemTargeting);
    save("inventory-healing-result", true);
    ui.openInventory(rules::CharacterId{3});
    input(click(inventoryLayout::cell(find({3}, "mana_potion"))));
    save("inventory-mana", true);
    session.party().spendMana({3}, 3);
    refresh();
    input(click(inventoryLayout::action(ui.snapshot().inventory, InventoryAction::Use)));
    input(click(hudLayout::card(3)));
    require(session.party().character({3})->mana == 3);
    save("inventory-mana-result");
    EffectRequest effect;
    effect.world = session.actors().snapshot().world;
    effect.caster = {3};
    effect.kind = EffectKind::MageArmor;
    effect.targets[0] = rules::CharacterId{3};
    effect.count = 1;
    require(bool(session.applyEffect(effect)));
    ui.inspect(3);
    save("character-active-effects", true);
    input(click(inventoryLayout::close));
    // Actual full bag: clone only the starting arrow quantity during preparation, then
    // use production splits to fill every cell. No invented snapshot or frame mutation.
    std::vector<rules::AttackDefinition> attacks;
    std::vector<rules::MonsterDefinition> monsters;
    std::vector<rules::RaceDefinition> races;
    std::vector<rules::ClassDefinition> classes;
    std::vector<rules::ItemDefinition> items;
    std::vector<rules::SpellDefinition> spells;
    for (std::uint16_t n = 0; n < catalog.attackCount(); ++n)
        attacks.push_back(*catalog.attack({n}));
    for (std::uint16_t n = 0; n < catalog.monsterCount(); ++n)
        monsters.push_back(*catalog.monster({n}));
    for (std::uint16_t n = 0; n < catalog.raceCount(); ++n)
        races.push_back(*catalog.race({n}));
    for (std::uint16_t n = 0; n < catalog.classCount(); ++n)
        classes.push_back(*catalog.characterClass({n}));
    for (std::uint16_t n = 0; n < catalog.itemCount(); ++n)
        items.push_back(*catalog.item({n}));
    for (std::uint16_t n = 0; n < catalog.spellCount(); ++n)
        spells.push_back(*catalog.spell({n}));
    auto& longBow = items[catalog.findItem("shortbow").index];
    longBow.name = "Shortbow with a long fixture label for inventory detail panel";
    longBow.description.clear();
    while (longBow.description.size() < 512)
        longBow.description += "This long test description checks readable pagination, preserved words, and "
                               "bounded detail layout. ";
    longBow.description.resize(512);
    auto preset = *catalog.preset();
    for (auto& owned : preset.characters[2].items)
        if (owned.item.index == catalog.findItem("arrow").index)
            owned.quantity = 100;
    const rules::Catalog fullRules(std::move(attacks), std::move(monsters), std::move(races),
                                   std::move(classes), std::move(items), preset, std::move(spells));
    GameSession full(map, fullRules, empty, 5, Player{}, false, timings);
    CommandController fullUi(full, timings);
    const auto source = full.party().inventory().ammunition({2}, rules::Ammunition::Arrow, fullRules);
    for (std::uint16_t cell = 0; cell < 48; ++cell)
        if (!full.party().inventory().at({{2}, cell})) {
            const InventoryRequest r{
                InventoryOp::Split, source, {{2}, cell}, 1, full.party().inventory().revision()};
            require(bool(full.inventoryChange(r)));
        }
    fullUi.refresh({}, false, false, true);
    fullUi.openInventory(rules::CharacterId{2});
    fullUi.inventorySelect(51);
    fullUi.inventoryAction(InventoryAction::Equip);
    fullUi.refresh({}, false, false, true);
    presenter.present(frames.frame(), full.movement(), map, false, true, &fullUi.snapshot(),
                      frames.picking());
    presenter.capture(directory / "inventory-full-bag.png");
    presenter.capture(directory / "inventory-full-bag-1280.png", 1280, 720);
    fullUi.inventoryDescription(1);
    fullUi.refresh({}, false, false, true);
    presenter.present(frames.frame(), full.movement(), map, false, true, &fullUi.snapshot(),
                      frames.picking());
    presenter.capture(directory / "inventory-description-page2.png");
    presenter.capture(directory / "inventory-description-page2-1280.png", 1280, 720);
    // Inspect danger without advancing or applying equipment changes.
    if (!placements.instances().empty()) {
        const auto idle = gameplayPlacements(map, placements);
        GameSession danger(map, catalog, idle, 5, combatCamera(map, idle), false, timings);
        CommandController dangerUi(danger, timings);
        dangerUi.record(danger.update(0, false));
        dangerUi.refresh({}, false, false, true);
        dangerUi.openInventory(rules::CharacterId{0});
        dangerUi.inventorySelect(50);
        dangerUi.inventoryAction(InventoryAction::Equip);
        dangerUi.refresh({}, false, false, true);
        presenter.present(frames.frame(), danger.movement(), map, false, true, &dangerUi.snapshot(), nullptr);
        presenter.capture(directory / "inventory-danger.png");
    }
}
} // namespace blobber
