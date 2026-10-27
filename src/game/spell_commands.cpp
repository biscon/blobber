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
    m.open = m.state == CastingState::SpellPicker;
    m.armed = m.state == CastingState::Targeting;
    m.kind = rules::SpellKind::Count;
    m.assigned = m.maximum = 0;
    m.single = false;
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
    const auto entry = [&](rules::SpellId spell, rules::Feature feature, const char* name, const char* desc,
                           int cost) {
        auto command = state_.selection;
        command.targetCount = 0;
        command.spell = spell;
        command.feature = feature;
        command.action = feature == rules::Feature::None ? PartyAction::Spell : PartyAction::Feature;
        const auto reason = session_->combat().validate(command, session_->movement().player);
        const int charges = feature == rules::Feature::SecondWind       ? int(c->secondWind)
                            : feature == rules::Feature::ArcaneRecovery ? int(c->arcaneRecovery)
                                                                        : -1;
        const auto* definition = party_.catalog().spell(spell);
        m.entries[m.count++] = {spell,  feature, name,
                                desc,   cost,    definition ? definition->kind : rules::SpellKind::Count,
                                reason, charges};
    };
    for (std::size_t i = 0; i < cls->spellCount; ++i) {
        const auto* s = party_.catalog().spell(cls->spells[i]);
        entry(cls->spells[i], rules::Feature::None, s->name.c_str(), s->description.c_str(),
              rules::spellRules(s->kind)->level * 2);
    }
    if (party_.hasFeature(caster, rules::ClassFeature::SecondWind))
        entry({}, rules::Feature::SecondWind, "Second Wind",
              "Heal yourself for 1d10 + level. One action; one use per run.", 0);
    if (party_.hasFeature(caster, rules::ClassFeature::ArcaneRecovery))
        entry({}, rules::Feature::ArcaneRecovery, "Arcane Recovery",
              "Recover 2 mana at level one. One use per run. Requires "
              "exploration without active threats.",
              0);
    if (session_->effects().concentration(caster).serial)
        entry({}, rules::Feature::DismissConcentration, "End concentration",
              "End your concentration voluntarily. No action, mana or cooldown cost.", 0);
    if (m.count) {
        m.cursor = std::min(m.cursor, m.count - 1);
        m.offset = std::min(m.offset, m.count > 7 ? m.count - 7 : 0);
    } else
        m.cursor = m.offset = 0;
    if (!m.casting())
        return;
    for (std::size_t i = 0; i < m.count; ++i) {
        const auto& e = m.entries[i];
        if (state_.magic.pending.action == PartyAction::Spell
                ? e.spell.index == state_.magic.pending.spell.index && e.feature == rules::Feature::None
                : e.feature == state_.magic.pending.feature) {
            m.name = e.name;
            m.description = e.description;
            m.cost = e.cost;
        }
    }
    const auto* s = party_.catalog().spell(state_.magic.pending.spell);
    const auto* r = s ? rules::spellRules(s->kind) : nullptr;
    if (state_.magic.pending.action == PartyAction::Spell && r) {
        m.kind = s->kind;
        m.single = !r->area && r->maxTargets == 1;
        m.maximum = r->maxTargets;
        m.hostile = r->hostile;
        m.area = r->area;
        m.missiles = s->kind == rules::SpellKind::MagicMissile;
    }
    if (m.state == CastingState::Resolving)
        return;
    if (m.area) {
        m.footprint = spellArea(actors_, session_->movement().player);
        m.cells = m.footprint.eligible;
        m.affected = m.footprint.actors;
        m.affectedCount = m.footprint.count;
        const auto cells = std::count(m.cells.begin(), m.cells.end(), true);
        std::snprintf(m.targets.data(), m.targets.size(), "%d front cells / %zu %s / No friendly fire",
                      int(cells), m.affectedCount, m.affectedCount == 1 ? "enemy" : "enemies");
    } else if (m.hostile) {
        for (std::size_t i = 0; i < m.pending.targetCount; ++i) {
            m.affected[m.affectedCount++] = m.pending.enemies[i];
            m.assigned += m.pending.darts[i];
        }
        std::snprintf(m.targets.data(), m.targets.size(),
                      m.missiles ? "Darts assigned %u/3" : "Enemies selected %u/%u",
                      m.missiles ? m.assigned : unsigned(m.pending.targetCount), m.maximum);
    } else if (m.pending.action == PartyAction::Spell) {
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
    if (!state_.magic.count &&
        !party_.hasFeature(state_.selection.character, rules::ClassFeature::SneakAttack))
        return;
    auto& m = state_.magic;
    m.cursor = bookCursors_[state_.selection.character.index];
    m.offset = bookOffsets_[state_.selection.character.index];
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
void CommandController::browseSpellbook(int delta) noexcept {
    auto& m = state_.magic;
    if (!m.count)
        return;
    m.cursor = static_cast<std::size_t>(std::clamp<int>(int(m.cursor) + delta, 0, int(m.count) - 1));
    if (m.cursor < m.offset)
        m.offset = m.cursor;
    if (m.cursor >= m.offset + 7)
        m.offset = m.cursor - 6;
    bookCursors_[state_.selection.character.index] = m.cursor;
    bookOffsets_[state_.selection.character.index] = m.offset;
}
void CommandController::scrollSpellbook(int delta) noexcept {
    auto& m = state_.magic;
    m.offset = std::size_t(std::clamp<int>(int(m.offset) + delta, 0, std::max(0, int(m.count) - 7)));
    m.cursor = std::clamp(m.cursor, m.offset, std::min(m.count ? m.count - 1 : 0, m.offset + 6));
    bookCursors_[state_.selection.character.index] = m.cursor;
    bookOffsets_[state_.selection.character.index] = m.offset;
}
void CommandController::chooseSpellbook(std::size_t index) noexcept {
    auto& m = state_.magic;
    if (!m.open || state_.suspended || index >= m.count)
        return;
    browseSpellbook(int(index) - int(m.cursor));
    const auto entry = m.entries[index];
    if (entry.reason != CommandReason::Ready && entry.reason != CommandReason::MissingTarget) {
        magicFeedback(commandMessage(entry.reason));
        return;
    }
    m.pending = state_.selection;
    m.pending.target.reset();
    m.pending.ally.reset();
    m.pending.targetCount = 0;
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
        if (m.missiles)
            ++c.darts[selected];
    }
    refreshMagic();
    updateAvailability();
}
void CommandController::magicAlly(std::size_t formation) noexcept {
    auto& m = state_.magic;
    auto& c = m.pending;
    if (!m.armed || m.hostile || state_.suspended || formation >= rules::partySize ||
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
SpellArea CommandController::floorPreview() const noexcept {
    const auto& m = state_.magic;
    if (!session_ || !m.armed || !m.area || state_.modal() || session_->movement().transition.active())
        return {};
    auto result = spellArea(actors_, session_->movement().player);
    result.kind = m.kind;
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
