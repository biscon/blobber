#include "app/creator_flow.hpp"
#include "game/commands.hpp"
#include "support/check.hpp"
#include "support/creation_fixtures.hpp"
#include "ui/interaction.hpp"
#include <iostream>
#include <memory>
#include <unistd.h>
using namespace blobber;
using namespace blobber::rules;
namespace {
struct Temp {
    std::filesystem::path path;
    Temp() {
        char name[] = "/tmp/blobber-creator-test-XXXXXX";
        const auto* dir = mkdtemp(name);
        if (!dir)
            throw std::runtime_error("mkdtemp");
        path = dir;
    }
    ~Temp() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};
int fault = -1;
bool fail(LibraryStage stage) {
    return int(stage) == fault;
}
UiInput click(UiRect r) {
    UiInput i;
    i.pointer = ArtPoint{float(r.x + r.width / 2), float(r.y + r.height / 2)};
    i.pressed[std::size_t(UiKey::Click)] = true;
    return i;
}
void activate(CreatorModel& m, FlowControls& controls, const Catalog& c, FlowAction action, int value = 0) {
    layoutCreator(controls, m, c);
    for (unsigned i = 0; i < controls.count; ++i)
        if (controls.entries[i].action == action && controls.entries[i].value == value) {
            routeCreator(m, controls, c, click(controls.entries[i].rect), {}, 0);
            return;
        }
    throw std::runtime_error("Missing creator control");
}
void model(const Catalog& c) {
    CreatorModel m;
    FlowControls controls;
    m.begin(c);
    check(!m.dirty() && !m.validStep(c, CreatorStep::Identity), "fresh draft has no name or class");
    m.cancel();
    check(m.canceled && !m.discard, "clean cancel");
    m.begin(c);
    TextInput edit;
    std::snprintf(edit.text.data(), 65, "New hero");
    routeCreator(m, controls, c, {}, edit, .1f);
    check(m.dirty() && std::string_view(m.draft.name.data()) == "New hero", "name input enters model");
    edit = {};
    edit.home = true;
    routeCreator(m, controls, c, {}, edit, 0);
    edit = {};
    std::snprintf(edit.text.data(), 65, "A ");
    routeCreator(m, controls, c, {}, edit, 0);
    check(std::string_view(m.draft.name.data()) == "A New hero", "insert at name cursor");
    edit = {};
    edit.backspace = true;
    UiInput back;
    back.pressed[std::size_t(UiKey::Cancel)] = true;
    routeCreator(m, controls, c, back, edit, 0);
    check(!m.discard, "backspace editing never discards");
    activate(m, controls, c, FlowAction::Next);
    activate(m, controls, c, FlowAction::Class, 3);
    check(m.preview && m.draft.characterClass.index == 3, "first class recommendations valid");
    m.go(c, CreatorStep::Learned);
    const auto learned = m.draft.spells.learned;
    m.go(c, CreatorStep::Prepared);
    m.recommend(c);
    check(m.draft.spells.learned == learned, "preparation recommendation preserves spellbook");
    const auto one = m.draft.spells.prepared & (~m.draft.spells.prepared + 1);
    m.draft.spells.prepared = one;
    m.refresh(c);
    check(m.validStep(c, CreatorStep::Prepared), "one ordinary prepared spell legal");
    m.draft.spells.prepared = 0;
    m.refresh(c);
    check(!m.validStep(c, CreatorStep::Prepared), "zero prepared rejected");
    m.recommend(c);
    const auto old = m.draft.spells.prepared;
    m.go(c, CreatorStep::Abilities);
    m.draft.abilities[3] = 8;
    m.refresh(c);
    check(m.draft.spells.prepared == old && !m.validStep(c, CreatorStep::Prepared),
          "reduced allowance retains overfull picks");
    m.recommend(c);
    m.characterClass(c, {0});
    check(!m.draft.spells.cantrips && !m.draft.spells.learned && !m.draft.spells.prepared,
          "martial class prunes spell choices");
    check(!m.applicable(c, CreatorStep::Cantrips) && !m.applicable(c, CreatorStep::Prepared),
          "noncaster skips stages");
    m.race(c, {3});
    check(m.applicable(c, CreatorStep::Racial), "elf fighter racial-only stage");
    m.go(c, CreatorStep::Racial);
    m.recommend(c);
    check(spellCount(m.draft.spells.racial) == 1, "racial recommendation");
    const auto name = m.draft.name;
    m.race(c, {1});
    check(m.draft.name == name && !m.draft.spells.racial && m.draft.portrait.index == 1,
          "race invalidation preserves name");
    m.go(c, CreatorStep::Review);
    UiInput wheel;
    wheel.pointer = ArtPoint{520, 150};
    wheel.scroll = -1;
    routeCreator(m, controls, c, wheel, {}, 0);
    check(m.reviewOffset == 1, "review wheel uses summary, not spell detail scrolling");
    m.cancel();
    check(m.discard && !m.canceled, "dirty cancel confirms");
    activate(m, controls, c, FlowAction::Keep);
    check(!m.discard && !m.canceled, "keep draft");
    m.cancel();
    activate(m, controls, c, FlowAction::Discard);
    check(m.canceled, "discard draft");
    TextEdit text;
    std::array<char, 65> value{};
    text.focus(true, value);
    TextInput full;
    full.text.fill('x');
    full.text[64] = 0;
    text.edit(value, full);
    check(std::strlen(value.data()) == 64, "text limit");
    text.edit(value, full);
    check(std::strlen(value.data()) == 64, "text cannot overflow");
    text.tick(.1f, false);
    check(!text.focused, "blur releases focus");
    for (std::uint16_t race = 0; race < 4; ++race)
        for (std::uint16_t cls = 0; cls < 4; ++cls) {
            m.begin(c);
            std::snprintf(m.draft.name.data(), 65, "Combination %u %u", race, cls);
            m.race(c, {race});
            m.characterClass(c, {cls});
            check(bool(m.preview), "all creation combinations legal");
            for (unsigned p = 0; p < portraitCount; ++p)
                if (c.race({race})->id == portrait({std::uint8_t(p)})->race) {
                    m.draft.portrait = {std::uint8_t(p)};
                    m.refresh(c);
                    check(bool(m.preview), "all matching portraits legal");
                }
            m.go(c, CreatorStep::Identity);
            unsigned seen = 0;
            while (m.step != CreatorStep::Review && seen++ < 12) {
                layoutCreator(controls, m, c);
                check(!controls.overflow && controls.count <= 128, "bounded controls");
                m.next(c, 1);
            }
            check(m.step == CreatorStep::Review && bool(m.preview),
                  "conditional forward navigation completes");
            auto builds = *c.preset();
            builds.characters[0] = finishBuild(c, m.draft, "new.hero");
            Party party(c, builds);
            check(party.character({0})->build.name == m.draft.name.data(),
                  "created build launches independently");
            for (std::uint16_t i = 0; i < c.spellCount(); ++i) {
                const auto bit = spellBit({i});
                const auto& grants = m.draft.spells;
                const bool eligible =
                    ((grants.cantrips | grants.prepared | grants.racial | grants.domain) & bit) != 0;
                check(bool(party.spellAccess({0}, {i})) == eligible, "live spell access matches selections");
            }
        }
}
void selection(const Catalog& c) {
    auto d = recommendedDraft(c, {3}, {3});
    SpellSelection s;
    s.context = choiceContext(c, draftIdentity(d), d.portrait, d.spells, SpellChoice::Cantrips);
    s.selected = s.context.baseline;
    s.filter(c);
    const auto picked = s.selected;
    const auto removed = SpellId{std::uint16_t(__builtin_ctz(picked))};
    std::uint32_t extra = s.context.available & ~s.selected;
    check(extra != 0, "choice alternative fixture");
    check(!s.toggle({std::uint16_t(__builtin_ctz(extra))}), "full budget rejects addition");
    check(s.toggle(removed) && s.toggle({std::uint16_t(__builtin_ctz(extra))}) && s.valid(),
          "full budget allows deselection then replacement");
    std::snprintf(s.search.data(), 65, "No such spell");
    s.filter(c);
    check(s.count == 0, "no search matches");
    s.search = {};
    s.filter(c);
    check(s.count > 0, "reset search");
    s = {};
    s.context.identity = draftIdentity(d);
    s.context.portrait = d.portrait;
    s.context.baseline = d.spells.learned;
    s.context.locked = d.spells.learned;
    s.context.available = classSpells(c, d.characterClass, 1) & ~s.context.locked;
    s.context.minimum = s.context.maximum = 2;
    s.filter(c);
    check(!s.toggle({std::uint16_t(__builtin_ctz(s.context.locked))}), "advancement existing spells locked");
    auto available = s.context.available;
    while (available) {
        const SpellId id{std::uint16_t(__builtin_ctz(available))};
        s.toggle(id);
        available &= available - 1;
    }
    check(s.valid() && spellCount(s.selected) == 2 && !(s.selected & s.context.locked),
          "advancement grants only two new choices");
    check(s.context.baseline == d.spells.learned, "immutable advancement baseline");
}
void persistence(const Catalog& c) {
    Temp temp;
    CharacterLibrary library(c, temp.path / "characters.json", {fail, nullptr});
    check(bool(library.initialize()), "initialize library");
    MenuModel menu;
    menu.screen = MenuScreen::Picker;
    menu.defaults();
    menu.activeSlot = 2;
    const auto formation = menu.choices;
    CreatorFlow flow;
    flow.begin(c, menu, library.snapshot());
    flow.model.draft = library.snapshot().records[0].draft;
    std::snprintf(menu.search.data(), 65, "hidden");
    menu.filterClass = true;
    menu.characterClass = {3};
    const auto before = library.snapshot().count;
    fault = int(LibraryStage::Write);
    auto result = flow.save(library, menu);
    check(!result.committed && flow.active && library.snapshot().count == before,
          "failed save retains draft");
    fault = -1;
    result = flow.save(library, menu);
    check(result.committed && !flow.active && menu.screen == MenuScreen::Picker, "save returns caller");
    check(menu.choices == formation && menu.activeSlot == 2 && menu.selected == result.id,
          "save highlights without assigning");
    check(!menu.search[0] && !menu.filterClass && library.snapshot().count == before + 1,
          "saved card made visible");
    flow.save(library, menu);
    check(library.snapshot().count == before + 1, "duplicate activation ignored");
    CharacterLibrary reopened(c, temp.path / "characters.json");
    check(bool(reopened.load()) && reopened.snapshot().find(result.id), "restart persistence");
    flow.begin(c, menu, library.snapshot());
    flow.model.draft = library.snapshot().records[0].draft;
    check(reopened.create(flow.model.draft, reopened.snapshot().revision).committed, "concurrent writer");
    result = flow.save(library, menu);
    check(result.error == LibraryError::Stale && flow.active, "stale save waits for renewed activation");
    result = flow.save(library, menu);
    check(result.committed, "explicit save after reload");
    menu.screen = MenuScreen::Library;
    flow.begin(c, menu, library.snapshot());
    flow.model.draft = library.snapshot().records[0].draft;
    fault = int(LibraryStage::DirectoryFlush);
    result = flow.save(library, menu);
    fault = -1;
    check(result.committed && !flow.active && result.error == LibraryError::DurabilityUncertain,
          "committed warning leaves creator");
    const auto count = library.snapshot().count;
    flow.save(library, menu);
    check(count == library.snapshot().count, "durability warning never duplicates");
    check(bool(library.load()), "reload after durability warning");
    // Capacity is a bounded fixture; no game content or user library is touched.
    while (library.snapshot().count < 128)
        check(library.create(library.snapshot().records[0].draft, library.snapshot().revision).committed,
              "fill fixture library");
    flow.begin(c, menu, library.snapshot());
    flow.model.draft = library.snapshot().records[0].draft;
    result = flow.save(library, menu);
    check(result.error == LibraryError::Full && flow.active, "full library keeps draft");
}
RestRequest request(GameSession& s) {
    RestRequest r;
    r.kind = RestKind::Long;
    r.world = s.actors().snapshot().world;
    r.token = s.restToken();
    return r;
}
unsigned kits(const Party& p) {
    unsigned n = 0;
    for (std::uint16_t c = 0; c < 4; ++c)
        for (std::uint16_t i = 0; i < bagCells; ++i)
            if (const auto* stack = p.inventory().stack(p.inventory().at({{c}, i}));
                stack && p.catalog().item(stack->item)->kind == ItemKind::Supply)
                n += stack->quantity;
    return n;
}
void preparation(const Catalog& c) {
    const auto map = hybridRoom();
    const NpcWorld empty(map, spellSpawn.cell, {}, {});
    auto owner = std::make_unique<GameSession>(map, c, empty, 71, spellSpawn, false);
    auto& s = *owner;
    auto r = request(s);
    check(s.previewRest(r).error == RestError::NoBenefit, "ordinary no-op rejected");
    check(s.party().alternativePreparation(r), "legal preparation-only benefit");
    const auto initial = s.party().preparationRequest();
    const auto time = s.combat().snapshot().gameplayMs;
    const auto n = kits(s.party());
    auto bad = r;
    bad.preparation.prepared[3] = UINT32_MAX;
    check(s.rest(bad).error == RestError::InvalidPreparation && kits(s.party()) == n &&
              s.combat().snapshot().gameplayMs == time,
          "invalid member makes whole rest atomic");
    bad = r;
    bad.preparation.inventoryRevision++;
    check(s.rest(bad).error == RestError::Stale, "stale inventory rejects preparation");
    bad = r;
    bad.preparation.revision++;
    check(s.rest(bad).error == RestError::Stale, "stale spell revision rejects preparation");
    bad = r;
    bad.kind = RestKind::Short;
    check(s.rest(bad).error == RestError::InvalidPreparation, "short rest rejects forged preparation");
    // One atomic transaction changes both living casters, leaving grants untouched.
    const auto wizardGrants = s.party().character({3})->build.spells;
    const auto wizardOptions = preparationOptions(c, s.party().character({3})->build.view(), wizardGrants);
    r.preparation.prepared[3] = wizardOptions & (~wizardOptions + 1);
    auto out = s.rest(r);
    check(out && out.preparationChanged && out.usedSupply, "prep-only rest succeeds at ordinary cost");
    check(kits(s.party()) == n - 1 && s.combat().snapshot().gameplayMs == time + 28800000,
          "exactly one kit and eight hours");
    check(s.rest(r).error == RestError::Stale && kits(s.party()) == n - 1, "single-use rest token");
    for (unsigned i = 0; i < 4; ++i)
        check(s.party().character({std::uint16_t(i)})->build.spells.prepared == r.preparation.prepared[i],
              "prepared lists published");
    check(s.party().character({3})->build.spells.learned == wizardGrants.learned &&
              s.party().character({3})->build.spells.cantrips == wizardGrants.cantrips &&
              s.party().character({3})->build.spells.racial == wizardGrants.racial,
          "rest changes preparation only");
    r = request(s);
    r.preparation = s.party().preparationRequest();
    check(s.rest(r).error == RestError::NoBenefit, "unchanged lists alone no benefit");
    CommandController ui(s);
    ui.refresh({}, false, false, true);
    check(ui.snapshot().restAvailable, "full-health rest icon allows preparation");
    ui.openCamp();
    ui.campKind(RestKind::Long);
    ui.openPreparation();
    check(ui.snapshot().camp.preparing, "open shared panel");
    const auto before = s.party().preparationRequest();
    UiInput escape;
    escape.pressed[std::size_t(UiKey::Cancel)] = true;
    ui.routePreparation(escape);
    check(!ui.snapshot().camp.preparing && s.party().preparationRequest().prepared == before.prepared,
          "cancel preparation no mutation");
    ui.openPreparation();
    auto hud = ui.snapshot().camp;
    FlowControls controls;
    layoutPreparation(controls, hud);
    for (unsigned i = 0; i < controls.count; ++i)
        if (controls.entries[i].action == FlowAction::Toggle && controls.entries[i].enabled &&
            !(hud.selection.selected & spellBit({std::uint16_t(controls.entries[i].value)}))) {
            ui.routePreparation(click(controls.entries[i].rect));
            break;
        }
    hud = ui.snapshot().camp;
    layoutPreparation(controls, hud);
    for (unsigned i = 0; i < controls.count; ++i)
        if (controls.entries[i].action == FlowAction::Done)
            ui.routePreparation(click(controls.entries[i].rect));
    check(!ui.snapshot().camp.preparing && s.party().preparationRequest().prepared == before.prepared,
          "Done stages without commit");
    ui.campKind(RestKind::Short);
    check(!ui.snapshot().camp.request.preparation.enabled, "short rest isolates staged changes");
    ui.campKind(RestKind::Long);
    check(ui.snapshot().camp.request.preparation.enabled, "long rest restores staging");
    ui.startRest();
    check(ui.snapshot().camp.playing, "start commits before playback");
    const auto committed = s.combat().snapshot().gameplayMs;
    ui.startRest();
    ui.tickCamp(.1, false);
    check(s.combat().snapshot().gameplayMs == committed && ui.snapshot().camp.elapsedMs == 0,
          "repeat/focus no duplicate");
    ui.closeOverlay();
    check(c.preset()->characters[1].spells.prepared == initial.prepared[1], "blueprint never changed");
    // A dying caster blocks the transaction; once stable, the same staged choices
    // can recover them alongside the rest of the party.
    r = request(s);
    check(s.party().alternativePreparation(r), "alternative for stable recovery");
    s.party().damage({3}, s.party().character({3})->hitPoints);
    const auto beforeDying = s.party().preparationRequest().prepared;
    check(!s.rest(r) && s.party().preparationRequest().prepared == beforeDying,
          "dying caster rejects whole rest without preparation mutation");
    s.party().stabilize({3});
    check(bool(s.rest(r)) && s.party().character({3})->hitPoints > 0,
          "stable caster recovers with legal preparation");
    // Dead recipients cannot receive a new preparation, and supply exhaustion never publishes masks.
    s.party().damage({3}, 10000);
    r = request(s);
    r.preparation = s.party().preparationRequest();
    r.preparation.prepared[3] = 1;
    check(s.rest(r).error == RestError::InvalidPreparation, "dead caster cannot prepare");
    for (std::uint16_t member = 0; member < 4; ++member)
        for (std::uint16_t i = 0; i < bagCells; ++i) {
            auto handle = s.party().inventory().at({{member}, i});
            const auto* stack = s.party().inventory().stack(handle);
            if (stack && c.item(stack->item)->kind == ItemKind::Supply)
                s.party().consumeItem(handle, stack->quantity);
        }
    r = request(s);
    check(s.party().alternativePreparation(r), "remaining living caster alternative");
    check(s.rest(r).error == RestError::NoSupplies, "preparation still needs supplies");
}
} // namespace
int main() {
    try {
        const auto c = creationCatalog();
        model(c);
        selection(c);
        persistence(c);
        preparation(c);
        std::cout << "Creator and rest preparation checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
