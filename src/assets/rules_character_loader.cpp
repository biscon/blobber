#include "assets/rules_character_loader.hpp"
#include "assets/rules_json.hpp"
#include "rules/creation.hpp"

namespace blobber {
namespace {
using namespace rules;
using namespace rules_json;
constexpr std::array<const char*, abilityCount> abilities{"strength",     "dexterity", "constitution",
                                                          "intelligence", "wisdom",    "charisma"};
constexpr std::array<const char*, weaponTypeCount> weapons{
    "dagger",    "mace",    "longsword",    "shortsword",     "shortbow", "longbow",
    "battleaxe", "handaxe", "light_hammer", "warhammer",      "rapier",   "hand_crossbow",
    "dart",      "sling",   "quarterstaff", "light_crossbow", "scimitar"};
template <std::size_t N>
std::size_t choice(const Json& value, const std::string& path, const std::array<const char*, N>& names) {
    const auto name = string(value, path);
    for (std::size_t i = 0; i < N; ++i)
        if (name == names[i])
            return i;
    fail(path, "unknown value: " + name);
}
bool boolean(const Json& value, const std::string& path) {
    if (!value.is_boolean())
        fail(path, "expected boolean");
    return value.get<bool>();
}
Abilities scores(const Json& value, const std::string& path) {
    fields(value, path, {"strength", "dexterity", "constitution", "intelligence", "wisdom", "charisma"});
    Abilities out{};
    for (std::size_t i = 0; i < abilityCount; ++i)
        out[i] = integer(value.at(abilities[i]), path + "." + abilities[i]);
    return out;
}
WeaponTraining training(const Json& value, const std::string& path, int version) {
    array(value, path, weaponTypeCount);
    WeaponTraining out = 0;
    for (std::size_t i = 0; i < value.size(); ++i) {
        const auto p = path + "[" + std::to_string(i) + "]";
        const auto name = string(value[i], p);
        if (version < 6 && name == "scimitar")
            fail(p, "scimitar requires schema 6");
        const auto bits = name == "simple" ? simpleWeaponTraining
                          : name == "martial"
                              ? allWeaponTraining & ~simpleWeaponTraining
                              : weaponBit(static_cast<WeaponType>(choice(value[i], p, weapons)));
        if (out & bits)
            fail(p, "duplicate weapon training");
        out |= bits;
    }
    return out;
}
template <class T>
std::uint16_t reference(const Json& value, const std::string& path, const std::vector<T>& definitions) {
    const auto id = identifier(value, path);
    for (std::size_t i = 0; i < definitions.size(); ++i)
        if (definitions[i].id == id)
            return static_cast<std::uint16_t>(i);
    fail(path, "unknown reference: " + id);
}
OwnedItemId ownedReference(const Json& value, const std::string& path) {
    if (value.is_null())
        return {};
    const auto index = integer(value, path);
    if (index < 0 || index >= static_cast<int>(maxStartingItems))
        fail(path, "expected owned item index 0..15 or null");
    return {static_cast<std::uint16_t>(index)};
}
} // namespace
CharacterContent loadCharacterContent(const nlohmann::json& root) {
    CharacterContent out;
    const int version = integer(root.at("schema_version"), "schema_version");
    const bool spellsEnabled = version >= 4;
    if (spellsEnabled) {
        array(root.at("spells"), "spells", Catalog::maxSpells);
        for (const auto& j : root.at("spells")) {
            fields(j, "spells[]", {"id", "name", "description", "kind"});
            SpellDefinition spell;
            spell.id = identifier(j.at("id"), "spells[].id");
            spell.name = string(j.at("name"), "spells[].name");
            spell.description = string(j.at("description"), "spells[].description");
            const auto kind = string(j.at("kind"), "spells[].kind");
            for (unsigned k = 0; k < static_cast<unsigned>(SpellKind::Count); ++k)
                if (kind == spellRules(static_cast<SpellKind>(k))->id)
                    spell.kind = static_cast<SpellKind>(k);
            if (!spellRules(spell.kind))
                fail("spells[].kind", "unknown concrete spell");
            out.spells.push_back(std::move(spell));
        }
    }
    const auto& races = root.at("races");
    array(races, "races", Catalog::maxRaces);
    for (std::size_t i = 0; i < races.size(); ++i) {
        const auto p = "races[" + std::to_string(i) + "]";
        const auto& r = races[i];
        if (version >= 8)
            fields(r, p,
                   {"id", "name", "ability_increases", "hit_points_per_level", "weapons", "sleep_immune",
                    "wizard_cantrip"});
        else if (version >= 7)
            fields(r, p,
                   {"id", "name", "ability_increases", "hit_points_per_level", "weapons", "sleep_immune"});
        else
            fields(r, p, {"id", "name", "ability_increases", "hit_points_per_level", "weapons"});
        out.races.push_back({identifier(r.at("id"), p + ".id"), string(r.at("name"), p + ".name"),
                             scores(r.at("ability_increases"), p + ".ability_increases"),
                             integer(r.at("hit_points_per_level"), p + ".hit_points_per_level"),
                             training(r.at("weapons"), p + ".weapons", version)});
        if (version >= 7)
            out.races.back().sleepImmune = boolean(r.at("sleep_immune"), p + ".sleep_immune");
        out.races.back().wizardCantrip = version >= 8 ? boolean(r.at("wizard_cantrip"), p + ".wizard_cantrip")
                                                      : out.races.back().id == "high_elf";
    }
    const auto& classes = root.at("classes");
    array(classes, "classes", Catalog::maxClasses);
    for (std::size_t i = 0; i < classes.size(); ++i) {
        const auto p = "classes[" + std::to_string(i) + "]";
        const auto& c = classes[i];
        if (spellsEnabled)
            fields(c, p,
                   {"id", "name", "hit_die", "saves", "weapons", "armor", "allows_defense", "casting",
                    "features", "spells"});
        else
            fields(c, p, {"id", "name", "hit_die", "saves", "weapons", "armor", "allows_defense"});
        ClassDefinition cls;
        cls.id = identifier(c.at("id"), p + ".id");
        cls.name = string(c.at("name"), p + ".name");
        cls.hitDie = integer(c.at("hit_die"), p + ".hit_die");
        cls.weapons = training(c.at("weapons"), p + ".weapons", version);
        cls.allowsDefense = boolean(c.at("allows_defense"), p + ".allows_defense");
        if (spellsEnabled) {
            cls.casting =
                static_cast<Casting>(choice(c.at("casting"), p + ".casting",
                                            std::array<const char*, 3>{"none", "wisdom", "intelligence"}));
            array(c.at("features"), p + ".features", 4);
            for (const auto& f : c.at("features")) {
                const auto bit = 1u << choice(f, p + ".features",
                                              std::array<const char*, 4>{"second_wind", "life_domain",
                                                                         "sneak_attack", "arcane_recovery"});
                if (cls.features & bit)
                    fail(p + ".features", "duplicate feature");
                cls.features |= bit;
            }
            array(c.at("spells"), p + ".spells", cls.spells.size());
            for (const auto& spell : c.at("spells"))
                cls.spells[cls.spellCount++] = {reference(spell, p + ".spells", out.spells)};
        }
        const auto& saves = c.at("saves");
        array(saves, p + ".saves", 2);
        if (saves.size() != 2)
            fail(p + ".saves", "expected two saving throw proficiencies");
        for (const auto& save : saves) {
            const auto index = choice(save, p + ".saves", abilities);
            if (cls.saves[index])
                fail(p + ".saves", "duplicate proficiency");
            cls.saves[index] = true;
        }
        const auto& armor = c.at("armor");
        array(armor, p + ".armor", 4);
        for (const auto& a : armor) {
            const auto bit = armorBit(static_cast<ArmorType>(
                choice(a, p + ".armor", std::array<const char*, 4>{"light", "medium", "heavy", "shield"})));
            if (cls.armor & bit)
                fail(p + ".armor", "duplicate proficiency");
            cls.armor |= bit;
        }
        out.classes.push_back(std::move(cls));
    }
    const auto& items = root.at("items");
    array(items, "items", Catalog::maxItems);
    for (std::size_t i = 0; i < items.size(); ++i) {
        const auto p = "items[" + std::to_string(i) + "]";
        auto j = items[i];
        if (!j.is_object() || !j.contains("kind"))
            fail(p + ".kind", "missing item kind");
        ItemDefinition item;
        item.kind = static_cast<ItemKind>(
            choice(j.at("kind"), p + ".kind",
                   std::array<const char*, 8>{"weapon", "armor", "shield", "ammunition", "gear", "consumable",
                                              "currency", "supply"}));
        if (version < 6 && item.kind > ItemKind::Consumable)
            fail(p + ".kind", "currency/supplies require schema 6");
        if (version >= 7 && item.kind == ItemKind::Armor) {
            item.metalArmor = boolean(j.at("metal_armor"), p + ".metal_armor");
            j.erase("metal_armor");
        }
        if (version >= 5) {
            if (!j.contains("stack_limit"))
                fail(p + ".stack_limit", "missing field");
            const auto limit = integer(j.at("stack_limit"), p + ".stack_limit");
            if (limit < 1 || limit > (version >= 6 ? 1000 : 100))
                fail(p + ".stack_limit", "expected 1..100");
            item.stackMaximum = static_cast<std::uint16_t>(limit);
            j.erase("stack_limit");
            if (item.kind == ItemKind::Weapon || item.kind == ItemKind::Ammunition) {
                if (!j.contains("ammunition"))
                    fail(p + ".ammunition", "missing field");
                item.ammunition = static_cast<Ammunition>(choice(
                    j.at("ammunition"), p + ".ammunition", std::array<const char*, 2>{"none", "arrow"}));
                j.erase("ammunition");
            }
            if (item.kind == ItemKind::Consumable) {
                if (!j.contains("effect"))
                    fail(p + ".effect", "missing field");
                item.consumable = static_cast<Consumable>(
                    1 + choice(j.at("effect"), p + ".effect", std::array<const char*, 2>{"healing", "mana"}));
                j.erase("effect");
            }
        } else if (item.kind == ItemKind::Consumable)
            fail(p + ".kind", "consumables require schema 5");
        if (item.kind == ItemKind::Weapon) {
            fields(j, p,
                   {"id", "name", "description", "weight", "kind", "weapon", "damage", "ranged", "finesse",
                    "two_handed", "versatile_sides"});
            item.weapon = static_cast<WeaponType>(choice(j.at("weapon"), p + ".weapon", weapons));
            if (version < 6 && item.weapon == WeaponType::Scimitar)
                fail(p + ".weapon", "scimitar requires schema 6");
            const auto& damage = j.at("damage");
            fields(damage, p + ".damage", {"count", "sides", "type"});
            item.damage = {integer(damage.at("count"), p + ".damage.count"),
                           integer(damage.at("sides"), p + ".damage.sides"), 0};
            item.damageType = static_cast<DamageType>(
                choice(damage.at("type"), p + ".damage.type",
                       std::array<const char*, 3>{"bludgeoning", "piercing", "slashing"}));
            item.ranged = boolean(j.at("ranged"), p + ".ranged");
            item.finesse = boolean(j.at("finesse"), p + ".finesse");
            item.twoHanded = boolean(j.at("two_handed"), p + ".two_handed");
            item.versatileSides = integer(j.at("versatile_sides"), p + ".versatile_sides");
        } else if (item.kind == ItemKind::Armor) {
            fields(j, p, {"id", "name", "description", "weight", "kind", "armor", "base_ac"});
            item.armor = static_cast<ArmorType>(
                choice(j.at("armor"), p + ".armor", std::array<const char*, 3>{"light", "medium", "heavy"}));
            item.baseArmorClass = integer(j.at("base_ac"), p + ".base_ac");
        } else {
            fields(j, p, {"id", "name", "description", "weight", "kind"});
        }
        if (version >= 5 && ((item.kind == ItemKind::Ammunition && item.ammunition == Ammunition::None) ||
                             (item.kind == ItemKind::Weapon &&
                              (item.weapon == WeaponType::Shortbow || item.weapon == WeaponType::Longbow) &&
                              item.ammunition != Ammunition::Arrow)))
            fail(p + ".ammunition", "arrows required for ammunition and bows");
        item.id = identifier(j.at("id"), p + ".id");
        item.name = string(j.at("name"), p + ".name");
        item.description = string(j.at("description"), p + ".description");
        item.weight = integer(j.at("weight"), p + ".weight");
        out.items.push_back(std::move(item));
    }
    if (version >= 8) {
        array(root.at("creation"), "creation", Catalog::maxClasses);
        for (const auto& j : root.at("creation")) {
            fields(j, "creation[]", {"class", "recommended", "groups", "supplies"});
            CreationClass entry;
            entry.characterClass = {reference(j.at("class"), "creation.class", out.classes)};
            entry.recommended = scores(j.at("recommended"), "creation.recommended");
            auto grant = [&](const Json& v) {
                fields(v, "grant", {"item", "quantity", "slot"});
                const int q = integer(v.at("quantity"), "grant.quantity");
                if (q < 1 || q > 1000)
                    fail("grant.quantity", "out of bounds");
                const auto slot = string(v.at("slot"), "grant.slot");
                std::uint8_t n = 255;
                if (slot == "body")
                    n = 0;
                else if (slot == "main_hand")
                    n = 1;
                else if (slot == "off_hand")
                    n = 2;
                else if (slot == "ranged")
                    n = 3;
                else if (slot != "bag")
                    fail("grant.slot", "unknown slot");
                return StartingGrant{{reference(v.at("item"), "grant.item", out.items)}, std::uint16_t(q), n};
            };
            array(j.at("groups"), "creation.groups", 3);
            for (const auto& group : j.at("groups")) {
                array(group, "creation.group", 2);
                if (group.empty())
                    fail("creation.group", "empty choices");
                auto& g = entry.groups[entry.groupCount++];
                for (const auto& option : group) {
                    fields(option, "option", {"id", "name", "items"});
                    auto& o = g.options[g.count++];
                    o.id = identifier(option.at("id"), "option.id");
                    o.name = string(option.at("name"), "option.name");
                    array(option.at("items"), "option.items", 2);
                    for (const auto& v : option.at("items"))
                        o.items[o.count++] = grant(v);
                }
            }
            array(j.at("supplies"), "creation.supplies", 6);
            for (const auto& v : j.at("supplies"))
                entry.supplies[entry.supplyCount++] = grant(v);
            out.creation.push_back(std::move(entry));
        }
    }
    if (version >= 8 && out.creation.size() != out.classes.size())
        fail("creation", "schema 8 requires creation metadata for every class");
    const auto& preset = root.at("preset_party");
    if (preset.is_null())
        return out;
    fields(preset, "preset_party", {"characters", "formation"});
    const auto& characters = preset.at("characters");
    array(characters, "preset_party.characters", partySize);
    if (characters.size() != partySize)
        fail("preset_party.characters", "expected exactly four characters");
    out.preset.emplace();
    for (std::size_t i = 0; i < partySize; ++i) {
        const auto p = "preset_party.characters[" + std::to_string(i) + "]";
        const auto& c = characters[i];
        if (version >= 8)
            fields(c, p,
                   {"id", "name", "race", "class", "abilities", "level", "defense", "items", "equipment",
                    "life_domain", "portrait", "spell_grants"});
        else if (spellsEnabled)
            fields(c, p,
                   {"id", "name", "race", "class", "abilities", "level", "defense", "items", "equipment",
                    "life_domain"});
        else
            fields(c, p,
                   {"id", "name", "race", "class", "abilities", "level", "defense", "items", "equipment"});
        auto& b = out.preset->characters[i];
        b.id = identifier(c.at("id"), p + ".id");
        b.name = string(c.at("name"), p + ".name");
        b.race = {reference(c.at("race"), p + ".race", out.races)};
        b.characterClass = {reference(c.at("class"), p + ".class", out.classes)};
        b.baseAbilities = scores(c.at("abilities"), p + ".abilities");
        b.level = integer(c.at("level"), p + ".level");
        b.defense = boolean(c.at("defense"), p + ".defense");
        if (spellsEnabled)
            b.lifeDomain = boolean(c.at("life_domain"), p + ".life_domain");
        b.portrait = version >= 8 ? findPortrait(string(c.at("portrait"), p + ".portrait"))
                                  : defaultPortrait(out.races[b.race.index].id);
        if (version >= 8) {
            const auto& grants = c.at("spell_grants");
            fields(grants, p + ".spell_grants", {"cantrips", "learned", "prepared", "domain", "racial"});
            auto mask = [&](const char* key) {
                array(grants.at(key), key, 32);
                std::uint32_t bits = 0;
                for (const auto& id : grants.at(key)) {
                    const auto bit = spellBit({reference(id, key, out.spells)});
                    if (bits & bit)
                        fail(key, "duplicate spell");
                    bits |= bit;
                }
                return bits;
            };
            b.spells = {mask("cantrips"), mask("learned"), mask("prepared"), mask("domain"), mask("racial")};
        }
        const auto& owned = c.at("items");
        array(owned, p + ".items", maxStartingItems);
        b.itemCount = owned.size();
        for (std::size_t n = 0; n < owned.size(); ++n) {
            const auto q = p + ".items[" + std::to_string(n) + "]";
            fields(owned[n], q, {"item", "quantity"});
            const int quantity = integer(owned[n].at("quantity"), q + ".quantity");
            if (quantity < 1 || quantity > UINT16_MAX)
                fail(q + ".quantity", "expected 1..65535");
            b.items[n] = {{reference(owned[n].at("item"), q + ".item", out.items)},
                          static_cast<std::uint16_t>(quantity)};
        }
        const auto& e = c.at("equipment");
        if (root.at("schema_version") >= 3)
            fields(e, p + ".equipment", {"body", "main_hand", "off_hand", "ranged"});
        else
            fields(e, p + ".equipment", {"body", "main_hand", "off_hand"});
        if (root.at("schema_version") >= 3)
            b.equipment.ranged = ownedReference(e.at("ranged"), p + ".equipment.ranged");
        b.equipment = {ownedReference(e.at("body"), p + ".equipment.body"),
                       ownedReference(e.at("main_hand"), p + ".equipment.main_hand"),
                       ownedReference(e.at("off_hand"), p + ".equipment.off_hand"), b.equipment.ranged};
    }
    const auto& formation = preset.at("formation");
    array(formation, "preset_party.formation", partySize);
    if (formation.size() != partySize)
        fail("preset_party.formation", "expected exactly four entries");
    for (std::size_t i = 0; i < partySize; ++i) {
        const auto p = "preset_party.formation[" + std::to_string(i) + "]";
        const auto id = identifier(formation[i], p);
        bool found = false;
        for (std::size_t c = 0; c < partySize; ++c) {
            if (id == out.preset->characters[c].id) {
                out.preset->formation[i] = {static_cast<std::uint16_t>(c)};
                found = true;
                break;
            }
        }
        if (!found)
            fail(p, "unknown character reference: " + id);
    }
    if (version < 8) {
        Catalog catalog({}, {}, out.races, out.classes, out.items, std::nullopt, out.spells);
        for (auto& b : out.preset->characters) {
            const auto derived = deriveCharacter(catalog, b);
            if (!derived)
                fail("preset_party", characterErrorMessage(derived.error));
            if (!recommendSpells(catalog, b.view(), b.spells))
                fail("preset_party.spells",
                     "legacy caster needs explicit legal selections and sufficient spell content (schema 8)");
        }
    }
    return out;
}
} // namespace blobber
