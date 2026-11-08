#include "game/commands.hpp"
#include "game/arena.hpp"
#include "game/campaign.hpp"
#include "game/perception.hpp"
#include "game/target_visibility.hpp"
#include "ui/layout.hpp"
#include "ui/player_text.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace blobber {
const char* actionName(PartyAction action) noexcept {
    constexpr const char* names[]{"Melee", "Back-row melee", "Defend", "Ranged",
                                  "Spell", "Item",           "Aid",    "Ability"};
    const auto i = static_cast<unsigned>(action);
    return i < static_cast<unsigned>(PartyAction::Count) ? names[i] : "Invalid action";
}
const char* commandMessage(CommandReason reason) noexcept {
    switch (reason) {
    case CommandReason::NoAmmunition:
        return "No compatible ammunition in this bag.";
    case CommandReason::InvalidItem:
        return "Select a usable potion from this character\'s bag.";
    case CommandReason::StaleInventory:
        return "Inventory changed; select the potion again.";
    case CommandReason::DeadAlly:
        return "Requires a living ally.";
    case CommandReason::Armored:
        return "Requires an unarmored ally.";
    case CommandReason::MustBeDying:
        return "Requires a dying ally.";
    case CommandReason::TargetsTooFar:
        return "Targets must be within 1 cell of each other.";
    case CommandReason::InsufficientMana:
        return "Not enough mana.";
    case CommandReason::NoCharges:
        return "No uses remaining this run.";
    case CommandReason::InvalidSpell:
        return "Spell is not in this character's spellbook.";
    case CommandReason::InvalidTargets:
        return "Invalid target selection or dart assignment.";
    case CommandReason::FullHealth:
        return "Target is already at full health.";
    case CommandReason::FullMana:
        return "Mana is already full.";
    case CommandReason::Unsafe:
        return "Wait until it is safe to recover.";
    case CommandReason::EffectCapacity:
        return "Too many spells are active. Wait for one to end.";
    case CommandReason::AttackPending:
        return "This attack is still playing.";
    case CommandReason::Cooldown:
        return "Action is recharging.";
    case CommandReason::OutOfRange:
        return "Target is out of range for this attack.";
    case CommandReason::Blocked:
        return "An obstacle blocks that action.";
    case CommandReason::Ready:
        return "Ready: spends this turn's action.";
    case CommandReason::Resolved:
        return "Action resolved.";
    case CommandReason::NoEncounter:
        return "Face adjacent enemies to fight.";
    case CommandReason::NotYourTurn:
        return "Wait for this character's turn.";
    case CommandReason::StaleTurn:
        return "That turn has already ended.";
    case CommandReason::FrontProtected:
        return "An enemy in front protects this target.";
    case CommandReason::RearBlocked:
        return "A companion in front blocks this melee attack.";
    case CommandReason::NoWeapon:
        return "No ready weapon for this action.";
    case CommandReason::InvalidAlly:
        return "Select another dying ally.";
    case CommandReason::LogOpen:
        return "Close the overlay to act.";
    case CommandReason::FeatureRequired:
        return "Requires the Rogue back-row feature.";
    case CommandReason::InvalidCharacter:
        return "Invalid character.";
    case CommandReason::InvalidAction:
        return "Invalid action.";
    case CommandReason::Unavailable:
        return "This action is not available yet.";
    case CommandReason::BackRowOnly:
        return "Select a back-row character.";
    case CommandReason::MissingTarget:
        return "Select a visible target.";
    case CommandReason::InvalidTarget:
        return "Target is gone or cannot act.";
    case CommandReason::NotVisible:
        return "Target is no longer visible.";
    case CommandReason::Moving:
        return "Wait for the current transition to finish.";
    case CommandReason::MapOpen:
        return "Close the map to target or confirm.";
    case CommandReason::Unfocused:
        return "Input paused while unfocused.";
    case CommandReason::CannotAct:
        return "Character is not conscious.";
    }
    return "Invalid request.";
}
void CombatLog::append(const char* message) noexcept {
    std::snprintf(entries_[next_].data(), messageBytes, "%s", message ? message : "");
    next_ = (next_ + 1) % capacity;
    count_ = std::min(count_ + 1, capacity);
}
const char* CombatLog::newest(std::size_t offset) const noexcept {
    return offset < count_ ? entries_[(next_ + capacity - 1 - offset) % capacity].data() : "";
}
CommandController::CommandController(GameSession& session, const std::vector<NpcTimings>& timings)
    : session_(session), party_(session.party()), actors_(session.actors()) {
    state_.selection.character = party_.snapshot().formation[0];
    if (!timings.empty() && timings.size() != actors_.snapshot().typeCount)
        throw std::invalid_argument("HUD animation/world type counts differ");
    for (const auto& timing : timings)
        validateNpcTimings(timing);
    std::copy(timings.begin(), timings.end(), dialogueTimings_.begin());
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i) {
        const auto& a = actors_.snapshot().actors[i];
        if (!a.hostile())
            continue;
        const auto& species = actors_.catalog().monster(a.monster)->id;
        state_.enemyPortraits[i] = species == "skeleton"        ? EnemyPortrait::Skeleton
                                   : species == "bugbear"       ? EnemyPortrait::Bugbear
                                   : species == "giant.rat"     ? EnemyPortrait::GiantRat
                                   : species == "zombie"        ? EnemyPortrait::Zombie
                                   : species == "goblin_shaman" ? EnemyPortrait::Shaman
                                                                : EnemyPortrait::Goblin;
    }
    state_.log = {};
    appendLog("Click an enemy to attack. Shield guards; Aid stabilizes.");
    refresh({}, false, false, true);
}
void CommandController::record(const CombatOutcome& out) noexcept {
    if (out.event == CombatEvent::None ||
        (out.attackId.world && out.attackId.world != actors_.snapshot().world))
        return;
    shake_.record(out, session_.timeMs(), actors_.snapshot().world);
    state_.worldTransform = shake_.sample(session_.timeMs(), actors_.snapshot().world);
    char line[CombatLog::messageBytes];
    const auto* actor = party_.character(out.character);
    const auto* recipient = party_.character(out.recipient);
    const char* name = actor ? actor->build.name.c_str() : "?";
    const char* target = recipient ? recipient->build.name.c_str() : "?";
    char species[rules::Catalog::maxIdBytes + 1] = "Monster";
    if (const auto* enemy = actors_.actor(out.enemy)) {
        std::snprintf(species, sizeof(species), "%s",
                      monsterName(actors_.catalog().monster(enemy->monster)->id));
    }
    const char* pack = !out.enemyAttack || !out.packTactics ? ""
                       : out.attackRollMode == rules::RollMode::Advantage
                           ? " Pack Tactics: advantage."
                           : " Pack Tactics: canceled by disadvantage.";
    switch (out.event) {
    case CombatEvent::Started:
        std::snprintf(line, sizeof(line),
                      out.resumedRound ? "Melee resumes. Continuing the round." : "Combat begins.");
        break;
    case CombatEvent::AttackStarted:
    case CombatEvent::ProjectileReleased:
        updateHover();
        return;
    case CombatEvent::AttackCanceled:
        std::snprintf(line, sizeof(line), "Attack canceled: %s.",
                      out.targetGone ? "target gone" : "attacker incapacitated");
        break;
    case CombatEvent::Attack:
        if (out.targetGone) {
            std::snprintf(line, sizeof(line), "Arrow arrives: target gone.");
            break;
        }
        recordFeedback(out);
        if (out.mirror.effect.serial) {
            std::snprintf(line, sizeof(line), "%s: attack redirected; duplicate %s.%s", target,
                          out.hit ? "destroyed" : "missed", pack);
            break;
        }
        if (out.enemyAttack && out.spell != rules::SpellKind::Count) {
            std::snprintf(line, sizeof(line), "%s #%u -> %s: %s, %lld damage%s.", species,
                          unsigned(out.enemy.slot) + 1, target, rules::spellRules(out.spell)->name,
                          static_cast<long long>(out.damage), out.died ? ", dead" : "");
            break;
        }
        if (out.damageAdjustment == rules::DamageAdjustment::Immune) {
            std::snprintf(line, sizeof(line), "%s #%u: immune to %s.", species, unsigned(out.enemy.slot) + 1,
                          rules::damageTypeName(out.damageType));
            break;
        }
        if (out.enemyAttack) {
            std::snprintf(line, sizeof(line), "%s #%u -> %s: %s (d20 %d), %lld damage%s.%s", species,
                          unsigned(out.enemy.slot) + 1, target,
                          out.critical ? "critical"
                          : out.hit    ? "hit"
                                       : "miss",
                          out.roll, static_cast<long long>(out.damage), out.died ? ", dead" : "", pack);
        } else
            std::snprintf(line, sizeof(line), "%s -> %s #%u: %s (d20 %d), %lld damage%s.", name, species,
                          unsigned(out.enemy.slot) + 1,
                          out.critical ? "critical"
                          : out.hit    ? "hit"
                                       : "miss",
                          out.roll, static_cast<long long>(out.damage), out.died ? ", dead" : "");
        if (out.damageAdjustment == rules::DamageAdjustment::Vulnerable) {
            appendLog(line);
            std::snprintf(line, sizeof(line), "%s #%u: %s vulnerability doubles damage.", species,
                          unsigned(out.enemy.slot) + 1, rules::damageTypeName(out.damageType));
        }
        if (out.fortitude.status != FortitudeStatus::None) {
            appendLog(line);
            const auto& f = out.fortitude;
            if (f.status == FortitudeStatus::Saved || f.status == FortitudeStatus::Failed)
                std::snprintf(line, sizeof(line), "%s #%u: Undead Fortitude %s (Con %lld vs DC %lld)%s.",
                              species, unsigned(out.enemy.slot) + 1,
                              f.status == FortitudeStatus::Saved ? "succeeds" : "fails",
                              static_cast<long long>(f.total), static_cast<long long>(f.difficulty),
                              f.status == FortitudeStatus::Saved ? ": remains at 1 HP" : ": dead");
            else
                std::snprintf(line, sizeof(line), "%s #%u: %s bypasses Undead Fortitude.", species,
                              unsigned(out.enemy.slot) + 1,
                              f.status == FortitudeStatus::RadiantBypass ? "radiant damage" : "critical hit");
        }
        break;
    case CombatEvent::Defend:
        std::snprintf(line, sizeof(line), "%s defends until their next turn.", name);
        break;
    case CombatEvent::Aid:
        if (!out.roll && out.spell != rules::SpellKind::SpareTheDying) {
            std::snprintf(line, sizeof(line), "%s wakes %s.", name, target);
            break;
        }
        std::snprintf(line, sizeof(line), "%s aids %s: %s (d20 %d, Wisdom vs DC 10).", name, target,
                      out.hit ? "stabilized" : "failed", out.roll);
        break;
    case CombatEvent::DeathSave:
        std::snprintf(line, sizeof(line), "%s: death save %d - %s (%u success / %u failure).", name, out.roll,
                      actor && actor->life == rules::Life::Conscious ? "regained 1 HP"
                      : actor && actor->life == rules::Life::Stable  ? "stable"
                      : out.died                                     ? "dead"
                                                                     : "dying",
                      actor ? unsigned(actor->successes) : 0, actor ? unsigned(actor->failures) : 0);
        break;
    case CombatEvent::Finished:
        line[0] = '\0';
        break;
    case CombatEvent::Move:
        std::snprintf(line, sizeof(line), "Enemy #%u moves.", unsigned(out.enemy.slot) + 1);
        break;
    case CombatEvent::Wake:
        std::snprintf(line, sizeof(line), "%s #%u wakes ally #%u.", species, unsigned(out.enemy.slot) + 1,
                      unsigned(out.awakened.slot) + 1);
        break;
    case CombatEvent::Dash:
        std::snprintf(line, sizeof(line), "Enemy #%u dashes.", unsigned(out.enemy.slot) + 1);
        break;
    case CombatEvent::Round:
        std::snprintf(line, sizeof(line), "New round.");
        break;
    case CombatEvent::Spent:
        if (out.enemy.world) {
            const char* cause = out.spentCause == SpentCause::Surprise   ? "surprised"
                                : out.spentCause == SpentCause::Cooldown ? "action is recharging"
                                                                         : "no available action";
            std::snprintf(line, sizeof(line), "%s #%u: %s.", species, unsigned(out.enemy.slot) + 1, cause);
        } else {
            const char* cause = out.spentCause == SpentCause::EndTurn    ? "ends their turn"
                                : out.spentCause == SpentCause::Cooldown ? "action is recharging"
                                                                         : "no available action";
            std::snprintf(line, sizeof(line), "%s: %s.", name, cause);
        }
        break;
    case CombatEvent::Disengaged:
        std::snprintf(line, sizeof(line), "Contact broken. Ranged approach resumes.");
        break;
    case CombatEvent::Cast: {
        if (out.enemyAttack) {
            std::snprintf(line, sizeof(line), "%s #%u begins %s.", species, unsigned(out.enemy.slot) + 1,
                          rules::spellRules(out.spell)->name);
            break;
        }
        const auto* spell = rules::spellRules(out.spell);
        std::snprintf(line, sizeof(line), "%s: %s.", name,
                      spell ? spell->name : rules::featureName(out.feature));
        break;
    }
    case CombatEvent::Healing:
    case CombatEvent::Recovery: {
        if (out.feature == rules::Feature::ActionSurge) {
            std::snprintf(line, sizeof(line), "%s gains one extra action.", name);
            break;
        }
        const auto* item = party_.catalog().item(out.item);
        std::snprintf(line, sizeof(line), "%s%s%s -> %s: +%lld %s.", name, item ? " uses " : "",
                      item ? item->name.c_str() : "", target, static_cast<long long>(out.damage),
                      out.spell == rules::SpellKind::FalseLife ? "temporary HP"
                      : out.event == CombatEvent::Healing      ? "HP"
                                                               : "mana");
        if (out.event == CombatEvent::Healing) {
            CombatFeedback f;
            f.active = f.party = f.hit = f.healing = true;
            f.character = out.recipient;
            f.damage = out.damage;
            f.portrait = out.spell == rules::SpellKind::FalseLife ? PortraitFeedbackKind::Protection
                                                                  : PortraitFeedbackKind::Healing;
            f.startedMs = out.occurredMs;
            showFeedback(f);
        }
        break;
    }
    case CombatEvent::SpellSave:
        if (out.feature == rules::Feature::EscapeWeb) {
            std::snprintf(line, sizeof(line), "Web escape: Strength check %d vs DC %d - %s.", out.saveTotal,
                          out.saveDifficulty, out.hit ? "escaped" : "restrained");
            break;
        }
        if (out.enemyAttack) {
            std::snprintf(line, sizeof(line), "%s: %s save %d vs DC %d - %s.", target,
                          rules::spellRules(out.spell)->name, out.saveTotal, out.saveDifficulty,
                          out.hit ? "saved" : "failed");
            break;
        }
        std::snprintf(line, sizeof(line), "Enemy #%u: %s save %d vs DC %d - %s.",
                      unsigned(out.enemy.slot) + 1, rules::spellRules(out.spell)->name, out.saveTotal,
                      out.saveDifficulty, out.hit ? "saved" : "failed");
        break;
    case CombatEvent::GuidingMark:
        std::snprintf(line, sizeof(line), "Enemy #%u: guiding light %s.", unsigned(out.enemy.slot) + 1,
                      out.hit ? "applied" : "ended");
        break;
    case CombatEvent::Effect: {
        const auto& e = out.effect;
        const char* effect = effectName(e.kind);
        char caster[48];
        if (e.caster.npc)
            std::snprintf(caster, sizeof(caster), "Enemy #%u ", unsigned(e.caster.enemy.slot) + 1);
        else
            caster[0] = '\0';
        if (e.notice == EffectNotice::InitialSave || e.notice == EffectNotice::RepeatSave ||
            e.notice == EffectNotice::ConcentrationSave)
            std::snprintf(line, sizeof(line), "%s%s%s: save %lld vs %lld - %s.", caster, effect,
                          e.notice == EffectNotice::ConcentrationSave ? " concentration" : "",
                          static_cast<long long>(e.total), static_cast<long long>(e.difficulty),
                          e.succeeded ? "saved" : "failed");
        else
            std::snprintf(line, sizeof(line), "%s%s: %s.", caster, effect,
                          e.notice == EffectNotice::Applied ? "applied" : "ended / recipient removed");
        if (e.notice == EffectNotice::Applied && e.target.npc && e.caster.npc) {
            appendLog(line);
            std::snprintf(line, sizeof(line), "Enemy #%u receives %s from #%u.",
                          unsigned(e.target.enemy.slot) + 1, effect, unsigned(e.caster.enemy.slot) + 1);
        }
        if (e.notice == EffectNotice::Applied && !e.target.npc) {
            CombatFeedback f;
            f.active = f.party = f.hit = true;
            const bool positive = e.kind == EffectKind::Bless || e.kind == EffectKind::ShieldOfFaith ||
                                  e.kind == EffectKind::MageArmor || e.kind == EffectKind::Guidance ||
                                  e.kind == EffectKind::Resistance || e.kind == EffectKind::Sanctuary ||
                                  e.kind == EffectKind::Aid || e.kind == EffectKind::MirrorImage ||
                                  e.kind == EffectKind::SpiritualWeapon;
            f.portrait = positive ? PortraitFeedbackKind::Protection : PortraitFeedbackKind::Harmful;
            f.character = e.target.character;
            f.startedMs = out.occurredMs;
            f.text = ""; // Names remain in the log and status tooltips.
            showFeedback(f);
        }
        break;
    }
    case CombatEvent::None:
        return;
    }
    if (line[0])
        appendLog(line);
    if (out.phase == CombatPhase::Victory)
        appendLog(state_.arena && arenaCleared(actors_.snapshot())
                      ? "Arena cleared. Rematch restores the starting setup; Edit Setup changes the scenario."
                      : "Victory. Surviving downed allies are stable; HP is not restored.");
    if (out.phase == CombatPhase::Defeat)
        appendLog(session_.campaign() ? "Defeat: this expedition has ended. Return to Main Menu."
                                      : "Defeat: nobody is conscious. Restart begins a fresh session.");
}
void CommandController::recordFeedback(const CombatOutcome& out) noexcept {
    const auto* actor = actors_.actor(out.enemy);
    if ((!out.enemyAttack && !actor) || (out.enemyAttack && !party_.character(out.recipient)))
        return;
    CombatFeedback next;
    next.active = true;
    next.party = out.enemyAttack;
    next.hit = out.hit;
    next.immune = out.damageAdjustment == rules::DamageAdjustment::Immune;
    if (next.immune)
        next.text = "IMMUNE";
    const auto* spell = rules::spellRules(out.spell);
    if (spell && !spell->attack && !out.hit)
        next.text = "SAVED";
    if (out.mirror.effect.serial)
        next.text = out.hit ? "IMAGE LOST" : "IMAGE MISSED";
    if (out.fortitude.status == FortitudeStatus::Saved)
        next.text = "1 HP";
    if (next.party && out.spell != rules::SpellKind::Count) {
        next.portrait =
            out.hit && out.damage > 0 ? PortraitFeedbackKind::Damage : PortraitFeedbackKind::Negated;
        next.element = out.damageType;
    }
    next.character = out.recipient;
    next.enemy = out.enemy;
    next.damage = out.damage;
    next.startedMs = out.occurredMs;
    if (session_.timeMs() < next.startedMs ||
        session_.timeMs() - next.startedMs >= CombatFeedback::durationMs)
        return;
    if (!next.party) {
        // This is the last presented target image, including for a newly dead actor.
        if (state_.targets.world != out.enemy.world || out.enemy.slot >= state_.targets.anchors.size())
            return;
        next.anchor = state_.targets.anchors[out.enemy.slot];
        if (!next.anchor.valid)
            return;
    }
    showFeedback(next);
}
void CommandController::showFeedback(const CombatFeedback& next) noexcept {
    const auto now = session_.timeMs();
    if (now < next.startedMs || now - next.startedMs >= CombatFeedback::durationMs)
        return;
    if (next.party && next.character.index < rules::partySize &&
        next.portrait != PortraitFeedbackKind::None) {
        const bool supportive = next.portrait == PortraitFeedbackKind::Healing ||
                                next.portrait == PortraitFeedbackKind::Protection;
        const unsigned duration = supportive                                       ? 640u
                                  : next.portrait == PortraitFeedbackKind::Negated ? 240u
                                                                                   : 400u;
        state_.portraitFeedback[next.character.index][supportive ? 0 : 1] = {next.portrait, next.element,
                                                                             next.startedMs, duration};
    }
    auto* destination = &state_.feedback[0];
    for (auto& entry : state_.feedback) {
        if (entry.active && entry.party == next.party &&
            (next.party ? entry.character.index == next.character.index
                        : sameActor(entry.enemy, next.enemy))) {
            destination = &entry;
            break;
        }
        if (!entry.active || (destination->active && entry.startedMs < destination->startedMs))
            destination = &entry;
    }
    *destination = next;
    if (next.party && next.hit && !next.healing && next.portrait != PortraitFeedbackKind::Protection &&
        next.damage > 0)
        hitUntil_[next.character.index] = next.startedMs + 150;
}
void CommandController::refreshFeedback(bool clear) noexcept {
    const auto epoch = session_.combat().spellVisuals().epoch;
    clear |= feedbackEpoch_ != epoch;
    feedbackEpoch_ = epoch;
    state_.presentationMs = session_.timeMs();
    state_.supportConditions = session_.effects().visualConditions();
    const auto& support = session_.supportVisuals();
    if (supportEpoch_ != support.epoch) {
        supportSeen_ = 0;
        supportEpoch_ = support.epoch;
    }
    if (clear) {
        state_.feedback = {};
        state_.portraitFeedback = {};
        hitUntil_ = {};
        return;
    }
    const auto now = session_.timeMs();
    state_.presentationMs = now;
    std::array<std::array<const SupportVisual*, 2>, rules::partySize> latest{};
    auto seen = supportSeen_;
    for (const auto& cue : support.records) {
        if (!cue.id.serial || cue.id.world != actors_.snapshot().world || cue.id.serial <= supportSeen_)
            continue;
        seen = std::max(seen, cue.id.serial);
        if (cue.target.npc || cue.target.character.index >= rules::partySize ||
            cue.spell == rules::SpellKind::SpiritualWeapon || cue.phase == SupportPhase::Placement ||
            now < cue.occurredMs || now - cue.occurredMs >= supportDuration(cue.phase))
            continue;
        const auto k = cue.spell;
        const bool harmful = k == rules::SpellKind::Bane || k == rules::SpellKind::Sleep ||
                             k == rules::SpellKind::ColorSpray || k == rules::SpellKind::HoldPerson ||
                             k == rules::SpellKind::Web || k == rules::SpellKind::Grease ||
                             k == rules::SpellKind::RayOfFrost;
        auto& selected = latest[cue.target.character.index][harmful ? 1 : 0];
        if (!selected || selected->id.serial < cue.id.serial)
            selected = &cue;
    }
    supportSeen_ = seen;
    for (unsigned i = 0; i < rules::partySize; ++i)
        for (unsigned channel = 0; channel < 2; ++channel) {
            const auto* cue = latest[i][channel];
            if (!cue)
                continue;
            auto& f = state_.portraitFeedback[i][channel];
            if (f.kind != PortraitFeedbackKind::None && f.startedMs > cue->occurredMs)
                continue;
            f = {channel ? PortraitFeedbackKind::Harmful : PortraitFeedbackKind::Protection,
                 rules::DamageType::Bludgeoning,
                 cue->occurredMs,
                 supportDuration(cue->phase),
                 cue->spell,
                 cue->phase,
                 cue->duplicate};
        }
    for (auto& channels : state_.portraitFeedback)
        for (auto& entry : channels)
            if (entry.kind != PortraitFeedbackKind::None &&
                (now < entry.startedMs || now - entry.startedMs >= entry.durationMs))
                entry = {};
    for (auto& entry : state_.feedback)
        if (entry.active && now - entry.startedMs >= CombatFeedback::durationMs)
            entry.active = false;
}
void CommandController::selectAlly(std::size_t index) noexcept {
    if (!state_.suspended && index < state_.allyCount)
        state_.selection.ally = state_.allies[index];
    updateAvailability();
}
bool CommandController::visible(ActorId id) const noexcept {
    if (state_.targets.world != actors_.snapshot().world ||
        state_.targets.revision != actors_.snapshot().revision)
        return false;
    for (std::size_t i = 0; i < state_.targets.candidateCount; ++i)
        if (sameActor(id, state_.targets.candidates[i]))
            return true;
    return false;
}
void CommandController::refresh(const TargetSnapshot& targets, bool moving, bool mapOpen,
                                bool focused) noexcept {
    state_.worldTransform = shake_.sample(session_.timeMs(), actors_.snapshot().world);
    moving |= session_.opening();
    if (session_.objective().known) {
        if (session_.objective().completed && !objectiveReported_) {
            state_.log.append(session_.fixtures().definition().depth == 3
                                  ? "Final guardians defeated."
                                  : "Guardians defeated. Gate opened; supplies await beyond.");
            objectiveReported_ = true;
        }
        if (session_.objective().capacityFailure && !objectiveFailureReported_) {
            state_.log.append("The gate could not open. Save your progress and restart the game.");
            objectiveFailureReported_ = true;
        }
    }
    if (state_.save.enabled) {
        state_.save.reason = session_.saveSafety();
        state_.save.allowed = !state_.save.reason;
    }
    state_.campaign = session_.campaign();
    state_.gameTime = session_.gameTime();
    state_.campaignTerminal = state_.campaign && session_.campaign()->status() != CampaignStatus::Active;
    state_.campaignVictory = state_.campaign && session_.campaign()->status() == CampaignStatus::Victory;
    state_.locationName = state_.campaign ? session_.campaign()->name() : "";
    state_.fixturePrompt[0] = 0;
    state_.lootHover[0] = 0;
    state_.floor = &session_.fixtures();
    state_.questKeys = session_.party().questKeys();
    if (!moving && !mapOpen && focused && interactionPromptsAvailable()) {
        const auto door = state_.campaign ? session_.campaign()->doorwayRequest() : TravelRequest{};
        const bool doorway =
            state_.campaign && door.kind == TravelKind::Doorway && door.doorway.slot != UINT16_MAX;
        if (doorway) {
            const auto check = session_.campaign()->previewTravel(door);
            if (check)
                std::snprintf(state_.fixturePrompt.data(), state_.fixturePrompt.size(), "[F] %s",
                              session_.campaign()->doorway(door.doorway)->action);
        }
        if (!doorway && nearbyResident()) {
            const auto* a = actors_.actor(*nearbyResident());
            const auto* c = a && dialogueCatalog_ ? dialogueCatalog_->find(a->npc.residentId) : nullptr;
            if (c)
                std::snprintf(state_.fixturePrompt.data(), state_.fixturePrompt.size(), "[F] Talk to %s",
                              c->name.c_str());
        }
        const auto request = session_.nearbyFixture();
        const auto* f = session_.fixtures().find(request.id);
        if (!doorway && !nearbyResident() && f &&
            (!session_.fixtures().definition().discovery || session_.fixtures().known(request.id.slot))) {
            const auto result = session_.interact(request, true);
            const char* action = nullptr;
            if (result) {
                switch (f->kind) {
                case FixtureKind::Door:
                    action = session_.fixtures().open(request.id.slot) ? "Close" : "Open";
                    break;
                case FixtureKind::Key:
                    if (!session_.fixtures().completed(request.id.slot))
                        action = "Take";
                    break;
                case FixtureKind::Upstairs:
                    action = "Ascend";
                    break;
                case FixtureKind::Downstairs:
                    action = state_.campaign && session_.campaign()->activeLocation() == 0 ? "Enter dungeon"
                                                                                           : "Descend";
                    break;
                case FixtureKind::Endpoint:
                case FixtureKind::GuardianGate:
                    break; // No player action; an open gate is only passable scenery.
                }
            }
            if (action)
                std::snprintf(state_.fixturePrompt.data(), state_.fixturePrompt.size(), "[F] %s", action);
        } else if (const auto chest =
                       doorway || nearbyResident() ? ContainerId{} : session_.nearbyContainer()) {
            const auto access = session_.containerAccess(chest);
            const auto* container = session_.loot().findContainer(chest);
            if (access && container)
                std::snprintf(state_.fixturePrompt.data(), state_.fixturePrompt.size(), "[F] %s",
                              container->opened ? "Inspect" : "Open");
        }
    }
    state_.development = session_.development();
    if (state_.camp.open)
        refreshCamp();
    if (state_.travel.open)
        refreshTravel();
    if (!focused) {
        state_.inventory.pointerDown = false;
        state_.inventory.dragging = false;
    }
    {
        const auto& combat = session_.combat().snapshot();
        refreshFeedback(false);
        if (combat.turn != state_.combat.turn || combat.phase != state_.combat.phase) {
            if (!realTime(combat.phase))
                state_.selection.action = PartyAction::Melee;
            state_.selection.target.reset();
            state_.selection.ally.reset();
            state_.selection.targetCount = 0;
            state_.selection.spell = {};
            state_.selection.feature = rules::Feature::None;
            if (state_.magic.state != CastingState::Resolving) {
                setCastingState(CastingState::Normal);
                state_.magic.pending = {};
            }
            if (state_.selection.action == PartyAction::Spell ||
                state_.selection.action == PartyAction::Feature)
                state_.selection.action = PartyAction::Melee;
            if (combat.activeCharacter && combat.turn != state_.combat.turn &&
                combat.phase != CombatPhase::Facing && state_.combat.phase != CombatPhase::Facing) {
                char line[96];
                std::snprintf(line, sizeof(line), "%s%s",
                              party_.character(*combat.activeCharacter)->build.name.c_str(),
                              combat.awaitingAction ? "'s turn." : " waits or resolves their turn.");
                appendLog(line);
                state_.selection.character = *combat.activeCharacter;
                state_.selection.action = PartyAction::Melee;
            }
        }
        if (state_.inventory.itemTargeting && (state_.inventory.itemCommand.turn != combat.turn ||
                                               state_.inventory.itemCommand.world != combat.world))
            state_.inventory.itemTargeting = false;
        state_.combat = combat;
        session_.combat().previewTurns(state_.combat);
        state_.selection.world = combat.world;
        state_.selection.turn = combat.turn;
    }
    suspension_ = !focused  ? CommandReason::Unfocused
                  : moving  ? CommandReason::Moving
                  : mapOpen ? CommandReason::MapOpen
                            : CommandReason::Ready;
    state_.suspended = suspension_ != CommandReason::Ready;
    floorHover(state_.suspended ? std::nullopt : hoveredLoot_);
    TargetSnapshot normalized;
    if (targets.count <= targets.ids.size() && targets.candidateCount <= targets.candidates.size() &&
        targets.world == actors_.snapshot().world && targets.revision == actors_.snapshot().revision) {
        normalized.world = targets.world;
        normalized.revision = targets.revision;
        normalized.anchors = targets.anchors;
        // Normalize membership in stable slot order, ignoring duplicates and invalid IDs.
        std::array<bool, NpcWorld::maxInstances> included{};
        for (std::size_t i = 0; i < targets.count; ++i) {
            const auto* a = actors_.actor(targets.ids[i]);
            if (a && a->hostile() && a->life == ActorLife::Alive)
                included[targets.ids[i].slot] = true;
        }
        for (std::size_t i = 0; i < included.size(); ++i)
            if (included[i]) {
                normalized.ids[normalized.count++] = actors_.id(i);
                normalized.anchors[i] = targets.anchors[i];
            }
    }
    if (normalized.world) {
        std::array<bool, NpcWorld::maxInstances> included{};
        const auto add = [&](ActorId id) {
            const auto* a = actors_.actor(id);
            if (a && a->hostile() && a->life == ActorLife::Alive)
                included[id.slot] = true;
        };
        for (std::size_t i = 0; i < normalized.count; ++i)
            add(normalized.ids[i]);
        for (std::size_t i = 0; i < targets.candidateCount; ++i)
            add(targets.candidates[i]);
        for (std::size_t i = 0; i < included.size(); ++i)
            if (included[i])
                normalized.candidates[normalized.candidateCount++] = actors_.id(i);
        const auto pose = session_.movement().player;
        const auto distance = [&](ActorId id) {
            const auto p = actorPosition(*actors_.actor(id), session_.timeMs());
            const double x = double(p.x) - (pose.cell.x + .5), y = double(p.y) - (pose.cell.y + .5);
            return x * x + y * y;
        };
        std::sort(normalized.candidates.begin(), normalized.candidates.begin() + normalized.candidateCount,
                  [&](ActorId a, ActorId b) {
                      const auto da = distance(a), db = distance(b);
                      return da != db ? da < db : a.slot < b.slot;
                  });
    }
    if (normalized.world != state_.targets.world || normalized.revision != state_.targets.revision ||
        normalized.candidateCount != state_.targets.candidateCount ||
        !std::equal(normalized.candidates.begin(), normalized.candidates.begin() + normalized.candidateCount,
                    state_.targets.candidates.begin(), sameActor))
        ++state_.targetPicker.revision;
    state_.targets = normalized;
    for (auto& feedback : state_.feedback) {
        if (!feedback.active || feedback.party)
            continue;
        if (moving || normalized.world != feedback.enemy.world)
            feedback.anchor.valid = false;
        else if (normalized.anchors[feedback.enemy.slot].valid)
            feedback.anchor = normalized.anchors[feedback.enemy.slot];
        else
            feedback.anchor.valid = false;
    }
    state_.protectedTargets.fill(false);
    state_.conditions.fill(0);
    for (std::size_t i = 0; i < actors_.snapshot().count; ++i)
        for (unsigned k = 0; k < unsigned(EffectKind::Count); ++k)
            if (session_.effects().has(actors_.id(i), EffectKind(k)))
                state_.conditions[i] |= 1u << k;
    for (std::size_t i = 0; i < state_.targets.count; ++i) {
        const auto* target = actors_.actor(state_.targets.ids[i]);
        const auto pose = session_.movement().player;
        state_.protectedTargets[i] = target && inFrontBand(pose.cell, pose.heading, target->npc.cell) &&
                                     !session_.combat().exposed(state_.targets.ids[i]);
    }
    state_.allyCount = 0;
    for (std::size_t i = 0; i < rules::partySize; ++i) {
        const auto id = party_.snapshot().formation[i];
        const auto& member = *party_.character(id);
        auto& card = state_.cards[i];
        card = {};
        card.id = id;
        card.name = member.build.name.c_str();
        card.raceName = party_.catalog().race(member.build.race)->name.c_str();
        card.abilities = member.stats.abilities;
        card.armorClass = member.stats.armorClass;
        card.level = member.build.level;
        card.levelEligible =
            session_.advancementEligibility(session_.advancementRequest(id)) == AdvancementError::None;
        card.portrait =
            member.build.portrait.index < rules::portraitCount ? 64u + member.build.portrait.index : 255u;
        const std::array<EquipmentSlot, 4> equipped{EquipmentSlot::MainHand, EquipmentSlot::OffHand,
                                                    EquipmentSlot::Body, EquipmentSlot::Ranged};
        for (std::size_t e = 0; e < equipped.size(); ++e)
            if (const auto* item = party_.equipped(id, equipped[e]))
                card.equipment[e] = item->name.c_str();
        card.className = party_.catalog().characterClass(member.build.characterClass)->name.c_str();
        card.hp = member.hitPoints;
        card.temporaryHp = member.temporaryHitPoints;
        card.maxHp = member.stats.maxHitPoints;
        card.mana = member.mana;
        card.maxMana = member.maxMana;
        card.hasMana = member.maxMana > 0;
        card.status = member.life == rules::Life::Conscious ? DisplayStatus::Ready
                      : member.life == rules::Life::Stable  ? DisplayStatus::Stable
                      : member.life == rules::Life::Dead    ? DisplayStatus::Dead
                                                            : DisplayStatus::Unconscious;
        card.successes = member.successes;
        card.failures = member.failures;
        {
            card.armorClass = session_.effects().armorClass(id);
            card.concentration = session_.effects().concentrationStatus(id);
            card.effects = session_.effects().statuses(id);
            card.hasDeathSaves = member.life == rules::Life::Dying;
            card.active = realTime(state_.combat.phase)
                              ? state_.selection.character.index == id.index
                              : state_.combat.awaitingAction && state_.combat.activeCharacter &&
                                    state_.combat.activeCharacter->index == id.index;
            card.selecting = realTime(state_.combat.phase);
            card.cooldownMs = state_.combat.cooldownMs[id.index];
            card.spent = state_.combat.spent[id.index] && !realTime(state_.combat.phase);
            card.defending = state_.combat.defending[id.index];
            card.recentlyHit = session_.timeMs() < hitUntil_[id.index];
            for (const auto& shot : session_.combat().attacks().attacks)
                card.incomingShot |= shot.incoming && shot.ranged && shot.character.index == id.index &&
                                     shot.stage == AttackStage::Flight;
            if ((member.life == rules::Life::Dying || session_.effects().has(id, EffectKind::Sleep)) &&
                id.index != state_.selection.character.index)
                state_.allies[state_.allyCount++] = id;
        }
    }
    if (state_.selection.target && (moving || !focused || !visible(*state_.selection.target))) {
        const auto* selected = actors_.actor(*state_.selection.target);
        if ((state_.combat.phase == CombatPhase::Turns && selected && selected->life == ActorLife::Alive))
            appendLog(commandMessage(moving     ? CommandReason::Moving
                                     : !focused ? CommandReason::Unfocused
                                                : CommandReason::NotVisible));
        state_.selection.target.reset();
    }
    if (moving) {
        state_.selection.targetCount = 0;
        if (state_.magic.armed)
            cancel();
    }
    reconcileMagic();
    refreshMagic();
    for (std::size_t i = 0; i < state_.cards.size(); ++i) {
        auto& card = state_.cards[i];
        const auto& m = state_.magic;
        card.castCaster = m.casting() && card.id.index == m.pending.character.index;
        card.castRecipient = m.armed && m.selectedAllies[i];
        card.castRisk = card.castRecipient && m.hostile && m.area;
        card.castEligible = !m.armed || m.hostile || m.pending.action != PartyAction::Spell ||
                            m.allyReasons[i] == CommandReason::Ready;
        card.castHovered =
            m.armed && state_.hover.kind == HoverKind::Portrait && state_.hover.index == int(i);
        card.castPreview = card.castRecipient ? m.name : "";
    }
    refreshInventory();
    refreshTrade();
    refreshLoot();
    refreshRestAvailability();
    updateAvailability();
    updateHover();
}
void CommandController::updateAvailability() noexcept {
    state_.availability = validate(state_.magic.armed ? state_.magic.pending : state_.selection);
    auto preview = state_.selection;
    for (std::size_t i = 0; i < state_.actionAvailability.size(); ++i) {
        preview.action = static_cast<PartyAction>(i);
        state_.actionAvailability[i] = validate(preview);
    }
    refreshTargetPicker();
}
void CommandController::selectCharacter(std::size_t index) noexcept {
    if (state_.selection.action == PartyAction::Aid && index < rules::partySize) {
        state_.selection.ally = party_.snapshot().formation[index];
        updateAvailability();
        return;
    }
    if (state_.magic.armed && index < rules::partySize &&
        state_.selection.character.index != party_.snapshot().formation[index].index)
        cancel();
    if (index < rules::partySize) {
        state_.selection.character = party_.snapshot().formation[index];
        refreshMagic();
    }
    updateAvailability();
}
void CommandController::selectAction(PartyAction action) noexcept {
    if (static_cast<unsigned>(action) >= static_cast<unsigned>(PartyAction::Count))
        return;
    state_.selection.targetCount = 0;
    state_.selection.spell = {};
    state_.selection.feature = rules::Feature::None;
    state_.selection.action = action;
    if (action == PartyAction::Defend || action == PartyAction::Aid)
        state_.selection.target.reset();
    if (action != PartyAction::Aid)
        state_.selection.ally.reset();
    updateAvailability();
}
void CommandController::cycleAction(int delta) noexcept {
    const int count = static_cast<int>(PartyAction::Count);
    selectAction(static_cast<PartyAction>(
        (static_cast<int>(state_.selection.action) + (delta < 0 ? count - 1 : 1)) % count));
}
void CommandController::selectTarget(ActorId target) noexcept {
    if (state_.suspended || !visible(target))
        return;
    state_.selection.target = target;
    refreshTargetPicker();
    for (std::size_t i = 0; i < state_.targetPicker.candidateCount; ++i)
        if (sameActor(target, state_.targetPicker.candidates[i]))
            state_.targetPage = i / 4;
    updateAvailability();
}
void CommandController::cycleTarget(int delta) noexcept {
    if (state_.selection.action == PartyAction::Aid) {
        if (!state_.allyCount)
            return;
        std::size_t next = delta < 0 ? state_.allyCount - 1 : 0;
        if (state_.selection.ally)
            for (std::size_t i = 0; i < state_.allyCount; ++i)
                if (state_.allies[i].index == state_.selection.ally->index)
                    next = (i + (delta < 0 ? state_.allyCount - 1 : 1)) % state_.allyCount;
        selectAlly(next);
        return;
    }
    cycleEnemyTarget(delta);
}
void CommandController::pageTargets(int delta) noexcept {
    refreshTargetPicker();
    if (!state_.targetPicker.available)
        return;
    const auto pages = (state_.targetPicker.candidateCount + 3) / 4;
    if (pages)
        state_.targetPage = (state_.targetPage + (delta < 0 ? pages - 1 : 1)) % pages;
    refreshTargetPicker();
}
void CommandController::scrollLog(int delta) noexcept {
    const auto count = state_.log.lineCount(state_.logExpanded       ? hudLayout::expandedLogColumns
                                            : state_.magic.casting() ? hudLayout::castingLogColumns
                                                                     : hudLayout::logColumns);
    const auto maximum = count ? count - 1 : 0;
    state_.logOffset = static_cast<std::size_t>(std::clamp<std::int64_t>(
        static_cast<std::int64_t>(state_.logOffset) + delta, 0, static_cast<std::int64_t>(maximum)));
}
void CommandController::cancel() noexcept {
    closeTargetPicker();
    state_.inventory.itemTargeting = false;
    state_.selection.targetCount = 0;
    state_.selection.spell = {};
    state_.selection.feature = rules::Feature::None;
    setCastingState(CastingState::Normal);
    state_.magic.pending = {};
    state_.magic.feedback = {};
    state_.selection.action = PartyAction::Melee;
    state_.selection.ally.reset();
    state_.selection.target.reset();
    updateAvailability();
}
void CommandController::movementStarted() noexcept {
    if (state_.selection.target) {
        appendLog("Movement cleared the selected target.");
    }
    cancel();
}
CommandReason CommandController::validate(const PartyCommand& command) const noexcept {
    if (state_.suspended)
        return suspension_;
    if (state_.modal())
        return CommandReason::LogOpen;
    const auto reason = session_.combat().validate(command, session_.movement().player);
    if (reason != CommandReason::Ready)
        return reason;
    return targetVisibility(command, party_.catalog(), state_.targets, actors_.snapshot());
}
CommandReason CommandController::submit(const PartyCommand& command) noexcept {
    const auto reason = validate(command);
    if (reason == CommandReason::Ready) {
        const auto submitted = command;
        const bool magic = submitted.action == PartyAction::Spell || submitted.action == PartyAction::Feature;
        const auto out = session_.submit(command);
        record(session_.events());
        if (out.reason == CommandReason::Resolved) {
            cancel();
            if (magic && session_.combat().casterResolving(submitted.character)) {
                state_.magic.pending = submitted;
                state_.magic.pending.targetCount = 0;
                setCastingState(CastingState::Resolving);
                refreshMagic();
            }
        }
        updateAvailability();
        return out.reason;
    }
    session_.audioEvents().emit({"ui.reject", {}, 0, 0, false, false, true});
    char line[CombatLog::messageBytes];
    const auto* member = party_.character(command.character);
    if (command.target && command.action != PartyAction::Defend)
        std::snprintf(line, sizeof(line), "%s: %s -> monster #%u - %s",
                      member ? member->build.name.c_str() : "?", actionName(command.action),
                      unsigned(command.target->slot) + 1, commandMessage(reason));
    else
        std::snprintf(line, sizeof(line), "%s: %s - %s", member ? member->build.name.c_str() : "?",
                      actionName(command.action), commandMessage(reason));
    appendLog(line);
    updateAvailability();
    return reason;
}
} // namespace blobber
