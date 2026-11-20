#include "game/commands.hpp"
#include "game/perception.hpp"
#include "ui/layout.hpp"
#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstring>
namespace blobber {
void CommandController::refreshMagic() noexcept {
    auto& m = state_.magic;
    const auto caster =
        m.state == CastingState::Normal || m.open ? state_.selection.character : m.pending.character;
    const auto* c = party_.character(caster);
    if (!c) {
        m = {};
        return;
    }
    m.count = 0;
    m.name = "";
    m.description = "";
    m.cost = 0;
    m.charges = -1;
    m.open = m.state == CastingState::SpellPicker;
    m.armed = m.state == CastingState::Targeting;
    m.kind = rules::SpellKind::Count;
    m.assigned = m.maximum = 0;
    m.healingPool = m.healingCapacity = m.healingMaximum = 0;
    m.single = m.ground = m.self = false;
    m.selectedAllies = {};
    m.allyReasons.fill(CommandReason::InvalidAlly);
    m.footprint = {};
    m.hostile = m.area = m.missiles = false;
    m.affectedCount = 0;
    m.cells = {};
    m.targets = {};
    m.mana = c->mana;
    m.maxMana = c->maxMana;
    m.attackBonus = c->stats.proficiency + party_.castingModifier(caster);
    m.difficulty = 8 + m.attackBonus;
    m.attackBonus += c->stats.spellAttackBonus;
    const auto* cls = party_.catalog().characterClass(c->build.characterClass);
    const bool ranger = cls->progression == rules::ProgressionClass::Ranger;
    m.spellcaster = (cls->casting != rules::Casting::None && (!ranger || c->build.level >= 2)) ||
                    c->build.spells.racial;
    static_assert(std::tuple_size<decltype(cls->spells)>::value + 10 <= MagicHud::capacity,
                  "Picker must hold all supported spells and features");
    const auto entry = [&](rules::SpellId spell, rules::Feature feature, const char* name, const char* desc,
                           int cost) {
        auto command = state_.selection;
        command.targetCount = 0;
        command.target.reset();
        command.spell = spell;
        command.feature = feature;
        command.action = feature == rules::Feature::None ? PartyAction::Spell : PartyAction::Feature;
        const auto reason = session_.combat().validate(command, session_.movement().player);
        const int charges = feature == rules::Feature::SecondWind          ? int(c->secondWind)
                            : feature == rules::Feature::ActionSurge       ? int(c->actionSurge)
                            : feature == rules::Feature::PreserveLife      ? int(c->channelDivinity)
                            : feature == rules::Feature::ArcaneRecovery    ? int(c->arcaneRecovery)
                            : feature == rules::Feature::Rage              ? int(party_.rageUses(caster))
                            : rules::frenzyStrike(feature)                 ? int(c->barbarian.frenzy)
                            : feature == rules::Feature::BardicInspiration ? party_.inspirationUses(caster)
                            : feature == rules::Feature::LayOnHands        ? c->layOnHands
                            : feature == rules::Feature::SacredWeapon      ? int(c->channelDivinity)
                                                                           : -1;
        const auto* definition = party_.catalog().spell(spell);
        assert(m.count < m.entries.size());
        if (m.count == m.entries.size())
            return;
        m.entries[m.count++] = {spell,
                                feature,
                                name,
                                desc,
                                cost,
                                definition ? definition->kind : rules::SpellKind::Count,
                                reason,
                                charges,
                                feature == rules::Feature::None ? PickerEntryKind::Spell
                                                                : PickerEntryKind::ActivatedFeature};
    };
    if (cls->progression == rules::ProgressionClass::Barbarian) {
        using F = rules::Feature;
        const bool raging = party_.raging(caster, state_.combat.gameplayMs);
        auto ability = [&](F f, const char* desc) { entry({}, f, rules::featureName(f), desc, 0); };
        if (raging)
            ability(F::EndRage, "End Rage early. Costs your bonus action.");
        else
            ability(F::Rage,
                    "Bonus action. For 60s, gain +2 Strength melee damage, resist physical hits and gain "
                    "advantage on Strength checks and saves. Ends concentration and blocks casting.");
        if (c->build.level >= 2) {
            ability(F::WeaponStrike, "Make a normal melee attack with your equipped weapon.");
            ability(F::RecklessAttack, "Gain advantage on Strength melee attacks this turn. Enemies have "
                                       "advantage against you until your next turn.");
        }
        if (c->build.level >= 3 && raging) {
            ability(F::FrenzyStrike, "Spend your extra swing for this Rage. Choose any legal melee target.");
            if (!c->barbarian.attacked)
                ability(F::RecklessFrenzy, "Spend Frenzy as your first swing with advantage. Enemies have "
                                           "advantage against you until your next turn.");
        }
    }
    if (cls->progression == rules::ProgressionClass::Bard)
        entry({}, rules::Feature::BardicInspiration, "Bardic Inspiration",
              "Give another companion a d6 for an attack or save. Bonus action; lasts one hour.", 0);
    for (std::uint16_t i = 0; i < party_.catalog().spellCount(); ++i) {
        const auto access = party_.spellAccess(caster, {i});
        if (!access)
            continue;
        const auto* s = party_.catalog().spell({i});
        const char* description =
            s->kind == rules::SpellKind::SpiritualWeapon
                ? "Summon and attack: 1d8 + WIS force. Lasts 60s, no concentration. Later: Abilities > "
                  "Command Spiritual Weapon. Move up to 2 cells and attack, free once per turn (6s outside "
                  "turns)."
                : s->description.c_str();
        entry({i}, rules::Feature::None, s->name.c_str(), description, access.cost);
        auto& row = m.entries[m.count - 1];
        row.source = access.source;
        row.casting = access.casting;
        row.attack = access.attack;
        row.difficulty = access.difficulty;
    }
    if (cls->progression == rules::ProgressionClass::Paladin) {
        entry({}, rules::Feature::LayOnHands, "Lay on Hands", "Choose a living ally and healing points. One action; pool refreshes on Long Rest. Disease and poison curing are deferred.", 0);
        if (c->build.level >= 2)
            entry({}, rules::Feature::DivineSmite, "Divine Smite", "Melee attack: spend 2 mana only on a hit for +2d8 radiant (+3d8 against undead/fiends). Critical doubles dice.", 2);
        if (c->build.level >= 3)
            entry({}, rules::Feature::SacredWeapon, "Sacred Weapon", "Empower your main-hand weapon for 60 seconds: +CHA (minimum 1) to attacks. One action and Channel Divinity; refreshes on rest. Ends on weapon change or incapacitation.", 0);
        if (session_.effects().sacredWeapon(caster).serial)
            entry({}, rules::Feature::DismissSacredWeapon, "Dismiss Sacred Weapon", "End Sacred Weapon without spending an action.", 0);
    }
    if (party_.hasFeature(caster, rules::ClassFeature::SecondWind))
        entry({}, rules::Feature::SecondWind, "Second Wind",
              "Heal yourself for 1d10 + level. One action; refreshes on a Short or Long Rest.", 0);
    if (party_.hasFeature(caster, rules::ClassFeature::ArcaneRecovery))
        entry({}, rules::Feature::ArcaneRecovery, "Arcane Recovery",
              "Recover 2 mana at levels 1-2, 4 at levels 3-4. Refreshes on Long Rest. Requires "
              "exploration without active threats.",
              0);
    if (c->build.level >= 2) {
        if (cls->progression == rules::ProgressionClass::Fighter)
            entry({}, rules::Feature::ActionSurge, "Action Surge",
                  "One extra normal action; activate before or after your normal action. Refreshes on a "
                  "Short or Long Rest.",
                  0);
        if (cls->progression == rules::ProgressionClass::Cleric && c->build.lifeDomain)
            entry({}, rules::Feature::PreserveLife, "Preserve Life",
                  "Requires a living party member below half max HP (rounded down). Shares 5 HP per cleric "
                  "level, lowest HP percentage first, up to that cap. Recharges on a short or long rest.",
                  0);
        if (cls->progression == rules::ProgressionClass::Rogue)
            entry({}, rules::Feature::CunningAction, "Cunning Action / Defend",
                  c->build.level >= 3
                      ? "Defend freely once per turn; automatic after your last normal action. Thief "
                        "may use this allowance for a potion instead."
                      : "Defend freely once per turn; automatic after your last normal action.",
                  0);
    }
    if (session_.effects().has(caster, EffectKind::Restrained))
        entry({}, rules::Feature::EscapeWeb, "Escape Web",
              "One action: Strength check against Web DC to end restraint.", 0);
    if (session_.effects().spiritualWeapon(caster)) {
        rules::SpellId id;
        for (std::uint16_t i = 0; i < party_.catalog().spellCount(); ++i)
            if (party_.catalog().spell({i})->kind == rules::SpellKind::SpiritualWeapon)
                id = {i};
        entry(id, rules::Feature::SpiritualWeapon, "Command Spiritual Weapon",
              "Free once per turn: move up to two clear floor steps and optionally attack an enemy in its "
              "destination. Click an enemy to attack immediately, or click floor and confirm to move.",
              0);
    }
    if (session_.effects().spiritualWeapon(caster))
        entry({}, rules::Feature::DismissSpiritualWeapon, "Dismiss Spiritual Weapon",
              "End this local summon before safe floor travel. No action, mana or cooldown cost.", 0);
    if (session_.effects().concentration(caster).serial)
        entry({}, rules::Feature::DismissConcentration, "End concentration",
              "End your concentration voluntarily. No action, mana or cooldown cost.", 0);
    const auto passive = [&](PassiveFeature id, const char* name, const char* description) {
        SpellBookEntry e;
        e.name = name;
        e.description = description;
        e.category = PickerEntryKind::AutomaticFeature;
        e.passive = id;
        assert(m.count < m.entries.size());
        if (m.count < m.entries.size())
            m.entries[m.count++] = e;
    };
    if (cls->progression == rules::ProgressionClass::Barbarian) {
        passive(PassiveFeature::UnarmoredDefense, "Unarmored Defense",
                "Use 10 + Dexterity and Constitution modifiers while unarmored. Shields work.");
        if (c->build.level >= 2)
            passive(PassiveFeature::DangerSense, "Danger Sense",
                    "Gain advantage on Dexterity saves against dangers you can see.");
    }
    if (c->build.defense ||
        (cls->progression == rules::ProgressionClass::Paladin && c->build.level >= 2))
        passive(PassiveFeature::Defense, "Defense",
                "+1 Armor Class while wearing body armor. Already included in your armor class.");
    if (cls->progression == rules::ProgressionClass::Bard && c->build.level >= 2)
        passive(PassiveFeature::SongOfRest, "Song of Rest",
                "Each companion spending Hit Dice during a short rest heals an additional 1d6.");
    if (cls->progression == rules::ProgressionClass::Bard && c->build.level >= 3)
        passive(PassiveFeature::CuttingWords, "Cutting Words",
                "When an enemy would hit, spend one use and your reaction to subtract 1d6 from its attack.");
    if (ranger && c->build.level >= 2)
        passive(PassiveFeature::Archery, "Archery", "+2 to ranged weapon attack rolls.");
    if (ranger && c->build.level >= 3)
        passive(PassiveFeature::ColossusSlayer, "Colossus Slayer",
                "Weapon hits against an already wounded enemy deal an extra 1d8 once per turn.");
    if (cls->casting == rules::Casting::None) {
        if (party_.hasFeature(caster, rules::ClassFeature::SneakAttack)) {
            passive(
                PassiveFeature::SneakAttack, "Sneak Attack",
                c->build.level >= 3
                    ? "Finesse/ranged hits add 2d6 once per round with advantage or an acting front ally; no "
                      "disadvantage."
                    : "Finesse or ranged hits add 1d6 with advantage, or an acting front ally engaging "
                      "the target. No disadvantage. Once per 6 gameplay seconds; criticals double the die.");
            passive(PassiveFeature::RearRowMelee, "Rear-row melee",
                    "Your normal melee attack works from the rear row. Targets must still be in "
                    "the three front cells and respect enemy front-row protection.");
        }
    }
    filterSpellbook();
    if (!m.casting())
        return;
    for (std::size_t i = 0; i < m.count; ++i) {
        const auto& e = m.entries[i];
        if (e.category == PickerEntryKind::AutomaticFeature)
            continue;
        if (state_.magic.pending.action == PartyAction::Spell
                ? e.spell.index == state_.magic.pending.spell.index && e.feature == rules::Feature::None
                : e.feature == state_.magic.pending.feature) {
            m.name = e.name;
            m.description = e.description;
            m.cost = e.cost;
            m.charges = e.charges;
        }
    }
    const auto* s = party_.catalog().spell(state_.magic.pending.spell);
    const auto* r = s ? rules::spellRules(s->kind) : nullptr;
    if (state_.magic.pending.action == PartyAction::Spell && r) {
        const auto access = party_.spellAccess(caster, state_.magic.pending.spell);
        m.attackBonus = access.attack;
        m.difficulty = access.difficulty;
        m.kind = s->kind;
        m.single = !r->area && !r->self && r->maxTargets == 1;
        m.ground = r->ground;
        m.self = r->self;
        m.maximum = r->maxTargets;
        m.hostile = r->hostile;
        m.area = r->area;
        m.missiles = s->kind == rules::SpellKind::MagicMissile || s->kind == rules::SpellKind::ScorchingRay;
    }
    if (m.pending.feature == rules::Feature::BardicInspiration) {
        m.maximum = 1;
        m.single = true;
    }
    if (rules::meleeFeature(m.pending.feature)) {
        m.hostile = true; m.single = true; m.maximum = 1;
    }
    if (m.pending.feature == rules::Feature::LayOnHands) {
        m.maximum = 1;
        m.healingPool = c->layOnHands;
        m.healingCapacity = 5 * c->build.level;
        if (m.pending.targetCount == 1) {
            const auto* ally = party_.character(m.pending.allies[0]);
            if (ally && ally->life != rules::Life::Dead)
                m.healingMaximum = std::max(0, std::min(c->layOnHands, ally->stats.maxHitPoints - ally->hitPoints));
            if (m.healingMaximum) {
                m.pending.healingAmount = std::clamp(m.pending.healingAmount, 1, m.healingMaximum);
                if (m.healingDragging)
                    m.healingDragAmount = std::clamp(m.healingDragAmount, 1.f, float(m.healingMaximum));
            }
        }
    }
    if (m.pending.feature == rules::Feature::SpiritualWeapon) {
        m.kind = rules::SpellKind::SpiritualWeapon;
        m.hostile = true;
        m.ground = true;
        m.single = true;
        m.maximum = 1;
    }
    if (m.state == CastingState::Resolving)
        return;
    if (m.area) {
        auto aimed = m.pending;
        if (m.hoverAim) {
            aimed.aimedSubcell = m.hoverAim;
            aimed.aimedCell.reset();
        } else if (m.hoverCell)
            aimed.aimedCell = m.hoverCell;
        m.footprint = session_.combat().area(aimed, session_.movement().player);
        m.cells = m.footprint.eligible;
        m.affected = m.footprint.actors;
        m.affectedCount = m.footprint.count;
        const auto cells = m.footprint.footprint.count();
        const auto allies = std::count(m.footprint.allies.begin(), m.footprint.allies.end(), true);
        for (std::size_t i = 0; i < rules::partySize; ++i)
            m.selectedAllies[i] = m.footprint.allies[party_.snapshot().formation[i].index];
        std::snprintf(m.targets.data(), m.targets.size(), "%d subcells / %zu enemies / %d allies%s",
                      int(cells), m.affectedCount, int(allies),
                      m.ground && !m.pending.aimedCell && !m.pending.aimedSubcell ? " / aim first" : "");
    } else if (m.hostile) {
        for (std::size_t i = 0; i < m.pending.targetCount; ++i) {
            m.affected[m.affectedCount++] = m.pending.enemies[i];
            m.assigned += m.pending.darts[i];
        }
        std::snprintf(m.targets.data(), m.targets.size(),
                      m.missiles ? (m.kind == rules::SpellKind::ScorchingRay ? "Rays assigned %u/3"
                                                                             : "Darts assigned %u/3")
                                 : "Enemies selected %u/%u",
                      m.missiles ? m.assigned : unsigned(m.pending.targetCount), m.maximum);
    } else if ((m.pending.action == PartyAction::Spell && !m.self) ||
               m.pending.feature == rules::Feature::LayOnHands ||
               m.pending.feature == rules::Feature::BardicInspiration) {
        for (std::size_t i = 0; i < rules::partySize; ++i) {
            auto one = m.pending;
            one.targetCount = 1;
            one.allies[0] = party_.snapshot().formation[i];
            if (m.pending.feature == rules::Feature::LayOnHands ||
                m.pending.feature == rules::Feature::BardicInspiration) {
                one.healingAmount = 1;
                m.allyReasons[i] = session_.combat().validate(one, session_.movement().player);
            } else
                m.allyReasons[i] = session_.combat().spellRecipient(one, session_.movement().player, 0);
            for (std::size_t j = 0; j < m.pending.targetCount; ++j)
                m.selectedAllies[i] |= one.allies[0].index == m.pending.allies[j].index;
        }
        if (m.pending.feature == rules::Feature::LayOnHands)
            std::snprintf(m.targets.data(), m.targets.size(), m.healingMaximum ? "Heal %d HP / Spend %d / Keep %d" : "Choose an injured ally", m.pending.healingAmount, m.pending.healingAmount, c->layOnHands - m.pending.healingAmount);
        else
            std::snprintf(m.targets.data(), m.targets.size(), "Allies selected %zu/%u", m.pending.targetCount, m.maximum);
    } else if (m.pending.feature == rules::Feature::PreserveLife) {
        const auto pool = party_.preserveLifePreview(caster);
        std::snprintf(m.targets.data(), m.targets.size(), "Healing: %d / %d / %d / %d HP (character order)",
                      pool[0], pool[1], pool[2], pool[3]);
    } else if (m.kind == rules::SpellKind::Goodberry)
        std::snprintf(m.targets.data(), m.targets.size(), "Party / confirm to cast");
    else
        std::snprintf(m.targets.data(), m.targets.size(), "Self / confirm to use");
}
void CommandController::healingAmount(int delta, bool maximum) noexcept {
    auto& m = state_.magic;
    if (!m.armed || state_.suspended || m.pending.feature != rules::Feature::LayOnHands || m.pending.targetCount != 1) return;
    const auto* source = party_.character(m.pending.character);
    const auto* target = party_.character(m.pending.allies[0]);
    if (!source || !target) return;
    const int cap = std::min(source->layOnHands, target->stats.maxHitPoints - target->hitPoints);
    if (cap < 1) return;
    m.pending.healingAmount = maximum ? cap : std::clamp(m.pending.healingAmount + delta, 1, cap);
    m.feedback = {};
    refreshMagic(); updateAvailability();
}
bool CommandController::routeHealingSlider(const UiInput& input, bool allowInput) noexcept {
    auto& m = state_.magic;
    if (!input.focused || !allowInput || state_.suspended || state_.modal() || !m.armed ||
        m.pending.feature != rules::Feature::LayOnHands || input.has(UiKey::Cancel) || input.has(UiKey::Back)) {
        m.healingDragging = false;
        return false;
    }
    const auto& current = session_.combat().snapshot();
    if (m.pending.world != current.world || m.pending.turn != current.turn) {
        m.healingDragging = false;
        return false;
    }
    if (m.pending.targetCount == 1 && m.pending.ally &&
        m.pending.ally->index == m.pending.allies[0].index &&
        (input.has(UiKey::Up) || input.has(UiKey::Down))) {
        m.healingDragging = false;
        healingAmount((input.has(UiKey::Up) ? 1 : -1) * (input.shift ? 5 : 1));
        return true;
    }
    const bool press = input.has(UiKey::Click) && input.pointer && hudLayout::healingSlider.contains(*input.pointer);
    if (!press && !m.healingDragging) return false;
    state_.buttons.cancel();
    if (press) {
        m.healingDragging = input.pointerTracked;
        const auto handle = hudLayout::healingHandle(m.pending.healingAmount, m.healingPool);
        m.healingDragOffset = m.healingMaximum > 0 && handle.contains(*input.pointer)
                                  ? input.pointer->x - hudLayout::healingHandleCenter(float(m.pending.healingAmount), m.healingPool)
                                  : 0;
    }
    if (input.pointer && m.healingMaximum > 0 && (press || input.pointerHeld || input.pointerReleased)) {
        const auto track = hudLayout::healingTrack;
        const float fraction = std::clamp((input.pointer->x - m.healingDragOffset - track.x) / float(track.width), 0.f, 1.f);
        m.healingDragAmount = std::clamp(fraction * m.healingPool, 1.f, float(m.healingMaximum));
        const int amount = int(std::lround(m.healingDragAmount));
        healingAmount(amount - m.pending.healingAmount);
    }
    if (!input.pointer || input.pointerReleased || (input.pointerTracked && !input.pointerHeld))
        m.healingDragging = false;
    return true;
}
void CommandController::setCastingState(CastingState next) noexcept {
    closeTargetPicker();
    auto& m = state_.magic;
    if (!state_.logExpanded) {
        const int before = m.casting() ? hudLayout::castingLogColumns : hudLayout::logColumns;
        const int after = next == CastingState::Targeting || next == CastingState::Resolving
                              ? hudLayout::castingLogColumns
                              : hudLayout::logColumns;
        state_.logOffset = state_.log.rewrapOffset(state_.logOffset, before, after);
    }
    m.healingDragging = false;
    m.state = next;
    m.open = next == CastingState::SpellPicker;
    m.armed = next == CastingState::Targeting;
}
void CommandController::magicFeedback(const char* text) noexcept {
    std::snprintf(state_.magic.feedback.data(), state_.magic.feedback.size(), "%s", text);
}
void CommandController::openSpellbook() noexcept {
    if (state_.suspended || state_.magic.state == CastingState::Resolving)
        return;
    closeOverlay();
    cancel();
    refreshMagic();
    auto& m = state_.magic;
    m.tab = bookTabs_[state_.selection.character.index];
    m.cursor = bookCursors_[state_.selection.character.index][m.tab];
    m.offset = bookOffsets_[state_.selection.character.index][m.tab];
    setCastingState(CastingState::SpellPicker);
    state_.hover = {};
    refreshMagic();
    updateAvailability();
}
void CommandController::backToSpellbook() noexcept {
    if (state_.magic.state != CastingState::Targeting || state_.suspended)
        return;
    openSpellbook();
}
namespace {
unsigned entryTab(const SpellBookEntry& e) noexcept {
    const auto* spell = rules::spellRules(e.kind);
    return e.category == PickerEntryKind::Spell && spell ? unsigned(spell->level) : MagicHud::abilityTab;
}
} // namespace
void CommandController::filterSpellbook() noexcept {
    auto& m = state_.magic;
    m.tabs = {};
    for (std::size_t i = 0; i < m.count; ++i)
        m.tabs[entryTab(m.entries[i])] = true;
    if (m.tab >= m.tabs.size() || !m.tabs[m.tab]) {
        m.tab = 0;
        while (m.tab + 1 < m.tabs.size() && !m.tabs[m.tab])
            ++m.tab;
        m.cursor = bookCursors_[state_.selection.character.index][m.tab];
        m.offset = bookOffsets_[state_.selection.character.index][m.tab];
    }
    m.visibleCount = 0;
    bool selected = false;
    for (std::size_t i = 0; i < m.count; ++i)
        if (entryTab(m.entries[i]) == m.tab) {
            m.visible[m.visibleCount++] = i;
            selected |= m.cursor == i;
        }
    if (!selected)
        m.cursor = m.visibleCount ? m.visible[0] : 0;
    m.offset = std::min(m.offset, m.visibleCount > 7 ? m.visibleCount - 7 : 0);
    for (std::size_t row = 0; row < m.visibleCount; ++row)
        if (m.visible[row] == m.cursor) {
            if (row < m.offset)
                m.offset = row;
            if (row >= m.offset + 7)
                m.offset = row - 6;
        }
}
void CommandController::rememberSpellbook() noexcept {
    const auto& m = state_.magic;
    const auto c = state_.selection.character.index;
    bookTabs_[c] = m.tab;
    bookCursors_[c][m.tab] = m.cursor;
    bookOffsets_[c][m.tab] = m.offset;
}
void CommandController::selectSpellbookTab(unsigned tab) noexcept {
    auto& m = state_.magic;
    if (!m.open || tab >= m.tabs.size() || !m.tabs[tab] || tab == m.tab)
        return;
    rememberSpellbook();
    m.tab = tab;
    m.cursor = bookCursors_[state_.selection.character.index][tab];
    m.offset = bookOffsets_[state_.selection.character.index][tab];
    filterSpellbook();
    rememberSpellbook();
}
void CommandController::cycleSpellbookTab(int delta) noexcept {
    const auto& m = state_.magic;
    const int direction = delta < 0 ? -1 : 1;
    for (int t = int(m.tab) + direction; t >= 0 && t < int(m.tabs.size()); t += direction)
        if (m.tabs[t]) {
            selectSpellbookTab(unsigned(t));
            return;
        }
}
void CommandController::browseSpellbook(int delta) noexcept {
    auto& m = state_.magic;
    if (!m.visibleCount)
        return;
    std::size_t row = 0;
    while (row + 1 < m.visibleCount && m.visible[row] != m.cursor)
        ++row;
    row = std::size_t(std::clamp<int>(int(row) + delta, 0, int(m.visibleCount) - 1));
    m.cursor = m.visible[row];
    if (row < m.offset)
        m.offset = row;
    if (row >= m.offset + 7)
        m.offset = row - 6;
    rememberSpellbook();
}
void CommandController::scrollSpellbook(int delta) noexcept {
    auto& m = state_.magic;
    if (!m.visibleCount)
        return;
    std::size_t row = 0;
    while (row + 1 < m.visibleCount && m.visible[row] != m.cursor)
        ++row;
    m.offset = std::size_t(std::clamp<int>(int(m.offset) + delta, 0, std::max(0, int(m.visibleCount) - 7)));
    row = std::clamp(row, m.offset, std::min(m.visibleCount - 1, m.offset + 6));
    m.cursor = m.visible[row];
    rememberSpellbook();
}
void CommandController::chooseSpellbook(std::size_t index) noexcept {
    auto& m = state_.magic;
    if (!m.open || state_.suspended || index >= m.count)
        return;
    selectSpellbookTab(entryTab(m.entries[index]));
    m.cursor = index;
    filterSpellbook();
    rememberSpellbook();
    const auto entry = m.entries[index];
    if (entry.category == PickerEntryKind::AutomaticFeature) {
        magicFeedback("");
        return;
    }
    if (entry.reason != CommandReason::Ready && entry.reason != CommandReason::MissingTarget) {
        magicFeedback(commandMessage(entry.reason));
        return;
    }
    m.pending = state_.selection;
    m.pending.target.reset();
    m.pending.ally.reset();
    m.pending.targetCount = 0;
    m.pending.aimedCell.reset();
    m.pending.aimedSubcell.reset();
    m.hoverCell.reset();
    m.hoverAim.reset();
    m.pending.action = entry.feature == rules::Feature::None ? PartyAction::Spell : PartyAction::Feature;
    m.pending.spell = entry.spell;
    m.pending.feature = entry.feature;
    m.pending.healingAmount = 1;
    magicFeedback("");
    setCastingState(CastingState::Targeting);
    state_.hover = {};
    refreshMagic();
    updateAvailability();
    if (entry.feature == rules::Feature::ActionSurge || entry.feature == rules::Feature::SecondWind)
        confirm();
}
bool CommandController::magicSingleTarget() const noexcept {
    return state_.magic.armed && state_.magic.single;
}
CommandReason CommandController::magicEligibility(ActorId id) const noexcept {
    const auto& m = state_.magic;
    if (!m.armed || !m.hostile || m.area)
        return CommandReason::InvalidTarget;
    if (!visible(id))
        return CommandReason::NotVisible;
    auto c = m.pending;
    if (rules::meleeFeature(c.feature)) {
        c.targetCount = 1; c.enemies[0] = id; c.target = id;
        return session_.combat().validate(c, session_.movement().player);
    }
    if (c.feature == rules::Feature::SpiritualWeapon) {
        c.targetCount = 1;
        c.enemies[0] = id;
        c.aimedCell = actors_.actor(id)->npc.cell;
        return session_.combat().validate(c, session_.movement().player);
    }
    std::size_t index = 0;
    if (m.kind == rules::SpellKind::AcidSplash && c.targetCount && !sameActor(c.enemies[0], id))
        index = 1;
    c.enemies[index] = id;
    return session_.combat().spellRecipient(c, session_.movement().player, index);
}
void CommandController::removeMagicRecipient(std::size_t index) noexcept {
    auto& c = state_.magic.pending;
    if (!state_.magic.armed || state_.suspended || index >= c.targetCount)
        return;
    for (std::size_t j = index + 1; j < c.targetCount; ++j) {
        c.enemies[j - 1] = c.enemies[j];
        c.allies[j - 1] = c.allies[j];
        c.darts[j - 1] = c.darts[j];
    }
    --c.targetCount;
    magicFeedback("");
    refreshMagic();
    updateAvailability();
}
void CommandController::magicEnemy(ActorId id, bool remove) noexcept {
    auto& m = state_.magic;
    auto& c = m.pending;
    if (!m.armed || !m.hostile || m.area || state_.suspended)
        return;
    c.target = id;
    std::size_t selected = c.targetCount;
    for (std::size_t i = 0; i < c.targetCount; ++i)
        if (sameActor(c.enemies[i], id))
            selected = i;
    magicFeedback("");
    if (remove || (!m.missiles && !m.single && selected < c.targetCount)) {
        if (selected < c.targetCount) {
            if (m.missiles && c.darts[selected] > 1)
                --c.darts[selected];
            else
                removeMagicRecipient(selected);
        }
    } else {
        const auto reason = magicEligibility(id);
        if (reason != CommandReason::Ready) {
            magicFeedback(commandMessage(reason));
            return;
        }
        if (m.missiles && m.assigned == 3) {
            magicFeedback("Remove a dart to reassign it.");
            return;
        }
        if (!m.single && selected == c.targetCount && c.targetCount == m.maximum) {
            magicFeedback("Remove a target first.");
            return;
        }
        if (m.single) {
            selected = 0;
            c.targetCount = 1;
        } else if (selected == c.targetCount) {
            ++c.targetCount;
            c.darts[selected] = 0;
        }
        c.enemies[selected] = id;
        if (c.feature == rules::Feature::SpiritualWeapon)
            c.aimedCell = actors_.actor(id)->npc.cell;
        if (m.missiles)
            ++c.darts[selected];
    }
    refreshMagic();
    updateAvailability();
}
void CommandController::magicAlly(std::size_t formation) noexcept {
    auto& m = state_.magic;
    auto& c = m.pending;
    if (!m.armed || m.hostile || m.self || state_.suspended || formation >= rules::partySize ||
        (c.action != PartyAction::Spell && c.feature != rules::Feature::BardicInspiration &&
         c.feature != rules::Feature::LayOnHands))
        return;
    const auto id = party_.snapshot().formation[formation];
    c.ally = id;
    magicFeedback("");
    for (std::size_t i = 0; i < c.targetCount; ++i)
        if (c.allies[i].index == id.index && !m.single && c.feature != rules::Feature::LayOnHands) {
            removeMagicRecipient(i);
            return;
        }
    if (m.allyReasons[formation] != CommandReason::Ready) {
        magicFeedback(commandMessage(m.allyReasons[formation]));
        return;
    }
    if (c.feature == rules::Feature::LayOnHands) {
        const bool changed = c.targetCount != 1 || c.allies[0].index != id.index;
        c.allies[0] = id; c.targetCount = 1;
        const auto* ally = party_.character(id);
        const int cap = std::min(party_.character(c.character)->layOnHands, ally->stats.maxHitPoints - ally->hitPoints);
        c.healingAmount = changed ? cap : std::clamp(c.healingAmount, 1, cap);
        if (changed) m.healingDragging = false;
    } else if (m.single) {
        c.allies[0] = id;
        c.targetCount = 1;
    } else if (c.targetCount < m.maximum)
        c.allies[c.targetCount++] = id;
    else
        magicFeedback("Remove a target first.");
    refreshMagic();
    updateAvailability();
}
void CommandController::magicKeyboardTarget(int delta) noexcept {
    auto& m = state_.magic;
    if (m.ground && m.area) {
        std::array<SpellAim, 625> points{};
        std::size_t count = 0, index = 0;
        unsigned formation = 0;
        for (unsigned i = 0; i < 4; ++i)
            if (party_.snapshot().formation[i].index == m.pending.character.index)
                formation = i;
        const auto pose = session_.movement().player;
        const auto caster = formationSubcell(pose, formation);
        for (int y = -12; y <= 12; ++y)
            for (int x = -12; x <= 12; ++x) {
                const SpellAim point{caster.x + x, caster.y + y};
                if (!validSpellAim(actors_.map(), pose, formation, point))
                    continue;
                if (m.pending.aimedSubcell && *m.pending.aimedSubcell == point)
                    index = count;
                points[count++] = point;
            }
        if (count)
            magicAim(points[(index + count + (delta < 0 ? -1 : 1)) % count], true);
        return;
    }
    if (m.ground) {
        std::array<Cell, 169> cells{};
        std::size_t count = 0, index = 0;
        const auto pose = session_.movement().player;
        for (int y = -6; y <= 6; ++y)
            for (int x = -6; x <= 6; ++x) {
                const auto cell = pose.cell + Cell{x, y};
                if (!spellCell(actors_.map(), pose, cell))
                    continue;
                if (m.pending.aimedCell && *m.pending.aimedCell == cell)
                    index = count;
                cells[count++] = cell;
            }
        if (count)
            magicGround(cells[(index + count + (delta < 0 ? -1 : 1)) % count], true);
        return;
    }
    if (m.hostile && !m.area) {
        cycleEnemyTarget(delta);
    } else if (!m.hostile) {
        unsigned index = delta < 0 ? 0 : 3;
        for (unsigned i = 0; i < rules::partySize; ++i)
            if (m.pending.ally && party_.snapshot().formation[i].index == m.pending.ally->index)
                index = i;
        index = (index + (delta < 0 ? 3 : 1)) % rules::partySize;
        m.pending.ally = party_.snapshot().formation[index];
    }
}
void CommandController::magicAim(std::optional<SpellAim> aim, bool select) noexcept {
    auto& m = state_.magic;
    if (!m.armed || !m.ground || !m.area || state_.suspended)
        return;
    unsigned formation = 0;
    for (unsigned i = 0; i < 4; ++i)
        if (party_.snapshot().formation[i].index == m.pending.character.index)
            formation = i;
    if (aim && !validSpellAim(actors_.map(), session_.movement().player, formation, *aim))
        aim.reset();
    m.hoverAim = aim;
    if (select && aim) {
        m.pending.aimedSubcell = aim;
        m.pending.aimedCell.reset();
        m.pending.target.reset();
        m.pending.targetCount = 0;
        m.hoverAim.reset();
    }
    refreshMagic();
    updateAvailability();
}
void CommandController::magicGround(std::optional<Cell> cell, bool select) noexcept {
    auto& m = state_.magic;
    if (m.area) {
        magicAim(cell ? std::optional<SpellAim>{cellAim(*cell)} : std::nullopt, select);
        return;
    }
    if (!m.armed || !m.ground || state_.suspended)
        return;
    if (cell && !spellCell(actors_.map(), session_.movement().player, *cell))
        cell.reset();
    m.hoverCell = cell;
    if (select && cell) {
        m.pending.aimedCell = cell;
        m.pending.target.reset();
        m.pending.targetCount = 0;
        m.hoverCell.reset();
    }
    refreshMagic();
    updateAvailability();
}
SpellArea CommandController::floorPreview() const noexcept {
    SpellArea result;
    const auto& m = state_.magic;
    if (m.armed && m.area && !state_.modal() && !session_.actionBusy()) {
        auto aimed = m.pending;
        if (m.hoverAim) {
            aimed.aimedSubcell = m.hoverAim;
            aimed.aimedCell.reset();
        } else if (m.hoverCell)
            aimed.aimedCell = m.hoverCell;
        result = session_.combat().area(aimed, session_.movement().player);
        result.kind = m.kind;
    }
    result.ground = session_.effects().ground();
    return result;
}
void CommandController::reconcileMagic() noexcept {
    auto& m = state_.magic;
    if (!m.casting())
        return;
    if (m.state == CastingState::Resolving) {
        if (!session_.combat().casterResolving(m.pending.character))
            cancel();
        return;
    }
    if (m.pending.world != state_.combat.world || m.pending.turn != state_.combat.turn ||
        m.pending.character.index != state_.selection.character.index ||
        !session_.effects().canAct(m.pending.character)) {
        cancel();
        return;
    }
    if (state_.suspended)
        return;
    if (m.hostile && m.pending.target && !visible(*m.pending.target))
        m.pending.target.reset();
    for (std::size_t i = 0; i < m.pending.targetCount;) {
        auto one = m.pending;
        one.targetCount = 1; one.enemies[0] = m.pending.enemies[i]; one.allies[0] = m.pending.allies[i];
        const auto reason = m.pending.action == PartyAction::Feature
                                ? session_.combat().validate(one, session_.movement().player)
                                : session_.combat().spellRecipient(m.pending, session_.movement().player, i);
        if (reason != CommandReason::Ready || (m.hostile && !visible(m.pending.enemies[i]))) {
            removeMagicRecipient(i);
            magicFeedback("Target no longer eligible; selection updated.");
        } else
            ++i;
    }
}
CommandReason CommandController::confirm() noexcept {
    if (state_.suspended)
        return suspension_;
    if (state_.modal())
        return CommandReason::LogOpen;
    if (state_.magic.state == CastingState::Resolving)
        return CommandReason::AttackPending;
    if (state_.magic.armed) {
        auto& m = state_.magic;
        if (m.single && (!m.ground || m.pending.target)) {
            CommandReason eligibility = CommandReason::MissingTarget;
            if (m.hostile && m.pending.target)
                eligibility = magicEligibility(*m.pending.target);
            else if (m.pending.ally)
                for (std::size_t i = 0; i < state_.cards.size(); ++i)
                    if (state_.cards[i].id.index == m.pending.ally->index)
                        eligibility = m.allyReasons[i];
            if (eligibility != CommandReason::Ready) {
                magicFeedback(commandMessage(eligibility));
                return eligibility;
            }
            m.pending.targetCount = 0;
            if (m.hostile && m.pending.target)
                magicEnemy(*m.pending.target);
            else if (m.pending.ally)
                for (std::size_t i = 0; i < state_.cards.size(); ++i)
                    if (state_.cards[i].id.index == m.pending.ally->index)
                        magicAlly(i);
        }
        const auto command = m.pending;
        const auto reason = submit(command);
        if (reason != CommandReason::Resolved)
            magicFeedback(commandMessage(reason));
        return reason;
    }
    return submit(state_.selection);
}
} // namespace blobber
