#include "game/commands.hpp"
#include "game/perception.hpp"
#include "ui/layout.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
namespace blobber {
void CommandController::refreshMagic() noexcept {
    auto& m = state_.magic;
    const auto caster =
        m.state == CastingState::Normal || m.open ? state_.selection.character : m.pending.character;
    const auto* c = party_.character(caster);
    if (!session_ || !c) {
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
    const auto* cls = party_.catalog().characterClass(c->build.characterClass);
    m.spellcaster = cls->casting != rules::Casting::None || c->build.spells.racial;
    static_assert(std::tuple_size<decltype(cls->spells)>::value + 6 <= MagicHud::capacity,
                  "Picker must hold all supported spells and features");
    const auto entry = [&](rules::SpellId spell, rules::Feature feature, const char* name, const char* desc,
                           int cost) {
        auto command = state_.selection;
        command.targetCount = 0;
        command.spell = spell;
        command.feature = feature;
        command.action = feature == rules::Feature::None ? PartyAction::Spell : PartyAction::Feature;
        const auto reason = session_->combat().validate(command, session_->movement().player);
        const int charges = feature == rules::Feature::SecondWind       ? int(c->secondWind)
                            : feature == rules::Feature::ActionSurge    ? int(c->actionSurge)
                            : feature == rules::Feature::PreserveLife   ? int(c->channelDivinity)
                            : feature == rules::Feature::ArcaneRecovery ? int(c->arcaneRecovery)
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
    for (std::uint16_t i = 0; i < party_.catalog().spellCount(); ++i) {
        const auto access = party_.spellAccess(caster, {i});
        if (!access)
            continue;
        const auto* s = party_.catalog().spell({i});
        entry({i}, rules::Feature::None, s->name.c_str(), s->description.c_str(), access.cost);
        auto& row = m.entries[m.count - 1];
        row.source = access.source;
        row.casting = access.casting;
        row.attack = access.attack;
        row.difficulty = access.difficulty;
    }
    if (party_.hasFeature(caster, rules::ClassFeature::SecondWind))
        entry({}, rules::Feature::SecondWind, "Second Wind",
              "Heal yourself for 1d10 + level. One action; refreshes on a Short or Long Rest.", 0);
    if (party_.hasFeature(caster, rules::ClassFeature::ArcaneRecovery))
        entry({}, rules::Feature::ArcaneRecovery, "Arcane Recovery",
              "Recover 2 mana at levels 1-2, 4 at level 3. Refreshes on Long Rest. Requires "
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
                  "Heal a pool of 5 x level, lowest HP percentage first; capped at half maximum HP. One "
                  "action. Refreshes on rest.",
                  0);
        if (cls->progression == rules::ProgressionClass::Rogue)
            entry({}, rules::Feature::CunningAction, "Cunning Action / Defend",
                  c->build.level >= 3
                      ? "Defend freely once per turn. Thief may use this allowance for a potion instead."
                      : "Defend freely once per turn, before or after your normal action.",
                  0);
    }
    if (session_->effects().has(caster, EffectKind::Restrained))
        entry({}, rules::Feature::EscapeWeb, "Escape Web",
              "One action: Strength check against Web DC to end restraint.", 0);
    if (session_->effects().spiritualWeapon(caster)) {
        rules::SpellId id;
        for (std::uint16_t i = 0; i < party_.catalog().spellCount(); ++i)
            if (party_.catalog().spell({i})->kind == rules::SpellKind::SpiritualWeapon)
                id = {i};
        entry(id, rules::Feature::SpiritualWeapon, "Command Spiritual Weapon",
              "Free once per turn: move up to two clear floor steps and optionally attack an enemy in its "
              "destination. Click a floor cell or an enemy, then confirm.",
              0);
    }
    if (session_->effects().concentration(caster).serial)
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
    if (cls->casting == rules::Casting::None) {
        if (c->build.defense)
            passive(PassiveFeature::Defense, "Defense",
                    "+1 Armor Class while wearing body armor. Already included in your armor class.");
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
    if (m.pending.feature == rules::Feature::SpiritualWeapon) {
        m.kind = rules::SpellKind::SpiritualWeapon;
        m.hostile = true;
        m.ground = true;
        m.maximum = 1;
    }
    if (m.state == CastingState::Resolving)
        return;
    if (m.area) {
        auto aimed = m.pending;
        if (m.hoverCell)
            aimed.aimedCell = m.hoverCell;
        m.footprint = session_->combat().area(aimed, session_->movement().player);
        m.cells = m.footprint.eligible;
        m.affected = m.footprint.actors;
        m.affectedCount = m.footprint.count;
        const auto cells = std::count(m.cells.begin(), m.cells.end(), true);
        const auto allies = std::count(m.footprint.allies.begin(), m.footprint.allies.end(), true);
        for (std::size_t i = 0; i < rules::partySize; ++i)
            m.selectedAllies[i] = m.footprint.allies[party_.snapshot().formation[i].index];
        std::snprintf(m.targets.data(), m.targets.size(), "%d cells / %zu enemies / %d allies%s", int(cells),
                      m.affectedCount, int(allies), m.ground && !m.pending.aimedCell ? " / aim first" : "");
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
    } else if (m.pending.action == PartyAction::Spell && !m.self) {
        for (std::size_t i = 0; i < rules::partySize; ++i) {
            auto one = m.pending;
            one.targetCount = 1;
            one.allies[0] = party_.snapshot().formation[i];
            m.allyReasons[i] = session_->combat().spellRecipient(one, session_->movement().player, 0);
            for (std::size_t j = 0; j < m.pending.targetCount; ++j)
                m.selectedAllies[i] |= one.allies[0].index == m.pending.allies[j].index;
        }
        std::snprintf(m.targets.data(), m.targets.size(), "Allies selected %zu/%u", m.pending.targetCount,
                      m.maximum);
    } else if (m.pending.feature == rules::Feature::PreserveLife) {
        const auto pool = party_.preserveLifePreview(caster);
        std::snprintf(m.targets.data(), m.targets.size(), "Healing: %d / %d / %d / %d HP (character order)",
                      pool[0], pool[1], pool[2], pool[3]);
    } else
        std::snprintf(m.targets.data(), m.targets.size(), "Self / confirm to use");
}
void CommandController::setCastingState(CastingState next) noexcept {
    auto& m = state_.magic;
    if (!state_.logExpanded) {
        const int before = m.casting() ? hudLayout::castingLogColumns : hudLayout::logColumns;
        const int after = next == CastingState::Targeting || next == CastingState::Resolving
                              ? hudLayout::castingLogColumns
                              : hudLayout::logColumns;
        state_.logOffset = state_.log.rewrapOffset(state_.logOffset, before, after);
    }
    m.state = next;
    m.open = next == CastingState::SpellPicker;
    m.armed = next == CastingState::Targeting;
}
void CommandController::magicFeedback(const char* text) noexcept {
    std::snprintf(state_.magic.feedback.data(), state_.magic.feedback.size(), "%s", text);
}
void CommandController::openSpellbook() noexcept {
    if (!session_ || state_.suspended || state_.magic.state == CastingState::Resolving)
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
    m.hoverCell.reset();
    m.pending.action = entry.feature == rules::Feature::None ? PartyAction::Spell : PartyAction::Feature;
    m.pending.spell = entry.spell;
    m.pending.feature = entry.feature;
    magicFeedback("");
    setCastingState(CastingState::Targeting);
    state_.hover = {};
    refreshMagic();
    updateAvailability();
}
bool CommandController::magicSingleTarget() const noexcept {
    return state_.magic.armed && state_.magic.single;
}
CommandReason CommandController::magicEligibility(ActorId id) const noexcept {
    const auto& m = state_.magic;
    if (!session_ || !m.armed || !m.hostile || m.area)
        return CommandReason::InvalidTarget;
    if (!visible(id))
        return CommandReason::NotVisible;
    auto c = m.pending;
    std::size_t index = 0;
    if (m.kind == rules::SpellKind::AcidSplash && c.targetCount && !sameActor(c.enemies[0], id))
        index = 1;
    c.enemies[index] = id;
    return session_->combat().spellRecipient(c, session_->movement().player, index);
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
        c.action != PartyAction::Spell)
        return;
    const auto id = party_.snapshot().formation[formation];
    c.ally = id;
    magicFeedback("");
    for (std::size_t i = 0; i < c.targetCount; ++i)
        if (c.allies[i].index == id.index && !m.single) {
            removeMagicRecipient(i);
            return;
        }
    if (m.allyReasons[formation] != CommandReason::Ready) {
        magicFeedback(commandMessage(m.allyReasons[formation]));
        return;
    }
    if (m.single) {
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
    if (m.ground && session_) {
        std::array<Cell, 169> cells{};
        std::size_t count = 0, index = 0;
        const auto pose = session_->movement().player;
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
    if (m.hostile && state_.targets.count) {
        std::size_t index = delta < 0 ? 0 : state_.targets.count - 1;
        for (std::size_t i = 0; i < state_.targets.count; ++i)
            if (m.pending.target && sameActor(*m.pending.target, state_.targets.ids[i]))
                index = i;
        index = (index + state_.targets.count + (delta < 0 ? -1 : 1)) % state_.targets.count;
        m.pending.target = state_.targets.ids[index];
    } else if (!m.hostile) {
        unsigned index = delta < 0 ? 0 : 3;
        for (unsigned i = 0; i < rules::partySize; ++i)
            if (m.pending.ally && party_.snapshot().formation[i].index == m.pending.ally->index)
                index = i;
        index = (index + (delta < 0 ? 3 : 1)) % rules::partySize;
        m.pending.ally = party_.snapshot().formation[index];
    }
}
void CommandController::magicGround(std::optional<Cell> cell, bool select) noexcept {
    auto& m = state_.magic;
    if (!session_ || !m.armed || !m.ground || state_.suspended)
        return;
    if (cell && !spellCell(actors_.map(), session_->movement().player, *cell))
        cell.reset();
    m.hoverCell = cell;
    if (select && cell) {
        m.pending.aimedCell = cell;
        m.hoverCell.reset();
    }
    refreshMagic();
    updateAvailability();
}
SpellArea CommandController::floorPreview() const noexcept {
    SpellArea result;
    const auto& m = state_.magic;
    if (!session_)
        return result;
    if (m.armed && m.area && !state_.modal() && !session_->movement().transition.active()) {
        auto aimed = m.pending;
        if (m.hoverCell)
            aimed.aimedCell = m.hoverCell;
        result = session_->combat().area(aimed, session_->movement().player);
        result.kind = m.kind;
    }
    result.ground = session_->effects().ground();
    return result;
}
void CommandController::reconcileMagic() noexcept {
    auto& m = state_.magic;
    if (!session_ || !m.casting())
        return;
    if (m.state == CastingState::Resolving) {
        if (!session_->combat().casterResolving(m.pending.character))
            cancel();
        return;
    }
    if (m.pending.world != state_.combat.world || m.pending.turn != state_.combat.turn ||
        m.pending.character.index != state_.selection.character.index ||
        !session_->effects().canAct(m.pending.character)) {
        cancel();
        return;
    }
    if (state_.suspended)
        return;
    for (std::size_t i = 0; i < m.pending.targetCount;) {
        const auto reason = session_->combat().spellRecipient(m.pending, session_->movement().player, i);
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
        if (m.single) {
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
