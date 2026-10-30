#include "game/party.hpp"
#include "support/check.hpp"
#include "support/party_fixtures.hpp"
#include <iostream>

using namespace blobber;
using namespace blobber::rules;
namespace {
void initialization() {
    const auto f = partyFixture();
    const auto catalog = f.catalog();
    Party party(catalog);
    const int hp[]{12, 12, 10, 7}, ac[]{19, 16, 14, 13};
    const Abilities scores[]{
        {16, 14, 15, 9, 13, 11}, {13, 10, 16, 8, 16, 12}, {8, 17, 14, 12, 13, 11}, {8, 16, 13, 16, 12, 10}};
    const Abilities saves[]{
        {5, 2, 4, -1, 1, 0}, {1, 0, 3, -1, 5, 3}, {-1, 5, 2, 3, 1, 0}, {-1, 3, 1, 5, 3, 0}};
    for (std::uint16_t i = 0; i < partySize; ++i) {
        const auto& c = *party.character({i});
        check(c.stats.abilities == scores[i] && c.stats.saves == saves[i],
              "Racial abilities and class saves compose");
        check(c.hitPoints == hp[i] && c.stats.maxHitPoints == hp[i] && c.stats.armorClass == ac[i],
              "Known level-one HP and equipped AC");
        check(c.availableHitDice == 1 && c.experience == 0 && c.stats.proficiency == 2,
              "New characters start recovered at level one");
        check(party.occupant(static_cast<FormationSlot>(i)) == &c, "Initial formation preserves identity");
    }
    check(!party.character({}) && !party.character({4}) && !party.occupant(static_cast<FormationSlot>(255)),
          "Invalid party references fail safely");
    const auto* original = party.character({0});
    check(party.swapFormation(FormationSlot::FrontLeft, FormationSlot::BackRight), "Formation swap succeeds");
    check(party.occupant(FormationSlot::BackRight) == original && original->hitPoints == 12,
          "Swap moves position, never resources or identity");
    const auto before = party.snapshot().formation;
    check(!party.swapFormation(FormationSlot::FrontLeft, static_cast<FormationSlot>(255)),
          "Invalid swap rejected");
    check(party.swapFormation(FormationSlot::FrontLeft, FormationSlot::FrontLeft),
          "Same-position swap valid");
    for (std::size_t i = 0; i < partySize; ++i)
        check(before[i].index == party.snapshot().formation[i].index,
              "Rejected/no-op swaps preserve formation");
    auto rearranged = f;
    rearranged.preset.formation = {{{3}, {2}, {1}, {0}}};
    const auto otherCatalog = rearranged.catalog();
    const Party other(otherCatalog);
    check(other.occupant(FormationSlot::FrontLeft)->build.id == "fixture.character3",
          "Loaded formation is respected");
    const auto rulesOnly = f.catalog(false);
    rejects([&] { Party noParty(rulesOnly); }, "Rules-only catalog cannot start a party");
    check(!catalog.race({}) && !catalog.characterClass({4}) && !catalog.item({10}),
          "Typed definition handles validate bounds");
    check(catalog.race(catalog.findRace("fixture.elf"))->name == "Elf fixture" &&
              catalog.characterClass(catalog.findClass("fixture.cleric"))->hitDie == 8 &&
              catalog.item(catalog.findItem("fixture.bow"))->ranged,
          "Stable strings resolve each content category");
    check(!catalog.race(catalog.findRace("missing")) &&
              !catalog.characterClass(catalog.findClass("missing")) &&
              !catalog.item(catalog.findItem("missing")),
          "Unknown content cannot silently select entry zero");
}
void derivation() {
    auto f = partyFixture();
    const auto catalog = f.catalog();
    for (std::uint16_t race = 0; race < 4; ++race)
        for (std::uint16_t cls = 0; cls < 4; ++cls) {
            auto b = f.preset.characters[3];
            b.race = {race};
            b.characterClass = {cls};
            check(bool(deriveCharacter(catalog, b)), "All sixteen race/class combinations accept legal gear");
        }
    auto b = f.preset.characters[0];
    auto stats = deriveCharacter(catalog, b).value;
    check(stats.weapons[0].attackBonus == 5 && stats.weapons[0].damage.sides == 8 &&
              stats.weapons[0].damage.modifier == 3,
          "One-handed sword derives attack and damage from Strength");
    check(stats.carriedWeight == 6400 && stats.carryingCapacity == 24000,
          "Equipment counted once in total weight");
    b.equipment.offHand = {};
    stats = deriveCharacter(catalog, b).value;
    check(stats.armorClass == 17 && stats.weapons[0].damage.sides == 10 && stats.carriedWeight == 6400,
          "Removing shield changes AC and versatile grip without discarding carried weight");
    b.equipment.body = {};
    check(deriveCharacter(catalog, b).value.armorClass == 12, "Defense does not apply without body armor");
    b.equipment.offHand = {2};
    check(deriveCharacter(catalog, b).value.armorClass == 14, "Shield alone does not activate Defense");
    b.defense = false;
    b.equipment = {{1}, {0}, {}};
    b.items[1].item = {6};
    b.baseAbilities[1] = 19;
    check(deriveCharacter(catalog, b).value.armorClass == 16, "Medium armor caps positive Dexterity at two");
    b.baseAbilities[1] = 6;
    check(deriveCharacter(catalog, b).value.armorClass == 12, "Medium armor preserves negative Dexterity");
    b.items[1].item = {5};
    check(deriveCharacter(catalog, b).value.armorClass == 9, "Light armor preserves negative Dexterity");
    b.items[1].item = {7};
    check(deriveCharacter(catalog, b).value.armorClass == 16, "Heavy armor ignores Dexterity");
    b = f.preset.characters[2];
    stats = deriveCharacter(catalog, b).value;
    check(stats.weapons[0].ability == Ability::Dexterity && stats.weapons[0].attackBonus == 5 &&
              stats.weapons[0].damage.modifier == 3,
          "Finesse uses higher Dexterity");
    check(stats.weapons[2].available && stats.weapons[2].ranged && stats.weapons[2].attackBonus == 5 &&
              !stats.weapons[3].available,
          "Carried bow has preview; ammunition does not");
    b.baseAbilities[0] = 20;
    check(deriveCharacter(catalog, b).value.weapons[0].ability == Ability::Strength,
          "Finesse can use higher Strength");
    b.baseAbilities[0] = 17;
    check(deriveCharacter(catalog, b).value.weapons[0].ability == Ability::Strength,
          "Finesse ties use Strength");
    b = f.preset.characters[3];
    b.items[0].item = {0};
    stats = deriveCharacter(catalog, b).value;
    check(!stats.weapons[0].proficient && stats.weapons[0].attackBonus == -1,
          "Untrained weapons omit proficiency");
    b.items[0].item = {3};
    check(deriveCharacter(catalog, b).value.weapons[0].attackBonus == 5,
          "Racial weapon training supplements class training");
    b.race = {0};
    check(!deriveCharacter(catalog, b).value.weapons[0].proficient, "Race change recalculates training");
    b.baseAbilities[2] = 1;
    check(deriveCharacter(catalog, b).value.maxHitPoints == 2, "Low Constitution HP stays valid");
    auto dartFixture = partyFixture();
    dartFixture.items[4].weapon = WeaponType::Dart;
    dartFixture.items[4].ranged = true;
    const auto darts = dartFixture.catalog(false);
    b = dartFixture.preset.characters[3];
    b.baseAbilities[0] = 20;
    const auto dart = deriveCharacter(darts, b).value.weapons[0];
    check(dart.ranged && dart.ability == Ability::Strength && dart.damage.modifier == 5,
          "Finesse overrides the ordinary ranged ability rule");
}
void failuresAndBounds() {
    auto f = partyFixture();
    const auto catalog = f.catalog();
    for (int fault = 0; fault < 15; ++fault) {
        auto b = f.preset.characters[0];
        switch (fault) {
        case 0:
            b.race = {};
            break;
        case 1:
            b.characterClass = {};
            break;
        case 2:
            b.level = 4;
            break;
        case 3:
            b.baseAbilities[0] = INT32_MAX;
            break;
        case 4:
            b.baseAbilities[0] = 30;
            break;
        case 5:
            b.itemCount = 17;
            break;
        case 6:
            b.items[0].quantity = 0;
            break;
        case 7:
            b.items[0].item = {};
            break;
        case 8:
            b.equipment.body = {3};
            break;
        case 9:
            b.equipment.offHand = {0};
            break;
        case 10:
            b.items[0].quantity = 2;
            break;
        case 11:
            b.items[0].item = {3};
            break;
        case 12:
            b.equipment.body = {0};
            break;
        case 13:
            b.id = " ";
            break;
        case 14:
            b.name = "\n";
            break;
        }
        const auto result = deriveCharacter(catalog, b);
        check(!result && result.value.maxHitPoints == 0,
              "Invalid character does not publish partial derived stats");
    }
    auto b = f.preset.characters[3];
    b.defense = true;
    check(deriveCharacter(catalog, b).error == CharacterError::InvalidFeature,
          "Defense requires class support");
    b.defense = false;
    b.items[1] = {{5}, 1};
    b.itemCount = 2;
    b.equipment.body = {1};
    check(deriveCharacter(catalog, b).error == CharacterError::ArmorProficiency, "Untrained armor rejected");
    b.equipment.body = {};
    b.items[1].item = {8};
    b.equipment.offHand = {1};
    check(deriveCharacter(catalog, b).error == CharacterError::ArmorProficiency, "Untrained shield rejected");
    b = f.preset.characters[3];
    b.equipment = {};
    b.items[0] = {{9}, 2400};
    check(bool(deriveCharacter(catalog, b)), "Exactly 120 pounds fits Strength 8");
    ++b.items[0].quantity;
    check(deriveCharacter(catalog, b).error == CharacterError::Overweight,
          "One additional arrow beyond capacity rejected");
    b.itemCount = 16;
    for (auto& i : b.items)
        i = {{9}, 1};
    check(bool(deriveCharacter(catalog, b)), "All sixteen owned entries supported");
    f.items[9].weight = INT32_MAX;
    const auto large = f.catalog(false);
    for (auto& i : b.items)
        i.quantity = UINT16_MAX;
    check(deriveCharacter(large, b).error == CharacterError::Overweight,
          "Large quantity/weight totals cannot overflow into acceptance");
    f = partyFixture();
    f.preset.formation[1] = {0};
    rejects([&] { (void)f.catalog(); }, "Duplicate formation rejected at catalog construction");
    f = partyFixture();
    f.preset.characters[1].id = f.preset.characters[0].id;
    rejects([&] { (void)f.catalog(); }, "Duplicate character identity rejected");
    for (int fault = 0; fault < 13; ++fault) {
        auto bad = partyFixture();
        switch (fault) {
        case 0:
            bad.races[0].abilityIncreases[0] = 3;
            break;
        case 1:
            bad.races[0].hitPointsPerLevel = -1;
            break;
        case 2:
            bad.classes[0].hitDie = 7;
            break;
        case 3:
            bad.classes[0].armor = 128;
            break;
        case 4:
            bad.items[0].damage.modifier = 1;
            break;
        case 5:
            bad.items[0].weapon = static_cast<WeaponType>(255);
            break;
        case 6:
            bad.items[0].versatileSides = 7;
            break;
        case 7:
            bad.items[0].twoHanded = true;
            break;
        case 8:
            bad.items[5].armor = ArmorType::Shield;
            break;
        case 9:
            bad.items[5].baseArmorClass = 0;
            break;
        case 10:
            bad.items[9].weight = -1;
            break;
        case 11:
            bad.classes[0].weapons = UINT32_MAX;
            break;
        case 12:
            bad.items[0].damageType = static_cast<DamageType>(255);
            break;
        }
        rejects([&] { (void)bad.catalog(false); }, "In-memory definitions cannot bypass validation");
    }
}
} // namespace
int main() {
    try {
        initialization();
        derivation();
        failuresAndBounds();
        std::cout << "Party checks passed\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
