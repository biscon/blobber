#include "rules/content.hpp"
#include <stdexcept>
#include <utility>

namespace blobber::rules {
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
Catalog::Catalog(std::vector<AttackDefinition> attacks, std::vector<MonsterDefinition> monsters)
    : attacks_(std::move(attacks)), monsters_(std::move(monsters)) {
    if (attacks_.size() > maxAttacks || monsters_.size() > maxMonsters)
        throw std::runtime_error("catalog: exceeds 64 attacks or 32 monsters");
    validateIds(attacks_, "attacks");
    validateIds(monsters_, "monsters");
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
} // namespace blobber::rules
