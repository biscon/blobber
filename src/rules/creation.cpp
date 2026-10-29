#include "rules/creation.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace blobber::rules {
namespace {
constexpr PortraitDefinition portraits[]{
    {"alden", "human", "alden.png"},
    {"brunna", "hill_dwarf", "brunna.png"},
    {"merrin", "lightfoot_halfling", "merrin.png"},
    {"lethiel", "high_elf", "lethiel.png"},
    {"human.mage", "human", "portraits/human-mage.png"},
    {"human.cleric", "human", "portraits/human-cleric.png"},
    {"human.rogue", "human", "portraits/human-rogue.png"},
    {"human.adventurer", "human", "portraits/human-adventurer.png"},
    {"dwarf.fighter", "hill_dwarf", "portraits/dwarf-fighter.png"},
    {"dwarf.mage", "hill_dwarf", "portraits/dwarf-mage.png"},
    {"dwarf.rogue", "hill_dwarf", "portraits/dwarf-rogue.png"},
    {"dwarf.adventurer", "hill_dwarf", "portraits/dwarf-adventurer.png"},
    {"halfling.fighter", "lightfoot_halfling", "portraits/halfling-fighter.png"},
    {"halfling.mage", "lightfoot_halfling", "portraits/halfling-mage.png"},
    {"halfling.cleric", "lightfoot_halfling", "portraits/halfling-cleric.png"},
    {"halfling.adventurer", "lightfoot_halfling", "portraits/halfling-adventurer.png"},
    {"elf.fighter", "high_elf", "portraits/elf-fighter.png"},
    {"elf.cleric", "high_elf", "portraits/elf-cleric.png"},
    {"elf.rogue", "high_elf", "portraits/elf-rogue.png"},
    {"elf.adventurer", "high_elf", "portraits/elf-adventurer.png"}};
static_assert(std::size(portraits) == portraitCount);
std::uint32_t kindBit(const Catalog& c, SpellKind k) noexcept {
    for (std::uint16_t i = 0; i < c.spellCount(); ++i)
        if (c.spell({i})->kind == k)
            return spellBit({i});
    return 0;
}
std::uint32_t wizardCantrips(const Catalog& c) noexcept {
    std::uint32_t mask = 0;
    for (std::uint16_t i = 0; i < c.spellCount(); ++i) {
        const auto& r = *spellRules(c.spell({i})->kind);
        if (!r.level && r.casting == Casting::Intelligence)
            mask |= spellBit({i});
    }
    return mask;
}
std::uint32_t choose(const Catalog& c, std::uint32_t available, unsigned count,
                     std::initializer_list<SpellKind> priority) noexcept {
    std::uint32_t out = 0;
    for (auto k : priority)
        if (spellCount(out) < count)
            out |= kindBit(c, k) & available;
    for (std::uint16_t i = 0; i < c.spellCount() && spellCount(out) < count; ++i)
        out |= spellBit({i}) & available;
    return out;
}
CharacterIdentityView identity(const CharacterDraft& d) noexcept {
    const auto end = std::find(d.name.begin(), d.name.end(), '\0');
    return {"draft",     {d.name.data(), std::size_t(end - d.name.begin())},
            d.race,      d.characterClass,
            d.abilities, 1,
            d.defense,   d.lifeDomain};
}
bool sameGear(const CharacterLoadout& a, const CharacterBuild& b) noexcept {
    if (a.itemCount != b.itemCount)
        return false;
    for (std::size_t i = 0; i < a.itemCount; ++i)
        if (a.items[i].item.index != b.items[i].item.index || a.items[i].quantity != b.items[i].quantity)
            return false;
    return a.equipment.body.index == b.equipment.body.index &&
           a.equipment.mainHand.index == b.equipment.mainHand.index &&
           a.equipment.offHand.index == b.equipment.offHand.index &&
           a.equipment.ranged.index == b.equipment.ranged.index;
}
} // namespace
const PortraitDefinition* portrait(PortraitId id) noexcept {
    return id.index < portraitCount ? &portraits[id.index] : nullptr;
}
PortraitId findPortrait(std::string_view id) noexcept {
    for (std::uint8_t i = 0; i < portraitCount; ++i)
        if (id == portraits[i].id)
            return {i};
    return {};
}
PortraitId defaultPortrait(std::string_view race) noexcept {
    for (std::uint8_t i = 0; i < portraitCount; ++i)
        if (race == portraits[i].race)
            return {i};
    return {};
}
const char* buildErrorMessage(BuildError e) noexcept {
    constexpr const char* messages[]{"Valid build",
                                     "Enter 1-64 printable characters",
                                     "Choose a supported race",
                                     "Choose a supported class",
                                     "Choose a portrait for this race",
                                     "Base scores must be 8-15",
                                     "Spend exactly 27 points",
                                     "Use the supported class features",
                                     "Choose three distinct class cantrips",
                                     "Choose six wizard spellbook spells",
                                     "Choose legal prepared spells within your allowance",
                                     "Life domain grants Bless and Cure Wounds",
                                     "Choose one distinct Intelligence-based wizard cantrip for High Elf",
                                     "Choose valid starting equipment",
                                     "Equipment or derived statistics are invalid",
                                     "Required creation content is missing"};
    return unsigned(e) < std::size(messages) ? messages[unsigned(e)] : "Invalid build";
}
int pointCost(int score) noexcept {
    constexpr int costs[]{0, 1, 2, 3, 4, 5, 7, 9};
    return score >= 8 && score <= 15 ? costs[score - 8] : -1;
}
int pointBuyCost(const Abilities& a) noexcept {
    int sum = 0;
    for (int n : a) {
        int p = pointCost(n);
        if (p < 0)
            return -1;
        sum += p;
    }
    return sum;
}
unsigned spellCount(std::uint32_t bits) noexcept {
    unsigned n = 0;
    while (bits) {
        bits &= bits - 1;
        ++n;
    }
    return n;
}
std::uint32_t spellBit(SpellId id) noexcept {
    return id.index < 32 ? std::uint32_t(1) << id.index : 0;
}
std::uint32_t classSpells(const Catalog& c, ClassId id, int level) noexcept {
    const auto* cls = c.characterClass(id);
    std::uint32_t out = 0;
    if (cls)
        for (std::size_t i = 0; i < cls->spellCount; ++i) {
            const auto* s = c.spell(cls->spells[i]);
            if (s && spellRules(s->kind)->level == level)
                out |= spellBit(cls->spells[i]);
        }
    return out;
}
int preparationLimit(const Catalog& c, const CharacterIdentityView& b) noexcept {
    const auto* cls = c.characterClass(b.characterClass);
    const auto* race = c.race(b.race);
    if (!cls || !race || cls->casting == Casting::None || b.level < 1 || b.level > 20)
        return 0;
    const auto i = cls->casting == Casting::Wisdom ? 4 : 3;
    if (!abilityModifier(b.baseAbilities[i]))
        return 0;
    const auto mod = abilityModifier(b.baseAbilities[i] + race->abilityIncreases[i]);
    return mod ? std::max(1, b.level + mod.value) : 0;
}
bool recommendSpells(const Catalog& c, const CharacterIdentityView& b, SpellGrants& s) noexcept {
    s = {};
    const auto* cls = c.characterClass(b.characterClass);
    const auto* race = c.race(b.race);
    if (!cls || !race)
        return false;
    const auto can = classSpells(c, b.characterClass, 0), leveled = classSpells(c, b.characterClass, 1);
    if (cls->casting == Casting::Wisdom) {
        s.cantrips =
            choose(c, can, 3, {SpellKind::SacredFlame, SpellKind::SpareTheDying, SpellKind::Guidance});
        if (b.lifeDomain)
            s.domain = kindBit(c, SpellKind::Bless) | kindBit(c, SpellKind::CureWounds);
        s.prepared = choose(c, leveled & ~s.domain, unsigned(preparationLimit(c, b)),
                            {SpellKind::Bane, SpellKind::ShieldOfFaith, SpellKind::GuidingBolt,
                             SpellKind::InflictWounds, SpellKind::Sanctuary});
    } else if (cls->casting == Casting::Intelligence) {
        s.cantrips = choose(c, can, 3, {SpellKind::FireBolt, SpellKind::AcidSplash, SpellKind::RayOfFrost});
        s.learned = choose(c, leveled, 6,
                           {SpellKind::MageArmor, SpellKind::MagicMissile, SpellKind::BurningHands,
                            SpellKind::Thunderwave, SpellKind::Sleep, SpellKind::Grease});
        s.prepared = choose(
            c, s.learned, unsigned(preparationLimit(c, b)),
            {SpellKind::MageArmor, SpellKind::MagicMissile, SpellKind::BurningHands, SpellKind::Thunderwave});
    }
    if (race->wizardCantrip)
        s.racial = choose(c, wizardCantrips(c) & ~s.cantrips, 1,
                          {SpellKind::FireBolt, SpellKind::RayOfFrost, SpellKind::ShockingGrasp,
                           SpellKind::AcidSplash, SpellKind::PoisonSpray});
    return validateGrants(c, b, s) == BuildError::None;
}
BuildError validateGrants(const Catalog& c, const CharacterIdentityView& b, const SpellGrants& s,
                          bool counts) noexcept {
    const auto* cls = c.characterClass(b.characterClass);
    const auto* race = c.race(b.race);
    if (!cls || !race)
        return BuildError::Content;
    const bool caster = cls->casting != Casting::None, wizard = cls->casting == Casting::Intelligence;
    if ((s.cantrips & ~classSpells(c, b.characterClass, 0)) || (!caster && s.cantrips) ||
        (counts && caster && spellCount(s.cantrips) != 3))
        return BuildError::Cantrips;
    if ((s.learned & ~classSpells(c, b.characterClass, 1)) || (!wizard && s.learned) ||
        (counts && wizard && spellCount(s.learned) != 6))
        return BuildError::Learned;
    const auto domain = b.lifeDomain ? kindBit(c, SpellKind::Bless) | kindBit(c, SpellKind::CureWounds) : 0;
    if (s.domain != domain || (counts && b.lifeDomain && spellCount(domain) != 2))
        return BuildError::Domain;
    const auto available = wizard ? s.learned : classSpells(c, b.characterClass, 1);
    if ((s.prepared & ~available) || (s.prepared & s.domain) || (!caster && s.prepared) ||
        (counts && caster && (!s.prepared || spellCount(s.prepared) > unsigned(preparationLimit(c, b)))))
        return BuildError::Prepared;
    if ((s.racial & ~wizardCantrips(c)) || (s.racial & s.cantrips) || (!race->wizardCantrip && s.racial) ||
        (race->wizardCantrip && spellCount(s.racial) != 1))
        return BuildError::Racial;
    return BuildError::None;
}
SpellAccess spellAccess(const Catalog& cat, const CharacterIdentity& b, const CharacterStats& stats,
                        SpellId id) noexcept {
    const auto* spell = cat.spell(id);
    const auto* cls = cat.characterClass(b.characterClass);
    const auto* race = cat.race(b.race);
    if (!spell || !cls || !race)
        return {};
    const auto bit = spellBit(id);
    const auto& r = *spellRules(spell->kind);
    SpellAccess out;
    if ((b.spells.racial & bit) && race->wizardCantrip && !r.level && r.casting == Casting::Intelligence) {
        out.source = SpellSource::Racial;
        out.casting = Casting::Intelligence;
    } else if ((b.spells.domain & bit) && b.lifeDomain &&
               hasFeature(cls->features, ClassFeature::LifeDomain) &&
               (spell->kind == SpellKind::Bless || spell->kind == SpellKind::CureWounds)) {
        out.source = SpellSource::Domain;
        out.casting = cls->casting;
    } else if ((classSpells(cat, b.characterClass, r.level) & bit) &&
               ((!r.level && (b.spells.cantrips & bit)) ||
                (r.level && (b.spells.prepared & bit) &&
                 (cls->casting != Casting::Intelligence || (b.spells.learned & bit))))) {
        out.source = SpellSource::Class;
        out.casting = cls->casting;
    }
    if (!out)
        return {};
    out.modifier = stats.modifiers[out.casting == Casting::Wisdom ? 4 : 3];
    out.attack = stats.proficiency + out.modifier;
    out.difficulty = 8 + out.attack;
    out.cost = r.level * 2;
    return out;
}
const CreationClass* creationClass(const Catalog& c, ClassId id) noexcept {
    return c.creation(id);
}
BuildPreview previewBuild(const Catalog& c, const CharacterDraft& d) noexcept {
    BuildPreview out;
    out.loadout.itemCount = 0;
    auto error = [&](BuildError e) {
        if (e != BuildError::None)
            out.errors |= 1u << unsigned(e);
    };
    const auto b = identity(d);
    const auto* race = c.race(d.race);
    const auto* cls = c.characterClass(d.characterClass);
    if (!validDisplayText(b.name, 64))
        error(BuildError::Name);
    if (!race)
        error(BuildError::Race);
    if (!cls)
        error(BuildError::Class);
    const auto* p = portrait(d.portrait);
    if (!p || !race || race->id != p->race)
        error(BuildError::Portrait);
    out.points = pointBuyCost(d.abilities);
    if (out.points < 0)
        error(BuildError::Abilities);
    else if (out.points != 27)
        error(BuildError::Budget);
    if (cls && (d.defense != cls->allowsDefense ||
                d.lifeDomain != hasFeature(cls->features, ClassFeature::LifeDomain)))
        error(BuildError::Features);
    error(validateGrants(c, b, d.spells));
    out.preparation = preparationLimit(c, b);
    const auto* options = c.creation(d.characterClass);
    auto add = [&](const StartingGrant& g) {
        if (out.loadout.itemCount >= maxStartingItems || !c.item(g.item) || !g.quantity ||
            g.quantity > stackLimit(*c.item(g.item))) {
            error(BuildError::Gear);
            return;
        }
        const OwnedItemId index{std::uint16_t(out.loadout.itemCount)};
        out.loadout.items[out.loadout.itemCount++] = {g.item, g.quantity};
        if (g.slot < 4) {
            OwnedItemId* slots[]{&out.loadout.equipment.body, &out.loadout.equipment.mainHand,
                                 &out.loadout.equipment.offHand, &out.loadout.equipment.ranged};
            if (slots[g.slot]->index != UINT16_MAX)
                error(BuildError::Gear);
            *slots[g.slot] = index;
        } else if (g.slot != 255)
            error(BuildError::Gear);
    };
    if (!options)
        error(BuildError::Content);
    else {
        for (std::size_t g = 0; g < 3; ++g) {
            if (g >= options->groupCount) {
                if (d.gear[g])
                    error(BuildError::Gear);
                continue;
            }
            const auto& group = options->groups[g];
            if (d.gear[g] >= group.count) {
                error(BuildError::Gear);
                continue;
            }
            const auto& opt = group.options[d.gear[g]];
            for (std::size_t i = 0; i < opt.count; ++i)
                add(opt.items[i]);
        }
        for (std::size_t i = 0; i < options->supplyCount; ++i)
            add(options->supplies[i]);
    }
    // Partial drafts can preview real statistics without needing a final name.
    auto previewIdentity = b;
    previewIdentity.name = "Draft";
    const auto derived = deriveCharacter(c, previewIdentity, out.loadout);
    out.derivation = derived.error;
    if (derived)
        out.stats = derived.value;
    else
        error(BuildError::Derived);
    return out;
}
CharacterDraft recommendedDraft(const Catalog& c, RaceId race, ClassId cls) noexcept {
    CharacterDraft d;
    d.race = race;
    d.characterClass = cls;
    if (const auto* r = c.race(race))
        d.portrait = defaultPortrait(r->id);
    if (const auto* k = c.characterClass(cls)) {
        d.defense = k->allowsDefense;
        d.lifeDomain = hasFeature(k->features, ClassFeature::LifeDomain);
    }
    if (const auto* o = c.creation(cls))
        d.abilities = o->recommended;
    recommendSpells(c, identity(d), d.spells);
    return d;
}
CharacterBuild finishBuild(const Catalog& c, const CharacterDraft& d, std::string id) {
    const auto preview = previewBuild(c, d);
    if (!preview || !validContentId(id))
        throw std::invalid_argument("Incomplete or invalid character draft/identity");
    CharacterBuild b;
    b.id = std::move(id);
    b.name = d.name.data();
    b.race = d.race;
    b.characterClass = d.characterClass;
    b.baseAbilities = d.abilities;
    b.defense = d.defense;
    b.lifeDomain = d.lifeDomain;
    b.portrait = d.portrait;
    b.spells = d.spells;
    b.itemCount = preview.loadout.itemCount;
    b.equipment = preview.loadout.equipment;
    std::copy_n(preview.loadout.items.begin(), b.itemCount, b.items.begin());
    if (!deriveCharacter(c, b))
        throw std::invalid_argument("Invalid character identity");
    return b;
}
bool draftFromBuild(const Catalog& c, const CharacterBuild& b, CharacterDraft& out) noexcept {
    if (b.level != 1 || !validDisplayText(b.name, 64))
        return false;
    CharacterDraft d;
    std::copy(b.name.begin(), b.name.end(), d.name.begin());
    d.race = b.race;
    d.characterClass = b.characterClass;
    d.portrait = b.portrait;
    d.abilities = b.baseAbilities;
    d.defense = b.defense;
    d.lifeDomain = b.lifeDomain;
    d.spells = b.spells;
    // At most 2^3 complete loadouts. This is a load/save boundary, never a per-frame search.
    for (unsigned bits = 0; bits < 8; ++bits) {
        for (unsigned g = 0; g < 3; ++g)
            d.gear[g] = (bits >> g) & 1;
        const auto p = previewBuild(c, d);
        if (p && sameGear(p.loadout, b)) {
            out = d;
            return true;
        }
    }
    return false;
}
} // namespace blobber::rules
