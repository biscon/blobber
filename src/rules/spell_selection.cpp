#include "rules/spell_selection.hpp"
#include <algorithm>
#include <cstring>
namespace blobber::rules {
namespace {
bool match(std::string_view name, std::string_view query) noexcept {
    auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? char(c + 'a' - 'A') : c; };
    return std::search(name.begin(), name.end(), query.begin(), query.end(),
                       [&](char a, char b) { return lower(a) == lower(b); }) != name.end() ||
           query.empty();
}
} // namespace
void SpellSelection::filter(const Catalog& c) noexcept {
    count = 0;
    for (unsigned pass = 0; pass < (context.editableFirst ? 2u : 1u); ++pass)
        for (std::uint16_t i = 0; i < c.spellCount(); ++i)
            if (((context.available | (context.showLocked ? context.locked : 0)) & spellBit({i})) &&
                (!context.editableFirst || bool(context.locked & spellBit({i})) == bool(pass)) &&
                match(c.spell({i})->name, search.data()))
                visible[count++] = {i};
    offset = std::min(offset, count > 6 ? count - 6 : 0);
    if (!c.spell(inspected)) {
        if (count)
            inspected = visible[0];
    }
}
bool SpellSelection::valid() const noexcept {
    const auto n = spellCount(selected);
    return !(selected & ~context.available) && !(selected & context.locked) && n >= context.minimum &&
           n <= context.maximum;
}
bool SpellSelection::toggle(SpellId id) noexcept {
    inspected = id;
    detailOffset = 0;
    const auto bit = spellBit(id);
    if (!(context.available & bit) || (context.locked & bit))
        return false;
    if (selected & bit) {
        selected &= ~bit;
        return true;
    }
    if (context.maximum == 1) {
        selected = bit;
        return true;
    }
    if (spellCount(selected) >= context.maximum)
        return false;
    selected |= bit;
    return true;
}
void SpellSelection::inspectSelected() noexcept {
    inspected = {};
    detailOffset = 0;
    for (std::uint16_t i = 0; i < 32; ++i)
        if (selected & spellBit({i})) {
            inspected = {i};
            break;
        }
}
CharacterIdentityView draftIdentity(const CharacterDraft& d) noexcept {
    const auto n = std::find(d.name.begin(), d.name.end(), '\0') - d.name.begin();
    return {"draft",     {d.name.data(), std::size_t(n)}, d.race, d.characterClass, d.abilities, 1, d.defense,
            d.lifeDomain};
}
std::uint32_t racialCantrips(const Catalog& c) noexcept {
    std::uint32_t bits = 0;
    for (std::uint16_t i = 0; i < c.spellCount(); ++i) {
        const auto* r = spellRules(c.spell({i})->kind);
        if (!r->level && r->casting == Casting::Intelligence)
            bits |= spellBit({i});
    }
    return bits;
}
std::uint32_t preparationOptions(const Catalog& c, const CharacterIdentityView& b,
                                 const SpellGrants& g) noexcept {
    const auto* cls = c.characterClass(b.characterClass);
    if (!cls || cls->casting == Casting::None)
        return 0;
    const auto eligible = eligibleLeveledSpells(c, b);
    return (cls->casting == Casting::Intelligence ? eligible & g.learned : eligible) & ~g.domain;
}
bool validPreparation(const Catalog& c, const CharacterIdentityView& b, const SpellGrants& g,
                      std::uint32_t selected) noexcept {
    const auto limit = preparationLimit(c, b);
    return limit > 0 && selected && !(selected & ~preparationOptions(c, b, g)) &&
           spellCount(selected) <= unsigned(limit);
}
SpellSelectionContext choiceContext(const Catalog& c, const CharacterIdentityView& b, PortraitId portrait,
                                    const SpellGrants& g, SpellChoice kind) noexcept {
    SpellSelectionContext out;
    out.identity = b;
    out.portrait = portrait;
    switch (kind) {
    case SpellChoice::Cantrips:
        out.baseline = g.cantrips;
        out.available = classSpells(c, b.characterClass, 0) & ~g.racial;
        out.minimum = out.maximum = 3;
        break;
    case SpellChoice::Racial:
        out.baseline = g.racial;
        out.available = racialCantrips(c) & ~g.cantrips;
        out.minimum = out.maximum = 1;
        break;
    case SpellChoice::Learned:
        out.baseline = g.learned;
        out.available = classSpells(c, b.characterClass, 1);
        out.minimum = out.maximum = 6;
        break;
    case SpellChoice::Prepared:
        out.baseline = g.prepared;
        out.available = preparationOptions(c, b, g);
        out.locked = g.domain;
        out.minimum = 1;
        out.maximum = unsigned(std::max(0, preparationLimit(c, b)));
        break;
    }
    return out;
}
std::uint32_t recommendedChoices(const Catalog& c, const SpellSelectionContext& ctx,
                                 SpellChoice kind) noexcept {
    SpellGrants g;
    recommendSpells(c, ctx.identity, g);
    auto preferred = kind == SpellChoice::Cantrips  ? g.cantrips
                     : kind == SpellChoice::Racial  ? g.racial
                     : kind == SpellChoice::Learned ? g.learned
                                                    : g.prepared;
    std::uint32_t result = 0;
    for (unsigned pass = 0; pass < 2; ++pass)
        for (std::uint16_t i = 0; i < c.spellCount() && spellCount(result) < ctx.maximum; ++i) {
            const auto bit = spellBit({i});
            if ((ctx.available & ~ctx.locked & bit) && (pass || (preferred & bit)))
                result |= bit;
        }
    return result;
}
} // namespace blobber::rules
