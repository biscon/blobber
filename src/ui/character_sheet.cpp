#include "ui/hud_painter.hpp"

namespace blobber {
using namespace hudInk;
namespace {
int paragraph(const HudPainter& p, const char* value, int x, int y, int width, int rows,
              Color color = hudInk::text) {
    if (!p.fonts)
        return 0;
    int row = 0;
    for (; row < rows && *value; ++row) {
        while (*value == ' ')
            ++value;
        if (!*value)
            break;
        char line[384]{};
        std::size_t count = 0, space = 0;
        while (value[count] && count < sizeof(line) - 1) {
            line[count] = value[count];
            line[count + 1] = 0;
            if (p.fonts->width(line, UiTextStyle::Small) > width)
                break;
            if (value[count] == ' ')
                space = count;
            ++count;
        }
        if (value[count] && space)
            count = space;
        count = std::max<std::size_t>(1, count);
        line[count] = 0;
        p.clipped(line, x, float(y + row * 11), width, color, UiTextStyle::Small);
        value += count;
    }
    return row;
}
const char* damageName(rules::DamageType type) {
    constexpr const char* names[]{"bludgeoning", "piercing", "slashing", "acid",   "fire",
                                  "force",       "radiant",  "necrotic", "thunder"};
    return names[static_cast<unsigned>(type)];
}
} // namespace
void HudPainter::characterSheet(const HudSnapshot& h) const {
    const auto& i = h.inventory;
    const auto& s = i.sheet;
    const HudCard* card = nullptr;
    int formation = 0;
    for (int n = 0; n < 4; ++n)
        if (h.cards[n].id.index == i.character.index) {
            card = &h.cards[n];
            formation = n;
        }
    if (!card)
        return;
    const auto& c = *card;
    char value[192];
    line({178, 72}, {178, 339}, edge);
    line({452, 72}, {452, 339}, edge);
    clipped(c.name, 16, 69, 150, gold, UiTextStyle::Display);
    picture(c.portrait, {17, 89, 77, 96});
    border({16, 88, 79, 98}, edge);
    std::snprintf(value, sizeof(value), "Level %d %s", c.level, c.className);
    int identityY = 99;
    identityY += paragraph(*this, value, 101, identityY, 69, 3) * 11 + 4;
    identityY += paragraph(*this, c.raceName, 101, identityY, 69, 3, muted) * 11 + 4;
    constexpr const char* positions[]{"Front left", "Front right", "Rear left", "Rear right"};
    text(positions[formation], 101, identityY, muted, UiTextStyle::Small);
    line({16, 198}, {169, 198}, edge);
    text("EXPERIENCE", 16, 207, gold, UiTextStyle::Display);
    if (s.nextLevelExperience) {
        std::snprintf(value, sizeof(value), "%u / %u XP", s.experience, s.nextLevelExperience);
        text(value, 16, 223);
        box({16, 237, 152, 10});
        const float progress = std::min(1.f, float(s.experience) / s.nextLevelExperience);
        fill({18, 239, int(148 * progress), 6}, gold);
        const auto remaining = s.nextLevelExperience - std::min(s.experience, s.nextLevelExperience);
        std::snprintf(value, sizeof(value), "%u XP to level %d", remaining, c.level + 1);
        text(value, 16, 251, muted, UiTextStyle::Small);
    } else {
        std::snprintf(value, sizeof(value), "%u XP", s.experience);
        text(value, 16, 223);
    }
    line({16, 268}, {169, 268}, edge);
    text("HIT DICE", 16, 277, gold, UiTextStyle::Display);
    std::snprintf(value, sizeof(value), "%d / %d d%d", s.hitDice, c.level, s.hitDie);
    text(value, 16, 293);
    if (c.hasMana) {
        std::snprintf(value, sizeof(value), "Mana %d / %d", c.mana, c.maxMana);
        text(value, 16, 310, teal);
        std::snprintf(value, sizeof(value), "Class %s %+d / Save DC %d",
                      s.casting == rules::Casting::Wisdom ? "WIS" : "INT", s.spellAttack, s.spellDifficulty);
        clipped(value, 16, 323, 153, muted, UiTextStyle::Small);
    }
    if (s.racialCasting) {
        std::snprintf(value, sizeof(value), "Racial INT %+d / DC %d", s.racialAttack, s.racialDifficulty);
        clipped(value, 16, c.hasMana ? 335 : 323, 153, teal, UiTextStyle::Small);
    }
    if (c.status != DisplayStatus::Ready) {
        const char* status = c.status == DisplayStatus::Dead     ? "Dead"
                             : c.status == DisplayStatus::Stable ? "Stable"
                                                                 : "Unconscious";
        std::snprintf(value, sizeof(value), "%s / Saves %u successes, %u failures", status,
                      unsigned(c.successes), unsigned(c.failures));
        clipped(value, 16, s.racialCasting && c.hasMana ? 350 : 338, 153, red, UiTextStyle::Small);
    }
    text("COMBAT", 188, 69, gold, UiTextStyle::Display);
    constexpr const char* stats[]{"HIT POINTS", "ARMOR CLASS", "INITIATIVE", "PROFICIENCY"};
    for (int n = 0; n < 4; ++n) {
        const UiRect r{188 + n * 65, 89, 60, 40};
        box(r);
        centered(stats[n], {r.x, r.y + 5, r.width, 10}, gold, UiTextStyle::Small);
        if (n == 0)
            std::snprintf(value, sizeof(value), c.temporaryHp ? "%d/%d +%d temp" : "%d / %d", c.hp, c.maxHp,
                          c.temporaryHp);
        else if (n == 1)
            std::snprintf(value, sizeof(value), "%d", i.armorClass);
        else
            std::snprintf(value, sizeof(value), "%+d", n == 2 ? s.initiative : s.proficiency);
        centered(value, {r.x, r.y + 17, r.width, 20}, n == 0 && c.hp == 0 ? red : hudInk::text,
                 UiTextStyle::Control);
    }
    line({188, 140}, {443, 140}, edge);
    text("EQUIPPED ATTACKS", 188, 147, gold, UiTextStyle::Display);
    for (int n = 0; n < 2; ++n) {
        const int y = 164 + n * 41;
        const auto& cell = i.cells[n ? 51 : 49];
        const auto& w = cell.weapon;
        box({188, y, 255, 35});
        const char* slot = n ? "RANGED" : "MAIN HAND";
        if (!cell.definition || !w.available) {
            text(slot, 193, y + 5, gold, UiTextStyle::Small);
            text("Not equipped", 193, y + 19, muted);
            continue;
        }
        picture(UiArt::itemIcon(*cell.definition), {190, y + 2, 31, 31});
        clipped(cell.definition->name.c_str(), 229, y + 5, 76, hudInk::text, UiTextStyle::Label);
        text(slot, 229, y + 21, gold, UiTextStyle::Small);
        line({308, float(y + 5)}, {308, float(y + 30)}, edge);
        text("Attack", 316, y + 4, muted, UiTextStyle::Small);
        std::snprintf(value, sizeof(value), "%+d", w.attackBonus);
        text(value, 316, y + 16);
        line({352, float(y + 5)}, {352, float(y + 30)}, edge);
        std::snprintf(value, sizeof(value), "%dd%d%+d %s", w.damage.count, w.damage.sides, w.damage.modifier,
                      damageName(w.damageType));
        clipped(value, 360, y + 5, 80, hudInk::text, UiTextStyle::Small);
        const auto low = std::max<std::int64_t>(0, w.damage.count + w.damage.modifier);
        const auto high =
            std::max<std::int64_t>(0, std::int64_t(w.damage.count) * w.damage.sides + w.damage.modifier);
        if (rules::ammunitionFor(*cell.definition) == rules::Ammunition::Arrow)
            std::snprintf(value, sizeof(value), "%lld-%lld damage / %u arrows", (long long)low,
                          (long long)high, s.arrows);
        else
            std::snprintf(value, sizeof(value), "%lld-%lld damage%s", (long long)low, (long long)high,
                          w.proficient ? "" : " / untrained");
        clipped(value, 360, y + 21, 80, muted, UiTextStyle::Small);
    }
    line({188, 249}, {443, 249}, edge);
    text("CLASS FEATURES", 188, 255, gold, UiTextStyle::Display);
    if (!s.featureCount)
        text("None", 188, 272, muted);
    const int rowHeight = s.featureCount > 2 ? 8 : 22;
    for (unsigned n = 0; n < s.featureCount; ++n) {
        const auto& f = s.features[n];
        const int y = 272 + int(n) * rowHeight;
        clipped(f.name, 188, y, 82, hudInk::text, UiTextStyle::Small);
        clipped(f.value.data(), 274, y, 166, gold, UiTextStyle::Small);
        if (s.featureCount <= 2)
            clipped(f.detail, 188, y + 10, 255, muted, UiTextStyle::Small);
    }
    line({188, 315}, {443, 315}, edge);
    text("ACTIVE EFFECTS", 188, 319, gold, UiTextStyle::Display);
    paragraph(*this, s.effects.data(), 188, 333, 255, 2, teal);
    text("ABILITIES & SAVES", 461, 69, gold, UiTextStyle::Display);
    border({461, 89, 165, 125}, edge);
    text("Ability", 467, 95, gold, UiTextStyle::Small);
    centered("Score", {520, 92, 28, 13}, gold, UiTextStyle::Small);
    centered("Modifier", {550, 92, 40, 13}, gold, UiTextStyle::Small);
    centered("Save", {594, 92, 30, 13}, gold, UiTextStyle::Small);
    constexpr const char* abilities[]{"Strength",     "Dexterity", "Constitution",
                                      "Intelligence", "Wisdom",    "Charisma"};
    for (int n = 0; n < 6; ++n) {
        const int y = 107 + n * 17;
        line({461, float(y)}, {626, float(y)}, edge);
        text(abilities[n], 467, y + 5, hudInk::text, UiTextStyle::Small);
        std::snprintf(value, sizeof(value), "%d", c.abilities[n]);
        centered(value, {520, y, 28, 17}, hudInk::text, UiTextStyle::Small);
        std::snprintf(value, sizeof(value), "%+d", s.modifiers[n]);
        centered(value, {550, y, 40, 17}, hudInk::text, UiTextStyle::Small);
        std::snprintf(value, sizeof(value), "%+d", s.saves[n]);
        centered(value, {594, y, 30, 17}, s.proficientSaves[n] ? teal : hudInk::text, UiTextStyle::Small);
    }
    text("Teal saves include proficiency.", 461, 220, teal, UiTextStyle::Small);
    line({461, 242}, {626, 242}, edge);
    text("PROFICIENCIES", 461, 250, gold, UiTextStyle::Display);
    text("Armor", 461, 269, gold, UiTextStyle::Small);
    paragraph(*this, s.armor.data(), 505, 269, 121, 3);
    text("Weapons", 461, 305, gold, UiTextStyle::Small);
    paragraph(*this, s.weapons.data(), 505, 305, 121, 4);
    text("1-4: character / Esc: close", 16, 345, muted, UiTextStyle::Small);
}
} // namespace blobber
