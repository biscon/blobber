#include "game/commands.hpp"
#include "game/perception.hpp"
#include <algorithm>
#include <cstdio>
namespace blobber {
void CommandController::refreshMagic() noexcept {
    auto& m = state_.magic;
    const auto* c = party_.character(state_.selection.character);
    if (!session_ || !c) {
        m = {};
        return;
    }
    m.count = 0;
    m.name = "";
    m.description = "";
    m.cost = 0;
    m.armed =
        state_.selection.action == PartyAction::Spell || state_.selection.action == PartyAction::Feature;
    m.hostile = m.area = m.missiles = false;
    m.affectedCount = 0;
    m.cells = {};
    m.targets = {};
    m.mana = c->mana;
    m.maxMana = c->maxMana;
    m.attackBonus = c->stats.proficiency + party_.castingModifier(state_.selection.character);
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
        m.entries[m.count++] = {spell, feature, name, desc, cost, reason, charges};
    };
    for (std::size_t i = 0; i < cls->spellCount; ++i) {
        const auto* s = party_.catalog().spell(cls->spells[i]);
        entry(cls->spells[i], rules::Feature::None, s->name.c_str(), s->description.c_str(),
              rules::spellRules(s->kind)->level * 2);
    }
    if (party_.hasFeature(state_.selection.character, rules::ClassFeature::SecondWind))
        entry({}, rules::Feature::SecondWind, "Second Wind",
              "Heal yourself for 1d10 + level. One action; one use per run.", 0);
    if (party_.hasFeature(state_.selection.character, rules::ClassFeature::ArcaneRecovery))
        entry({}, rules::Feature::ArcaneRecovery, "Arcane Recovery",
              "Recover 2 mana at level one. One use per run. Requires "
              "exploration without active threats.",
              0);
    if (session_->effects().concentration(state_.selection.character).serial)
        entry({}, rules::Feature::DismissConcentration, "End concentration",
              "End your concentration voluntarily. No action, mana or cooldown cost.", 0);
    if (m.count) {
        m.cursor = std::min(m.cursor, m.count - 1);
        m.offset = std::min(m.offset, m.count > 10 ? m.count - 10 : 0);
    } else
        m.cursor = m.offset = 0;
    if (!m.armed)
        return;
    for (std::size_t i = 0; i < m.count; ++i) {
        const auto& e = m.entries[i];
        if (state_.selection.action == PartyAction::Spell
                ? e.spell.index == state_.selection.spell.index && e.feature == rules::Feature::None
                : e.feature == state_.selection.feature) {
            m.name = e.name;
            m.description = e.description;
            m.cost = e.cost;
        }
    }
    const auto* s = party_.catalog().spell(state_.selection.spell);
    const auto* r = s ? rules::spellRules(s->kind) : nullptr;
    if (state_.selection.action == PartyAction::Spell && r) {
        m.hostile = r->hostile;
        m.area = r->area;
        m.missiles = s->kind == rules::SpellKind::MagicMissile;
    }
    std::size_t used = 0;
    if (m.area) {
        const auto pose = session_->movement().player;
        for (int side = -1; side <= 1; ++side) {
            const auto cell = pose.cell + forward(pose.heading) + right(pose.heading) * side;
            m.cells[side + 1] = inMeleeFront(actors_.map(), pose.cell, pose.heading, cell);
        }
        for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
            const auto* a = actors_.actor(actors_.id(i));
            if (a && a->life == ActorLife::Alive &&
                inMeleeFront(actors_.map(), pose.cell, pose.heading, a->npc.cell))
                m.affected[m.affectedCount++] = actors_.id(i);
        }
        std::snprintf(m.targets.data(), m.targets.size(),
                      "Three front cells / %zu enemies / no friendly fire", m.affectedCount);
    } else {
        for (std::size_t i = 0; i < state_.selection.targetCount; ++i) {
            if (m.hostile) {
                used += std::snprintf(m.targets.data() + used, m.targets.size() - used, "%s#%u%s",
                                      i ? ", " : "", unsigned(state_.selection.enemies[i].slot) + 1,
                                      m.missiles ? " darts:" : "");
                if (m.missiles)
                    used += std::snprintf(m.targets.data() + used, m.targets.size() - used, "%u",
                                          unsigned(state_.selection.darts[i]));
                m.affected[m.affectedCount++] = state_.selection.enemies[i];
            } else {
                const auto* ally = party_.character(state_.selection.allies[i]);
                used += std::snprintf(m.targets.data() + used, m.targets.size() - used, "%s%.40s",
                                      i ? ", " : "", ally ? ally->build.name.c_str() : "?");
            }
        }
        if (!state_.selection.targetCount)
            std::snprintf(m.targets.data(), m.targets.size(), "%s",
                          state_.selection.action == PartyAction::Feature ? "Self / confirm to use"
                          : m.hostile                                     ? "Select visible enemies"
                                                                          : "Select ally portraits");
    }
}
void CommandController::openSpellbook() noexcept {
    if (!session_ || state_.suspended)
        return;
    closeOverlay();
    cancel();
    refreshMagic();
    if (!state_.magic.count &&
        !party_.hasFeature(state_.selection.character, rules::ClassFeature::SneakAttack))
        return;
    state_.magic.open = true;
    state_.hover = {};
    updateAvailability();
}
void CommandController::browseSpellbook(int delta) noexcept {
    auto& m = state_.magic;
    if (!m.count)
        return;
    m.cursor = static_cast<std::size_t>(std::clamp<int>(int(m.cursor) + delta, 0, int(m.count) - 1));
    if (m.cursor < m.offset)
        m.offset = m.cursor;
    if (m.cursor >= m.offset + 10)
        m.offset = m.cursor - 9;
}
void CommandController::chooseSpellbook(std::size_t index) noexcept {
    if (!state_.magic.open || index >= state_.magic.count)
        return;
    const auto entry = state_.magic.entries[index];
    closeOverlay();
    cancel();
    state_.selection.action =
        entry.feature == rules::Feature::None ? PartyAction::Spell : PartyAction::Feature;
    state_.selection.spell = entry.spell;
    state_.selection.feature = entry.feature;
    refreshMagic();
    updateAvailability();
}
bool CommandController::magicSingleTarget() const noexcept {
    const auto* s = party_.catalog().spell(state_.selection.spell);
    const auto* r = s ? rules::spellRules(s->kind) : nullptr;
    return state_.selection.action == PartyAction::Spell && r && !r->area && r->maxTargets == 1;
}
void CommandController::magicEnemy(ActorId id) noexcept {
    auto& c = state_.selection;
    if (!state_.magic.armed || !state_.magic.hostile || state_.magic.area || state_.suspended || !visible(id))
        return;
    const auto* s = party_.catalog().spell(c.spell);
    if (!s)
        return;
    const auto& r = *rules::spellRules(s->kind);
    c.target = id;
    std::size_t selected = c.targetCount;
    for (std::size_t i = 0; i < c.targetCount; ++i)
        if (sameActor(c.enemies[i], id))
            selected = i;
    if (state_.magic.missiles) {
        if (!c.targetCount) {
            c.enemies[0] = id;
            c.darts[0] = 3;
            c.targetCount = 1;
        } else {
            std::size_t donor = c.targetCount;
            for (std::size_t i = 0; i < c.targetCount; ++i)
                if (i != selected && (donor == c.targetCount || c.darts[i] > c.darts[donor]))
                    donor = i;
            if (donor < c.targetCount && (selected < c.targetCount || c.targetCount < 3)) {
                if (selected == c.targetCount) {
                    c.enemies[selected] = id;
                    c.darts[selected] = 0;
                    ++c.targetCount;
                }
                ++c.darts[selected];
                if (!--c.darts[donor]) {
                    for (std::size_t j = donor + 1; j < c.targetCount; ++j) {
                        c.enemies[j - 1] = c.enemies[j];
                        c.darts[j - 1] = c.darts[j];
                    }
                    --c.targetCount;
                }
            }
        }
    } else if (r.maxTargets == 1) {
        c.enemies[0] = id;
        c.targetCount = 1;
    } else if (selected < c.targetCount) {
        for (std::size_t j = selected + 1; j < c.targetCount; ++j)
            c.enemies[j - 1] = c.enemies[j];
        --c.targetCount;
    } else if (c.targetCount < r.maxTargets)
        c.enemies[c.targetCount++] = id;
    refreshMagic();
    updateAvailability();
}
void CommandController::magicAlly(std::size_t formation) noexcept {
    auto& c = state_.selection;
    if (!state_.magic.armed || state_.magic.hostile || state_.suspended || formation >= rules::partySize)
        return;
    const auto* s = party_.catalog().spell(c.spell);
    if (!s)
        return;
    const auto& r = *rules::spellRules(s->kind);
    const auto id = party_.snapshot().formation[formation];
    c.ally = id;
    std::size_t selected = c.targetCount;
    for (std::size_t i = 0; i < c.targetCount; ++i)
        if (c.allies[i].index == id.index)
            selected = i;
    if (r.maxTargets == 1) {
        c.allies[0] = id;
        c.targetCount = 1;
    } else if (selected < c.targetCount) {
        for (std::size_t j = selected + 1; j < c.targetCount; ++j)
            c.allies[j - 1] = c.allies[j];
        --c.targetCount;
    } else if (c.targetCount < r.maxTargets)
        c.allies[c.targetCount++] = id;
    refreshMagic();
    updateAvailability();
}
void CommandController::magicKeyboardTarget(int delta) noexcept {
    if (state_.magic.hostile)
        cycleTarget(delta);
    else {
        unsigned index = 0;
        if (state_.selection.ally)
            for (unsigned i = 0; i < rules::partySize; ++i)
                if (party_.snapshot().formation[i].index == state_.selection.ally->index)
                    index = i;
        index = (index + (delta < 0 ? 3 : 1)) % rules::partySize;
        state_.selection.ally = party_.snapshot().formation[index];
    }
}
} // namespace blobber
