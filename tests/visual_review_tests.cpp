#include "game/commands.hpp"
#include "support/check.hpp"
#include "support/npc_fixtures.hpp"
#include "support/render_fixtures.hpp"
#include "support/spell_fixtures.hpp"
#include "ui/interaction.hpp"
#include "ui/spell_layout.hpp"
#include <cstring>
#include <iostream>
using namespace blobber;
namespace {
void key(CommandController& ui, UiKey k) {
    UiInput input;
    input.pressed[unsigned(k)] = true;
    routeHudInput(ui, input);
}
void tabsAndHover() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    const auto places = spellPlacements(map, 0);
    GameSession s(map, catalog, places, 5, spellSpawn, false);
    CommandController ui(s);
    const auto refresh = [&] { ui.refresh({}, false, false, true); };
    refresh();
    ui.selectCharacter(3);
    ui.openSpellbook();
    const auto& m = ui.snapshot().magic;
    check(m.tab == 0 && m.visibleCount == 5 && m.tabs[1] && m.tabs[10] && !m.tabs[2],
          "Wizard displays populated level tabs and all five cantrips");
    key(ui, UiKey::Down);
    const auto cantrip = m.cursor;
    key(ui, UiKey::Right);
    check(m.tab == 1 && m.visibleCount == 8, "Right switches to eight level-one spells");
    ui.browseSpellbook(99);
    const auto spell = m.cursor;
    check(m.offset == 1 && m.entries[spell].kind == rules::SpellKind::ColorSpray,
          "Scroll stays inside level and reaches last spell");
    key(ui, UiKey::Right);
    check(m.tab == 10 && m.visibleCount == 1 && m.entries[m.cursor].feature == rules::Feature::ArcaneRecovery,
          "Abilities excludes spells");
    key(ui, UiKey::Left);
    check(m.cursor == spell && m.offset == 1, "Per-tab cursor and scroll restored");
    key(ui, UiKey::Left);
    check(m.cursor == cantrip && m.offset == 0, "Cantrip position remembered independently");
    ui.selectSpellbookTab(1);
    key(ui, UiKey::Confirm);
    check(m.pending.spell.index == m.entries[spell].spell.index && m.kind == rules::SpellKind::ColorSpray,
          "Filtered selection retains authoritative spell identity");
    ui.backToSpellbook();
    check(m.tab == 1 && m.cursor == spell && m.offset == 1, "Back returns to same tab and row");
    ui.closeOverlay();
    ui.selectCharacter(1);
    ui.openSpellbook();
    check(m.tab == 0 && m.visibleCount == 4 && !m.tabs[10], "Cleric initially has no empty ability tab");
    ui.closeOverlay();
    ui.selectCharacter(3);
    ui.openSpellbook();
    check(m.tab == 1 && m.cursor == spell, "Changing character preserves its book position");
    ui.closeOverlay();
    const auto apply = [&](EffectKind kind, unsigned caster) {
        EffectRequest r;
        r.world = s.actors().snapshot().world;
        r.caster = {std::uint16_t(caster)};
        r.kind = kind;
        r.targets[0] = rules::CharacterId{3};
        r.count = 1;
        r.difficulty = 1000;
        check(bool(s.applyEffect(r)), "Status fixture applies");
    };
    apply(EffectKind::Bless, 1);
    apply(EffectKind::Bane, 2);
    apply(EffectKind::ShieldOfFaith, 0);
    apply(EffectKind::MageArmor, 3);
    apply(EffectKind::Resistance, 3);
    apply(EffectKind::Sanctuary, 3);
    check(s.party().grantTemporary({3}, 5, 3600000), "Temporary HP fixture");
    refresh();
    check(ui.snapshot().cards[3].effects.count == 7,
          "All friendly and hostile effects, including temporary HP");
    ui.openSpellbook();
    const auto r = hudLayout::status(3, 0);
    UiInput hover;
    hover.pointer = ArtPoint{float(r.x + 1), float(r.y + 1)};
    routeHudInput(ui, hover);
    check(ui.snapshot().hover.kind == HoverKind::Status &&
              std::strstr(ui.snapshot().hover.lines[0].data(), "Bless") && m.open,
          "Status hover works over visible portrait while picker remains open");
    const auto duration = ui.snapshot().hover.lines[1];
    s.update(30, true);
    refresh();
    routeHudInput(ui, hover);
    check(duration == ui.snapshot().hover.lines[1], "Paused browsing never advances displayed duration");
    const auto overflow = hudLayout::status(3, 5);
    hover.pointer = ArtPoint{float(overflow.x + 1), float(overflow.y + 1)};
    routeHudInput(ui, hover);
    check(std::strstr(ui.snapshot().hover.lines[0].data(), "More") &&
              std::strstr(ui.snapshot().hover.lines[3].data(), "False Life"),
          "Overflow exposes names and durations");
    ui.selectSpellbookTab(10);
    s.endConcentration(s.actors().snapshot().world, {3});
    refresh();
    check(m.visibleCount == 1 && m.entries[m.cursor].feature == rules::Feature::ArcaneRecovery,
          "Disappearing End Concentration reconciles ability selection");
    const auto mana = s.party().character({3})->mana;
    hover.pressed[unsigned(UiKey::Click)] = true;
    routeHudInput(ui, hover);
    check(!m.open && s.party().character({3})->mana == mana && !m.armed,
          "Closing click on portrait never leaks into targeting or casting");
}
void summaryAndPose() {
    const auto map = hybridRoom();
    const auto catalog = spellCatalog();
    auto places = spellPlacements(map, 1);
    Party party(catalog);
    ActorWorld actors(map, catalog, places);
    rules::DiceRng rng(7);
    EffectController effects(party, actors, rng);
    CombatEvents events;
    const auto id = actors.id(0);
    auto appearance = testProneAppearance();
    const std::vector<NpcTimings> timings{npcTimings(appearance)};
    NpcAnimator animator(actors, timings);
    const SceneRenderer renderer(testGreaseMaterials(), {appearance});
    effects.condition(id, EffectKind::Sleep, {3}, 0, events);
    auto statuses = effects.statuses(id);
    check(statuses.count == 1 && statuses.entries[0].spell == rules::SpellKind::Sleep &&
              statuses.entries[0].remainingMs == 60000,
          "Sleep icon and lifetime come from active condition");
    animator.setTime(10, &effects);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Prone &&
              animator.snapshot().frames[0].active == 0,
          "Sleep selects single prone frame");
    const auto revision = animator.snapshot().revision;
    animator.setTime(2000, &effects);
    check(animator.snapshot().revision == revision, "Static prone pose does not animate or invalidate frame");
    const auto cell = actors.actor(id)->npc.cell;
    for (unsigned heading = 0; heading < 4; ++heading) {
        const Player player{cell + forward(Heading(heading)) * -2, Heading(heading)};
        Frame frame;
        PickingBuffer pick;
        renderer.compose(map, player, frame, &actors, &animator.snapshot(), &pick);
        check(pick.targets().count == 1 && actors.occupied(cell),
              "All prone directions keep visible picking and occupancy");
        const auto low = pick.targets().anchors[id.slot].badgeTop;
        auto standing = animator.snapshot();
        standing.frames[0].clip = NpcAnimation::Idle;
        renderer.compose(map, player, frame, &actors, &standing, &pick);
        check(pick.targets().anchors[id.slot].badgeTop < low,
              "Prone body silhouette is lower than standing body");
    }
    effects.condition(id, EffectKind::Knockdown, {3}, 2000, events);
    effects.wake(id, 2000, events);
    animator.setTime(2000, &effects);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Prone,
          "Waking does not clear independent knockdown");
    check(effects.stand(id, 2000, events), "Living awake creature stands");
    animator.setTime(2000, &effects);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Idle, "Standing restores normal animation");
    effects.clockMode(true);
    effects.condition(id, EffectKind::Blind, {3}, 2000, events);
    effects.condition(id, EffectKind::Slow, {3}, 2000, events);
    effects.condition(id, EffectKind::Knockdown, {3}, 2000, events);
    statuses = effects.statuses(id);
    check(statuses.count == 3 && statuses.entries[0].clock == StatusClock::CasterTurn &&
              statuses.entries[2].clock == StatusClock::Standing,
          "Turn-bound statuses report their actual expiry rule");
    effects.casterTurn({3}, 2000, events);
    check(effects.statuses(id).count == 1, "Caster turn expires blind and slow summaries");
    actors.damage(id, 9999, 2010);
    animator.setTime(2010, &effects);
    check(animator.snapshot().frames[0].clip == NpcAnimation::Death, "Death takes precedence over prone");
    EffectRequest armor;
    armor.world = actors.snapshot().world;
    armor.kind = EffectKind::MageArmor;
    armor.caster = {3};
    armor.targets[0] = rules::CharacterId{3};
    armor.count = 1;
    check(bool(effects.apply(armor, 0, events)), "First armor instance");
    effects.clockMode(false);
    effects.advance(1000, 1000, true, events);
    check(bool(effects.apply(armor, 1000, events)), "Second armor instance");
    const auto grouped = effects.statuses(rules::CharacterId{3});
    check(grouped.count == 1 && grouped.entries[0].remainingMs == 28800000,
          "Duplicate status has one icon and longest remaining lifetime");
    effects.advance(28801000, 28801000, true, events);
    check(!effects.statuses(rules::CharacterId{3}).count, "Expired status disappears");
}
void badgeLayout() {
    HudSnapshot hud;
    hud.targets.world = 1;
    hud.targets.count = 2;
    for (unsigned i = 0; i < 2; ++i) {
        hud.targets.ids[i] = {std::uint16_t(i), 1};
        auto& a = hud.targets.anchors[i];
        a.valid = true;
        a.badgeX = 250;
        a.badgeTop = 110;
        hud.conditions[i] = (1u << unsigned(EffectKind::Sleep)) | (1u << unsigned(EffectKind::Knockdown));
        hud.magic.affected[i] = hud.targets.ids[i];
    }
    hud.magic.affectedCount = 2;
    const auto status = statusMarkerLayout(hud);
    const auto casts = spellMarkerLayout(hud);
    check(status[0] && status[1] && !badgesOverlap(*status[0], *status[1]),
          "Overlapping actors get separate icon rows");
    for (const auto& cast : casts)
        if (cast)
            for (const auto& row : status)
                if (row)
                    check(!badgesOverlap(*cast, *row), "Casting badges cannot cover status badges");
}
} // namespace
int main() {
    try {
        tabsAndHover();
        summaryAndPose();
        badgeLayout();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    std::cout << "Spell tabs, status summaries/tooltips and prone presentation passed\n";
}
