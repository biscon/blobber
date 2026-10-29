#include "rules/content.hpp"
#include "rules/creation.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace blobber::rules {
Catalog::~Catalog() = default;
Catalog::Catalog(const Catalog&) = default;
Catalog::Catalog(Catalog&&) noexcept = default;
const CreationClass* Catalog::creation(ClassId id) const noexcept {
    for (const auto& c : creation_)
        if (c.characterClass.index == id.index)
            return &c;
    return nullptr;
}
bool validContentId(std::string_view id) noexcept {
    if (id.empty() || id.size() > Catalog::maxIdBytes)
        return false;
    for (const unsigned char c : id)
        if (c < 33 || c > 126)
            return false;
    return true;
}
namespace {
template <class T> void validateIds(const std::vector<T>& definitions, const char* category) {
    for (std::size_t i = 0; i < definitions.size(); ++i) {
        if (!validContentId(definitions[i].id))
            throw std::runtime_error(std::string(category) +
                                     ".id: expected 1..128 printable nonspace ASCII bytes");
        for (std::size_t j = 0; j < i; ++j)
            if (definitions[i].id == definitions[j].id)
                throw std::runtime_error(std::string(category) + ".id: duplicate " + definitions[i].id);
    }
}
} // namespace
Catalog::Catalog(std::vector<AttackDefinition> attacks, std::vector<MonsterDefinition> monsters,
                 std::vector<RaceDefinition> races, std::vector<ClassDefinition> classes,
                 std::vector<ItemDefinition> items, std::optional<PresetParty> preset,
                 std::vector<SpellDefinition> spells, std::vector<CreationClass> creation)
    : creation_(std::move(creation)), spells_(std::move(spells)), attacks_(std::move(attacks)),
      monsters_(std::move(monsters)), races_(std::move(races)), classes_(std::move(classes)),
      items_(std::move(items)), preset_(std::move(preset)) {
    if (attacks_.size() > maxAttacks || monsters_.size() > maxMonsters)
        throw std::runtime_error("catalog: exceeds 64 attacks or 32 monsters");
    if (spells_.size() > maxSpells)
        throw std::runtime_error("spells: exceeds 32 definitions");
    validateIds(spells_, "spells");
    for (std::size_t i = 0; i < spells_.size(); ++i) {
        const auto& spell = spells_[i];
        if (!spellRules(spell.kind) || !validDisplayText(spell.name, 64) ||
            !validDisplayText(spell.description, 512))
            throw std::runtime_error("spells: invalid kind/name/description");
        for (std::size_t j = 0; j < i; ++j)
            if (spells_[j].kind == spell.kind)
                throw std::runtime_error("spells: duplicate concrete kind");
    }
    validateIds(attacks_, "attacks");
    validateIds(monsters_, "monsters");
    if (races_.size() > maxRaces || classes_.size() > maxClasses || items_.size() > maxItems)
        throw std::runtime_error("catalog: exceeds 16 races, 16 classes, or 64 items");
    validateIds(races_, "races");
    validateIds(classes_, "classes");
    validateIds(items_, "items");
    for (const auto& r : races_) {
        if (!validDisplayText(r.name, 64) || r.hitPointsPerLevel < 0 || r.hitPointsPerLevel > 1 ||
            (r.weapons & ~allWeaponTraining))
            throw std::runtime_error("races." + r.id + ": invalid name, HP bonus, or weapon training");
        for (const int increase : r.abilityIncreases)
            if (increase < 0 || increase > 2)
                throw std::runtime_error("races." + r.id + ".ability_increases: expected 0..2");
    }
    for (const auto& c : classes_) {
        if (static_cast<unsigned>(c.casting) > static_cast<unsigned>(Casting::Intelligence) ||
            (c.features & ~15u) || c.spellCount > c.spells.size() ||
            (hasFeature(c.features, ClassFeature::LifeDomain) && c.casting != Casting::Wisdom) ||
            (hasFeature(c.features, ClassFeature::ArcaneRecovery) && c.casting != Casting::Intelligence))
            throw std::runtime_error("classes: invalid casting/features");
        for (std::size_t i = 0; i < c.spellCount; ++i) {
            const auto* s = spell(c.spells[i]);
            if (!s || spellRules(s->kind)->casting != c.casting)
                throw std::runtime_error("classes: invalid spell reference or class eligibility");
            for (std::size_t j = 0; j < i; ++j)
                if (c.spells[j].index == c.spells[i].index)
                    throw std::runtime_error("classes: duplicate spell reference");
        }

        if (!validDisplayText(c.name, 64) ||
            (c.hitDie != 6 && c.hitDie != 8 && c.hitDie != 10 && c.hitDie != 12) ||
            (c.weapons & ~allWeaponTraining) || (c.armor & ~15u) ||
            std::count(c.saves.begin(), c.saves.end(), true) != 2)
            throw std::runtime_error("classes." + c.id + ": invalid name, hit die, or training");
    }
    for (const auto& i : items_) {
        if (!validDisplayText(i.name, 64) || !validDisplayText(i.description, 512) || i.weight < 0 ||
            static_cast<unsigned>(i.kind) > static_cast<unsigned>(ItemKind::Supply))
            throw std::runtime_error("items." + i.id + ": invalid name, description, weight, or kind");
        const unsigned expected = i.kind == ItemKind::Currency     ? 1000
                                  : i.kind == ItemKind::Supply     ? 5
                                  : i.kind == ItemKind::Ammunition ? 100
                                  : i.kind == ItemKind::Consumable ? 20
                                                                   : 1;
        if (stackLimit(i) != expected || static_cast<unsigned>(i.ammunition) > 1 ||
            static_cast<unsigned>(i.consumable) > 2 ||
            (i.kind == ItemKind::Consumable) != (i.consumable != Consumable::None) ||
            (i.ammunition != Ammunition::None && i.kind != ItemKind::Ammunition &&
             (i.kind != ItemKind::Weapon || !i.ranged)))
            throw std::runtime_error("items: invalid stack, ammunition or consumable definition");
        if (i.kind == ItemKind::Weapon &&
            (!weaponBit(i.weapon) || !validDice(i.damage) || i.damage.modifier != 0 ||
             static_cast<unsigned>(i.damageType) > static_cast<unsigned>(DamageType::Slashing) ||
             (i.versatileSides && (!validDice({1, i.versatileSides, 0}) ||
                                   i.versatileSides <= i.damage.sides || i.twoHanded || i.ranged))))
            throw std::runtime_error("items." + i.id + ": invalid weapon properties");
        if (i.kind == ItemKind::Armor &&
            (static_cast<unsigned>(i.armor) > static_cast<unsigned>(ArmorType::Heavy) ||
             i.baseArmorClass < 1 || i.baseArmorClass > 30))
            throw std::runtime_error("items." + i.id + ": invalid armor properties");
    }
    if (creation_.size() > maxClasses)
        throw std::runtime_error("Too many creation classes");
    for (std::size_t n = 0; n < creation_.size(); ++n) {
        const auto& c = creation_[n];
        if (!characterClass(c.characterClass) || c.groupCount > 3 || c.supplyCount > 6 ||
            pointBuyCost(c.recommended) != 27)
            throw std::runtime_error("Invalid creation class");
        for (std::size_t j = 0; j < n; ++j)
            if (creation_[j].characterClass.index == c.characterClass.index)
                throw std::runtime_error("Duplicate creation class");
        auto grant = [&](const StartingGrant& g) {
            const auto* d = item(g.item);
            if (!d || !g.quantity || g.quantity > stackLimit(*d) || (g.slot > 3 && g.slot != 255) ||
                (g.slot < 4 && g.quantity != 1) || d->kind == ItemKind::Supply)
                throw std::runtime_error("Invalid starting grant");
        };
        for (std::size_t g = 0; g < c.groupCount; ++g) {
            const auto& group = c.groups[g];
            if (!group.count || group.count > 2)
                throw std::runtime_error("Invalid gear group");
            for (std::size_t o = 0; o < group.count; ++o) {
                const auto& option = group.options[o];
                if (!validContentId(option.id) || !validDisplayText(option.name, 64) || !option.count ||
                    option.count > 2)
                    throw std::runtime_error("Invalid gear option");
                if (o && option.id == group.options[0].id)
                    throw std::runtime_error("Duplicate gear option");
                for (std::size_t i = 0; i < option.count; ++i)
                    grant(option.items[i]);
            }
        }
        for (std::size_t i = 0; i < c.supplyCount; ++i)
            grant(c.supplies[i]);
    }
    if (preset_) {
        if (!validFormation(preset_->formation))
            throw std::runtime_error("preset_party.formation: expected permutation of four characters");
        for (std::size_t i = 0; i < partySize; ++i) {
            const auto& b = preset_->characters[i];
            if (!creation_.empty()) {
                CharacterDraft draft;
                if (!draftFromBuild(*this, b, draft))
                    throw std::runtime_error("Preset is not a valid curated starting build");
            }
            const auto derived = deriveCharacter(*this, b);
            if (!derived)
                throw std::runtime_error("preset_party.characters[" + std::to_string(i) +
                                         "]: " + characterErrorMessage(derived.error));
            for (std::size_t j = 0; j < i; ++j)
                if (b.id == preset_->characters[j].id)
                    throw std::runtime_error("preset_party.characters: duplicate character ID");
        }
    }
    for (const auto& a : attacks_) {
        if (a.kind != AttackKind::Melee && a.kind != AttackKind::Ranged)
            throw std::runtime_error("attacks." + a.id + ".kind: invalid attack kind");
        if (!validDice(a.damage))
            throw std::runtime_error("attacks." + a.id + ".damage: invalid dice");
        if (a.damageType != DamageType::Bludgeoning && a.damageType != DamageType::Piercing &&
            a.damageType != DamageType::Slashing)
            throw std::runtime_error("attacks." + a.id + ".damage.type: invalid physical damage type");
    }
    for (const auto& m : monsters_) {
        if (m.lootCount > m.loot.size())
            throw std::runtime_error("Monster loot exceeds eight stacks");
        for (std::size_t j = 0; j < m.lootCount; ++j) {
            const auto& e = m.loot[j];
            const auto* d = item(e.item);
            const auto maximum = unsigned(e.quantity) * (e.sides ? e.sides : 1u);
            if (!d || !e.quantity || !e.first || e.first > e.last || e.last > 100 ||
                (e.sides && !validDice({e.quantity, e.sides, 0})) || maximum > stackLimit(*d))
                throw std::runtime_error("Invalid monster loot entry");
        }
        if (m.traits & ~static_cast<Traits>(Trait::NimbleEscape))
            throw std::runtime_error("Unsupported monster trait");
        for (const int score : m.abilities)
            if (!abilityModifier(score))
                throw std::runtime_error("monsters." + m.id + ".abilities: scores must be 1..30");
        if (m.armorClass < 1 || m.armorClass > 30 || m.hitPoints <= 0)
            throw std::runtime_error("monsters." + m.id +
                                     ": armor_class must be 1..30; hit_points must be positive");
        if (m.attackCount == 0 || m.attackCount > m.attacks.size())
            throw std::runtime_error("monsters." + m.id + ".attacks: expected 1..8 attacks");
        for (std::size_t i = 0; i < m.attackCount; ++i) {
            if (!attack(m.attacks[i]))
                throw std::runtime_error("monsters." + m.id + ".attacks: invalid reference");
            for (std::size_t j = 0; j < i; ++j)
                if (m.attacks[i].index == m.attacks[j].index)
                    throw std::runtime_error("monsters." + m.id + ".attacks: duplicate reference");
        }
    }
}
const SpellDefinition* Catalog::spell(SpellId id) const noexcept {
    return id.index < spells_.size() ? &spells_[id.index] : nullptr;
}
SpellId Catalog::findSpell(std::string_view id) const noexcept {
    for (std::size_t i = 0; i < spells_.size(); ++i)
        if (spells_[i].id == id)
            return {static_cast<std::uint16_t>(i)};
    return {};
}
const AttackDefinition* Catalog::attack(AttackId id) const noexcept {
    return id.index < attacks_.size() ? &attacks_[id.index] : nullptr;
}
const MonsterDefinition* Catalog::monster(MonsterId id) const noexcept {
    return id.index < monsters_.size() ? &monsters_[id.index] : nullptr;
}
AttackId Catalog::findAttack(std::string_view id) const noexcept {
    for (std::size_t i = 0; i < attacks_.size(); ++i)
        if (attacks_[i].id == id)
            return {static_cast<std::uint16_t>(i)};
    return {};
}
MonsterId Catalog::findMonster(std::string_view id) const noexcept {
    for (std::size_t i = 0; i < monsters_.size(); ++i)
        if (monsters_[i].id == id)
            return {static_cast<std::uint16_t>(i)};
    return {};
}
const RaceDefinition* Catalog::race(RaceId id) const noexcept {
    return id.index < races_.size() ? &races_[id.index] : nullptr;
}
const ClassDefinition* Catalog::characterClass(ClassId id) const noexcept {
    return id.index < classes_.size() ? &classes_[id.index] : nullptr;
}
const ItemDefinition* Catalog::item(ItemId id) const noexcept {
    return id.index < items_.size() ? &items_[id.index] : nullptr;
}
RaceId Catalog::findRace(std::string_view id) const noexcept {
    for (std::size_t i = 0; i < races_.size(); ++i)
        if (races_[i].id == id)
            return {static_cast<std::uint16_t>(i)};
    return {};
}
ClassId Catalog::findClass(std::string_view id) const noexcept {
    for (std::size_t i = 0; i < classes_.size(); ++i)
        if (classes_[i].id == id)
            return {static_cast<std::uint16_t>(i)};
    return {};
}
ItemId Catalog::findItem(std::string_view id) const noexcept {
    for (std::size_t i = 0; i < items_.size(); ++i)
        if (items_[i].id == id)
            return {static_cast<std::uint16_t>(i)};
    return {};
}
} // namespace blobber::rules
