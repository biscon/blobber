#include "game/commands.hpp"
#include "game/perception.hpp"
#include "ui/layout.hpp"
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
    case CommandReason::SameCellRequired:
        return "Choose enemies in the same cell.";
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
        return "Recovery requires exploration without active threats.";
    case CommandReason::EffectCapacity:
        return "No effect capacity available.";
    case CommandReason::AttackPending:
        return "This attack is still playing.";
    case CommandReason::Cooldown:
        return "Action is recharging.";
    case CommandReason::OutOfRange:
        return "Target is out of range for this attack.";
    case CommandReason::Blocked:
        return "That step is blocked.";
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
        return "Front slot protects target.";
    case CommandReason::RearBlocked:
        return "Your front slot blocks melee.";
    case CommandReason::NoWeapon:
        return "No ready weapon for this action.";
    case CommandReason::InvalidAlly:
        return "Select another dying ally.";
    case CommandReason::LogOpen:
        return "Close the overlay to act.";
    case CommandReason::FeatureRequired:
        return "Requires the Rogue back-row feature.";
    case CommandReason::PreviewAccepted:
        return "Preview only: no costs or combat.";
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
        return "Wait for movement to finish.";
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
CommandController::CommandController(const Party& party, const ActorWorld& actors)
    : party_(party), actors_(actors) {
    state_.selection.character = party.snapshot().formation[0];
    refresh({}, false, false, true);
    appendLog("Choose a character, action and target. Mana is unavailable until the spell slice.");
}
CommandController::CommandController(GameSession& session, const std::vector<NpcTimings>& timings)
    : CommandController(session.party(), session.actors()) {
    if (!timings.empty() && timings.size() != actors_.snapshot().typeCount)
        throw std::invalid_argument("HUD animation/world type counts differ");
    for (const auto& timing : timings)
        validateNpcTimings(timing);
    session_ = &session;
    state_.log = {};
    appendLog("Click an enemy to attack. Shield guards; Aid stabilizes.");
    refresh({}, false, false, true);
}
void CommandController::record(const CombatOutcome& out) noexcept {
    if (out.event == CombatEvent::None ||
        (out.attackId.world && out.attackId.world != actors_.snapshot().world))
        return;
    char line[CombatLog::messageBytes];
    const auto* actor = party_.character(out.character);
    const auto* recipient = party_.character(out.recipient);
    const char* name = actor ? actor->build.name.c_str() : "?";
    const char* target = recipient ? recipient->build.name.c_str() : "?";
    switch (out.event) {
    case CombatEvent::Started:
        std::snprintf(line, sizeof(line), "Combat begins.");
        break;
    case CombatEvent::AttackStarted:
    case CombatEvent::ProjectileReleased:
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
        if (out.enemyAttack) {
            std::snprintf(line, sizeof(line), "Goblin #%u -> %s: %s (d20 %d), %lld damage%s.",
                          unsigned(out.enemy.slot) + 1, target,
                          out.critical ? "critical"
                          : out.hit    ? "hit"
                                       : "miss",
                          out.roll, static_cast<long long>(out.damage), out.died ? ", dead" : "");
        } else
            std::snprintf(line, sizeof(line), "%s -> goblin #%u: %s (d20 %d), %lld damage%s.", name,
                          unsigned(out.enemy.slot) + 1,
                          out.critical ? "critical"
                          : out.hit    ? "hit"
                                       : "miss",
                          out.roll, static_cast<long long>(out.damage), out.died ? ", dead" : "");
        break;
    case CombatEvent::Defend:
        std::snprintf(line, sizeof(line), "%s defends until their next turn.", name);
        break;
    case CombatEvent::Aid:
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
    case CombatEvent::Round:
        std::snprintf(line, sizeof(line), "New round.");
        break;
    case CombatEvent::Spent:
        std::snprintf(line, sizeof(line), "Turn spent: cooldown, NPC surprise or no legal action.");
        break;
    case CombatEvent::Disengaged:
        std::snprintf(line, sizeof(line), "Contact broken. Ranged approach resumes.");
        break;
    case CombatEvent::Cast: {
        const auto* spell = rules::spellRules(out.spell);
        std::snprintf(line, sizeof(line), "%s: %s.", name,
                      spell                                       ? spell->name
                      : out.feature == rules::Feature::SecondWind ? "Second Wind"
                                                                  : "Arcane Recovery");
        break;
    }
    case CombatEvent::Healing:
    case CombatEvent::Recovery: {
        const auto* item = party_.catalog().item(out.item);
        std::snprintf(line, sizeof(line), "%s%s%s -> %s: +%lld %s.", name, item ? " uses " : "",
                      item ? item->name.c_str() : "", target, static_cast<long long>(out.damage),
                      out.event == CombatEvent::Healing ? "HP" : "mana");
        if (out.event == CombatEvent::Healing) {
            CombatFeedback f;
            f.active = f.party = f.hit = f.healing = true;
            f.character = out.recipient;
            f.damage = out.damage;
            f.startedMs = out.occurredMs;
            showFeedback(f);
        }
        break;
    }
    case CombatEvent::SpellSave:
        std::snprintf(line, sizeof(line), "Enemy #%u: %s save %d vs DC %d - %s.",
                      unsigned(out.enemy.slot) + 1, rules::spellRules(out.spell)->name, out.saveTotal,
                      out.saveDifficulty, out.hit ? "saved" : "failed");
        break;
    case CombatEvent::GuidingMark:
        std::snprintf(line, sizeof(line), "Enemy #%u: guiding light %s.", unsigned(out.enemy.slot) + 1,
                      out.hit ? "applied" : "ended");
        break;
    case CombatEvent::Effect: {
        constexpr const char* effects[]{"Bless", "Bane", "Shield of Faith", "Mage Armor", "Hold Person"};
        const auto& e = out.effect;
        const char* effect =
            static_cast<unsigned>(e.kind) < 5 ? effects[static_cast<unsigned>(e.kind)] : "Effect";
        if (e.notice == EffectNotice::InitialSave || e.notice == EffectNotice::RepeatSave ||
            e.notice == EffectNotice::ConcentrationSave)
            std::snprintf(line, sizeof(line), "%s%s: save %lld vs %lld - %s.", effect,
                          e.notice == EffectNotice::ConcentrationSave ? " concentration" : "",
                          static_cast<long long>(e.total), static_cast<long long>(e.difficulty),
                          e.succeeded ? "saved" : "failed");
        else
            std::snprintf(line, sizeof(line), "%s: %s.", effect,
                          e.notice == EffectNotice::Applied ? "applied" : "ended / recipient removed");
        if (e.notice == EffectNotice::Applied && !e.target.npc) {
            constexpr const char* labels[]{"BLESS", "BANE", "FAITH", "ARMOR", "HELD"};
            CombatFeedback f;
            f.active = f.party = f.hit = true;
            f.healing = e.kind != EffectKind::Bane && e.kind != EffectKind::HoldPerson;
            f.character = e.target.character;
            f.startedMs = out.occurredMs;
            f.text = labels[static_cast<unsigned>(e.kind)];
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
        appendLog("Victory. Surviving downed allies are stable; HP is not restored. Continue to explore.");
    if (out.phase == CombatPhase::Defeat)
        appendLog("Defeat: nobody is conscious. Restart begins a fresh session.");
}
void CommandController::recordFeedback(const CombatOutcome& out) noexcept {
    const auto* actor = actors_.actor(out.enemy);
    if (!session_ || !actor || (out.enemyAttack && !party_.character(out.recipient)))
        return;
    CombatFeedback next;
    next.active = true;
    next.party = out.enemyAttack;
    next.hit = out.hit;
    const auto* spell = rules::spellRules(out.spell);
    if (spell && !spell->attack && !out.hit)
        next.text = "SAVED";
    next.character = out.recipient;
    next.enemy = out.enemy;
    next.damage = out.damage;
    next.startedMs = out.occurredMs;
    if (session_->timeMs() < next.startedMs ||
        session_->timeMs() - next.startedMs >= CombatFeedback::durationMs)
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
    if (next.party && next.hit && next.damage > 0)
        hitUntil_[next.character.index] = next.startedMs + 150;
}
void CommandController::refreshFeedback(bool clear) noexcept {
    if (clear) {
        state_.feedback = {};
        hitUntil_ = {};
        return;
    }
    const auto now = session_->timeMs();
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
    for (std::size_t i = 0; i < state_.targets.count; ++i)
        if (sameActor(id, state_.targets.ids[i]))
            return true;
    return false;
}
void CommandController::refresh(const TargetSnapshot& targets, bool moving, bool mapOpen, bool focused,
                                const HudDemo* demo) noexcept {
    state_.production = session_ != nullptr;
    if (state_.camp.open)
        refreshCamp();
    if (!focused) {
        state_.inventory.pointerDown = false;
        state_.inventory.dragging = false;
    }
    if (session_) {
        const auto& combat = session_->combat().snapshot();
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
        session_->combat().previewTurns(state_.combat);
        state_.selection.world = combat.world;
        state_.selection.turn = combat.turn;
    }
    suspension_ = !focused  ? CommandReason::Unfocused
                  : moving  ? CommandReason::Moving
                  : mapOpen ? CommandReason::MapOpen
                            : CommandReason::PreviewAccepted;
    state_.suspended = suspension_ != CommandReason::PreviewAccepted;
    TargetSnapshot normalized;
    if (targets.count <= targets.ids.size() && targets.world == actors_.snapshot().world &&
        targets.revision == actors_.snapshot().revision) {
        normalized.world = targets.world;
        normalized.revision = targets.revision;
        normalized.anchors = targets.anchors;
        // Normalize membership in stable slot order, ignoring duplicates and invalid IDs.
        std::array<bool, NpcWorld::maxInstances> included{};
        for (std::size_t i = 0; i < targets.count; ++i) {
            const auto* a = actors_.actor(targets.ids[i]);
            if (a && a->life == ActorLife::Alive)
                included[targets.ids[i].slot] = true;
        }
        for (std::size_t i = 0; i < included.size(); ++i)
            if (included[i]) {
                normalized.ids[normalized.count++] = actors_.id(i);
                normalized.anchors[i] = targets.anchors[i];
            }
    }
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
    if (session_)
        for (std::size_t i = 0; i < state_.targets.count; ++i) {
            const auto* target = actors_.actor(state_.targets.ids[i]);
            const auto pose = session_->movement().player;
            state_.protectedTargets[i] = target && inFrontBand(pose.cell, pose.heading, target->npc.cell) &&
                                         !session_->combat().exposed(state_.targets.ids[i]);
        }
    state_.demo = !session_ && demo && demo->enabled;
    state_.allyCount = 0;
    state_.demoScenario = state_.demo ? demo->scenario : 0;
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
        constexpr const char* portraits[]{"alden", "brunna", "merrin", "lethiel"};
        for (unsigned art = 0; art < 4; ++art)
            if (member.build.id == portraits[art])
                card.portrait = art;
        const std::array<EquipmentSlot, 4> equipped{EquipmentSlot::MainHand, EquipmentSlot::OffHand,
                                                    EquipmentSlot::Body, EquipmentSlot::Ranged};
        for (std::size_t e = 0; e < equipped.size(); ++e)
            if (const auto* item = party_.equipped(id, equipped[e]))
                card.equipment[e] = item->name.c_str();
        card.className = party_.catalog().characterClass(member.build.characterClass)->name.c_str();
        card.hp = member.hitPoints;
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
        if (session_) {
            card.armorClass = session_->effects().armorClass(id);
            card.concentrating = session_->effects().concentration(id).serial != 0;
            constexpr const char* statusNames[]{"Bless ", "Bane ", "Faith ", "Armor ", "Held "};
            std::size_t used = 0;
            for (unsigned e = 0; e < 5; ++e)
                if (session_->effects().has(id, static_cast<EffectKind>(e)))
                    used += std::snprintf(card.effects.data() + used, card.effects.size() - used, "%s",
                                          statusNames[e]);
            card.hasDeathSaves = member.life == rules::Life::Dying;
            card.active = realTime(state_.combat.phase)
                              ? state_.selection.character.index == id.index
                              : state_.combat.awaitingAction && state_.combat.activeCharacter &&
                                    state_.combat.activeCharacter->index == id.index;
            card.selecting = realTime(state_.combat.phase);
            card.cooldownMs = state_.combat.cooldownMs[id.index];
            card.spent = state_.combat.spent[id.index] && !realTime(state_.combat.phase);
            card.defending = state_.combat.defending[id.index];
            card.recentlyHit = session_->timeMs() < hitUntil_[id.index];
            for (const auto& shot : session_->combat().attacks().attacks)
                card.incomingShot |= shot.incoming && shot.ranged && shot.character.index == id.index &&
                                     shot.stage == AttackStage::Flight;
            if (member.life == rules::Life::Dying && id.index != state_.selection.character.index)
                state_.allies[state_.allyCount++] = id;
        }
        if (state_.demo) {
            const auto& r = demo->resources[id.index];
            card.hp = std::clamp(r.hp, 0, card.maxHp);
            card.mana = std::max(0, r.mana);
            card.maxMana = std::max(card.mana, r.maxMana);
            card.hasMana = true;
            card.status = r.status;
        }
    }
    if (state_.selection.target && (moving || !focused || !visible(*state_.selection.target))) {
        const auto* selected = actors_.actor(*state_.selection.target);
        if (!session_ ||
            (state_.combat.phase == CombatPhase::Turns && selected && selected->life == ActorLife::Alive))
            appendLog(commandMessage(moving     ? CommandReason::Moving
                                     : !focused ? CommandReason::Unfocused
                                                : CommandReason::NotVisible));
        state_.selection.target.reset();
    }
    const auto pages = (state_.targets.count + 3) / 4;
    state_.targetPage = pages ? std::min(state_.targetPage, pages - 1) : 0;
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
        card.castEligible = !m.armed || m.hostile || m.pending.action != PartyAction::Spell ||
                            m.allyReasons[i] == CommandReason::Ready;
        card.castHovered =
            m.armed && state_.hover.kind == HoverKind::Portrait && state_.hover.index == int(i);
        card.castPreview = card.castRecipient ? m.name : "";
    }
    refreshInventory();
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
}
void CommandController::selectCharacter(std::size_t index) noexcept {
    if (session_ && state_.selection.action == PartyAction::Aid && index < rules::partySize) {
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
    const int count = session_ ? static_cast<int>(PartyAction::Count) : 6;
    selectAction(static_cast<PartyAction>(
        (static_cast<int>(state_.selection.action) + (delta < 0 ? count - 1 : 1)) % count));
}
void CommandController::selectTarget(ActorId target) noexcept {
    if (state_.suspended || !visible(target))
        return;
    state_.selection.target = target;
    for (std::size_t i = 0; i < state_.targets.count; ++i)
        if (sameActor(target, state_.targets.ids[i]))
            state_.targetPage = i / 4;
    updateAvailability();
}
void CommandController::cycleTarget(int delta) noexcept {
    if (session_ && state_.selection.action == PartyAction::Aid) {
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
    if (state_.suspended || !state_.targets.count)
        return;
    const auto count = state_.targets.count;
    std::size_t next = delta < 0 ? count - 1 : 0;
    if (state_.selection.target)
        for (std::size_t i = 0; i < count; ++i)
            if (sameActor(*state_.selection.target, state_.targets.ids[i]))
                next = (i + (delta < 0 ? count - 1 : 1)) % count;
    selectTarget(state_.targets.ids[next]);
}
void CommandController::pageTargets(int delta) noexcept {
    if (state_.suspended)
        return;
    const auto pages = (state_.targets.count + 3) / 4;
    if (pages)
        state_.targetPage = (state_.targetPage + (delta < 0 ? pages - 1 : 1)) % pages;
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
    if (session_) {
        if (state_.modal())
            return CommandReason::LogOpen;
        const auto reason = session_->combat().validate(command, session_->movement().player);
        if (reason != CommandReason::Ready)
            return reason;
        if ((command.action == PartyAction::Melee || command.action == PartyAction::Ranged) &&
            command.target && !visible(*command.target))
            return CommandReason::NotVisible;
        if (command.action == PartyAction::Spell) {
            const auto* spell = party_.catalog().spell(command.spell);
            const auto* r = spell ? rules::spellRules(spell->kind) : nullptr;
            if (r && r->hostile && !r->area)
                for (std::size_t i = 0; i < command.targetCount; ++i)
                    if (!visible(command.enemies[i]))
                        return CommandReason::NotVisible;
        }
        return CommandReason::Ready;
    }
    if (!party_.character(command.character))
        return CommandReason::InvalidCharacter;
    if (static_cast<unsigned>(command.action) >= static_cast<unsigned>(PartyAction::Count))
        return CommandReason::InvalidAction;
    bool back = false;
    for (std::size_t i = 0; i < rules::partySize; ++i)
        if (state_.cards[i].id.index == command.character.index) {
            if (state_.cards[i].status != DisplayStatus::Ready)
                return CommandReason::CannotAct;
        }
    for (std::size_t i = 2; i < rules::partySize; ++i)
        if (party_.snapshot().formation[i].index == command.character.index)
            back = true;
    if (command.action >= PartyAction::Ranged)
        return CommandReason::Unavailable;
    if (command.action == PartyAction::BackMelee && !back)
        return CommandReason::BackRowOnly;
    if (command.action == PartyAction::Defend)
        return CommandReason::PreviewAccepted;
    if (!command.target)
        return CommandReason::MissingTarget;
    const auto* actor = actors_.actor(*command.target);
    if (!actor || actor->life != ActorLife::Alive)
        return CommandReason::InvalidTarget;
    return visible(*command.target) ? CommandReason::PreviewAccepted : CommandReason::NotVisible;
}
CommandReason CommandController::submit(const PartyCommand& command) noexcept {
    const auto reason = validate(command);
    if (session_ && reason == CommandReason::Ready) {
        const auto submitted = command;
        const bool magic = submitted.action == PartyAction::Spell || submitted.action == PartyAction::Feature;
        const auto out = session_->submit(command);
        record(session_->events());
        if (out.reason == CommandReason::Resolved) {
            cancel();
            if (magic && session_->combat().casterResolving(submitted.character)) {
                state_.magic.pending = submitted;
                state_.magic.pending.targetCount = 0;
                setCastingState(CastingState::Resolving);
                refreshMagic();
            }
        }
        updateAvailability();
        return out.reason;
    }
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
